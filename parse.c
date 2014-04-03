#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <assert.h>
#include <string.h>

#define BUFSIZE 0x1000

int inParagraph = 0;

#define PUTLITERAL(literal) fwrite(literal,1,sizeof(literal)-1,stdout)
#define PUTS(s) fwrite(s,1,strlen(s),stdout)
#define PUTC(c) fputc(c,stdout)

static void processText(xmlChar* text, int lastisElement, int nextisElement) {
    xmlChar* start = text;
    xmlChar* end = start;
    int first = 1;
    while(end) {
        end = strchr(start,'\n');
        if(end==NULL) {
            if(strlen(start)>0) {
                if(!inParagraph) {
                    inParagraph = 1;
                    PUTLITERAL("<p>");
                }
                first = 0;
                PUTS(start);
                if(!nextisElement && inParagraph) {
                    inParagraph = 0;
                    PUTLITERAL("</p>");
                }
            }
        } else {
            if(lastisElement) {
                PUTC('\n');
                lastisElement = 0;
            }
            if(end==start) {
                if(end[1]=='\0')
                    PUTC('\n');
            } else if(start<end) {
                if(!inParagraph) {
                    inParagraph = 1;
                    PUTLITERAL("<p>");
                }
                first = 0;
                fwrite(start,1,end-start,stdout);
                if(inParagraph) {
                    inParagraph = 0;
                    PUTLITERAL("</p>");
                }
            }
            start = end+1;
            end = start;
        }
    }
}

static void printElement(xmlNode* cur) {
    PUTC('<');
    PUTS(cur->name);
    xmlAttr* attr = cur->properties;
    for(;attr;attr=attr->next) {
        PUTC(' ');
        PUTS(attr->name);
        PUTC('=');
        PUTC('"');
        PUTS(attr->children->content);
        PUTC('"');
    }
    if(cur->children==NULL) {
        PUTLITERAL("/>");
        return;
    } else {
        PUTC('>');
        xmlNode* parent = cur;
        for(cur = parent->children; cur; cur = cur->next) {
            switch(cur->type) {
            case XML_COMMENT_NODE:
                PUTLITERAL("<!--");
                PUTS(cur->content);
                PUTLITERAL("-->\n");
                continue;
            case XML_ELEMENT_NODE:
                printElement(cur);
                continue;
            case XML_TEXT_NODE:
                PUTS(cur->content);
                continue;
            case XML_CDATA_SECTION_NODE:
                PUTLITERAL("<![CDATA[");
                PUTS(cur->content);
                PUTLITERAL("]]>");
                continue;
            default:
                fprintf(stderr,"wtf is %d",cur->type);
                exit(3);
            };
        }
        PUTLITERAL("</");
        PUTS(parent->name);
        PUTC('>');
    }
}

static void processRoot(xmlNode* root) {
    xmlNode* cur;
    int lastisElement = 0;
    for(cur=root->children;cur;cur = cur->next) {
        switch(cur->type) {
        case XML_COMMENT_NODE:
            PUTLITERAL("<!--");
            PUTS(cur->content);
            PUTLITERAL("-->\n");
            continue;
        case XML_ELEMENT_NODE:
            printElement(cur);
            lastisElement = 1;
            continue;
        case XML_CDATA_SECTION_NODE:
            PUTLITERAL("<![CDATA[");
            PUTS(cur->content);
            PUTLITERAL("]]>");
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

#define HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root>"
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

static void tryPut(const char* maybeFile) {
    if(!maybeFile) return;

    FILE* inp = fopen(maybeFile,"rt");
    if(inp) {
        char buf[0x1000];
        for(;;) {
            ssize_t amt = fread(buf,1,0x1000,inp);
            if(amt<=0) break;
            fwrite(buf,1,amt,stdout);
        }
        fclose(inp);
    } else {
        // XXX: this is a horrible hack how do
        if(strchr(maybeFile,'\n') == NULL && strchr(maybeFile,'<')==NULL) return;
        // the file doesn't exist, so it must be a text blurb
        fwrite(maybeFile,1,strlen(maybeFile),stdout);
    }
}

int main(void) {

    LIBXML_TEST_VERSION;

    xmlDoc* doc = readFunky();
    assert(doc);
    xmlNode* root = xmlDocGetRootElement(doc);
    assert(root);
    xmlNode* cur = root;
    PUTLITERAL("<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
         "<html lang=\"en\" xml:lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\">\n"
         "<head>\n"
         "<meta http-equiv=\"content-type\" content=\"application/xhtml+xml; charset=utf-8\" />");
    const char* title = getenv("title");
    if(title) {
        PUTLITERAL("<title>");
        PUTS(title);
        PUTLITERAL("</title>\n");
    }
    const char* style = getenv("style");
    if(style) {
        PUTLITERAL("<link rel=\"stylesheet\" href=\"");
        PUTS(style);
        PUTLITERAL("\" />");
    }
    tryPut(getenv("head"));
    PUTLITERAL("</head><body>");
    tryPut(getenv("top"));
    if(title) {
        PUTLITERAL("<h1>");
        PUTS(title);
        PUTLITERAL("</h1>\n");
    }
    processRoot(root);
    if(inParagraph)
        PUTLITERAL("</p>");
    tryPut(getenv("footer"));
    PUTLITERAL("</body></html>");
    return 0;
}
