#include <stdio.h>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#define NESTED_READ_WORDS 256
#define NESTED_READ_OUTER 8

extern "C" int kernel(const int src[NESTED_READ_WORDS], int outer_count);

int main() {
  static int data[NESTED_READ_WORDS];
  static int original[NESTED_READ_WORDS];
  for (int i = 0; i < NESTED_READ_WORDS; ++i)
    data[i] = original[i] = (i * 17 + 3) % 31 - 15;

  int expected = 0;
  for (int outer = 0; outer < NESTED_READ_OUTER; ++outer) {
    const int base = outer * 11;
    const unsigned count = static_cast<unsigned>((outer * 3) % 5);
    for (unsigned i = 0; i < count; ++i)
      expected += data[base + static_cast<int>(i)];
  }

#ifdef __BAMBU_SIM__
  m_param_alloc(0, sizeof(data));
#endif
  const int actual = kernel(data, NESTED_READ_OUTER);
  if (actual != expected) {
    fprintf(stderr, "NESTED_READ_GOLDEN_FAIL expected=%d actual=%d\n", expected, actual);
    return 1;
  }
  for (int i = 0; i < NESTED_READ_WORDS; ++i) {
    if (data[i] != original[i]) {
      fprintf(stderr, "NESTED_READ_CANARY_FAIL index=%d expected=%d actual=%d\n",
              i, original[i], data[i]);
      return 1;
    }
  }
  printf("NESTED_READ_GOLDEN_PASS OUTER=%d ZERO_TRIP=1 BASES=VARIABLE COUNTS=VARIABLE CANARY=PASS\n",
         NESTED_READ_OUTER);
  return 0;
}
