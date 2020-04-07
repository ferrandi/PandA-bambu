#pragma mask a exponent -127 127
float single_prec_sqrt(float a)
{
   return __builtin_sqrtf(a);
}