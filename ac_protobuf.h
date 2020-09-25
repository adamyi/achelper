// This file is part of achelper, Adam's C helper written for uni assignments.
// Copyright 2019 Adam Yi <i@adamyi.com>
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

// ac_protobuf: my own implementation of Protobuf

#include <stddef.h>
#include <stdint.h>

#ifndef AC_HELPER_ACPROTOBUF_H_
#define AC_HELPER_ACPROTOBUF_H_

#define AC_PROTOBUF_PRINT_BLOB_LEN 100

typedef struct ac_protobuf_field {
  uint32_t id;
  uint32_t wiretype;
  void *value;
  size_t len;
  struct ac_protobuf_field *next;
} ac_protobuf_field_t;

typedef struct ac_protobuf_message {
  ac_protobuf_field_t *fields;
  size_t fieldnum;
} ac_protobuf_message_t;

typedef struct ac_protobuf_string {
  uint8_t *value;
  size_t len;
} ac_protobuf_string_t;

void ac_protobuf_enable_logging();
void ac_protobuf_disable_logging();

size_t ac_vbe2uint64(uint8_t *vbe, uint64_t *result, size_t len);
size_t ac_uint642vbe(uint64_t val, uint8_t *vbe);
size_t ac_decode_protobuf_field(uint8_t *msg, ac_protobuf_field_t **field,
                                size_t len);
ac_protobuf_message_t *ac_decode_protobuf_msg(uint8_t *msg, size_t bytes,
                                              size_t *readbytes);
ac_protobuf_message_t *ac_decode_protobuf_msg_with_n_fields(uint8_t *msg,
                                                            size_t len,
                                                            int fields,
                                                            size_t *readbytes);
ac_protobuf_field_t *ac_find_protobuf_field_in_msg(ac_protobuf_message_t *msg,
                                                   uint32_t fieldnum);
uint8_t *ac_encode_protobuf_msg(ac_protobuf_message_t *msg, size_t *len);
void ac_protobuf_free_msg(ac_protobuf_message_t *msg);
void ac_protobuf_print_msg(ac_protobuf_message_t *msg);

#endif  // AC_HELPER_ACPROTOBUF_H_
