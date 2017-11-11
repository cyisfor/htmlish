all: setup test_parse test_parse_chat test_copynode parse unparse libhtmlish.a

XMLVERSION:=include/libxml/xmlversion.h
CFLAGS+=-g  -Ilibxml2/include -Ihtml_when/src/ -Ihtml_when/
LINK=libtool --tag CC --mode link $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
LDLIBS+=$(shell xml2-config --libs | sed -e's/\s*-lxml2\s*//g')

N=test_copynode
OUT=test_copynode
$(eval $(PROGRAM))

N=test_parse_chat parse_chat error
OUT=test_parse_chat
$(eval $(PROGRAM))

N=test_parse htmlish parse_chat error
OUT=test_parse
$(eval $(PROGRAM))

N=unparse input
OUT=unparse
$(eval $(PROGRAM))

N=main htmlish parse_chat input error
OUT=parse
$(eval $(PROGRAM))

N=htmlish parse_chat input error
OUT=libhtmlish.la
$(eval $(PROGRAM))
