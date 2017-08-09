#include <libxml/tree.h>

void htmlish(xmlNode* dest, int fd);

extern const char defaultTemplate[];

xmlDoc* readFunky(int fd, const char* content);
