all: cuckoo20_st

cuckoo20_st: cuckoo20_st.c memlayout20_st.h siphash_avx2.h
	gcc -o cuckoo20_st -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper cuckoo20_st.c
