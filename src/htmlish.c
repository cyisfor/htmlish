#define LIBXML2_NEW_BUFFER
#define _GNU_SOURCE

#include "input.h"
#include "parse_chat.h"

#include "libxmlfixes/wanted_tags.gen.h"
#include "libxmlfixes.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
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
	bool first;
};

bool debugging = false;

static xmlNode* copyToNew(xmlNode* old, xmlNode* base, bool sibling) {
  xmlNode* new = xmlDocCopyNode(old,base->doc,1);
  if(sibling) {
		xmlAddNextSibling(base,new);
	} else {
		xmlAddChild(base,new);
	}
  return new;
}

static xmlNode* moveToNew(xmlNode* old, xmlNode* base, bool sibling) {
  xmlNode* new = copyToNew(old,base,sibling);
  xmlUnlinkNode(old);
  return new;
}
static void moveToNewDerp(xmlNode* old, void* ctx) {
  xmlNode* head = (xmlNode*) ctx;
  moveToNew(old,head,false);
}

static void newthingy(struct ishctx* ctx, xmlNode* thingy) {
    if(ctx->first) {
			xmlAddChild(ctx->e,thingy);
			ctx->first = false;
			ctx->e = thingy;
			return;
		}
		if(ctx->inParagraph) {
			xmlAddChild(ctx->e,thingy);
			// but don't set ctx->e since still adding to it
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
        //xmlNodeAddContentLen(ctx->e,"\n",1);
        newthingy(ctx,xmlNewNode(ctx->ns,"p"));
        //xmlSetProp(ctx->e,"where",where);
        //xmlNodeAddContentLen(ctx->e,"\n",1);
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

static void processRoot(struct ishctx* ctx, xmlNode* root);

void subhish(xmlNode* e, struct ishctx* ctx) {
	//fprintf(stderr,"Hish weeeeee %s %s\n",e->name,e->properties->name);
	xmlUnlinkNode(e);

	xmlUnsetProp(e,"hish");
	xmlNode* dangling = xmlNewNode(ctx->ns,"derp");
	/* process the contents of this node like the root one */
	struct ishctx subctx = {
		.endedNewline = false,
		.e = dangling,
		.ns = ctx->ns,
		.inParagraph = false,
		// first is true, so everything ends up in dangling->children
		.first = true
	};
	processRoot(&subctx,e);
	xmlNode* base = ctx->e;
	// !ctx->inParagraph...?
	xmlNode* ne = moveToNew(e,base,false);
	newthingy(ctx,ne);

	ne->children = dangling->children;
	dangling->children = NULL;
	xmlUnlinkNode(dangling);
}

static bool maybeHish(xmlNode* e, struct ishctx* ctx) {
  if(xmlHasProp(e,"hish")) {
		subhish(e, ctx);
    return true;
  }
  return false;
}


static void processText(struct ishctx* ctx, xmlChar* text) {
    if(!*text) return;

    xmlChar* start = text;
    xmlChar* end = start;
    while(isspace(*end)) {
			if(*end == '\n') {
        // starts blank w/ newlines, so be sure to start a new paragraph.
				// end can be == start "\n\t" or end can be 1+ since the first's whitespace
				// " \n" or "\t\n" or etc
				if(end > start + 1) {
					xmlNodeAddContentLen(ctx->e, start, end-start-1);
				}
				start = end;
        maybeEndParagraph(ctx,"start");
			}
			++end;
		}
		if(end != start)  {
			xmlNodeAddContentLen(ctx->e, start, end-start);
			start = end;
		}
    bool first = true;
    for(;;) {
        end = strchrnul(start,'\n');
        if(*end == 0) {
            if(*start != 0) {
                // only start a paragraph once we're sure we got something to put in it.
                xmlChar* c;
                for(c=start;c!=end;++c) {
                    if(!isspace(*c)) {
											/* we found something that wasn't a space between this and
												 the last newline */
												ctx->endedNewline = false;
                        maybeStartParagraph(ctx,"beginning");
                        // no newlines between start and nul. Just leave it in the current paragraph!
												if(end != start)
													xmlNodeAddContentLen(ctx->e,start,end-start);
                        return;
                    }
                }
            }
						// only found spaces between the last newline and the end!
						ctx->endedNewline = true;
            return;
        } else {
					// until we check further, let's assume there's nothing but space beyond
					ctx->endedNewline = true;
				}

        if(start!=end) {
            // only start a paragraph once we're sure we got something to put in it.
            xmlChar* c;
            for(c=start;c!=end;++c) {
                if(!isspace(*c)) {
									// found something not a newline
									ctx->endedNewline = false;
									//only add a paragraph if it isn't ALL spaces
									maybeStartParagraph(ctx,"middle");
									first = false;
									xmlNodeAddContentLen(ctx->e,start,end-start);
									start = end;
									maybeEndParagraph(ctx,"middle");
									break;
                }
            }
            // there's more to the text node we know, because end is at \n not \0
        }

        if(*(end + 1) == '\0') {
						if(end != start) {
							xmlNodeAddContentLen(ctx->e, start, end-start);
							start = end;
						}
            maybeEndParagraph(ctx,"final");
            // always end a paragraph on a newline ending, but not if there's no newline
            // since "blah <i>blah</i> blah" shouldn't be two paragraphs.
            // save the ended newline state to check the next e
            ctx->endedNewline = true;
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
        case XML_ELEMENT_NODE: {
					enum wanted_tags tag = lookup_wanted(e->name);
					switch(tag) {
					case W_CHAT: // yay, coupling!
						fputs("chat found\n", stderr);
						maybeEndParagraph(ctx,"chat");
						/* all <chat> is hish, so that parse_chat can work on paragraphs, not
							 line-ish-things. No need to reinvent the line/tag/mixer/thingy.
						*/
						subhish(e, ctx);
						e = next;
						continue;
					case W_UL:
					case W_OL:
					case W_P:
					case W_DIV:
					case W_DD:
					case W_DL:
					case W_TABLE:
					case W_HR:
					case W_BLOCKQUOTE:
						// no need to start (or have) a paragraph. This element is huge.
						maybeEndParagraph(ctx,"block"); //XXX: let block elements stay inside a paragraph if on same line?
						if(maybeHish(e,ctx)) {
							e = next;
							continue;
						}
						break;
					default:
						//start a paragraph if this element is a wimp
						//but only if the last text node ended on a newline.
						//or at the beginning
						//otherwise the last text node and this should be in the same
						//paragraph
						if(ctx->first || ctx->endedNewline) {
							static char buf[0x100] = "";
							if(debugging)
								snprintf(buf,0x100,"wimp tag {{%s}}",e->name);
							maybeEndParagraph(ctx,buf);
							ctx->endedNewline = false;
						}
						// make sure this wimp is in a paragraph, not before it.
						maybeStartParagraph(ctx,"wimp");
					};
					// fall through
				}
        default:
            newthingy(ctx,xmlDocCopyNode(e,ctx->e->doc,1));
        };
        e = next;
    }
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
		const char* semi = strchr(envstyle,';');
		if(semi == NULL) {
			return createStyle(head,envstyle);
		}
		bool done = false;
		for(;;) {
			char buf[0x100];
			assert(semi - envstyle < 0x100);
			memcpy(buf,envstyle,semi-envstyle);
			buf[semi-envstyle] = '\0';
			createStyle(head,buf);
			envstyle = semi+1;
			if(done) break;
			semi = strchrnul(envstyle,';');
			if(*semi == '\0') done = true;
			// one more loop, with semi at the end.
		}
  }

  foreachNode(root,"stylesheet",removeStylesheets,head);
  foreachNode(root,"link",moveToNewDerp,head);
}

static void doIntitle2(xmlNode* target, void* ctx) {
	if(ctx) {
    xmlReplaceNode(target,xmlNewText(ctx));
	} else {
		xmlUnlinkNode(target);
		xmlFreeNode(target);
	}
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
    struct titleseeker ts;
    ts.title = NULL;
    assert(ts.title == NULL);

    // this allows setting the title inline anywhere via a <title> tag.
    foreachNode(root,"title",eliminateTitles,(void*)&ts);

		const char* title = ts.title == NULL ? getenv("title") : ts.title;

		if(title) {
			xmlNode* te = findOrCreate(head,"title");
			xmlChar* thead = getenv("titlehead");
			if(thead)
				xmlNodeAddContent(te,thead);
			xmlNodeAddContent(te,title);
		}

    // this is sneaky, modifies the <intitle/> element discovered with doTitle
    doIntitle(oroot, title);
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

void htmlish_doc(xmlNode* oroot, xmlNode* content, xmlDoc* doc, bool as_children);

void htmlish_str(xmlNode* content, const char* s, size_t l, bool as_children) {
	xmlNode* oroot = xmlDocGetRootElement(content->doc);
	
	xmlDoc* doc = strFunky(s,l);
	return htmlish_doc(oroot,content,doc,as_children);
}
#define LITLEN(a) a,sizeof(a)-1
void htmlish(xmlNode* content, int fd, bool as_children) {

	xmlNode* oroot = xmlDocGetRootElement(content->doc);
	
	xmlDoc* doc = readFunky(fd,LITLEN("bad file passed"));
	return htmlish_doc(oroot,content,doc,as_children);
}

void htmlish_doc(xmlNode* oroot, xmlNode* content, xmlDoc* doc, bool as_children) {
	struct ishctx ctx = {
		.endedNewline = false,
		.e = content,
		.first = as_children
	};

	xmlNode* root = xmlDocGetRootElement(doc);
	assert(root);
	// html/body
	root = root->children;
	assert(root);
	ctx.ns = oroot->ns;


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

	// parses the htmlish OUTPUT
	parse_chat(content,ohead); // <chat> tags
}


xmlNode* getContent(xmlNode* oroot, bool createBody, bool* as_child) {
  xmlNode* content = fuckXPath(oroot,"content");
  if(content) {
		*as_child = false;
		return content;
  }
	*as_child = true;
	content = fuckXPathDivId(oroot,"content");
	if(content) {
		return content;
	}

	if(false == createBody) {
		return oroot; // just use root I guess?
	}
	
	xmlNode* body = findOrCreate(oroot,"body");
	assert(body != NULL);
  return body;
}
