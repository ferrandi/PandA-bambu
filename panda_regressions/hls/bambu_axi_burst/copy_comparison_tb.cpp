#include <stdio.h>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#ifndef COPY_COMPARISON_PROFILE
#error COPY_COMPARISON_PROFILE must select a copy comparison profile
#endif

#define COPY_COMPARISON_MAXN 4096
#define COPY_COMPARISON_N 1024

extern "C" void kernel(const int *__restrict src, int *__restrict dst, int n);

static int source_value(int index) {
  return index * 65537 + 0x13579b;
}

static int destination_canary(int index) {
  return (index * 2246822519u) ^ 0x5a5a0000;
}

int main() {
  static int src[COPY_COMPARISON_MAXN];
  static int dst[COPY_COMPARISON_MAXN];
  for (int i = 0; i < COPY_COMPARISON_MAXN; ++i) {
    src[i] = source_value(i);
    dst[i] = destination_canary(i);
  }

#ifdef __BAMBU_SIM__
  m_param_alloc(0, sizeof(src));
  m_param_alloc(1, sizeof(dst));
#endif

  kernel(src, dst, COPY_COMPARISON_N);

  for (int i = 0; i < COPY_COMPARISON_N; ++i) {
    if (dst[i] != source_value(i)) {
      fprintf(stderr, "COPY_COMPARISON_GOLDEN_FAIL: dst index=%d\n", i);
      return 1;
    }
  }
  for (int i = COPY_COMPARISON_N; i < COPY_COMPARISON_MAXN; ++i) {
    if (dst[i] != destination_canary(i)) {
      fprintf(stderr, "COPY_COMPARISON_GOLDEN_FAIL: canary index=%d\n", i);
      return 1;
    }
  }
  for (int i = 0; i < COPY_COMPARISON_MAXN; ++i) {
    if (src[i] != source_value(i)) {
      fprintf(stderr, "COPY_COMPARISON_GOLDEN_FAIL: source mutated at index=%d\n", i);
      return 1;
    }
  }

#if COPY_COMPARISON_PROFILE == 1
  printf("COPY_COMPARISON_GOLDEN_PASS PROFILE=burst-two-bundles N=%d\n", COPY_COMPARISON_N);
#elif COPY_COMPARISON_PROFILE == 2
  printf("COPY_COMPARISON_GOLDEN_PASS PROFILE=legacy-two-bundles N=%d\n", COPY_COMPARISON_N);
#elif COPY_COMPARISON_PROFILE == 3
  printf("COPY_COMPARISON_GOLDEN_PASS PROFILE=legacy-shared N=%d\n", COPY_COMPARISON_N);
#elif COPY_COMPARISON_PROFILE == 4
  printf("COPY_COMPARISON_GOLDEN_PASS PROFILE=duplex-shared N=%d\n", COPY_COMPARISON_N);
#endif
  return 0;
}
