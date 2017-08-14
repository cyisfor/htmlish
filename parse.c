#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <assert.h>
#include <string.h>

#define BUFSIZE 0x1000

static void processText(xmlChar* text, int lastisElement, int nextisElement) {
    xmlChar* start = text;
    xmlChar* end = start;
    int first = 1;
    while(end) {
        end = strchr(start,'\n');
        if(end==NULL) {
            if(strlen(start)>0) {
                if(!(first && lastisElement))
                    fputs("<p>",stdout);
                first = 0;
                fputs(start,stdout);
                if(!nextisElement)
                    puts("</p>");
            }
        } else {
            if(start<end) {
                if(!(first && lastisElement))
                    fputs("<p>",stdout);
                first = 0;
                fwrite(start,1,end-start,stdout);
                puts("</p>");
            }
            start = end+1;
            end = start;
        }
    }
}

static void printElement(xmlNode* cur) {
    printf("<%s",cur->name);
    xmlAttr* attr = cur->properties;
    for(;attr;attr=attr->next) {
        printf(" %s=\"%s\"",attr->name,attr->children->content);
    }
    if(cur->children==NULL) {
        fputs("/>",stdout);
        return;
    } else {
        fputc('>',stdout);
        xmlNode* parent = cur;
        for(cur = parent->children; cur; cur = cur->next) {
            switch(cur->type) {
            case XML_ELEMENT_NODE:
                printElement(cur);
                continue;
            case XML_TEXT_NODE:
                fputs(cur->content,stdout);
                continue;
            default:
                fprintf(stderr,"wtf is %d",cur->type);
                exit(3);
            };
        }
        fputs("</",stdout);
        fputs(parent->name,stdout);
        fputc('>',stdout);
    }
}

static void processRoot(xmlNode* root) {
    xmlNode* cur;
    int lastisElement = 0;
    for(cur=root->children;cur;cur = cur->next) {
        switch(cur->type) {
        case XML_ELEMENT_NODE:
            printElement(cur);
            lastisElement = 1;
            continue;
        case XML_TEXT_NODE:
            processText(cur->content,
                        lastisElement,
                        cur->next && cur->next->type != XML_TEXT_NODE );
            lastisElement = 0;
            continue;
        default:
            fprintf(stderr,"wwtf is %d",cur->type);
            exit(3);
        };
    }
}

#define HEADER "<?xml version=\"1.0\"?>\n<root>"
#define FOOTER "</root>"

xmlDoc* readFunky(void) {
    xmlParserCtxtPtr ctx;
    char buf[BUFSIZE];
    ctx = xmlCreatePushParserCtxt(NULL, NULL,
                                   "",0,"htmlish.xml");
    assert(ctx);

    xmlParseChunk(ctx, HEADER, sizeof(HEADER)-1, 0);
    for(;;) {
        ssize_t amt = read(0,buf,BUFSIZE);
        if(amt<=0) break;
        xmlParseChunk(ctx, buf, amt, 0);
    }

    xmlParseChunk(ctx,FOOTER,sizeof(FOOTER)-1, 1);
    xmlDoc* doc = ctx->myDoc;
    if(!ctx->wellFormed) {
        fprintf(stderr,"Warning: not well formed.\n");
    }
    xmlFreeParserCtxt(ctx);
    return doc;
}

int main(void) {

    LIBXML_TEST_VERSION;

    xmlDoc* doc = readFunky();
    assert(doc);
    xmlNode* root = xmlDocGetRootElement(doc);
    assert(root);
    xmlNode* cur = root;
    puts("<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
         "<html lang=\"en\" xml:lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\">\n"
         "<head>");
    if(getenv("title")) {
        printf("<title>%s</title>\n",getenv("title"));
    }
    puts("</head><body>");
    processRoot(root);
    puts("</body></html>");
    return 0;
}
