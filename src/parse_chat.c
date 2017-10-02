#define _GNU_SOURCE // memchr

#include "libxmlfixes/wanted_tags.h"
#include "libxmlfixes/libxmlfixes.h" // nextE

#include "parse_chat.h"

#include <stdlib.h> // qsort, bsearch, size_t
#include <stdio.h> // snprintf... should use atoi but base 16...
#include <ctype.h> // isspace
#include <string.h> // memchr
#include <stdbool.h>
#include <assert.h>

#define LITLEN(a) a,(sizeof(a)-1)

typedef size_t S; // easier to type
typedef unsigned short u16;

// thanks, djb
static
u16
makehash(xmlChar *str, S left)
{
    u16 hash = 5381;
    int c;

		if(left == 0) return hash;

    do {
			--left; // backwards derp
			c = str[left];
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		} while (left);

    return hash;
}

struct chatctx {
	u16* names;
	int nnames;
	xmlNode* dest;
	bool odd;
};

static
int compare_numps(const void* ap, const void* bp) {
	return
		*((u16*)ap)
		-
		*((u16*)bp);
}

u16 chat_intern(struct chatctx* ctx, xmlChar* name, S nlen) {
	u16 hash = makehash(name,nlen);
	if(ctx->nnames <= 10) {
		int i;
		for(i=0;i<ctx->nnames;++i) {
			if(hash == ctx->names[i]) {
				return i;
			}
		}
	} else {
		// productive to binary search
		u16* result = bsearch(&hash, ctx->names,
																		ctx->nnames, sizeof(*ctx->names),
																		compare_numps);
		if(result != NULL) {
			return *result;
		}
	}
	ctx->names = realloc(ctx->names,sizeof(*ctx->names)*(++ctx->nnames));
	ctx->names[ctx->nnames-1] = hash;
	if(ctx->nnames > 10) {
		// productive to sort it
		qsort(ctx->names, ctx->nnames, sizeof(*ctx->names), compare_numps);
		// now where did it go...
		unsigned long* result = bsearch(&hash, ctx->names,
																		ctx->nnames, sizeof(*ctx->names),
																		compare_numps);
		assert(result != NULL);
		return *result;
	}
	return ctx->nnames-1;
}

static
void add_line(struct chatctx* ctx, xmlChar* name, S nlen, xmlChar* val, S vlen) {
	xmlNode* row = xmlNewNode(ctx->dest->ns,"tr");
#if 0
	if(ctx->odd) {
		xmlSetProp(row,"class", "o");
		ctx->odd = false;
	} else {
		// no need for an even class
		ctx->odd = true;
	}
#endif
	
	u16 id = chat_intern(ctx, name,nlen); // may have collisions, but who cares

	char buf[0x100] = "n";
	// id should be low, since index into names, not the hash itself
	snprintf(buf+1,0x100-1,"%x",id);
	xmlSetProp(row,"class",buf);

	xmlNode* namecell = xmlNewNode(ctx->dest->ns,"th");

	xmlNodeAddContentLen(namecell,name,nlen);
	xmlNode* vcell = xmlNewNode(ctx->dest->ns,"td");
	xmlNodeAddContentLen(vcell,val,vlen);

	xmlAddChild(row,namecell);
	xmlAddChild(row,vcell);
	xmlAddChild(ctx->dest,row);
}

