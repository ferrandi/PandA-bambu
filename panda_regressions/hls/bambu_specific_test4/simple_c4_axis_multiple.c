#pragma HLS_interface a axis
#pragma HLS_interface b axis
#pragma HLS_interface c axis
#pragma HLS_interface d axis
void sum3numbers(short* a, short* b, short* c, short* d)
{
   int i;
   #pragma nounroll
   for(i=0; i < 8; ++i)
      d[i] = a[i] + b[i] + c[i];
}
