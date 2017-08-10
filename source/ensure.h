#include <stdio.h> // perror
#include <stdlib.h> // abort
#define ensure0(a) if(0 != (a)) { perror(#a); abort(); }
#define ensure_ne(a,b) if((a) == (b)) { perror(#a); abort(); }
