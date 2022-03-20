#pragma HLS_interface d acknowledge
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
