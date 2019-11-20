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

// ac_protobuf: my own implementation of Protobuf - (de)serialization library

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ac_config.h"
#include "ac_log.h"
#include "ac_memory.h"
#include "ac_protobuf.h"

size_t ac_vbe2uint64(uint8_t *vbe, uint64_t *result, size_t len) {
  *result = 0;
  size_t ret = 0;
  if (len > 8) len = 8;
  do {
    ++ret;
    if (ret > len) {
      ac_log(AC_LOG_INFO,
             "protobuf: cannot decode VBE since there aren't enough bytes in "
             "msg (remaining: %u)",
             len);
      return 0;
    }
    *result = (*result << 7) | (*vbe & 0x7F);
  } while (*(vbe++) & 0x80);
  return ret;
}

size_t ac_uint642vbe(uint64_t val, uint8_t *vbe) {
  if (val == 0) {
    *vbe = 0;
    return 1;
  }
  uint32_t msb;
  for (msb = 63; msb >= 0; msb--) {
    if (val & (1ULL << msb)) break;
  }
  size_t ret = (msb + 1) / 7;
  if (ret * 7 < (msb + 1)) ret++;
  for (int j = ret - 1; j >= 0; j--) {
    if (j == ret - 1)
      vbe[j] = val & 0x7F;
    else
      vbe[j] = 0x80 | (val & 0x7F);
    val >>= 7;
  }
  return ret;
}

size_t ac_decode_protobuf_field(uint8_t *msg, ac_protobuf_field_t **field,
                                size_t len) {
  // NOTES(adamyi@): msgkey should be able to store within uint32_t since
  // fieldnum max size is 2^29 but our API for VB decoding returns uint64_t
  uint64_t msgkey;
  size_t ret = ac_vbe2uint64(msg, &msgkey, len);
  if (ret == 0) return 0;
  ac_protobuf_field_t *f =
      ac_malloc(sizeof(ac_protobuf_field_t), "[ac internal] protobuf field");
  f->id = msgkey >> 3;
  f->wiretype = msgkey & 0x07;
  f->next = NULL;
  switch (f->wiretype) {
    case 0:  // varint
      f->value = ac_malloc(8, "[ac internal] protobuf varint");
      size_t vberead =
          ac_vbe2uint64(msg + ret, (uint64_t *)(f->value), len - ret);
      if (vberead == 0) {
        free(f->value);
        free(f);
        return 0;
      }
      ret += vberead;
      ac_log(AC_LOG_DEBUG, "%d: varint - %lu", f->id, *(uint64_t *)(f->value));
      break;
    case 1:  // 64-bit
      if (len - ret < 8) {
        ac_log(AC_LOG_INFO,
               "protobuf: cannot read 64-bit, remaining bytes %u < 8",
               len - ret);
        free(f);
        return 0;
      }
      f->value = ac_malloc(8, "[ac internal] protobuf 64bit");
      memcpy(f->value, msg + ret, 8);
      ret += 8;
      ac_log(AC_LOG_DEBUG, "%d: 64bit - %lu", f->id, *(uint64_t *)(f->value));
      break;
    case 2:;  // length-delimited
      uint64_t length;
      uint32_t lenlen = ac_vbe2uint64(msg + ret, &length, len - ret);
      if (lenlen == 0) {
        free(f);
        return 0;
      }
      ret += lenlen;
      if (len - ret < length) {
        ac_log(AC_LOG_INFO,
               "protobuf: cannot read %u bytes for length-delimited field, "
               "remaining bytes %u < %u",
               length, len - ret, length);
        free(f);
        return 0;
      }
      f->value =
          ac_malloc(length + 1, "[ac internal] protobuf length-delimited");
      memcpy(f->value, msg + ret, length);
      *((char *)(f->value + length)) = '\0';
      f->len = length;
      ret += length;
      ac_log(AC_LOG_DEBUG, "%d: lendel - %s", f->id, f->value);
      break;
    case 3:  // start group
    case 4:  // end group
      ac_log(AC_LOG_ERROR,
             "Protobuf group is deprecated and hence not implemented in "
             "achelper!");
      break;
    case 5:  // 32-bit
      if (len - ret < 4) {
        ac_log(AC_LOG_INFO,
               "protobuf: cannot read 32-bit, remaining bytes %u < 4",
               len - ret);
        free(f);
        return 0;
      }
      f->value = ac_malloc(4, "[ac internal] protobuf 32bit");
      memcpy(f->value, msg + ret, 4);
      ret += 4;
      ac_log(AC_LOG_DEBUG, "%d: 32bit - %lu", f->id, *(uint32_t *)(f->value));
      break;
  }
  *field = f;
  return ret;
}

ac_protobuf_message_t *ac_decode_protobuf_msg(uint8_t *msg, size_t len,
                                              size_t *readbytes) {
  return ac_decode_protobuf_msg_with_n_fields(msg, len, -1, readbytes);
}

