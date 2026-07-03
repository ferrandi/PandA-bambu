#include "libm.h"

static inline __attribute__((always_inline)) int __local_signbitf(float x)
{
	union {
		float f;
		uint32_t i;
	} y = { x };
	return y.i>>31;
}

// FIXME: macro in math.h
int __signbitf(float x)
{
	return __local_signbitf(x);
}

#undef signbitf
int signbitf(float x)
{
	return __local_signbitf(x);
}

