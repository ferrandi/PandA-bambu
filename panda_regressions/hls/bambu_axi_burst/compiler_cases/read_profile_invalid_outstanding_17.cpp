extern "C" int read_profile_invalid_outstanding_17(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=17
  return a[0];
}
