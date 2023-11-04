
# bootstrap makefile
builddir := objs

.DEFAULT all: 
	mkdir -p $(builddir) && cd $(builddir) && $(MAKE) -f ../tools.mak $@
