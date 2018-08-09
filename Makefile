all: build/Makefile
	$(MAKE) -C build && $(MAKE) -C build install

# --disable-shared causes us to link with the shared library
# in libxml2/ not the systemwide one, and does not actually
# disable shared.
build/Makefile: configure libxmlfixes/configure libxml2/configure | build
	cd build && ../configure --disable-shared --prefix=$(realpath .) --bindir=$(realpath .)

define AUTORECONF
$(TOP)/configure: $(TOP)/configure.ac $(TOP)/Makefile.in
	cd $(TOP) && autoconf

$(TOP)/configure.ac: | $(TOP)

$(TOP)/config.h.in: | $(TOP)
	cd $(TOP) && autoheader

$(TOP)/Makefile.in: $(TOP)/Makefile.am $(TOP)/aclocal.m4 $(TOP)/config.h.in
	cd $(TOP) && automake --add-missing

$(TOP)/aclocal.m4: $(TOP)/m4/libtool.m4
	cd $(TOP) && aclocal -I m4

$(TOP)/m4/libtool.m4: | $(TOP)
	cd $(TOP) && libtoolize --automake
endef

TOP:=.
$(eval $(AUTORECONF))
TOP:=libxml2
$(eval $(AUTORECONF))
TOP:=libxmlfixes
$(eval $(AUTORECONF))

libxml2: | libxmlfixes
	ln -rs libxmlfixes/libxml2 libxml2
libxmlfixes:
	git submodule init --update
build:
	mkdir $@

.PHONY: all
