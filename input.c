#include "input.h"
#include <libxml/xmlIO.h>
#include <libxml/nanohttp.h>
#include <libxml/nanoftp.h>

#include <sys/stat.h>

#include <wordexp.h>

#include <unistd.h> // read write etc
#include <fcntl.h> // open

#include <sys/wait.h>


#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

const char* cacheDir = NULL;

bool reallyDownload = false;

static int match(const char* uri) {
    if(reallyDownload) return 0;
    fprintf(stderr,"cache match %s\n",uri);
    // any other remote URLs that don't conform to this?
    return strstr(uri,"://")== NULL ? 0 : 1;
}

static void* cacheopen(char const* url) {
    const char* basename = strrchr(url,'/');
    assert(basename);

    static char dest[508];
    snprintf(dest,508,"%s/%s",cacheDir,basename);

    bool didtemp = false;

    int doopen(void) {
        int result = open(dest,O_RDONLY);
        if(result<0) {
            char temp[512];
            snprintf(temp,512,"%s.temp",dest);

            if(xmlIOHTTPMatch(url)) {
                void* ctx = xmlNanoHTTPOpen(url, NULL);
                if(xmlNanoHTTPReturnCode(ctx) != 200) {
                    // XXX: it always is... w3.org dies on HTTP/1.0
                    xmlNanoHTTPClose(ctx);
                    fprintf(stderr,"Curl fallback for %s\n",url);
                    int pid = fork();
                    if(pid == 0) {
                        execlp("curl","curl","-o",temp,url);
                        abort();
                    }
                    int status = 0;
                    waitpid(pid,&status,0);
                    if(!(WIFEXITED(status) && (0 == WEXITSTATUS(status)))) {
                        fprintf(stderr,"CUrl failed! %x %d\n",status,WEXITSTATUS(status));
                        abort();
                    }
                } else {
                    assert(0==xmlNanoHTTPSave(ctx,temp));
                }
            } else if(xmlIOFTPMatch(url)) {
                void* ftp = xmlNanoFTPOpen(url);
                int out = open(temp,O_WRONLY|O_TRUNC|O_CREAT,0644);
                assert(out>0);
                char buf[0x1000];
                for(;;) {
                    int amt = xmlNanoFTPRead(ftp, buf, 0x1000);
                    if(amt==0) break;
                    assert(amt>0);
                    write(out,buf,amt);
                }
                close(out);            
            } else {
                FILE* fp = xmlFileOpen(url);
                struct stat fpstat;
                if(!fp) {
                    fprintf(stderr,"No idea what to do with url %s\n",url);
                    abort();
                }
                int inp = fileno(fp);
                assert(0==fstat(inp,&fpstat));
                off_t left = fpstat.st_size;
                int out = open(temp,O_WRONLY|O_TRUNC|O_CREAT,0644);
                assert(out>0);
                do {
                    ssize_t amt = sendfile(out,inp,NULL,left);
                    if(amt<0) {
                        perror(url);
                    }
                    assert(amt>=0);
                    left -= amt;
                } while(left > 0);
                fclose(fp);
                close(out);
            }
            rename(temp,dest); // doesn't matter if fails
            unlink(temp); // in case it failed
            return doopen();
        }
        return result;
    }
    return (void*)(intptr_t) doopen();
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

    xmlNanoHTTPInit();
    xmlNanoFTPInit();

    xmlRegisterInputCallbacks(match,cacheopen,(void*)read,(void*)close);
    xmlRegisterDefaultInputCallbacks();
}
