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

#ifndef MEMORYPA_H
#define MEMORYPA_H

#ifdef _MSC_VER
#include <windows.h>
#include <intrin.h>
#pragma intrinsic(_InterlockedOr8)
#pragma intrinsic(_InterlockedAnd8)
#include <io.h>
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <sys/syscall.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define MEMORYPA_WRITE _write
#define MEMORYPA_FILENO _fileno
#else
#define MEMORYPA_WRITE write
#define MEMORYPA_FILENO fileno
#endif

#define MEMORYPA_WRITE_OPTION_STDOUT 0
#define MEMORYPA_WRITE_OPTION_STDERR 1
#define MEMORYPA_INITIALIZER_SLAB_SIZE 1024

const size_t memorypa_one = 1;

typedef struct {
  void *(*malloc)(size_t);
  void *(*realloc)(void*,size_t);
  void (*free)(void*);
} memorypa_functions;

typedef struct {
  unsigned char power;
  size_t padding;
  size_t amount;
  size_t own_block_size;
  size_t own_size;
  size_t own_relative_position;
} memorypa_pool_options;

int memorypa_write(int option, const void *buffer, unsigned int count);
int memorypa_write_decimal(size_t number, unsigned int right_align, int write_option);
int memorypa_write_hex(size_t number, unsigned int right_align, int write_option);
int memorypa_write_message(const char *message, int write_option);
unsigned char memorypa_initialize();
unsigned char memorypa_pools_are_invalid();
void memorypa_destroy();
size_t memorypa_get_size_t_size();
size_t memorypa_get_size_t_bit_size();
size_t memorypa_get_size_t_half_bit_size();
size_t memorypa_get_u_char_bit_size();
size_t memorypa_msb(size_t value);
size_t memorypa_get_thread_id();
size_t memorypa_mhash(size_t value);
void * memorypa_malloc(size_t size);
void * memorypa_aligned_malloc(size_t alignment, size_t size);
void * memorypa_calloc(size_t amount, size_t unit_size);
void * memorypa_aligned_calloc(size_t alignment, size_t amount, size_t unit_size);
void * memorypa_realloc(void *data, size_t new_size);
void * memorypa_aligned_realloc(void *data, size_t alignment, size_t new_size);
void memorypa_free(void *data);
size_t memorypa_malloc_usable_size(void *data);
void * memorypa_profile_malloc(size_t size);
void * memorypa_profile_aligned_malloc(size_t alignment, size_t size);
void * memorypa_profile_calloc(size_t amount, size_t unit_size);
void * memorypa_profile_aligned_calloc(size_t alignment, size_t amount, size_t unit_size);
void * memorypa_profile_realloc(void *data, size_t new_size);
void * memorypa_profile_aligned_realloc(void *data, size_t alignment, size_t new_size);
void memorypa_profile_free(void *data);
size_t memorypa_profile_malloc_usable_size(void *data);
void memorypa_profile_print();

#ifndef _MSC_VER
// Intended for overriding by the user (do not define within this library):
void memorypa_initializer_options(memorypa_functions *functions, memorypa_pool_options *sets_of_pool_options);
#endif

#ifdef __cplusplus
}
#endif

#endif
