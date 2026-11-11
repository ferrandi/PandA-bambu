#define NESTED_READ_WORDS 256

extern "C" int kernel(const int src[NESTED_READ_WORDS], int outer_count) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
  int sum = 0;
  for (int outer = 0; outer < outer_count; ++outer) {
    const int base = outer * 11;
    const unsigned count = static_cast<unsigned>((outer * 3) % 5);
    for (unsigned i = 0; i < count; ++i)
      sum += src[base + static_cast<int>(i)];
  }
  return sum;
}
