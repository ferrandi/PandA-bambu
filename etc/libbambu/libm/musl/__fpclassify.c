#include "musl_math.h"
#include <stdint.h>

static inline __attribute__((always_inline)) int __local_fpclassify(double x)
{
	union {double f; uint64_t i;} u = {x};
	int e = u.i>>52 & 0x7ff;
	if (!e) return u.i<<1 ? FP_SUBNORMAL : FP_ZERO;
	if (e==0x7ff) return u.i<<12 ? FP_NAN : FP_INFINITE;
	return FP_NORMAL;
}

int __fpclassify(double x)
{
	return __local_fpclassify(x);
}

#undef fpclassify
int fpclassify(double x)
{
	return __local_fpclassify(x);
}
