#include <stdio.h>
#include <stdlib.h>

#define BURST_TEST_MAXN 4096
#ifndef BURST_TEST_DEFAULT_N
#define BURST_TEST_DEFAULT_N 1024
#endif

extern "C" void kernel(int *, int);

static int expected_value(int index) {
  return (index * 13 + 7) ^ (index >> 2);
}

int main(int argc, char **argv) {
  const int n = argc > 1 ? atoi(argv[1]) : BURST_TEST_DEFAULT_N;
  if (n < 0 || n > BURST_TEST_MAXN) {
    fprintf(stderr, "AXI_BURST_WRITE_GOLDEN_FAIL: N outside 0..%d: %d\n",
            BURST_TEST_MAXN, n);
    return 2;
  }

  static int dst[BURST_TEST_MAXN];
  for (int i = 0; i < BURST_TEST_MAXN; ++i) dst[i] = 0x5a5a0000 ^ i;
  kernel(dst, n);

  for (int i = 0; i < n; ++i) {
    const int expected = expected_value(i);
    if (dst[i] != expected) {
      fprintf(stderr, "AXI_BURST_WRITE_GOLDEN_FAIL: index=%d expected=%08x actual=%08x\n",
              i, (unsigned)expected, (unsigned)dst[i]);
      return 1;
    }
  }
  for (int i = n; i < BURST_TEST_MAXN; ++i) {
    const int canary = 0x5a5a0000 ^ i;
    if (dst[i] != canary) {
      fprintf(stderr, "AXI_BURST_WRITE_GOLDEN_FAIL: canary index=%d expected=%08x actual=%08x\n",
              i, (unsigned)canary, (unsigned)dst[i]);
      return 1;
    }
  }
  printf("AXI_BURST_WRITE_GOLDEN_PASS N=%d\n", n);
  return 0;
}
