#define _GNU_SOURCE // memcmp

#include "parse_chat.h"
#include "libxmlfixes/src/libxmlfixes.h"
#include <libxml/HTMLtree.h> // output
#include <libxml/HTMLparser.h> // input
#include <libxml/tree.h> // xmlCopyDoc

#include <sys/mman.h> // mmap
#include <sys/stat.h>

#include <stdio.h>
#include <assert.h>
#include <string.h> // memcmp

#include <fcntl.h> // open, O_RDONLY
#include <unistd.h> // close

#define LITLEN(a) (a),sizeof(a)-1

int main(int argc, char *argv[])
{

	LIBXML_TEST_VERSION;

	void on_error(void * userData, xmlErrorPtr error) {
		if(htmlish_handled_error(error)) return;
	}
	xmlSetStructuredErrorFunc(NULL,on_error);
	
	xmlDoc* template = htmlReadMemory(LITLEN(
																			"<!DOCTYPE html>\n"
																			"<html><body/></html>"),
		"about:blank",
		"UTF-8",
		0);

	int i = 13;

		char buf[0x100];
		snprintf(buf,0x100,"test/parse%d.hish",i);
		int fd = open(buf,O_RDONLY);
		assert(fd > 0);
		printf("test %d...",i);
		fflush(stdout);
		xmlDoc* doc = readFunky(fd,LITLEN("bad file passed"));
		assert(doc);
		parse_chat(doc);
		xmlNode* content = doc->children->next->children; // doctype -> html -> body
		assert(content);
		close(fd);
		xmlChar* result = NULL;
		int rlen = 0;
		htmlDocDumpMemory(doc,&result,&rlen);
		snprintf(buf,0x100,"test/parse%d.html",i);
		fd = open(buf,O_RDONLY);
		if(fd < 0) {
			write(1,result,rlen);
			printf("check test/parse%d.html\n",i);
WRITE_ANYWAY:
			fd = open(buf,O_WRONLY|O_CREAT|O_TRUNC,0644);
			assert(fd >= 0);
			write(fd,result,rlen);
			exit(23);
		} else {
			struct stat info;
			fstat(fd, &info);
			char* expected = mmap(NULL,info.st_size,PROT_READ,MAP_PRIVATE,fd,0);
			close(fd);
			if(info.st_size != rlen) {
				printf("sizes don't match! expected: %d actual: %d\n",info.st_size,rlen);
			}
			if(info.st_size != rlen || 0 != memcmp(expected,result,rlen)) {
				puts("expected:");
				fwrite(expected,info.st_size,1,stdout);
				puts("actual:");
				fwrite(result,rlen,1,stdout);
				puts("^C to not update derp");
				getchar();
				goto WRITE_ANYWAY;
			}
		
		puts("passed.");
	}
			
	return 0;
}
