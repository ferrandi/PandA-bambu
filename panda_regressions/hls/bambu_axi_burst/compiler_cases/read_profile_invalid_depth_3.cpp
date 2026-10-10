extern "C" int read_profile_invalid_depth_3(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth=3
  return a[0];
}
