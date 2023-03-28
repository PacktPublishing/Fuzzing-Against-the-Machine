/*
 * AFL hypercalls
 *
 * Compile with -DTEST to take inputs from stdin without using hypercalls.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "drv.h"

int aflTestMode = 0;

#define SZ 4096
static u_long bufsz;
static char *buf;
static u_int64_t *arr;

static void
aflInit(void)
{
    static int aflInit = 0;
    char *pg;

    if(aflInit)
        return;

    pg = mmap(NULL, SZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(pg == (void*)-1) {
        perror("mmap");
        exit(1);
    }
    memset(pg, 0, SZ); // touch all the bits!

    arr = (u_int64_t *)pg;
    buf = pg + 2 * sizeof arr[0];
    bufsz = SZ - 2 * sizeof arr[0];

    aflInit = 1;
}


static inline unsigned int aflCall(unsigned int a0, unsigned int a1, unsigned int a2)
{

    unsigned int ret;
    register long r0 asm ("r0") = a0;
    register long r1 asm ("r1") = a1;
    register long r2 asm ("r2") = a2;

    //asm(".word 0x0f4c4641"
    asm volatile("svc 0x3f" //.byte 0x3f, 0xdf" // 0x0f4c4641"
            : "=r"(r0)
            : "r"(r0), "r"(r1), "r"(r2)
            );
    ret = r0;

    return ret;
}

int
startForkserver(int ticks)
{
    aflInit();
    if(aflTestMode)
        return 0;
    return aflCall(1, ticks, 0);
}

char *
getWork(u_long *sizep)
{
    aflInit();
    if(aflTestMode)
        *sizep = read(0, buf, bufsz);
    else
        *sizep = aflCall(2, (u_long)buf, bufsz);
    return buf;
}

/* buf should point to u_int64_t[2] */
int
startWork(u_int64_t start, u_int64_t end)
{
    aflInit();
    if(aflTestMode)
        return 0;
    arr[0] = start;
    arr[1] = end;
    return aflCall(3, (u_long)arr, 0);
}

int
doneWork(int val)
{
    aflInit();
    if(aflTestMode)
        return 0;
    return aflCall(4, (u_long)val, 0);
}

