BUILT_SOURCES = libxmlfixes/wanted_tags.gen.h

ACLOCAL_AMFLAGS = -I m4

XMLINC=-I$(srcdir)/libxml2/include \
	-I$(srcdir)/libxmlfixes/ \
	-I$(srcdir)/libxmlfixes/libxml2/build/include

bin_PROGRAMS = parse

lib_LTLIBRARIES = libhtmlish.la

AM_CFLAGS=$(XMLINC) -I $(srcdir)/src -I $(srcdir)/libxmlfixes/src

libhtmlish_la_SOURCES = \
	src/htmlish.c \
	src/parse_chat.c \
	src/input.c \
	src/error.c
libhtmlish_la_LIBADD = libxmlfixes/lib/libxmlfixes.la

parse_LDADD = libhtmlish.la \
	$(srcdir)/libxmlfixes/libxmlfixes.la \
	$(srcdir)/libxml2/lib/libxml2.la

parse_SOURCES = src/main.c

libxmlfixes/lib/libxmlfixes.la libxmlfixes/wanted_tags.gen.h:
	$(MAKE) -C $(srcdir)/libxmlfixes install
libxml2/lib/libxml2.la: | libxml2
	$(MAKE) -C libxml2/build

libxml2:
	ln -rs libxmlfixes/libxml2 libxml2
