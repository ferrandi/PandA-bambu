extern "C" int stride_two(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; i += 2) acc += src[i];
  return acc;
}
