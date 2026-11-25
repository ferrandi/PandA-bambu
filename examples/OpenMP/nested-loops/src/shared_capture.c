#include <omp.h>

// The shared local variable 'factor', and the by-reference captures of 'a'
// and 'n', are only read inside the parallel region: they are candidates for
// the parameter promotion analysis of the openmpBambu plug-in. With typed
// pointers (clang-13) the parameters are promoted; with opaque pointers
// (clang-16 and newer) the analysis skips them, and the kernel must still
// compile and execute correctly.
void __attribute__((noinline)) shared_capture(int* a, int n)
{
   int factor = 3;
#pragma omp parallel for num_threads(4)
   for(int i = 0; i < n; ++i)
      a[i] = a[i] * factor;
}
