// Copyright (c) 2019 Nader G. Zeid
//
// This file is part of Memorypa.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Memorypa. If not, see <https://www.gnu.org/licenses/gpl.html>.

#include "memorypa.h"

static void *(*memorypa_given_malloc)(size_t) = NULL;
static void *(*memorypa_given_realloc)(void*,size_t) = NULL;
static void (*memorypa_given_free)(void*) = NULL;
static unsigned char memorypa_initializing = 0;
static unsigned char memorypa_initialized = 0;
static unsigned char memorypa_initializer_thread_id_lock = 0;
static size_t memorypa_initializer_thread_id = 0;
static unsigned char memorypa_initializer_slab[MEMORYPA_INITIALIZER_SLAB_SIZE];
static size_t memorypa_initializer_slab_index = 0;

static size_t memorypa_u_char_size = 0;
static size_t memorypa_u_char_bit_size = 0;
static size_t memorypa_size_t_size = 0;
static size_t memorypa_size_t_bit_size = 0;
static size_t memorypa_size_t_half_bit_size = 0;
static size_t memorypa_size_t_half_bit_size_next_power = 0;
static size_t memorypa_u_char_p_size = 0;

static size_t memorypa_2st = 0;
static size_t memorypa_1uc_4st_1ucp = 0;
static size_t memorypa_1ucp_2uc = 0;
static size_t memorypa_1uc_1st = 0;
static size_t memorypa_1uc_2st = 0;
static size_t memorypa_1uc_3st = 0;
static size_t memorypa_1uc_4st = 0;
static size_t memorypa_2uc = 0;
static size_t memorypa_1ucp_1uc = 0;
static size_t memorypa_1st_2uc = 0;
static size_t memorypa_1st_1uc = 0;

static size_t memorypa_mhash_first_magic = 0;
static size_t memorypa_mhash_second_magic = 0;
static size_t memorypa_everything_size = 0;
static unsigned char *memorypa_everything = NULL;
static size_t memorypa_profile_list_size = 0;
static unsigned char *memorypa_profile_list = NULL;
static size_t memorypa_pool_list_size = 0;
static unsigned char **memorypa_pool_list = NULL;
static unsigned char memorypa_profile_lock = 0;

static inline size_t memorypa_own_get_thread_id() {
  #ifdef _MSC_VER
  return GetCurrentThreadId();
  #else
  return syscall(SYS_gettid);
  #endif
}

static inline unsigned char memorypa_lock_test_set(unsigned char *operand) {
  #ifdef _MSC_VER
  return _InterlockedOr8((char *)operand, 1);
  #else
  return __atomic_fetch_or(operand, 1, __ATOMIC_ACQUIRE);
  #endif
}

static inline void memorypa_unlock_clear(unsigned char *operand) {
  #ifdef _MSC_VER
  _InterlockedAnd8((char *)operand, 0);
  #else
  __atomic_clear(operand, __ATOMIC_RELEASE);
  #endif
}

static inline unsigned char memorypa_lock_load(unsigned char *operand) {
  #ifdef _MSC_VER
  return _InterlockedOr8((char *)operand, 0);
  #else
  return __atomic_load_n(operand, __ATOMIC_ACQUIRE);
  #endif
}

static inline void memorypa_lock(unsigned char *lock) {
  while(memorypa_lock_test_set(lock));
}

static inline void memorypa_unlock(unsigned char *lock) {
  memorypa_unlock_clear(lock);
}

/*
  Returns the position of the most significant bit of the given
  number. The minimum is 1 and the maximum is some small power of 2,
  meaning that the result numbers bits from 1 (least significant) up to
  said power of 2 (most significant). The power of 2 in question must be
  greater than or equal to the number of bits in the current machine
  word. E.g. if you're dealing with a wacky architecture with 28-bit
  words, adjust the maximum of this function to 32 bits by setting the
  variable "memorypa_size_t_half_bit_size_next_power" to 16. This
  particular bisection algorithm requires a proper power of 2 to split
  the machine word when searching for set bits. Anything else and the
  bisection will fail with truncation errors.
*/
static inline size_t memorypa_own_msb(size_t value) {
  size_t output = 1;
  size_t current_split = memorypa_size_t_half_bit_size_next_power;
  do {
    if(value >> current_split) {
      output += current_split;
      value >>= current_split;
    }
    current_split >>= 1;
  }
  while(current_split);
  return output;
}

static inline size_t memorypa_adjust_msb(size_t value, size_t msb, size_t padding) {
  if(padding) {
    if(value <= padding) {
      return msb - 1;
    }
    size_t adjusted_msb = msb - 1;
    size_t bit_check = value - padding;
    bit_check >>= adjusted_msb;
    return adjusted_msb + bit_check;
  }
  return msb;
}

static inline void memorypa_profile_increment(size_t index) {
  unsigned char *block = memorypa_profile_list + (index * memorypa_2st);
  size_t count = ++(*((size_t *)block));
  block += memorypa_size_t_size;
  if(count > *((size_t *)block)) {
    *((size_t *)block) = count;
  }
}

static inline void memorypa_profile_decrement(size_t index) {
  --(*((size_t *)(memorypa_profile_list + (index * memorypa_2st))));
}

static inline size_t memorypa_profile_get_count(size_t index) {
  return *((size_t *)(memorypa_profile_list + (index * memorypa_2st)));
}

static inline size_t memorypa_profile_get_max(size_t index) {
  return *((size_t *)(memorypa_profile_list + (index * memorypa_2st) + memorypa_size_t_size));
}

/*
  unsigned char lock
  size_t block_size
  size_t block_padding
  size_t block_amount
  size_t free_blocks
  unsigned char *block_list
  unsigned char *free_block_list[block_amount]
  {unsigned char *pool, unsigned char terminator[2], unsigned char data[block_size]} blocks[block_amount]
*/
static inline size_t memorypa_pool_get_block_list_offset(size_t block_amount) {
  return memorypa_1uc_4st_1ucp + (memorypa_u_char_p_size * block_amount);
}

static inline size_t memorypa_pool_get_total_size(size_t block_size, size_t block_amount) {
  return memorypa_pool_get_block_list_offset(block_amount) + ((memorypa_1ucp_2uc + block_size) * block_amount);
}

static inline void memorypa_pool_set_lock(unsigned char *pool) {
  memorypa_unlock_clear(pool);
}

static inline unsigned char memorypa_pool_lock_load(unsigned char *pool) {
  return memorypa_lock_load(pool);
}

static inline void memorypa_pool_lock(unsigned char *pool) {
  memorypa_lock(pool);
}

static inline void memorypa_pool_unlock(unsigned char *pool) {
  memorypa_unlock(pool);
}

static inline void memorypa_pool_set_block_size(unsigned char *pool, size_t size) {
  *((size_t *)(pool + memorypa_u_char_size)) = size;
}

static inline size_t memorypa_pool_get_block_size(unsigned char *pool) {
  return *((size_t *)(pool + memorypa_u_char_size));
}

static inline void memorypa_pool_set_block_padding(unsigned char *pool, size_t padding) {
  *((size_t *)(pool + memorypa_1uc_1st)) = padding;
}

static inline size_t memorypa_pool_get_block_padding(unsigned char *pool) {
  return *((size_t *)(pool + memorypa_1uc_1st));
}

