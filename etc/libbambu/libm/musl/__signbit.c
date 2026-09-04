#include "libm.h"

static inline __attribute__((always_inline)) int __local_signbit(double x)
{
	union {
		double d;
		uint64_t i;
	} y = { x };
	return y.i>>63;
}

// FIXME: macro in math.h
int __signbit(double x)
{
	return __local_signbit(x);
}

#undef signbit
int signbit(double x)
{
	return __local_signbit(x);
}
