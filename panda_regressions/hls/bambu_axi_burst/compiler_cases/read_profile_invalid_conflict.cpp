extern "C" int read_profile_invalid_conflict(const int *a, const int *b) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=3
#pragma HLS interface mode=m_axi port=b bundle=data num_read_outstanding=4
  return a[0] + b[0];
}
