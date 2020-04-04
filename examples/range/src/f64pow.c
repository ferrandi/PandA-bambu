#pragma mask a significand 15
#pragma mask b significand 15
double double_prec_pow(double a, double b)
{
   return __builtin_pow(a,b);
}