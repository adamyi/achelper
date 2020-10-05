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

// ac_protoc: my own implementation of Protobuf - naive proto compiler

// NOTES: this is very hacky right now due to the lack of a proper AST.

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXL 4096

struct field {
  unsigned int id;
  int type;
  char name[MAXL];
};

char *types[] = {"double",   "float",    "int32",  "int64",   "uint32",
                 "uint64",   "sint32",   "sint64", "fixed32", "fixed64",
                 "sfixed32", "sfixed64", "bool",   "string",  "bytes"};
char *typesdef[] = {"double",
                    "float",
                    "int32_t",
                    "int64_t",
                    "uint32_t",
                    "uint64_t",
                    "int32_t",
                    "int64_t",
                    "uint32_t",
                    "uint64_t",
                    "int32_t",
                    "int64_t",
                    "bool",
                    "ac_protobuf_string_t",
                    "ac_protobuf_string_t"};
uint32_t wiretype[] = {1, 5, 0, 0, 0, 0, 0, 0, 5, 1, 5, 1, 0, 2, 2};
int typesn = 15;

void write_files(const char *dir, const char *msgname, struct field *fields,
                 int fieldn) {
  char filename[MAXL];
  sprintf(filename, "%s/%s.pb.h", dir, msgname);
  FILE *fout = fopen(filename, "w");
  fprintf(fout, "// clang-format off\n");
  fprintf(fout,
          "// this file is auto-generated with ac_protoc. Do not modify by "
          "hand.\n\n");
  fprintf(fout, "#ifndef AC_HELPER_ACPROTOBUF_MSG_HEADER_%s\n", msgname);
  fprintf(fout, "#define AC_HELPER_ACPROTOBUF_MSG_HEADER_%s\n\n", msgname);
  fprintf(fout, "#include <stdbool.h>\n");
  fprintf(fout, "#include <stdint.h>\n");
  fprintf(fout, "#include \"achelper/ac_protobuf.h\"\n\n");

  fprintf(fout, "struct %s {\n", msgname);
  for (int i = 0; i < fieldn; i++) {
    fprintf(fout, "  %s %s;\n", typesdef[fields[i].type], fields[i].name);
  }
  fprintf(fout, "};\n\n");
  fprintf(fout,
          "struct %s *parse%sFromProtobufMsg(ac_protobuf_message_t *msg);\n",
          msgname, msgname);
  fprintf(fout,
          "struct %s *parse%sFromBytes(uint8_t *bytes, size_t len, size_t "
          "*read);\n",
          msgname, msgname);
  fprintf(fout,
          "ac_protobuf_message_t *encode%sToProtobufMsg(struct %s *msg);\n",
          msgname, msgname);
  fprintf(fout, "uint8_t *encode%sToBytes(struct %s *msg, size_t *len);\n",
          msgname, msgname);
  fprintf(fout, "void free%s(struct %s *val);\n\n", msgname, msgname);
  fprintf(fout, "#endif\n");

  fprintf(fout, "// clang-format on\n");
  fclose(fout);

  sprintf(filename, "%s/%s.pb.c", dir, msgname);
  fout = fopen(filename, "w");
  fprintf(fout, "// clang-format off\n");
  fprintf(fout,
          "// this file is auto-generated with ac_protoc. Do not modify by "
          "hand.\n\n");
  fprintf(fout, "#include <memory.h>\n");
  fprintf(fout, "#include <stdbool.h>\n");
  fprintf(fout, "#include <stdint.h>\n");
  fprintf(fout, "#include <stdlib.h>\n");
  fprintf(fout, "#include \"achelper/ac_protobuf.h\"\n\n");
  fprintf(fout, "#include \"%s.pb.h\"\n\n", msgname);

  fprintf(fout,
          "struct %s *parse%sFromProtobufMsg(ac_protobuf_message_t *msg) {\n",
          msgname, msgname);
  fprintf(fout, "  struct %s *ret = malloc(sizeof(struct %s));\n", msgname,
          msgname);
  for (int i = 0; i < fieldn; i++) {
    fprintf(fout,
            "  ac_protobuf_field_t *%s_f = ac_find_protobuf_field_in_msg(msg, "
            "%u);\n",
            fields[i].name, fields[i].id);
    if (fields[i].type == 13 || fields[i].type == 14) {
      fprintf(fout, "  ret->%s.value = ", fields[i].name);
      fprintf(fout, "malloc(%s_f->len + 1);\n", fields[i].name);
      fprintf(fout, "  memcpy(ret->%s.value, %s_f->value, %s_f->len);\n",
              fields[i].name, fields[i].name, fields[i].name);
      fprintf(fout, "  ret->%s.value[%s_f->len] = 0;\n", fields[i].name,
              fields[i].name);
      fprintf(fout, "  ret->%s.len = %s_f->len;\n", fields[i].name,
              fields[i].name);
    } else {
      fprintf(fout, "  ret->%s = ", fields[i].name);
      fprintf(fout, "*(%s *)(%s_f->value);\n", typesdef[fields[i].type],
              fields[i].name);
    }
  }
  fprintf(fout, "  return ret;\n");
  fprintf(fout, "}\n\n");

  fprintf(fout,
          "struct %s *parse%sFromBytes(uint8_t *bytes, size_t len, size_t "
          "*read) {\n",
          msgname, msgname);
  fprintf(fout,
          "  ac_protobuf_message_t *msg = ac_decode_protobuf_msg(bytes, len, "
          "read);\n");
  fprintf(fout, "  struct %s *ret = parse%sFromProtobufMsg(msg);\n", msgname,
          msgname);
  fprintf(fout, "  ac_protobuf_free_msg(msg);\n");
  fprintf(fout, "  return ret;\n");
  fprintf(fout, "}\n\n");

  fprintf(fout,
          "ac_protobuf_message_t *encode%sToProtobufMsg(struct %s *msg) {\n",
          msgname, msgname);
  fprintf(fout,
          "  ac_protobuf_message_t *ret = "
          "malloc(sizeof(ac_protobuf_message_t));\n");
  if (fieldn == 0) fprintf(fout, "  ret->fields = NULL;\n");
  for (int i = 0; i < fieldn; i++) {
    fprintf(
        fout,
        "  ac_protobuf_field_t *%s_f = malloc(sizeof(ac_protobuf_field_t));\n",
        fields[i].name);
    fprintf(fout, "  %s_f->id = %u;\n", fields[i].name, fields[i].id);
    if (i == 0)
      fprintf(fout, "  ret->fields = %s_f;\n", fields[i].name);
    else
      fprintf(fout, "  %s_f->next = %s_f;\n", fields[i - 1].name,
              fields[i].name);
    if (i == fieldn - 1)
      fprintf(fout, "  %s_f->next = NULL;\n", fields[i].name);
    fprintf(fout, "  %s_f->wiretype = %u;\n", fields[i].name,
            wiretype[fields[i].type]);
    if (wiretype[fields[i].type] == 2) {
      fprintf(fout, "  %s_f->len = msg->%s.len;\n", fields[i].name,
              fields[i].name);
      fprintf(fout, "  %s_f->value = malloc(%s_f->len + 1);\n", fields[i].name,
              fields[i].name);
      fprintf(fout, "  memcpy(%s_f->value, msg->%s.value, %s_f->len);\n",
              fields[i].name, fields[i].name, fields[i].name);
      fprintf(fout, "  *(uint8_t *)(%s_f->value + %s_f->len) = 0;\n",
              fields[i].name, fields[i].name);
    } else if (wiretype[fields[i].type] == 5) {
      fprintf(fout, "  %s_f->value = malloc(4);\n", fields[i].name);
      fprintf(fout, "  *(uint32_t *)(%s_f->value) = (uint32_t)(msg->%s);\n",
              fields[i].name, fields[i].name);
    } else {
      fprintf(fout, "  %s_f->value = malloc(8);\n", fields[i].name);
      fprintf(fout, "  *(uint64_t *)(%s_f->value) = (uint64_t)(msg->%s);\n",
              fields[i].name, fields[i].name);
    }
  }
  fprintf(fout, "  return ret;\n");
  fprintf(fout, "}\n\n");

  fprintf(fout, "uint8_t *encode%sToBytes(struct %s *msg, size_t *len) {\n",
          msgname, msgname);
  fprintf(fout, "  ac_protobuf_message_t *pmsg = encode%sToProtobufMsg(msg);\n",
          msgname);
  fprintf(fout, "  uint8_t *ret = ac_encode_protobuf_msg(pmsg, len);\n");
  fprintf(fout, "  ac_protobuf_free_msg(pmsg);\n");
  fprintf(fout, "  return ret;\n");
  fprintf(fout, "}\n\n");

  fprintf(fout, "void free%s(struct %s *val) {\n", msgname, msgname);
  for (int i = 0; i < fieldn; i++) {
    if (fields[i].type == 13 || fields[i].type == 14) {
      fprintf(fout, "  if (val->%s.value)\n", fields[i].name);
      fprintf(fout, "    ac_free(val->%s.value);\n", fields[i].name);
    }
  }
  fprintf(fout, "  ac_free(val);\n");
  fprintf(fout, "}\n");

  fprintf(fout, "// clang-format on\n");
  fclose(fout);
}

