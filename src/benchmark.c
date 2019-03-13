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
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned long long int ustime() {
  #ifdef _MSC_VER
  LARGE_INTEGER tv, frequency;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&tv);
  tv.QuadPart *= 1000000;
  tv.QuadPart /= frequency.QuadPart;
  return tv.QuadPart;
  #else
  struct timeval tv;
  if(gettimeofday(&tv, NULL) == 0)
    return (unsigned long long int)(tv.tv_sec) * 1000000L + (unsigned long long int)(tv.tv_usec);
  return 0;
  #endif
}

#define MEMORYPA_BENCHMARK_BLOCKS_COUNT 1000

static void time_allocation(unsigned char id) {
  size_t tries = 6000;
  size_t block_size_max = 131072;
  size_t current_block_size;
  size_t malloc_count = 0;
  size_t calloc_count = 0;
  size_t realloc_count = 0;
  size_t free_count = 0;
  void *blocks[MEMORYPA_BENCHMARK_BLOCKS_COUNT];
  size_t blocks_sizes[MEMORYPA_BENCHMARK_BLOCKS_COUNT];
  size_t i = 0;
  do {
    blocks[i] = NULL;
    blocks_sizes[i] = 0;
  }
  while(++i < MEMORYPA_BENCHMARK_BLOCKS_COUNT);
  srand((unsigned int)(ustime()));
  int operation;
  size_t index;
  unsigned long long int start = ustime();
  void *realloc_result;
  i = 0;
  do {
    operation = rand() % 4;
    index = rand() % MEMORYPA_BENCHMARK_BLOCKS_COUNT;
    switch(operation) {
      case 0:
        if(blocks[index] == NULL) {
          current_block_size = (rand() * (index + 1)) % block_size_max + 1;
          blocks[index] = malloc(current_block_size);
          ++malloc_count;
          *((unsigned char *)blocks[index]) = 1;
          blocks_sizes[index] = current_block_size;
        }
        break;
      case 1:
        if(blocks[index] == NULL) {
          current_block_size = (rand() * (index + 1)) % block_size_max + 1;
          blocks[index] = calloc(current_block_size, 1);
          ++calloc_count;
          *((unsigned char *)blocks[index]) = 1;
          blocks_sizes[index] = current_block_size;
          memset(blocks[index], 1, blocks_sizes[index]);
        }
        break;
      case 2:
        if(blocks[index] != NULL) {
          current_block_size = (rand() * (index + 1)) % block_size_max + 1;
          realloc_result = realloc(blocks[index], current_block_size);
          ++realloc_count;
          if(realloc_result != NULL) {
            blocks[index] = realloc_result;
            blocks_sizes[index] = current_block_size;
          }
        }
        break;
      case 3:
        if(blocks[index] == NULL) {
          while(++index < MEMORYPA_BENCHMARK_BLOCKS_COUNT && blocks[index] == NULL);
        }
        if(index < MEMORYPA_BENCHMARK_BLOCKS_COUNT) {
          free(blocks[index]);
          ++free_count;
          blocks[index] = NULL;
          blocks_sizes[index] = 0;
        }
        break;
    }
  }
  while(++i < tries);
  i = 0;
  do {
    if(blocks[i] != NULL) {
      free(blocks[i]);
      ++free_count;
      blocks[i] = NULL;
      blocks_sizes[i] = 0;
    }
  }
  while(++i < MEMORYPA_BENCHMARK_BLOCKS_COUNT);
  printf(
    "T%u: total time: %lluus\n"
    "T%u: malloc count: %zu\n"
    "T%u: calloc count: %zu\n"
    "T%u: realloc count: %zu\n"
    "T%u: free count: %zu\n",
    id, ustime() - start,
    id, malloc_count,
    id, calloc_count,
    id, realloc_count,
    id, free_count
  );
}

#ifdef _MSC_VER
static unsigned __stdcall benchmark_first_thread(void * first_thread_data) {
#else
static void * benchmark_first_thread(void * first_thread_data) {
#endif
  (void)first_thread_data;
  time_allocation(2);
  #ifdef _MSC_VER
  return 0;
  #else
  return NULL;
  #endif
}

int main() {
  #ifdef _MSC_VER
  uintptr_t first_thread_handle = _beginthreadex(NULL, 0, benchmark_first_thread, NULL, 0, NULL);
  if(!first_thread_handle) {
  #else
  pthread_t first_thread_handle;
  if(pthread_create(&first_thread_handle, NULL, benchmark_first_thread, NULL)) {
  #endif
    fprintf(stderr, "Failed to set up the first thread!\n");
    exit(EXIT_FAILURE);
  }
  time_allocation(1);
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
  printf("Done!\n\n");
  return 0;
}
