ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = parse
lib_LTLIBRARIES = libhtmlish.la

AM_CFLAGS=-I $(includedir)/libxml2/ -I $(includedir)/libxmlfixes/

libhtmlish_la_SOURCES = \
	src/htmlish.c \
	src/parse_chat.c \
	src/input.c \
	src/error.c
libhtmlish_la_HEADERS = src/htmlish.h src/input.h src/parse_chat.h
libhtmlish_ladir = $(includedir)/libhtmlish

parse_LDADD = libhtmlish.la \
	-L$(libdir) \
	-lxmlfixes \
	-lxml2

parse_SOURCES = src/main.c
