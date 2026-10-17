extern "C" void duplex_write_fifo_conflict(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
  for (int i = 0; i < n; ++i) {
    const int value = data[i];
    data[i] = value * 3 + 1;
  }
}
