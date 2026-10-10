extern "C" int read_profile_invalid_duplicate_outstanding_conflict(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=3 num_read_outstanding=4
  return a[0];
}
