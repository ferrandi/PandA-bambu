// Sequential (unit-stride) read of n words through an AXI master port.

#define MAXN 4096

extern "C" int kernel(const int src[MAXN], int n) {
#ifdef BAMBU
#pragma HLS interface mode = m_axi port = src offset = direct bundle = src max_read_burst_length = 16
#ifndef NOCACHE
#pragma HLS cache bundle = src line_count = 16 line_size = 32 ways = 2 rep_policy = lru write_policy = wt
#endif
#endif
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];   // <-- the sequential read
  return acc;
}