static
void take_line(struct chatctx* ctx, xmlChar* s, size_t n) {
	// empty line:
	if(n == 0) return;
	if(n == 1 && s[0] == ':') {
		// line with only colon?
		return;
	}
	int i;
	ssize_t colon = -1;
	for(i=0;i<n;++i) {
		if(!isspace(s[i])) {
			if(s[i] == ':') {
				colon = i;
			}
			break;
		}
	}
	if(i == n) return;
	int start = i; // eat space at beginning
	// find colon:
	if(colon == -1) {
		for(i=start;i<n;++i) {
			if(s[i] == ':') {
				colon = i;
				break;
			}
		}
		if(i==n) return;
		assert(colon != -1);
	}

	int endname = colon-1;
	while(isspace(s[endname])) {
		--endname;
		assert(endname > start);
	}
	int startval = colon+1;
	while(isspace(s[startval])) {
		if(startval + 1 == n) {
			// empty value?
			add_line(ctx, s, endname-start+1, NULL, 0);
			break;
		}
		++startval;
	}
	int endval = n;
	assert(startval != endval);
	while(isspace(s[endval])) {
		--endval;
		assert(endval > startval);
	}
	add_line(ctx, s, endname-start+1, s + startval, endval-startval+1);
}

/* XXX: since htmlish doesn't copy the old head into the new document,
	 we have to write to the new head,
	 yet otherwise we're modifying the input document in-place...
*/

static
void craft_style(struct chatctx* ctx, xmlNode* head) {
	// do NOT use ctx->dest because it's in the input document...
	short id;
	// XXX: xmlNewTextLen?
	xmlNode* text = xmlNewCDataBlock(head->doc, NULL, 0);
	xmlNodeAddContentLen(text, LITLEN("\n"));
	xmlNodeAddContentLen(text, LITLEN(".chat  { border-collapse: collapse; }\n"));
	xmlNodeAddContentLen(text, LITLEN(".chat th { padding-right: 1ex; text-align: right; }\n"));

	char idbuf[0x100];
	char huebuf[0x100];
	char buf[0x100];		
			


	for(id=0;id<ctx->nnames;++id) {

		int idlen = snprintf(idbuf,0x100,"%x",id);

		xmlNodeAddContentLen(text, LITLEN(".n"));
		xmlNodeAddContentLen(text, idbuf,idlen);
												 
		xmlNodeAddContentLen(text, LITLEN(" th {\n  color: hsl("));
		srandom(ctx->names[id]);

		float hue = random() * 360.0 / RAND_MAX;
		float sat = random() * 40.0 / RAND_MAX + 40.0; // higher saturation
		float light = 30.0 + random() * 20.0 / RAND_MAX; // relatively dark

		int huelen = snprintf(huebuf,0x100,"%f",hue);
		
		xmlNodeAddContentLen(text, huebuf,huelen);
												 
		xmlNodeAddContentLen(text, LITLEN(", "));
		xmlNodeAddContentLen(text, buf,
												 snprintf(buf,0x100,"%f",sat));
		xmlNodeAddContentLen(text, LITLEN("%, "));
		xmlNodeAddContentLen(text, buf,
												 snprintf(buf,0x100,"%f",light));
		xmlNodeAddContentLen(text, LITLEN("%);\n}\n"));

		// now the background

		xmlNodeAddContentLen(text, LITLEN(".n"));
		xmlNodeAddContentLen(text, idbuf,idlen);
		xmlNodeAddContentLen(text, LITLEN(" * {\n  background-color: hsl("));
		xmlNodeAddContentLen(text, huebuf,huelen);
		xmlNodeAddContentLen(text, LITLEN(", 100%,98%);\n}\n")); // very light
	}
	xmlNode* style = xmlNewNode(head->ns, "style");
	xmlAddChild(style,text);
	//printf("ummmmm %s\n",head->name);
	xmlAddChild(head, style);
}

