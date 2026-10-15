extern "C" void write_attributes_non_m_axi(int dst[16], int n) {
#pragma HLS interface port=dst mode=array elem_count=16 max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=64
  for (int i = 0; i < n; ++i) dst[i] = i;
}
