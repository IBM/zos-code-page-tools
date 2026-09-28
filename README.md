# zos-code-page-tools

A set of z/OS code page tools for working with mixed EBCDIC / ASCII / UTF-8 data.
Handy for conversion, inspection, verification, and file tagging (CCSID).

Tools:

| Tool | Purpose |
| ---- | ------- |
| `tagfile` | Detect file content and set the z/OS file tag (CCSID) accordingly |
| `utf8-verify` | Verify well-formed UTF-8; optionally escape multibyte characters to ASCII |
| `cat2` | Display mixed EBCDIC + ASCII input as readable text |
| `aeconv` | Convert files in-place between ASCII (CCSID 819) and EBCDIC (CCSID 1047) |

Common CCSIDs used here:

- `819` — ISO-8859-1 / ASCII
- `1047` — EBCDIC Latin-1
- `1208` — UTF-8
- `65535` — binary / untagged

## Prerequisites

- z/OS 2.4 or later (file tagging with `tagfile` is z/OS-only)
- `git`, z/OS `clang` (zopen), `make`
- For `cat2` / `aeconv` / `utf8-verify`, a POSIX build environment is enough to
  lint and smoke-test; `tagfile` builds portably but tagging is a no-op
  off z/OS.

## Build

```sh
make
```

The build embeds the version via `-DZOSCPT_VERSION="..."`, taken from
`$ZOSCPT_VERSION` or else `git describe --tags --always --dirty`.
A build without the define reports `dev`. Each tool prints it:

```sh
tagfile -v        # -V also works
utf8-verify -V
cat2 -V
aeconv -V
```

Executables are placed in `objs/`:

```sh
ls objs/
# aeconv  cat2  tagfile  utf8-verify
```

Install (z/OS):

```sh
make install PREFIX=/usr
# or staged:
# make install PREFIX=/usr DESTDIR=/tmp/stage
```

Clean:

```sh
make clean
```

Build with warnings / hardening (default in `tools.mak`):

```sh
make -C objs -f ../tools.mak
# -Wall -Wextra -Wpedantic -Wformat-security -fstack-protector-strong -D_FORTIFY_SOURCE=2
```

To override the compiler / flags on z/OS:

```sh
CCOVERRIDE=clang CFLAGSOVERRIDE="-O2 -Wall" make
```

## Usage

### tagfile — auto-tag files by content

```sh
tagfile [-bdquhrv] [files ...]
tagfile -q -r dir/ files ...
```

Detects whether each regular file looks like EBCDIC, ASCII, or UTF-8 and sets
the z/OS file tag (`t:1 ccsid:<n>`) via `__chattr()`. Binary files get
`t:0` (untagged) with a guessed CCSID, or are left alone with `-b`.

Options:

```text
-d  dry run: do not tag anything; exit non-zero if tagging would be needed
-b  do not tag binary files (CCSID 65535)
-q  quiet operation
-r  recurse into subdirectories
-u  tag UTF-8 files with 1208 instead of 819
-h  display help (exit 0)
-v, -V  display version (exit 0)
```

Notes:

- Empty (0-byte) files are reported and skipped.
- Symlinks to directories are not descended (avoids symlink loops).
- Overlong paths are skipped with an error instead of overflowing the buffer.
- Exit status is `0` on success, `1` if any file needed tagging / errored
  (or `2` for bad usage). Use `-d` in CI to detect untagged files.
- Example:

```sh
tagfile -r src/ include/
tagfile -d -q -r .   # fail CI if anything is untagged
```

### utf8-verify — verify UTF-8, optionally escape to ASCII

```sh
utf8-verify -i [input] -o [output] [-u] [-v]
```

With no `-i`, or when input is `-`, reads standard input.
With no `-o`, or when output is `-`, writes standard output.

Options:

