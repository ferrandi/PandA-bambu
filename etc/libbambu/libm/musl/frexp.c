#include "musl_math.h"
#include <stdint.h>

double frexp(double x, int *e)
{
	union { double d; uint64_t i; } y = { x };
	int ee = y.i>>52 & 0x7ff;

	if (!ee) {
		if (!x) {
			*e = 0;
			return x;
		} 
		x *= 0x1p54;
		*e -= 54;
	} else if (ee == 0x7ff) {
		return x;
	}

	*e = ee - 0x3fe;
	y.i &= 0x800fffffffffffffull;
	y.i |= 0x3fe0000000000000ull;
	return y.d;
}
