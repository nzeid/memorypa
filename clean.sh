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

if [ "$(printf %.8s "$(basename "$(pwd)")")" != "memorypa" ]; then
  printf "\nExecute the script from the root Memorypa directory.\n\n"
  exit 1;
fi
rm -vf bin/lib32 bin/lib64 bin/benchmark_* bin/example_* bin/test_* lib32/libmemorypa* lib64/libmemorypa*
