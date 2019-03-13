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
#include "memorypa_overrider.h"
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <errno.h>
#include "memorypa.h"
#endif

// For C++ compatibility:
#ifdef __cplusplus
extern "C" {
#endif

// "memorypa_overrider.h" declares "memorypa_alignment":
#ifndef _MSC_VER
// Declare memory alignment in exactly one place:
static size_t memorypa_alignment = 8;
#endif

/*
  Same settings as "example_standard.c" but with overriding.
*/
#ifdef _MSC_VER
__declspec(dllexport) void memorypa_initializer_options(memorypa_functions *functions, memorypa_pool_options *sets_of_pool_options) {
#else
void memorypa_initializer_options(memorypa_functions *functions, memorypa_pool_options *sets_of_pool_options) {
#endif
  // See "example_with_overriding.c":
  #ifdef _MSC_VER
  HINSTANCE win_library = LoadLibrary(TEXT("msvcrt.dll"));
  functions->malloc = (void *(*)(size_t))GetProcAddress(win_library, "malloc");
  functions->free = (void (*)(void*))GetProcAddress(win_library, "free");
  // FreeLibrary(win_library);
  #else
  // functions->malloc = (void *(*)(size_t))(dlsym(RTLD_NEXT, "malloc"));
  // functions->free = (void (*)(void*))(dlsym(RTLD_NEXT, "free"));
  *(void **)(&(functions->malloc)) = dlsym(RTLD_NEXT, "malloc");
  *(void **)(&(functions->free)) = dlsym(RTLD_NEXT, "free");
  #endif
  /*
    As shown in "example_standard.c", but we introduce padding to make way
    for alignment. Padding isn't actually necessary, but combined with the
    aligned family of allocators it will result in a profile that's
    identical to one without alignment. The drawback of course is that it
    increases memory usage.

    It's up to you to decide how to use padding to optimize memory usage.
  */
  sets_of_pool_options[0].power = 7;
  sets_of_pool_options[0].padding = memorypa_alignment;
  sets_of_pool_options[0].amount = 500;
  //
  sets_of_pool_options[1].power = 8;
  sets_of_pool_options[1].padding = memorypa_alignment;
  sets_of_pool_options[1].amount = 200;
  //
  sets_of_pool_options[2].power = 9;
  sets_of_pool_options[2].padding = memorypa_alignment;
  sets_of_pool_options[2].amount = 200;
  //
  sets_of_pool_options[3].power = 10;
  sets_of_pool_options[3].padding = memorypa_alignment;
  sets_of_pool_options[3].amount = 300;
  //
  sets_of_pool_options[4].power = 12;
  sets_of_pool_options[3].padding = memorypa_alignment;
  sets_of_pool_options[4].amount = 500;
  //
  sets_of_pool_options[4].power = 17;
  sets_of_pool_options[3].padding = memorypa_alignment;
  sets_of_pool_options[4].amount = 2;
}

// MSVC does not support weak symbols (hence "memorypa_overrider.h"):
#ifndef _MSC_VER

#ifdef __cplusplus
void * malloc(size_t size) throw() {
#else
void * malloc(size_t size) {
#endif
  char buffer[100];
  #ifdef _MSC_VER
  sprintf_s(buffer, 100, "aligned malloc - %zu bytes\n", size);
  _write(1, buffer, strlen(buffer));
  #else
  sprintf(buffer, "aligned malloc - %zu bytes\n", size);
  write(1, buffer, strlen(buffer));
  #endif
  return memorypa_aligned_malloc(memorypa_alignment, size);
}

#ifdef __cplusplus
void * calloc(size_t amount, size_t unit_size) throw() {
#else
void * calloc(size_t amount, size_t unit_size) {
#endif
  char buffer[100];
  #ifdef _MSC_VER
  sprintf_s(buffer, 100, "aligned calloc - %zu of size %zu bytes\n", amount, unit_size);
  _write(1, buffer, strlen(buffer));
  #else
  sprintf(buffer, "aligned calloc - %zu of size %zu bytes\n", amount, unit_size);
  write(1, buffer, strlen(buffer));
  #endif
  return memorypa_aligned_calloc(memorypa_alignment, amount, unit_size);
}

#ifdef __cplusplus
void * realloc(void *data, size_t new_size) throw() {
#else
void * realloc(void *data, size_t new_size) {
#endif
  char buffer[100];
  #ifdef _MSC_VER
  sprintf_s(buffer, 100, "aligned realloc - pointer %p to %zu bytes\n", data, new_size);
  _write(1, buffer, strlen(buffer));
  #else
  sprintf(buffer, "aligned realloc - pointer %p to %zu bytes\n", data, new_size);
  write(1, buffer, strlen(buffer));
  #endif
  return memorypa_aligned_realloc(data, memorypa_alignment, new_size);
}

#ifdef __cplusplus
void free(void *data) throw() {
#else
void free(void *data) {
#endif
  if(data != NULL) {
    char buffer[100];
    #ifdef _MSC_VER
    sprintf_s(buffer, 100, "free - pointer %p\n", data);
    _write(1, buffer, strlen(buffer));
    #else
    sprintf(buffer, "free - pointer %p\n", data);
    write(1, buffer, strlen(buffer));
    #endif
  }
  memorypa_free(data);
}

#ifdef __cplusplus
int posix_memalign(void **data_pointer, size_t alignment, size_t size) throw() {
#else
int posix_memalign(void **data_pointer, size_t alignment, size_t size) {
#endif
  char buffer[100];
  #ifdef _MSC_VER
  sprintf_s(buffer, 100, "posix_memalign - %zu bytes with alignment of %zu\n", size, alignment);
  _write(1, buffer, strlen(buffer));
  #else
  sprintf(buffer, "posix_memalign - %zu bytes with alignment of %zu\n", size, alignment);
  write(1, buffer, strlen(buffer));
  #endif
  *data_pointer = memorypa_aligned_malloc(alignment, size);
  return (*data_pointer == NULL) ? ENOMEM : 0;
}

// MSVC does not support weak symbols (see above):
#endif

// Also for C++ compatibility:
#ifdef __cplusplus
}
#endif

int main() {
  // See "example_with_overriding.c" about "memorypa_initialize()".
  // malloc:
  char *hello_world = (char *)(malloc(14));
  memcpy(hello_world, "Hello world!\n", 14);
  printf("%s", hello_world);
  free(hello_world);
  // calloc:
  hello_world = (char *)(calloc(2, 7));
  memcpy(hello_world, "Hello world!\n", 14);
  printf("%s", hello_world);
  // realloc:
  hello_world = (char *)realloc(hello_world, 16);
  memcpy(hello_world, "Hello, worlds!\n", 16);
  printf("%s", hello_world);
  free(hello_world);
  #ifdef _MSC_VER
  // See "example_with_overriding.c"... this test is useful!
  hello_world = _strdup("Hello, worlds all over!\n");
  printf("%s", hello_world);
  free(hello_world);
  #else
  // posix_memalign:
  void **hello_world_p = (void **)(&hello_world);
  if(posix_memalign(hello_world_p, 8, 14)) {
    printf("An error has occurred with \"posix_memalign\"!\n");
  }
  else {
    memcpy(hello_world, "Hello world!\n", 14);
    printf("%s", hello_world);
    free(hello_world);
  }
  #endif
  // See "example_with_overriding.c" about "memorypa_destroy()".
  return 0;
}
