#if (defined(AXI_CASE_DUPLICATE_ATTRIBUTES) + defined(AXI_CASE_INVALID_ABOVE_LIMIT) + defined(AXI_CASE_INVALID_CONFLICT) + defined(AXI_CASE_INVALID_MALFORMED) + defined(AXI_CASE_INVALID_NEGATIVE) + defined(AXI_CASE_INVALID_NON_AXI) + defined(AXI_CASE_INVALID_ZERO) + defined(AXI_CASE_LEGACY_NON_AXI_BURST_HINTS)) != 1
#error "Select exactly one fixture case macro"
#endif

#if defined(AXI_CASE_DUPLICATE_ATTRIBUTES)
extern "C" int duplicate_attributes(const int *a, const int *b, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=shared max_read_burst_length=16
#pragma HLS interface mode=m_axi port=b offset=direct bundle=shared max_read_burst_length=16
  return n > 0 ? a[0] + b[0] : 0;
}
#elif defined(AXI_CASE_INVALID_ABOVE_LIMIT)
extern "C" int invalid_above_limit(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=257
  return n > 0 ? a[0] : 0;
}
#elif defined(AXI_CASE_INVALID_CONFLICT)
extern "C" int invalid_conflict(const int *a, const int *b, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=shared max_read_burst_length=8
#pragma HLS interface mode=m_axi port=b offset=direct bundle=shared max_read_burst_length=16
  return n > 0 ? a[0] + b[0] : 0;
}
#elif defined(AXI_CASE_INVALID_MALFORMED)
extern "C" int invalid_malformed(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length="16abc"
  return n > 0 ? a[0] : 0;
}
#elif defined(AXI_CASE_INVALID_NEGATIVE)
extern "C" int invalid_negative(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length="-1"
  return n > 0 ? a[0] : 0;
}
#elif defined(AXI_CASE_INVALID_NON_AXI)
extern "C" int invalid_non_axi(const int *a, int n) {
#pragma HLS interface mode=bram port=a max_read_burst_length=16 read_fifo_depth=8
  return n > 0 ? a[0] : 0;
}
#elif defined(AXI_CASE_INVALID_ZERO)
extern "C" int invalid_zero(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=0
  return n > 0 ? a[0] : 0;
}
#elif defined(AXI_CASE_LEGACY_NON_AXI_BURST_HINTS)
extern "C" int legacy_non_axi_burst_hints(const int input[1], int output[1]) {
#pragma HLS interface mode=ap_memory port=input elem_count=1 max_read_burst_length=16
#pragma HLS interface mode=ap_memory port=output elem_count=1 max_write_burst_length=8
  output[0] = input[0];
  return output[0];
}
#endif
