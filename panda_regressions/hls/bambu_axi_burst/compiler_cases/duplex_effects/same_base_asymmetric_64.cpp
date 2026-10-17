extern "C" void same_base_asymmetric_64(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=8 num_read_outstanding=2 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  for (int i = 0; i < n; ++i) {
    const int value = data[i];
    data[i] = value * 3 + 1;
  }
}

extern "C" void unrelated_architectureless_root(int *data, int n) {
  data[0] = n;
}
