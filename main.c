#include "input.h"
#include "parse.h"

#include <libxml/HTMLparser.h>

static void parseEnvFile(const char* path, xmlNodeSetPtr nodes) {
	if(!path) return;

	int inp = open(path,O_RDONLY);
	xmlDoc* doc = readFunky(inp,path);
	close(inp);
	if(!doc) {
		fprintf(stderr,"Couldn't parse %s",path);
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
}


int main(void) {

    if(getenv("printtemplate")) {
        puts(defaultTemplate);
        return 0;
    }
    LIBXML_TEST_VERSION;

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

		htmlish(getContent(xmlDocGetRootElement(output),false),0);
    htmlSaveFileFormat("-",output,"utf-8",1);
    return 0;
}
