all: parse unparse

CFLAGS+=-g -Ilibxml2/include/

unparse: unparse.o input.o libxml2/.libs/libxml2.a
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)
parse: main.o parse.o input.o libxmlfixes.o libxml2/.libs/libxml2.a
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

libxml2/.libs/libxml2.a: libxml2/configure
	cd libxml2 && ./configure && make -j4

libxml2/configure:
	cd libxml2 && sh autogen.sh
