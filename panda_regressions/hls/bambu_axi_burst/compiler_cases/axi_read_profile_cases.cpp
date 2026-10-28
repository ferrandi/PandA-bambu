#if (defined(AXI_CASE_READ_PROFILE_INVALID_CONFLICT) + defined(AXI_CASE_READ_PROFILE_INVALID_DEPTH_3) + defined(AXI_CASE_READ_PROFILE_INVALID_DEPTH_8192) + defined(AXI_CASE_READ_PROFILE_INVALID_DEPTH_ZERO) + defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_DEPTH_COMMA) + defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_DEPTH_CONFLICT) + defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_DEPTH_MISSING) + defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_DEPTH_RANGE) + defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_OUTSTANDING_COMMA) + defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_OUTSTANDING_CONFLICT) + defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_OUTSTANDING_MISSING) + defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_OUTSTANDING_RANGE) + defined(AXI_CASE_READ_PROFILE_INVALID_HUGE) + defined(AXI_CASE_READ_PROFILE_INVALID_MALFORMED) + defined(AXI_CASE_READ_PROFILE_INVALID_NEGATIVE) + defined(AXI_CASE_READ_PROFILE_INVALID_NON_AXI) + defined(AXI_CASE_READ_PROFILE_INVALID_OUTSTANDING_17) + defined(AXI_CASE_READ_PROFILE_INVALID_OUTSTANDING_ZERO) + defined(AXI_CASE_READ_PROFILE_O16_D4096) + defined(AXI_CASE_READ_PROFILE_O1_D256) + defined(AXI_CASE_READ_PROFILE_O3_D32) + defined(AXI_CASE_READ_PROFILE_VALID) + defined(AXI_CASE_READ_PROFILE_WITHOUT_BURST)) != 1
#error "Select exactly one fixture case macro"
#endif

#if defined(AXI_CASE_READ_PROFILE_INVALID_CONFLICT)
extern "C" int read_profile_invalid_conflict(const int *a, const int *b) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=3
#pragma HLS interface mode=m_axi port=b bundle=data num_read_outstanding=4
  return a[0] + b[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DEPTH_3)
extern "C" int read_profile_invalid_depth_3(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth=3
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DEPTH_8192)
extern "C" int read_profile_invalid_depth_8192(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth=8192
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DEPTH_ZERO)
extern "C" int read_profile_invalid_depth_zero(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth=0
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_DEPTH_COMMA)
extern "C" int read_profile_invalid_duplicate_depth_comma(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth=8,16
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_DEPTH_CONFLICT)
extern "C" int read_profile_invalid_duplicate_depth_conflict(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth=8 read_fifo_depth=16
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_DEPTH_MISSING)
extern "C" int read_profile_invalid_duplicate_depth_missing(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth num_read_outstanding=1
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_DEPTH_RANGE)
extern "C" int read_profile_invalid_duplicate_depth_range(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth=3 read_fifo_depth=8
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_OUTSTANDING_COMMA)
extern "C" int read_profile_invalid_duplicate_outstanding_comma(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=3,4
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_OUTSTANDING_CONFLICT)
extern "C" int read_profile_invalid_duplicate_outstanding_conflict(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=3 num_read_outstanding=4
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_OUTSTANDING_MISSING)
extern "C" int read_profile_invalid_duplicate_outstanding_missing(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding read_fifo_depth=8
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_DUPLICATE_OUTSTANDING_RANGE)
extern "C" int read_profile_invalid_duplicate_outstanding_range(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=0 num_read_outstanding=1
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_HUGE)
extern "C" int read_profile_invalid_huge(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=999999999999999999999999999999999999999999999999999999
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_MALFORMED)
extern "C" int read_profile_invalid_malformed(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data read_fifo_depth="8beats"
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_NEGATIVE)
extern "C" int read_profile_invalid_negative(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding="-1"
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_NON_AXI)
extern "C" int read_profile_invalid_non_axi(const int *a) {
#pragma HLS interface mode=bram port=a read_fifo_depth=8
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_OUTSTANDING_17)
extern "C" int read_profile_invalid_outstanding_17(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=17
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_INVALID_OUTSTANDING_ZERO)
extern "C" int read_profile_invalid_outstanding_zero(const int *a) {
#pragma HLS interface mode=m_axi port=a bundle=data num_read_outstanding=0
  return a[0];
}
#elif defined(AXI_CASE_READ_PROFILE_O16_D4096)
extern "C" int read_profile_o16_d4096(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=16 read_fifo_depth=4096
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#elif defined(AXI_CASE_READ_PROFILE_O1_D256)
extern "C" int read_profile_o1_d256(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=256
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#elif defined(AXI_CASE_READ_PROFILE_O3_D32)
extern "C" int read_profile_o3_d32(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=3 read_fifo_depth=32
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#elif defined(AXI_CASE_READ_PROFILE_VALID)
extern "C" int read_profile_valid(const int *a, const int *b, const int *c, const int *d, const int *e,
                                  const int *f, const int *g, const int *h, const int *i, const int *j,
                                  const int *k) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=oa num_read_outstanding=1
#pragma HLS interface mode=m_axi port=b offset=direct bundle=ob num_read_outstanding=3
#pragma HLS interface mode=m_axi port=c offset=direct bundle=oc num_read_outstanding=15
#pragma HLS interface mode=m_axi port=d offset=direct bundle=od num_read_outstanding=16
#pragma HLS interface mode=m_axi port=e offset=direct bundle=oe read_fifo_depth=1
#pragma HLS interface mode=m_axi port=f offset=direct bundle=of read_fifo_depth=8
#pragma HLS interface mode=m_axi port=g offset=direct bundle=og read_fifo_depth=32
#pragma HLS interface mode=m_axi port=h offset=direct bundle=oh read_fifo_depth=256
#pragma HLS interface mode=m_axi port=i offset=direct bundle=oi read_fifo_depth=4096
#pragma HLS interface mode=m_axi port=j offset=direct bundle=oj num_read_outstanding=03 num_read_outstanding=3 read_fifo_depth=0008 read_fifo_depth=8
#pragma HLS interface mode=m_axi port=k offset=direct bundle=oj num_read_outstanding=3 read_fifo_depth=8
  return a[0] + b[0] + c[0] + d[0] + e[0] + f[0] + g[0] + h[0] + i[0] + j[0] + k[0];
}
#elif defined(AXI_CASE_READ_PROFILE_WITHOUT_BURST)
extern "C" int read_profile_without_burst(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data num_read_outstanding=3 read_fifo_depth=8
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#endif
