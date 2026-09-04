#include <fenv.h>
#include "musl_math.h"

/* nearbyint is the same as rint, but it must not raise the inexact exception */

double nearbyint(double x)
{
#ifdef FE_INEXACT
#ifndef NO_RAISE_EXCEPTIONS
	#pragma STDC FENV_ACCESS ON
#endif
	int e;

	e = fetestexcept(FE_INEXACT);
#endif
	x = rint(x);
#ifdef FE_INEXACT
	if (!e)
		feclearexcept(FE_INEXACT);
#endif
	return x;
}
