#include <stdio.h>
#include <stdlib.h>

#ifndef DUPLEX_PROFILE
#error "DUPLEX_PROFILE must be defined (1=symmetric, 2=asymmetric)"
#endif

#if DUPLEX_PROFILE != 1 && DUPLEX_PROFILE != 2
#error "DUPLEX_PROFILE must be 1 (symmetric) or 2 (asymmetric)"
#endif

#define DUPLEX_COPY_TEST_WORDS 4096
#ifndef DUPLEX_COPY_TEST_DEFAULT_N
#define DUPLEX_COPY_TEST_DEFAULT_N 1024
#endif

extern "C" void kernel(int *, int *, int);

static int initial_value(int index) {
  return ((index * 37 + 11) % 201) - 100;
}

int main(int argc, char **argv) {
  static int src[DUPLEX_COPY_TEST_WORDS];
  static int dst[DUPLEX_COPY_TEST_WORDS];
  int source_before[DUPLEX_COPY_TEST_WORDS];
  int destination_before[DUPLEX_COPY_TEST_WORDS];
  const int count = argc > 1 ? atoi(argv[1]) : DUPLEX_COPY_TEST_DEFAULT_N;
  if (count < 0 || count > DUPLEX_COPY_TEST_WORDS) {
    fprintf(stderr, "AXI_BURST_DUPLEX_COPY_GOLDEN_FAIL: N outside 0..%d: %d\n",
            DUPLEX_COPY_TEST_WORDS, count);
    return 2;
  }

  for (int index = 0; index < DUPLEX_COPY_TEST_WORDS; ++index) {
    src[index] = initial_value(index);
    dst[index] = initial_value(index + DUPLEX_COPY_TEST_WORDS);
    source_before[index] = src[index];
    destination_before[index] = dst[index];
  }

  kernel(src, dst, count);

  for (int index = 0; index < DUPLEX_COPY_TEST_WORDS; ++index) {
    const int expected = index < count ? source_before[index] : destination_before[index];
    if (src[index] != source_before[index] || dst[index] != expected) {
      fprintf(stderr,
              "AXI_BURST_DUPLEX_COPY_GOLDEN_FAIL profile=%d n=%d index=%d src=%08x/%08x dst=%08x/%08x\n",
              DUPLEX_PROFILE, count, index, (unsigned)src[index],
              (unsigned)source_before[index], (unsigned)dst[index],
              (unsigned)expected);
      return 1;
    }
  }

  printf("AXI_BURST_DUPLEX_COPY_GOLDEN_PASS profile=%d N=%d\n", DUPLEX_PROFILE, count);
  return 0;
}
