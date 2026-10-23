#include <cstdio>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
extern "C" size_t __m_param_size(uint8_t);
#endif

#ifndef MDPI_EXTENT_CASE
#error MDPI_EXTENT_CASE must select an extent test case
#endif

#if MDPI_EXTENT_CASE == 1 || MDPI_EXTENT_CASE == 5 || MDPI_EXTENT_CASE == 7
extern "C" void kernel(int *data);
#elif MDPI_EXTENT_CASE == 4
extern "C" void kernel(unsigned char *data);
#elif MDPI_EXTENT_CASE == 2 || MDPI_EXTENT_CASE == 6
extern "C" void kernel(int data[1]);
#elif MDPI_EXTENT_CASE == 3
extern "C" void kernel(unsigned char data[1]);
#elif MDPI_EXTENT_CASE == 8
extern "C" void kernel(int (&data)[4]);
#elif MDPI_EXTENT_CASE == 9 || MDPI_EXTENT_CASE == 10
extern "C" void kernel(int (*data)[4]);
#endif

int main() {
#if MDPI_EXTENT_CASE == 3
  static unsigned char data[1] = {7};
#elif MDPI_EXTENT_CASE == 4
  static unsigned char data[38];
  for (size_t i = 0; i < sizeof(data); ++i) {
    data[i] = static_cast<unsigned char>(0x40 + i);
  }
#elif MDPI_EXTENT_CASE == 6
  static int data[2] = {7, 9};
#elif MDPI_EXTENT_CASE == 8
  static int data[4] = {7, 9, 11, 13};
#elif MDPI_EXTENT_CASE == 9 || MDPI_EXTENT_CASE == 10
  static int data[2][4] = {{1, 2, 3, 4}, {5, 6, 7, 8}};
#else
  static int data[1] = {7};
#endif

#if MDPI_EXTENT_CASE == 4
  m_param_alloc(0, sizeof(data));
#endif

  kernel(data);
#if MDPI_EXTENT_CASE == 4
  for (size_t i = 0; i < sizeof(data); ++i) {
    const auto expected = static_cast<unsigned char>(0x40 + i + (i == 0 ? 1 : 0));
    if (data[i] != expected) {
      std::fprintf(stderr, "MDPI_EXTENT_GOLDEN_FAIL CASE=%d index=%zu value=%u expected=%u\n", MDPI_EXTENT_CASE,
                   i, static_cast<unsigned>(data[i]), static_cast<unsigned>(expected));
      return 1;
    }
  }
#elif MDPI_EXTENT_CASE == 8
  static const int expected[4] = {7, 9, 11, 14};
  for (size_t i = 0; i < 4; ++i) {
    if (data[i] != expected[i]) {
      std::fprintf(stderr, "MDPI_EXTENT_GOLDEN_FAIL CASE=%d index=%zu value=%d expected=%d\n", MDPI_EXTENT_CASE,
                   i, data[i], expected[i]);
      return 1;
    }
  }
#elif MDPI_EXTENT_CASE == 9 || MDPI_EXTENT_CASE == 10
  static const int expected[2][4] = {{1, 2, 3, 4}, {5, 6, 7, 9}};
  for (size_t row = 0; row < 2; ++row) {
    for (size_t column = 0; column < 4; ++column) {
      if (data[row][column] != expected[row][column]) {
        std::fprintf(stderr, "MDPI_EXTENT_GOLDEN_FAIL CASE=%d row=%zu column=%zu value=%d expected=%d\n",
                     MDPI_EXTENT_CASE, row, column, data[row][column], expected[row][column]);
        return 1;
      }
    }
  }
#else
  if (data[0] != 8) {
    std::fprintf(stderr, "MDPI_EXTENT_GOLDEN_FAIL CASE=%d value=%u\n", MDPI_EXTENT_CASE,
                 static_cast<unsigned>(data[0]));
    return 1;
  }
#endif
#ifdef __BAMBU_SIM__
  const size_t expected_size = MDPI_EXTENT_CASE == 1 ? 4 :
                               MDPI_EXTENT_CASE == 3 ? 1 : MDPI_EXTENT_CASE == 4 ? 38 :
                               MDPI_EXTENT_CASE == 6 ? 8 : MDPI_EXTENT_CASE == 8 ? 16 :
                               MDPI_EXTENT_CASE == 10 ? 32 : 4;
  if (__m_param_size(0) != expected_size) {
    std::fprintf(stderr, "MDPI_EXTENT_GOLDEN_FAIL CASE=%d size=%zu expected=%zu\n", MDPI_EXTENT_CASE,
                 __m_param_size(0), expected_size);
    return 1;
  }
#if MDPI_EXTENT_CASE == 6
  if (data[1] != 9) {
    std::fprintf(stderr, "MDPI_EXTENT_GOLDEN_FAIL CASE=%d tail=%d\n", MDPI_EXTENT_CASE, data[1]);
    return 1;
  }
#endif
#endif
  std::printf("MDPI_EXTENT_GOLDEN_PASS CASE=%d\n", MDPI_EXTENT_CASE);
  return 0;
}
