#pragma mask a exponent -1023 1023
double double_prec_sqrt(double a)
{
   return __builtin_sqrt(a);
}