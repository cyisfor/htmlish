ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = parse
lib_LTLIBRARIES = libhtmlish.la

AM_CFLAGS=-I libxml2/include -I $(srcdir)/libxml2/include \
		-I libxmlfixes/src -I $(srcdir)/libxmlfixes/src

libhtmlish_la_SOURCES = \
	src/htmlish.c \
	src/parse_chat.c \
	src/input.c \
	src/error.c
libhtmlish_LIBADD = libxmlfixes/libxmlfixes.la
libhtmlish_la_HEADERS = src/htmlish.h src/input.h src/parse_chat.h
libhtmlish_ladir = $(includedir)/libhtmlish

parse_LDADD = libhtmlish.la

parse_SOURCES = src/main.c

noinst_SUBDIRS = libxmlfixes
