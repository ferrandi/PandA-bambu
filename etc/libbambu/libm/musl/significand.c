#define _GNU_SOURCE
#include "musl_math.h"

double significand(double x)
{
	return scalbn(x, -ilogb(x));
}
