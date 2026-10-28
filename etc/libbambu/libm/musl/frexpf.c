#include "musl_math.h"
#include <stdint.h>

float frexpf(float x, int *e)
{
	union { float f; uint32_t i; } y = { x };
	int ee = y.i>>23 & 0xff;

	if (!ee) {
		if (!x) {
			*e = 0;
			return x;
		}
		x *= 0x1p25;
		*e -= 25;
	} else if (ee == 0xff) {
		return x;
	}

	*e = ee - 0x7e;
	y.i &= 0x807ffffful;
	y.i |= 0x3f000000ul;
	return y.f;
}
