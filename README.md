# zos-code-page-tools
A set of zos code pages tools, handy for conversion from codepages such as EBCDIC and ASCII

Usage example:

#### tagfile -q -r dir/ files ....
      set file tag of files based on detected file content
      -d: do not tag anything, just dry run, exit 0 if tagging is not necessary
      -q, quiet operation
      -h, this information
      -r, recurse subdirectory
      -u, tag UTF-8 files with 1208 instead of 819
      -b, do not tag binary files. 


#### utf8-verify -i [input file] -o [output file] -u 
      With no FILE, or when FILE is -, read standard input.
       -i,  input file name to read, '-' read standard input
       -o,  output file name to write to with converted multiple characters to ascii C
            \uxxxx (fixed-length, 4 hex digits) and \Uxxxxxxxx (fixed-length, 8 hex digits)
       -u,  convert to U+(xxxx | xxxxx | xxxxxx) form instead of the C notation
       -v,  verbose
 
#### cat2 < inputfile
#### (some process ) | cat2
      With no FILE, or when FILE is -, read standard input.
      -o [logfile]             save raw input to file [logfile]
      --help                   show this dialog
      -help                    show this dialog
      -a                       output in ASCII
      -e                       output in EBCDIC
      -2                       output in file descriptor 2 (stderr)
      
cat2 would try to display file content in readable text, converting a mix of EBCDIC and ASCII lines.
     
#### cat2 -o rawdata.txt f - g
      Convert f's contents, then standard input,
      then g's contents to terminal, input data saved in rawdata.txt.
####  cat2
      Convert standard input to standard output

#### aeconv -a2e [files ...]
    Convert from ascii (ccsid 819) to ebcdic (ccsid 1047)
#### aeconv -e2a [files ...]
    Convert from ebcdic (ccsiaed 1047) to ascii (ccsid 819)

note: aeconv conversion is done in-place, i.e. it is destructive (no temp file is created)



To build:
```
make
```

The executables are placed in objs/ 

     
