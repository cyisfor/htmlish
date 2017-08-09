#include "parse.h"

#include <libxml/HTMLtree.h> // output
#include <libxml/HTMLparser.h> // input

int main(int argc, char *argv[])
{
	xmlDoc* template = htmlParseMemory(LITLEN(
																 "<!DOCTYPE html>\n"
																 "<html><body/></html>"));

	int i;
	for(i=0;;++i) {
		char buf[0x100];
		snprintf(buf,"test/parse%d.hish",i);
		int fd = open(buf,O_RDONLY);
		if(fd < 0) break;
		printf("test %s\n",buf);
		xmlDoc* doc = xmlCopyDoc(template);
		xmlDoc* content = doc->children->children; // body
		assert(content);
		htmlish(content,fd);
		htmlDocDump(stdout,doc);
		puts("setup this to be the expected result");
		exit(23);
	}
			
	return 0;
}
