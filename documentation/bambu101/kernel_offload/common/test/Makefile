CFLAGS=-Wall -Werror

test: input_* test_support
	./test_support

test_support: test_support.c ../support.h ../support.c
	cc -o test_support $(CFLAGS) -I.. test_support.c ../support.c

clean:
	rm -f test_support
	rm -rf test_support.dSYM
	rm -f testfile
