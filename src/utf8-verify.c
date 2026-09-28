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

/* Version string: defaults to "dev" and is overridden at build time via
 * -DZOSCPT_VERSION="..." (see tools.mak: ZOSCPT_VERSION, with git
 * describe / dev-<timestamp> fallback). */
#ifndef ZOSCPT_VERSION
#define ZOSCPT_VERSION "dev"
#endif

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

static ssize_t full_write(int fd, const void *buf, size_t count) {
  const unsigned char *p = (const unsigned char *)buf;
  size_t done = 0;
  while (done < count) {
    ssize_t rc = write(fd, p + done, count - done);
    if (rc < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    if (rc == 0) {
      errno = EIO;
      return -1;
    }
    done += (size_t)rc;
  }
  return (ssize_t)done;
}

int work(int fd, int outfd, const char *filename, int verbose, int u) {
  unsigned char onebyte;
  char output[80];
  int bytes;
  int state = 0;
  unsigned int value;
  unsigned char d[4];
  size_t offset = 0;
  int linenum = 1;
  ssize_t n = read(fd, &onebyte, 1);
  while (n == 1) {
    switch (state) {
    case 0:
      state = byte0_next_state[onebyte];
      if (-1 == state) {
        if (verbose)
          fprintf(stderr,
                  "Error detected in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, byte 0x%02X malformed, not one of 0xxxxxxx, "
                  "110xxxxx, 1110xxxx, 11110xxx\n",
                  filename, (unsigned long)offset, linenum, onebyte);
        return -1;
      }
      if (state == 0) {
        if (onebyte == 0x0a)
          ++linenum;
        if (full_write(outfd, &onebyte, 1) < 0)
          return -1;
        break;
      } else {
        d[0] = onebyte;
      }
      break;
    case 1:
      if ((onebyte & 0xc0) == 0x80) {
        d[1] = onebyte;
        value = (((unsigned int)(d[0] & 0x1f)) << 6) |
                ((unsigned int)(d[1] & 0x3f));
        if (value < 0x80 || value > 0x7ff) {
          if (verbose)
            fprintf(
                stderr,
                "Error detected in: \"%s\", "
                "Invalid unicode sequence at file offset %lu around line "
                "%d, 2-byte sequence 0x%02X%02X value U+%04X invalid, range "
                "out of U+0080 and U+07FF\n",
                filename, (unsigned long)offset, linenum, d[0], d[1], value);
          return -1;
        }
        bytes = u ? snprintf(output, 80, "U+%04X", value)
                  : snprintf(output, 80, "\\u%04X", value);
        if (bytes < 0 || bytes >= 80)
          return -1;
        if (full_write(outfd, output, (size_t)bytes) < 0)
          return -1;
        state = 0;
      } else {
        if (verbose)
          fprintf(stderr,
                  "Error detected in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, 2-byte sequence 0x%02X%02X 2nd byte malformed, not "
                  "110xxxxx-10xxxxxx\n",
                  filename, (unsigned long)offset, linenum, d[0], onebyte);
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
                  "Error detected in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, 3-byte sequence 0x%02X%02Xxx 2nd byte malformed, not "
                  "1110xxxx-10xxxxxx-xxxxxxxx\n",
                  filename, (unsigned long)offset, linenum, d[0], onebyte);
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
                  "Error detected in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, 4-byte sequence 0x%02X%02Xxxxx 2nd byte malformed, not "
                  "11110xxx-10xxxxxx-xxxxxxxx-xxxxxxxx\n",
                  filename, (unsigned long)offset, linenum, d[0], onebyte);
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
              "Error detected in: \"%s\", "
              "Invalid unicode sequence at file offset %lu around line "
              "%d, 4-byte sequence 0x%02X%02X%02Xxx 3rd byte malformed, not "
              "11110xxx-10xxxxxx-10xxxxxxx-xxxxxxxx\n",
              filename, (unsigned long)offset, linenum, d[0], d[1], onebyte);
        return -1;
      }
      break;

    case 22:
      if ((onebyte & 0xc0) == 0x80) {
        d[2] = onebyte;
        value =
            ((0x000f & d[0]) << 12) | ((0x003f & d[1]) << 6) | (0x3f & d[2]);
        if (value < 0x0800 || value > 0x0ffff ||
            (value >= 0xd800 && value <= 0xdfff)) {
          if (verbose)
            fprintf(stderr,
                    "Error detected in: \"%s\", "
                    "Invalid unicode sequence at file offset %lu around line "
                    "%d, 3-byte sequence 0x%02X%02X%02X value U+%04X "
                    "invalid, range "
                    "out of U+0800 and U+FFFF (surrogates U+D800-U+DFFF "
                    "rejected)\n",
                    filename, (unsigned long)offset, linenum, d[0], d[1],
                    d[2], value);
          return -1;
        }
        bytes = u ? snprintf(output, 80, "U+%04X", value)
                  : snprintf(output, 80, "\\u%04X", value);
        if (bytes < 0 || bytes >= 80)
          return -1;
        if (full_write(outfd, output, (size_t)bytes) < 0)
          return -1;
        state = 0;
      } else {
        if (verbose)
          fprintf(stderr,
                  "Error detected in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, 3-byte sequence 0x%02X%02X%02X 3rd byte malformed, not "
                  "11110xxx-10xxxxxx-10xxxxxxx\n",
                  filename, (unsigned long)offset, linenum, d[0], d[1],
                  onebyte);
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
                    "Error detected in: \"%s\", "
                    "Invalid unicode sequence at file offset %lu around line "
                    "%d, 4-byte sequence 0x%02X%02X%02X%02X value U+%05X "
                    "invalid, range "
                    "out of U+10000 and U+10FFFF\n",
                    filename, (unsigned long)offset, linenum, d[0], d[1],
                    d[2], d[3], value);
          return -1;
        }
        bytes = u ? snprintf(output, 80, "U+%04X", value)
                  : snprintf(output, 80, "\\U%08X", value);
        if (bytes < 0 || bytes >= 80)
          return -1;
        if (full_write(outfd, output, (size_t)bytes) < 0)
          return -1;
        state = 0;
      } else {
        if (verbose)
          fprintf(stderr,
                  "Error detected in: \"%s\", "
                  "Invalid unicode sequence at file offset %lu around line "
                  "%d, 4-byte sequence 0x%02X%02X%02X%02Xx 4th byte "
                  "malformed, not "
                  "11110xxx-10xxxxxx-10xxxxxxx-10xxxxxx\n",
                  filename, (unsigned long)offset, linenum, d[0], d[1], d[2],
                  onebyte);
        return -1;
      }
      break;
    default:
      if (verbose)
        fprintf(stderr,
                "Error detected in: \"%s\", "
                "Invalid unicode sequence at file offset %lu around line "
                "%d, parser in unknown state %d, byte read 0x%02X\n",
                filename, (unsigned long)offset, linenum, state, onebyte);
      return -1;
    }
    ++offset;
    n = read(fd, &onebyte, 1);
  }
  if (n < 0) {
    int err = errno;
    if (verbose) {
      errno = err;
      fprintf(stderr, "Error detected in: \"%s\", read failed: %s\n",
              filename, strerror(err));
    }
    return -1;
  }
  if (state != 0) {
    if (verbose)
      fprintf(stderr,
              "Error detected in: \"%s\", "
              "Unexpected end of file at file offset %lu around line "
              "%d, parser in state %d, byte read 0x%02X\n",
              filename, (unsigned long)offset, linenum, state, onebyte);
    return -1;
  }
  return 0;
}
int help(int argc, char **argv) {
  (void)argc;
  (void)argv;
  fprintf(stderr, "\n\
NAME\n\
       utf8-verify - check and optionally convert multibyte code points to U'....' or u'....' notation\n\
\n\
SYNOPSIS\n\
       utf8-verify -i [input file] -o [output file] [-u] [-v] [-V]\n\
\n\
DESCRIPTION\n\
       Verify FILE, writing converted output.\n\
\n\
       With no FILE, or when FILE is -, read standard input.\n\
       With no -o, or when output FILE is -, write standard output.\n\
\n\
       -i,  input file name to read, '-' read standard input \n\
       -o,  output file name to write to with converted multibyte characters to ascii C\n\
            \\uxxxx (fixed-length, 4 hex digits) and \\Uxxxxxxxx (fixed-length, 8 hex digits)\n\
       -u,  convert to U+(xxxx | xxxxx | xxxxxx) form instead of the C notation\n\
       -v,  verbose\n\
       -V,  display version and exit\n\
\n\
RETURN\n\
        0,  no error\n\
        1,  malformed utf-8 detected\n\
        2,  other errors (bad usage, I/O failure)\n\
\n\
utf8-verify version " ZOSCPT_VERSION "\n\
\n");
  return 0;
}
int main(int argc, char **argv) {
  opterr = 0;
  int c;
  int verbose = 0;
  char *output_file = "-";
  int outfd = 1;
  char *input_file = "-";
  int infd = 0;
  int error = 0;
  int u = 0;
  while ((c = getopt(argc, argv, "i:o:huVv")) != -1)
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
      break;
    case 'v':
      verbose = 1;
      break;
    case 'V':
      printf("utf8-verify %s\n", ZOSCPT_VERSION);
      return 0;
    default:
      fprintf(stderr, "unexpected option %c unknown. see -h\n", optopt);
      return 2;
    }

  for (int i = optind; i < argc; ++i) {
    fprintf(stderr, "unexpected argument %s unknown. see -h\n", argv[i]);
    error = 1;
  }
  if (error)
    return 2;

  if (0 != strcmp("-", input_file)) {
    int fd = open(input_file, O_RDONLY);
    if (fd != -1)
      infd = fd;
    else {
      int err = errno;
      fprintf(stderr, "Unable to open %s for read\n", input_file);
      errno = err;
      perror("open for read");
      return 2;
    }
  }
  if (0 != strcmp("-", output_file)) {
    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd != -1)
      outfd = fd;
    else {
      int err = errno;
      fprintf(stderr, "Unable to open %s for write\n", output_file);
      errno = err;
      perror("open for write");
      return 2;
    }
  }
  int rc = work(infd, outfd, input_file, verbose, u);
  if (rc != 0)
    return 1;
  return 0;
}
