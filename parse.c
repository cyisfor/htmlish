#define LIBXML2_NEW_BUFFER
#define _GNU_SOURCE

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
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

int inParagraph = 0;
xmlNode* cur = NULL;

static void maybeStartParagraph(void) {
    if(!inParagraph) {
        inParagraph = 1;
        xmlNode* next = xmlNewNode(NULL,"p");
        xmlAddNextSibling(cur,next);
        cur = next;
    }
}

static void maybeEndParagraph(void) {
    if(inParagraph) {
        xmlNode* next = xmlNewText("");
        xmlAddNextSibling(cur,next);
        cur = next;
        inParagraph = 0;
    }
}

static void processText(xmlChar* text, int lastisElement, int nextisElement) {
    xmlChar* start = text;
    xmlChar* end = start;
    int first = 1;
    while(end) {
        end = strchr(start,'\n');
        if(end==NULL) {
            if(strlen(start)>0) {
                maybeStartParagraph();
                first = 0;
                xmlNodeAddContent(cur,start);
                if(!nextisElement)
                    maybeEndParagraph();
            }
        } else {
            if(lastisElement) {
                lastisElement = 0;
            }
            if(start<end) {
                maybeStartParagraph();
                first = 0;
                xmlNodeAddContentLen(cur,start,end-start);
                maybeEndParagraph();
            }
            start = end+1;
            end = start;
        }
    }
}

static void processRoot(xmlNode* root) {
    xmlNode* e;
    int lastisElement = 0;
    for(e=root->children;e;e = e->next) {
        switch(e->type) {
        case XML_TEXT_NODE:
            processText(
                    e->content,
                        lastisElement,
                        e->next && e->next->type != XML_TEXT_NODE );
            lastisElement = 0;
            continue;
        case XML_ELEMENT_NODE:
            lastisElement = 1;
        default:
            xmlAddNextSibling(cur,e);
            cur = e;
        };
    }
}

#define HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<root>"
#define FOOTER "</root>"