static inline void memorypa_pool_set_block_amount(unsigned char *pool, size_t amount) {
  *((size_t *)(pool + memorypa_1uc_2st)) = amount;
}

static inline size_t memorypa_pool_get_block_amount(unsigned char *pool) {
  return *((size_t *)(pool + memorypa_1uc_2st));
}

static inline void memorypa_pool_set_free_blocks(unsigned char *pool, size_t free_blocks) {
  *((size_t *)(pool + memorypa_1uc_3st)) = free_blocks;
}

static inline size_t memorypa_pool_get_free_blocks(unsigned char *pool) {
  return *((size_t *)(pool + memorypa_1uc_3st));
}

static inline void memorypa_pool_set_block_list(unsigned char *pool, size_t block_amount) {
  *((unsigned char **)(pool + memorypa_1uc_4st)) = pool + memorypa_pool_get_block_list_offset(block_amount);
}

static inline unsigned char * memorypa_pool_get_block_list(unsigned char *pool) {
  return *((unsigned char **)(pool + memorypa_1uc_4st));
}

static inline unsigned char * memorypa_pool_get_free_block_list(unsigned char *pool) {
  return pool + memorypa_1uc_4st_1ucp;
}

static inline unsigned char * memorypa_pool_free_block_list_at(unsigned char *free_block_list, size_t index) {
  return free_block_list + (memorypa_u_char_p_size * index);
}

static inline void memorypa_pool_free_block_set_block(unsigned char *free_block, unsigned char *block) {
  *((unsigned char **)free_block) = block;
}

static inline unsigned char * memorypa_pool_free_block_get_block(unsigned char *free_block) {
  return *((unsigned char **)free_block);
}

static inline unsigned char * memorypa_pool_block_list_at(unsigned char *block_list, size_t block_size, size_t index) {
  return block_list + ((memorypa_1ucp_2uc + block_size) * index);
}

static inline void memorypa_pool_block_set_pool(unsigned char *block, unsigned char *pool) {
  *((unsigned char **)block) = pool;
}

static inline unsigned char * memorypa_pool_block_get_pool(unsigned char *block) {
  return *((unsigned char **)block);
}

static inline void memorypa_pool_block_set_terminator(unsigned char *block) {
  block += memorypa_u_char_p_size;
  *block = 0;
  block += memorypa_u_char_size;
  *block = 0;
}

static inline unsigned short memorypa_pool_block_get_terminator(unsigned char *block) {
  block += memorypa_u_char_p_size;
  unsigned short output = *block;
  output <<= memorypa_u_char_bit_size;
  block += memorypa_u_char_size;
  output += *block;
  return output;
}

static inline unsigned char * memorypa_pool_block_get_data(unsigned char *block) {
  return block + memorypa_1ucp_2uc;
}

/*
  We have an unsigned short (only two bytes on normal computers) for
  storing the alignment offset. Because I'm scared of endianness, this
  function intentionally shifts and stores each byte in explicit
  order. This guarantees that the terminator short is always zero when
  the offset is small.
*/
static inline void memorypa_pool_block_set_data_offset(unsigned char *data, unsigned short offset) {
  data -= memorypa_2uc;
  *data = (unsigned char)(offset >> memorypa_u_char_bit_size);
  data += memorypa_u_char_size;
  *data = (unsigned char)offset;
}

static inline unsigned char * memorypa_pool_block_get_block_from_data(unsigned char *data) {
  data -= memorypa_2uc;
  unsigned short offset = *data;
  offset <<= memorypa_u_char_bit_size;
  data += memorypa_u_char_size;
  offset += *data;
  // "1uc" instead of "2uc" because of the current value of "data":
  data -= memorypa_1ucp_1uc;
  data -= offset;
  return data;
}

static inline void memorypa_profile_real_set_size(unsigned char *real, size_t size) {
  *((size_t *)real) = size;
}

static inline size_t memorypa_profile_real_get_size(unsigned char *real) {
  return *((size_t *)real);
}

static inline void memorypa_profile_real_set_terminator(unsigned char *real) {
  real += memorypa_size_t_size;
  *real = 0;
  real += memorypa_u_char_size;
  *real = 0;
}

static inline unsigned short memorypa_profile_real_get_terminator(unsigned char *real) {
  real += memorypa_size_t_size;
  unsigned short output = *real;
  output <<= memorypa_u_char_bit_size;
  real += memorypa_u_char_size;
  output += *real;
  return output;
}

static inline unsigned char * memorypa_profile_real_get_data(unsigned char *real) {
  return real + memorypa_1st_2uc;
}

static inline unsigned char * memorypa_profile_get_real_from_data(unsigned char *data) {
  data -= memorypa_2uc;
  unsigned short offset = *data;
  offset <<= memorypa_u_char_bit_size;
  data += memorypa_u_char_size;
  offset += *data;
  // "1uc" instead of "2uc" because of the current value of "data":
  data -= memorypa_1st_1uc;
  data -= offset;
  return data;
}

static inline unsigned char * memorypa_profile_allocate(size_t size) {
  unsigned char *data = memorypa_given_malloc(memorypa_1st_2uc + size);
  if(data != NULL) {
    size_t power = memorypa_own_msb(size);
    unsigned char *pool = memorypa_pool_list[power - 1];
    if(pool != NULL) {
      power = memorypa_adjust_msb(size, power, memorypa_pool_get_block_padding(pool));
    }
    memorypa_profile_real_set_size(data, size);
    memorypa_profile_real_set_terminator(data);
    memorypa_lock(&memorypa_profile_lock);
    memorypa_profile_increment(power);
    memorypa_unlock(&memorypa_profile_lock);
    data = memorypa_profile_real_get_data(data);
  }
  return data;
}

static inline void memorypa_profile_deallocate_real(unsigned char *real) {
  memorypa_lock(&memorypa_profile_lock);
  size_t size = memorypa_profile_real_get_size(real);
  size_t power = memorypa_own_msb(size);
  unsigned char *pool = memorypa_pool_list[power - 1];
  if(pool != NULL) {
    power = memorypa_adjust_msb(size, power, memorypa_pool_get_block_padding(pool));
  }
  memorypa_profile_decrement(power);
  memorypa_unlock(&memorypa_profile_lock);
  memorypa_given_free(real);
}

