__attribute__((noinline)) void sum3numbers_nested(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c + *d;
}

#pragma HLS_interface a none
#pragma HLS_interface b none
#pragma HLS_interface c valid
#pragma HLS_interface d valid
void sum3numbers(short* a, short* b, short* c, short* d)
{
   sum3numbers_nested(a,b,c,d);
   *a += *b;
}
