#include <libxml/tree.h>

void htmlish(xmlDoc* dest, int src);

extern const char defaultTemplate[];

xmlDoc* readFunky(int fd, const char* content);
