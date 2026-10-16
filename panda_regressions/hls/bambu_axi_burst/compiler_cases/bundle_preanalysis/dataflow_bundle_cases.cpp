// M-B1.1 DATAFLOW adaptations for AXI write-burst bundle provenance.

struct GlobalStorage {
  int words[4096];
};

static GlobalStorage global_storage;

extern "C" __attribute__((noinline)) void fill(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = i + 1;
}

extern "C" void dataflow_struct_field(int n) {
#pragma HLS DATAFLOW
  fill(global_storage.words, n);
}

extern "C" __attribute__((noinline)) void forwarded_write(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=task_output max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = i + 1;
}

extern "C" void dataflow_forwarded_input(int *input, int n) {
#pragma HLS interface mode=m_axi port=input offset=direct bundle=input
#pragma HLS DATAFLOW
  forwarded_write(input, n);
}
