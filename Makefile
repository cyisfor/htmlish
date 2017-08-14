all: test_parse test_copynode parse unparse libhtmlish.a
LIBXML:=html_when/libxml2/
XMLVERSION:=include/libxml/xmlversion.h
CFLAGS+=-g -I$(LIBXML)/include -Ihtml_when/src/
LINK=gcc $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
LDLIBS+=$(shell xml2-config --libs | sed -e's/-xml2//g')

O=$(patsubst %,o/%.o,$N) $(LIBXML)/.libs/libxml2.a html_when/libhtmlwhen.a 
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

html_when/libhtmlwhen.a: descend

descend:
	$(MAKE) -C html_when libhtmlwhen.a

.PHONY: descend

o/%.o: src/%.c $(LIBXML)/$(XMLVERSION) | o
	$(CC) $(CFLAGS) -c -o $@ $<

o:
	mkdir $@

$(LIBXML)/include/xmlversion.h: descend
