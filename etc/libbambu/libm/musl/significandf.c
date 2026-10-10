#define _GNU_SOURCE
#include "musl_math.h"

float significandf(float x)
{
	return scalbnf(x, -ilogbf(x));
}
