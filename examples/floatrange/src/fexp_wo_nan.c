#pragma mask a exponent -127 127
float single_prec_exp(float a)
{
   return __builtin_expf(a);
}