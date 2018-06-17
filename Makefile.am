BUILT_SOURCES = libxmlfixes/wanted_tags.gen.h

ACLOCAL_AMFLAGS = -I m4

XMLINC=-I$(srcdir)/libxml2/include -I$(srcdir)/libxml2/build/include

bin_PROGRAMS = parse

lib_LTLIBRARIES = libhtmlish.la

AM_CFLAGS=$(XMLINC) -I $(srcdir)/html_when/src -I $(srcdir)/src -I $(srcdir)/libxmlfixes/src

libhtmlish_la_SOURCES = \
	src/htmlish.c \
	src/parse_chat.c \
	src/input.c \
	src/error.c

parse_LDADD = libhtmlish.la \
	libxmlfixes/libxmlfixes.la \
	libxml2/libxml2.la
parse_SOURCES = src/main.c

libxmlfixes/libxmlfixes.la libxmlfixes/wanted_tags.gen.h:
	$(MAKE) -C libxmlfixes
libxml2/libxml2.la:
	$(MAKE) -C libxml2
