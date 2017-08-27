#include "html_when.h"

#include "input.h"
#include "htmlish.h"
#include "libxmlfixes/libxmlfixes.h"
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/xpath.h> // NodeSetPtr
#include <libxml/xmlerror.h> // Set/GetError
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h> // open, O_*
#include <unistd.h> // close
#include <string.h> // strlen, memcmp

const char defaultTemplate[] =
  "<!DOCTYPE html>\n"
  "<html>\n"
  "<head><meta charset=\"utf-8\"/>\n"
  "<title/><header/></head>\n"
  "<body><h1><intitle/></h1>\n"
  "<top/><content/><footer/>\n"
  "</body></html>";


static void parseEnvFile(const char* path, xmlNodeSetPtr nodes) {
	if(!path) return;

	int inp = open(path,O_RDONLY);
	size_t plen = strlen(path);
	xmlDoc* doc = readFunky(inp,path,plen);
	close(inp);
	if(!doc) {
		fprintf(stderr,"Couldn't parse %.*s",plen,path);
		exit(5);
	}
	xmlNode* root = xmlDocGetRootElement(doc);
	xmlNode* cur = root;  // body
	nodes->nodeNr = 0;
	for(;cur;cur = cur->next) {
		++nodes->nodeNr;
	}
	nodes->nodeTab = malloc(sizeof(xmlNode*)*nodes->nodeNr);
	cur = root;
	int i = 0;
	for(;cur;++i) {
		xmlNode* next = cur->next;
		nodes->nodeTab[i] = cur;
		xmlUnlinkNode(cur);
		cur = next;
	}
	xmlFreeDoc(doc);
}


struct dbfderp {
	const char* name;
	const char* path;
	// save these, so we can copy them multiple times for every <header> tag or whatnot
	xmlNodeSet replacement;
};

static void doByFile2(xmlNode* target, void* ctx) {
	struct dbfderp* derp = (struct dbfderp*) ctx;
	if(!derp->path) {
		if(target) {
			xmlUnlinkNode(target);
			xmlFreeNode(target);
		}
	} else {
		if(!target) {
			fprintf(stderr,"No target found for %s\n",derp->name);
		}
		xmlNode* cur = target;
		int i = 0;
		for(;i<derp->replacement.nodeNr;++i) {
			xmlNode* new = xmlDocCopyNode(derp->replacement.nodeTab[i],cur->doc,1);
			xmlAddNextSibling(cur,new);
			cur = new;
		}
		xmlUnlinkNode(target);
		xmlFreeNode(target);
	}
}

/* Add the XML fragment contained in a file, in the place of
 * a specified placeholder element such as <header/>
 */

static void doByFile(xmlDoc* output, const char* name) {
	const char* path = getenv(name);
	struct dbfderp derp = { name, path };
	parseEnvFile(derp.path,&derp.replacement);
	foreachNode(xmlDocGetRootElement(output),name,doByFile2,&derp);
	int i = 0;
	for(;i<derp.replacement.nodeNr;++i) {
		xmlFreeNode(derp.replacement.nodeTab[i]);
	}
	free(derp.replacement.nodeTab);
}


static void dump_to_fd(int dest, xmlDoc* src) {
	xmlOutputBuffer* out = xmlOutputBufferCreateFd(dest,encoding);
	ensure_ne(out,NULL);
	/* note, the encoding string passed to htmlDocContentDumpOutput is
		 totally ignored, and should not be there, since xmlOutputBuffer
		 handles encoding from this point.
	*/
	htmlDocContentDumpOutput(out,src,NULL);
	// libxml
	ensure_ge(xmlOutputBufferClose(out),0);
}



int main(void) {

    if(getenv("printtemplate")) {
        puts(defaultTemplate);
        return 0;
    }
    LIBXML_TEST_VERSION;

		void on_error(void * userData, xmlErrorPtr error) {
			if(error->code == XML_HTML_UNKNOWN_TAG) {
				const char* name = error->str1;
				size_t nlen = strlen(name);
				#define IS(a) (nlen == sizeof(a)-1) && (0 == memcmp(name,a,sizeof(a)-1))
				if(IS("top") || IS("content") || IS("header") || IS("footer") || IS("intitle")
					 || IS("when")) // XXX: coupling arrrrgh
					// not errors, these get removed by template stuffs
					return;
			}
			fprintf(stderr,"um %d %s %s\n",error->code, error->message,
							error->level == XML_ERR_FATAL ? "fatal..." : "ok");
			return;
		}
		xmlSetStructuredErrorFunc(NULL,on_error);

    setupInput();

    xmlDoc* output;
    if(getenv("template")) {
        output = htmlParseFile(getenv("template"),"utf-8");
        if(!output) {
            fprintf(stderr,"Error: template failed... not sure if well formed or not.\n");
            exit(2);
        }
    } else {
        output = htmlReadMemory(defaultTemplate,sizeof(defaultTemplate),"","utf-8",
																HTML_PARSE_RECOVER |
																HTML_PARSE_NOBLANKS |
																HTML_PARSE_COMPACT);
    }
		/* This sets up the template, filling in placeholder elements
			 with the stuff that the environment carries */

		doByFile(output,"header");
		doByFile(output,"top");
		doByFile(output,"footer");

		bool as_child = false;
		xmlNode* content = getContent(xmlDocGetRootElement(output),false,&as_child);
		htmlish(content,0,as_child);
		if(!as_child) {
			// throw away placeholder node
			xmlUnlinkNode(content);
			xmlFreeNode(content);
		}
		html_when((xmlNode*)output); // manage <when> logic
    dump_to_fd(1,output);
    return 0;
}
