#include <libxml/tree.h>
#include <stdbool.h>

void htmlish_str(xmlNode* content, const char* s, size_t l, bool as_children);
void htmlish(xmlNode* content, int fd, bool as_children);
xmlNode* getContent(xmlNode* oroot, bool createBody, bool* as_child);
bool htmlish_handled_error(xmlErrorPtr error);
