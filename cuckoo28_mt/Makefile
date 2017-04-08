all: cuckoo28_1t cuckoo28_2t cuckoo28_4t cuckoo28_8t

cuckoo28_1t: cuckoo28_mt.c memlayout28_mt.h siphash_avx2.h
	gcc -o cuckoo28_1t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=1 cuckoo28_mt.c

cuckoo28_2t: cuckoo28_mt.c memlayout28_mt.h siphash_avx2.h
	gcc -o cuckoo28_2t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=2 cuckoo28_mt.c

cuckoo28_4t: cuckoo28_mt.c memlayout28_mt.h siphash_avx2.h
	gcc -o cuckoo28_4t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=4 cuckoo28_mt.c

cuckoo28_8t: cuckoo28_mt.c memlayout28_mt.h siphash_avx2.h
	gcc -o cuckoo28_8t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=8 cuckoo28_mt.c
