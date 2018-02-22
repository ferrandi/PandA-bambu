#include <math.h>
float formula_pow(float a, float b, float c)
{
   return acosf((powf(a,2) + powf(b,2) - powf(c,2))/(2*a*b));
}

float formula_mult(float a, float b, float c)
{
   return acosf((a*a + b*b - c*c)/(2*a*b));
}

double double_formula_pow(double a, double b, double c)
{
   return acos((pow(a,2) + pow(b,2) - pow(c,2))/(2*a*b));
}

double double_formula_mult(double a, double b, double c)
{
   return acos((a*a + b*b - c*c)/(2*a*b));
}
