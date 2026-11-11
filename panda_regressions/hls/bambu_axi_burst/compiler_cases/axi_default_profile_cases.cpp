#if (defined(AXI_DEFAULT_CASE_READ) + defined(AXI_DEFAULT_CASE_WRITE) + \
     defined(AXI_DEFAULT_CASE_DUPLEX) + defined(AXI_DEFAULT_CASE_STRIDE) + \
     defined(AXI_DEFAULT_CASE_CACHE) + defined(AXI_DEFAULT_CASE_NESTED) + \
     defined(AXI_DEFAULT_CASE_NO_ACCESS)) != 1
#error "Select exactly one default AXI burst profile case"
#endif

#if (defined(AXI_DEFAULT_READ_OVERRIDE) + defined(AXI_DEFAULT_READ_HINT_OUTSTANDING) + \
     defined(AXI_DEFAULT_READ_HINT_FIFO) + defined(AXI_DEFAULT_READ_HINT_BOTH)) > 1
#error "Select at most one read profile variation"
#endif
#if (defined(AXI_DEFAULT_WRITE_OVERRIDE) + defined(AXI_DEFAULT_WRITE_HINT_OUTSTANDING) + \
     defined(AXI_DEFAULT_WRITE_HINT_FIFO) + defined(AXI_DEFAULT_WRITE_HINT_BOTH)) > 1
#error "Select at most one write profile variation"
#endif
#if (defined(AXI_DEFAULT_DUPLEX_EXPLICIT_O1) + defined(AXI_DEFAULT_DUPLEX_READ_HINT) + \
     defined(AXI_DEFAULT_DUPLEX_WRITE_HINT)) > 1
#error "Select at most one duplex profile variation"
#endif

#if defined(AXI_DEFAULT_CASE_READ)
extern "C" int read_only(const int *src, int n) {
#if defined(AXI_DEFAULT_READ_OVERRIDE)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=8 num_read_outstanding=3
#elif defined(AXI_DEFAULT_READ_HINT_OUTSTANDING)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data num_read_outstanding=3
#elif defined(AXI_DEFAULT_READ_HINT_FIFO)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data read_fifo_depth=64
#elif defined(AXI_DEFAULT_READ_HINT_BOTH)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data num_read_outstanding=3 read_fifo_depth=64
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data
#endif
  int sum = 0;
  for (int i = 0; i < n; ++i)
    sum += src[i];
  return sum;
}
#elif defined(AXI_DEFAULT_CASE_WRITE)
extern "C" int write_only(int *dst, int n) {
#if defined(AXI_DEFAULT_WRITE_OVERRIDE)
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=8 num_write_outstanding=3
#elif defined(AXI_DEFAULT_WRITE_HINT_OUTSTANDING)
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data write_burst_outstanding=3
#elif defined(AXI_DEFAULT_WRITE_HINT_FIFO)
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data write_fifo_depth=64
#elif defined(AXI_DEFAULT_WRITE_HINT_BOTH)
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data write_burst_outstanding=3 write_fifo_depth=64
#else
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data
#endif
  for (int i = 0; i < n; ++i)
    dst[i] = i + 1;
  return n;
}
#elif defined(AXI_DEFAULT_CASE_DUPLEX)
extern "C" int duplex_copy(const int *__restrict src, int *__restrict dst, int n) {
#if defined(AXI_DEFAULT_DUPLEX_EXPLICIT_O1)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 num_write_outstanding=1
#elif defined(AXI_DEFAULT_DUPLEX_READ_HINT)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared num_read_outstanding=3
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared
#elif defined(AXI_DEFAULT_DUPLEX_WRITE_HINT)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared write_fifo_depth=64
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared
#endif
  int sum = 0;
  for (int i = 0; i < n; ++i) {
    dst[i] = src[i];
    sum += src[i];
  }
  return sum;
}
#elif defined(AXI_DEFAULT_CASE_STRIDE)
extern "C" int stride_read(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data
  int sum = 0;
  for (int i = 0; i < n; ++i)
    sum += src[2 * i];
  return sum;
}
#elif defined(AXI_DEFAULT_CASE_CACHE)
extern "C" int cached_read(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data
#pragma HLS cache bundle=data line_count=16 line_size=32 ways=2 rep_policy=lru write_policy=wt
  int sum = 0;
  for (int i = 0; i < n; ++i)
    sum += src[i];
  return sum;
}
#elif defined(AXI_DEFAULT_CASE_NESTED)
extern "C" int nested_read(const int src[256], int outer_count) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data
  int sum = 0;
  for (int outer = 0; outer < outer_count; ++outer) {
    const int base = outer * 11;
    const unsigned count = static_cast<unsigned>((outer * 3) % 5);
    for (unsigned i = 0; i < count; ++i)
      sum += src[base + static_cast<int>(i)];
  }
  return sum;
}
#elif defined(AXI_DEFAULT_CASE_NO_ACCESS)
extern "C" int no_access(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data
  (void)src;
  return n;
}
#endif
