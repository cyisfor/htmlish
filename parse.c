#include "input.h"

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
#include <signal.h>

/* if a line is blank, a paragraph follows, or a block element.
 * so, after processText, if it ends in \n, set a flag to check the next element.
 * when the next element is a text node, or like <u> or an <i>, start a paragraph, 
 * then add it to the paragraph.
 * when it's a <blockquote> or an <ul> add it without starting a paragraph.
 *
 * If a line is blank, a paragraph ends. So if the text node ends in \n\n, end the paragraph before
 * doing the above. Any \n in the text node respectively ends, and starts a paragraph.
 */

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

struct ishctx {
    xmlNode* e;
    bool inParagraph;
    bool endedNewline;
};

xmlNode* currentAdderThingy = NULL;

bool debugging = false;

static void newthingy(struct ishctx* ctx, xmlNode* thingy) {
    if(ctx->inParagraph) {
        xmlAddChild(ctx->e,thingy);
    } else {
        assert(xmlAddNextSibling(ctx->e,thingy));
        ctx->e = thingy;
    }
}

static void maybeStartParagraph(struct ishctx* ctx, const char* where) {
    static char buf[0x200];
    if(!ctx->inParagraph) {
        if(debugging) {
            snprintf(buf,sizeof(buf),"start %s",where);
            newthingy(ctx,xmlNewComment(buf));
        }
        xmlNodeAddContentLen(ctx->e,"\n",1);
        newthingy(ctx,xmlNewNode(NULL,"p"));
        //xmlSetProp(ctx->e,"where",where);
        xmlNodeAddContentLen(ctx->e,"\n",1);
        ctx->inParagraph = true;
    }
}

static void maybeEndParagraph(struct ishctx* ctx, const char* where) {
    static char buf[0x200];
    if(debugging) {
        snprintf(buf,sizeof(buf),"end %s",where);
        newthingy(ctx,xmlNewComment(buf));
    }
    if(ctx->inParagraph) {
        //xmlAddChild(ctx->e,xmlNewComment(where));
        ctx->inParagraph = false;
    }
}

static void processText(struct ishctx* ctx, xmlChar* text) {
    if(!*text) return;

    xmlChar* start = text;
    xmlChar* end = start;
    bool first = true;
    if(*start == '\n') {
        // starts blank, so be sure to start a new paragraph.
        maybeEndParagraph(ctx,"start");
    }

    for(;;) {
        end = strchrnul(start,'\n');
        if(*end == 0) {
            if(*start != 0) {                 
                // only start a paragraph once we're sure we got something to put in it.
                xmlChar* c;
                for(c=start;c!=end;++c) {
                    if(!isspace(*c)) {
                        maybeStartParagraph(ctx,"beginning");
                        // no newlines between start and nul. Just leave it in the current paragraph!
                        xmlNodeAddContentLen(ctx->e,start,end-start);
                        break;
                    }
                }
            }
            return;
        } 
            
        if(start!=end) {
            // only start a paragraph once we're sure we got something to put in it.
            xmlChar* c;
            for(c=start;c!=end;++c) {
                if(!isspace(*c)) {
                    //only add a paragraph if it isn't ALL spaces
                    maybeStartParagraph(ctx,"middle");
                    first = false;
                    xmlNodeAddContentLen(ctx->e,start,end-start);
                    maybeEndParagraph(ctx,"middle");
                    break;
                }
            }
            // there's more to the text node we know, because end is at \n not \0
        }

        if(*(end + 1) == '\0') {
            ctx->endedNewline = true;
            maybeEndParagraph(ctx,"final");
            // always end a paragraph on a newline ending, but not if there's no newline
            // since "blah <i>blah</i> blah" shouldn't be two paragraphs.
            // save the ended newline state to check the next e
            return;
        }
        start = end+1;
    }
}

#define LITCMP(a,l) strncmp(a,l,sizeof(l))

