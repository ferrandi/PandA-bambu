#include "ap_int.h"

#pragma HLS_interface a fifo
#pragma HLS_interface b fifo
#pragma HLS_interface c fifo
#pragma HLS_interface d fifo
void sum3numbers(ap_int<67>* a, ap_int<67>* b, ap_int<67>* c, ap_int<68>* d)
{
   *d = *a + *b + *c;
}
