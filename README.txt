
Memorypa
========

Memorypa is a memory pooling agent written in C/C++ that targets the
GNU and Microsoft Visual C++ compiler collections. Include, compile,
and link your program to Memorypa to improve performance. You can even
safely override the C standard allocation functions.



Requirements
------------

- GCC or Microsoft Visual Studio.
- A standard 32-bit or 64-bit architecture/platform.



Rushing Into This
-----------------

Take a quick look at the compiler commands in the build
scripts. They're literally one line for each example. The main use
cases are "example_standard.c" and "example_with_overriding.c".

Look at "example_profiler.c" to understand how to measure your
program's memory usage.

Look at "example_standard.c" for basic usage.

Look at "example_with_overriding.c" for overriding/replacing
allocation functions.



Quick Start for GCC
-------------------

The core Memorypa code is isolated to "memorypa.h" and "memorypa.c".

For 64-bit GCC, compile and test:

  ./clean.sh
  ./build_m64.sh
  ./bin/test_memorypa_c profile
  ./bin/test_memorypa_c
  ./bin/benchmark_c
  ./bin/benchmark_memorypa_c
  ./bin/example_profiler_c
  ./bin/example_standard_c
  ./bin/example_with_overriding_c
  ./bin/example_with_overriding_and_alignment_c

Similarly for 32-bit GCC:

  ./clean.sh
  ./build_m32.sh
  ./bin/test_memorypa_c32 profile
  ./bin/test_memorypa_c32
  ./bin/benchmark_c32
  ./bin/benchmark_memorypa_c32
  ./bin/example_profiler_c32
  ./bin/example_standard_c32
  ./bin/example_with_overriding_c32
  ./bin/example_with_overriding_and_alignment_c32

To link to any 64-bit external project, all you need is:

  ls include/memorypa.h
  ls lib64/libmemorypa.so

Or for 32-bit:

  ls include/memorypa.h
  ls lib32/libmemorypa.so



Quick Start for MSVC
--------------------

Once again, the core Memorypa code is isolated to "memorypa.h" and
"memorypa.c".

"memorypa_overrider.h" and "memorypa_overrider.c" form a tiny DLL used
solely for overriding/replacing allocation functions. Please note that
"memorypa_overrider.h", "memorypa_overrider.c", and
"memorypa_overrider.def" are NOT exhaustive. Currently they only cover
"malloc", "calloc", "realloc", and "free". I'm happy to add more
functions as needed but you can always add them yourself!

"build_win.cmd" was only tested with "cl" under built-in Visual Studio
command prompts. It's possible to compile through the IDE or a basic
command prompt as long as close attention is paid to its "cl"
commands.

To target a specific architecture, simply start the correct MSVC
command prompt. E.g. use "x64 Native Tools Command Prompt" for 64-bit,
and "x86 Native Tools Command Prompt" for 32-bit.

"cd" into the Memorypa folder and run:

  clean.cmd
  build_win.cmd
  .\win\test_memorypa.exe profile
  .\win\test_memorypa.exe
  .\win\benchmark.exe
  .\win\benchmark_memorypa.exe
  .\win\example_profiler.exe
  .\win\example_standard.exe
  .\win\example_with_overriding.exe
  .\win\example_with_overriding_and_alignment.exe

For basic usage, link to any external project with:

  .\include\memorypa.h
  .\win\memorypa.lib
  .\win\memorypa.dll

For overriding/replacing various allocation functions, link using:

  .\include\memorypa.h
  .\include\memorypa_overrider.h
  .\win\memorypa.lib
  .\win\memorypa.dll
  .\win\memorypa_overrider.lib
  .\win\memorypa_overrider.dll



Features
--------

I encourage quick starters to look at the example files, and
enthusiasts to simply read "memorypa.h" and "memorypa.c" (there's not
a lot of code). Memorypa is a compact library that:

- Provides all the C standard dynamic memory allocation functions
  prefixed with "memorypa_", i.e. "memorypa_malloc", "memorypa_calloc",
  "memorypa_realloc", and "memorypa_free".
  - All functions follow the standard.
  - All functions have exactly the same parameters as their counterparts.
  - The allocators do NOT return pointers with "native" alignment. See the
    Warnings section.

- Provides variations of said functions with the prefix
  "memorypa_aligned_" that return aligned pointers. This is purely for
  compatibility. Basic bitwise math is used to waste space in a given
  block so that the right pointer is produced.

- Provides profiler variations of ALL allocation functions.
  - These are all passthrough functions. They directly use the allocation
    functions defined in "memorypa_initializer_options" with significant
    overhead.
  - These are the only functions that affect the output of
    "memorypa_profile_print". See "example_profiler.c".

- Obtains the pool configuration from the user's
  "memorypa_initializer_options" definition.
  - The definition should populate the arguments "functions" and
    "sets_of_pool_options" as shown in the examples.
  - The code that can be placed in this function is entirely up to the
    user. The bundled examples have hardcoded configurations, but an
    fopen/fread strategy for parsing a separate file is easily possible as
    long as strings/arrays are initialized on the stack.

- Allows for perfectly safe execution while overriding the standard
  allocation functions. See "example_with_overriding.c".
  - Use of the profiler functions while overriding is also safe.
  - The given Linux example overrides "malloc", "calloc", "realloc",
    "free", and "posix_memalign". There are other allocation functions
    that are not included here that may need to be overridden.
  - The given Windows example depends on a second library
    "memorypa_overrider.dll" and only overrides "malloc", "calloc",
    "realloc", and "free". There are also other allocation functions that
    may need to be overridden.

