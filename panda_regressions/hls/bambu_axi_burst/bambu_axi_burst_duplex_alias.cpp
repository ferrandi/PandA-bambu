#ifndef DUPLEX_ALIAS_PROFILE
#error "DUPLEX_ALIAS_PROFILE must be defined (1=symmetric, 2=asymmetric)"
#endif

#if DUPLEX_ALIAS_PROFILE != 1 && DUPLEX_ALIAS_PROFILE != 2
#error "DUPLEX_ALIAS_PROFILE must be 1 (symmetric) or 2 (asymmetric)"
#endif

#ifdef DUPLEX_ALIAS_FLOAT_BITS
typedef float duplex_alias_data_t;
#else
typedef int duplex_alias_data_t;
#endif

extern "C" void kernel(duplex_alias_data_t *src, duplex_alias_data_t *dst, int n) {
#if DUPLEX_ALIAS_PROFILE == 1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=2 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=8 write_burst_outstanding=1 write_fifo_depth=16
#endif
#if DUPLEX_ALIAS_CASE == 5
  for (int outer = 0; outer < n; ++outer) {
    duplex_alias_data_t *read_base = src + outer * 8;
    duplex_alias_data_t *write_base = dst + outer * 8;
    const unsigned int inner_count = static_cast<unsigned int>(outer);
    for (unsigned int i = 0; i < inner_count; ++i)
      write_base[i] = read_base[i];
  }
#elif DUPLEX_ALIAS_CASE == 6
  for (int outer = 0; outer < n; ++outer) {
    const unsigned int inner_count = static_cast<unsigned int>(outer);
    for (unsigned int i = 0; i < inner_count; ++i)
      dst[i] = src[i];
  }
#else
  for (int i = 0; i < n; ++i)
    dst[i] = src[i];
#endif
}
