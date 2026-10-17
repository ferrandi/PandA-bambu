extern "C" int read_profile_invalid_duplicate_outstanding_range(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=0 num_read_outstanding=1
  return a[0];
}
