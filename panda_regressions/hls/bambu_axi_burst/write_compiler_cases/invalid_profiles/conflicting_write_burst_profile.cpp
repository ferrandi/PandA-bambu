extern "C" void conflicting_write_burst_profile(int *left, int *right, int n) {
#pragma HLS interface mode=m_axi port=left offset=direct bundle=data max_write_burst_length=16
#pragma HLS interface mode=m_axi port=right offset=direct bundle=data max_write_burst_length=32
  for (int i = 0; i < n; ++i) { left[i] = i; right[i] = i; }
}
