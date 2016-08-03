#include "input.h"

#define LIBXML2_NEW_BUFFER
#define _GNU_SOURCE

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/HTMLtree.h>
#include <libxml/HTMLparser.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <ctype.h> //isspace
#include <unistd.h> //read, close

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

/* Should be able to perform this whitespace processing inside other elements,
   maybe if they're given a special attribute to indicate it */

struct ishctx {
    xmlNode* e;
    xmlNs* ns;
    bool inParagraph;
    bool endedNewline;
};

bool debugging = false;

static xmlNode* copyToNew(xmlNode* old, xmlNode* parent) {
  xmlNode* new = xmlDocCopyNode(old,parent->doc,1);
  xmlAddChild(parent,new);
  return new;
}

static xmlNode* moveToNew(xmlNode* old, xmlNode* parent) {
  xmlNode* new = copyToNew(old,parent);
  xmlUnlinkNode(old);
  return new;
}
static void moveToNewDerp(xmlNode* old, void* ctx) {
  xmlNode* head = (xmlNode*) ctx;
  moveToNew(old,head);
}

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
        newthingy(ctx,xmlNewNode(ctx->ns,"p"));
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
    xmlNode* new = xmlNewNode(parent->ns,name);
    xmlAddChild(parent,new);
    return findOrCreate(new,next);
}

static xmlNode* getContent(xmlNode* oroot, bool createBody) {
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
    } else if(false == createBody) {
      content = oroot;
    } else {
      xmlNode* body = findOrCreate(oroot,"body");
      assert(body != NULL);
      content = body->children;
    }
  }
  return content;
}

static void processRoot(struct ishctx* ctx, xmlNode* root);

static bool maybeHish(xmlNode* e, struct ishctx* ctx) {
  if(xmlHasProp(e,"hish")) {
    fprintf(stderr,"Hish weeeeee %s %s\n",e->name,e->properties->name);
    xmlUnlinkNode(e);

    xmlUnsetProp(e,"hish");
    xmlNode* dangling = xmlNewNode(ctx->ns,"derp");
    /* process the contents of this node like the root one */
    struct ishctx subctx = {
      .endedNewline = false,
      .e = dangling,
      .ns = ctx->ns,
      .inParagraph = false,
    };
    processRoot(&subctx,e);
    xmlNode* ne = moveToNew(e,ctx->e);
    ne->children = dangling->next;
    xmlUnlinkNode(dangling);
    return true;
  }
  return false;
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
                  if(debugging) {
                    fprintf(stderr,"uhhh %d\n",end-start);
                  }
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
                if(maybeHish(e,ctx)) {
                  e = next;
                  continue;
                }
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

        default:
            newthingy(ctx,xmlDocCopyNode(e,ctx->e->doc,1));
        };
        e = next;
    }
}

void libxml2SUCKS(xmlNode* cur) {
    /* libxml2 is stupid about namespaces.
     * When you copy a node from one document to another, it does not adjust the namespaces to accomodate. That results in a document along the lines of
     * <html xmlns="somecrappylongurlthatispointless">...<i xmlns="theothernamespaceintheolddocument">italic</i>...</html>
     *
     * The xmlns attribute is moronic in of itself, but it is necessary to just blindly leak any
     * namespaces in the old document (since they can't be freed without guaranteeing double frees)
     * so it doesn't pollute the new document with xmlns all over the place.
     *
     * That's not all. libxml2 is moronic about namespaces in that it does not consult a lookup table
     * to produce a namespace by URL. So despite URLs being unique identifiers for namespaces, it
     * ignores that and just creates namespaces as they're needed to be created during parsing.
     *
     * That means you end up with (and I kid you not) XML like this:
     * <html xmlns="http://www.w3.org/1999/xhtml"> ... <i xmlns="http://www.w3.org/1999/xhtml">italic</i> ...</html>
     *
     * That's right. libxml2 will set the xmlns attribute for child nodes in the SAME NAMESPACE AS
     * THE PARENT. Because libxml2 doesn't use a lookup table, and there's no way to tell the parser
     * which namespace object to use, when you parse one document in a namespace, then parse another
     * document in the exact same namespace, they will have DIFFERENT namespace objects, with the same URL.
     * So libxml2 sees a child node with a different namespace object in memory and assumes the URL
     * must be different (despite denying the capability to reuse namespace objects) and dutifully
     * sets the xmlns attribute... to the same namespace as it's already in.
     *
     * Only solutions may be #1: leak all namespace objects in the old document by setting them NULL,
     * so it doesn't ever set xmlns attributes for any child elements copied over to the new document.
     * or #2: leak all namespace objects in the old document, by setting them to the namespace of
     * the new one, but only if the old namespace has the same URL as the new one.
     *
     * In both cases libxml2 will create a hard coded namespace object for both documents setting it
     * recursively and this may not be avoided or overridden, so you just have to descend through
     * the document tree a SECOND time, setting all the namespaces for each element to NULL or whatever.
     *
     * And what the hell is nsDef? Off with its head!
     */
    if(cur==NULL) return;
    cur->ns = NULL;
    cur->nsDef = NULL;
    libxml2SUCKS(cur->children);
    libxml2SUCKS(cur->next);
}

#define HEADER "<!DOCTYPE html>\n" \
    "<html><body>"

#define FOOTER "</body></html>"

