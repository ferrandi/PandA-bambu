#ifndef DUPLEX_ALIAS_PROFILE
#error "DUPLEX_ALIAS_PROFILE must be defined (1=symmetric, 2=asymmetric)"
#endif

#if DUPLEX_ALIAS_PROFILE != 1 && DUPLEX_ALIAS_PROFILE != 2
#error "DUPLEX_ALIAS_PROFILE must be 1 (symmetric) or 2 (asymmetric)"
#endif

extern "C" void kernel(int *src, int *dst, int n) {
#if DUPLEX_ALIAS_PROFILE == 1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=2 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=8 write_burst_outstanding=1 write_fifo_depth=16
#endif
  for (int i = 0; i < n; ++i)
    dst[i] = src[i];
}
