all: test_parse test_copynode parse unparse libhtmlish.a
XMLVERSION:=include/libxml/xmlversion.h
CFLAGS+=-g -Ilibxml2/include -Ihtml_when/src/
LINK=gcc $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')

O=$(patsubst %,o/%.o,$N) libxml2/.libs/libxml2.a html_when/libhtmlwhen.a 
S=$(patsubst %,src/%.c,$N)

N=test_copynode
test_copynode: $O 
	$(LINK)

N=test_parse htmlish 
test_parse: $O
	$(LINK)

N=unparse input
unparse: $O 
	$(LINK)

N=main htmlish input 
parse: $O 
	$(LINK)

N=htmlish input
libhtmlish.a: $O
	sh ./funnyar.sh $@ $O | ar -M

libxml2/.libs/libxml2.a html_when/libhtmlwhen.a: descend

descend:
	$(MAKE) -C html_when libhtmlwhen.a

.PHONY: descend html_when

o/%.o: src/%.c libxml2/$(XMLVERSION) | o
	$(CC) $(CFLAGS) -c -o $@ $<

o:
	mkdir $@

libxml2/include/xmlversion.h: descend

setup:
	sh setup.sh
	$(MAKE) -C html_when setup
