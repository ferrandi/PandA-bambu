extern "C" int read_profile_invalid_negative(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding="-1"
  return a[0];
}
