#ifndef DUPLEX_INDEPENDENT_PROFILE
#error "DUPLEX_INDEPENDENT_PROFILE must be defined (1=symmetric, 2=asymmetric)"
#endif

extern "C" unsigned bambu_axi_burst_duplex_independent(
    unsigned * __restrict src, unsigned * __restrict dst, unsigned n) {
#if DUPLEX_INDEPENDENT_PROFILE == 1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#elif DUPLEX_INDEPENDENT_PROFILE == 2
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=2 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=8 write_burst_outstanding=1 write_fifo_depth=16
#else
#error "DUPLEX_INDEPENDENT_PROFILE must be 1 (symmetric) or 2 (asymmetric)"
#endif
  unsigned checksum = 0;
  for (unsigned i = 0; i < n; ++i) {
    checksum += src[i];
    dst[i] = (i * 0x9e3779b9u) ^ 0xa5a55a5au;
  }
  return checksum;
}
