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

/*
  "memorypa_initializer_options" must be defined when including
  "memorypa.h". It's probably best to defined it above "main".
*/
#ifdef _MSC_VER
__declspec(dllexport) void memorypa_initializer_options(memorypa_functions *functions, memorypa_pool_options *sets_of_pool_options) {
#else
void memorypa_initializer_options(memorypa_functions *functions, memorypa_pool_options *sets_of_pool_options) {
#endif
  /*
    Select the Memorypa allocator and deallocator. For standard use,
    simply defer to "malloc" and "free".
  */
  functions->malloc = malloc;
  functions->realloc = realloc;
  functions->free = free;
  /*
    Select the pools that will be put to use and each of their maxima.

    Pools must be set in ascending order.

    If you are introducing Memorypa into an existing program, look at
    "example_profiler.c" or "example_profiler_with_overriding.c". This
    will allow you to obtain the following configuration options with
    little to no guesswork.

    The slab size for each pool is 2^p - 1 where "p" is "power". For
    example:

      sets_of_pool_options[0].power = 7;
      sets_of_pool_options[0].amount = 500;

    This means that there will be 500 slabs available for allocations of
    up to 2^7 - 1 or 127 bytes.

    Note that because power 7 is the first to appear in this particular
    configuration, all the lesser powers will drop into it. E.g. an
    allocation of 62 bytes will be dropped in the power 7 pool even
    though it would otherwise fit in the power 6 pool. Since power 6
    isn't defined, it defaults to power 7. This cascading pattern is
    followed all the way up to the highest pool, in this case power 12.
  */
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
  /*
    Because power 11 isn't defined, all allocations greater than or equal
    to 2^10 and less than 2^11 will default to power 12.
  */
  sets_of_pool_options[4].power = 12;
  sets_of_pool_options[4].amount = 500;
  /*
    Since no other powers are set, any allocation greater than or equal to
    2^12 will generate an error and return NULL.
  */
}

int main() {
  memorypa_initialize();
  char *hello_world = (char *)(memorypa_malloc(14));
  memcpy(hello_world, "Hello world!\n", 14);
  printf("%s", hello_world);
  memorypa_free(hello_world);
  memorypa_destroy();
  return 0;
}