static inline unsigned char memorypa_pool_block_is_invalid(unsigned char *block, unsigned char *assigned_pool) {
  if(memorypa_pool_block_get_pool(block) != assigned_pool) {
    memorypa_write_message("memorypa: Block ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_hex((size_t)block, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" of pool ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_hex((size_t)assigned_pool, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" has the incorrect pool!\n", MEMORYPA_WRITE_OPTION_STDERR);
    return 1;
  }
  if(memorypa_pool_block_get_terminator(block)) {
    memorypa_write_message("memorypa: Block ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_hex((size_t)block, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" of pool ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_hex((size_t)assigned_pool, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" has an invalid terminator!\n", MEMORYPA_WRITE_OPTION_STDERR);
    return 2;
  }
  return 0;
}

static inline unsigned char memorypa_pool_free_block_is_invalid(unsigned char *free_block, unsigned char *assigned_pool, size_t *free_blocks) {
  unsigned char *block = memorypa_pool_free_block_get_block(free_block);
  if(block != NULL) {
    ++(*free_blocks);
    return memorypa_pool_block_is_invalid(block, assigned_pool);
  }
  return 0;
}

static inline unsigned char memorypa_pool_is_invalid(unsigned char *pool, size_t *output_size) {
  if(memorypa_pool_lock_load(pool) > 1) {
    memorypa_write_message("memorypa: Pool ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_hex((size_t)pool, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" has invalid lock value!\n", MEMORYPA_WRITE_OPTION_STDERR);
    return 1;
  }
  memorypa_pool_lock(pool);
  size_t block_size = memorypa_pool_get_block_size(pool);
  size_t block_padding = memorypa_pool_get_block_padding(pool);
  size_t power_of_two = block_size - block_padding + 1;
  if(power_of_two & (power_of_two - 1)) {
    memorypa_write_message("memorypa: Pool ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_hex((size_t)pool, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" has invalid block size (", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(block_size, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(") and padding (", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(block_padding, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(")!\n", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_pool_unlock(pool);
    return 2;
  }
  size_t block_amount = memorypa_pool_get_block_amount(pool);
  *output_size = memorypa_pool_get_total_size(block_size, block_amount);
  size_t free_blocks = memorypa_pool_get_free_blocks(pool);
  size_t counted_free_blocks = 0;
  unsigned char *current_list = memorypa_pool_get_free_block_list(pool);
  size_t i = 0;
  while(i < block_amount) {
    if(memorypa_pool_free_block_is_invalid(memorypa_pool_free_block_list_at(current_list, i), pool, &counted_free_blocks)) {
      memorypa_pool_unlock(pool);
      return 3;
    }
    ++i;
  }
  if(counted_free_blocks != free_blocks) {
    memorypa_write_message("memorypa: Pool ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_hex((size_t)pool, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" has invalid free block amount!\n", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_pool_unlock(pool);
    return 4;
  }
  current_list = memorypa_pool_get_block_list(pool);
  i = 0;
  while(i < block_amount) {
    if(memorypa_pool_block_is_invalid(memorypa_pool_block_list_at(current_list, block_size, i), pool)) {
      memorypa_pool_unlock(pool);
      return 5;
    }
    ++i;
  }
  memorypa_pool_unlock(pool);
  return 0;
}

static inline void memorypa_pool_initialize(unsigned char *pool, size_t block_size, size_t block_padding, size_t block_amount) {
  memorypa_pool_set_lock(pool);
  memorypa_pool_set_block_size(pool, block_size);
  memorypa_pool_set_block_padding(pool, block_padding);
  memorypa_pool_set_block_amount(pool, block_amount);
  memorypa_pool_set_free_blocks(pool, block_amount);
  memorypa_pool_set_block_list(pool, block_amount);
  unsigned char *block_list = memorypa_pool_get_block_list(pool);
  unsigned char *free_block_list = memorypa_pool_get_free_block_list(pool);
  unsigned char *current_block;
  size_t i = 0;
  while(i < block_amount) {
    current_block = memorypa_pool_block_list_at(block_list, block_size, i);
    memorypa_pool_block_set_pool(current_block, pool);
    memorypa_pool_block_set_terminator(current_block);
    memorypa_pool_free_block_set_block(
      memorypa_pool_free_block_list_at(free_block_list, i),
      current_block
    );
    ++i;
  }
}

static inline void memorypa_pools_initialize(memorypa_pool_options *sets_of_options) {
  // Include the profile list's size:
  memorypa_everything_size = memorypa_profile_list_size;
  // Include the pool list's size:
  memorypa_everything_size += memorypa_pool_list_size;
  /*
    Calculate the total size then allocate. Note that the whole strategy
    of this library is to divide memory allocation purely along powers of
    2, and to satisfy the requested size using its most significant
    bit. E.g. if someone requests 124 bytes, its MSB ("memorypa_own_msb"
    above) is 7 so a block from "memorypa_pool_list[7]" will be quickly
    returned automatically. Note that it's impossible for a number greater
    than 127 to have an MSB of 7, so the block size of the pool at
    "memorypa_pool_list[7]" is 127, not 128 as one might expect.
  */
  size_t i = 0;
  size_t j = 0;
  size_t sets_of_options_size = 0;
  while(i < memorypa_size_t_bit_size) {
    j = i + 1;
    if(sets_of_options[i].power) {
      ++sets_of_options_size;
      sets_of_options[i].own_block_size = (memorypa_one << sets_of_options[i].power) - 1 + sets_of_options[i].padding;
      sets_of_options[i].own_size = memorypa_pool_get_total_size(sets_of_options[i].own_block_size, sets_of_options[i].amount);
      sets_of_options[i].own_relative_position = memorypa_everything_size;
      memorypa_everything_size += sets_of_options[i].own_size;
      if(j < memorypa_size_t_bit_size && sets_of_options[j].power && sets_of_options[i].power >= sets_of_options[j].power) {
        memorypa_write_message("memorypa: Invalid options! Specified powers must be unique and in ascending order!\n", MEMORYPA_WRITE_OPTION_STDERR);
        exit(EXIT_FAILURE);
      }
    }
    else {
      break;
    }
    i = j;
  }
  memorypa_everything = memorypa_given_malloc(memorypa_everything_size);
  if(memorypa_everything == NULL) {
    memorypa_write_message("memorypa: Cannot initialize any pools because the given \"malloc\" returned null!\n", MEMORYPA_WRITE_OPTION_STDERR);
    exit(EXIT_FAILURE);
  }
  memset(memorypa_everything, 0, memorypa_everything_size);
  // Merely set the profile list (it's already zeroed out):
  memorypa_profile_list = memorypa_everything;
  // Prepare the pool list:
  memorypa_pool_list = (unsigned char **)(memorypa_everything + memorypa_profile_list_size);
  /*
    Each index in "memory_pool_list" represents the power of 2 of the
    block size of its pool, as shown above. If the user does not provide a
    pool for a given power, then the next highest available power is
    used. If the user did not provide a pool with higher power for a given
    index, then it's left as null. This pattern allows the user to
    minimize the number of pool options and allocate only as many pools as
    needed.
  */
  i = 0;
  j = 0;
  while(i < memorypa_size_t_bit_size) {
    if(j < sets_of_options_size) {
      if(i > sets_of_options[j].power) {
        while(++j < sets_of_options_size) {
          if(i <= sets_of_options[j].power) {
            memorypa_pool_list[i] = memorypa_everything + sets_of_options[j].own_relative_position;
            break;
          }
        }
      }
      else {
        memorypa_pool_list[i] = memorypa_everything + sets_of_options[j].own_relative_position;
      }
    }
    else {
      memorypa_pool_list[i] = NULL;
    }
    ++i;
  }
  // Initialize each pool:
  i = 0;
  while(i < sets_of_options_size) {
    memorypa_pool_initialize(memorypa_everything + sets_of_options[i].own_relative_position, sets_of_options[i].own_block_size, sets_of_options[i].padding, sets_of_options[i].amount);
    ++i;
  }
}

static inline unsigned char * memorypa_pool_allocate(unsigned char *pool) {
  unsigned char *output = NULL;
  memorypa_pool_lock(pool);
  size_t free_blocks = memorypa_pool_get_free_blocks(pool);
  if(free_blocks) {
    memorypa_pool_set_free_blocks(pool, --free_blocks);
    unsigned char *free_block = memorypa_pool_get_free_block_list(pool);
    free_block = memorypa_pool_free_block_list_at(free_block, free_blocks);
    output = memorypa_pool_free_block_get_block(free_block);
    memorypa_pool_free_block_set_block(free_block, NULL);
  }
  memorypa_pool_unlock(pool);
  return output;
}

static inline unsigned char * memorypa_rescue_allocate_for_data(size_t size) {
  unsigned char *output = memorypa_given_malloc(memorypa_1ucp_2uc + size);
  if(output != NULL) {
    memorypa_pool_block_set_pool(output, NULL);
    memorypa_pool_block_set_terminator(output);
    output = memorypa_pool_block_get_data(output);
  }
  return output;
}

static inline unsigned char * memorypa_rescue_reallocate_for_default_data(unsigned char *block, size_t new_size) {
  unsigned char *output = memorypa_given_realloc(block, memorypa_1ucp_2uc + new_size);
  if(output != NULL) {
    output = memorypa_pool_block_get_data(output);
  }
  return output;
}

static inline unsigned char * memorypa_rescue_reallocate_for_data(unsigned char *block, size_t new_size, size_t offset) {
  unsigned char *output = memorypa_given_realloc(block, memorypa_1ucp_2uc + new_size + offset);
  if(output != NULL) {
    output = memorypa_pool_block_get_data(output) + offset;
  }
  return output;
}

static inline void memorypa_pool_deallocate(unsigned char *block) {
  unsigned char *pool = memorypa_pool_block_get_pool(block);
  if(pool == NULL) {
    memorypa_given_free(block);
  }
  else {
    memorypa_pool_lock(pool);
    size_t free_blocks = memorypa_pool_get_free_blocks(pool);
    unsigned char *free_block = memorypa_pool_get_free_block_list(pool);
    free_block = memorypa_pool_free_block_list_at(free_block, free_blocks);
    memorypa_pool_free_block_set_block(free_block, block);
    memorypa_pool_set_free_blocks(pool, ++free_blocks);
    memorypa_pool_unlock(pool);
  }
}

static inline unsigned char * memorypa_own_malloc(size_t size) {
  unsigned char *output = NULL;
  size_t power = memorypa_own_msb(size);
  unsigned char *pool = memorypa_pool_list[power - 1];
  if(pool != NULL) {
    power = memorypa_adjust_msb(size, power, memorypa_pool_get_block_padding(pool));
  }
  pool = memorypa_pool_list[power];
  if(pool != NULL) {
    if((output = memorypa_pool_allocate(pool)) != NULL) {
      output = memorypa_pool_block_get_data(output);
    }
    else {
      output = memorypa_rescue_allocate_for_data(size);
      memorypa_write_message("memorypa: Pool #", MEMORYPA_WRITE_OPTION_STDERR);
      memorypa_write_decimal(power, 0, MEMORYPA_WRITE_OPTION_STDERR);
      memorypa_write_message(" is out of memory for size ", MEMORYPA_WRITE_OPTION_STDERR);
      memorypa_write_decimal(size, 0, MEMORYPA_WRITE_OPTION_STDERR);
      memorypa_write_message("! (1)\n", MEMORYPA_WRITE_OPTION_STDERR);
    }
  }
  else {
    output = memorypa_rescue_allocate_for_data(size);
    memorypa_write_message("memorypa: Pool #", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(power, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" has not been initialized for size ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(size, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message("! (2)\n", MEMORYPA_WRITE_OPTION_STDERR);
  }
  return output;
}

static inline unsigned char * memorypa_own_aligned_malloc(size_t size, unsigned short alignment) {
  unsigned char *data = memorypa_own_malloc(size + alignment - 1);
  if(data != NULL) {
    size_t offset = (size_t)data & (alignment - 1);
    if(offset) {
      offset = alignment - offset;
      data += offset;
      memorypa_pool_block_set_data_offset(data, (unsigned short)offset);
    }
  }
  return data;
}

static inline unsigned char * memorypa_own_calloc(size_t amount, size_t unit_size) {
  size_t size = amount * unit_size;
  unsigned char *data = memorypa_own_malloc(size);
  if(data != NULL) {
    memset(data, 0, size);
  }
  return data;
}

static inline unsigned char * memorypa_own_aligned_calloc(size_t amount, size_t unit_size, unsigned short alignment) {
  size_t size = amount * unit_size;
  unsigned char *data = memorypa_own_aligned_malloc(size, alignment);
  if(data != NULL) {
    memset(data, 0, size);
  }
  return data;
}

static inline void memorypa_own_free(unsigned char *data) {
  // The spec allows "NULL" to be passed without failure:
  if(data != NULL) {
    memorypa_pool_deallocate(memorypa_pool_block_get_block_from_data(data));
  }
}

static inline unsigned char * memorypa_own_realloc(unsigned char *data, size_t new_size) {
  // Yes, the spec allows this:
  if(data == NULL) {
    return memorypa_own_malloc(new_size);
  }
  //
  unsigned char *block = memorypa_pool_block_get_block_from_data(data);
  unsigned char *pool = memorypa_pool_block_get_pool(block);
  unsigned char *default_data = memorypa_pool_block_get_data(block);
  if(pool == NULL) {
    memorypa_write_message("memorypa: This is a rescued allocation! Using the given \"realloc\". (3)\n", MEMORYPA_WRITE_OPTION_STDERR);
    return memorypa_rescue_reallocate_for_data(block, new_size, data - default_data);
  }
  size_t power = memorypa_own_msb(new_size);
  unsigned char *new_pool = memorypa_pool_list[power - 1];
  if(new_pool != NULL) {
    power = memorypa_adjust_msb(new_size, power, memorypa_pool_get_block_padding(new_pool));
  }
  new_pool = memorypa_pool_list[power];
  if(new_pool == NULL) {
    unsigned char *new_data = memorypa_rescue_allocate_for_data(new_size);
    if(new_data != NULL) {
      size_t offset_size = memorypa_pool_get_block_size(pool) - (data - default_data);
      memcpy(new_data, data, offset_size < new_size ? offset_size : new_size);
      memorypa_pool_deallocate(block);
    }
    memorypa_write_message("memorypa: Pool #", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(power, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" has not been initialized for size ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(new_size, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message("! (4)\n", MEMORYPA_WRITE_OPTION_STDERR);
    return new_data;
  }
  if(pool == new_pool) {
    if(default_data != data) {
      size_t offset_size = memorypa_pool_get_block_size(pool) - (data - default_data);
      memmove(default_data, data, offset_size < new_size ? offset_size : new_size);
    }
    return default_data;
  }
  unsigned char *new_data = memorypa_pool_allocate(new_pool);
  if(new_data == NULL) {
    new_data = memorypa_rescue_allocate_for_data(new_size);
    if(new_data != NULL) {
      size_t offset_size = memorypa_pool_get_block_size(pool) - (data - default_data);
      memcpy(new_data, data, offset_size < new_size ? offset_size : new_size);
      memorypa_pool_deallocate(block);
    }
    memorypa_write_message("memorypa: Pool #", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(power, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" is out of memory for size ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(new_size, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message("! (5)\n", MEMORYPA_WRITE_OPTION_STDERR);
  }
  else {
    new_data = memorypa_pool_block_get_data(new_data);
    size_t offset_size = memorypa_pool_get_block_size(pool) - (data - default_data);
    memcpy(new_data, data, offset_size < new_size ? offset_size : new_size);
    memorypa_pool_deallocate(block);
  }
  return new_data;
}

static inline unsigned char * memorypa_own_aligned_realloc(unsigned char *data, size_t new_size, unsigned short alignment) {
  // Yes, the spec allows this:
  if(data == NULL) {
    return memorypa_own_aligned_malloc(new_size, alignment);
  }
  //
  new_size += alignment - 1;
  unsigned char *block = memorypa_pool_block_get_block_from_data(data);
  unsigned char *pool = memorypa_pool_block_get_pool(block);
  unsigned char *default_data = memorypa_pool_block_get_data(block);
  if(pool == NULL) {
    size_t offset = data - default_data;
    default_data = memorypa_rescue_reallocate_for_default_data(block, new_size + offset);
    if(default_data == NULL) {
      return default_data;
    }
    data = default_data + offset;
    size_t new_offset = (size_t)default_data & (alignment - 1);
    if(new_offset) {
      new_offset = alignment - new_offset;
    }
    unsigned char *new_data = default_data + new_offset;
    if(new_data != data) {
      new_size -= new_offset;
      memmove(new_data, data, new_size);
      memorypa_pool_block_set_data_offset(new_data, (unsigned short)new_offset);
    }
    memorypa_write_message("memorypa: This is a rescued allocation! Using the given \"realloc\". (6)\n", MEMORYPA_WRITE_OPTION_STDERR);
    return new_data;
  }
  size_t power = memorypa_own_msb(new_size);
  unsigned char *new_pool = memorypa_pool_list[power - 1];
  if(new_pool != NULL) {
    power = memorypa_adjust_msb(new_size, power, memorypa_pool_get_block_padding(new_pool));
  }
  new_pool = memorypa_pool_list[power];
  if(new_pool == NULL) {
    unsigned char *new_data = memorypa_rescue_allocate_for_data(new_size);
    if(new_data != NULL) {
      size_t offset = (size_t)new_data & (alignment - 1);
      if(offset) {
        offset = alignment - offset;
      }
      new_data += offset;
      new_size -= offset;
      memorypa_pool_block_set_data_offset(new_data, (unsigned short)offset);
      size_t offset_size = memorypa_pool_get_block_size(pool) - (data - default_data);
      memcpy(new_data, data, offset_size < new_size ? offset_size : new_size);
      memorypa_pool_deallocate(block);
    }
    memorypa_write_message("memorypa: Pool #", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(power, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" has not been initialized for size ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(new_size, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message("! (7)\n", MEMORYPA_WRITE_OPTION_STDERR);
    return new_data;
  }
  if(pool == new_pool) {
    size_t offset = (size_t)default_data & (alignment - 1);
    if(offset) {
      offset = alignment - offset;
    }
    unsigned char *new_data = default_data + offset;
    if(new_data != data) {
      new_size -= offset;
      size_t offset_size = memorypa_pool_get_block_size(pool) - (data - default_data);
      memmove(new_data, data, offset_size < new_size ? offset_size : new_size);
      memorypa_pool_block_set_data_offset(new_data, (unsigned short)offset);
    }
    return new_data;
  }
  unsigned char *new_data = memorypa_pool_allocate(new_pool);
  if(new_data == NULL) {
    new_data = memorypa_rescue_allocate_for_data(new_size);
    if(new_data != NULL) {
      size_t offset = (size_t)new_data & (alignment - 1);
      if(offset) {
        offset = alignment - offset;
      }
      new_data += offset;
      new_size -= offset;
      memorypa_pool_block_set_data_offset(new_data, (unsigned short)offset);
      size_t offset_size = memorypa_pool_get_block_size(pool) - (data - default_data);
      memcpy(new_data, data, offset_size < new_size ? offset_size : new_size);
      memorypa_pool_deallocate(block);
    }
    memorypa_write_message("memorypa: Pool #", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(power, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" is out of memory for size ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(new_size, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message("! (8)\n", MEMORYPA_WRITE_OPTION_STDERR);
  }
  else {
    new_data = memorypa_pool_block_get_data(new_data);
    size_t offset = (size_t)new_data & (alignment - 1);
    if(offset) {
      offset = alignment - offset;
    }
    new_data += offset;
    new_size -= offset;
    memorypa_pool_block_set_data_offset(new_data, (unsigned short)offset);
    size_t offset_size = memorypa_pool_get_block_size(pool) - (data - default_data);
    memcpy(new_data, data, offset_size < new_size ? offset_size : new_size);
    memorypa_pool_deallocate(block);
  }
  return new_data;
}

static inline unsigned char * memorypa_own_profile_aligned_malloc(size_t size, unsigned short alignment) {
  unsigned char *data = memorypa_profile_allocate(size + alignment - 1);
  if(data != NULL) {
    size_t offset = (size_t)data & (alignment - 1);
    if(offset) {
      offset = alignment - offset;
      data += offset;
      // This function is intentionally recycled:
      memorypa_pool_block_set_data_offset(data, (unsigned short)offset);
    }
  }
  return data;
}

static inline void memorypa_own_profile_realloc_copy(unsigned char *data, unsigned char *new_data, size_t new_size) {
  unsigned char *real = memorypa_profile_get_real_from_data(data);
  unsigned char *default_data = memorypa_profile_real_get_data(real);
  size_t size = memorypa_profile_real_get_size(real) - (data - default_data);
  memcpy(new_data, data, size < new_size ? size : new_size);
  memorypa_profile_deallocate_real(real);
}

int memorypa_write(int write_option, const void *buffer, unsigned int count) {
  switch(write_option) {
    case MEMORYPA_WRITE_OPTION_STDOUT:
      return MEMORYPA_WRITE(MEMORYPA_FILENO(stdout), buffer, count);
    case MEMORYPA_WRITE_OPTION_STDERR:
      return MEMORYPA_WRITE(MEMORYPA_FILENO(stderr), buffer, count);
  }
  return MEMORYPA_WRITE(MEMORYPA_FILENO(stdout), buffer, count);
}

int memorypa_write_decimal(size_t number, unsigned int right_align, int write_option) {
  unsigned char digits[20];
  memset(digits, ' ', 20);
  unsigned char *digit_index = digits + 20;
  unsigned char digit;
  do {
    digit = number % 10;
    digit += 48;
    *(--digit_index) = digit;
  }
  while((number /= 10));
  unsigned int output_size = (unsigned int)(digits + 20 - digit_index);
  if(right_align > output_size && right_align <= 20) {
    digit_index = digits + 20 - right_align;
    output_size = right_align;
  }
  return memorypa_write(write_option, digit_index, output_size);
}

int memorypa_write_hex(size_t number, unsigned int right_align, int write_option) {
  unsigned char digits[16];
  memset(digits, '0', 16);
  unsigned char *digit_index = digits + 16;
  unsigned char digit;
  do {
    digit = number & 15;
    digit += digit < 10 ? 48 : 55;
    *(--digit_index) = digit;
  }
  while((number >>= 4));
  unsigned int output_size = (unsigned int)(digits + 16 - digit_index);
  if(right_align > output_size && right_align <= 16) {
    digit_index = digits + 16 - right_align;
    output_size = right_align;
  }
  return memorypa_write(write_option, digit_index, output_size);
}

int memorypa_write_message(const char *message, int write_option) {
  return memorypa_write(write_option, message, (unsigned int)(strlen(message)));
}

unsigned char memorypa_initialize() {
  // Test atomic functions:
  unsigned char lock = 0;
  if(memorypa_lock_test_set(&lock) != 0 || lock != 1) {
    memorypa_write_message("memorypa: Lock test from 0 to 1 failed!\n", MEMORYPA_WRITE_OPTION_STDERR);
    exit(EXIT_FAILURE);
  }
  if(memorypa_lock_load(&lock) != 1) {
    memorypa_write_message("memorypa: Lock test loading 1 failed!\n", MEMORYPA_WRITE_OPTION_STDERR);
    exit(EXIT_FAILURE);
  }
  lock = 1;
  if(memorypa_lock_test_set(&lock) != 1 || lock != 1) {
    memorypa_write_message("memorypa: Lock test from 1 to 1 failed!\n", MEMORYPA_WRITE_OPTION_STDERR);
    exit(EXIT_FAILURE);
  }
  lock = 1;
  memorypa_unlock_clear(&lock);
  if(lock != 0) {
    memorypa_write_message("memorypa: Lock test from 1 to 0 failed!\n", MEMORYPA_WRITE_OPTION_STDERR);
    exit(EXIT_FAILURE);
  }
  if(memorypa_lock_load(&lock) != 0) {
    memorypa_write_message("memorypa: Lock test loading 0 failed!\n", MEMORYPA_WRITE_OPTION_STDERR);
    exit(EXIT_FAILURE);
  }
  // Test locks:
  memorypa_lock(&lock);
  memorypa_unlock(&lock);
  memorypa_lock(&lock);
  memorypa_unlock(&lock);
  // Prevent recursive initialization:
  memorypa_lock(&memorypa_initializer_thread_id_lock);
  if(memorypa_initializer_thread_id == memorypa_own_get_thread_id()) {
    memorypa_unlock(&memorypa_initializer_thread_id_lock);
    return 0;
  }
  memorypa_unlock(&memorypa_initializer_thread_id_lock);
  // Halt during initialization, then return accordingly:
  memorypa_lock(&memorypa_initializing);
  if(memorypa_lock_load(&memorypa_initialized)) {
    memorypa_unlock(&memorypa_initializing);
    return 1;
  }
  memorypa_lock(&memorypa_initializer_thread_id_lock);
  memorypa_initializer_thread_id = memorypa_own_get_thread_id();
  memorypa_unlock(&memorypa_initializer_thread_id_lock);
  // Prepare "size_t" sizes:
  memorypa_u_char_size = sizeof(unsigned char);
  memorypa_u_char_bit_size = memorypa_u_char_size * CHAR_BIT;
  memorypa_size_t_size = sizeof(size_t);
  memorypa_size_t_bit_size = memorypa_size_t_size * memorypa_u_char_bit_size;
  memorypa_size_t_half_bit_size = memorypa_size_t_bit_size / 2;
  memorypa_u_char_p_size = sizeof(unsigned char *);
  // Prepare offsets:
  memorypa_2st = 2 * memorypa_size_t_size;
  memorypa_1uc_4st_1ucp = memorypa_u_char_size + (4 * memorypa_size_t_size) + memorypa_u_char_p_size;
  memorypa_1ucp_2uc = memorypa_u_char_p_size + (2 * memorypa_u_char_size);
  memorypa_1uc_1st = memorypa_u_char_size + memorypa_size_t_size;
  memorypa_1uc_2st = memorypa_u_char_size + (2 * memorypa_size_t_size);
  memorypa_1uc_3st = memorypa_u_char_size + (3 * memorypa_size_t_size);
  memorypa_1uc_4st = memorypa_u_char_size + (4 * memorypa_size_t_size);
  memorypa_2uc = 2 * memorypa_u_char_size;
  memorypa_1ucp_1uc = memorypa_u_char_p_size + memorypa_u_char_size;
  memorypa_1st_2uc = memorypa_size_t_size + (2 * memorypa_u_char_size);
  memorypa_1st_1uc = memorypa_size_t_size + memorypa_u_char_size;
  // Prepare list sizes:
  memorypa_profile_list_size = memorypa_size_t_bit_size * memorypa_2st;
  memorypa_pool_list_size = memorypa_size_t_bit_size * memorypa_u_char_p_size;
  // Retrieve configuration:
  memorypa_functions functions;
  memorypa_pool_options *sets_of_pool_options;
  switch(memorypa_size_t_bit_size) {
    case 32:
      ;
      memorypa_pool_options sets_of_pool_options_32[32];
      sets_of_pool_options = sets_of_pool_options_32;
      break;
    case 64:
      ;
      memorypa_pool_options sets_of_pool_options_64[64];
      sets_of_pool_options = sets_of_pool_options_64;
      break;
    default:
      memorypa_write_message("memorypa: Non-standard size for machine word! (1)\n", MEMORYPA_WRITE_OPTION_STDERR);
      exit(EXIT_FAILURE);
  }
  memset(sets_of_pool_options, 0, memorypa_size_t_bit_size * sizeof(memorypa_pool_options));
  #ifdef _MSC_VER
  typedef void(*memorypa_initializer_options_type)(memorypa_functions *, memorypa_pool_options *);
  char win_file_name[MAX_PATH];
  GetModuleFileName(NULL, win_file_name, MAX_PATH);
  HINSTANCE win_library = LoadLibrary(TEXT(win_file_name));
  memorypa_initializer_options_type memorypa_initializer_options = (memorypa_initializer_options_type)GetProcAddress(win_library, "memorypa_initializer_options");
  memorypa_initializer_options(&functions, sets_of_pool_options);
  FreeLibrary(win_library);
  #else
  memorypa_initializer_options(&functions, sets_of_pool_options);
  #endif
  // Recover original allocation functions:
  memorypa_given_malloc = functions.malloc;
  memorypa_given_realloc = functions.realloc;
  memorypa_given_free = functions.free;
  // For MSB function:
  memorypa_size_t_half_bit_size_next_power = 1;
  while(memorypa_size_t_half_bit_size > (memorypa_size_t_half_bit_size_next_power <<= 1));
  // Choose magic numbers:
  if(memorypa_size_t_size == 4 && memorypa_u_char_bit_size == 8) {
    memorypa_mhash_first_magic = 40499u;
    memorypa_mhash_second_magic = 40493u;
  }
  else if(memorypa_size_t_size == 8 && memorypa_u_char_bit_size == 8) {
    memorypa_mhash_first_magic = 2654435789u;
    memorypa_mhash_second_magic = 2654435761u;
  }
  else {
    memorypa_write_message("memorypa: Non-standard size for machine word! (2)\n", MEMORYPA_WRITE_OPTION_STDERR);
    exit(EXIT_FAILURE);
  }
  // Test thread ID fetching:
  if(!memorypa_own_get_thread_id()) {
    memorypa_write_message("memorypa: Failed to get positive thread ID!\n", MEMORYPA_WRITE_OPTION_STDERR);
    exit(EXIT_FAILURE);
  }
  // Prepare the pool:
  memorypa_pools_initialize(sets_of_pool_options);
  // Trigger pooling:
  memorypa_lock_test_set(&memorypa_initialized);
  // Initialization is complete:
  memorypa_unlock(&memorypa_initializing);
  return 1;
}

unsigned char memorypa_pools_are_invalid() {
  size_t output_pool_size;
  size_t i = 0;
  do {
    if(memorypa_pool_list[i] != NULL) {
      if(memorypa_pool_is_invalid(memorypa_pool_list[i], &output_pool_size)) {
        return 1;
      }
    }
  }
  while(++i < memorypa_size_t_bit_size);
  size_t total_size = memorypa_profile_list_size + memorypa_pool_list_size;
  unsigned char *current_pool = memorypa_everything + total_size;
  do {
    if(memorypa_pool_is_invalid(current_pool, &output_pool_size)) {
      return 2;
    }
    current_pool += output_pool_size;
    total_size += output_pool_size;
  }
  while(total_size < memorypa_everything_size);
  if(total_size != memorypa_everything_size) {
    memorypa_write_message("memorypa: The calculated total size is off: ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(total_size, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message(" vs. ", MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_decimal(memorypa_everything_size, 0, MEMORYPA_WRITE_OPTION_STDERR);
    memorypa_write_message("!\n", MEMORYPA_WRITE_OPTION_STDERR);
    return 3;
  }
  return 0;
}

void memorypa_destroy() {
  memorypa_lock(&memorypa_initializing);
  if(memorypa_pool_list != NULL) {
    unsigned char *previous = NULL;
    size_t i = 0;
    do {
      /*
        All pools must be locked before a clean "free" can happen. Pool
        assignment is always in ascending order of block size even when a
        single pool sits across multiple indices. Track the "previous" to
        prevent deadlocks.
      */
      if(memorypa_pool_list[i] != NULL && memorypa_pool_list[i] != previous) {
        memorypa_pool_lock(memorypa_pool_list[i]);
        previous = memorypa_pool_list[i];
      }
    }
    while(++i < memorypa_size_t_bit_size);
    memset(memorypa_everything, 0, memorypa_everything_size);
    memorypa_given_free(memorypa_everything);
    memorypa_everything = NULL;
    memorypa_everything_size = 0;
    memorypa_pool_list = NULL;
    memorypa_unlock_clear(&memorypa_initialized);
  }
  memorypa_unlock(&memorypa_initializing);
}

// Don't forget to initialize!
size_t memorypa_get_size_t_size() {
  return memorypa_size_t_size;
}

// Don't forget to initialize!
size_t memorypa_get_size_t_bit_size() {
  return memorypa_size_t_bit_size;
}

// Don't forget to initialize!
size_t memorypa_get_size_t_half_bit_size() {
  return memorypa_size_t_half_bit_size;
}

// Don't forget to initialize!
size_t memorypa_get_u_char_bit_size() {
  return memorypa_u_char_bit_size;
}

// Don't forget to initialize!
size_t memorypa_msb(size_t value) {
  return memorypa_own_msb(value);
}

size_t memorypa_get_thread_id() {
  return memorypa_own_get_thread_id();
}

// Don't forget to initialize!
size_t memorypa_mhash(size_t value) {
  size_t top_half = value >> memorypa_size_t_half_bit_size;
  size_t bottom_half = value << memorypa_size_t_half_bit_size;
  bottom_half >>= memorypa_size_t_half_bit_size;
  size_t mixer_bottom_half = top_half + bottom_half;
  size_t mixer_top_half = mixer_bottom_half >> memorypa_size_t_half_bit_size;
  mixer_bottom_half <<= memorypa_size_t_half_bit_size;
  mixer_bottom_half >>= memorypa_size_t_half_bit_size;
  top_half ^= mixer_top_half;
  top_half *= memorypa_mhash_first_magic;
  top_half <<= memorypa_size_t_half_bit_size;
  top_half >>= memorypa_size_t_half_bit_size;
  bottom_half = mixer_bottom_half * memorypa_mhash_first_magic;
  bottom_half <<= memorypa_size_t_half_bit_size;
  bottom_half >>= memorypa_size_t_half_bit_size;
  mixer_bottom_half = top_half + bottom_half;
  mixer_top_half = mixer_bottom_half >> memorypa_size_t_half_bit_size;
  mixer_bottom_half <<= memorypa_size_t_half_bit_size;
  mixer_bottom_half >>= memorypa_size_t_half_bit_size;
  top_half ^= mixer_top_half;
  top_half *= memorypa_mhash_second_magic;
  bottom_half = mixer_bottom_half * memorypa_mhash_second_magic;
  top_half <<= memorypa_size_t_half_bit_size;
  top_half >>= memorypa_size_t_half_bit_size;
  bottom_half <<= memorypa_size_t_half_bit_size;
  return top_half | bottom_half;
}

void * memorypa_malloc(size_t size) {
  if(memorypa_lock_load(&memorypa_initialized) || memorypa_initialize()) {
    return memorypa_own_malloc(size);
  }
  unsigned char *output = memorypa_initializer_slab + memorypa_initializer_slab_index;
  memorypa_initializer_slab_index += size;
  return output;
}

void * memorypa_aligned_malloc(size_t alignment, size_t size) {
  if(memorypa_lock_load(&memorypa_initialized) || memorypa_initialize()) {
    return memorypa_own_aligned_malloc(size, (unsigned short)alignment);
  }
  // We ignore alignment in this extremely ridiculous situation:
  unsigned char *output = memorypa_initializer_slab + memorypa_initializer_slab_index;
  memorypa_initializer_slab_index += size;
  return output;
}

void * memorypa_calloc(size_t amount, size_t unit_size) {
  if(memorypa_lock_load(&memorypa_initialized) || memorypa_initialize()) {
    return memorypa_own_calloc(amount, unit_size);
  }
  unsigned char *output = memorypa_initializer_slab + memorypa_initializer_slab_index;
  size_t size = amount * unit_size;
  memorypa_initializer_slab_index += size;
  memset(output, 0, size);
  return output;
}

void * memorypa_aligned_calloc(size_t alignment, size_t amount, size_t unit_size) {
  if(memorypa_lock_load(&memorypa_initialized) || memorypa_initialize()) {
    return memorypa_own_aligned_calloc(amount, unit_size, (unsigned short)alignment);
  }
  // We ignore alignment in this extremely ridiculous situation:
  unsigned char *output = memorypa_initializer_slab + memorypa_initializer_slab_index;
  size_t size = amount * unit_size;
  memorypa_initializer_slab_index += size;
  memset(output, 0, size);
  return output;
}

void * memorypa_realloc(void *data, size_t new_size) {
  if(
    (unsigned char *)data >= memorypa_initializer_slab
    && (unsigned char *)data < memorypa_initializer_slab + MEMORYPA_INITIALIZER_SLAB_SIZE
  ) {
    return NULL;
  }
  return memorypa_own_realloc(data, new_size);
}

void * memorypa_aligned_realloc(void *data, size_t alignment, size_t new_size) {
  if(
    (unsigned char *)data >= memorypa_initializer_slab
    && (unsigned char *)data < memorypa_initializer_slab + MEMORYPA_INITIALIZER_SLAB_SIZE
  ) {
    return NULL;
  }
  return memorypa_own_aligned_realloc(data, new_size, (unsigned short)alignment);
}

void memorypa_free(void *data) {
  if(
    (unsigned char *)data >= memorypa_initializer_slab
    && (unsigned char *)data < memorypa_initializer_slab + MEMORYPA_INITIALIZER_SLAB_SIZE
  ) {
    return;
  }
  memorypa_own_free(data);
}

size_t memorypa_malloc_usable_size(void *data) {
  if(
    (unsigned char *)data >= memorypa_initializer_slab
    && (unsigned char *)data < memorypa_initializer_slab + MEMORYPA_INITIALIZER_SLAB_SIZE
  ) {
    return 0;
  }
  unsigned char *pool = memorypa_pool_block_get_block_from_data(data);
  // Calling "memorypa_pool_block_get_data" in case of alignment:
  unsigned char *default_data = memorypa_pool_block_get_data(pool);
  pool = memorypa_pool_block_get_pool(pool);
  if(pool == NULL) {
    return 0;
  }
  return memorypa_pool_get_block_size(pool) - ((unsigned char *)data - default_data);
}

void * memorypa_profile_malloc(size_t size) {
  if(memorypa_lock_load(&memorypa_initialized) || memorypa_initialize()) {
    return memorypa_profile_allocate(size);
  }
  // Don't bother counting here:
  unsigned char *output = memorypa_initializer_slab + memorypa_initializer_slab_index;
  memorypa_initializer_slab_index += size;
  return output;
}

void * memorypa_profile_aligned_malloc(size_t alignment, size_t size) {
  if(memorypa_lock_load(&memorypa_initialized) || memorypa_initialize()) {
    return memorypa_own_profile_aligned_malloc(size, (unsigned short)alignment);
  }
  // Ignore alignment and don't bother counting here:
  unsigned char *output = memorypa_initializer_slab + memorypa_initializer_slab_index;
  memorypa_initializer_slab_index += size;
  return output;
}

void * memorypa_profile_calloc(size_t amount, size_t unit_size) {
  size_t size = amount * unit_size;
  if(memorypa_lock_load(&memorypa_initialized) || memorypa_initialize()) {
    unsigned char *data = memorypa_profile_allocate(size);
    if(data != NULL) {
      memset(data, 0, size);
    }
    return data;
  }
  // Don't bother counting here:
  unsigned char *output = memorypa_initializer_slab + memorypa_initializer_slab_index;
  memorypa_initializer_slab_index += size;
  memset(output, 0, size);
  return output;
}

void * memorypa_profile_aligned_calloc(size_t alignment, size_t amount, size_t unit_size) {
  size_t size = amount * unit_size;
  if(memorypa_lock_load(&memorypa_initialized) || memorypa_initialize()) {
    unsigned char *data = memorypa_own_profile_aligned_malloc(size, (unsigned short)alignment);
    if(data != NULL) {
      memset(data, 0, size);
    }
    return data;
  }
  // Ignore alignment and don't bother counting here:
  unsigned char *output = memorypa_initializer_slab + memorypa_initializer_slab_index;
  memorypa_initializer_slab_index += size;
  memset(output, 0, size);
  return output;
}

void * memorypa_profile_realloc(void *data, size_t new_size) {
  if(
    (unsigned char *)data >= memorypa_initializer_slab
    && (unsigned char *)data < memorypa_initializer_slab + MEMORYPA_INITIALIZER_SLAB_SIZE
  ) {
    return NULL;
  }
  unsigned char *new_data = memorypa_profile_allocate(new_size);
  if(new_data != NULL && data != NULL) {
    memorypa_own_profile_realloc_copy(data, new_data, new_size);
  }
  return new_data;
}

void * memorypa_profile_aligned_realloc(void *data, size_t alignment, size_t new_size) {
  if(
    (unsigned char *)data >= memorypa_initializer_slab
    && (unsigned char *)data < memorypa_initializer_slab + MEMORYPA_INITIALIZER_SLAB_SIZE
  ) {
    return NULL;
  }
  unsigned char *new_data = memorypa_own_profile_aligned_malloc(new_size, (unsigned short)alignment);
  if(new_data != NULL && data != NULL) {
    memorypa_own_profile_realloc_copy(data, new_data, new_size);
  }
  return new_data;
}

void memorypa_profile_free(void *data) {
  if(
    (unsigned char *)data >= memorypa_initializer_slab
    && (unsigned char *)data < memorypa_initializer_slab + MEMORYPA_INITIALIZER_SLAB_SIZE
  ) {
    return;
  }
  if(data != NULL) {
    memorypa_profile_deallocate_real(memorypa_profile_get_real_from_data(data));
  }
}

size_t memorypa_profile_malloc_usable_size(void *data) {
  if(
    (unsigned char *)data >= memorypa_initializer_slab
    && (unsigned char *)data < memorypa_initializer_slab + MEMORYPA_INITIALIZER_SLAB_SIZE
  ) {
    return 0;
  }
  unsigned char *real = memorypa_profile_get_real_from_data(data);
  unsigned char *default_data = memorypa_profile_real_get_data(real);
  return memorypa_profile_real_get_size(real) - ((unsigned char *)data - default_data);
}

void memorypa_profile_print() {
  size_t i = 0;
  size_t padding, count, max;
  unsigned char *pool;
  memorypa_write_message("memorypa: Current profile:\n", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message("memorypa:   Power   Padding     Count       Max\n", MEMORYPA_WRITE_OPTION_STDOUT);
  do {
    memorypa_lock(&memorypa_profile_lock);
    pool = memorypa_pool_list[i];
    padding = pool == NULL ? 0 : memorypa_pool_get_block_padding(pool);
    count = memorypa_profile_get_count(i);
    max = memorypa_profile_get_max(i);
    memorypa_unlock(&memorypa_profile_lock);
    if(max) {
      memorypa_write_message("memorypa: ", MEMORYPA_WRITE_OPTION_STDOUT);
      memorypa_write_decimal(i, 7, MEMORYPA_WRITE_OPTION_STDOUT);
      memorypa_write_message("   ", MEMORYPA_WRITE_OPTION_STDOUT);
      memorypa_write_decimal(padding, 7, MEMORYPA_WRITE_OPTION_STDOUT);
      memorypa_write_message("   ", MEMORYPA_WRITE_OPTION_STDOUT);
      memorypa_write_decimal(count, 7, MEMORYPA_WRITE_OPTION_STDOUT);
      memorypa_write_message("   ", MEMORYPA_WRITE_OPTION_STDOUT);
      memorypa_write_decimal(max, 7, MEMORYPA_WRITE_OPTION_STDOUT);
      memorypa_write_message("\n", MEMORYPA_WRITE_OPTION_STDOUT);
    }
  }
  while(++i < memorypa_size_t_bit_size);
}
