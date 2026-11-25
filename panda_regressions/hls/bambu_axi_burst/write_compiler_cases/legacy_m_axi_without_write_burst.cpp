extern "C" void scalar_m_axi_without_burst(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data
  for (int i = 0; i < n; ++i) dst[i] = i;
}
