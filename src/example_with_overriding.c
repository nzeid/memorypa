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

/*
  Same settings as "example_standard.c" but with overriding.
*/
#ifdef _MSC_VER
__declspec(dllexport) void memorypa_initializer_options(memorypa_functions *functions, memorypa_pool_options *sets_of_pool_options) {
#else
void memorypa_initializer_options(memorypa_functions *functions, memorypa_pool_options *sets_of_pool_options) {
#endif
  /*
    Because we want to override the standard allocation functions, we must
    pull the actual "malloc" and "free" functions from the appropriate
    library:
  */
  #ifdef _MSC_VER
  HINSTANCE win_library = LoadLibrary(TEXT("msvcrt.dll"));
  functions->malloc = (void *(*)(size_t))GetProcAddress(win_library, "malloc");
  functions->free = (void(*)(void*))GetProcAddress(win_library, "free");
  // FreeLibrary(win_library);
  #else
  // functions->malloc = (void *(*)(size_t))(dlsym(RTLD_NEXT, "malloc"));
  // functions->free = (void (*)(void*))(dlsym(RTLD_NEXT, "free"));
  *(void **)(&(functions->malloc)) = dlsym(RTLD_NEXT, "malloc");
  *(void **)(&(functions->free)) = dlsym(RTLD_NEXT, "free");
  #endif
  // See "example_standard.c":
  sets_of_pool_options[0].power = 7;
  sets_of_pool_options[0].amount = 500;
  //
  sets_of_pool_options[1].power = 8;
  sets_of_pool_options[1].amount = 200;
  //
  sets_of_pool_options[2].power = 9;
  sets_of_pool_options[2].amount = 200;
  //
  sets_of_pool_options[3].power = 10;
  sets_of_pool_options[3].amount = 300;
  //
  sets_of_pool_options[4].power = 12;
  sets_of_pool_options[4].amount = 500;
  //
  sets_of_pool_options[4].power = 17;
  sets_of_pool_options[4].amount = 2;
}

// MSVC does not support weak symbols (hence "memorypa_overrider.h"):
#ifndef _MSC_VER

#ifdef __cplusplus
void * malloc(size_t size) throw() {
#else
void * malloc(size_t size) {
#endif
  memorypa_write_message("malloc - ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_decimal(size, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" bytes\n", MEMORYPA_WRITE_OPTION_STDOUT);
  return memorypa_malloc(size);
}

#ifdef __cplusplus
void * calloc(size_t amount, size_t unit_size) throw() {
#else
void * calloc(size_t amount, size_t unit_size) {
#endif
  memorypa_write_message("calloc - ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_decimal(amount, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" of size ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_decimal(unit_size, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" bytes\n", MEMORYPA_WRITE_OPTION_STDOUT);
  return memorypa_calloc(amount, unit_size);
}

#ifdef __cplusplus
void * realloc(void *data, size_t new_size) throw() {
#else
void * realloc(void *data, size_t new_size) {
#endif
  memorypa_write_message("realloc - pointer ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_hex((size_t)data, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" to ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_decimal(new_size, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" bytes\n", MEMORYPA_WRITE_OPTION_STDOUT);
  return memorypa_realloc(data, new_size);
}

#ifdef __cplusplus
void free(void *data) throw() {
#else
void free(void *data) {
#endif
  memorypa_write_message("free - pointer ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_hex((size_t)data, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message("\n", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_free(data);
}

#ifdef __cplusplus
int posix_memalign(void **data_pointer, size_t alignment, size_t size) throw() {
#else
int posix_memalign(void **data_pointer, size_t alignment, size_t size) {
#endif
  memorypa_write_message("posix_memalign - ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_decimal(size, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" bytes with alignment of ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_decimal(alignment, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message("\n", MEMORYPA_WRITE_OPTION_STDOUT);
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
  /*
    "memorypa_initialize" is actually not required in any context. We
    explicitly omit it here because the pooling agent pretty much always
    gets initialized before "main" is even executed due to the
    overriding. E.g. "calloc" is called at some point before "main" is
    executed.
  */
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
  /*
    An additional test for MSVC to make sure overriding actually
    happened. If the "free" segfaults and the printed pointer looks wrong,
    then the overriding "malloc" never made it to the linked libraries,
    which is bad.
  */
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
  /*
    We also omit "memorypa_destroy". If Memorypa is destroyed before
    "main" is complete, then a segfault will occur when "free" is called
    AFTER "main". C++ destructors are guaranteed to cause this, among
    other things.
  */
  return 0;
}
