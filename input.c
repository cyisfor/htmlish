#include "input.h"
#include <libxml/xmlIO.h>

#include <sys/stat.h>

#include <wordexp.h>

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

const char* cacheDir = NULL;

bool reallyDownload = false;

static int match(const char* uri) {
    if(reallyDownload) return 0;
    fprintf(stderr,"cache match %s\n",uri);
    // any other remote URLs that don't conform to this?
    return strstr(uri,"://")== NULL ? 0 : 1;
}

static int recursive_read(xmlParserInputBufferPtr source, char* buffer, int pushed, int left, bool* eof) {
    //if(left == 0) return pushed;
    assert(left>0);

    int nowpushed = xmlParserInputBufferPush(source,left,buffer);
    assert(nowpushed>=0);

    pushed += nowpushed;
    buffer += nowpushed;
    left -= nowpushed;

    if(left>0) {
        int ok = xmlParserInputBufferRead(source,left);
        if(ok<0) {
            *eof = true;
        } else {
            // XXX: no need to fill up to full capacity every time...
            return recursive_read(source,buffer,pushed,left);
        }
    }
    return pushed;
}

static void* cacheopen(char const* url) {
    const char* basename = strrchr(url,'/');
    assert(basename);

    static char dest[508];
    snprintf(dest,508,"%s/%s",cacheDir,basename);

    static char temp[512];
    bool didtemp = false;

    int doopen(void) {
        int result = open(dest,O_RDONLY);
        if(result<0) {
            xmlParserInputBufferPtr source = xmlParserInputBufferCreateFile(fopen(dest,"rt"),XML_CHAR_ENCODING_UTF8);
            if(!didtemp) {
                snprintf(temp,512,"%s.temp",dest);
                didtemp = true;
            }
            int out = open(temp,O_WRONLY|O_TRUNC|O_CREAT,0644);
            assert(out>0);
            char buf[0x1000];
            bool eof = false;
            do {
                int amt = recursive_read(source,buf,0,0x1000,&eof);
                // should return the tail end of the file, when eof is true.
                write(out,buf,amt);
            } while(!eof);
            close(out);
            rename(temp,dest); // doesn't matter if fails
            unlink(temp); // in case it failed
            return doopen();
        }
        return result;
    }
    return doopen();
}

void setupInput(void) {
    wordexp_t p;
    struct stat info;
    wordexp("~/.cache/", &p, 0);
    assert(p.we_wordc > 0);
    if(0!=stat(p.we_wordv[0],&info)) {
        mkdir(p.we_wordv[0],0755);
    }
    assert(0==wordexp("~/.cache/xml/",&p, WRDE_REUSE));
    if(0!=stat(p.we_wordv[0],&info)) {
        mkdir(p.we_wordv[0],0755);
    }
    cacheDir = strdup(p.we_wordv[0]);
    wordfree(&p);

    assert(cacheDir);

    xmlRegisterInputCallbacks(match,cacheopen,read,close);
    xmlRegisterDefaultInputCallbacks();
}
