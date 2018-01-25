#define _GNU_SOURCE // memchr

#include "libxmlfixes/o/wanted_tags.gen.h"
#include "libxmlfixes/src/libxmlfixes.h" // nextE

#include "parse_chat.h"

#include <stdlib.h> // qsort, bsearch, size_t
#include <stdio.h> // snprintf... should use atoi but base 16...
#include <ctype.h> // isspace
#include <string.h> // memchr
#include <stdbool.h>
#include <assert.h>
#include <error.h>

#define LITLEN(a) a,(sizeof(a)-1)

typedef size_t S; // easier to type
typedef unsigned short u16;

// thanks, djb

static
const u16 hashinit = 5381;

static
u16 hashchurn(const xmlChar* str, S left, u16 hash) {
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


// takes a churned hash, provides a conveniently orderly number
// also saves how many chatters, to setup style sheet for all of them
u16 chat_intern(struct chatctx* ctx, u16 hash) {
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
	/*
		these should go in a global stylesheet.
		xmlNodeAddContentLen(text, LITLEN(".chat  { border-collapse: collapse; }\n"));
		xmlNodeAddContentLen(text, LITLEN(".chat th { padding-right: 1ex; text-align: right; }\n"));
	*/

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
u16 churnonetag(xmlNode* e, u16 hash) {
	if(e->name) {
//		hash = hashchurn("<",1,hash);
		hash = hashchurn(e->name,strlen(e->name),hash);
		// attributes? pff
//		hash = hashchurn(">",1,hash);
	} else {
		hash = hashchurn("text",4,hash);
	}
	if(e->content) {
		hash = hashchurn(e->content,strlen(e->content),hash);
	}
	// end tags? pfffff
	return hash;
}

static
u16 churntag(xmlNode* top, u16 hash) {
	hash = churnonetag(top,hash);
	void recurse(xmlNode* e) {
		while(e) {
			hash = churnonetag(e, hash);
			recurse(e->children);
			e = e->next;
		}
	}
	recurse(top->children);
	return hash;
}

static
void divvy_text(xmlChar* content, S colon, S length, xmlNode* name, xmlNode* value) {
	S leftstart;
	for(leftstart=0;leftstart<colon;++leftstart) {
		if(!isspace(content[leftstart])) {
			S leftend;
			for(leftend=colon-1;leftend>=leftstart;--leftend) {
				if(!isspace(content[leftend])) {
					// remember, leftend is not a length, but the last position so length-1
					xmlNodeAddContentLen(name,
															 content+leftstart,
															 leftend-leftstart+1);
					break;
				}
			}
			break;
		}
	}
	// that's the left side of the colon...
	S rightstart;
	for(rightstart=colon+1;rightstart<length;++rightstart) {
		if(!isspace(content[rightstart])) {

			S rightend;
			xmlNode* first = value->children;
			if(!first) {
				/* do NOT strip on the end of the right side if further elements.
					 That'll result in
					 a: this is <i>italicized</i> 'k?
					 =>
					 <th>a</th><td>this is<i>italicized</i> 'k?
					 which looks like:
					 a this isitalicized 'k?
				*/
				for(rightend=length-1;rightend>=rightstart;--rightend) {
					if(!isspace(content[rightend])) {
						xmlNodeAddContentLen(value,
																 content+rightstart,
																 rightend-rightstart+1);
						return;
					}
				}
			}
			// we have children, so don't strip end of the right
			xmlNode* newt = xmlNewTextLen(content+rightstart,
													 length-rightstart);

			// add text before the first element... tricky
			// no convenient method like with appending.
			
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
			
			return;
		}
	}
	// there was only space on the right side of the colon?
	// makes perfect sense if "a:     <i>saying</i> stuff"
}

static
void divvy_siblings(struct chatctx* ctx, xmlNode* middle, S colon) {
	assert(middle);
	xmlNode* row = xmlNewNode(middle->ns,"tr");
	xmlNode* name = xmlNewNode(middle->ns,"th");
	xmlNode* value = xmlNewNode(middle->ns,"td");
	xmlAddChild(ctx->dest, row);
	xmlAddChild(row,name);
	xmlAddChild(row,value);
	u16 hash = hashinit;

	xmlNode* cur = middle->parent->children; // ->first...
	assert(cur);
	while(cur != middle) {
		xmlNode* next = cur->next;
		// interestingly, if you add a child without unlinking it, it results in corrupted memory.
		xmlUnlinkNode(cur);
		xmlAddChild(name,cur);
		hash = churntag(cur,hash);
		cur = next;
	}

	// don't forget the text inside middle
	hash = hashchurn(middle->content, colon, hash);
	
	u16 id = chat_intern(ctx, hash); // may have collisions, but who cares

	char buf[0x100] = "n";
	// id should be low, since index into names, not the hash itself
	snprintf(buf+1,0x100-1,"%x",id);
	xmlSetProp(row,"class",buf);

	cur = middle->next;
	while(cur) {
		xmlNode* next = cur->next;
		xmlUnlinkNode(cur);
		xmlAddChild(value, cur);
		cur = next;
	}

	xmlChar* content = middle->content;
	if(content) {
		divvy_text(content, colon, strlen(content), name, value);
	}
}

static
void process_paragraph(struct chatctx* ctx, xmlNode* e) {
	/* after hishification,chat will now be a list of paragraphs, each of which have a name,
		 then text containing :, then a value. */
	xmlNode* cur;
	xmlNode* middle = NULL;
	xmlChar* colon = NULL; // depends on if middle is set
	// searching for the middle element (a text node with a colon)
	for(cur=e->children;cur;cur=cur->next) {
		if(cur->type == XML_TEXT_NODE) {
			colon = strchr(cur->content,':');
			if(colon != NULL) {
				middle = cur;
				break;
			}
		}
	}

	if(middle) {
		// found a colon-separator!
		// divide the paragraph's children in half... before is the name, after is the value. Also divide the middle in half, to two text nodes before and after
		return divvy_siblings(ctx, middle, colon - middle->content);
	} else {
		// just add a single row cell
		xmlNode* row = xmlNewNode(e->ns,"tr");
		xmlNode* cell = xmlNewNode(e->ns,"td");
		xmlSetProp(cell, "colspan","2");
		xmlAddChild(ctx->dest, row);
		xmlAddChild(row,cell);
		xmlNode* cur = e->children;
		do {
			xmlNode* next = cur->next;
			xmlUnlinkNode(cur);
			xmlAddChild(cell,cur);
			cur = next;
		} while(cur);
	}
}

static
void process_chat(struct chatctx* ctx, xmlNode* chat) {
	// one table per chat block
	xmlNode* table = xmlNewNode(chat->ns,"table");
	xmlSetProp(table, "class","chat");
	ctx->dest = table;
	ctx->odd = true;

	xmlNode* p = chat->children;
	while(p) {
		p = nextE(p);
		if(!p) break;
		process_paragraph(ctx, p);
		// paragraph is cleaned out, but next ps should be there.
		p = p->next;
	}
	// okay, done. replace the destroyed chat tag with our table.
	xmlAddNextSibling(chat,table);
	xmlUnlinkNode(chat);
	xmlFreeNode(chat);
}

static
void doparse(struct chatctx* ctx, xmlNode* top) {
	if(!top) return;
	xmlNode* next = top->next;
	if(top->type == XML_ELEMENT_NODE) {
		if(lookup_wanted(top->name) == W_CHAT) {
			process_chat(ctx, top);
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
