all: test_parse test_copynode parse unparse libhtmlish.a
LIBXML:=html_when/libxml2/

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

o/%.o: src/%.c | o
	$(CC) $(CFLAGS) -c -o $@ $<

o:
	mkdir $@

N=htmlish
$O: $(LIBXML)/include/xmlversion.h

$(LIBXML)/include/xmlversion.h: descend