#define BUFSIZE 0x1000
xmlDoc* readFunky(int fd, const char* content) {
    htmlParserCtxtPtr ctx;
    char buf[BUFSIZE];
    ctx = htmlCreatePushParserCtxt(NULL, NULL,
                                   "",0,"htmlish.xml",
								   XML_CHAR_ENCODING_UTF8);
    assert(ctx);
    htmlCtxtUseOptions(ctx,
					   HTML_PARSE_NONET |
					   HTML_PARSE_COMPACT |
					   HTML_PARSE_RECOVER
					   );
    htmlParseChunk(ctx, HEADER, sizeof(HEADER)-1, 0);
    if(fd<0) {
        htmlParseChunk(ctx,content,strlen(content),0);
    } else {
        for(;;) {
            ssize_t amt = read(fd,buf,BUFSIZE);
            if(amt<=0) break;
            htmlParseChunk(ctx, buf, amt, 0);
        }
    }

    htmlParseChunk(ctx,FOOTER,sizeof(FOOTER)-1, 1);
    xmlDoc* doc = ctx->myDoc;
    if(!ctx->wellFormed) {
        fprintf(stderr,"Warning: not well formed.\n");
    }
    xmlFreeParserCtxt(ctx);
    libxml2SUCKS(doc->children);
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

struct dostylederp {
  xmlNode* outhead;
  bool hasContents;
  bool needFree;
  bool found;
};

void createStyle(xmlNode* head, const char* url) {
  xmlNode* style = xmlNewNode(head->ns,"link");
  xmlSetProp(style,"href",url);
  xmlSetProp(style,"rel","stylesheet");
  xmlSetProp(style,"type","text/css");
  xmlAddChild(head,style);
}

/* Remove a <stylesheet/> fake node from the source document
 * using it to add a <link rel="stylesheet" etc blah/> to the
 * head of the output document.
 * <stylesheet>URL</stylesheet> with only 1 text child
 * TODO: strip whitespace on either side of the URL
 */

static void removeStylesheets(xmlNode* target, void* ctx) {
  xmlNode* head = (xmlNode*) ctx;
  createStyle(head,target->children[0].content);
  xmlUnlinkNode(target);
}

/* add styles from the environment, from <stylesheet/> tags and from
 * <link rel="stylesheet".../> tags, to the <head> of the output document.
 * TODO: coalesce <style/> tags into an external stylesheet
 */

void doStyle(xmlNode* root, xmlNode* head) {
  const char* envstyle = getenv("style");
  if(envstyle) {
    createStyle(head,envstyle);
  }

  foreachNode(root,"stylesheet",removeStylesheets,head);
  foreachNode(root,"link",moveToNewDerp,head);
}

static void doIntitle2(xmlNode* target, void* ctx) {
    xmlReplaceNode(target,xmlNewText(ctx));
}

void doIntitle(xmlNode* oroot, const char* title) {
    foreachNode(oroot,"intitle",doIntitle2,(void*)title);
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

void doTitle(xmlNode* oroot, xmlNode* root, xmlNode* head) {
    const char* contents = getenv("title");

    struct titleseeker ts;
    ts.title = NULL;
    assert(ts.title == NULL);

    // this allows setting the title inline anywhere via a <title> tag.
    foreachNode(root,"title",eliminateTitles,(void*)&ts);

    assert(contents || ts.title);

    xmlNode* title = findOrCreate(head,"title");
    xmlAddChild(title,xmlNewText(ts.title ? ts.title : contents));

    // this is sneaky, modifies the <intitle/> element discovered with doTitle
    doIntitle(oroot, ts.title ? ts.title : contents);
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

static void doMetas(xmlNode* root, xmlNode* head) {
    foreachNode(root,"meta",moveToNewDerp,head);
}

const char defaultTemplate[] =
  "<!DOCTYPE html>\n"
  "<html>\n"
  "<head><meta charset=\"utf-8\"/>\n"
  "<title/><header/></head>\n"
  "<body><h1><intitle/></h1>\n"
  "<top/><content/><footer/>\n"
  "</body></html>";

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

    xmlDoc* doc = readFunky(0,"<main htmlish markup>");
    assert(doc);
    xmlNode* oroot = xmlDocGetRootElement(output);
    struct ishctx ctx = {
        .endedNewline = false,
        .e = getContent(oroot,false)
    };
    xmlNode* root = xmlDocGetRootElement(doc);
	assert(root);
	// html/body
	root = root->children;
    assert(root);
    ctx.ns = oroot->ns;

    /* This sets up the template, filling in placeholder elements
       with the stuff that the environment carries */
    doByFile(output,"header");
    doByFile(output,"top");
    doByFile(output,"footer");
    xmlNode *ohead = fuckXPath(oroot,"head");
    if(ohead == NULL) {
        ohead = xmlNewNode(ctx.ns,"head");
        if(root->children) {
            xmlAddPrevSibling(oroot->children,ohead);
        } else {
            xmlAddChild(oroot,ohead);
        }
    }
    /* these remove junk from the source document, putting it in the proper
       places in the output document.
    */
    doTitle(oroot,root,ohead);
    doMetas(root,ohead);
    doStyle(root,ohead);

    /* all stuff removed, process the whitespace! */
    processRoot(&ctx,root);

    htmlSaveFileFormat("-",output,"utf-8",1);
    return 0;
}
