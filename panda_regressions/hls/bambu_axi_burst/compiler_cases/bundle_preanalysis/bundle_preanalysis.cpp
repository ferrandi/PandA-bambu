// Manual M-B1.1 bundle pre-analysis fixtures. Each top is compiled separately.

extern "C" void distinct_normal(const int *src, int *dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" void distinct_reversed(int *dst, const int *src, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" void nonqualifiable_read_normal(const int *src, int *dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = src[2 * i];
}

extern "C" void nonqualifiable_read_reversed(int *dst, const int *src, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
  for (int i = 0; i < n; ++i) dst[i] = src[2 * i];
}

extern "C" void same_bundle_normal(const int *src, int *dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" void same_bundle_reversed(int *dst, const int *src, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
  for (int i = 0; i < n; ++i) dst[i] = src[i];
}

extern "C" void inplace_pointwise(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) data[i] = data[i] + 1;
}

extern "C" void mixed_unsupported_store(int *dst, int n, int index) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = i;
  dst[index] = n;
}

extern "C" void unused_same_bundle_normal(int *dst, int *unused, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1
#pragma HLS interface mode=m_axi port=unused offset=direct bundle=output
  for (int i = 0; i < n; ++i) dst[i] = i + 7;
}

extern "C" void unused_same_bundle_reversed(int *unused, int *dst, int n) {
#pragma HLS interface mode=m_axi port=unused offset=direct bundle=output
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = i + 7;
}
