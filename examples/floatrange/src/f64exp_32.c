#pragma mask a significand 32
double double_prec_exp(double a)
{
   return __builtin_exp(a);
}