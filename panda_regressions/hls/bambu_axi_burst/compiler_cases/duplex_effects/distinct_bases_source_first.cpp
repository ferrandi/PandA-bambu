extern "C" void distinct_bases_source_first(int * __restrict src, int * __restrict dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  for (int i = 0; i < n; ++i) dst[i] = src[i] + 1;
}
