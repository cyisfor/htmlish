#include "libxmlfixes.h"
#include "ensure.h"
#include "html_when.h"
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <assert.h>

int main(int argc, char**argv) {
	htmlParserCtxt* ctxt;
	xmlSubstituteEntitiesDefault(0);

	xmlInitParser();
	ctxt = htmlNewParserCtxt();
	assert (ctxt != NULL);
	ctxt->recovery = 1;
	void	on_error(void * userData, xmlErrorPtr error) {
		fprintf(stderr,"um %s %s\n",error->message,
						error->level == XML_ERR_FATAL ? "fatal..." : "ok");
		return;
	}
	xmlSetStructuredErrorFunc(NULL,on_error);
	ctxt->sax->serror = &on_error;

	xmlDoc* doc = htmlCtxtReadFd(ctxt,
															 0,"","UTF-8",
															 HTML_PARSE_RECOVER |
															 HTML_PARSE_NONET |
															 HTML_PARSE_COMPACT);
	HTML5_plz(doc);

	htmlNodeDumpFileFormat(stderr,doc,doc,"UTF-8",1);
	ensure_ne(NULL,doc)
	html_when((xmlNode*)doc); // magic...
	htmlSaveFileEnc("/tmp/output.deleteme",doc,"UTF-8");
}
