__attribute__((noinline)) void sum3numbers_nested(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c + *d;
}

#pragma HLS interface port = a mode = none
#pragma HLS interface port = b mode = none
#pragma HLS interface port = c mode = valid
#pragma HLS interface port = d mode = valid
void sum3numbers(short* a, short* b, short* c, short* d)
{
   sum3numbers_nested(a, b, c, d);
   *a += *b;
}