- Provides a global pool validator "memorypa_pools_are_invalid" to
  validate the entire allocation. See the Warnings section about the
  best approach to keeping things in top shape.

- Provides potentially useful, cross-platform side functions:
  - "memorypa_get_thread_id" to get the current thread ID.
  - "memorypa_mhash" for a decent multiplicative hash function that adapts
    to the size of "size_t".



Design
------

Memorypa obtains from the operating system literally all its pool
memory literally once. The memory is calculated from the pool
configuration right on the first call to Memorypa. So, the pool
configuration must be established and optimized on separate runs of
the target program. In other words, determine the target program's
peak memory usage using the Memorypa profiling functions, then run it
with the actual pooling functions.

Internalizing memory management this way is not innovative by any
stretch. Peak performance requires eliminating the overhead of context
switches, page faults, and so on caused by repeated use of the
built-ins. And experienced programmers often have no trouble
implementing simple allocators on-the-fly to do just that.

So why Memorypa?

1) Thread safety is performant. Each pool has its own mutex, and the
   mutexes are live locking. Locking is extremely short and never
   explicitly yeilds execution to another process.

2) Memory efficiency is designed carefully around powers of
   two. Allocations are made very quickly using fast and simple
   calculations.

3) The entire suite of functions can be used while overriding the
   built-in allocation functions. Define your own "malloc" and use
   "memorypa_malloc" within it. Memorypa was tested against many
   unexpected situations, like when "malloc" is invoked before "main"
   begins, or "malloc" is called recursively. This all means that
   Memorypa will safely take control of "malloc" from any of the target
   program's external libraries, not just the program itself.

4) It's portable and lightweight. Memorypa currently targets the GNU and
   Visual Studio compilers, and there'll be more if things pick up.



Warnings
--------

The Memorpa profiling functions were intentionally separated from the
actual pooling functions to limit maintenance issues. Use a macro, a
wrapper function, or a function pointer to nickname the Memorypa
functions, then spread that nickname around your code. That way you
can effortlessly switch between profiling and pooling.

Some programs/libraries expect the C standard allocators to return
pointers that are multiples of 4 or 8 or something. The Memorypa pools
live in one massive, contiguous block of memory and maximizing its
efficiency requires this convention to be abandoned. In other words,
the pooling allocation functions that don't specify alignment return
multiples of... anything. However, the profiling functions defer to
the "malloc" and "free" set in the configuration so those will
obviously do whatever the OS is doing. In any case, see
"example_with_overriding_and_alignment.c" for a workaround using the
explicit alignment functions.

Because Memorypa is built around performance, there is basically zero
memory protection. That is in fact one of its optimizing
properties. The trade-off is that any incidents of memory corruption,
namely overflows/overruns, will cause all hell to break loose. The
bundled "memorypa_pools_are_invalid" can be used to incrementally hunt
down some issues but the MUCH better approach is to eject Memorypa and
use established debuggers. The GNU debugger (GDB), Valgrind, and the
built-in Visual Studio debugger are all perfectly good. Once the
target program is rock solid, there will be no problems using
Memorypa. And if that turns out to be a lie (ha!), the build scripts
can be modified to include debugging symbols. So... debug!!!

There is a hidden performance penalty when *unnecessarily* using
"calloc". Built-in allocators lazily allocate memory such that if
"calloc" is called but the target memory is never read or modified,
then no allocation actually occurs. This has no consequences when
using "memorypa_malloc" or "memorypa_realloc", but "memorypa_calloc"
must wipe the memory before providing it. Memorypa has no way of
determining usage ahead of time, so this must be a full wipe every
time. On the other hand, the standard "calloc" only needs to perform
this wipe when the memory is acted upon, and it does so only in
"pages". There are even optimizations that allow it to skip the wipe
entirely. So, if the target program uses "calloc" without actually
doing anything with the provided memory, expect some disappointment
when switching to "memorypa_calloc". Please allocate memory
competently.

The ability to pad the slab size of each pool is very powerful. Pool n
is not stuck accepting slabs of maximum size 2^n - 1. If memory
efficiency is unacceptable, especially at higher pools, use padding to
bucket those allocations into lower pools, saving space.

The "alignment" argument in the "aligned" functions MUST be a power of
2, with a maximum of 32768. Using anything else will result in
undefined behavior.

And finally, as mentioned previously, the given examples for
overriding built-in allocators are currently NOT exhaustive. One might
think "malloc", "calloc", "realloc", and "free" are all there is, but
there are in fact a bunch of other non-standard functions unique to
each OS ("posix_memalign" is in there). These tend to be allocators
that need to interact with "free". These functions simply need to be
located and overridden as well. The examples will be patched as
needed.



Contributing
------------

Please communicate changes over this project's GitLab/GitHub
pages. Bear in mind that the GitLab/GitHub pages are merely mirrors of
a separate "official" repository. Any accepted changes will get due
authorship and credit.

https://gitlab.com/nzeid/memorypa
https://github.com/nzeid/memorypa



License
-------

Copyright (c) 2019 Nader G. Zeid

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/gpl.html>.
