extern "C" int invalid_zero(const int *a, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=0
  return n > 0 ? a[0] : 0;
}
