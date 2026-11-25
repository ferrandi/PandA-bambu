#include <stdint.h>

extern "C" uint32_t first_read_independent_work(const uint32_t *src, int n, uint32_t seed) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=gmem max_read_burst_length=1 num_read_outstanding=1

  uint32_t q = seed;
  uint32_t sum = 0;
  if (n > 0) {
    const uint32_t count = static_cast<uint32_t>(n);
    for (uint32_t i = 0; i < count; ++i) {
      const uint32_t value = src[i];
      uint32_t t = q + i;
      t ^= t << 13;
      t ^= t >> 17;
      t ^= t << 5;
      t += 0x9e3779b9u;
      q = t;
      sum += value;
    }
  }
  return sum ^ q;
}
