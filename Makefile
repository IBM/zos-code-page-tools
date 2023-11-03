# makefile for zos
# run by:
# the zopen version of make
# make -f [path-to-this]/Makefile
# objects and dll will be created in current directory.

all: cat2 tagfile aeconv utf8-verify

ME:=$(firstword $(MAKEFILE_LIST))
USER:=$(shell id -nu)
GITDIR:=$(shell dirname $(ME))/.
COMMIT:=zoscptools-$(shell echo $$(cd $(GITDIR) && git log -n 1 --no-color) | sed -e "s/^commit \([^ ]*\) .*/\1/" )

ifndef SRC_DIR
SRC_DIR:=$(GITDIR)
endif

ifndef CC
CC=clang
endif

ifndef CPP
CC=clang++
endif

LINK:=$(CC) -V -W ,CALL,REUS=RENT,MAP,XREF,LIST,LP64 -Wl,XPLINK

ifndef CFLAG
CFLAGS=\
-DPATH_MAX=1023  \
-D_AE_BIMODAL=1  \
-D_ALL_SOURCE  \
-U_ENHANCED_ASCII_EXT  \
-D_ENHANCED_ASCII_EXT=0xFFFFFFFF  \
-D_Export=extern  \
-D_LARGE_TIME_API  \
-D_OPEN_MSGQ_EXT  \
-D_OPEN_SYS_FILE_EXT=1  \
-D_OPEN_SYS_SOCK_IPV6  \
-D_UNIX03_SOURCE  \
-D_UNIX03_THREADS  \
-D_UNIX03_WITHDRAWN  \
-D_XOPEN_SOURCE_EXTENDED  \
-D_XPLATFORM_SOURCE=1  \
-D__static_assert=static_assert  \
-D_POSIX_C_SOURCE=200809L  \
-fasm  \
-fzos-le-char-mode=ascii  \
-isystem/usr/include  \
-I$(GITDIR)/include \
-m64
endif

vpath %.h $(SRC_DIR)/ $(SRC_DIR)/src/
vpath %.c $(SRC_DIR)/ $(SRC_DIR)/src/

-include localtargets

%.o: %.c $(ME)
	@echo
	@echo cd $(PWD)
	_AS_MACLIB=SYS1.MACLIB $(CC) -c $(CFLAGS) -o $@ $<

%: %.o $(ME)
	@echo
	@echo cd $(PWD)
	$(LINK) -o $@ $<
	
clean:
	-@ for f in *.u *.o $(all) ;do  [ -r $$f ] && rm $$f  ; done; true

