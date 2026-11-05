#ifndef DUPLEX_PROFILE
#error "DUPLEX_PROFILE must be defined (1=symmetric, 2=asymmetric)"
#endif

extern "C" void kernel(int * __restrict src, int * __restrict dst,
                        unsigned int read_bound, unsigned int write_bound,
                        unsigned int n) {
#if DUPLEX_PROFILE == 1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#elif DUPLEX_PROFILE == 2
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=2 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=8 write_burst_outstanding=1 write_fifo_depth=16
#else
#error "DUPLEX_PROFILE must be 1 (symmetric) or 2 (asymmetric)"
#endif
  int value = 0x5a17c0de;
  for (unsigned int i = 0; i < n; ++i) {
    if (i < read_bound)
      value = src[i];
    if (i < write_bound)
      dst[i] = value;
  }
}