struct field fields[MAXL];

void readFile(const char *filename, const char *target) {
  char buf[MAXL];
  FILE *fin = fopen(filename, "r");
  int state = 0;
  char msgname[MAXL];
  int fieldn = 0;
  while (fgets(buf, MAXL, fin)) {
    char *curr = buf;
    while (isspace(*curr)) curr++;
    if (state == 0) {
      if (strncmp(curr, "message", 7) != 0) continue;
      curr += 7;
      sscanf(curr, "%s {", msgname);
      printf("%s\n", msgname);
      state = 1;
    } else if (state == 1) {
      if (*curr == '}') {
        write_files(target, msgname, fields, fieldn);
        fieldn = 0;
        state = 0;
      } else {
        char type[MAXL];
        sscanf(curr, "%s %s = %u;", type, fields[fieldn].name,
               &(fields[fieldn].id));
        printf("%s\n", fields[fieldn].name);
        for (int i = 0; i < typesn; i++) {
          if (strcmp(type, types[i]) == 0) {
            fields[fieldn++].type = i;
            break;
          }
        }
      }
    }
  }
}

int main(int argc, const char *argv[]) {
  setbuf(stdout, NULL);
  if (argc < 3) {
    fprintf(stderr, "usage: ./protoc-c PROTO_FILES target_dir/\n");
    exit(1);
  }
  for (int i = 1; i < argc - 1; i++) {
    readFile(argv[i], argv[argc - 1]);
  }
}
