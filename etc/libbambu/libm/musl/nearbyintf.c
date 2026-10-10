#include <fenv.h>
#include "musl_math.h"

float nearbyintf(float x)
{
#ifdef FE_INEXACT
#ifndef NO_RAISE_EXCEPTIONS
	#pragma STDC FENV_ACCESS ON
#endif
	int e;

	e = fetestexcept(FE_INEXACT);
#endif
	x = rintf(x);
#ifdef FE_INEXACT
	if (!e)
		feclearexcept(FE_INEXACT);
#endif
	return x;
}
