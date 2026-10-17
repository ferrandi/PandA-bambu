extern "C" int read_profile_invalid_duplicate_depth_missing(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth num_read_outstanding=1
  return a[0];
}
