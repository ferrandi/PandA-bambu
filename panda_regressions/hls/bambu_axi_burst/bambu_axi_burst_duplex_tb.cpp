#include <stdio.h>
#include <stdlib.h>

#ifndef DUPLEX_PROFILE
#error "DUPLEX_PROFILE must be defined (1=symmetric, 2=asymmetric)"
#endif

#if DUPLEX_PROFILE != 1 && DUPLEX_PROFILE != 2
#error "DUPLEX_PROFILE must be 1 (symmetric) or 2 (asymmetric)"
#endif

#define DUPLEX_TEST_WORDS 4096
#ifndef DUPLEX_TEST_DEFAULT_N
#define DUPLEX_TEST_DEFAULT_N 1024
#endif

extern "C" void kernel(int *, int);

static int initial_value(int index) {
  return ((index * 37 + 11) % 201) - 100;
}

int main(int argc, char **argv) {
  static int data[DUPLEX_TEST_WORDS];
  const int count = argc > 1 ? atoi(argv[1]) : DUPLEX_TEST_DEFAULT_N;
  if (count < 0 || count > DUPLEX_TEST_WORDS) {
    fprintf(stderr, "AXI_BURST_DUPLEX_GOLDEN_FAIL: N outside 0..%d: %d\n",
            DUPLEX_TEST_WORDS, count);
    return 2;
  }

  for (int index = 0; index < DUPLEX_TEST_WORDS; ++index)
    data[index] = initial_value(index);

  kernel(data, count);

  for (int index = 0; index < DUPLEX_TEST_WORDS; ++index) {
    const int expected = index < count ? initial_value(index) * 3 + 1 : initial_value(index);
    if (data[index] != expected) {
      fprintf(stderr,
              "AXI_BURST_DUPLEX_GOLDEN_FAIL profile=%d n=%d index=%d expected=%08x actual=%08x\n",
              DUPLEX_PROFILE, count, index, (unsigned)expected,
              (unsigned)data[index]);
      return 1;
    }
  }

  printf("AXI_BURST_DUPLEX_GOLDEN_PASS profile=%d N=%d\n", DUPLEX_PROFILE, count);
  return 0;
}
