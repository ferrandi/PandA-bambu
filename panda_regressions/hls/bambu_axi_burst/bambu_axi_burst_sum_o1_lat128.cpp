// Same kernel as bambu_axi_burst_sum_o1.cpp with latency metadata kept separate
// from the --mem-delay-read stimulus selected by the regression.
#define MAXN 4096
extern "C" int kernel(const int src[MAXN], int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=256 latency=128
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
