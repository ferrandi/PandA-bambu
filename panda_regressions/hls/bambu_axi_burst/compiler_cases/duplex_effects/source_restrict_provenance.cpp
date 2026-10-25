typedef int * __restrict restricted_int_ptr;

extern "C" void restrict_both(int * __restrict input, int * __restrict output, int n)
{
#pragma HLS interface mode=m_axi port=input offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=output offset=direct bundle=shared
   output[0] = input[0] + n;
}

extern "C" void restrict_typedef(restricted_int_ptr input, int *output, int n)
{
#pragma HLS interface mode=m_axi port=input offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=output offset=direct bundle=shared
   output[0] = input[0] + n;
}

extern "C" void restrict_absent(int *input, int *output, int n)
{
#pragma HLS interface mode=m_axi port=input offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=output offset=direct bundle=shared
   output[0] = input[0] + n;
}

extern "C" void restrict_one(int * __restrict input, int *output, int n)
{
#pragma HLS interface mode=m_axi port=input offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=output offset=direct bundle=shared
   output[0] = input[0] + n;
}
