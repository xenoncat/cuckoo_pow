all: cuckoo28_st

cuckoo28_st: cuckoo28_st.c memlayout28_st.h siphash_avx2.h
	gcc -o cuckoo28_st -std=gnu11 -mavx2 -O3 -fomit-frame-pointer -mno-vzeroupper cuckoo28_st.c
