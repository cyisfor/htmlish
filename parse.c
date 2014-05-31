#define LIBXML2_NEW_BUFFER

#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <assert.h>
#include <string.h>

/* possible states:
 * 1 text+blankline, element = end paragraph in the middle
 * 2 element, blankline+text = start paragraph in the middle
 * 3 text(noblank), element = nothing
 * 4 element, (noblank)+text = nothing
 * 5 file start, text = start paragraph before
 * 6 file start, element = nothing
 * 7 text, file end = end paragraph after
 * 8 element, file end = nothing
 *
 * state = e, text can follow with state = text, e (text is same even if blankline etc)
 * cannot start with "end paragraph" states or "file end" states
 * starting states ending in element always do nothing
 *
 * 1 = end state (for example)
 * 2, 1 = wrap in a paragraph e, p( t.strip ), e
 * 2, 3 = e, p(t.strip, e)
 * 2, 7 = wrap in a paragraph e, p( t.strip ), file end
 * 3, 2 = nothing (for example)
 * 4, 1 = p(e, t), e
 * 4, 3 = nothing
 * 4, 7 = p(e, t)
 * 5, 1 = p(t), e
 * 5, 3 = p(t, e)
 * 5, 7 = p(t)
 *
 * A = file start, B = first thingy or file end
 * if B is file end, empty document, done
 * C = second thingy or file end
 * if C is file end
 *   if A is text, wrap in paragraph
 *   done
 * until done
 *   A = B
 *   B = C
 *   repeat
 *     C = next thingy
 *     if B is text and C is text
 *       (text, text ? bleah!)
 *       B = append(B,C)
 *     else 
 *       break
 *   if A is file start (state 5)
 *     if B is element continue    
 *     if C is file end (state 5,7)
 *       makeP(B)
 *       break done
 *     if B.endblank (state 5,1)
 *       makeP(B)
 *     else (state 5,3)
 *       makeP(B,C)
 *       UGH also have to split text in the middle...
 *   else if A is element (states 2, 4)
 *     if B is element continue
 *     ( should have broken loop if C is file end, so B is never file end)
 *     if B.startblank (state 2)
 *       if C is element (states 1, 3)
 *         if B.endblank (state 2,1)
 *           makeP(B)
 *         else (state 2,3)
 *           makeP(B,C)
 *       else if C is file end (state 2,7)
 *         makeP(B)
 *         break done
 *     else (state 4)
 *       if C is element (states 1, 3)
 *         if B.endblank (state 4,1)
 *           makeP(A,B)
 *       else if C is end file (state 4,7)
 *         makeP(A,B)
 *         break done
 *   else (states 1, 3)
 *     if B is element 
 *   if A,B is state (1,3,7,8) continue
 *   if A,B is state 2
 *
 * first, check if starting state ends in element, do nothing
 * second, 
 */

#define BUFSIZE 0x1000

int inParagraph = 0;

xmlOutputBufferPtr xout = NULL;

#define PUTLITERAL(literal) xmlOutputBufferWrite(xout,sizeof(literal)-1,literal)
#define PUTPLAIN(s) xmlOutputBufferWrite(xout,strlen(s),s)
#define PUTC(c) xmlOutputBufferWrite(xout,1,&c);

void mywrite(const xmlChar* input, ssize_t len) {
    int i;
    for(i=0;i<len;++i) {
        xmlChar c = input[i];
        switch(c) {
            case '<':
                PUTLITERAL("&lt;");
                break;
            case '>':
                PUTLITERAL("&gt;");
                break;
            case '&':
                PUTLITERAL("&amp;");
                break;
            default:
                PUTC(c);
        };
    }
}


#define WRITE(s,len) mywrite(s,len)
#define PUTS(s) WRITE(s,strlen(s))

#define PUTELEMENT(doc,e) xmlNodeDumpOutput(xout,doc,e,2,1,"utf-8")

static void processText(xmlDoc* doc, xmlChar* text, int lastisElement, int nextisElement) {
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
                PUTLITERAL("\n");
                lastisElement = 0;
            }
            if(end==start) {
                if(end[1]=='\0')
                    PUTLITERAL("\n");
            } else if(start<end) {
                if(!inParagraph) {
                    inParagraph = 1;
                    PUTLITERAL("<p>");
                }
                first = 0;
                WRITE(start,end-start);
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
            PUTELEMENT(cur->doc,cur);
            lastisElement = 1;
            continue;
        case XML_CDATA_SECTION_NODE:
            PUTLITERAL("<![CDATA[");
            PUTPLAIN(cur->content);
            PUTLITERAL("]]>");
            continue;
        case XML_TEXT_NODE:
            processText(cur->doc,
                    cur->content,
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
            xmlOutputBufferWrite(xout,amt,buf);
        }
        fclose(inp);
    } else {
        // XXX: this is a horrible hack how do
        if(strchr(maybeFile,'\n') == NULL && strchr(maybeFile,'<')==NULL) return;
        // the file doesn't exist, so it must be a text blurb
        xmlOutputBufferWrite(xout,strlen(maybeFile),maybeFile);
    }
}

int main(void) {

    LIBXML_TEST_VERSION;

    xout = xmlOutputBufferCreateFile(stdout,xmlGetCharEncodingHandler(XML_CHAR_ENCODING_UTF8));

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
    xmlOutputBufferFlush(xout);
    return 0;
}
