extern "C" int read_profile_invalid_outstanding_zero(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=0
  return a[0];
}
