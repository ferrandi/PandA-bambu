#ifndef UtilSollya_HH
#define UtilSollya_HH
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>


#include "sollya.h"	// Do NOT use libsollya from user's environment
sollya_chain_t makeIntPtrChainToFromBy(int m, int n, int k) ;
sollya_chain_t makeIntPtrChainFromArray(int m, int *a);

char *sPrintBinary(mpfr_t x);
char *sPrintBinaryZ(mpfr_t x);
#endif
