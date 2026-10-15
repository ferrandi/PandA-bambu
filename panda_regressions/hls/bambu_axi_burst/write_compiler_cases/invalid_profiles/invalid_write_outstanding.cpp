extern "C" void invalid_write_outstanding(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=17
  for (int i = 0; i < n; ++i) dst[i] = i;
}