ac_protobuf_message_t *ac_decode_protobuf_msg_with_n_fields(uint8_t *msg,
                                                            size_t len,
                                                            int fields,
                                                            size_t *readbytes) {
  ac_protobuf_message_t *ret =
      ac_malloc(sizeof(ac_protobuf_message_t), "[ac internal] protobuf msg");
  ret->fields = NULL;
  ret->fieldnum = 0;
  ac_protobuf_field_t **next = &(ret->fields);
  size_t read, curr;
  for (curr = 0, read = 0; curr < len && fields != 0; curr += read, fields--) {
    read = ac_decode_protobuf_field(msg + curr, next, len - curr);
    ac_log(AC_LOG_DEBUG, "ac_decode_protobuf_field read %d bytes", read);
    if (read == 0) {
      ac_protobuf_free_msg(ret);
      ac_log(AC_LOG_INFO,
             "ac_decode_protobuf_msg_with_n_fields failed since there aren't "
             "enough bytes");
      return NULL;
    }
    next = &((*next)->next);
  }
  *readbytes = curr;
  return ret;
}

uint8_t *ac_encode_protobuf_msg(ac_protobuf_message_t *msg, size_t *bytes) {
  size_t len = 8;
  ac_protobuf_field_t *curr = msg->fields;
  while (curr != NULL) {
    switch (curr->wiretype) {
      case 0:  // varint
        len += 8;
      case 1:  // 64-bit
        len += 8;
        break;
      case 2:;  // length-delimited
        len += 8 + curr->len;
        break;
      case 3:  // start group
      case 4:  // end group
        ac_log(AC_LOG_ERROR,
               "Protobuf group is deprecated and hence not implemented in "
               "achelper!");
        break;
      case 5:  // 32-bit
        len += 4;
        break;
    }
    curr = curr->next;
  }
  uint8_t *ret = ac_malloc(len, "[ac internal] protobuf encode msg");
  uint8_t *cret = ret;
  curr = msg->fields;
  while (curr != NULL) {
    uint64_t key = curr->id << 3 | curr->wiretype;
    cret += ac_uint642vbe(key, cret);
    switch (curr->wiretype) {
      case 0:  // varint
        cret += ac_uint642vbe(*((uint64_t *)curr->value), cret);
        break;
      case 1:  // 64-bit
        memcpy(cret, curr->value, 8);
        cret += 8;
        break;
      case 2:;  // length-delimited
        cret += ac_uint642vbe(curr->len, cret);
        memcpy(cret, curr->value, curr->len);
        cret += curr->len;
        break;
      case 3:  // start group
      case 4:  // end group
        ac_log(AC_LOG_ERROR,
               "Protobuf group is deprecated and hence not implemented in "
               "achelper!");
        break;
      case 5:  // 32-bit
        memcpy(cret, curr->value, 4);
        cret += 4;
        break;
    }
    curr = curr->next;
  }
  *bytes = (size_t)(cret - ret);
  return ret;
}

ac_protobuf_field_t *ac_find_protobuf_field_in_msg(ac_protobuf_message_t *msg,
                                                   uint32_t fieldnum) {
  ac_protobuf_field_t *curr = msg->fields;
  while (curr != NULL) {
    if (curr->id == fieldnum) return curr;
    curr = curr->next;
  }
  return NULL;
}

void ac_protobuf_free_msg(ac_protobuf_message_t *msg) {
  ac_protobuf_field_t *curr = msg->fields;
  if (curr != NULL) {
    ac_protobuf_field_t *next = curr->next;
    while (curr != NULL) {
      free(curr->value);
      free(curr);
      curr = next;
      if (curr != NULL) next = curr->next;
    }
  }
  free(msg);
}

void ac_protobuf_print_msg(ac_protobuf_message_t *msg) {
  printf("{\n");
  ac_protobuf_field_t *curr = msg->fields;
  while (curr != NULL) {
    printf("  %u (type: %u) = ", curr->id, curr->wiretype);
    switch (curr->wiretype) {
      case 0:  // varint
      case 1:  // 64-bit
        printf("%lu", *((uint64_t *)curr->value));
        break;
      case 2:;  // length-delimited
        size_t dlen = curr->len;
        if (dlen > AC_PROTOBUF_PRINT_BLOB_LEN)
          dlen = AC_PROTOBUF_PRINT_BLOB_LEN;
        char pr[AC_PROTOBUF_PRINT_BLOB_LEN + 1];
        memcpy(pr, curr->value, dlen);
        pr[dlen] = '\0';
        bool str = true;
        for (int i = 0; i < dlen; i++) {
          if (!isprint(pr[i])) {
            str = false;
            break;
          }
        }
        if (str) {
          printf("%s", pr);
        } else {
          for (int i = 0; i < dlen; i++) {
            printf("%02x ", *(uint8_t *)(curr->value + i));
          }
        }
        if (dlen == AC_PROTOBUF_PRINT_BLOB_LEN) {
          printf("... (total len: %zu)", curr->len);
        }
        break;
      case 3:  // start group
        printf("start group");
        break;
      case 4:  // end group
        printf("end group");
        break;
      case 5:  // 32-bit
        printf("%u", *((uint32_t *)curr->value));
        break;
    }
    printf("\n");

    curr = curr->next;
  }
  printf("}\n");
}
