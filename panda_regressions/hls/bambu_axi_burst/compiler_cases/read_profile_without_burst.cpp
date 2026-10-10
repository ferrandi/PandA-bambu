extern "C" int read_profile_without_burst(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data num_read_outstanding=3 read_fifo_depth=8
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