```text
-i  input file name, '-' for stdin
-o  output file name, '-' for stdout. Multibyte characters are escaped to:
      \uxxxx (4 hex digits) and \Uxxxxxxxx (8 hex digits)
-u  use U+xxxx / U+xxxxx / U+xxxxxx form instead of C \u / \U notation
-v  verbose diagnostics on stderr
-V  display version (exit 0)
-h  display help (exit 0)
```

Validation follows RFC 3629:

- 2-byte `C2–DF 80–BF`, 3-byte `E0–EF 80–BF 80–BF`,
  4-byte `F0–F7 80–BF 80–BF 80–BF`
- Overlongs rejected by range checks; surrogates `U+D800–U+DFFF` rejected;
  `> U+10FFFF` and truncated sequences rejected.

Exit codes:

```text
0  valid UTF-8
1  malformed UTF-8 detected (or read/write failure during verify)
2  bad usage / I/O error opening files
```

Examples:

```sh
utf8-verify -i in.txt -o out.ascii -v
cat in.txt | utf8-verify -i - -o - -u > escaped.txt
```

Security note: `-o` truncates/creates with mode `0666 & ~umask`. Do not point
it at a symlink you do not trust.

### cat2 — display mixed EBCDIC / ASCII

```sh
cat2 [OPTION]... [FILE]...
cat2 < inputfile
some-process | cat2
```

Tries each line as ASCII and EBCDIC and prints the most readable form.
Useful when a log mixes both encodings.

```text
-o [logfile]  save raw input to [logfile] (appended, created 0644)
--help, -help show help (exit 0)
-a            force ASCII output
-e            force EBCDIC output
-2            write output to stderr (fd 2)
-V            display version (exit 0)
```

With no `FILE`, or when `FILE` is `-`, reads standard input.

Examples:

```sh
cat2 -o rawdata.txt f - g
# f, then stdin, then g -> terminal; raw bytes appended to rawdata.txt

cat2 file-with-mixed-encoding.txt | less
```

Notes:

- `-o` appends (`O_APPEND`). Repeated runs grow the file — truncate first
  if you want a fresh log.
- Write errors (EPIPE, disk full) are reported via non-zero exit instead of
  being silently ignored.
- Exit `0` on success, `1` on I/O error, `2` on bad usage.

### aeconv — ASCII <-> EBCDIC in-place converter

```sh
aeconv -a2e [files ...]   # ASCII (819) -> EBCDIC (1047)
aeconv -e2a [files ...]   # EBCDIC (1047) -> ASCII (819)
aeconv -V                 # display version (exit 0)
```

> WARNING: conversion is done in-place and is destructive — no temp file or
> backup is created. Back up files first. Interrupting mid-file leaves a
> partially converted file.

- Only regular files are converted; empty files are skipped.
- Files are re-validated with `fstat()` after `open()` to reduce
  stat/open TOCTOU races.
- Short writes / `EINTR` are retried; any I/O error aborts that file and
  moves to the next.
- Exit `0` if all files converted, `1` if any failed, `2` for bad usage.

Example:

```sh
cp important.txt important.txt.bak
aeconv -e2a important.txt
```

## Security notes

- `aeconv` overwrites files in place. Ensure correct ownership / permissions
  before running, and keep backups.
- `tagfile` changes file tags via `__chattr()`. Use `-d` (dry run) first to
  preview, especially with `-r`.
- `cat2 -o` appends to the log file. Verify the log path is not a symlink to
  a sensitive file.
- `utf8-verify -o` truncates the output file. Same symlink caution as above.
- All tools now build warning-free with `-Wall -Wextra -Wpedantic` and use
  `_FORTIFY_SOURCE=2` / stack protectors where supported.

## Downstream

Packaged port: [zopencommunity/zos-code-page-toolsport](https://github.com/zopencommunity/zos-code-page-toolsport) —
tracks this repo's releases via a `ZOSCPT_VERSION` bump.

## License

Apache License 2.0 — see [LICENSE](LICENSE).
