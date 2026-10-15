extern "C" void write_burst_one(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=1
  for (int i = 0; i < n; ++i) dst[i] = i;
}

extern "C" void write_burst_three_default_fifo(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=3
  for (int i = 0; i < n; ++i) dst[i] = i;
}

extern "C" void write_burst_256(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=256
  for (int i = 0; i < n; ++i) dst[i] = i;
}

extern "C" void write_outstanding_one(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = i;
}

extern "C" void write_outstanding_sixteen(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=16 write_fifo_depth=64
  for (int i = 0; i < n; ++i) dst[i] = i;
}

extern "C" void write_duplicate_equal_profile(int *dst, int *unused, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=64
#pragma HLS interface mode=m_axi port=unused offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=64
  for (int i = 0; i < n; ++i) dst[i] = i;
}

