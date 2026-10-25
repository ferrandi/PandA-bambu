// Manual F1b compiler fixtures. Each function isolates a write-burst shape.

extern "C" void write_count(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = i;
}

extern "C" void write_count_seed(int *dst, int n, int seed) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = seed + i;
}

extern "C" void write_nonpositive_bound(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = i;
}

// The two bundles address different buffers, and the compiler only accepts that claim when the
// signature states it: without __restrict the ranges may overlap, so the burst engines stay
// inactive and the scalar interface is kept (see cross_bundle_burst_is_disjoint).
extern "C" void copy_distinct_buffers(int *__restrict dst, const int *__restrict src, int *unused, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" void copy_distinct_buffers_reversed(const int *__restrict src, int *__restrict dst, int *unused, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" void copy_distinct_with_unused_write_owner(int *__restrict dst, const int *__restrict src, int *unused,
                                                       int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16
#pragma HLS interface mode=m_axi port=unused offset=direct bundle=output
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

// The unrestricted twin of the accepted pair. The two bundles are still distinct in the source,
// but nothing proves that their ranges are disjoint, so both directions must keep the scalar
// m_axi interface: the read burst would prefetch values that in-order semantics require to
// observe the stores of previous iterations.
extern "C" void copy_unrestricted_buffers(int *dst, const int *src, int *unused, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

// F2: the same shape, with more than one write burst allowed in flight.  The G2 testbench
// monitors the outstanding count against these values instead of assuming one.
extern "C" void write_count_seed_two(int *dst, int n, int seed) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=2 write_fifo_depth=32
  for (int i = 0; i < n; ++i) dst[i] = seed + i;
}

extern "C" void write_count_seed_three(int *dst, int n, int seed) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=3 write_fifo_depth=32
  for (int i = 0; i < n; ++i) dst[i] = seed + i;
}

extern "C" void write_with_unused_same_bundle(int *dst, int *unused, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
#pragma HLS interface mode=m_axi port=unused offset=direct bundle=output
  for (int i = 0; i < n; ++i) dst[i] = i + 7;
}

extern "C" void write_with_unused_same_bundle_reversed(int *unused, int *dst, int n) {
#pragma HLS interface mode=m_axi port=unused offset=direct bundle=output
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = i + 7;
}

extern "C" void has_break(int *dst, int n, int stop) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
  for (int i = 0; i < n; ++i) { if (i == stop) break; dst[i] = i; }
}

extern "C" void early_return(int *dst, int n, int stop) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
  for (int i = 0; i < n; ++i) { if (i == stop) return; dst[i] = i; }
}

extern "C" void conditional_store(int *dst, int n, int enable) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
  for (int i = 0; i < n; ++i) if (enable) dst[i] = i;
}

extern "C" void strided_store(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
  for (int i = 0; i < n; i += 2) dst[i] = i;
}

extern "C" void byte_store(unsigned char *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = (unsigned char)i;
}

extern "C" void halfword_store(unsigned short *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = (unsigned short)i;
}

volatile int write_effect_token;
extern "C" void volatile_effect(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
  for (int i = 0; i < n; ++i) { write_effect_token = i; dst[i] = i; }
}

extern "C" __attribute__((noinline)) int opaque_write_effect(int value) {
  write_effect_token = value;
  return value;
}
extern "C" void call_effect_before_region(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
  int seed = opaque_write_effect(n);
  for (int i = 0; i < n; ++i) dst[i] = seed + i;
}

extern "C" void mixed_recognized_and_unknown(int *dst, int n, int index) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = i;
  dst[index] = 7;
}

extern "C" void cache_write(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
#pragma HLS cache bundle=data line_count=16 line_size=32 ways=2 rep_policy=lru write_policy=wt
  for (int i = 0; i < n; ++i) dst[i] = i;
}

// Same-base pointwise read/write is handled by duplex support. The effect-analysis
// gate verifies the stronger production duplex structure for this source shape.
extern "C" void read_write_same_bundle(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16
  for (int i = 0; i < n; ++i) data[i] = data[i] + 1;
}

extern "C" void read_write_distinct_buffers_same_bundle(int *dst, const int *src, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" void valid_write_profile(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=64
  for (int i = 0; i < n; ++i) dst[i] = i;
}

