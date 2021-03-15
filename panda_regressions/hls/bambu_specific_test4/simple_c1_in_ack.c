#pragma HLS_interface a acknowledge
#pragma HLS_interface b acknowledge
#pragma HLS_interface c acknowledge
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
