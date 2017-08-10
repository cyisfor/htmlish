CFLAGS+=-ggdb -Ilibxml2/include/
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')
test: source/app.c libhtmlwhen.a libxml2/.libs/libxml2.a
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

libxml2/.libs/libxml2.a: libxml2/configure
	cd libxml2 && ./configure && make

libxml2/configure:
	cd libxml2 && sh autogen.sh

libhtmlwhen.a: o/libxmlfixes.o o/html_when.o o/selectors.o | o
	$(AR) $(ARFLAGS) $@ $^

o:
	mkdir $@

o/%.o: source/%.c | o
	gcc $(CFLAGS) -c -o $@ $^
