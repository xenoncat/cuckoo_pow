#ifndef THREADS
#define THREADS 4
#endif

#if THREADS == 1
    #define THREADPLAN0 130
    #define THREADPLAN1 126
    //B0_SLOTS divisible by 64 to make CK_BIG1 aligned by 64
    #define B0_SLOTS 8399936UL
    #define B1_SLOTS 5313856
    #define B2_SLOTS 2494144
    #define B3_SLOTS 1476928
    #define B7_SLOTS 439360
    //#define B8_SLOTS 336960
    #define BR_SLOTS 336960
    #define BS_SLOTS 336960
    #define BP_SLOTS 336960
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
    #define B0_SLOTS 4202944UL
    #define B1_SLOTS 2660928
    #define B2_SLOTS 1247936
    #define B3_SLOTS 739904
    #define B7_SLOTS 219712
    //#define B8_SLOTS 170048
    #define BR_SLOTS 170048
    #define BS_SLOTS 170048
    #define BP_SLOTS 170048
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
    #define B0_SLOTS 2104896UL
    #define B1_SLOTS 1331904
    #define B2_SLOTS 625984
    #define B3_SLOTS 371904
    #define B7_SLOTS 109888
    //#define B8_SLOTS 86976
    #define BR_SLOTS 86976
    #define BS_SLOTS 86976
    #define BP_SLOTS 86976
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
    #define B0_SLOTS 1055040UL
    #define B1_SLOTS 667968
    #define B2_SLOTS 314944
    #define B3_SLOTS 186944
    #define B7_SLOTS 54976
    //#define B8_SLOTS 44992
    #define BR_SLOTS 44992
    #define BS_SLOTS 44992
    #define BP_SLOTS 44992
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

#define HALFSIZE 0x80000000U
#define LOGHALFSIZE 31
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
#define B7_SLOTSIZE 6
#define BR_SLOTSIZE 6
#define BS_SLOTSIZE 5
#define BP_SLOTSIZE 4
#define B_SLOTPAYLOADSHIFT (LOGHALFSIZE-B_LOG)

#define S_BUCKETS 256UL
#define S_LOG 8
#define S_BUCKETMASK (S_BUCKETS - 1)
#define S0_SLOTS 37056
#define S1_SLOTS 24768
#define S8_SLOTS 20544
#define SR_SLOTS 20544
#define S0_SLOTSIZE 5
#define S1_SLOTSIZE 5
#define S8_SLOTSIZE 6
#define SR_SLOTSIZE 6

#define R_ENTRIES 32768
#define R_LOG 15

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
#define CK_BIG0BODY (CK_BIG0HEAD + THREADS * B_BUCKETS * 8)
#define CK_EDGE1 CK_BIG0BODY
#define CK_BIG1 (CK_BIG0BODY + THREADPLAN0 * THREADS * THREADS * B0_SLOTS * B0_SLOTSIZE)
#define CK_BIG2 (CK_BIG1 + THREADS * B_BUCKETS * (B1_SLOTS * B1_SLOTSIZE + 8) + 65536)
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
