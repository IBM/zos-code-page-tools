/**********************************************************************
 * Copyright 2022 IBM Corp.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 *  either express or implied. See the License for the specific
 *  language governing permissions and limitations under the
 *  License.
 *
 * -----------------------------------------------------------------
 *
 * Disclaimer of Warranties:
 *
 *   The following enclosed code is sample code created by IBM
 *   Corporation.  This sample code is not part of any standard
 *   IBM product and is provided to you solely for the purpose
 *   of assisting you in the development of your applications.
 *   The code is provided "AS IS", without warranty of any kind.
 *   IBM shall not be liable for any damages arising out of your
 *   use of the sample code, even if they have been advised of
 *   the possibility of such damages.
 *
 * -----------------------------------------------------------------
 *
 *  Program: cat2
 *
 *  Purpose: print file (mix of ascii and ebcdic) with autoconversion if
 * possible.
 *
 *  Syntax: cat2 [files ... ]
 *
 */
// build with
// /gsa/tlbgsa/projects/x/xlcmpbld/run/vacpp/dev/os390/daily/D171215/xlclang
// -qasm -DDOUBLE_CONVERT=1 -q64 -o cat2 cat2.c
//
#define _XOPEN_SOURCE 600
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static inline void *__convert_one_to_one(const void *tbl, void *dst,
                                         size_t size, const void *src) {
  int i;
  unsigned char *target = (unsigned char *)dst;
  const unsigned char *source = (const unsigned char *)src;
  const unsigned char *table = (const unsigned char *)tbl;
  for (i = 0; i < size; ++i) {
    target[i] = table[source[i]];
  }
  return target;
}
#if __MVS__
#include <_Nascii.h>
static int __setfdccsid(int fd, int t_ccsid) {
  attrib_t attr;
  memset(&attr, 0, sizeof(attr));
  attr.att_filetagchg = 1;
  attr.att_filetag.ft_txtflag = (t_ccsid >> 16);
  attr.att_filetag.ft_ccsid = (t_ccsid & 0x0ffff);
  return __fchattr(fd, &attr, sizeof(attr));
}
#else
static int __setfdccsid(int fd, int t_ccsid) { return 0; }
#endif

void unblock(int fd) { fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK); }
void block(int fd) { fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK); }

#if defined(TABLE_BUILD)
static const unsigned char a2e[256] __attribute__((aligned(8))) = {
    0x00, 0x01, 0x02, 0x03, 0x37, 0x2d, 0x2e, 0x2f, 0x16, 0x05, 0x15, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x3c, 0x3d, 0x32, 0x26,
    0x18, 0x19, 0x3f, 0x27, 0x1c, 0x1d, 0x1e, 0x1f, 0x40, 0x5a, 0x7f, 0x7b,
    0x5b, 0x6c, 0x50, 0x7d, 0x4d, 0x5d, 0x5c, 0x4e, 0x6b, 0x60, 0x4b, 0x61,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0x7a, 0x5e,
    0x4c, 0x7e, 0x6e, 0x6f, 0x7c, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xad, 0xe0, 0xbd, 0x5f, 0x6d,
    0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x91, 0x92,
    0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6,
    0xa7, 0xa8, 0xa9, 0xc0, 0x4f, 0xd0, 0xa1, 0x07, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x06, 0x17, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x09, 0x0a, 0x1b,
    0x30, 0x31, 0x1a, 0x33, 0x34, 0x35, 0x36, 0x08, 0x38, 0x39, 0x3a, 0x3b,
    0x04, 0x14, 0x3e, 0xff, 0x41, 0xaa, 0x4a, 0xb1, 0x9f, 0xb2, 0x6a, 0xb5,
    0xbb, 0xb4, 0x9a, 0x8a, 0xb0, 0xca, 0xaf, 0xbc, 0x90, 0x8f, 0xea, 0xfa,
    0xbe, 0xa0, 0xb6, 0xb3, 0x9d, 0xda, 0x9b, 0x8b, 0xb7, 0xb8, 0xb9, 0xab,
    0x64, 0x65, 0x62, 0x66, 0x63, 0x67, 0x9e, 0x68, 0x74, 0x71, 0x72, 0x73,
    0x78, 0x75, 0x76, 0x77, 0xac, 0x69, 0xed, 0xee, 0xeb, 0xef, 0xec, 0xbf,
    0x80, 0xfd, 0xfe, 0xfb, 0xfc, 0xba, 0xae, 0x59, 0x44, 0x45, 0x42, 0x46,
    0x43, 0x47, 0x9c, 0x48, 0x54, 0x51, 0x52, 0x53, 0x58, 0x55, 0x56, 0x57,
    0x8c, 0x49, 0xcd, 0xce, 0xcb, 0xcf, 0xcc, 0xe1, 0x70, 0xdd, 0xde, 0xdb,
    0xdc, 0x8d, 0x8e, 0xdf};

