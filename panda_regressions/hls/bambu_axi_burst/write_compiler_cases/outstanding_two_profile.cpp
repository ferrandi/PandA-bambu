extern "C" void outstanding_two_profile(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=2
  for (int i = 0; i < n; ++i) dst[i] = i;
}
