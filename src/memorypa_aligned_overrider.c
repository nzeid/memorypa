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
  char buffer[100];
  #ifdef _MSC_VER
  sprintf_s(buffer, 100, "memorypa_aligned_overrider.c - malloc - %zu bytes\n", size);
  _write(1, buffer, strlen(buffer));
  #else
  sprintf(buffer, "memorypa_aligned_overrider.c - malloc - %zu bytes\n", size);
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
  sprintf_s(buffer, 100, "memorypa_aligned_overrider.c - calloc - %zu of size %zu bytes\n", amount, unit_size);
  _write(1, buffer, strlen(buffer));
  #else
  sprintf(buffer, "memorypa_aligned_overrider.c - calloc - %zu of size %zu bytes\n", amount, unit_size);
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
  sprintf_s(buffer, 100, "memorypa_aligned_overrider.c - realloc - pointer %p to %zu bytes\n", data, new_size);
  _write(1, buffer, strlen(buffer));
  #else
  sprintf(buffer, "memorypa_aligned_overrider.c - realloc - pointer %p to %zu bytes\n", data, new_size);
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
    sprintf_s(buffer, 100, "memorypa_aligned_overrider.c - free - pointer %p\n", data);
    _write(1, buffer, strlen(buffer));
    #else
    sprintf(buffer, "memorypa_aligned_overrider.c - free - pointer %p\n", data);
    write(1, buffer, strlen(buffer));
    #endif
  }
  memorypa_free(data);
}
