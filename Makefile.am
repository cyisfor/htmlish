BUILT_SOURCES =

ACLOCAL_AMFLAGS = -I m4

XMLINC=-I$(srcdir)/libxml2/include -I$(srcdir)/libxml2/build/include

lib_LTLIBRARIES = libhtmlish.la

libhtmlish_la_CFLAGS=$(XMLINC) -I $(srcdir)/html_when/src -I $(srcdir)/src -I $(srcdir)/libxmlfixes/src
libhtmlish_la_SOURCES = \
	src/htmlish.c \
	src/parse_chat.c \
	src/input.c \
	src/error.c

libhtmlish_la_LIBADD = $(srcdir)/html_when/libhtmlwhen.la
include m4/make-sublib.mk

A:=$(info $(call make_sublib, html_when, libhtmlwhen.la))
B:=$(info $(call git_local, html_when, $(HOME)/code/html_when, https://github.com/cyisfor/html_when.git))
