#pragma mask a exponent -1023 1023
double double_prec_exp(double a)
{
   return __builtin_exp(a);
}