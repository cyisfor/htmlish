#include <libxml/tree.h> // xmlNode*
#include <stdio.h>

#define LITLEN(a) (a),sizeof(a)-1
int main(int argc, char *argv[])
{
	xmlDoc* doc = xmlParseMemory(LITLEN(
																 "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<xml><thing></thing></xml>"));
	xmlDocDump(stdout,doc);
	xmlDoc* other = xmlParseMemory(LITLEN(
																 "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<xml><thing2></thing2></xml>"));
	xmlDocDump(stdout,other);
	xmlNode* copy = xmlDocCopyNode(doc->children, other, 1);
	xmlAddChild(other->children, copy);
	puts("--------");
	xmlDocDump(stdout,other);
	return 0;
}
