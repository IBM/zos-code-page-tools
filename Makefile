
# bootstrap makefile
builddir := objs

.PHONY: all install check clean
all install check clean:
	mkdir -p $(builddir) && cd $(builddir) && $(MAKE) -f ../tools.mak $@
