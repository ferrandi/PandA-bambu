extern "C" void write_fifo_with_default_burst(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data write_fifo_depth=64
  for (int i = 0; i < n; ++i) dst[i] = i;
}
