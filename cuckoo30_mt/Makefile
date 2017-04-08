all: cuckoo30_1t cuckoo30_2t cuckoo30_4t cuckoo30_8t

cuckoo30_1t: cuckoo30_mt.c memlayout30_mt.h siphash_avx2.h
	gcc -o cuckoo30_1t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=1 cuckoo30_mt.c

cuckoo30_2t: cuckoo30_mt.c memlayout30_mt.h siphash_avx2.h
	gcc -o cuckoo30_2t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=2 cuckoo30_mt.c

cuckoo30_4t: cuckoo30_mt.c memlayout30_mt.h siphash_avx2.h
	gcc -o cuckoo30_4t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=4 cuckoo30_mt.c

cuckoo30_8t: cuckoo30_mt.c memlayout30_mt.h siphash_avx2.h
	gcc -o cuckoo30_8t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=8 cuckoo30_mt.c
