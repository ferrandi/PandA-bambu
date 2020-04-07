#pragma mask a significand 42
double double_prec_exp(double a)
{
   return __builtin_exp(a);
}