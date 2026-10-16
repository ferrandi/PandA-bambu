static const int weights[4] = {1, 2, 3, 4};

void const_array_partition(int* output)
{
#pragma HLS ARRAY_PARTITION variable=weights complete dim=0
   output[0] = weights[0];
}
