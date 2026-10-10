extern "C" int invalid_non_axi(const int *a, int n) {
#pragma HLS interface mode=bram port=a max_read_burst_length=16
  return n > 0 ? a[0] : 0;
}
