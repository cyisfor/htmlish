all: build/Makefile
	$(MAKE) -C build && $(MAKE) -C build install

build/Makefile: configure libxmlfixes/configure libxml2/configure | build
	cd build && ../configure --prefix=$(realpath .) --bindir=$(realpath .)

configure: configure.ac Makefile.in 
	autoconf
	touch $@

config.h.in:
	autoheader

Makefile.in: Makefile.am aclocal.m4 config.h.in
	automake --add-missing

aclocal.m4: m4/libtool.m4
	aclocal -I m4

m4/libtool.m4:
	libtoolize --automake

build:
	mkdir $@

libxml2/configure libxmlfixes/configure:
	cd $(dir $@) && autoreconf -i

.PHONY: all
