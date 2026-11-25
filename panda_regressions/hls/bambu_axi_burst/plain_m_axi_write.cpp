#define MAXN 4096

extern "C" void kernel(int dst[MAXN], int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=dst
  for (int i = 0; i < n; ++i) dst[i] = (i * 13 + 7) ^ (i >> 2);
}
