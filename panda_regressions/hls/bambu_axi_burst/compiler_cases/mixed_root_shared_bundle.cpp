// Combine roots with the same m_axi bundle but different read ABIs. The
// defines below deliberately change declaration order: --top-fname ordering
// alone does not change the CallGraphManager's root traversal order.
#if defined(SHARED_PROFILE_B32_FIRST) || defined(SHARED_PROFILE_O_CONFLICT_FIRST) || \
    defined(SHARED_PROFILE_D_CONFLICT_FIRST) || defined(SHARED_PROFILE_DEFAULT_MATCH_FIRST) || \
    defined(SHARED_PROFILE_NO_MAX_O3_FIRST) || defined(SHARED_PROFILE_SINGLE_BURST_CONFLICT_FIRST)
extern "C" int configured32(const int *src, int n) {
#if defined(SHARED_PROFILE_O_CONFLICT_FIRST)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=3
#elif defined(SHARED_PROFILE_NO_MAX_O3_FIRST) || defined(SHARED_PROFILE_SINGLE_BURST_CONFLICT_FIRST)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data num_read_outstanding=3
#elif defined(SHARED_PROFILE_D_CONFLICT_FIRST)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 read_fifo_depth=8
#elif defined(SHARED_PROFILE_DEFAULT_MATCH_FIRST)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=16 read_fifo_depth=256
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=32
#endif
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#endif

#if defined(SHARED_RESOURCE_LEGACY_FIRST)
#define AXI_ANALYSIS_CASE_07
#include "axi_analysis_cases.cpp"
#undef AXI_ANALYSIS_CASE_07
#define AXI_ANALYSIS_CASE_04
#include "axi_analysis_cases.cpp"
#undef AXI_ANALYSIS_CASE_04
#else
#define AXI_ANALYSIS_CASE_04
#include "axi_analysis_cases.cpp"
#undef AXI_ANALYSIS_CASE_04
#define AXI_ANALYSIS_CASE_07
#include "axi_analysis_cases.cpp"
#undef AXI_ANALYSIS_CASE_07
#endif

#if !defined(SHARED_PROFILE_B32_FIRST) && !defined(SHARED_PROFILE_O_CONFLICT_FIRST) && \
    !defined(SHARED_PROFILE_D_CONFLICT_FIRST) && !defined(SHARED_PROFILE_DEFAULT_MATCH_FIRST) && \
    !defined(SHARED_PROFILE_NO_MAX_O3_FIRST) && !defined(SHARED_PROFILE_SINGLE_BURST_CONFLICT_FIRST)
extern "C" int configured32(const int *src, int n) {
#if defined(SHARED_PROFILE_O_CONFLICT)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=3
#elif defined(SHARED_PROFILE_NO_MAX_O3)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data num_read_outstanding=3
#elif defined(SHARED_PROFILE_SINGLE_BURST_CONFLICT)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data num_read_outstanding=3
#elif defined(SHARED_PROFILE_D_CONFLICT)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 read_fifo_depth=8
#elif defined(SHARED_PROFILE_DEFAULT_MATCH)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16 num_read_outstanding=16 read_fifo_depth=256
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=32
#endif
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
#endif