unsigned char e2a[256];

const char *isescape(unsigned int i) {
  switch (i) {
  case 0x0a:
    return "\\n";
  case 0x0d:
    return "\\r";
  case 0x09:
    return "\\t";
  case 0x5c:
    return "\\\\";
  case 0x27:
    return "\\'";
  case 0x22:
    return "\\\"";
  case 0x3f:
    return "\\?";
#if 1 // although these escapse sequences are legit, they are almost never
      // used.
  case 0x07:
    return "\\a";
  case 0x08:
    return "\\b";
  case 0x0c:
    return "\\f";
  case 0x0b:
    return "\\v";
#endif
  }
  return 0;
}

#if ' ' != 0x20
#error wrong platform for table creation
#endif

void gen_conv_table(const char *name,
                    unsigned char (*converter)(unsigned char)) {
  unsigned int i;
  printf("static const unsigned char %s [256] = {\n", name);
  for (i = 0; i < 256; ++i) {
    if (0 == (i) % 8) {
      printf("/* %02x */ ", i);
    }
    printf("0x%02x, ", converter(i));
    if (0 == (i + 1) % 8) {
      printf("\n");
    }
  }
  if (0 != (i + 1) % 8) {
    printf("\n");
  }
  printf("};\n");
}
void gen_print_table(const char *name,
                     unsigned char (*converter)(unsigned char)) {
  unsigned int i;
  printf("static const unsigned char %s [256] = {\n", name);
  for (i = 0; i < 256; ++i) {
    unsigned char result = converter(i);
    if (0 == (i) % 8) {
      printf("/* %02x */ ", i);
    }
    const char *str = isescape(result);
    if (str) {
      printf("'%s', ", str);
    } else {
      if (isprint(result))
        printf("'%c',  ", result);
      else
        printf("0,    ");
    }
    if (0 == (i + 1) % 8) {
      printf("\n");
    }
  }
  if (0 != (i + 1) % 8) {
    printf("};\n");
  } else
    printf("};\n");
}

