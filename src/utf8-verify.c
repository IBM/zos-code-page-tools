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
 *  Program: utf8-verify
 *
 *  Purpose: Verify a file is in well-formed UTF-8
 *           Optionally convert to pure ascii with the U notation
 *
 *  Syntax: utf8-verify [files ... ]
 */
#if defined(__MVS__)
#include <_Nascii.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int byte0_next_state[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,
    3,  -1, -1, -1, -1, -1, -1, -1, -1};

int work(int fd, int outfd, const char *filename, int verbose, int u) {
  unsigned char c;
  unsigned char onebyte;
  char output[80];
  int bytes;
  int state = 0;
  unsigned int value;
  unsigned char d[4];
  size_t offset = 0;
  int linenum = 1;
  c = read(fd, &onebyte, 1);
  while (c == 1) {
    switch (state) {
    case 0:
      state = byte0_next_state[onebyte];
      if (-1 == state) {
        if (verbose)
          fprintf(stderr,
                  "Error deleted in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, byte 0x%02X malformed, not one of 0xxxxxxx, "
                  "110xxxxx, 1110xxxx, 11110xxx\n",
                  filename, offset, linenum, onebyte);
        return -1;
      }
      if (state == 0) {
        if (onebyte == 0x0a)
          ++linenum;
        write(outfd, &onebyte, 1);
        break;
      } else {
        d[0] = onebyte;
      }
      break;
    case 1:
      if ((onebyte & 0xc0) == 0x80) {
        d[1] = onebyte;
        value = (0x1c & d[0] << 6) | (((0x03 & d[0]) << 6) | (0x3f & d[1]));
        if (value < 0x80 || value > 0x7ff) {
          if (verbose)
            fprintf(
                stderr,
                "Error deleted in: \"%s\", "
                "Invalid unicode sequence at file offset %lu around line "
                "%d, 2-byte sequence 0x%02X%02X value U+%04X invalid, range "
                "out of U+0080 and U+07FF\n",
                filename, offset, linenum, d[0], d[1], value);
          return -1;
        }
        bytes = u ? snprintf(output, 80, "U+%04X", value)
                  : snprintf(output, 80, "\\u%04X", value);
        write(outfd, output, bytes);
        state = 0;
      } else {
        if (verbose)
          fprintf(stderr,
                  "Error deleted in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, 2-byte sequence 0x%02X%02X 2nd byte malformed, not "
                  "110xxxxx-10xxxxxx\n",
                  filename, offset, linenum, d[0], onebyte);
        return -1;
      }
      break;

    case 2:
      if ((onebyte & 0xc0) == 0x80) {
        d[1] = onebyte;
        state = 22;
      } else {
        if (verbose)
          fprintf(stderr,
                  "Error deleted in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, 3-byte sequence 0x%02X%02Xxx 2nd byte malformed, not "
                  "1110xxxx-10xxxxxx-xxxxxxxx\n",
                  filename, offset, linenum, d[0], onebyte);
        return -1;
      }
      break;

    case 3:
      if ((onebyte & 0xc0) == 0x80) {
        d[1] = onebyte;
        state = 33;
      } else {
        if (verbose)
          fprintf(stderr,
                  "Error deleted in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, 4-byte sequence 0x%02X%02Xxxxx 2nd byte malformed, not "
                  "11110xxx-10xxxxxx-xxxxxxxx-xxxxxxxx\n",
                  filename, offset, linenum, d[0], onebyte);
        return -1;
      }
      break;

    case 33:
      if ((onebyte & 0xc0) == 0x80) {
        d[2] = onebyte;
        state = 333;
      } else {
        if (verbose)
          fprintf(
              stderr,
              "Error deleted in: \"%s\", "
              "Invalid unicode sequence at file offset %lu around line "
              "%d, 4-byte sequence 0x%02X%02X%02Xxx 3rd byte malformed, not "
              "11110xxx-10xxxxxx-10xxxxxxx-xxxxxxxx\n",
              filename, offset, linenum, d[0], d[1], onebyte);
        return -1;
      }
      break;

    case 22:
      if ((onebyte & 0xc0) == 0x80) {
        d[2] = onebyte;
        value =
            ((0x000f & d[0]) << 12) | ((0x003f & d[1]) << 6) | (0x3f & d[2]);
        if (value < 0x0800 || value > 0x0ffff) {
          if (verbose)
            fprintf(stderr,
                    "Error deleted in: \"%s\", "
                    "Invalid unicode sequence at file offset %lu around line "
                    "%d, 3-byte sequence 0x%02X%02X%02X value U+%04X "
                    "invalid, range "
                    "out of U+0800 and U+FFFF\n",
                    filename, offset, linenum, d[0], d[1], d[2], value);
          return -1;
        }
        bytes = u ? snprintf(output, 80, "U+%04X", value)
                  : snprintf(output, 80, "\\u%04X", value);
        write(outfd, output, bytes);
        state = 0;
      } else {
        if (verbose)
          fprintf(stderr,
                  "Error deleted in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, 3-byte sequence 0x%02X%02X%02X 3rd byte malformed, not "
                  "11110xxx-10xxxxxx-10xxxxxxx\n",
                  filename, offset, linenum, d[0], d[1], onebyte);
        return -1;
      }
      break;
    case 333:
      if ((onebyte & 0xc0) == 0x80) {
        d[3] = onebyte;
        value = ((0x0007 & d[0]) << 18) | ((0x003f & d[1]) << 12) |
                ((0x003f & d[2]) << 6) | (0x3f & d[3]);
        if (value < 0x010000 || value > 0x010ffff) {
          if (verbose)
            fprintf(stderr,
                    "Error deleted in: \"%s\", "
                    "Invalid unicode sequence at file offset %lu around line "
                    "%d, 4-byte sequence 0x%02X%02X%02X%02X value U+%05X "
                    "invalid, range "
                    "out of U+10000 and U+10FFFF\n",
                    filename, offset, linenum, d[0], d[1], d[2], d[3], value);
          return -1;
        }
        bytes = u ? snprintf(output, 80, "U+%04X", value)
                  : snprintf(output, 80, "\\U%08X", value);
        write(outfd, output, bytes);
        state = 0;
      } else {
        if (verbose)
          fprintf(stderr,
                  "Error deleted in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, 4-byte sequence 0x%02X%02X%02X%02Xx 4th byte "
                  "malformed, not "
                  "11110xxx-10xxxxxx-10xxxxxxx-10xxxxxx\n",
                  filename, offset, linenum, d[0], d[1], d[2], onebyte);
        return -1;
      }
      break;
    default:
      if (verbose)
        fprintf(stderr,
                "Error deleted in: \"%s\", "
                "Invalid unicode sequence at file offset %lu around line "
                "%d, parser in unknown state %d, byte read 0x%02X\n",
                filename, offset, linenum, state, onebyte);
      return -1;
    }
    ++offset;
    c = read(fd, &onebyte, 1);
  }
  if (state != 0) {
    if (verbose)
      fprintf(stderr,
              "Error deleted in: \"%s\", "
              "Excepted End of File detected at file offset %lu around line "
              "%d, parser in state %d, byte read 0x%02X\n",
              filename, offset, linenum, state, onebyte);
    return -1;
  }
  return 0;
}
int help(int argc, char **argv) {
  fprintf(stderr, "\n\
NAME\n\
       utf8-ver - check and optionally convert multibyte code points to U'....' or u'....' notation\n\
\n\
SYNOPSIS\n\
       utf8-ver [OPTION]... [FILE]\n\
\n\
DESCRIPTION\n\
       Verify FILE to standard output.\n\
\n\
       With no FILE, or when FILE is -, read standard input.\n\
\n\
       -i,  input file name to read, '-' read standard input \n\
       -o,  output file name to write to with converted multiple characters to ascii C\n\
            \\uxxxx (fixed-length, 4 hex digits) and \\Uxxxxxxxx (fixed-length, 8 hex digits)\n\
       -u,  convert to U+(xxxx | xxxxx | xxxxxx) form instead of the C notation\n\
       -v,  verbose\n\
\n\
RETURN\n\
        0,  no error\n\
        1,  malformed utf-8 detected\n\
        255,  other errors\n\
\n\
utf8-ver version 1.0\n\
\n");
  return -1;
}
int main(int argc, char **argv) {
  opterr = 0;
  int c;
  int verbose = 0;
  char *output_file = "/dev/null";
  int outfd = 1;
  char *input_file = "-";
  int infd = 0;
  int error = 0;
  int u = 0;
  while (c = getopt(argc, argv, "i:o:huv"), c != -1)
    switch (c) {
    case 'i':
      input_file = optarg;
      break;
    case 'o':
      output_file = optarg;
      break;
    case 'h':
      return help(argc, argv);
    case 'u':
      u = 1;
    case 'v':
      verbose = 1;
      break;
    default:
      fprintf(stderr, "unexpected option %c unknown. see -h\n", optopt);
      return -1;
    }

  for (int i = optind; i < argc; ++i) {
    fprintf(stderr, "unexpected argument %s unknown. see -h\n", argv[i]);
    error = -1;
  }
  if (error)
    return error;

  if (0 != strcmp("-", input_file)) {
    int fd = open(input_file, O_RDONLY);
    if (fd != -1)
      infd = fd;
    else {
      int err = errno;
      fprintf(stderr, "Unable to open %s for read\n", input_file);
      errno = err;
      perror("open for read");
      return -1;
    }
  }
  if (0 != strcmp("-", output_file)) {
    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd != -1)
      outfd = fd;
    else {
      int err = errno;
      fprintf(stderr, "Unable to open %s for write\n", input_file);
      errno = err;
      perror("open for write");
      return -1;
    }
  }
  return work(infd, outfd, input_file, verbose, u);
}
