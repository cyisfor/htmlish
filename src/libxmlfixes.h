#include <libxml/tree.h> // xmlNode

xmlNode* fuckXPath(xmlNode* parent, const char* name);
xmlNode* fuckXPathDivId(xmlNode* parent, const char* id);
xmlNode* findOrCreate(xmlNode* parent, const char* path);
