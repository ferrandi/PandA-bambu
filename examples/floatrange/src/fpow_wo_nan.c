#pragma mask a exponent -127 127
#pragma mask b exponent -127 127
float single_prec_pow(float a, float b)
{
   return __builtin_powf(a,b);
}