all: test_copynode parse unparse

CFLAGS+=-g -Ilibxml2/include/
LINK=gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)
O=$(patsubst %,o/%.o,$N)
S=$(patsubst %,src/%.c,$N)

N=test_copynode
test_copynode: $O libxml2/.libs/libxml2.a
	$(LINK)

N=unparse input
unparse: $O libxml2/.libs/libxml2.a
	$(LINK)

N=main parse input libxmlfixes
parse: $O libxml2/.libs/libxml2.a
	$(LINK)

libxml2/.libs/libxml2.a: libxml2/configure
	cd libxml2 && ./configure && make -j4

libxml2/configure:
	cd libxml2 && sh autogen.sh

o/%.o: src/%.c | o
	$(CC) $(CFLAGS) -c -o $@ $<

o:
	mkdir $@
