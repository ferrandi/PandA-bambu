// Golden-result testbench shared by the T4B sum and scheduling witnesses.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define T4B_MAXN 4096
#ifndef T4B_DEFAULT_N
#define T4B_DEFAULT_N 1024
#endif
#ifndef T4B_SEED
#define T4B_SEED 0x13579bdfu
#endif

#if defined(T4B_FIXTURE_INDEPENDENT)
extern "C" uint32_t first_read_independent_work(const uint32_t *, int, uint32_t);
#elif defined(T4B_FIXTURE_MULXOR)
extern "C" uint32_t first_read_mulxor_work(const uint32_t *, int, uint32_t);
#else
extern "C" int kernel(const int *, int);
#endif

static uint32_t witness_word(uint32_t i) {
  return (i * 0x9e3779b9u) ^ (i << 7) ^ 0xa5a55a5au;
}

static uint32_t expected_q(uint32_t n) {
  uint32_t q = T4B_SEED;
  for (uint32_t i = 0; i < n; ++i) {
#if defined(T4B_FIXTURE_INDEPENDENT)
    uint32_t t = q + i;
    t ^= t << 13;
    t ^= t >> 17;
    t ^= t << 5;
    t += 0x9e3779b9u;
    q = t;
#elif defined(T4B_FIXTURE_MULXOR)
    uint32_t t = q;
    t = (t * T4B_SEED) ^ T4B_SEED;
    t = (t * T4B_SEED) ^ T4B_SEED;
    t = (t * T4B_SEED) ^ T4B_SEED;
    t = (t * T4B_SEED) ^ T4B_SEED;
    q = t;
#else
    (void)i;
#endif
  }
  return q;
}

int main(int argc, char **argv) {
  const int n = argc > 1 ? atoi(argv[1]) : T4B_DEFAULT_N;
  if (n < 0 || n > T4B_MAXN) {
    fprintf(stderr, "T4B_GOLDEN_FAIL: N outside 0..%d: %d\n", T4B_MAXN, n);
    return 2;
  }

#if defined(T4B_FIXTURE_INDEPENDENT) || defined(T4B_FIXTURE_MULXOR)
  static uint32_t data[T4B_MAXN];
  for (int i = 0; i < T4B_MAXN; ++i) data[i] = witness_word((uint32_t)i);
  uint32_t sum = 0;
  for (int i = 0; i < n; ++i) sum += data[i];
  const uint32_t expected = sum ^ expected_q((uint32_t)n);
#if defined(T4B_FIXTURE_INDEPENDENT)
  const uint32_t actual = first_read_independent_work(data, n, T4B_SEED);
#else
  const uint32_t actual = first_read_mulxor_work(data, n, T4B_SEED);
#endif
#else
  static int data[T4B_MAXN];
  for (int i = 0; i < T4B_MAXN; ++i) data[i] = (int)((i * 17 + 3) % 31) - 15;
  int expected = 0;
  for (int i = 0; i < n; ++i) expected += data[i];
  const int actual = kernel(data, n);
#endif

  if (actual != expected) {
    fprintf(stderr, "T4B_GOLDEN_FAIL: N=%d expected=%08x actual=%08x\n",
            n, (unsigned)expected, (unsigned)actual);
    return 1;
  }
  printf("T4B_GOLDEN_PASS N=%d result=%08x\n", n, (unsigned)actual);
  return 0;
}
