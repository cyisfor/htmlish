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

include m4/make-sublib.mk
m4_define([@name@], [html_when])
m4_define([@target@], [libhtmlwhen.la])
@MAKE_SUBLIB@

name:=libxml2
target:=libxml2.la
$(eval $(make_sublib))
