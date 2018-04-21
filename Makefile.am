BUILT_SOURCES =

ACLOCAL_AMFLAGS = -I m4

XMLFIXES_C=$(srcdir)/libxmlfixes/src/libxmlfixes.c

XMLINC=-I$(srcdir)/libxml2/include -I$(srcdir)/libxml2/build/include

lib_LTLIBRARIES = libhtmlish.la

libhtmlish_la_CFLAGS=$(XMLINC) -I $(srcdir)/htmlish/html_when/src -I $(srcdir)/htmlish/src -I $(srcdir)/libxmlfixes/src
libhtmlish_la_SOURCES = \
	src/htmlish.c \
	src/parse_chat.c \
	src/input.c \
	src/error.c \
	$(XMLFIXES_C)

libhtmlish_la_LIBADD = html_when/libhtmlwhen.la
m4_define([@name@], [q])
m4_define([@target@], [libhtmlwhen.la])
MAKE_SUBLIB(

m4_define([@name@], [libxml2])
m4_define([@target@], [libxml2.la])
MAKE_SUBLIB
