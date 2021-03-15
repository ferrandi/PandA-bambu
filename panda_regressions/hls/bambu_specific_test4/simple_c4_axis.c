#pragma HLS_interface a axis
#pragma HLS_interface b axis
#pragma HLS_interface c axis
#pragma HLS_interface d axis
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
