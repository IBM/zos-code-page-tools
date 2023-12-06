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
 *  Program: tagfile
 *
 *  Purpose: Auto tag files with ccsid based on data.
 *
 *  Syntax: tagfile [-bhqru] [files ... ]
 *
 *********************************************************************/

#if __MVS__
// #define _POSIX_C_SOURCE 200809L
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2
#endif
#define _XOPEN_SOURCE_EXTENDED 1
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#define _OPEN_SYS
#define _OPEN_SYS_DIR_EXT
#endif

#include <_Nascii.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define PROG "tagfile"
#define MAJOR_VERSION 1
#define MINOR_VERSION 0

const static char ebcdic_valid[256] = {
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0};

const static char ascii_valid[256] = {
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const static char utf8pat[256] = {
    // UTF8 states
    // 0xxxxxxx = 0
    // 10xxxxxx = 1 (mulbyte trail)
    // 110xxxxx = 2 (2 byte head)
    // 1110xxxx = 3 (3 byte lead)
    // 11110xxx = 4 (4 byte lead)
    // 9 non-text
    9, 9, 9, 9, 9, 9, 9, 0, 0, 0, 0, 9, 0, 0, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 0, 9, 9, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 9, 9, 9, 9, 9, 9, 9, 9};

typedef struct cp_state {
  size_t ebcdic_cnt;
  size_t total_ebcdic;
  size_t ascii_cnt;
  size_t total_ascii;
  int utf8_st;
  int utf8_format_error;
} cp_state_t;

struct options {
  int qflag;
  int uflag;
  int bflag;
  int dflag;
  int hflag;
  int rflag;
  int errcnt;
  int vflag;
};

static void determine_ebcdic(const char *buffer, size_t size,
                             cp_state_t *state);
static void determine_ascii(const char *buffer, size_t size, cp_state_t *state);
static void determine_utf8(const char *buffer, size_t size, cp_state_t *state);
static int dofile(const char *name, struct options *opts);
static int dowork(const char *path, struct options *p);
static int expander(const char *path, struct options *opts);

static void determine_ebcdic(const char *buffer, size_t size,
                             cp_state_t *state) {
  int i;
  int c;
  for (i = 0; i < size; ++i) {
    c = buffer[i] & 255;
    if (ebcdic_valid[c]) {
      ++(state->ebcdic_cnt);
    }
    ++(state->total_ebcdic);
  }
}

static void determine_ascii(const char *buffer, size_t size,
                            cp_state_t *state) {
  int i;
  int c;
  for (i = 0; i < size; ++i) {
    c = buffer[i] & 255;
    if (ascii_valid[c]) {
      ++(state->ascii_cnt);
    }
    ++(state->total_ascii);
  }
}

static void determine_utf8(const char *buffer, size_t size, cp_state_t *state) {
  int i;
  int c;
  enum {
    onebyte = 1,
    twobyte0,
    threebyte0,
    threebyte1,
    fourbyte0,
    fourbyte1,
    fourbyte2,
    bad
  } state_t;
  int st = onebyte;
  if (state->utf8_st == 0) {
    state->utf8_st = onebyte;
    st = onebyte;
  } else
    st = state->utf8_st;
  for (i = 0; i < size; ++i) {
    c = buffer[i] & 255;
    switch (st) {
    case onebyte:
      if (utf8pat[c] == 9) {
        st = bad;
        break;
      }
      if (utf8pat[c] == 1) {
        st = bad;
        break;
      }
      if (utf8pat[c] == 2) {
        st = twobyte0;
      } else if (utf8pat[c] == 3) {
        st = threebyte0;
      } else if (utf8pat[c] == 4) {
        st = fourbyte0;
      } else if (utf8pat[c] == 0) {
      } else
        st = bad;
      break;
    case twobyte0:
      if (utf8pat[c] == 1) {
        st = onebyte;
      } else
        st = bad;
      break;
    case threebyte0:
      if (utf8pat[c] == 1) {
        st = threebyte1;
      } else
        st = bad;
      break;
    case threebyte1:
      if (utf8pat[c] == 1) {
        st = onebyte;
      } else
        st = bad;
      break;
    case fourbyte0:
      if (utf8pat[c] == 1) {
        st = fourbyte1;
      } else
        st = bad;
      break;
    case fourbyte1:
      if (utf8pat[c] == 1) {
        st = fourbyte2;
      } else
        st = bad;
      break;
    case fourbyte2:
      if (utf8pat[c] == 1) {
        st = onebyte;
      } else
        st = bad;
      break;
    default:
      st = bad;
    }
    if (st == bad) {
      state->utf8_format_error = 1;
    }
  }
  state->utf8_st = st;
}

static int dofile(const char *name, struct options *opts) {
  FILE *f;
  int fd = -1;
  size_t size = 0;
  size_t offset = 0;
  char *buffer;
  int b;
  int tag = 0;
  unsigned short ccsid = 0;
  int makechange = 0;
  attrib_t attr;
  struct stat st;
  cp_state_t state;
  if (stat(name, &st) != 0) {
    if (!opts->qflag)
      fprintf(stderr, "stat() error on %s: %s\n", name, strerror(errno));
    opts->errcnt++;
    return 1;
  }
  if (st.st_size == 0) {
    if (!opts->qflag)
      printf("file: %s is of size 0, tagged with t:%d ccsid:%d\n", name,
             st.st_tag.ft_txtflag, st.st_tag.ft_ccsid);
    opts->errcnt++;
    return 1;
  }
  size = st.st_size;
  memset(&state, 0, sizeof(state));

  if ((fd = open(name, O_RDWR, 0)) == -1) {
    if (!opts->qflag)
      printf("file: %s cannot open, errno %d\n", name, errno);
    opts->errcnt++;
    return 1;
  }
  buffer = (char *)mmap(0, 4096, PROT_READ, MAP_PRIVATE, fd, offset);
  if (buffer) {
    if (size > 4096) {
      b = 4096;
      size -= 4096;
      offset += 4096;
    } else {
      b = size;
      offset += size;
      size = 0;
    }
  }
  while (buffer && b) {
    int i;
    determine_ebcdic(buffer, b, &state);
    determine_ascii(buffer, b, &state);
    determine_utf8(buffer, b, &state);
    munmap(buffer, 4096);
    buffer = (char *)mmap(0, 4096, PROT_READ, MAP_PRIVATE, fd, offset);
    if (buffer) {
      if (size > 4096) {
        b = 4096;
        size -= 4096;
        offset += 4096;
      } else {
        b = size;
        offset += size;
        size = 0;
      }
    } else {
      if (!opts->qflag)
        printf("mmap errno %d, offset %zu \n", errno, offset);
      close(fd);
      opts->errcnt++;
      return 1;
    }
  }
  close(fd);
  if (state.ebcdic_cnt > 0 && state.ebcdic_cnt == state.total_ebcdic) {
    tag = 1;
    ccsid = 1047;
    if (st.st_tag.ft_ccsid != ccsid) {
      makechange = 1;
    }
  } else if (state.ascii_cnt > 0 && state.ascii_cnt == state.total_ascii) {
    tag = 1;
    ccsid = 819;
    if (st.st_tag.ft_ccsid != ccsid) {
      makechange = 1;
    }
  } else if (state.ascii_cnt > 0 && state.utf8_format_error == 0 &&
             state.utf8_st == 1) {
    tag = 1;
    if (opts->uflag)
      ccsid = 1208;
    else
      ccsid = 819;
    if (st.st_tag.ft_ccsid != ccsid) {
      makechange = 1;
    }
  } else {
    tag = 0;
    if (state.ebcdic_cnt > 0 &&
        ((state.ebcdic_cnt * 105) / (state.total_ebcdic * 100) > 0)) {
      ccsid = 1047;
    } else if (state.ascii_cnt > 0 &&
               ((state.ascii_cnt * 105) / (state.total_ascii * 100) > 0)) {
      ccsid = 819;
    } else {
      ccsid = 65535;
    }
    if (st.st_tag.ft_ccsid != ccsid) {
      makechange = 1;
    }
    if (opts->bflag && ccsid == 65535) {
      makechange = 0;
    }
  }
  if (makechange) {
    memset(&attr, 0, sizeof(attr));
    attr.att_filetagchg = 1;
    attr.att_filetag.ft_ccsid = ccsid;
    attr.att_filetag.ft_txtflag = tag;
    if (!opts->qflag) {
      printf("file: %s changing file tag from t:%d ccsid:%d to t:%d ccsid:%d\n",
             name, st.st_tag.ft_txtflag, st.st_tag.ft_ccsid, tag, ccsid);
    }
    if (opts->dflag) {
      opts->errcnt++; // if file tag needs change, then we should return
                      // non-zero on dry run.
      return 1;
    }
    if (__chattr((char *)name, &attr, sizeof(attr)) != 0) {
      if (!opts->qflag)
        fprintf(stderr, "__chattr() error on %s: %s\n", name, strerror(errno));
      opts->errcnt++;
      return 1;
    }
  } else {
    if (!opts->qflag) {
      printf("file: %s file tag unchanged t:%d ccsid:%d\n", name,
             st.st_tag.ft_txtflag, st.st_tag.ft_ccsid);
    }
  }
  return 0;
}

static int dowork(const char *path, struct options *p) {
  struct stat st;
  int rc;
  rc = stat(path, &st);
  if (rc) {
    if (!p->qflag)
      fprintf(stderr, "stat() error on %s: %s\n", path, strerror(errno));
    p->errcnt++;
    return -1;
  } else {
    if (S_ISDIR(st.st_mode)) {
      if (path[0] != 0)
        if (p->rflag) {
          expander(path, p);
        }
    } else if (S_ISREG(st.st_mode)) {
      return dofile(path, p);
    } else {
      if (!p->qflag)
        fprintf(stderr, "# %s is not a file or directory\n", path);
      p->errcnt++;
      return -1;
    }
  }
  return 0;
}

static int expander(const char *dirpath, struct options *opts) {
  int rc;
  struct dirent *entry;
  char path[1025];
  DIR *dir = opendir(dirpath);
  if (!dir) {
    fprintf(stderr, "opendir() error on %s: %s\n", dirpath, strerror(errno));
    opts->errcnt++;
    return -1;
  }
  while (entry = readdir(dir), entry != 0) {
    if (entry->d_name[0] != '.' ||
        (entry->d_name[1] != 0 &&
         (entry->d_name[1] != '.' || entry->d_name[2] != 0))) {
      strncpy(path, dirpath, sizeof(path) - 1);
      strncat(path, "/", sizeof(path) - 1);
      strncat(path, entry->d_name, sizeof(path) - 1);
      dowork(path, opts);
    }
  }
  closedir(dir);
  return 0;
}

void version(void) {
  printf("%s v%d.%d (%s %s)\n", PROG, MAJOR_VERSION, MINOR_VERSION, __DATE__,
         __TIME__);
}

void syntax(void) {
  printf("\n"
         "NAME\n"
         "\t%s - auto tag files with ccsid based on data\n\n"
         "SYNOPSIS\n"
         "\t%s [OPTION]... [FILE]...\n\n"
         "DESCRIPTION\n"
         "\tOptions:\n\n"
         "\t-d: do not tag anything, just dry run, exit 0 if tagging is not "
         "necessary\n"
         "\t-b: do not tag binary files\n"
         "\t-h: display syntax information\n"
         "\t-q: quiet operation\n"
         "\t-r: recurse subdirectories\n"
         "\t-u: tag UTF-8 files with codepage 1208 instead of 819\n"
         "\t-v: display version information\n"
         "\n",
         PROG, PROG);
}

int main(int argc, char **argv) {
  struct options opts;
  int index;
  int c;
  int rc = 0;
  opterr = 0;
  memset(&opts, 0, sizeof(opts));
  while (c = getopt(argc, argv, "bdquhrv"), c != -1)
    switch (c) {
    case 'q':
      opts.qflag = 1;
      break;
    case 'b':
      opts.bflag = 1;
      break;
    case 'd':
      opts.dflag = 1;
      break;
    case 'h':
      opts.hflag = 1;
      break;
    case 'r':
      opts.rflag = 1;
      break;
    case 'u':
      opts.uflag = 1;
      break;
    case 'v':
      opts.vflag = 1;
      break;
    case '?':
      fprintf(stderr, "option %c unknown\n", optopt);
      opts.hflag = 1;
      break;
    default:
      fprintf(stderr, "unexpected option %c unknown\n", optopt);
      opts.hflag = 1;
    }

  if (opts.hflag) {
    syntax();
    return -1;
  }

  if (opts.vflag) {
    version();
    return -1;
  }

  int oldstate = __ae_autoconvert_state(_CVTSTATE_OFF);

  for (index = optind; index < argc; ++index) {
    dowork(argv[index], &opts);
  }

  if (!opts.qflag) {
    fprintf(stderr, "Done, %d errors detected\n", opts.errcnt);
  }
  return opts.errcnt;
}
