// Combine roots with the same m_axi bundle but different read ABIs. The
// defines below deliberately change declaration order: --top-fname ordering
// alone does not change the CallGraphManager's root traversal order.
#if defined(SHARED_PROFILE_B32_FIRST) || defined(SHARED_PROFILE_O_CONFLICT_FIRST) || \
    defined(SHARED_PROFILE_D_CONFLICT_FIRST) || defined(SHARED_PROFILE_DEFAULT_MATCH_FIRST)
extern "C" int configured32(const int *src, int n) {
#if defined(SHARED_PROFILE_O_CONFLICT_FIRST)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=3
#elif defined(SHARED_PROFILE_D_CONFLICT_FIRST)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 read_fifo_depth=8
#elif defined(SHARED_PROFILE_DEFAULT_MATCH_FIRST)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=32
#endif
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#endif

#if defined(SHARED_RESOURCE_LEGACY_FIRST)
#include "unsupported_loops.cpp"
#include "recognized_loops.cpp"
#else
#include "recognized_loops.cpp"
#include "unsupported_loops.cpp"
#endif

#if !defined(SHARED_PROFILE_B32_FIRST) && !defined(SHARED_PROFILE_O_CONFLICT_FIRST) && \
    !defined(SHARED_PROFILE_D_CONFLICT_FIRST) && !defined(SHARED_PROFILE_DEFAULT_MATCH_FIRST)
extern "C" int configured32(const int *src, int n) {
#if defined(SHARED_PROFILE_O_CONFLICT)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=3
#elif defined(SHARED_PROFILE_D_CONFLICT)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 read_fifo_depth=8
#elif defined(SHARED_PROFILE_DEFAULT_MATCH)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=32
#endif
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#endif