static void processRoot(struct ishctx* ctx, xmlNode* root) {
    xmlNode* e = root->children;
    while(e) {
        xmlNode* next = e->next;
        switch(e->type) {
        case XML_ENTITY_NODE:
        case XML_ENTITY_REF_NODE:
            fprintf(stderr,"Entity boo %s %s\n",e->name,e->content);
            exit(23);
            break;
        case XML_TEXT_NODE:
            // if this is a text node, it doesn't matter whether the previous text node
            // ended in a newline or not. The important thing is to know whether THIS
            // text node ended in a newline or not.
            ctx->endedNewline = false;
            processText(
                    ctx,
                    e->content);
            break;
        case XML_ELEMENT_NODE:
            { 
                bool fakeElement = 
                0 == LITCMP(e->name,"title") ||
                    0 == LITCMP(e->name,"meta");
                if(!fakeElement) {
                    { bool blockElement = 
                        0 == LITCMP(e->name,"ul") ||
                            0 == LITCMP(e->name,"ol") ||
                            0 == LITCMP(e->name,"p") || // inception
                            0 == LITCMP(e->name,"div") ||
                            0 == LITCMP(e->name,"table") ||
                            0 == LITCMP(e->name,"blockquote");
                        if(blockElement) {
                            // no need to start (or have) a paragraph. This element is huge.
                            maybeEndParagraph(ctx,"block"); //XXX: let block elements stay inside a paragraph if on same line?
                        } else {
                            //start a paragraph if this element is a wimp
                            //but only if the last text node ended on a newline.
                            //otherwise the last text node and this should be in the same
                            //paragraph
                            if(ctx->endedNewline) { 
                                static char buf[0x100] = "";
                                if(debugging) 
                                    snprintf(buf,0x100,"wimp tag {{%s}}",e->name);
                                maybeEndParagraph(ctx,buf);
                                // make sure this wimp is in the paragraph, not before it.
                                maybeStartParagraph(ctx,buf);
                                ctx->endedNewline = false;
                            }
                        }
                    }
                }
            }
        default:
            newthingy(ctx,xmlDocCopyNode(e,ctx->e->doc,1));
        };
        e = next;
    }
}

static void fixDTD(xmlDoc* doc) {
    if(!doc->extSubset) {
        fputs("bokr\n",stderr);
        xmlNodePtr dtd = (xmlNodePtr) xmlNewDtd(doc, "html", "-//W3C//DTD XHTML 1.0 Strict//EN",
                "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd");
#if FIREFOX_DOES_NOT_SUCK
        if(doc->children) {
            xmlAddPrevSibling(doc->children,(xmlNodePtr)dtd);
            // the above eats the dtd
        } else {
            xmlAddChild((xmlNodePtr)doc,(xmlNodePtr)dtd);
            // the above will eat the dtd once you add another child
        }
        xmlNewDtd(doc, "html", "-//W3C//DTD XHTML 1.1//EN", "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd");
#endif /* FIREFOX_DOES_NOT_SUCK */
    }
}

#define HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n" \
    "<html>"

#define FOOTER "</html>"

