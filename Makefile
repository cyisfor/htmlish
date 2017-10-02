all: setup test_parse test_parse_chat test_copynode parse unparse libhtmlish.a

XMLVERSION:=include/libxml/xmlversion.h
CFLAGS+=-g  -Ilibxml2/include -Ihtml_when/src/ -Ihtml_when/
LINK=gcc $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
LDLIBS+=$(shell xml2-config --libs | sed -e's/\s*-lxml2\s*//g')

O=$(patsubst %,o/%.o,$N) libxml2/.libs/libxml2.a html_when/libhtmlwhen.a html_when/libxmlfixes/libxmlfixes.a 
S=$(patsubst %,src/%.c,$N)


o/%.o: src/%.c libxml2/$(XMLVERSION) | o 
	$(CC) $(CFLAGS) -c -o $@ $<

N=test_copynode
test_copynode: $O 
	$(LINK)

N=test_parse_chat parse_chat
test_parse_chat: $O
	$(LINK)

N=test_parse htmlish parse_chat
test_parse: $O
	$(LINK)

N=unparse input
unparse: $O 
	$(LINK)

N=main htmlish parse_chat input 
parse: $O 
	$(LINK)

N=htmlish parse_chat input
libhtmlish.a: $O
	sh ./funnyar.sh $@ $O | ar -M

libxml2/.libs/libxml2.a html_when/libhtmlwhen.a: descend

descend:
	$(MAKE) -C html_when libhtmlwhen.a

.PHONY: 	html_when clean

o:
	mkdir $@

libxml2/$(XMLVERSION): descend

setup: ./setup.sh
	sh ./setup.sh
	$(MAKE) -C html_when setup

./setup.sh: git-tools/funcs.sh
git-tools/funcs.sh:
	git submodule update --init

git-tools/pushcreate: git-tools/pushcreate.c
	$(MAKE) -C git-tools

push: setup ./git-tools/pushcreate 
	[[ -n "$(remote)" ]]
	./git-tools/pushcreate "$(remote)"
	$(MAKE) -C html_when push remote="$(remote)/html_when"

clean:
	git clean -ndx
	@echo 'OK? (^C if not)'
	@read
	git clean -fdx
