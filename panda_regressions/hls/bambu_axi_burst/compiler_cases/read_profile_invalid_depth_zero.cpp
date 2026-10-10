extern "C" int read_profile_invalid_depth_zero(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth=0
  return a[0];
}
