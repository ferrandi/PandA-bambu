#pragma mask a significand 32
double double_prec_sqrt(double a)
{
   return __builtin_sqrt(a);
}