#define BUFSIZE 0x1000
xmlDoc* readFunky(int fd) {
    xmlParserCtxtPtr ctx;
    char buf[BUFSIZE];
    ctx = xmlCreatePushParserCtxt(NULL, NULL,
                                   "",0,"htmlish.xml");
    assert(ctx);

    xmlParseChunk(ctx, HEADER, sizeof(HEADER)-1, 0);
    for(;;) {
        ssize_t amt = read(fd,buf,BUFSIZE);
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

static void parseEnvFile(const char* path, xmlNode* last) {
    int inp = open(path,O_RDONLY);
    assert(inp >= 0);
    xmlDoc* doc = readFunky(inp);
    close(inp);
    if(!doc) {
        fprintf(stderr,"Couldn't parse %s",path);            
        exit(5);
    }
    xmlNode* root = xmlDocGetRootElement(doc);
    xmlNode* cur = root;
    while(cur) {
        xmlNode* next = cur->next;
        xmlAddNextSibling(last,cur);
        // add siblings after the element passed to the function, and then after the sibling added.
        last = cur;
        cur = next;
    }
    xmlFreeDoc(doc);
}

xmlNode* fuckXPath(xmlNode* parent, const char* name) {
    if(strcmp(parent->name,name)==0)
        return parent;
    xmlNode* cur = parent->children;
    for(;cur;cur=cur->next) {
        xmlNode* ret = fuckXPath(cur,name);
        if(ret)
            return ret;
    }
    return NULL;
}

void foreachNode(xmlNode* parent, const char* name, void (*handle)(xmlNode*,void*), void* ctx) {
    if(strcmp(parent->name,name)==0)
        handle(parent,ctx);
    xmlNode* cur = parent->children;
    for(;cur;cur=cur->next) {
        foreachNode(cur,name,handle,ctx);
    }
}

xmlNode* findOrCreate(xmlNode* parent, const char* path) {
    if(*path == 0)
        return parent;

    const char* next = strchrnul(path,'/');
    xmlNode* cur = parent->children;
    for(;cur;cur = cur->next) {
        if(0==memcmp(cur->name,path,next-path)) {
            return findOrCreate(cur,next);
        }
    }

    static char name[0x100];
    memcpy(name,path,next-path);
    name[next-path] = 0;
    xmlNode* new = xmlNewNode(NULL,name);
    xmlAddChild(parent,new);
    return findOrCreate(new,next);
}

struct dbfderp {
    const char* name;
    const char* path;
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
        parseEnvFile(derp->path,target);
        xmlUnlinkNode(target);
        xmlFreeNode(target);
    }
}

static void doByFile(xmlDoc* output, const char* name) {
    const char* path = getenv(name);
    struct dbfderp derp = { name, path };
    foreachNode(xmlDocGetRootElement(output),name,doByFile2,&derp);
}

struct dostylederp {
    bool hasContents;
    bool found;
    xmlNode* style;
};

void doStyle2(xmlNode* target, void* ctx) {
    struct dostylederp* derp = (struct dostylederp*)ctx;
    derp->found = true;

    if(derp->hasContents) {
        xmlReplaceNode(target,derp->style);
    } else {
        xmlUnlinkNode(target);
        xmlFreeNode(target);
        return;
    }
}

void doStyle(xmlDoc* output) {
    const char* contents = getenv("style");
    xmlNode* root = xmlDocGetRootElement(output);
    struct dostylederp derp = { contents != NULL, false };
    if(derp.hasContents)
        derp.style = xmlNewNode(NULL,"link");

    foreachNode(root,"style",doStyle2,&derp);
    if(!derp.found) { 
        fprintf(stderr,"No styles found\n");
        if(!contents) return;
        xmlNode* head = findOrCreate(xmlDocGetRootElement(output),"head");
        assert(head);
        xmlAddChild(head,derp.style);
    }

    xmlSetProp(derp.style,"href",contents);
    xmlSetProp(derp.style,"rel","stylesheet");
    xmlSetProp(derp.style,"type","text/css");
}

static void doIntitle2(xmlNode* target, void* ctx) {
    xmlReplaceNode(target,ctx);
}

void doIntitle(xmlDoc* output, const char* title) {
    xmlNode* root = xmlDocGetRootElement(output);
    foreachNode(root,"intitle",doIntitle2,xmlNewText(title));
}
    
void doTitle(xmlDoc* output) {
    const char* contents = getenv("title");
    xmlNode* root = xmlDocGetRootElement(output);

    foreachNode(root,"title",(void*)xmlAddChild,xmlNewText(contents));
    doIntitle(output,contents);
}

const char defaultTemplate[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
    "<head><title/><style/><header/></head>\n"
    "<body><h1><intitle/></h1><top/><content/><footer/></body></html>";

int main(void) {

    if(getenv("printtemplate")) {
        puts(defaultTemplate);
        return 0;
    }
    LIBXML_TEST_VERSION;

    xmlDoc* doc = readFunky(0);
    assert(doc);
    xmlNode* root = xmlDocGetRootElement(doc);
    assert(root);

    xmlDoc* output;
    if(getenv("template")) {
        output = xmlParseFile(getenv("template"));
        if(!output) {
            fprintf(stderr,"Error: template failed... not sure if well formed or not.\n");
            exit(2);
        }
    } else {
        output = xmlParseMemory(defaultTemplate,sizeof(defaultTemplate));
    }

    doByFile(output,"header");
    doByFile(output,"top");
    doByFile(output,"footer");
    doStyle(output);
    doTitle(output);

    xmlNode* content = fuckXPath(xmlDocGetRootElement(output),"content");
    if(content) {
        xmlNode* text = content->prev;
        if(text) {
            xmlUnlinkNode(content);
            xmlFreeNode(content);
            content = text;
        } else {
            text = xmlNewText("");
            xmlReplaceNode(content,text);
            xmlFreeNode(content);
            content = text;
        }
    } else {
        xmlNode* body = findOrCreate(xmlDocGetRootElement(output),"body");
        assert(body != NULL);
        content = body->children;
    }

    cur = content;
    processRoot(root);

    xmlSaveFile("-",output);
    return 0;
}
