#define _GNU_SOURCE
#include "musl_math.h"

int finitef(float x)
{
	return isfinite(x);
}
