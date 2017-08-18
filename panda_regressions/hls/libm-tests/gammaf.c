#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define check_d1(func, param, expected) \
do { \
	int err; hex_union ur; hex_union up; \
	float result = func##f(param); up.f = param; ur.f = result; \
	errors += (err = (result != (expected))); \
	err \
	? printf("FAIL: %s(%g/"HEXFMT")=%g/"HEXFMT" (expected %g)\n", \
		#func, (float)(param), (int)up.hex, result, (int)ur.hex, (float)(expected)) \
	: printf("PASS: %s(%g)=%g\n", #func, (float)(param), result); \
} while (0)

#define HEXFMT "%x"
typedef union {
	float f;
	uint32_t hex;
} hex_union;
float result;

#define M_2_SQRT_PIl   3.5449077018110320545963349666822903L   /* 2 sqrt (M_PIl)  */
#define M_SQRT_PIl     1.7724538509055160272981674833411451L   /* sqrt (M_PIl)  */

float zero = 0.0;
float minus_zero = 0.0;
float nan_value = 0.0;
int errors = 0;

int main(void)
{
        nan_value /= nan_value;
        minus_zero = copysignf(zero, -1.0);


	/* In glibc, gamma == lgamma. (In BSD, it's == tgamma */
	check_d1(gamma, -__builtin_inff(), __builtin_inff());
	//check_d1(gamma, __builtin_inff(), NAN);
	check_d1(gamma, 0.0, __builtin_inff()); /* pole */
	check_d1(gamma, minus_zero, __builtin_inff()); /* pole */
	check_d1(gamma, 1.0, 0.0);
	check_d1(gamma, 2.0, 0.0);
	check_d1(gamma, FLT_MAX/2, __builtin_inff()); /* overflow to inf */
	check_d1(gamma, FLT_MAX, __builtin_inff()); /* overflow to inf */
	check_d1(gamma, __builtin_inff(), __builtin_inff()); /* overflow to inf */
	check_d1(gamma, 7, logf(2*3*4*5*6)); /* normal value */

	printf("Errors: %d\n", errors);
	return errors;
}
