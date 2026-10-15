extern "C" void conflicting_write_fifo_profile(int *left, int *right, int n) {
#pragma HLS interface mode=m_axi port=left offset=direct bundle=data max_write_burst_length=16 write_fifo_depth=32
#pragma HLS interface mode=m_axi port=right offset=direct bundle=data max_write_burst_length=16 write_fifo_depth=64
  for (int i = 0; i < n; ++i) { left[i] = i; right[i] = i; }
}
