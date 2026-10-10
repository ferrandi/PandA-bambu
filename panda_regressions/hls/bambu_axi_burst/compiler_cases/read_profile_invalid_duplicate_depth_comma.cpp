extern "C" int read_profile_invalid_duplicate_depth_comma(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth=8,16
  return a[0];
}
