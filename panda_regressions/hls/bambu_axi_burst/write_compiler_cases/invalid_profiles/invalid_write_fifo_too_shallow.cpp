extern "C" void invalid_write_fifo_too_shallow(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_fifo_depth=8
  for (int i = 0; i < n; ++i) dst[i] = i;
}
