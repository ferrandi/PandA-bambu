#define _GNU_SOURCE
#include "musl_math.h"

int finite(double x)
{
	return isfinite(x);
}
