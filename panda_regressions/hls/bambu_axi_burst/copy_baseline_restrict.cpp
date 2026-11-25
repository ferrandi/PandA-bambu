// Restricted counterpart of the two-bundle copy baseline (P1 review item).
//
// copy_baseline.cpp profile 1 declares the same two distinct m_axi bundles but its signature
// carries no __restrict, so the disjointness of the two ranges is not proven and the compiler keeps
// the scalar interface there. This fixture is the positive half of that A/B: with the caller's
// restrict contract on both pointers the same pragmas activate the read and the write burst engine.
#ifndef COPY_BASELINE_RESTRICT_PROFILE
#define COPY_BASELINE_RESTRICT_PROFILE 1
#endif

#define COPY_BASELINE_MAXN 4096

extern "C" void kernel(const int *__restrict src, int *__restrict dst, int n) {
#if COPY_BASELINE_RESTRICT_PROFILE == 1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#elif COPY_BASELINE_RESTRICT_PROFILE == 2
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output
#else
#error unsupported COPY_BASELINE_RESTRICT_PROFILE
#endif
  for (int i = 0; i < n; ++i) {
    dst[i] = src[i];
  }
}
