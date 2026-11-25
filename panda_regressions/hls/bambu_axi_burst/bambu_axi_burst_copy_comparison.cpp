#ifndef COPY_COMPARISON_PROFILE
#error COPY_COMPARISON_PROFILE must select a copy comparison profile
#endif

#define COPY_COMPARISON_MAXN 4096

extern "C" void kernel(const int *__restrict src, int *__restrict dst, int n) {
#if COPY_COMPARISON_PROFILE == 1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#elif COPY_COMPARISON_PROFILE == 2
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#elif COPY_COMPARISON_PROFILE == 3
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#elif COPY_COMPARISON_PROFILE == 4
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#else
#error unsupported COPY_COMPARISON_PROFILE
#endif
  for (int i = 0; i < n; ++i) {
    dst[i] = src[i];
  }
}
