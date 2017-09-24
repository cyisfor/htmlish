#include "input.h"

#include <libxml/tree.h>
#include <libxml/HTMLparser.h>

#include <ctype.h> // isspace
#include <string.h> // strncmp

#include <stdio.h>

static void printCollapseWhitespace(xmlChar* s) {
    for(;;) {
        switch(*s) {
            case 0:
                return;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                // current is space, go past the spaces
                while(isspace(*++s)) ;
                putchar(' ');
                continue;
            default:
                // print, then increment
                putchar(*s++);
        };
    }
    putchar('\n');
    putchar('\n');
}

int main(void) {
    xmlBufferPtr buf = xmlBufferCreateSize(1024);
    setupInput();
    htmlDocPtr doc = htmlReadFd(0, "stdin.html", "utf-8", HTML_PARSE_RECOVER | HTML_PARSE_NOBLANKS);
    
    void showParagraph(xmlNodePtr p) {
        p = p->children;
        while(p) {
            xmlNodeDump(buf,(xmlDocPtr)doc,p,0,0);
            p = p->next;
        }
        xmlChar* content = xmlBufferDetach(buf);
        printCollapseWhitespace(content);
        xmlFree(content);
    }

    void showDocument(xmlNodePtr cur) {
        while(cur) {
            switch(cur->type) {
                case XML_HTML_DOCUMENT_NODE:
                    return showDocument(cur->children);
                case XML_ELEMENT_NODE:
                    {
                    if(0==strncmp("p",cur->name,1)) {
                        showParagraph(cur);
                    } else {
                        showDocument(cur->children);
                    }
                    }
                    break;
            };
            cur = cur->next;
        }
    }

    puts("-----------------------------------");
    xmlSaveFile("-",doc);
    puts("-----------------------------------");
    showDocument((xmlNodePtr)doc);
    return 0;
}
