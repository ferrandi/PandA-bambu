#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define check_d1(func, param, expected) \
   do { \
      int err; hex_union ur; hex_union up; \
      double result = func(param); up.f = param; ur.f = result; \
      errors += (err = (result != (expected))); \
      err \
      ? printf("FAIL: %s(%g/"HEXFMT")=%g/"HEXFMT" (expected %g) %u\n", \
#func, (double)(param), (long long)up.hex, result, (long long)ur.hex, (double)(expected), result > expected ? 0 : 1) \
      : printf("PASS: %s(%g)=%g\n", #func, (double)(param), result); \
   } while (0)

#define HEXFMT "%08llx"
typedef union {
   double f;
   uint64_t hex;
} hex_union;
double result;
#define M_2_SQRT_PIl   3.544907701811032208L   /* 2 sqrt (M_PIl)  */
#define M_SQRT_PIl     1.7724538509055160272981674833411451L   /* sqrt (M_PIl)  */

double zero = 0.0;
double minus_zero = 0.0;
double nan_value = 0.0;
int errors = 0;

int main(void)
{
   nan_value /= nan_value;
   minus_zero = copysign(zero, -1.0);

   //check_d1(tgamma, __builtin_inf(), NAN);
   //check_d1(tgamma, negative_integer, NAN);
   check_d1(tgamma, 0.0, __builtin_inf()); /* pole */
   check_d1(tgamma, minus_zero, (-__builtin_inf())); /* pole */
   check_d1(tgamma, DBL_MAX/2, __builtin_inf()); /* overflow to inf */
   check_d1(tgamma, DBL_MAX, __builtin_inf()); /* overflow to inf */
   check_d1(tgamma, __builtin_inf(), __builtin_inf()); /* overflow to inf */
   check_d1(tgamma, 7, 2*3*4*5*6); /* normal value */
   check_d1(tgamma, -0.5, -M_2_SQRT_PIl); /* normal value (testing negative points) */
   printf("Errors: %d\n", errors);
   return errors;
}
