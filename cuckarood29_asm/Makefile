MAINOBJ := cuckarood29.o benchmark.o
OBJ     := blake2b.o unrename.o findcycles.o
ASMOBJ  := sipseed0.o round1.o round2.o round3.o round4.o round14.o round15.o round16.o round17.o round40.o roundpinit.o roundp0.o roundp1.o roundpr0.o roundpr1.o siprecover.o

all: t4/cuckarood29 t8/cuckarood29 t2/cuckarood29 t1/cuckarood29 t4/benchmark t8/benchmark t2/benchmark t1/benchmark

t1/cuckarood29: $(addprefix t1/, cuckarood29.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

t1/benchmark: $(addprefix t1/, benchmark.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

$(addprefix t1/, $(MAINOBJ) $(OBJ)): t1/%.o : %.c memlayout.h | t1/
	gcc -o $@ -c -std=gnu11 -DTHREADS=1 -O3 -march=native -masm=intel -mno-vzeroupper -fno-ident -fno-asynchronous-unwind-tables -Wall $<

$(addprefix t1/, $(ASMOBJ)): t1/%.o : %.S memlayout.h | t1/
	gcc -o $@ -c -DTHREADS=1 $<


t2/cuckarood29: $(addprefix t2/, cuckarood29.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

t2/benchmark: $(addprefix t2/, benchmark.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

$(addprefix t2/, $(MAINOBJ) $(OBJ)): t2/%.o : %.c memlayout.h | t2/
	gcc -o $@ -c -std=gnu11 -DTHREADS=2 -O3 -march=native -masm=intel -mno-vzeroupper -fno-ident -fno-asynchronous-unwind-tables -Wall $<

$(addprefix t2/, $(ASMOBJ)): t2/%.o : %.S memlayout.h | t2/
	gcc -o $@ -c -DTHREADS=2 $<


t4/cuckarood29: $(addprefix t4/, cuckarood29.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

t4/benchmark: $(addprefix t4/, benchmark.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

$(addprefix t4/, $(MAINOBJ) $(OBJ)): t4/%.o : %.c memlayout.h | t4/
	gcc -o $@ -c -std=gnu11 -DTHREADS=4 -O3 -march=native -masm=intel -mno-vzeroupper -fno-ident -fno-asynchronous-unwind-tables -Wall $<

$(addprefix t4/, $(ASMOBJ)): t4/%.o : %.S memlayout.h | t4/
	gcc -o $@ -c -DTHREADS=4 $<


t8/cuckarood29: $(addprefix t8/, cuckarood29.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

t8/benchmark: $(addprefix t8/, benchmark.o $(OBJ) $(ASMOBJ))
	gcc -o $@ $^ -lpthread -Xlinker --strip-all

$(addprefix t8/, $(MAINOBJ) $(OBJ)): t8/%.o : %.c memlayout.h | t8/
	gcc -o $@ -c -std=gnu11 -DTHREADS=8 -O3 -march=native -masm=intel -mno-vzeroupper -fno-ident -fno-asynchronous-unwind-tables -Wall $<

$(addprefix t8/, $(ASMOBJ)): t8/%.o : %.S memlayout.h | t8/
	gcc -o $@ -c -DTHREADS=8 $<

t1/ t2/ t4/ t8/:
	mkdir -p t1 t2 t4 t8

.PHONY: clean
clean:
	rm -rf t1 t2 t4 t8