static
void divvy_siblings(struct chatctx* ctx, xmlNode* middle, int colon) {
	xmlNode* row = xmlNewNode(middle->ns,"tr");
	xmlNode* name = xmlNewNode(middle->ns,"th");
	xmlNode* value = xmlNewNode(middle->ns,"td");
	xmlAddChild(row,name);
	xmlAddChild(row,value);
	xmlNode* cur = middle->first; // ->parent->children...
	while(cur != middle) {
		xmlNode* next = cur->next;
		xmlAddChild(name,cur);
		cur = next;
	}

	int leftstart;
	for(leftstart=0;leftstart<colon;++leftstart) {
		if(!isspace(content[leftstart])) {
			int leftend;
			for(leftend=colon;leftend>leftstart;--leftend) {
				if(!isspace(content[leftend])) {
					xmlNodeAddContentLen(name,
															 content+leftstart,
															 leftend-leftstart);
					break;
				}
			}
			break;
		}
	// or maybe there's no (nonblank) plain text between <i>specialsnowflake</i> and :
	int rightstart;
	for(rightstart=colon+1;rightstart<length;++rightstart) {
		if(!isspace(content[rightstart])) {
			int rightend;
			for(rightend=length-1;rightend>colon;--rightend) {
				if(!isspace(content[rightend])) {
					// add text before the first element... tricky
					// no convenient method like with appending.
					
					xmlNode* newt = xmlNewTextLen(content+rightstart,
																				rightend-rightstart);
					xmlNode* first = value->first;
					if(!first) {
						// just set it as the newt?
						xmlAddChild(value,newt);
					} else {
						switch(first->type) {
						case XML_TEXT_NODE:
						case XML_CDATA_SECTION_NODE:
							// add it to the new one, then toss the old one!
							xmlNodeAddContent(newt, first->content);
							// prev not next, so less setup for rearranging newt as the first
							xmlAddPrevSibling(first, newt);
							xmlUnlinkNode(first);
							break;
						case XML_ELEMENT_NODE:
						case XML_COMMENT_NODE:
							// add the text node as a previous sibling.
							xmlAddPrevSibling(first, newt);
							// do NOT unlink first, since we're still using it
							break;
						default:
							error(23,0,"uh... why is there a weird node down here? %d",first->type);
						};
					}
				}
			}
			break;
		}
	}
						
}
static
void process_paragraph(struct chatctx* ctx, xmlNode* e) {
	/* after hishification,chat will now be a list of paragraphs, each of which have a name,
		 then text containing :, then a value. */
	xmlNode* middle = e->children;
	for(;;) {
		if(middle->type == XML_TEXT_NODE) {
			xmlChar* colon = strchr(middle->content,':');
			if(colon != NULL) {
				// divide this text node in half... before is the name, after is the value.
				return divvy_siblings(ctx, middle, colon);
			}
		}
		colon = colon->next;
		if(!colon) return;
	}
}

static
void process_chat(struct chatctx* ctx, xmlNode* chat) {
	// one table per chat block
	xmlNode* table = xmlNewNode(e->ns,"table");
	xmlSetProp(table, "class","chat");
	ctx->dest = table;
	ctx->odd = true;

	xmlNode* p = chat->children;
	while(p) {
		p = nextE(p);
		if(!p) break;
		process_paragraph(ctx, p);
	}
	// okay, done. replace it with the table.
	xmlAddNextSibling(chat,table);
	xmlUnlinkNode(chat);
	xmlFreeNode(chat);
}

static
void doparse(struct chatctx* ctx, xmlNode* top) {
	if(!top) return;
	xmlNode* next = top->next;
	if(top->name) {
		if(lookup_wanted(top->name) == W_CHAT) {
			found_chat(ctx, top);
		} else {
			doparse(ctx, top->children); // depth usually less than breadth
		}
	} else if(top->type == XML_HTML_DOCUMENT_NODE) {
		return doparse(ctx, top->children); // oops
	}
	return doparse(ctx, next); // tail recursion on -O2
}

void parse_chat(xmlNode* top, xmlNode* ohead) {
	
	struct chatctx ctx = {
		.odd = true,
		.names = NULL,
		.nnames = 0,
		.dest = NULL, // set to current table
	};
	doparse(&ctx, top->children);
	
	/* now craft the stylesheet... because one style per name
		 is better than one style per row */

	if(ctx.nnames > 0) {
		craft_style(&ctx, ohead);
		free(ctx.names);
	}
}
