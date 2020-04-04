#pragma mask a significand 15
double double_prec_exp(double a)
{
   return __builtin_exp(a);
}