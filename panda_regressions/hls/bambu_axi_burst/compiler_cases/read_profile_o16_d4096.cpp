extern "C" int read_profile_o16_d4096(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=16 read_fifo_depth=4096
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
