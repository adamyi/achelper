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

#include <memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "ac_config.h"
#include "ac_log.h"
#include "ac_memory.h"

#define AC_MEMORY_DEBUG(file, line, values...) \
  if (memconf.logging_enabled) _ac_log(file, line, AC_LOG_DEBUG, values)

static ac_memory_config_t memconf = {.type = AC_MEMORY_LIBC_MALLOC,
                                     .panic_on_oom = false,
                                     .logging_enabled = true};

void ac_set_memory_config(ac_memory_config_t *config) {
  memcpy(&memconf, config, sizeof(ac_memory_config_t));
}

ac_memory_config_t ac_get_memory_config() { return memconf; }

void *_ac_malloc(const char *file, int line, size_t size, const char *name) {
  if (size == 0) {
    AC_MEMORY_DEBUG(file, line,
                    "Tried to allocate memory of size 0 for %s, returned NULL",
                    name);
    return NULL;
  }
  void *ptr;
  switch (memconf.type) {
    case AC_MEMORY_LIBC_MALLOC:
      ptr = malloc(size);
      break;
    case AC_MEMORY_DUMMY_STACK:
      if (memconf.size > size) {
        memconf.size -= size;
        ptr = memconf.region;
        memconf.region += size;
      }
      break;
    default:
      ac_log(AC_LOG_ERROR, "ac_free: memconf->type invalid");
  }

  if (ptr == NULL && memconf.panic_on_oom) {
    _ac_log(file, line, AC_LOG_FATAL,
            "Failed to allocate memory of size %d at %p for %s", size, ptr,
            name);
    __builtin_unreachable();
  }
  AC_MEMORY_DEBUG(file, line, "Allocated memory of size %d at %p for %s", size,
                  ptr, name);
  return ptr;
}

void ac_free(void *ptr) {
  switch (memconf.type) {
    case AC_MEMORY_LIBC_MALLOC:
      free(ptr);
      break;
    case AC_MEMORY_DUMMY_STACK:
      // nothing to do...
      break;
    default:
      ac_log(AC_LOG_ERROR, "ac_free: memconf->type invalid");
  }
}
