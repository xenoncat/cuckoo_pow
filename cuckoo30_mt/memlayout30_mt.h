#ifndef THREADS
#define THREADS 4
#endif

#if THREADS == 1
    #define THREADPLAN0 129
    #define THREADPLAN1 127
    //B0_SLOTS divisible by 64 to make CK_BIG1 aligned by 64
    #define B0_SLOTS 2113280
    #define B1_SLOTS 1337088
    #define B2_SLOTS 628992
    #define B3_SLOTS 376064
    #define BR_SLOTS 257024
    #define BP_SLOTS 257024
    #define LOGTHREADS 0
    #define PAIRSLIST_PT 262144
    #define PI_SLOTS_2 262144
    #define PI_SLOTS_3 10240
    #define PI_SLOTS_4 512
    #define PI_SLOTS_5 64
    #define PI_SLOTS_6 32
    //557568 dwords
    #define PI_THREADSTRIDE 2230336
#elif THREADS == 2
    #define THREADPLAN0 65
    #define THREADPLAN1 63
    //B0_SLOTS divisible by 16 to make CK_BIG1 aligned by 64
    #define B0_SLOTS 1056640
    #define B1_SLOTS 668544
    #define B2_SLOTS 314496
    #define B3_SLOTS 192000
    #define BR_SLOTS 128512
    #define BP_SLOTS 128512
    #define LOGTHREADS 1
    #define PAIRSLIST_PT 131072
    #define PI_SLOTS_2 131072
    #define PI_SLOTS_3 6400
    #define PI_SLOTS_4 384
    #define PI_SLOTS_5 64
    #define PI_SLOTS_6 32
    //283392 dwords
    #define PI_THREADSTRIDE 1133632
#elif THREADS == 4
    #define THREADPLAN0 33
    #define THREADPLAN1 31
    //B0_SLOTS divisible by 4 to make CK_BIG1 aligned by 64
    #define B0_SLOTS 531200
    #define B1_SLOTS 336384
    #define B2_SLOTS 159232
    #define B3_SLOTS 96000
    #define BR_SLOTS 64256
    #define BP_SLOTS 64256
    #define LOGTHREADS 2
    #define PAIRSLIST_PT 65536
    #define PI_SLOTS_2 65536
    #define PI_SLOTS_3 3200
    #define PI_SLOTS_4 192
    #define PI_SLOTS_5 32
    #define PI_SLOTS_6 32
    //141792 dwords
    #define PI_THREADSTRIDE 567232
#elif THREADS == 8
    #define THREADPLAN0 17
    #define THREADPLAN1 15
    #define B0_SLOTS 266496
    #define B1_SLOTS 169216
    #define B2_SLOTS 80128
    #define B3_SLOTS 48000
    #define BR_SLOTS 32128
    #define BP_SLOTS 32128
    #define LOGTHREADS 3
    #define PAIRSLIST_PT 32768
    #define PI_SLOTS_2 32768
    #define PI_SLOTS_3 1600
    #define PI_SLOTS_4 128
    #define PI_SLOTS_5 32
    #define PI_SLOTS_6 32
    //71200 dwords
    #define PI_THREADSTRIDE 284864
#else
    #error
#endif

#define HALFSIZE 0x20000000
#define LOGHALFSIZE 29
#define EDGEMASK (HALFSIZE - 1)
#define NODEMASK (HALFSIZE - 1)
#define NONCEMASK (2*HALFSIZE - 1)
#define NONCEMASKE (2*HALFSIZE - 2)
#define HALFCYCLELENGTH 21

#define B_BUCKETS 256UL
#define BPT (256/THREADS)
#define B_LOG 8
#define B_BUCKETMASK (B_BUCKETS - 1)
#define B0_SLOTSIZE 5
#define B1_SLOTSIZE 5
#define B2_SLOTSIZE 5
#define B3_SLOTSIZE 5
#define BR_SLOTSIZE 6
#define BP_SLOTSIZE 4
#define B_SLOTPAYLOADSHIFT (LOGHALFSIZE-B_LOG)

#define S_BUCKETS 256
#define S_LOG 8
#define S_BUCKETMASK (S_BUCKETS - 1)
#define S0_SLOTS 9280
#define S1_SLOTS 6208
#define SR_SLOTS 5056
#define S0_SLOTSIZE 5
#define S1_SLOTSIZE 5
#define SR_SLOTSIZE 6

#define R_ENTRIES 8192
#define R_LOG 13

#define PTS0_SIZE (R_ENTRIES + S_BUCKETS * (S0_SLOTS * S0_SLOTSIZE + 4))
#define PTS1_SIZE (R_ENTRIES + S_BUCKETS * (S1_SLOTS * S1_SLOTSIZE + 4))
#define PAIRSLIST_SIZE 262144

#define CK_BARRIER 3840
#define CK_THREADRESULT0 3904
#define CK_THREADRESULT1 3968
#define CK_SIPSTATE 4032
#define CK_SOLPTR 4072
//#define CK_RAMSIZE0 4080
//#define CK_RAMSIZE1 4084
//#define CK_REN3 4088
#define CK_ABORT 4092

#define CK_BIG0HEAD 4096UL
#define CK_BIG0BODY (CK_BIG0HEAD + THREADS * B_BUCKETS * 4)
#define CK_EDGE1 CK_BIG0BODY
#define CK_BIG1 (CK_BIG0BODY + THREADPLAN0 * THREADS * THREADS * B0_SLOTS * B0_SLOTSIZE)
#define CK_BIG2 (CK_BIG1 + THREADS * B_BUCKETS * (B1_SLOTS * B1_SLOTSIZE + 4) + 65536)
#define CK_BIG3 CK_BIG1
#define CK_PTS1 (CK_BIG2 + THREADS * B_BUCKETS * (B2_SLOTS * B2_SLOTSIZE + 4) + 65536)
#define CK_PTS0 (CK_BIG0BODY + B_BUCKETS * THREADS * B0_SLOTS * B0_SLOTSIZE)
#define CK_PAIRSLIST CK_BIG1
#define CK_PAIRSBITFIELD (CK_PAIRSLIST + PAIRSLIST_SIZE*12)
#define CK_PAIRSINDEX (CK_PAIRSBITFIELD + PAIRSLIST_SIZE/8)
#define CK_CYCLEBUF (CK_PAIRSINDEX + THREADS * PI_THREADSTRIDE)
#define CK_JUNCTIONS (CK_CYCLEBUF + 4096)
#define CK_BIGP CK_BIG2
#define CK_RENAMELIST (CK_BIG2 - 33554432)
#if ((CK_BIG3 + THREADS * B_BUCKETS * (B3_SLOTS * B3_SLOTSIZE + 4)) > CK_RENAMELIST)
    #error "B3_SLOTS too big"
#endif

#define CK_CONTEXT_SIZE (CK_PTS1 + THREADS * PTS1_SIZE)
