#pragma mask a significand 42
#pragma mask b significand 42
double double_prec_pow(double a, double b)
{
   return __builtin_pow(a,b);
}