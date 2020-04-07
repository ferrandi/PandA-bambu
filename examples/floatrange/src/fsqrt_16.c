#pragma mask a significand 16
float single_prec_sqrt(float a)
{
   return __builtin_sqrtf(a);
}