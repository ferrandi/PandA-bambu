extern "C" void invalid_write_profile(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=0
  for (int i = 0; i < n; ++i) dst[i] = i;
}
