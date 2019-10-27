MAINOBJ := cuckatoo31.o benchmark.o
OBJ     := blake2b.o findcycles.o unrename.o siprecover2.o
ASMOBJ  := sipseed0.o sipseed1.o round2.o round3.o round4.o round8.o round9.o round10.o round14.o round15.o round16.o round17.o round18.o round95.o round96.o roundp1.o roundp2.o siprecover.o

all: t4/cuckatoo31 t8/cuckatoo31 t4/benchmark t8/benchmark

t4/cuckatoo31: $(addprefix t4/, cuckatoo31.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

t8/cuckatoo31: $(addprefix t8/, cuckatoo31.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

t4/benchmark: $(addprefix t4/, benchmark.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

t8/benchmark: $(addprefix t8/, benchmark.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

$(addprefix t4/, $(MAINOBJ) $(OBJ)): t4/%.o : %.c memlayout.h | t4/
	gcc -o $@ -c -std=gnu11 -DTHREADS=4 -O3 -march=native -masm=intel -mno-vzeroupper -fno-ident -fno-asynchronous-unwind-tables -Wall $<

$(addprefix t4/, $(ASMOBJ)): t4/%.o : %.S memlayout.h | t4/
	gcc -o $@ -c -DTHREADS=4 $<

$(addprefix t8/, $(MAINOBJ) $(OBJ)): t8/%.o : %.c memlayout.h | t8/
	gcc -o $@ -c -std=gnu11 -DTHREADS=8 -O3 -march=native -masm=intel -mno-vzeroupper -fno-ident -fno-asynchronous-unwind-tables -Wall $<

$(addprefix t8/, $(ASMOBJ)): t8/%.o : %.S memlayout.h | t8/
	gcc -o $@ -c -DTHREADS=8 $<

t4/:
	mkdir -p t4

t8/:
	mkdir -p t8

.PHONY: clean
clean:
	rm -rf t4 t8
