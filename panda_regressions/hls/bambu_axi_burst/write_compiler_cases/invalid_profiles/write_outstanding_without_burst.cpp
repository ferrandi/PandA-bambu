extern "C" void write_outstanding_without_burst(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = i;
}
