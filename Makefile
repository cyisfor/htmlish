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

html_when/libhtmlwhen.a: descend

descend:
	$(MAKE) -C html_when libhtmlwhen.a

.PHONY: descend

o/%.o: src/%.c libxml2/$(XMLVERSION) | o
	$(CC) $(CFLAGS) -c -o $@ $<

o:
	mkdir $@

libxml2/include/xmlversion.h: descend

setup: libxml2 html_when

libxml2: ./html_when/libxml2
	ln -rs $< $@
./html_when/libxml2: html_when

html_when: 
	if [[ -d $@ ]]; then \
		git clone --recurse-submodules https://github.com/cyisfor/html_when.git pending-$@ && \
		$(MAKE) -C pending-$@ && \
		mv pending-$@ $@ \
	else \
		cd $@ && git pull
