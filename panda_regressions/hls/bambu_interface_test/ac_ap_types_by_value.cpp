#include <ac_int.h>
#include <ap_int.h>

void ac_ap_types_by_value(ac_int<16, true> input, ap_uint<16> unused, ac_int<18, true>& output)
{
   (void)unused;
   output = input;
}
