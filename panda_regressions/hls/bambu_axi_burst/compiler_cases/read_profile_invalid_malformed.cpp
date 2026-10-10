extern "C" int read_profile_invalid_malformed(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth="8beats"
  return a[0];
}
