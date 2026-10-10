#if defined(LATENCY_CASE_LATENCY_OMITTED)
extern "C" int latency_omitted(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i];
  return sum;
}
#endif

#if defined(LATENCY_CASE_LATENCY_64)
extern "C" int latency_64(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16 latency=64
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i];
  return sum;
}
#endif

#if defined(LATENCY_CASE_LATENCY_LEADING_ZEROES)
extern "C" int latency_leading_zeroes(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16 latency=00064
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i];
  return sum;
}
#endif

#if defined(LATENCY_CASE_LATENCY_UINT32_MAX)
extern "C" int latency_uint32_max(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16 latency=4294967295
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i];
  return sum;
}
#endif

#if defined(LATENCY_CASE_LATENCY_DUPLICATE_SAME)
extern "C" int latency_duplicate_same(const int *a, const int *b, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16 latency=64
#pragma HLS interface mode=m_axi port=b offset=direct bundle=data max_read_burst_length=16 latency=00064
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i] + b[i];
  return sum;
}
#endif

#if defined(LATENCY_REVERSE_ROOT_ORDER)
#if defined(LATENCY_CASE_LATENCY_ZERO)
extern "C" int latency_zero(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16 latency=0
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i];
  return sum;
}
#endif
#if defined(LATENCY_CASE_LATENCY_SHARED_DEFAULT_FIRST)
extern "C" int latency_shared_default_first(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i];
  return sum;
}
#endif
#else
#if defined(LATENCY_CASE_LATENCY_SHARED_DEFAULT_FIRST)
extern "C" int latency_shared_default_first(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i];
  return sum;
}
#endif
#if defined(LATENCY_CASE_LATENCY_ZERO)
extern "C" int latency_zero(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16 latency=0
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i];
  return sum;
}
#endif
#endif

#if defined(LATENCY_CASE_LATENCY_SHARED_ZERO_FIRST)
extern "C" int latency_shared_zero_first(const int *a, const int *b, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16 latency=0
#pragma HLS interface mode=m_axi port=b offset=direct bundle=data max_read_burst_length=16
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i] + b[i];
  return sum;
}
#endif

#if defined(LATENCY_CASE_LATENCY_SHARED_DEFAULT_CONFLICT)
extern "C" int latency_shared_default_conflict(const int *a, const int *b, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16
#pragma HLS interface mode=m_axi port=b offset=direct bundle=data max_read_burst_length=16 latency=64
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i] + b[i];
  return sum;
}
#endif

#if defined(LATENCY_CASE_LATENCY_SHARED_EXPLICIT_CONFLICT)
extern "C" int latency_shared_explicit_conflict(const int *a, const int *b, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16 latency=64
#pragma HLS interface mode=m_axi port=b offset=direct bundle=data max_read_burst_length=16 latency=128
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i] + b[i];
  return sum;
}
#endif

#if defined(LATENCY_CASE_LATENCY_NON_AXI)
extern "C" int latency_non_axi(const int a[16], int n) {
#pragma HLS interface mode=ap_memory port=a elem_count=16 latency=64
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i];
  return sum;
}
#endif

#if defined(LATENCY_CASE_LATENCY_NON_AXI_BASELINE)
extern "C" int latency_non_axi_baseline(const int a[16], int n) {
#pragma HLS interface mode=ap_memory port=a elem_count=16
  int sum = 0;
  for(int i = 0; i < n; ++i) sum += a[i];
  return sum;
}
#endif
