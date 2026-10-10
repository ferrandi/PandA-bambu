extern "C" int read_profile_invalid_huge(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=999999999999999999999999999999999999999999999999999999
  return a[0];
}
