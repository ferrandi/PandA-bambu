extern "C" int read_profile_invalid_non_axi(const int *a) {
#pragma HLS interface mode=bram port=a read_fifo_depth=8
  return a[0];
}