#define BUFSIZE 0x1000
xmlDoc* readFunky(int fd, const char* content) {
    xmlParserCtxtPtr ctx;
    char buf[BUFSIZE];
    ctx = xmlCreatePushParserCtxt(NULL, NULL,
                                   "",0,"htmlish.xml");
    xmlCtxtUseOptions(ctx,XML_PARSE_NOENT|XML_PARSE_DTDVALID);
    assert(ctx);
    getEntitySAXFunc oldGetEntity = ctx->sax->getEntity;

    xmlEntityPtr newGetEntity(void* ectx, const xmlChar* name) {
        xmlEntityPtr entity = oldGetEntity(ectx,name);
        if(entity) return entity;
        entity = xmlGetDtdEntity(ctx->myDoc, name);
        if(entity) return entity;
        fixDTD(ctx->myDoc);
        entity = xmlGetDtdEntity(ctx->myDoc, name);
        if(entity) return entity;
                    
        fprintf(stderr,"Warning: jury rigging entity %s\n",name);
        xmlAddDtdEntity(ctx->myDoc,
                name,
                XML_INTERNAL_GENERAL_ENTITY,
                "-//W3C//DTD XHTML 1.1//EN", 
                "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd",
                name);
        return xmlGetDtdEntity(ctx->myDoc, name);
    }
    ctx->sax->getEntity = newGetEntity;

    xmlParseChunk(ctx, HEADER, sizeof(HEADER)-1, 0);
    if(fd<0) {
        xmlParseChunk(ctx,content,strlen(content),0);
    } else {
        for(;;) {
            ssize_t amt = read(fd,buf,BUFSIZE);
            if(amt<=0) break;
            xmlParseChunk(ctx, buf, amt, 0);
        }
    }

    xmlParseChunk(ctx,FOOTER,sizeof(FOOTER)-1, 1);
    xmlDoc* doc = ctx->myDoc;
    if(!ctx->wellFormed) {
        fprintf(stderr,"Warning: not well formed.\n");
    }
    xmlFreeParserCtxt(ctx);
    return doc;
}
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
    xmlNode* cur = root;
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

