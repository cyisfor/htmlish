#include <libxml/tree.h>
#include <stdbool.h>

void htmlish(xmlNode* content, int fd, bool as_children);
xmlNode* getContent(xmlNode* oroot, bool createBody, bool* as_child);