void gen_preference_table(const char *name) {
  unsigned int i;
  printf("static const unsigned char %s [256] = {\n", name);

  printf("#if ' ' == 0x20\n");
  for (i = 0; i < 256; ++i) {
    if (0 == (i) % 8) {
      printf("/* %02x */ ", i);
    }
    if (((i >= 'a' && i <= 'i') || (i >= 'j' && i <= 'r') ||
         (i >= 's' && i <= 'z')) ||
        ((i >= 'A' && i <= 'I') || (i >= 'J' && i <= 'R') ||
         (i >= 'S' && i <= 'Z'))) {
      printf("5,    ");
    } else if (i == ' ') {
      printf("4,    ");
    } else if (i >= '0' && i <= '9') {
      printf("3,    ");
    } else if (i == '?' || i == '.' || i == ';') {
      printf("2,    ");
    } else if (i == '<' || i == '+' || i == '|' || i == '&' || i == '!' ||
               i == '$' || i == '*' || i == ')' || i == '^' || i == '-' ||
               i == '/' || i == ',' || i == '%' || i == '>' || i == ':' ||
               i == '#' || i == '@' || i == '\'' || i == '=' || i == '\\' ||
               i == '[' || i == ']' || i == '~' || i == '{' || i == '}') {
      printf("1,    ");
    } else {
      printf("0,    ");
    }
    if (0 == (i + 1) % 8) {
      printf("\n");
    }
  }
  printf("#else\n");
  for (i = 0; i < 256; ++i) {
    if (0 == (i) % 8) {
      printf("/* %02x */ ", i);
    }
    unsigned char t = e2a[i];
    if (((t >= 'a' && t <= 'i') || (t >= 'j' && t <= 'r') ||
         (t >= 's' && t <= 'z')) ||
        ((t >= 'A' && t <= 'I') || (t >= 'J' && t <= 'R') ||
         (t >= 'S' && t <= 'Z'))) {
      printf("5,    ");
    } else if (t == ' ') {
      printf("4,    ");
    } else if (t >= '0' && t <= '9') {
      printf("3,    ");
    } else if (t == '?' || t == '.' || t == ';') {
      printf("2,    ");
    } else if (t == '<' || t == '+' || t == '|' || t == '&' || t == '!' ||
               t == '$' || t == '*' || t == ')' || t == '^' || t == '-' ||
               t == '/' || t == ',' || t == '%' || t == '>' || t == ':' ||
               t == '#' || t == '@' || t == '\'' || t == '=' || t == '\\' ||
               t == '[' || t == ']' || t == '~' || t == '{' || t == '}') {
      printf("1,    ");
    } else {
      printf("0,    ");
    }
    if (0 == (i + 1) % 8) {
      printf("\n");
    }
  }
  printf("#endif\n");

  if (0 != (i + 1) % 8) {
    printf("};\n");
  } else
    printf("};\n");
}

unsigned char conv_null(unsigned char in) { return in; }
unsigned char conv_a2e(unsigned char in) { return a2e[in]; }
unsigned char conv_e2a(unsigned char in) { return e2a[in]; }
unsigned char conv_a22e(unsigned char in) { return a2e[a2e[in]]; }
unsigned char conv_e22a(unsigned char in) { return e2a[e2a[in]]; }
unsigned char conv_invalid(unsigned char in) {
  if (isprint(in) || isescape(in))
    return 0;
  if (isprint(e2a[in]) || isescape(e2a[in]))
    return 0;
#if defined(DOUBLE_CONVERT)
  if (isprint(a2e[in]) || isescape(a2e[in]))
    return 0;
  if (isprint(e2a[e2a[in]]) || isescape(e2a[e2a[in]]))
    return 0;
#endif
  return '.';
}

int main() {
  unsigned int i;
  unsetenv("_BPXK_AUTOCVT");
  memset(e2a, 0, 256);
  for (i = 0; i < 256; ++i) {
    e2a[a2e[i]] = i;
  }
  gen_conv_table("a2e", conv_a2e);
  gen_conv_table("e2a", conv_e2a);
  gen_conv_table("a22e", conv_a2e);
  gen_conv_table("e22a", conv_e2a);
  //  gen_conv_table("inv", conv_invalid);

  gen_print_table("print_ascii", conv_null);
  gen_print_table("print_ebcdic", conv_e2a);
#if defined(DOUBLE_CONVERT)
  gen_print_table("print_ascii2", conv_a2e);
  gen_print_table("print_ebcdic2", conv_e22a);
#endif
  gen_print_table("print_invalid", conv_invalid);
}
#else // TABLE_BUILD
#include "cat2.h"
int help(int argc, char **argv) {
  fprintf(stderr, "\n\
Usage: %s [OPTION]... [FILE]...\n\
Concatenate FILE(s) to standard output in human readable form.\n\
\n\
With no FILE, or when FILE is -, read standard input.\n\
\n\
\n\
  -o [logfile]             save raw input to file [logfile]\n\
  --help                   show this dialog\n\
  -help                    show this dialog\n\
  -a                       output in ASCII\n\
  -e                       output in EBCDIC\n\
  -2                       output in file descriptor 2 (stderr)\n\
\n\
Examples:\n\
  %s -o rawdata.txt f - g\n\
                          Convert f's contents, then standard input,\n\
                          then g's contents to terminal, input data saved in rawdata.txt.\n\
  %s\n\
                          Convert standard input to standard output.\n\
\n\
",
          argv[0], argv[0], argv[0]);
  return -1;
}
static int orig_conv_state;
static int ebcdic_out;
static int outfd = 1;
static int iam_ebcdic;
static int files = 0;
static int rawfd = -1;
#if defined(DOUBLE_CONVERT)
#define tabcount 5
static const unsigned char *tables[tabcount] = {
    print_ascii, print_ebcdic, print_ascii2, print_ebcdic2, print_invalid};
