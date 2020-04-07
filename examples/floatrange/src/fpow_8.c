#pragma mask a significand 8
#pragma mask b significand 8
float single_prec_pow(float a, float b)
{
   return __builtin_powf(a,b);
}