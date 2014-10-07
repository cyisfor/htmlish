#inclued "input.h"
#include <libxml/xmlIO.h>

const char* cacheDir = NULL;

bool reallyDownload = false;

static int match(const char* uri) {
    if(reallyDownload) return 0;
    // any other remote URLs that don't conform to this?
    return strstr(uri,"://")== NULL ? 0 : 1;
}

static void* cacheopen(char const* url) {
    const char* basename = strrchr(url,'/');
    assert(basename);

    xmlParserInputBufferPtr result = NULL;

    static const char dest[512];
    snprintf(dest,512,"%s/%s",cacheDir,basename);
    struct stat info;
    if(0==stat(dest,&info)) {
        result = xmlParserInputBufferCreateFile(fopen(dest,"rt"),XML_CHAR_ENCODING_UTF8);
    } else {
        // recursion blocker
        reallyDownload = true;
        result = xmlParserInputBufferCreateFilename(url,XML_CHAR_ENCODING_UTF8);
        reallyDownload = false;
    }
    return result;
}

static int recursive_read(xmlParserInputBufferPtr source, char* buffer, int pushed, int left) {
    //if(left == 0) return pushed;
    assert(left>0);

    int nowpushed = xmlParserInputBufferPush(source,left,buffer);
    assert(nowpushed>=0);

    pushed += nowpushed;
    buffer += nowpushed;
    left -= nowpushed;

    if(left>0) {
        int ok = xmlParserInputBufferRead(source,len-pushed);
        assert(ok==0);
        // no need to fill up to full capacity every time...
        return recursive_read(source,buffer,pushed,left);
    }
    return pushed;
}

static int cacheread(void* ctx, char* buffer, int len) {
    xmlParserInputBufferPtr source = (xmlParserInputBufferPtr)ctx;
    return recursive_read(source,buffer,0,len);
}

static int cacheclose(void* ctx) {
    xmlParserInputBufferPtr source = (xmlParserInputBufferPtr)ctx;
    xmlFreeParserInputBuffer(source);
    return 0;
}

void setupInput(void) {
    wordexp_t exp;
    struct stat info;
    wordexp("~/.cache/", &p, 0);
    assert(p.we_wordc > 0);
    if(0!=stat(p.we_wordv[0],&info)) {
        mkdir(p.we_wordv[0]);
    }
    wordexp("~/.cache/xml/",&p, WRDE_REUSE);
    cacheDir = strdup(p.we_wordv[0]);
    wordfree(&p);

    assert(cacheDir);

    xmlRegisterInputCallbacks(match,cacheopen,cacheread,cacheclose);
    xmlRegisterDefaultInputCallbacks();
}
