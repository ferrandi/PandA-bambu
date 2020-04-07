#pragma mask a significand 32
#pragma mask b significand 32
double double_prec_pow(double a, double b)
{
   return __builtin_pow(a,b);
}