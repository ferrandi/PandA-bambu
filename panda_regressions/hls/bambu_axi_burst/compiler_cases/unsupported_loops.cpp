// Explicit max-burst request on shapes the first recognizer must reject.
extern "C" int stride_two(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; i += 2) acc += src[i];
  return acc;
}

extern "C" int conditional_load(const int *src, int n, int choose) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) if (choose) acc += src[i];
  return acc;
}

extern "C" int early_exit(const int *src, int n, int stop) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) { if (i == stop) break; acc += src[i]; }
  return acc;
}

extern "C" int pointer_chase(const int *const *next, int n) {
#pragma HLS interface mode=m_axi port=next offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  const int *p = next[0];
  for (int i = 0; i < n; ++i) { acc += *p; p = next[i]; }
  return acc;
}

extern "C" void stores_only(int *dst, const int *src, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" int mixed_bundle(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=data max_read_burst_length=16
#pragma HLS interface mode=m_axi port=data offset=direct bundle=data max_write_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) { int v = data[i]; data[i] = v + 1; acc += v; }
  return acc;
}
