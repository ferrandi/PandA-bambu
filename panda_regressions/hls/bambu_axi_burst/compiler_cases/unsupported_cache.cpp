extern "C" int cached_region(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=data max_read_burst_length=16
#pragma HLS cache bundle=data line_count=16 line_size=32 ways=2 rep_policy=lru write_policy=wt
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += src[i];
  return acc;
}
