extern "C" int read_profile_invalid_duplicate_outstanding_missing(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding read_fifo_depth=8
  return a[0];
}
