#include <libxml/tree.h> // xmlNode*
#include <stdio.h>


int main(int argc, char *argv[])
{
	xmlDoc* doc = xmlParseMemory(LITLEN(
																 "<?xml charset=\"utf-8\"?>\n<xml><thing></thing></xml>"));
	xmlDocDump(stdout,doc);
	xmlDoc* other = xmlParseMemory(LITLEN(
																 "<?xml charset=\"utf-8\"?>\n<xml><thing2></thing2></xml>"));
	xmlDocDump(stdout,other);
	xmlNode* copy = xmlDocCopyNode(doc->root, other, 1);
	xmlAddChild(other->root, copy);
	puts("--------");
	xmlDocDump(stdout,other);
	return 0;
}
