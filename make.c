#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

int verboseSystem(const char* command) {
    printf("> %s\n",command);
    return system(command);
}

struct string {
    char* str;
    ssize_t space;
    ssize_t len;
};

struct string* makeString(const char* what) {
    struct string* self = malloc(sizeof(struct string));
    self->str = strdup(what);
    self->space = strlen(what);
    self->len = self->space;
}

struct string* stringAppend(struct string* what, const char* thing) {
    ssize_t tlen = strlen(thing);
    while(what->len+tlen > what->space) {
        what->space = ((what->len+tlen)/0x10+1)*0x10;
        what->str = realloc(what->str,what->space);
    }
    memcpy(what->str+what->len,thing,tlen);
    what->str[what->len+tlen]='\0';
    what->len += tlen;
    return what;
}

void vCompile(const char* target, va_list list) {
    struct stat tstat;
    int olderExists = (0 != stat(target,&tstat));
    // if no target, then olderExists is by default true
    struct string* command = makeString("gcc ");
    command = stringAppend(command,getenv("CCARGS"));
    if(getenv("link")==NULL)
        command = stringAppend(command," -c");
    command = stringAppend(command," -o ");
    command = stringAppend(command,target);

#define ADD_ARG(arg) { command = stringAppend(command," "); command = stringAppend(command,arg); }

    for(;;) {
        char* next = va_arg(list,char*);
        if(NULL==next) break;
        if(!olderExists) {
            struct stat buf;
            stat(next,&buf);
            if(tstat.st_mtime < buf.st_mtime) {
                olderExists = 1;
            }
        }
        ADD_ARG(next);
    }

    if(olderExists)
        assert(0==verboseSystem(command->str));
    free(command->str);
    free(command);
}

void compile(const char* target, ...) {
    unsetenv("link");
    va_list list;
    va_start(list,target);
    vCompile(target,list);
    va_end(list);
}

void linky(const char* target, ...) {
    setenv("link","1",1);
    va_list list;
    va_start(list,target);
    vCompile(target,list);
    va_end(list);
}


int main(void) {
    setenv("CCARGS","`xml2-config --cflags --libs`",1);
    compile("parse.o","parse.c",NULL);
    linky("parse","parse.o",NULL);
}
