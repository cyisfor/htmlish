#include <libxml/tree.h>
#include <libxml/HTMLParser.h>

#include <stdio.h>

inline void printCollapseWhitespace(xmlChar* s) {
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
            default:
                // print, then increment
                putchar(*s++);
        };
    }
    putchar('\n\n');
}

int main(void) {
    xmlBufferPtr buf = xmlBufferCreateSize(1024);
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
        


inline void showDocument(xmlNodePtr cur) {
    while(cur) {
        switch(cur->type) {
            case XML_ELEMENT_NODE:
                if(0==strncmp("p",cur->name,1)) {
                    showParagraph(cur);
                } else {
                    showDocument(cur->children);
                }
            break;
        }
    }
}




    xmlNodePtr root = (xmlNodePtr)doc;
    showDocument(root);
    return 0;
}
