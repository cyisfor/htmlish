#include "selectors.h"
#include <stdlib.h> // malloc...
#include <error.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
static void find_destroy(struct Selector* s) {
}

void find_start(struct Selector* s, const char* name) {
	assert(s->last == DOWN);
	s->name = name;
}

const char* namefor(xmlNode* n) {
	if(n==NULL) return "(null)";
	if(n->type == XML_TEXT_NODE) {
		return xmlNodeGetContent(n);
	}
	return n->name;
}

xmlNode* find_next(xmlNode* cur, struct Selector* pos) {
#define POS pos->data[pos->n-1]
	if(!cur) return NULL;
	bool right(void) {
		if(!cur->next) return false;
		fprintf(stderr,"RIGHT %s->%s\n",
						namefor(cur),
						namefor(cur->next));
		cur = cur->next;
		return true;
	}

	bool down(void) {
		if(!cur->children) return false;
		fprintf(stderr,"DOWN %s->%s\n",
						namefor(cur),
						namefor(cur->children));
		cur = cur->children;
		return true;
	}

	bool up(void) {
		if(!cur->parent) return false;
		fprintf(stderr,"UP %s->%s\n",
						namefor(cur),
						namefor(cur->parent));
		cur = cur->parent;
		return true;
	}
		
	for(;;) {
		if(cur->type == XML_ELEMENT_NODE && strcasecmp(cur->name, pos->name)==0) {
			fprintf(stderr,"found it!\n");
			return cur;
		}
		bool check = down();
		if(!check) check = right();
		if(!check) {
			// if we go up, just go immediately right but not down
			check = up();
			if(check)
				check = right();
			if(!check) {
				// can't go right, down, or up/right
				find_destroy(pos);
				return NULL;
			}
		}
	}
}
