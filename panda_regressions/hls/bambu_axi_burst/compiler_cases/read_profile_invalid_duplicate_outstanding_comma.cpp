extern "C" int read_profile_invalid_duplicate_outstanding_comma(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=3,4
  return a[0];
}
