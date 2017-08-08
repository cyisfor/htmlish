#include "input.h"
#include "parse.h"

#include <libxml/HTMLParser.h>

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

		htmlish(output,0);
    htmlSaveFileFormat("-",output,"utf-8",1);
    return 0;
}
