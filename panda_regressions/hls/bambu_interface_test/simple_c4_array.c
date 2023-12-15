#pragma HLS interface port=d mode=array elem_count=8
void sum3numbers(short a[8], short b[8], short c[8], short d[8])
{
   unsigned i;
   for(i = 0; i < 8; ++i)
      d[i] = a[i] + b[i] + c[i];
}
