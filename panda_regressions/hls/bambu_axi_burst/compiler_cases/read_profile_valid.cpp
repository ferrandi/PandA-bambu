extern "C" int read_profile_valid(const int *a, const int *b, const int *c, const int *d, const int *e,
                                  const int *f, const int *g, const int *h, const int *i, const int *j,
                                  const int *k) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=oa num_read_outstanding=1
#pragma HLS interface mode=m_axi port=b offset=direct bundle=ob num_read_outstanding=3
#pragma HLS interface mode=m_axi port=c offset=direct bundle=oc num_read_outstanding=15
#pragma HLS interface mode=m_axi port=d offset=direct bundle=od num_read_outstanding=16
#pragma HLS interface mode=m_axi port=e offset=direct bundle=oe read_fifo_depth=1
#pragma HLS interface mode=m_axi port=f offset=direct bundle=of read_fifo_depth=8
#pragma HLS interface mode=m_axi port=g offset=direct bundle=og read_fifo_depth=32
#pragma HLS interface mode=m_axi port=h offset=direct bundle=oh read_fifo_depth=256
#pragma HLS interface mode=m_axi port=i offset=direct bundle=oi read_fifo_depth=4096
#pragma HLS interface mode=m_axi port=j offset=direct bundle=oj num_read_outstanding=03 num_read_outstanding=3 read_fifo_depth=0008 read_fifo_depth=8
#pragma HLS interface mode=m_axi port=k offset=direct bundle=oj num_read_outstanding=3 read_fifo_depth=8
  return a[0] + b[0] + c[0] + d[0] + e[0] + f[0] + g[0] + h[0] + i[0] + j[0] + k[0];
}
