#define _GNU_SOURCE // memcmp

#include "libxmlfixes/src/libxmlfixes.h"
#include "htmlish.h"

#include <libxml/HTMLtree.h> // output
#include <libxml/HTMLparser.h> // input
#include <libxml/tree.h> // xmlCopyDoc
#include <libxml/xmlIO.h> // buffers

#include <sys/mman.h> // mmap
#include <sys/stat.h>
#include <sys/wait.h> // waitpid

#include <stdio.h>
#include <assert.h>
#include <string.h> // memcmp

#include <fcntl.h> // open, O_RDONLY
#include <unistd.h> // close

#define LITLEN(a) (a),sizeof(a)-1


static void dump_to_mem(xmlDoc* doc,xmlChar** result, size_t* rlen) {
	// SIGH
	static xmlCharEncodingHandler* encoding = NULL;
	xmlOutputBuffer* out = xmlAllocOutputBuffer(encoding);
	assert(out != NULL);
	/* note, the encoding string passed to htmlDocContentDumpOutput is
		 totally ignored, and should not be there, since xmlOutputBuffer
		 handles encoding from this point.
	*/
	htmlDocContentDumpOutput(out,doc,NULL);
	*rlen = xmlOutputBufferGetSize(out);
	const xmlChar* derp = xmlOutputBufferGetContent(out);
	*result = malloc(*rlen);
	memcpy(*result, derp, *rlen);
	
	assert(xmlOutputBufferClose(out) >= 0);
}

const char defaultTemplate[] =
  "<!DOCTYPE html>\n"
  "<html>\n"
  "<head><meta charset=\"utf-8\"/>\n"
  "</head>\n"
  "<body/></html>";

void on_error(void * userData, xmlErrorPtr error) {
	if(htmlish_handled_error(error)) return;
}

int main(int argc, char *argv[])
{

	LIBXML_TEST_VERSION;

	xmlSetStructuredErrorFunc(NULL,on_error);

	xmlDoc* template = htmlReadMemory(defaultTemplate,sizeof(defaultTemplate)-1,
																		"about:blank",
																		"utf-8",
																		HTML_PARSE_RECOVER |
																		HTML_PARSE_NOBLANKS |
																		HTML_PARSE_COMPACT);

	void testone(int i) {
		char buf[0x100];
		snprintf(buf,0x100,"test/parse%d.hish",i);
		int fd = open(buf,O_RDONLY);
		if(fd < 0) {
			exit(0);
		}
		printf("test %d...",i);
		fflush(stdout);
		xmlDoc* doc = xmlCopyDoc(template, 1);
		// doc -> doctype -> html -> head -> body 
		xmlNode* content = nextE(doc->children->next->children->next); 
		assert(content);
		htmlish(content,fd,true);
		close(fd);
		xmlChar* result = NULL;
		size_t rlen = 0;
		dump_to_mem(doc,&result,&rlen);
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
				printf("sizes don't match! expected: %ld actual: %ld\n",info.st_size,rlen);
			}
			if(info.st_size != rlen || 0 != memcmp(expected,result,rlen)) {
				puts(buf);
				sleep(1);
				puts("expected:");
				fwrite(expected,info.st_size,1,stdout);
				puts("actual:");
				fwrite(result,rlen,1,stdout);
				FILE* d = fopen("derp","wt");
				fwrite(result, rlen, 1, d);
				fclose(d);
				int pid = fork();
				if(pid == 0) {
					execlp("diff","diff","-u",buf,"derp",NULL);
				}
				waitpid(pid,NULL,0);
				puts("^C to not update derp");
				getchar();
				goto WRITE_ANYWAY;
			}
		}
		puts("passed.");
	}
	if(getenv("test")) {
		testone(strtol(getenv("test"),NULL,0));
	} else {
		int i;
		for(i=1;;++i) {
			testone(i);
		}
	}
			
	return 0;
}
