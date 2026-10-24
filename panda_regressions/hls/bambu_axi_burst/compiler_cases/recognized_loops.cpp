// Compiler-level burst recognizer fixtures. Each exported function isolates a
// source shape; these are ordinary C++ inputs, not expected/generated RTL.
#define MAXN 4096

extern "C" int renamed_sum(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

extern "C" int two_regions(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  for (int j = 0; j < n; ++j) acc += src[j];
  return acc;
}

static int scan_region(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

extern "C" int repeated_regions(const int *src, int n) {
  // Exercise two dynamic entries to the same callee/configured resource.
  int acc = scan_region(src, n);
  acc += scan_region(src, n);
  return acc;
}

extern "C" int zero_count(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

extern "C" int one_count(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}

extern "C" int nonmultiple_count(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
