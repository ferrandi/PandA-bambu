typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_bases_two_loops(restricted_int_ptr src, restricted_int_ptr dst,
                                        unsigned read_count, unsigned write_count) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < read_count; ++i) value = src[i];
  for (unsigned i = 0; i < write_count; ++i) dst[i] = value;
}
