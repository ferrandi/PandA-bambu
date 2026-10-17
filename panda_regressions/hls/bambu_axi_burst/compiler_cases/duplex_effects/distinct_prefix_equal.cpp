typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_prefix_equal(restricted_int_ptr src, restricted_int_ptr dst,
                                      unsigned trip_count, unsigned access_bound) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < trip_count; ++i) {
    if (i < access_bound) value = src[i];
    if (i < access_bound) dst[i] = value;
  }
}
