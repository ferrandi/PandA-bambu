#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define BURST_TEST_MAXN 4096
#ifndef BURST_TEST_DEFAULT_N
#define BURST_TEST_DEFAULT_N 1024
#endif

#ifdef BURST_TEST_FLOAT_COPY
#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
extern "C" size_t __m_param_size(uint8_t);
#endif
extern "C" void copy_float_payload(float *__restrict, const float *__restrict, int);

static uint32_t source_bits(int index) {
  static const uint32_t patterns[] = {
      0x00000000u, 0x80000000u, 0x3fc00000u, 0xbfa00000u, 0x7f800000u,
      0xff800000u, 0x7fc12345u, 0x00000001u, 0x7f7fffffu, 0xffc54321u};
  return patterns[index % (sizeof(patterns) / sizeof(patterns[0]))];
}

int main() {
  const int n = 257;
  static float src[BURST_TEST_MAXN];
  static float dst[BURST_TEST_MAXN];
  uint32_t expected[BURST_TEST_MAXN];
  for (int i = 0; i < BURST_TEST_MAXN; ++i) {
    expected[i] = i < n ? source_bits(i) : 0;
    const uint32_t canary = 0x5a5a0000u ^ static_cast<uint32_t>(i);
    const uint32_t input = i < n ? expected[i] : (0x31415926u ^ static_cast<uint32_t>(i));
    memcpy(&src[i], &input, sizeof(input));
    memcpy(&dst[i], &canary, sizeof(canary));
  }
  copy_float_payload(dst, src, n);
#ifdef __BAMBU_SIM__
  if (__m_param_size(0) != BURST_TEST_MAXN * sizeof(float) ||
      __m_param_size(1) != BURST_TEST_MAXN * sizeof(float)) {
    fprintf(stderr, "AXI_BURST_FLOAT_COPY_GOLDEN_FAIL: MDPI extents=%zu,%zu expected=%zu\n",
            __m_param_size(0), __m_param_size(1),
            BURST_TEST_MAXN * sizeof(float));
    return 1;
  }
#endif
  for (int i = 0; i < BURST_TEST_MAXN; ++i) {
    uint32_t actual;
    memcpy(&actual, &dst[i], sizeof(actual));
    const uint32_t wanted = i < n ? expected[i] : (0x5a5a0000u ^ static_cast<uint32_t>(i));
    if (actual != wanted) {
      fprintf(stderr, "AXI_BURST_FLOAT_COPY_GOLDEN_FAIL: index=%d expected=%08x actual=%08x\n",
              i, wanted, actual);
      return 1;
    }
  }
  printf("AXI_BURST_FLOAT_COPY_GOLDEN_PASS N=%d\n", n);
  return 0;
}
#else
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
#endif
