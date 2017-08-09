#include "parse.h"

#include <libxml/HTMLtree.h> // output
#include <libxml/HTMLparser.h> // input
#include <libxml/tree.h> // xmlCopyDoc

#include <stdio.h>
#include <assert.h>

#include <fcntl.h> // open, O_RDONLY

#define LITLEN(a) (a),sizeof(a)-1

int main(int argc, char *argv[])
{
	xmlDoc* template = htmlReadMemory(LITLEN(
																			"<!DOCTYPE html>\n"
																			"<html><body/></html>"),
		"about:blank",
		"UTF-8",
		0);

	int i;
	for(i=1;;++i) {
		char buf[0x100];
		snprintf(buf,0x100,"test/parse%d.hish",i);
		int fd = open(buf,O_RDONLY);
		if(fd < 0) break;
		printf("test %s\n",buf);
		xmlDoc* doc = xmlCopyDoc(template, 1);
		xmlNode* content = doc->children->next->children; // doctype -> html -> body
		assert(content);
		htmlish(content,fd);
		close(fd);
		htmlDocDump(stdout,doc);
		snprintf(buf,0x100,"test/parse%d.html",i);
		fd = open(buf,O_RDONLY);
		if(fd < 0) {
			printf("setup this to go in test/parse%d.html",i);
			exit(23);
		}
	}
			
	return 0;
}
