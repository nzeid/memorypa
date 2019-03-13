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

#ifndef MEMORYPA_OVERRIDER_H
#define MEMORYPA_OVERRIDER_H

#include "memorypa.h"

#ifdef __cplusplus
extern "C" {
#endif

// A place to set alignment globally:
const size_t memorypa_alignment = 8;

void * malloc(size_t size);
void * calloc(size_t amount, size_t unit_size);
void * realloc(void *data, size_t new_size);
void free(void *data);

#ifdef __cplusplus
}
#endif

#endif
