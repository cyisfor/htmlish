all: test_parse test_copynode parse unparse

CFLAGS+=-g -Ilibxml2/include/ -Ihtml_when/source/
LINK=gcc $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')

O=$(patsubst %,o/%.o,$N) libxml2/.libs/libxml2.a html_when/libhtmlwhen.a
S=$(patsubst %,src/%.c,$N)

N=test_copynode
test_copynode: $O 
	$(LINK)

N=test_parse htmlish libxmlfixes
test_parse: $O
	$(LINK)

N=unparse input
unparse: $O 
	$(LINK)

N=main htmlish input libxmlfixes
parse: $O 
	$(LINK)

libxml2/.libs/libxml2.a: libxml2/configure
	cd libxml2 && ./configure && make -j4

libxml2/configure:
	cd libxml2 && sh autogen.sh

html_when/libhtmlwhen.a: always
	$(MAKE) -C html_when

always:

o/%.o: src/%.c | o
	$(CC) $(CFLAGS) -c -o $@ $<

o:
	mkdir $@
