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
