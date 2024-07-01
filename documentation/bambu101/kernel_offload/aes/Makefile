BAMBU?=bambu

CFLAGS?=-O3 -m32 -Wall -Wno-unused-label
TOP_FNAME?=aes256_encrypt_ecb
BAMBU_FLAGS?=--compiler=I386_GCC49 -O1 -m32 --generate-interface=INFER --generate-tb=aes_test.c --simulate

BAMBU_FLAGS_EXTRA=--top-fname=$(TOP_FNAME) --simulator=VERILATOR --verilator-parallel=4 -v4 --print-dot --no-clean
# BAMBU_FLAGS_EXTRA+= -fno-unroll-loops
# BAMBU_FLAGS_EXTRA+= --enable-function-proxy
# BAMBU_FLAGS_EXTRA+= -fno-inline-functions -fno-inline --panda-parameter=function-opt=0

CFLAGS_EXTRA=-I../common
LD_FLAGS_EXTRA=-Wl,-rpath,.

SRCS=local_support.c ../common/support.c

KERN_SRCS=aes.c
KERN_LIB=aes.so

FILES=$(SRCS) $(KERN_LIB) aes.h ../common/support.h

.PHONY: run hls clean clean-hls

$(KERN_LIB): $(KERN_SRCS) aes.h
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -shared -fPIC -o $@ $<

testbench: aes_test.c $(KERN_LIB)
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) $(LD_FLAGS_EXTRA) -o $@ $^

aes: $(FILES) ../common/harness.c
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) $(LD_FLAGS_EXTRA) -o aes $(SRCS) $(KERN_LIB) ../common/harness.c

run: aes input.data check.data
	./aes input.data check.data

generate: $(FILES) generate.c
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) -o generate $(SRCS) generate.c
	./generate

hls: $(KERN_SRCS) aes.h
	$(BAMBU) $(BAMBU_FLAGS) $(BAMBU_FLAGS_EXTRA) $(CFLAGS_EXTRA) $(KERN_SRCS)

clean-hls:
	rm -rf HLS_output panda-temp $(TOP_FNAME).v *.mem memory_allocation_* simulate_* synthesize_* results.txt transcript

clean: clean-hls
	rm -f $(KERN_LIB) aes testbench generate output.data
