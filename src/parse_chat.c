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

static
void craft_style(struct chatctx* ctx) {
	short id;
	// XXX: xmlNewTextLen?
	xmlNode* text = xmlNewCDataBlock(ctx->doc, NULL, 0);
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
	xmlNode* style = xmlNewNode(ctx->dest->ns, "style");
	xmlAddChild(style,text);
	// root -down-> DOCTYPE -next-> html -down-> head
	xmlNode* head = ctx->doc->children->next->children;
	if(lookup_wanted(head->name) != W_HEAD) {
		// this document only has a <body> so far...
		xmlNode* realhead = xmlNewNode(ctx->dest->ns, "head");
		xmlAddPrevSibling(head,realhead);
		head = realhead;
		//printf("ummmmm %s %s\n",head->name,head->next->name);
	}
	assert(lookup_wanted(head->name) == W_HEAD);
	//printf("ummmmm %s\n",head->name);
	xmlAddChild(head, style);
}

static
void found_chat(struct chatctx* ctx, xmlDoc* doc, xmlNode* e) {
	xmlNode* te = e->children;
	assert(te->type == XML_TEXT_NODE);
	xmlNode* table = xmlNewNode(e->ns,"table");
	xmlSetProp(table, "class","chat");
	ctx->dest = table;
	ctx->odd = true;

	xmlChar* start = te->content;
	size_t left = strlen(start);
	for(;;) {
		xmlChar* nl = memchr(start,'\n',left);
		if(nl == NULL) {
			take_line(ctx, start,left);
			break;
		} else if(nl == start) {
			// blank line
			--left;
			start = nl+1;
		} else {
			// not counting newline
			take_line(ctx, start,nl-start-1);
			// we eat the newline though
			left -= nl-start+1;
			start = nl+1;
		}
	}
	xmlAddNextSibling(e,table);
	xmlUnlinkNode(e);
	xmlFreeNode(e);

}

static
void doparse(struct chatctx* ctx, xmlDoc* doc, xmlNode* top) {
	if(!top) return;
	xmlNode* next = top->next;
	if(top->name) {
		if(lookup_wanted(top->name) == W_CHAT) {
			return found_chat(ctx, doc, top);
		}
		doparse(ctx, doc, top->children); // depth usually less than breadth
	} else if(top->type == XML_HTML_DOCUMENT_NODE) {
		return doparse(ctx, doc, top->children); // oops
	}
	return doparse(ctx, doc, next); // tail recursion on -O2
}

void parse_chat(xmlDoc* top) {
	struct chatctx ctx = {
		.odd = true,
		.names = NULL,
		.nnames = 0,
		.dest = NULL, // set to current table
		.doc = top
	};
	doparse(&ctx, top,top->children);
	
	/* now craft the stylesheet... because one style per name
		 is better than one style per row */

	if(ctx.nnames > 0) {
		craft_style(&ctx);
		free(ctx.names);
	}
}
