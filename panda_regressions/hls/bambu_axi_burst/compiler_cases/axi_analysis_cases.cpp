#if (defined(AXI_ANALYSIS_CASE_01) + defined(AXI_ANALYSIS_CASE_02) + defined(AXI_ANALYSIS_CASE_03) + defined(AXI_ANALYSIS_CASE_04) + defined(AXI_ANALYSIS_CASE_05) + defined(AXI_ANALYSIS_CASE_06) + defined(AXI_ANALYSIS_CASE_07) + defined(AXI_ANALYSIS_CASE_08) + defined(AXI_ANALYSIS_CASE_09)) != 1
#error "Select exactly one compiler fixture case macro"
#endif

#if defined(AXI_ANALYSIS_CASE_01)
extern int ordinary_global[256];
static volatile int volatile_global[256];

extern "C" void copy_read_first(const int *__restrict src, int *__restrict dst, int *unused, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=z_input max_read_burst_length=16
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=a_output max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" void write_only(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = i;
}

extern "C" int read_with_ordinary_global(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i] + ordinary_global[i];
  return acc;
}

extern "C" int read_with_volatile_global(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i] + volatile_global[i];
  return acc;
}
#elif defined(AXI_ANALYSIS_CASE_02)
// Memory effects and possible aliasing must keep the sequential reads on the
// legacy path even when source and destination use distinct AXI bundles.
extern "C" int store_before_src_first(const int *src, int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=destination
#pragma HLS interface mode=m_axi port=src offset=direct bundle=source max_read_burst_length=16
  dst[0] = 7;
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

// Reverse the C parameter order and the pragma order. The result must not
// depend on InterfaceInfer's parameter enumeration order.
extern "C" int store_before_dst_first(int *dst, const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=source max_read_burst_length=16
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=destination
  dst[0] = 9;
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

// A volatile global access is an explicit effect before the candidate region;
// it must not be hidden by frontend inlining or dead-code elimination.
volatile int effect_token;

extern "C" int volatile_access_before_region(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=source max_read_burst_length=16
  effect_token = n;
  int acc = effect_token;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

// Even a store after the loop conservatively keeps the entire function on the
// legacy path until inter-region memory effects are modeled explicitly.
extern "C" int store_after_region(const int *src, int *dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=source max_read_burst_length=16
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=destination
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  dst[0] = acc;
  return acc;
}

// A write in a separate loop before the candidate read loop is also an
// ordering effect, even though it uses another bundle.
extern "C" int store_in_intermediate_loop(const int *src, int *dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=source max_read_burst_length=16
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=destination
  for (int j = 0; j < 1; ++j) dst[j] = j;
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#elif defined(AXI_ANALYSIS_CASE_03)
// A legal scheduling latency of one remains metadata; the Mantis case supplies
// its memory-model delay separately.
#define MAXN 4096
extern "C" int kernel(const int a[MAXN], int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16 latency=1
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += a[i];
  return acc;
}
#elif defined(AXI_ANALYSIS_CASE_04)
// Compiler-level burst recognizer fixtures. Each exported function isolates a
// source shape; these are ordinary C++ inputs, not expected/generated RTL.
#define MAXN 4096

extern "C" int renamed_sum(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

extern "C" int two_regions(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  for (int j = 0; j < n; ++j) acc += src[j];
  return acc;
}

static int scan_region(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

extern "C" int repeated_regions(const int *src, int n) {
  // Exercise two dynamic entries to the same callee/configured resource.
  int acc = scan_region(src, n);
  acc += scan_region(src, n);
  return acc;
}

extern "C" int zero_count(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

extern "C" int one_count(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

extern "C" int nonmultiple_count(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#elif defined(AXI_ANALYSIS_CASE_05)
extern "C" int cached_region(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
#pragma HLS cache bundle=data line_count=16 line_size=32 ways=2 rep_policy=lru write_policy=wt
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#elif defined(AXI_ANALYSIS_CASE_06)
// Requests outside the initial engine capabilities: IOB cache, transfer width
// other than 32 bits, and a byte-offset/misaligned access.
extern "C" int cached_region(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
#pragma HLS cache bundle=data line_count=16 line_size=32 ways=2 rep_policy=lru write_policy=wt
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

extern "C" int narrow_region(const unsigned char *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

extern "C" int misaligned_region(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  const unsigned char *bytes = reinterpret_cast<const unsigned char *>(src);
  int acc = 0;
  for (int i = 0; i < n; ++i) {
    const int *p = reinterpret_cast<const int *>(bytes + 1 + 4 * i);
    acc += *p;
  }
  return acc;
}
#elif defined(AXI_ANALYSIS_CASE_07)
// Explicit max-burst request on shapes the first recognizer must reject.
extern "C" int stride_two(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; i += 2) acc += src[i];
  return acc;
}

extern "C" int conditional_load(const int *src, int n, int choose) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) if (choose) acc += src[i];
  return acc;
}

extern "C" int early_exit(const int *src, int n, int stop) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) { if (i == stop) break; acc += src[i]; }
  return acc;
}

extern "C" int pointer_chase(const int *const *next, int n) {
#pragma HLS interface mode=m_axi port=next offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  const int *p = next[0];
  for (int i = 0; i < n; ++i) { acc += *p; p = next[i]; }
  return acc;
}

extern "C" void stores_only(int *dst, const int *src, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" int mixed_bundle(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=data max_read_burst_length=16
#pragma HLS interface mode=m_axi port=data offset=direct bundle=data max_write_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) { int v = data[i]; data[i] = v + 1; acc += v; }
  return acc;
}
#elif defined(AXI_ANALYSIS_CASE_08)
extern "C" int stride_two(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; i += 2) acc += src[i];
  return acc;
}
#elif defined(AXI_ANALYSIS_CASE_09)
extern "C" void duplex_write_fifo_conflict(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
  for (int i = 0; i < n; ++i) {
    const int value = data[i];
    data[i] = value * 3 + 1;
  }
}
#endif
