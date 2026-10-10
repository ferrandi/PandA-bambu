extern "C" int duplicate_attributes(const int *a, const int *b, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=shared max_read_burst_length=16
#pragma HLS interface mode=m_axi port=b offset=direct bundle=shared max_read_burst_length=16
  return n > 0 ? a[0] + b[0] : 0;
}
