// Keep the extent visible to Bambu/MDPI so pointer-based runtime calls map the
// complete test data region, including invocations whose base is src + 2.
extern "C" int root_reinvocation(const int src[64], int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
  int sum = 0;
  for (int i = 0; i < n; ++i) sum += src[i];
  return sum;
}
