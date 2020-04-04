#pragma mask a significand 15
double double_prec_pow(double a)
{
   return __builtin_sqrt(a);
}