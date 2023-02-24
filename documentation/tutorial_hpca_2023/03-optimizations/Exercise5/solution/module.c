#include <math.h>

#ifdef FP_SINGLE
#define FP_TYPE float
#define ACOS(a) acosf(a)
#else
#define FP_TYPE double
#define ACOS(a) acos(a)
#endif

#ifdef MULT_SQUARE
#define SQUARE(a) (a*a)
#else
#ifdef FP_SINGLE
#define SQUARE(a) powf(a,2)
#else
#define SQUARE(a) pow(a,2)
#endif
#endif

FP_TYPE awesome_math(FP_TYPE a, FP_TYPE b, FP_TYPE c)
{
   return ACOS((SQUARE(a) + SQUARE(b) - SQUARE(c))/(2*a*b));
}
