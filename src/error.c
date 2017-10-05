#include "htmlish.h"
#include "html_when.h"
#define LITLEN(a) a,(sizeof(a)-1)

#include "libxmlfixes/wanted_tags.h"
#include <libxml/xmlerror.h>
#include <string.h>


#include <stdio.h>

bool htmlish_handled_error(xmlErrorPtr error) {
	if(html_when_handled_error(error)) return true;
	switch(error->code) {
	case XML_ERR_ERROR:
		if(0==strncmp(error->message,LITLEN("htmlParseEntityRef: expecting ';'"))) {
			// they want us to replace every & with &AMP; in attributes that are
			// ALREADY DELINEATED BY QUOTATION MARKS
			return true;
		}
		break;
	case XML_HTML_UNKNOWN_TAG: {
		const char* name = error->str1;
		if(lookup_wanted(name) != UNKNOWN_TAG) return true;
		size_t nlen = strlen(name);
#define IS(a) (nlen == sizeof(a)-1) && (0 == memcmp(name,a,sizeof(a)-1))
		if(IS("top") || IS("content") || IS("header") || IS("footer") || IS("intitle")
			 || IS("chat"))
			// not errors, these get removed by template stuffs
			return true;
		break;
	}
	};
	fprintf(stderr,"um %d %s %s\n",error->code, error->message,
					error->level == XML_ERR_FATAL ? "fatal..." : "ok");
	return false;
}
