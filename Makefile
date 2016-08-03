all: parse unparse
export PATH:=/custom/libxml2/bin:$(PATH)
CFLAGS:=$(CFLAGS) -g `xml2-config --cflags`
LDFLAGS:= -Wl,-rpath=/custom/libxml2/lib $(LDFLAGS) -g `xml2-config --libs`

unparse: unparse.o input.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)
parse: parse.o input.o
	xml2-config --libs
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)
