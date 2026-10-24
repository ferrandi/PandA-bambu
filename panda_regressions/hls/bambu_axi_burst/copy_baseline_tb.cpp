#include <stdio.h>

#define COPY_BASELINE_MAXN 4096
#define COPY_BASELINE_N 1024

extern "C" void kernel(const int src[COPY_BASELINE_MAXN],
                        int dst[COPY_BASELINE_MAXN], int n);

static int source_value(int index) {
  return index * 65537 + 0x13579b;
}

static int destination_canary(int index) {
  return (index * 2246822519u) ^ 0x5a5a0000;
}

int main() {
  static int src[COPY_BASELINE_MAXN];
  static int dst[COPY_BASELINE_MAXN];
  for (int i = 0; i < COPY_BASELINE_MAXN; ++i) {
    src[i] = source_value(i);
    dst[i] = destination_canary(i);
  }

  kernel(src, dst, COPY_BASELINE_N);

  for (int i = 0; i < COPY_BASELINE_N; ++i) {
    if (dst[i] != source_value(i)) {
      fprintf(stderr, "COPY_BASELINE_GOLDEN_FAIL: dst index=%d expected=%08x actual=%08x\n",
              i, (unsigned)source_value(i), (unsigned)dst[i]);
      return 1;
    }
  }
  for (int i = COPY_BASELINE_N; i < COPY_BASELINE_MAXN; ++i) {
    if (dst[i] != destination_canary(i)) {
      fprintf(stderr, "COPY_BASELINE_GOLDEN_FAIL: canary index=%d expected=%08x actual=%08x\n",
              i, (unsigned)destination_canary(i), (unsigned)dst[i]);
      return 1;
    }
  }
  for (int i = 0; i < COPY_BASELINE_MAXN; ++i) {
    if (src[i] != source_value(i)) {
      fprintf(stderr, "COPY_BASELINE_GOLDEN_FAIL: source mutated at index=%d\n", i);
      return 1;
    }
  }

  printf("COPY_BASELINE_GOLDEN_PASS N=%d\n", COPY_BASELINE_N);
  return 0;
}
