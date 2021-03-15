#pragma HLS_interface a valid
#pragma HLS_interface b valid
#pragma HLS_interface c valid
#pragma HLS_interface d bus
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
