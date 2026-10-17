// A plain m_axi bundle with no burst attributes and no 'latency' attribute. The
// frontend injects latency=0 for every m_axi bundle that omits it, so the value
// must be read as "no per-bundle override" and the global --mem-delay-read /
// --mem-delay-write must apply. Before that distinction this design compiled but
// failed under --simulate.
#define MAXN 4096
extern "C" int kernel(const int src[MAXN], int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
