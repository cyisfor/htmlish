all: parse unparse

CFLAGS:=$(CFLAGS) -g `xml2-config --cflags`
LDFLAGS:=$(LDFLAGS) -g `xml2-config --libs`

unparse: unparse.o input.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)
parse: parse.o input.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)
