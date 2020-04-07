#pragma mask a significand 8
float single_prec_exp(float a)
{
   return __builtin_expf(a);
}