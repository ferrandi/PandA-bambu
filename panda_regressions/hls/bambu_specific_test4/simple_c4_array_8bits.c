#pragma HLS_interface d array 8
void sum3numbers(char a[8], char b[8], char c[8], char d[8])
{
   unsigned i;
   for(i = 0; i < 8; ++i)
      d[i] = a[i] + b[i] + c[i];
}
