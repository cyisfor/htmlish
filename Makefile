all: parse

CFLAGS:=$(CFLAGS) -g `xml2-config --cflags`
LDFLAGS:=$(LDFLAGS) -g `xml2-config --libs`

parse: parse.o input.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)
