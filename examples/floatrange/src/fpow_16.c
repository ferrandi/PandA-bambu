#pragma mask a significand 16
#pragma mask b significand 16
float single_prec_pow(float a, float b)
{
   return __builtin_powf(a,b);
}