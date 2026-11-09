#define MAXN 4096
extern "C" int kernel(const int src[MAXN], int n) {
#if (defined(BAMBU_AXI_SUM_B1_O3) + defined(BAMBU_AXI_SUM_B1_O3_EXPLICIT) + defined(BAMBU_AXI_SUM_O5) + \
     defined(BAMBU_AXI_SUM_O8) + defined(BAMBU_AXI_SUM_O16) + defined(BAMBU_AXI_SUM_LAT128)) > 1
#error "select at most one AXI sum profile"
#elif defined(BAMBU_AXI_SUM_B1_O3)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src num_read_outstanding=3 read_fifo_depth=8 latency=64
#elif defined(BAMBU_AXI_SUM_B1_O3_EXPLICIT)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=1 num_read_outstanding=3 read_fifo_depth=8 latency=64
#elif defined(BAMBU_AXI_SUM_O5)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=16 num_read_outstanding=5 read_fifo_depth=256 latency=64
#elif defined(BAMBU_AXI_SUM_O8)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=16 num_read_outstanding=8 read_fifo_depth=256 latency=64
#elif defined(BAMBU_AXI_SUM_O16)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=16 num_read_outstanding=16 read_fifo_depth=256 latency=64
#elif defined(BAMBU_AXI_SUM_LAT128)
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=256 latency=128
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=256 latency=64
#endif
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
