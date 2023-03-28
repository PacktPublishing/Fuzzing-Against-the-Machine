#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <net/if.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include "fuzz.h"
#include "drv.h"
#include "sysc.h"

#define MAXFILTCALLS 10

#define READ(_x)    if (fuzzread(0, &_x, sizeof(_x)) < sizeof(_x)) continue


/* return true if we should execute this call */
static int 
filterCalls(unsigned short *filtCalls, int nFiltCalls, struct sysRec *recs, int nrecs) 
{
    int i, j, match;

    /* all records should have calls on the filtCalls list */
    for(i = 0; i < nrecs; i++) {
        match = 0;
        for(j = 0; j < nFiltCalls; j++) {
            if(recs[i].nr == filtCalls[j])
                match = 1;
        }   
        /* note: empty list is a match */
        if(!match && nFiltCalls > 0)  
            return 0;
    }   
    return 1;
}


int main() {
    struct sysRec recs[3];
    struct slice slice;
    unsigned short filtCalls[MAXFILTCALLS];
    char *prog, buf[256];
    u_long sz;
    long x;
    int opt, nrecs, nFiltCalls, parseOk;
    int noSyscall = 0;
    int enableTimer = 0;

    nFiltCalls = 0;

    while (true) {
        printf("vmstop\n");
        fuzz_vm_stop();
        printf("sthread\n");
        fuzz_set_thread();

        while (1) {
          sz = fuzzread(0, &buf, sizeof(slice));
          mkSlice(&slice, buf, sz); 
          parseOk = parseSysRecArr(&slice, 3, recs, &nrecs);
          if(parseOk == 0 && filterCalls(filtCalls, nFiltCalls, recs, nrecs)) {
            if(noSyscall) {
              x = 0;
            } else {
            /* note: if this crashes, watcher will do doneWork for us */
            x = doSysRecArr(recs, nrecs);
            }   
          }
        }
    }
}
