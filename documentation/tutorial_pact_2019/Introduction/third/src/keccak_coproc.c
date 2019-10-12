#include "Keccak.h"

void keccak_coproc(UINT64 *A)
{
    unsigned int i;
    for(i=0;i<nrRounds;i++) { 
        theta(A);
        rho(A);
        pi(A);
        chi(A);
        iota(A,i);
    }
}
