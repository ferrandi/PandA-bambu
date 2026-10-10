// Golden-result testbench for the classic AXI burst sum regression.
#include <stdio.h>
#include <stdlib.h>

#define BURST_TEST_MAXN 4096
#ifndef BURST_TEST_DEFAULT_N
#define BURST_TEST_DEFAULT_N 1024
#endif

extern "C" int kernel(const int *, int);

int main(int argc, char **argv) {
  const int n = argc > 1 ? atoi(argv[1]) : BURST_TEST_DEFAULT_N;
  if (n < 0 || n > BURST_TEST_MAXN) {
    fprintf(stderr, "AXI_BURST_GOLDEN_FAIL: N outside 0..%d: %d\n", BURST_TEST_MAXN, n);
    return 2;
  }

  static int data[BURST_TEST_MAXN];
  for (int i = 0; i < BURST_TEST_MAXN; ++i) data[i] = (i * 17 + 3) % 31 - 15;
  int expected = 0;
  for (int i = 0; i < n; ++i) expected += data[i];
  const int actual = kernel(data, n);

  if (actual != expected) {
    fprintf(stderr, "AXI_BURST_GOLDEN_FAIL: N=%d expected=%08x actual=%08x\n",
            n, (unsigned)expected, (unsigned)actual);
    return 1;
  }
  printf("AXI_BURST_GOLDEN_PASS N=%d result=%08x\n", n, (unsigned)actual);
  return 0;
}
