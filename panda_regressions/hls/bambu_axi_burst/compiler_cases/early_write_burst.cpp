extern int ordinary_global[256];
static volatile int volatile_global[256];

extern "C" void copy_read_first(const int *src, int *dst, int *unused, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=z_input max_read_burst_length=16
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=a_output max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" void write_only(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16
  for (int i = 0; i < n; ++i) dst[i] = i;
}

extern "C" int read_with_ordinary_global(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i] + ordinary_global[i];
  return acc;
}

extern "C" int read_with_volatile_global(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i] + volatile_global[i];
  return acc;
}

