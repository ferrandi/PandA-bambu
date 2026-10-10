// The metadata path accepts any uint32 'latency', but the value is used to drive the
// testbench memory model, which receives the nominal latency minus one. Below two it
// would wrap, so the flow must reject it instead of building an unusable model.
//
// Two details let this case actually reach that check instead of failing earlier for
// an unrelated reason: the interface is offset=direct, and the parameter is a sized
// array so the shared testbench can be generated. Both are prerequisites, not defects.
#define MAXN 4096
extern "C" int latency_below_minimum(const int a[MAXN], int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_read_burst_length=16 latency=1
  int acc = 0;
  for (int i = 0; i < n; ++i) acc += a[i];
  return acc;
}
