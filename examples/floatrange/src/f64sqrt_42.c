#pragma mask a significand 42
double double_prec_sqrt(double a)
{
   return __builtin_sqrt(a);
}