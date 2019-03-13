#! /bin/sh

# Copyright (c) 2019 Nader G. Zeid
#
# This file is part of Memorypa.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Memorypa. If not, see <https://www.gnu.org/licenses/gpl.html>.

if [ "$(printf %.8s "`basename $(pwd)`")" != "memorypa" ]; then
  printf "\nExecute the script from the root Memorypa directory.\n\n"
  exit 1;
fi
rm -f bin/lib32
ln -sn ../lib32 bin/lib32
gcc -c -O3 -std=c11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -fPIC -I./include -o lib32/libmemorypa.o src/memorypa.c
gcc -shared -std=c11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -o lib32/libmemorypa.so lib32/libmemorypa.o -lc
printf "gcc -m32 ...\n"
gcc -std=c11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/test_memorypa.c -o bin/test_memorypa_c32 -L./lib32 -lmemorypa -lpthread
gcc -std=c11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 src/benchmark.c -o bin/benchmark_c32 -lpthread -lc
gcc -std=c11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/benchmark_memorypa.c -o bin/benchmark_memorypa_c32 -L./lib32 -lmemorypa -lpthread
gcc -std=c11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/example_standard.c -o bin/example_standard_c32 -L./lib32 -lmemorypa
gcc -std=c11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/example_profiler.c -o bin/example_profiler_c32 -L./lib32 -lmemorypa
gcc -std=c11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/example_with_overriding.c -o bin/example_with_overriding_c32 -L./lib32 -lmemorypa -ldl
gcc -std=c11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/example_with_overriding_and_alignment.c -o bin/example_with_overriding_and_alignment_c32 -L./lib32 -lmemorypa -ldl
printf "g++ -m32 ...\n"
g++ -std=c++11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/test_memorypa.c -o bin/test_memorypa_cpp32 -L./lib32 -lmemorypa -lpthread
g++ -std=c++11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 src/benchmark.c -o bin/benchmark_cpp32 -lpthread -lc
g++ -std=c++11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/benchmark_memorypa.c -o bin/benchmark_memorypa_cpp32 -L./lib32 -lmemorypa -lpthread
g++ -std=c++11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/example_standard.c -o bin/example_standard_cpp32 -L./lib32 -lmemorypa
g++ -std=c++11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/example_profiler.c -o bin/example_profiler_cpp32 -L./lib32 -lmemorypa
g++ -std=c++11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/example_with_overriding.c -o bin/example_with_overriding_cpp32 -L./lib32 -lmemorypa -ldl
g++ -std=c++11 -Wpedantic -Wall -Wextra -Wpointer-arith -Werror=vla -m32 -Wl,-rpath,\$ORIGIN/lib32 -I./include src/example_with_overriding_and_alignment.c -o bin/example_with_overriding_and_alignment_cpp32 -L./lib32 -lmemorypa -ldl