#else
#define tabcount 3
static const unsigned char *tables[tabcount] = {print_ascii, print_ebcdic,
                                                print_invalid};
#endif

#define slide_width 256

void wrline(unsigned char *str, int len) {
  unsigned char tr[slide_width];
  if (ebcdic_out) {
    if (iam_ebcdic) {
      write(outfd, str, len);
    } else {
      for (int i = 0; i < len; ++i) {
        tr[i] = a2e[str[i]];
      }
      write(outfd, tr, len);
    }
  } else {
    if (iam_ebcdic) {
      for (int i = 0; i < len; ++i) {
        tr[i] = e2a[str[i]];
      }
      write(outfd, tr, len);
    } else {
      write(outfd, str, len);
    }
  }
}

struct slide_buf {
  unsigned int offset;
  unsigned int count;
  unsigned char *buffer[tabcount];
  unsigned char _buffer[tabcount * (slide_width + 2)];
  int eof;
};

void init_buf(int fd, struct slide_buf *buf) {
  memset(buf, 0, sizeof(struct slide_buf));
  unsigned char *begin = &(buf->_buffer[0]);
  for (int i = 0; i < tabcount; ++i) {
    buf->buffer[i] = begin;
    begin += (slide_width + 1);
  }
}

void refill_buf(int fd, struct slide_buf *buf) {
  char ch;
  int rc;
  int i, tab;
  if (buf->offset > 0) {
    if (buf->count > 0) {
      for (i = 0; i < tabcount; ++i) {
        memmove(&(buf->buffer[i][0]), &(buf->buffer[i][buf->offset]),
                buf->count);
        buf->buffer[i][buf->count] = 0;
      }
    }
    buf->offset = 0;
  }
  if (buf->eof) {
    return;
  }

  if (buf->count < slide_width) {
    unsigned sz = slide_width - buf->count;
    unsigned char localbuf[slide_width + 1];
    if (buf->count == 0) {
      block(fd);
      rc = read(fd, localbuf, sz);
      unblock(fd);
    } else {
      rc = read(fd, localbuf, sz);
    }
    if (rc > 0) {
      if (rawfd != -1)
        write(rawfd, localbuf, rc);
      localbuf[rc] = 0;
    }
    if (rc == 0) {
      localbuf[0] = 0;
      if (errno != EAGAIN)
        buf->eof = 1;
    }
    int sz_read = rc;
    if (sz_read > 0) {
      unsigned _cnt = buf->count;
      for (tab = 0; tab < tabcount; ++tab) {
        const unsigned char *table = tables[tab];
        unsigned char *output = buf->buffer[tab] + _cnt;
        __convert_one_to_one(table, output, sz_read, localbuf);
        output[sz_read] = 0; // this last table doesn't map 0 to 0 so it must
                             // be terminated explicitly
      }
      buf->count += sz_read;
    }
  }
  return;
}

int buf_has_data(struct slide_buf *buf) {
  if (buf->count > 0)
    return 1;
  return 0;
}

int strlen_n(const unsigned char *str) {
  int res = 0;
  while (*str) {
    ++res;
    if (*str == '\n')
      break;
    ++str;
  }
  return res;
}

