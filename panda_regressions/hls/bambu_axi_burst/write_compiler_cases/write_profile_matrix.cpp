#if (defined(WRITE_PROFILE_BURST_1) + defined(WRITE_PROFILE_BURST_3) + \
     defined(WRITE_PROFILE_BURST_256) + defined(WRITE_PROFILE_OUTSTANDING_1) + \
     defined(WRITE_PROFILE_OUTSTANDING_2) + defined(WRITE_PROFILE_OUTSTANDING_3) + \
     defined(WRITE_PROFILE_OUTSTANDING_16) + defined(WRITE_PROFILE_OUTSTANDING_DEFAULT_BURST) + \
     defined(WRITE_PROFILE_FIFO_DEFAULT_BURST) + defined(WRITE_PROFILE_DUPLICATE_EQUAL) + \
     defined(WRITE_PROFILE_VALID) + defined(WRITE_PROFILE_INVALID_BURST_ZERO) + \
     defined(WRITE_PROFILE_INVALID_BURST_257) + defined(WRITE_PROFILE_INVALID_OUTSTANDING_ZERO) + \
     defined(WRITE_PROFILE_INVALID_OUTSTANDING_17) + defined(WRITE_PROFILE_INVALID_FIFO_4097) + \
     defined(WRITE_PROFILE_INVALID_FIFO_48) + defined(WRITE_PROFILE_INVALID_FIFO_8) + \
     defined(WRITE_PROFILE_INVALID_NON_M_AXI) + defined(WRITE_PROFILE_CONFLICTING_BURST) + \
     defined(WRITE_PROFILE_CONFLICTING_FIFO)) != 1
#error "Select exactly one write profile case macro"
#endif

#if defined(WRITE_PROFILE_BURST_1)
extern "C" void write_burst_one(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=1
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_BURST_3)
extern "C" void write_burst_three_default_fifo(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=3
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_BURST_256)
extern "C" void write_burst_256(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=256
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_OUTSTANDING_1)
extern "C" void write_outstanding_one(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_OUTSTANDING_2)
extern "C" void outstanding_two_profile(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=2
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_OUTSTANDING_3)
extern "C" void write_outstanding_three(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=3
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_OUTSTANDING_16)
extern "C" void write_outstanding_sixteen(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=16 write_fifo_depth=64
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_OUTSTANDING_DEFAULT_BURST)
extern "C" void write_outstanding_with_default_burst(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_FIFO_DEFAULT_BURST)
extern "C" void write_fifo_with_default_burst(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data write_fifo_depth=64
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_DUPLICATE_EQUAL)
extern "C" void write_duplicate_equal_profile(int *dst, int *unused, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=64
#pragma HLS interface mode=m_axi port=unused offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=64
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_VALID)
extern "C" void valid_write_profile(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=64
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_INVALID_BURST_ZERO)
extern "C" void invalid_write_profile(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=0
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_INVALID_BURST_257)
extern "C" void write_burst_257(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=257
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_INVALID_OUTSTANDING_ZERO)
extern "C" void write_outstanding_zero(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=0
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_INVALID_OUTSTANDING_17)
extern "C" void invalid_write_outstanding(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=17
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_INVALID_FIFO_4097)
extern "C" void write_fifo_out_of_range(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_fifo_depth=4097
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_INVALID_FIFO_48)
extern "C" void invalid_write_fifo_non_power_two(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_fifo_depth=48
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_INVALID_FIFO_8)
extern "C" void invalid_write_fifo_too_shallow(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_fifo_depth=8
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_INVALID_NON_M_AXI)
extern "C" void write_attributes_non_m_axi(int *dst, int n) {
#pragma HLS interface mode=array port=dst offset=direct bundle=data write_fifo_depth=64
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(WRITE_PROFILE_CONFLICTING_BURST)
extern "C" void conflicting_write_burst_profile(int *left, int *right, int n) {
#pragma HLS interface mode=m_axi port=left offset=direct bundle=data max_write_burst_length=16
#pragma HLS interface mode=m_axi port=right offset=direct bundle=data max_write_burst_length=32
  for (int i = 0; i < n; ++i) { left[i] = i; right[i] = i; }
}
#elif defined(WRITE_PROFILE_CONFLICTING_FIFO)
extern "C" void conflicting_write_fifo_profile(int *left, int *right, int n) {
#pragma HLS interface mode=m_axi port=left offset=direct bundle=data max_write_burst_length=16 write_fifo_depth=32
#pragma HLS interface mode=m_axi port=right offset=direct bundle=data max_write_burst_length=16 write_fifo_depth=64
  for (int i = 0; i < n; ++i) { left[i] = i; right[i] = i; }
}
#endif
