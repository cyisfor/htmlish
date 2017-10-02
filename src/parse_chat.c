#define _GNU_SOURCE // memchr

#include "libxmlfixes/wanted_tags.h"

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
	xmlDoc* doc;
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
	if(ctx->odd) {
		xmlSetProp(row,"class", "o");
		ctx->odd = false;
	} else {
		// no need for an even class
		ctx->odd = true;
	}

	u16 id = chat_intern(ctx, name,nlen); // may have collisions, but who cares

	xmlNode* namecell = xmlNewNode(ctx->dest->ns,"td");

	char buf[0x100] = "n";
	// id should be low, since index into names, not the hash itself
	snprintf(buf+1,0x100-1,"%x\0",id);
	xmlSetProp(namecell,"class",buf);

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

static
void craft_style(struct chatctx* ctx) {
	short id;
	// XXX: xmlNewTextLen?
	xmlNode* text = xmlNewCDataBlock(ctx->doc, NULL, 0);
	xmlNodeAddContentLen(text, LITLEN("\n"));
	for(id=0;id<ctx->nnames;++id) {
		char buf[0x100];

		xmlNodeAddContentLen(text, LITLEN(".n"));
		xmlNodeAddContentLen(text, buf,
												 snprintf(buf,0x100,"%x",id));
		xmlNodeAddContentLen(text, LITLEN(" { color: hsl("));
		srandom(id);

		float hue = random() / ((float)RAND_MAX);
		float sat = random() * 100.0 / RAND_MAX; // any saturatin
		float light = 5.0 + random() * 20.0 / RAND_MAX; // relatively dark
		
		xmlNodeAddContentLen(text, buf,
												 snprintf(buf,0x100,"%f",hue));
		xmlNodeAddContentLen(text, LITLEN("turn, "));
		xmlNodeAddContentLen(text, buf,
												 snprintf(buf,0x100,"%f",sat));
		xmlNodeAddContentLen(text, LITLEN("%, "));
		xmlNodeAddContentLen(text, buf,
												 snprintf(buf,0x100,"%f",light));
		xmlNodeAddContentLen(text, LITLEN("%); }\n"));
	}
	xmlNode* style = xmlNewNode(ctx->dest->ns, "style");
	xmlAddChild(style,text);
	// root -down-> DOCTYPE -next-> html -down-> head
	xmlNode* head = ctx->doc->children->next->children;
	if(lookup_wanted(head->name) != W_HEAD) {
		// this document only has a <body> so far...
		xmlNode* realhead = xmlNewNode(ctx->dest->ns, "head");
		xmlAddPrevSibling(head,realhead);
		head = realhead;
	}
	xmlAddChild(head, style);
}

static
void found_chat(xmlDoc* doc, xmlNode* e) {
	xmlNode* te = e->children;
	assert(te->type == XML_TEXT_NODE);
	xmlNode* table = xmlNewNode(e->ns,"table");
	xmlSetProp(table, "class","chat");
	struct chatctx ctx = {
		.odd = true,
		.names = NULL,
		.nnames = 0,
		.dest = table,
		.doc = doc
	};
	xmlChar* start = te->content;
	size_t left = strlen(start);
	for(;;) {
		xmlChar* nl = memchr(start,'\n',left);
		if(nl == NULL) {
			take_line(&ctx, start,left);
			break;
		} else if(nl == start) {
			// blank line
			--left;
			start = nl+1;
		} else {
			// not counting newline
			take_line(&ctx, start,nl-start-1);
			// we eat the newline though
			left -= nl-start+1;
			start = nl+1;
		}
	}
	xmlAddNextSibling(e,table);
	xmlUnlinkNode(e);
	xmlFreeNode(e);

	/* now craft the stylesheet... because one style per name
		 is better than one style per row */

	if(ctx.nnames > 0) {
		craft_style(&ctx);
		free(ctx.names);
	}
}

static
void doparse(xmlDoc* doc, xmlNode* top) {
	if(!top) return;
	if(top->name) {
		if(lookup_wanted(top->name) == W_CHAT) {
			return found_chat(doc, top);
		}
		doparse(doc, top->children); // depth usually less than breadth
	} else if(top->type == XML_HTML_DOCUMENT_NODE) {
		return doparse(doc, top->children); // oops
	}
	return doparse(doc, top->next); // tail recursion on -O2
}

void parse_chat(xmlDoc* top) {
	return doparse(top,top->children);
}
