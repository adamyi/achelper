// This file is part of achelper, Adam's C helper written for UNSW assignments.
// Copyright 2018 Adam Yi <i@adamyi.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ac_config.h"
#include "ac_constant.h"

#ifndef AC_HELPER_ACMEMORY_H_
#define AC_HELPER_ACMEMORY_H_

#define ac_malloc(values...) _ac_malloc(__FILE__, __LINE__, values)

#define AC_MEMORY_LIBC_MALLOC 0  // delegate job to default malloc
#define AC_MEMORY_DUMMY_STACK \
  1  // an extremely naive memory allocator, with a pre-allocated stack region

// TODO: implement my own heap

typedef struct ac_memory_config {
  uint8_t type;
  bool logging_enabled;
  bool panic_on_oom;
  void *region;
  size_t size;
} ac_memory_config_t;

void ac_set_memory_config(ac_memory_config_t *config);
ac_memory_config_t ac_get_memory_config();

void *_ac_malloc(const char *file, int line, size_t size, const char *name);
void ac_free(void *ptr);

#endif  // AC_HELPER_ACMEMORY_H_
