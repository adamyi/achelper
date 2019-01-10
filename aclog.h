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

#include "acconfig.h"
#include "acconstant.h"

#ifndef ACHELPER_ACLOG_H_
#define ACHELPER_ACLOG_H_

#define ACLOG_FATAL 0
#define ACLOG_ERROR 1
#define ACLOG_INFO 2
#define ACLOG_DEBUG 3

#define ac_log(values...) _ac_log(__LINE__, values)

void ac_setLoggingTag(const char *tag);
char *ac_getLoggingTag();

void _ac_log(int line, int level, char *fmt, ...);

#endif  // ACHELPER_ACLOG_H_