xmlNode* fuckXPathDivId(xmlNode* parent, const char* id) {
    if(strcmp(parent->name,"div")==0) {
        const char* test = xmlGetProp(parent,"id");
        if(test && strcmp(test,id) == 0)
            return parent;
    }
    xmlNode* cur = parent->children;
    for(;cur;cur=cur->next) {
        xmlNode* ret = fuckXPathDivId(cur,id);
        if(ret)
            return ret;
    }
    return NULL;
}
void foreachNode(xmlNode* parent, const char* name, void (*handle)(xmlNode*,void*), void* ctx) {
    if(!parent->name) {
        assert(!parent->children);
        // we don't process text
        return;
    }
    if(strcmp(parent->name,name)==0)
        handle(parent,ctx);
    xmlNode* cur = parent->children;
    while(cur) {
        xmlNode* next = cur->next;
        foreachNode(cur,name,handle,ctx);
        cur = next;
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

static void doByFile(xmlDoc* output, const char* name) {
    const char* path = getenv(name);
    struct dbfderp derp = { name, path };
    parseEnvFile(derp.path,&derp.replacement);
    foreachNode(xmlDocGetRootElement(output),name,doByFile2,&derp);
}

struct dostylederp {
    const char* url;
    bool hasContents;
    bool needFree;
    bool found;
};

xmlNode* createStyle(struct dostylederp* derp) {
    xmlNode* style = xmlNewNode(NULL,"link");
    xmlSetProp(style,"href",derp->url);
    xmlSetProp(style,"rel","stylesheet");
    xmlSetProp(style,"type","text/css");
    return style;
}

static void findStyle(xmlNode* target, void* ctx) {
    struct dostylederp* derp = (struct dostylederp*)ctx;
    if(!derp->hasContents) {
        derp->hasContents = true;
        derp->url = strdup(target->children[0].content);
        derp->needFree = true;
    }
    xmlUnlinkNode(target);
}

void doStyle2(xmlNode* target, void* ctx) {
    struct dostylederp* derp = (struct dostylederp*)ctx;
    derp->found = true;

    if(derp->hasContents) {
        xmlReplaceNode(target,createStyle(derp));
    } else {
        xmlUnlinkNode(target);
    }
    // xmlFreeNode(target);
}

void doStyle(xmlDoc* output,xmlNode* root, xmlNode* head) {
    const char* contents = getenv("style");
    struct dostylederp derp = { contents, contents != NULL, false, false };
    if(!derp.hasContents) {
        foreachNode(root,"stylesheet",findStyle,&derp);
        // if(!derp.hasContents) whocares();
    }

    foreachNode(root,"style",doStyle2,&derp);

    if(derp.hasContents && !derp.found) {
        // we have a style sheet, but no place in the template to put the link.

        xmlAddChild(head,createStyle(&derp));
    }
    if(derp.needFree) 
        free((char*)derp.url);
}

static void doIntitle2(xmlNode* target, void* ctx) {
    xmlReplaceNode(target,xmlNewText(ctx));
}

void doIntitle(xmlDoc* output, xmlNode* root, const char* title) {
    foreachNode(root,"intitle",doIntitle2,(void*)title);
}

struct titleseeker {
    char* title;
};
    
static void eliminateTitles(xmlNode* target, void* ctx) {
    struct titleseeker* ts = (struct titleseeker*) ctx;
    if(ts->title == NULL && target->children) {
        const char* title = target->children[0].content;
        ts->title = strdup(title);
    }
    xmlUnlinkNode(target);
    //xmlFreeNode(target);
}

void doTitle(xmlDoc* output, xmlNode* root, xmlNode* head) {
    const char* contents = getenv("title");

    struct titleseeker ts;
    ts.title = NULL;
    assert(ts.title == NULL);

    // this allows setting the title inline anywhere via a <title> tag.
    foreachNode(root,"title",eliminateTitles,(void*)&ts);

    assert(contents || ts.title);

    xmlNode* title = findOrCreate(head,"title");
    xmlAddChild(title,xmlNewText(ts.title ? ts.title : contents));

    doIntitle(output, root, ts.title ? ts.title : contents);
    free(ts.title);
}


struct metaseeker {
    xmlNode** meta;
    int nmeta;
};

static void extractMetas(xmlNode* target, void* ctx) {
    struct metaseeker* ms = (struct metaseeker*) ctx;
    xmlUnlinkNode(target);
    ms->meta = realloc(ms->meta,ms->nmeta+1);
    ms->meta[ms->nmeta] = target;
    ++ms->nmeta;
}

void doMetas(xmlDoc* output, xmlNode* root, xmlNode* head) {
    struct metaseeker ms = { NULL, 0 };
    foreachNode(root,"meta",extractMetas,(void*)&ms);
    int i;
    for(i=0;i<ms.nmeta;++i) {
        xmlSetTreeDoc(ms.meta[i],output);
        xmlAddChild(head,ms.meta[i]);
    }
    free(ms.meta);
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

    setupInput();

    xmlDoc* doc = readFunky(0,"<main htmlish markup>");
    assert(doc);

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
    fixDTD(output);
    xmlNode* oroot = xmlDocGetRootElement(output);
    xmlNode* content = fuckXPath(oroot,"content");
    if(content) {
        xmlNode* text = xmlNewText("");
        xmlReplaceNode(content,text);
        xmlFreeNode(content);
        content = text;
    } else {
        content = fuckXPathDivId(oroot,"content");

        if(content) {
            if(content->children == NULL) {
                xmlNode* text = xmlNewTextLen("",0);
                assert(text);
                xmlAddChild(content,text);                
            }
            content = content->children;
            assert(content);
        } else {
            xmlNode* body = findOrCreate(oroot,"body");
            assert(body != NULL);
            content = body->children;
        }
    }
    struct ishctx ctx = {
        .endedNewline = false,
        .e = content
    };
    xmlNode* root = xmlDocGetRootElement(doc);
    assert(root);
    processRoot(&ctx,root);

    doByFile(output,"header");
    doByFile(output,"top");
    doByFile(output,"footer");
    xmlNode *ohead = fuckXPath(oroot,"head");
    if(ohead == NULL) {
        ohead = xmlNewNode(NULL,"head");
        if(root->children) {
            xmlAddPrevSibling(oroot->children,ohead);
        } else {
            xmlAddChild(oroot,ohead);
        }
    }
    doTitle(output,oroot,ohead);
    doMetas(output,oroot,ohead);
    doStyle(output,oroot,ohead);

    xmlSaveFormatFile("-",output,1);
    return 0;
}