unsigned find_longest_in_buf(struct slide_buf *buf, unsigned *len) {
  int tab;
  int index = 0;
  unsigned max = 0;
  static int last_index = -1;

  for (tab = 0; tab < tabcount; ++tab) {
    int tmp = strlen_n(buf->buffer[tab]);
    if (tmp > max) {
      max = tmp;
      index = tab;
      if ((buf->buffer[tab])[tmp - 1] == '\n') {
        break;
      }
    } else if (tmp == max) {
      if (tab == last_index) {
        index = tab;
        if ((buf->buffer[tab])[tmp - 1] == '\n') {
          break;
        }
      }
    }
  }
  last_index = index;
  *len = max;
  return index;
}

int dofile(int fd) {
  struct slide_buf buf;
  unblock(fd);
  init_buf(fd, &buf);
  refill_buf(fd, &buf);
  unsigned len = 0;
  while (buf_has_data(&buf)) {
    unsigned candidate_index = find_longest_in_buf(&buf, &len);
    wrline(buf.buffer[candidate_index], len);
    buf.offset = len;
    buf.count -= len;
    refill_buf(fd, &buf);
  }
  return 0;
}

int main(int argc, char **argv) {
  unsigned int i;
  int rc = 0;
  int doneone = 0;

#if __MVS__
  unsetenv("_BPXK_AUTOCVT");
  orig_conv_state = __ae_autoconvert_state(_CVTSTATE_OFF);
  __setfdccsid(0, 0x10000 + 819);
  __setfdccsid(1, 0x10000 + 819);
  __setfdccsid(2, 0x10000 + 819);
#endif

#if (' ' == 0x40)
  ebcdic_out = 1;
  iam_ebcdic = 1;
  tables[0] = print_ebcdic;
  tables[1] = print_ascii;
#if defined(DOUBLE_CONVERT)
  tables[2] = print_ebcdic2;
  tables[3] = print_ascii2;
  tables[4] = print_invalid;
#else
  tables[2] = print_invalid;
#endif
#elif (' ' == 0x20)
  ebcdic_out = 0;
  iam_ebcdic = 0;
  tables[0] = print_ascii;
  tables[1] = print_ebcdic;
#if defined(DOUBLE_CONVERT)
  tables[2] = print_ascii2;
  tables[3] = print_ebcdic2;
  tables[4] = print_invalid;
#else
  tables[2] = print_invalid;
#endif
#else
  assert(!!!"platform not determined");
#endif
  if (argc > 1) {
    for (i = 1; i < argc; ++i) {
      if (!strcmp("-e", argv[i])) {
        ebcdic_out = 1;
      } else if (!strcmp("-a", argv[i])) {
        ebcdic_out = 0;
      } else if (!strcmp("-2", argv[i])) {
        outfd = 2;
      } else if (!strcmp("--help", argv[i])) {
        return help(argc, argv);
      } else if (!strcmp("-help", argv[i])) {
        return help(argc, argv);
      } else if (!strcmp("-o", argv[i])) {
        if ((i + 1) < argc) {
          ++i;
          rawfd = open(argv[i], O_WRONLY | O_CREAT | O_APPEND,
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
          if (-1 == rawfd) {
            int err = errno;
            fprintf(stderr, "errno %d on open %s : %s\n", err, argv[i],
                    strerror(err));
            return err;
          }
        }
      } else if (!strcmp("-", argv[i])) {
        rc += dofile(0);
        doneone = 1;
      } else {
        int fd = open(argv[i], O_RDONLY);
        if (fd >= 0) {
          files = 1;
          rc += dofile(fd);
          doneone = 1;
          close(fd);
        } else {
          int err = errno;
          fprintf(stderr, "errno %d on open %s : %s\n", err, argv[i],
                  strerror(err));
          return err;
        }
      }
    }
  }
  if (doneone == 0)
    rc += dofile(0);
  return rc;
}
#endif // TABLE_BUILD
