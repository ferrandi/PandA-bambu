#ifndef COPY_BASELINE_PROFILE
#error COPY_BASELINE_PROFILE must select a baseline profile
#endif

#define COPY_BASELINE_MAXN 4096

extern "C" void kernel(const int src[COPY_BASELINE_MAXN], int dst[COPY_BASELINE_MAXN], int n) {
#if COPY_BASELINE_PROFILE == 1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#elif COPY_BASELINE_PROFILE == 2
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output
#elif COPY_BASELINE_PROFILE == 3
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared
#else
#error unsupported COPY_BASELINE_PROFILE
#endif
  for (int i = 0; i < n; ++i) {
    dst[i] = src[i];
  }
}
