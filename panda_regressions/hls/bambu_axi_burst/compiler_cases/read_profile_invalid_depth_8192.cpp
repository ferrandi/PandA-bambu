extern "C" int read_profile_invalid_depth_8192(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth=8192
  return a[0];
}
