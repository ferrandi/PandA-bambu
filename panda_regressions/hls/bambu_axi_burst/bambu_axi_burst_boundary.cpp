#define MAXN 4096
extern "C" int kernel(const int src[MAXN], int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=256 num_read_outstanding=16 read_fifo_depth=4096 latency=64
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
