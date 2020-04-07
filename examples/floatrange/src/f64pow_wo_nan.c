#pragma mask a exponent -1023 1023
#pragma mask b exponent -1023 1023
double double_prec_pow(double a, double b)
{
   return __builtin_pow(a,b);
}