#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include "memlayout.h"

typedef struct {
    uint32_t next;
    uint32_t to;
} link;

typedef struct {
    uint32_t node;
    uint32_t linkid;
    uint32_t pathlen;
} stackitem;

void FindCycles(void *ctx)
{
    stackitem *stack = ctx + CK_CYCLESTACK;
    link *links = ctx + CK_LINKS;
    uint32_t *pu32solcount = ctx;
    uint64_t *pu64sol = ctx + 8;
    uint32_t *adjlist = ctx + CK_ADJLIST;
    uint32_t rootnode, tonode, pathlen, linkid0;
    int32_t stackptr, i, j, linkid;

    stackptr = 0;
    stack[0].pathlen = 0;
    pu32solcount[0] = 0;
    pu32solcount[1] = 0;

    for (rootnode=0; rootnode<262144; rootnode++) {
        linkid = adjlist[rootnode];
        if (linkid <= 0) {
            continue;
        }
        adjlist[rootnode] = (linkid & 0x0fffffff) | 0xc0000000;
        //stackptr = 0;
        stack[0].node = rootnode;

        do {
            stack[stackptr].linkid = linkid;
            tonode = links[linkid].to;
            linkid = adjlist[tonode];
            pathlen = 0;
            while (linkid) {
                pathlen++;
                if (linkid & 0x40000000) {
                    linkid0 = linkid & 0x0fffffff;
                    for (i=stackptr; (i>=0) && (tonode != stack[i].node); i--) {
                        pathlen += stack[i].pathlen;
                        adjlist[stack[i].node] |= 0x20000000;
                    }
                    if (stack[i].linkid & 0x20000000) {
                        //printf("Repeated %4d-cycle\n", pathlen*2);
                        break;
                    }
                    if (i<0) {
                        printf("BUG: Cannot find linkid on stack\n");
                        break;
                    }
                    printf("%4d-cycle found\n", pathlen*2);
                    if (pathlen == 21) {
                        linkid = stack[i].linkid;
                        tonode = links[linkid & 0x0fffffff].to;
                        *(pu64sol++) = tonode;
                        for (j=0; j<20; j++) {
                            linkid = adjlist[tonode];
                            if (linkid & 0x40000000) {
                                linkid = stack[++i].linkid;
                            }
                            tonode = links[linkid & 0x0fffffff].to;
                            *(pu64sol++) = tonode;
                        }
                        *pu32solcount += 1;
                        //Optional error check
                        linkid = adjlist[tonode];
                        if (!(linkid & 0x40000000)) {
                            printf("BUG: Cycle active flag not set\n");
                        }
                        if (i != stackptr) {
                            printf("BUG: Cycle length error\n");
                        }
                        //End of optional error check
                    }
                    if ((i == stackptr) && !(links[linkid0].next)) {
                        //printf(",");
                        links[linkid0].to = 0;
                    }
                    break;
                }
                linkid0 = linkid & 0x0fffffff;
                if (links[linkid0].next) {
                    if ((linkid & 0xa0000000) == 0x80000000) {
                        break;
                    }
                    stackptr++;
                    //if (stackptr >= 70) {
                    //    printf("stackptr %d\n", stackptr);
                    //}
                    stack[stackptr].node = tonode;
                    stack[stackptr].linkid = linkid;
                    stack[stackptr].pathlen = pathlen;
                    pathlen = 0;
                    adjlist[tonode] = (linkid & 0x0fffffff) | 0xc0000000;
                }
                else if (pathlen == 200) {
                    stackptr++;
                    //printf(".");
                    //if (stackptr >= 70) {
                    //    printf("stackptr B %d\n", stackptr);
                    //}
                    stack[stackptr].node = tonode;
                    stack[stackptr].linkid = linkid;
                    stack[stackptr].pathlen = pathlen;
                    pathlen = 0;
                    adjlist[tonode] = (linkid & 0x0fffffff) | 0xc0000000;
                }
                adjlist[tonode] |= 0x80000000;
                tonode = links[linkid0].to;
                linkid = adjlist[tonode];
            }
            stackptr++;
            do {
                linkid = stack[--stackptr].linkid;
                linkid = links[linkid & 0x0fffffff].next;
                if (linkid)
                    break;
                adjlist[stack[stackptr].node] &= 0xbfffffff;
            } while (stackptr);
        } while (linkid);
    }
}
