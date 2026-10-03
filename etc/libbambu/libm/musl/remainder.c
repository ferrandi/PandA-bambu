#include "musl_math.h"

static inline __attribute__((always_inline)) double __local_remainder(double x, double y)
{
	int q;
	return remquo(x, y, &q);
}

double remainder(double x, double y)
{
	return __local_remainder(x, y);
}

double drem(double x, double y)
{
	return __local_remainder(x, y);
}
