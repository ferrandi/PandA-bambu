// Write-only burst kernel with 2 write bursts in flight (MAX_OUTSTANDING=2).
// Same loop body and FIFO depth as plain_m_axi_write_burst.cpp; only the outstanding profile changes.
#define MAXN 4096

extern "C" void kernel(int dst[MAXN], int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=dst max_write_burst_length=16 write_burst_outstanding=2 write_fifo_depth=32
  for (int i = 0; i < n; ++i) dst[i] = (i * 13 + 7) ^ (i >> 2);
}
