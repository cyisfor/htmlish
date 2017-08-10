#include "output.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h> // abort
#include <assert.h>

#define LITLEN(a) a,sizeof(a)-1
#define STRLEN(a) (a),strlen(a)

// gumbo sucks
static const unsigned char kGumboTagSizes[] = {
#include "tag_sizes.h"
    0,  // TAG_UNKNOWN
    0,  // TAG_LAST
};

void output(int dest, GumboNode* cur) {
	switch(cur->type) {
	case GUMBO_NODE_TEXT:
	case GUMBO_NODE_CDATA:
	case GUMBO_NODE_WHITESPACE:
		write(dest,cur->v.text.text,strlen(cur->v.text.text));
		return;
	case GUMBO_NODE_COMMENT:
		write(dest,LITLEN("<!--"));
		write(dest,cur->v.text.text,strlen(cur->v.text.text));
		write(dest,LITLEN("-->"));
		return;
	case GUMBO_NODE_DOCUMENT:
		if(cur->v.document.has_doctype) {
			// we'll only be producing html5 thx
			size_t len = strlen(cur->v.document.name);
			assert(len == 4);
			assert(memcmp(cur->v.document.name,"html",4) == 0);
			assert(*cur->v.document.public_identifier == 0);
			assert(*cur->v.document.system_identifier == 0);
		}
		write(dest,LITLEN("<!DOCTYPE html>\n"));
		// fall through
	case GUMBO_NODE_ELEMENT:
	case GUMBO_NODE_TEMPLATE:
		{ write(dest,LITLEN("<"));
			GumboElement* e = &cur->v.element;
			GumboStringPiece name;
			if(e->tag == GUMBO_TAG_UNKNOWN) {
				name = e->original_tag;
				gumbo_tag_from_original_text(&name);
			} else {
				name.data = gumbo_normalized_tagname(e->tag);
				name.length = kGumboTagSizes[e->tag];
			}
			write(dest,name.data,name.length);
			int i;
			for(i=0;i<e->attributes.length;++i) {
				write(dest,LITLEN(" "));
				GumboAttribute* a = (GumboAttribute*)e->attributes.data[i];
				write(dest,STRLEN(a->name));
				if(0!=a->original_value.length) {
					write(dest,LITLEN("="));
					// includes quotation marks
					write(dest,a->original_value.data,a->original_value.length);
				}
			}
			if(0 == e->children.length) {
				write(dest,LITLEN(" />"));
			} else {
				write(dest,LITLEN(">"));
				for(i=0;i<e->children.length;++i) {
					output(dest,e->children.data[i]);
				}
				write(dest,LITLEN("</"));
				write(dest,name.data,name.length);
				write(dest,LITLEN(">"));
			}
		}
		break;
	default:
		write(2,LITLEN("bad node type... wat is template?\n"));
		abort();
	};
}
