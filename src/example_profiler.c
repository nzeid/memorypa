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
  functions->free = free;
  /*
    Since we're only profiling, the only thing that needs to be configured
    (if at all) is padding. Padding increases the size of the given power
    by the number specified in bytes. In the following example, power 7 by
    default has 127-byte slabs, but the padding of 8 makes it store
    135-byte slabs. The profiler will properly bucket allocations between
    128 and 135 into power 7.

    If a large configuration is already defined, there's no problem. You
    can simply use the profiler functions in-place. The existing
    configuration won't interfere with the output.

    Power 6 is also specified here merely to prevent the padding of power
    7 from applying to lower powers. See "example_standard.c" for an
    explanation of how pool cascading works.
  */
  sets_of_pool_options[0].power = 6;
  sets_of_pool_options[0].padding = 0;
  sets_of_pool_options[1].power = 7;
  sets_of_pool_options[1].padding = 8;
}

int main() {
  memorypa_initialize();
  /*
    Note the "memorypa_profile_malloc". Size 14 will produce output for
    power 4.
  */
  char *hello_world = (char *)(memorypa_profile_malloc(14));
  memcpy(hello_world, "Hello world!\n", 14);
  printf("%s", hello_world);
  /*
    Note the "memorypa_profile_free". The profile family of functions must
    be used together.
  */
  memorypa_profile_free(hello_world);
  /*
    To test the padding defined above, allocate 135 bytes then 136
    bytes. The former will fall into power 7, the latter into power 8.
  */
  void *within_padding = memorypa_profile_malloc(134);
  void *exceeds_padding = memorypa_profile_malloc(138);
  memorypa_profile_free(within_padding);
  memorypa_profile_free(exceeds_padding);
  // Print the profile:
  memorypa_profile_print();
  //
  memorypa_destroy();
  return 0;
}
