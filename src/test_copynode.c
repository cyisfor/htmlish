#include <libxml/tree.h> // xmlNode*
#include <stdio.h>
#include <assert.h>

#define LITLEN(a) (a),sizeof(a)-1
int main(int argc, char *argv[])
{
	xmlDoc* doc = xmlParseMemory(LITLEN(
																 "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
																 "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
																 "<html xmlns=\"http://www.w3.org/1999/xhtml\"><body></body></html>"));
	xmlDocDump(stdout,doc);
	xmlDoc* other = xmlParseMemory(LITLEN(
																	 "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
																 "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
																	 "<html xmlns=\"http://www.w3.org/1999/xhtml\"><head></head></html>"));
	xmlDocDump(stdout,other);
	assert(doc->children);
	xmlNode* copy = xmlDocCopyNode(doc->children, other, 1);
	xmlAddChild(other->children, copy);
	printf("um %s\n",copy->name);
	puts("--------");
	xmlDocDump(stdout,other);
	return 0;
}
