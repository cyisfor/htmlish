all: parse

parse: parse.c
	gcc $(CFLAGS) -g `xml2-config --cflags --libs` -o $@ $^
