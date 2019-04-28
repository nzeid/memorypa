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

#include "memorypa_overrider.h"

#ifdef __cplusplus
void * malloc(size_t size) throw() {
#else
void * malloc(size_t size) {
#endif
  memorypa_write_message("memorypa_aligned_overrider.c - malloc - ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_decimal(size, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" bytes\n", MEMORYPA_WRITE_OPTION_STDOUT);
  return memorypa_aligned_malloc(memorypa_alignment, size);
}

#ifdef __cplusplus
void * calloc(size_t amount, size_t unit_size) throw() {
#else
void * calloc(size_t amount, size_t unit_size) {
#endif
  memorypa_write_message("memorypa_aligned_overrider.c - calloc - ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_decimal(amount, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" of size ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_decimal(unit_size, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" bytes\n", MEMORYPA_WRITE_OPTION_STDOUT);
  return memorypa_aligned_calloc(memorypa_alignment, amount, unit_size);
}

#ifdef __cplusplus
void * realloc(void *data, size_t new_size) throw() {
#else
void * realloc(void *data, size_t new_size) {
#endif
  memorypa_write_message("memorypa_aligned_overrider.c - realloc - pointer ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_hex((size_t)data, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" to ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_decimal(new_size, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message(" bytes\n", MEMORYPA_WRITE_OPTION_STDOUT);
  return memorypa_aligned_realloc(data, memorypa_alignment, new_size);
}

#ifdef __cplusplus
void free(void *data) throw() {
#else
void free(void *data) {
#endif
  memorypa_write_message("memorypa_aligned_overrider.c - free - pointer ", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_hex((size_t)data, 0, MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_write_message("\n", MEMORYPA_WRITE_OPTION_STDOUT);
  memorypa_free(data);
}
