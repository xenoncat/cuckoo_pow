all: cuckoo32_1t cuckoo32_2t cuckoo32_4t cuckoo32_8t

cuckoo32_1t: cuckoo32_mt.c memlayout32_mt.h siphash_avx2.h
	gcc -o cuckoo32_1t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=1 cuckoo32_mt.c

cuckoo32_2t: cuckoo32_mt.c memlayout32_mt.h siphash_avx2.h
	gcc -o cuckoo32_2t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=2 cuckoo32_mt.c

cuckoo32_4t: cuckoo32_mt.c memlayout32_mt.h siphash_avx2.h
	gcc -o cuckoo32_4t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=4 cuckoo32_mt.c

cuckoo32_8t: cuckoo32_mt.c memlayout32_mt.h siphash_avx2.h
	gcc -o cuckoo32_8t -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper -pthread -DTHREADS=8 cuckoo32_mt.c
