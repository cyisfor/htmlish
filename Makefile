all: build/Makefile
	$(MAKE) -C build && $(MAKE) -C build install

build/Makefile: configure | build
	cd build && ../configure --prefix=$(realpath .) --bindir=$(realpath .)

configure: configure.ac Makefile.am
	autoreconf -i

build:
	mkdir $@

.PHONY: all
