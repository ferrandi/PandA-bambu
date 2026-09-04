#include "musl_math.h"

static inline __attribute__((always_inline)) float __local_remainderf(float x, float y)
{
	int q;
	return remquof(x, y, &q);
}

float remainderf(float x, float y)
{
	return __local_remainderf(x, y);
}

float dremf(float x, float y)
{
	return __local_remainderf(x, y);
}
