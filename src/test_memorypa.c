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

#ifdef _MSC_VER
#include <process.h>
#else
#include <pthread.h>
#endif

#include "memorypa.h"

#ifdef _MSC_VER
__declspec(dllexport) void memorypa_initializer_options(memorypa_functions *functions, memorypa_pool_options *sets_of_pool_options) {
#else
void memorypa_initializer_options(memorypa_functions *functions, memorypa_pool_options *sets_of_pool_options) {
#endif
  functions->malloc = malloc;
  functions->free = free;
  sets_of_pool_options[0].power = 7;
  sets_of_pool_options[0].amount = 500;
  sets_of_pool_options[1].power = 8;
  sets_of_pool_options[1].amount = 200;
  sets_of_pool_options[2].power = 9;
  sets_of_pool_options[2].amount = 200;
  sets_of_pool_options[3].power = 10;
  sets_of_pool_options[3].amount = 300;
  sets_of_pool_options[4].power = 11;
  // Test padding!
  sets_of_pool_options[4].padding = 64;
  //
  sets_of_pool_options[4].amount = 400;
  sets_of_pool_options[5].power = 12;
  sets_of_pool_options[5].amount = 500;
  sets_of_pool_options[6].power = 13;
  sets_of_pool_options[6].amount = 50;
}

static size_t size_t_u_char_bit_diff = 0;
static unsigned char memorypa_test_profile_mode = 0;

#define MEMORYPA_TEST_HASHES_POWER 13

static void memorypa_test_mhash() {
  size_t memorypa_hashes_size = 1 << MEMORYPA_TEST_HASHES_POWER;
  size_t memorypa_hash_keys_size = memorypa_hashes_size >> 1;
  size_t size_t_bit_size = memorypa_get_size_t_bit_size();
  size_t size_t_half_bit_size = memorypa_get_size_t_half_bit_size();
  srand((unsigned int)(memorypa_mhash(memorypa_get_thread_id()) >> size_t_half_bit_size));
  /*
    Random test:
  */
  size_t results[1 << MEMORYPA_TEST_HASHES_POWER];
  memset(results, 0, memorypa_hashes_size * sizeof(size_t));
  size_t max_resolutions = 0;
  size_t i = 0;
  do {
    size_t result = memorypa_mhash(rand());
    size_t hash = result >> (size_t_bit_size - MEMORYPA_TEST_HASHES_POWER);
    size_t resolutions = 0;
    while(results[hash] > 0) {
      ++resolutions;
      result = memorypa_mhash(result);
      hash = result >> (size_t_bit_size - MEMORYPA_TEST_HASHES_POWER);
    }
    if(resolutions > max_resolutions) {
      max_resolutions = resolutions;
    }
    ++results[hash];
  }
  while(++i < memorypa_hash_keys_size);
  printf("Random test - max resolutions: %zu\n\n", max_resolutions);
  /*
    Linear sequence test:
  */
  size_t reported = 0;
  size_t multiple = 1;
  do {
    memset(results, 0, memorypa_hashes_size * sizeof(size_t));
    max_resolutions = 0;
    i = 0;
    do {
      size_t result = memorypa_mhash(i * multiple);
      size_t hash = result >> (size_t_bit_size - MEMORYPA_TEST_HASHES_POWER);
      size_t resolutions = 0;
      while(results[hash] > 0) {
        ++resolutions;
        result = memorypa_mhash(result);
        hash = result >> (size_t_bit_size - MEMORYPA_TEST_HASHES_POWER);
      }
      if(resolutions > max_resolutions) {
        max_resolutions = resolutions;
      }
      ++results[hash];
    }
    while(++i < memorypa_hash_keys_size);
    if(max_resolutions > 15) {
      ++reported;
      printf("Linear sequence test (%zu) - max resolutions: %zu\n", multiple, max_resolutions);
    }
  }
  while(++multiple <= 10000);
  if(reported) {
    printf("\n");
  }
  printf("Linear sequence test collisions reported: %zu\n\n", reported);
  /*
    Exponential sequence test:
  */
  reported = 0;
  size_t exponent = 1;
  do {
    memset(results, 0, memorypa_hashes_size * sizeof(size_t));
    max_resolutions = 0;
    i = 0;
    do {
      size_t result = memorypa_mhash(i + (memorypa_one << exponent));
      size_t hash = result >> (size_t_bit_size - MEMORYPA_TEST_HASHES_POWER);
      size_t resolutions = 0;
      while(results[hash] > 0) {
        ++resolutions;
        result = memorypa_mhash(result);
        hash = result >> (size_t_bit_size - MEMORYPA_TEST_HASHES_POWER);
      }
      if(resolutions > max_resolutions) {
        max_resolutions = resolutions;
      }
      ++results[hash];
    }
    while(++i < memorypa_hash_keys_size);
    if(max_resolutions > 15) {
      ++reported;
      printf("Exponential sequence test (%zu) - max resolutions: %zu\n", multiple, max_resolutions);
    }
  }
  while(++exponent < size_t_bit_size);
  if(reported) {
    printf("\n");
  }
  printf("Exponential sequence test collisions reported: %zu\n\n", reported);
}

static void memorypa_test_set_block(void *block, size_t block_size) {
  size_t seed = (size_t)block;
  size_t i = 0;
  do {
    *((unsigned char *)block + i) = (unsigned char)((seed = memorypa_mhash(seed)) >> size_t_u_char_bit_diff);
  }
  while(++i < block_size);
}

static unsigned char memorypa_test_check_block(void *block, size_t block_size, size_t seed) {
  size_t i = 0;
  do {
    if(*((unsigned char *)block + i) != (unsigned char)((seed = memorypa_mhash(seed)) >> size_t_u_char_bit_diff)) {
      return 0;
    }
  }
  while(++i < block_size);
  return 1;
}

static void memorypa_test_allocation(unsigned char id) {
  size_t blocks_size = memorypa_get_size_t_half_bit_size();
  blocks_size <<= 5;
  void **blocks;
  size_t *blocks_sizes;
  switch(blocks_size) {
    case 512:
      ;
      void *blocks_32[512];
      size_t blocks_sizes_32[512];
      blocks = blocks_32;
      blocks_sizes = blocks_sizes_32;
      break;
    case 1024:
      ;
      void *blocks_64[1024];
      size_t blocks_sizes_64[1024];
      blocks = blocks_64;
      blocks_sizes = blocks_sizes_64;
      break;
    default:
      fprintf(stderr, "Non-standard size for machine word!\n");
      exit(EXIT_FAILURE);
  }
  size_t multiple = 4;
  size_t i = 0;
  do {
    blocks[i] = NULL;
    blocks_sizes[i] = multiple * (i + 1);
  }
  while(++i < blocks_size);
  size_t tries = blocks_size << 3;
  printf("Allocation Test:\n%u) blocks_size: %zu\n%u) multiple: %zu\n%u) tries: %zu\n\n", id, blocks_size, id, multiple, id, tries);
  size_t malloc_attempts = 0;
  size_t malloc_count = 0;
  size_t malloc_failed_count = 0;
  size_t calloc_attempts = 0;
  size_t calloc_count = 0;
  size_t calloc_failed_count = 0;
  size_t realloc_attempts = 0;
  size_t realloc_count = 0;
  size_t realloc_failed_count = 0;
  size_t free_attempts = 0;
  size_t free_count = 0;
  size_t seed = memorypa_get_thread_id();
  unsigned char operation;
  size_t block_index;
  size_t new_block_index;
  size_t block_index_size_check;
  size_t j;
  unsigned char test_check = 0;
  size_t current_alignment;
  i = 0;
  do {
    operation = ((seed = memorypa_mhash(seed)) >> size_t_u_char_bit_diff) & 3;
    switch(operation) {
      case 0:
        {
          operation = ((seed = memorypa_mhash(seed)) >> size_t_u_char_bit_diff) & 1;
          switch(operation) {
            case 0:
              {
                ++malloc_attempts;
                block_index = (seed = memorypa_mhash(seed)) % blocks_size;
                if(blocks[block_index] == NULL) {
                  blocks[block_index] = memorypa_test_profile_mode ? memorypa_profile_malloc(blocks_sizes[block_index]) : memorypa_malloc(blocks_sizes[block_index]);
                  if(blocks[block_index] == NULL) {
                    ++malloc_failed_count;
                  }
                  else {
                    memorypa_test_set_block(blocks[block_index], blocks_sizes[block_index]);
                    ++malloc_count;
                    block_index_size_check = memorypa_test_profile_mode ? memorypa_profile_malloc_usable_size(blocks[block_index]) : memorypa_malloc_usable_size(blocks[block_index]);
                    if(block_index_size_check < blocks_sizes[block_index]) {
                      printf("%u) Block %zu reports incorrect size!\n", id, block_index);
                    }
                  }
                }
              }
              break;
            default:
              {
                ++malloc_attempts;
                block_index = (seed = memorypa_mhash(seed)) % blocks_size;
                if(blocks[block_index] == NULL) {
                  current_alignment = ((seed = memorypa_mhash(seed)) >> size_t_u_char_bit_diff) & 7;
                  current_alignment = memorypa_one << (current_alignment + 3);
                  blocks[block_index] = memorypa_test_profile_mode ? memorypa_profile_aligned_malloc(current_alignment, blocks_sizes[block_index]) : memorypa_aligned_malloc(current_alignment, blocks_sizes[block_index]);
                  if(blocks[block_index] == NULL) {
                    ++malloc_failed_count;
                  }
                  else {
                    if((size_t)(blocks[block_index]) & (current_alignment - 1)) {
                      printf("%u) Block %zu violates alignment!\n", id, block_index);
                    }
                    memorypa_test_set_block(blocks[block_index], blocks_sizes[block_index]);
                    ++malloc_count;
                    block_index_size_check = memorypa_test_profile_mode ? memorypa_profile_malloc_usable_size(blocks[block_index]) : memorypa_malloc_usable_size(blocks[block_index]);
                    if(block_index_size_check < blocks_sizes[block_index]) {
                      printf("%u) Block %zu reports incorrect size!\n", id, block_index);
                    }
                  }
                }
              }
          }
        }
        break;
      case 1:
        {
          operation = ((seed = memorypa_mhash(seed)) >> size_t_u_char_bit_diff) & 1;
          switch(operation) {
            case 0:
              {
                ++calloc_attempts;
                block_index = (seed = memorypa_mhash(seed)) % blocks_size;
                if(blocks[block_index] == NULL) {
                  blocks[block_index] = memorypa_test_profile_mode ? memorypa_profile_calloc(1, blocks_sizes[block_index]) : memorypa_calloc(1, blocks_sizes[block_index]);
                  if(blocks[block_index] == NULL) {
                    ++calloc_failed_count;
                  }
                  else {
                    memorypa_test_set_block(blocks[block_index], blocks_sizes[block_index]);
                    ++calloc_count;
                    block_index_size_check = memorypa_test_profile_mode ? memorypa_profile_malloc_usable_size(blocks[block_index]) : memorypa_malloc_usable_size(blocks[block_index]);
                    if(block_index_size_check < blocks_sizes[block_index]) {
                      printf("%u) Block %zu reports incorrect size!\n", id, block_index);
                    }
                  }
                }
              }
              break;
            default:
              {
                ++calloc_attempts;
                block_index = (seed = memorypa_mhash(seed)) % blocks_size;
                if(blocks[block_index] == NULL) {
                  current_alignment = ((seed = memorypa_mhash(seed)) >> size_t_u_char_bit_diff) & 7;
                  current_alignment = memorypa_one << (current_alignment + 3);
                  blocks[block_index] = memorypa_test_profile_mode ? memorypa_profile_aligned_calloc(current_alignment, 1, blocks_sizes[block_index]) : memorypa_aligned_calloc(current_alignment, 1, blocks_sizes[block_index]);
                  if(blocks[block_index] == NULL) {
                    ++calloc_failed_count;
                  }
                  else {
                    if((size_t)(blocks[block_index]) & (current_alignment - 1)) {
                      printf("%u) Block %zu violates alignment!\n", id, block_index);
                    }
                    memorypa_test_set_block(blocks[block_index], blocks_sizes[block_index]);
                    ++calloc_count;
                    block_index_size_check = memorypa_test_profile_mode ? memorypa_profile_malloc_usable_size(blocks[block_index]) : memorypa_malloc_usable_size(blocks[block_index]);
                    if(block_index_size_check < blocks_sizes[block_index]) {
                      printf("%u) Block %zu reports incorrect size!\n", id, block_index);
                    }
                  }
                }
              }
          }
        }
        break;
      case 2:
        {
          operation = ((seed = memorypa_mhash(seed)) >> size_t_u_char_bit_diff) & 1;
          switch(operation) {
            case 0:
              {
                ++realloc_attempts;
                new_block_index = (seed = memorypa_mhash(seed)) % blocks_size;
                block_index = (seed = memorypa_mhash(seed)) % blocks_size;
                do {
                  if(blocks[block_index] != NULL) {
                    if(blocks[new_block_index] != NULL) {
                      memorypa_test_profile_mode ? memorypa_profile_free(blocks[new_block_index]) : memorypa_free(blocks[new_block_index]);
                      blocks[new_block_index] = NULL;
                      ++free_count;
                    }
                    // Don't test copied data if there's none!
                    test_check = blocks[block_index] != NULL;
                    /*
                      If "new_block_index" equals "block_index", null will be passed to
                      "realloc", which is actually OK!
                    */
                    blocks[new_block_index] = memorypa_test_profile_mode ? memorypa_profile_realloc(blocks[block_index], blocks_sizes[new_block_index]) : memorypa_realloc(blocks[block_index], blocks_sizes[new_block_index]);
                    if(blocks[new_block_index] == NULL) {
                      ++realloc_failed_count;
                    }
                    else {
                      size_t max_size = blocks_sizes[block_index] > blocks_sizes[new_block_index] ? blocks_sizes[new_block_index] : blocks_sizes[block_index];
                      if(test_check && !memorypa_test_check_block(blocks[new_block_index], max_size, (size_t)(blocks[block_index]))) {
                        printf("%u) Block %zu fails consistency check!\n", id, new_block_index);
                      }
                      if(new_block_index != block_index) {
                        blocks[block_index] = NULL;
                      }
                      memorypa_test_set_block(blocks[new_block_index], blocks_sizes[new_block_index]);
                      ++realloc_count;
                      block_index_size_check = memorypa_test_profile_mode ? memorypa_profile_malloc_usable_size(blocks[new_block_index]) : memorypa_malloc_usable_size(blocks[new_block_index]);
                      if(block_index_size_check < blocks_sizes[new_block_index]) {
                        printf("%u) Block %zu reports incorrect size!\n", id, block_index);
                      }
                    }
                    break;
                  }
                }
                while(++block_index < blocks_size);
              }
              break;
            default:
              {
                ++realloc_attempts;
                new_block_index = (seed = memorypa_mhash(seed)) % blocks_size;
                block_index = (seed = memorypa_mhash(seed)) % blocks_size;
                do {
                  if(blocks[block_index] != NULL) {
                    if(blocks[new_block_index] != NULL) {
                      memorypa_test_profile_mode ? memorypa_profile_free(blocks[new_block_index]) : memorypa_free(blocks[new_block_index]);
                      blocks[new_block_index] = NULL;
                      ++free_count;
                    }
                    // Don't test copied data if there's none!
                    test_check = blocks[block_index] != NULL;
                    /*
                      If "new_block_index" equals "block_index", null will be passed to
                      "realloc", which is actually OK!
                    */
                    current_alignment = ((seed = memorypa_mhash(seed)) >> size_t_u_char_bit_diff) & 7;
                    current_alignment = memorypa_one << (current_alignment + 3);
                    blocks[new_block_index] = memorypa_test_profile_mode ? memorypa_profile_aligned_realloc(blocks[block_index], current_alignment, blocks_sizes[new_block_index]) : memorypa_aligned_realloc(blocks[block_index], current_alignment, blocks_sizes[new_block_index]);
                    if(blocks[new_block_index] == NULL) {
                      ++realloc_failed_count;
                    }
                    else {
                      if((size_t)(blocks[new_block_index]) & (current_alignment - 1)) {
                        printf("%u) Block %zu violates alignment!\n", id, new_block_index);
                      }
                      size_t max_size = blocks_sizes[block_index] > blocks_sizes[new_block_index] ? blocks_sizes[new_block_index] : blocks_sizes[block_index];
                      if(test_check && !memorypa_test_check_block(blocks[new_block_index], max_size, (size_t)(blocks[block_index]))) {
                        printf("%u) Block %zu fails consistency check!\n", id, new_block_index);
                      }
                      if(new_block_index != block_index) {
                        blocks[block_index] = NULL;
                      }
                      memorypa_test_set_block(blocks[new_block_index], blocks_sizes[new_block_index]);
                      ++realloc_count;
                      block_index_size_check = memorypa_test_profile_mode ? memorypa_profile_malloc_usable_size(blocks[new_block_index]) : memorypa_malloc_usable_size(blocks[new_block_index]);
                      if(block_index_size_check < blocks_sizes[new_block_index]) {
                        printf("%u) Block %zu reports incorrect size!\n", id, block_index);
                      }
                    }
                    break;
                  }
                }
                while(++block_index < blocks_size);
              }
          }
        }
        break;
      default:
        {
          ++free_attempts;
          block_index = (seed = memorypa_mhash(seed)) % blocks_size;
          do {
            if(blocks[block_index] != NULL) {
              memorypa_test_profile_mode ? memorypa_profile_free(blocks[block_index]) : memorypa_free(blocks[block_index]);
              blocks[block_index] = NULL;
              ++free_count;
              break;
            }
          }
          while(++block_index < blocks_size);
        }
    }
    memorypa_pools_are_invalid();
    j = 0;
    do {
      if(blocks[j] != NULL) {
        if(!memorypa_test_check_block(blocks[j], blocks_sizes[j], (size_t)(blocks[j]))) {
          printf("%u) Block %zu fails consistency check!\n", id, j);
        }
      }
    }
    while(++j < blocks_size);
  }
  while(++i < tries);
  printf(
    "%u) malloc_attempts: %zu\n"
    "%u) malloc_count: %zu\n"
    "%u) malloc_failed_count: %zu\n"
    "%u) calloc_attempts: %zu\n"
    "%u) calloc_count: %zu\n"
    "%u) calloc_failed_count: %zu\n"
    "%u) realloc_attempts: %zu\n"
    "%u) realloc_count: %zu\n"
    "%u) realloc_failed_count: %zu\n"
    "%u) free_attempts: %zu\n"
    "%u) free_count: %zu\n\n",
    id, malloc_attempts,
    id, malloc_count,
    id, malloc_failed_count,
    id, calloc_attempts,
    id, calloc_count,
    id, calloc_failed_count,
    id, realloc_attempts,
    id, realloc_count,
    id, realloc_failed_count,
    id, free_attempts,
    id, free_count
  );
}

#ifdef _MSC_VER
static unsigned __stdcall memorypa_test_first_thread(void * first_thread_data) {
#else
static void * memorypa_test_first_thread(void * first_thread_data) {
#endif
  (void)first_thread_data;
  memorypa_test_allocation(2);
  #ifdef _MSC_VER
  return 0;
  #else
  return NULL;
  #endif
}

int main(int argc, char const *argv[]) {
  memorypa_initialize();
  size_t_u_char_bit_diff = memorypa_get_size_t_bit_size() - memorypa_get_u_char_bit_size();
  if(argc > 1 && !strcmp(argv[1], "profile")) {
    memorypa_test_profile_mode = 1;
  }
  #ifdef _MSC_VER
  uintptr_t first_thread_handle = _beginthreadex(NULL, 0, memorypa_test_first_thread, NULL, 0, NULL);
  if(!first_thread_handle) {
  #else
  pthread_t first_thread_handle;
  if(pthread_create(&first_thread_handle, NULL, memorypa_test_first_thread, NULL)) {
  #endif
    fprintf(stderr, "Failed to set up the first thread!\n");
    exit(EXIT_FAILURE);
  }
  memorypa_test_allocation(1);
  #ifdef _MSC_VER
  if(WaitForSingleObject((HANDLE)first_thread_handle, INFINITE) != WAIT_OBJECT_0) {
  #else
  if(pthread_join(first_thread_handle, NULL)) {
  #endif
    fprintf(stderr, "Failed to wait for the first thread!\n");
    exit(EXIT_FAILURE);
  }
  #ifdef _MSC_VER
  CloseHandle((HANDLE)first_thread_handle);
  #endif
  if(memorypa_test_profile_mode) {
    memorypa_profile_print();
    printf("\n");
  }
  memorypa_test_mhash();
  memorypa_destroy();
  printf("Done!\n\n");
  return 0;
}
