/*
 * The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
 * Michael Peeters and Gilles Van Assche. For more information, feedback or
 * questions, please refer to our website: http://keccak.noekeon.org/
 * Implementation by the designers,
 * hereby denoted as "the implementer".
 * To the extent possible under law, the implementer has waived all copyright
 * and related or neighboring rights to the source code in this file.
 * http://creativecommons.org/publicdomain/zero/1.0/
 *
 * ASIC-oriented variant: technology memories cannot rely on power-up
 * initialization, so the round constants are provided by the testbench.
 */
typedef unsigned char UINT8;
typedef unsigned long long int UINT64;
#define nrRounds 24

#define index(x, y) (((x)%5)+5*((y)%5))
#define ROL64(a, offset) ((offset != 0) ? ((((UINT64)a) << offset) ^ (((UINT64)a) >> (64-offset))) : a)

unsigned int keccak_rho_offset(unsigned int lane)
{
    switch(lane) {
        case 0: return 0;
        case 1: return 1;
        case 2: return 62;
        case 3: return 28;
        case 4: return 27;
        case 5: return 36;
        case 6: return 44;
        case 7: return 6;
        case 8: return 55;
        case 9: return 20;
        case 10: return 3;
        case 11: return 10;
        case 12: return 43;
        case 13: return 25;
        case 14: return 39;
        case 15: return 41;
        case 16: return 45;
        case 17: return 15;
        case 18: return 21;
        case 19: return 8;
        case 20: return 18;
        case 21: return 2;
        case 22: return 61;
        case 23: return 56;
        default: return 14;
    }
}

void theta(UINT64 *A)
{
    unsigned int x, y;
    UINT64 C[5], D[5];

    for(x=0; x<5; x++) {
        C[x] = 0;
        for(y=0; y<5; y++)
            C[x] ^= A[index(x, y)];
    }
    for(x=0; x<5; x++)
        D[x] = ROL64(C[(x+1)%5], 1) ^ C[(x+4)%5];
    for(x=0; x<5; x++)
        for(y=0; y<5; y++)
            A[index(x, y)] ^= D[x];
}

void rho(UINT64 *A)
{
    unsigned int x, y;

    for(x=0; x<5; x++) for(y=0; y<5; y++)
        A[index(x, y)] = ROL64(A[index(x, y)], keccak_rho_offset(index(x, y)));
}

void pi(UINT64 *A)
{
    unsigned int x, y;
    UINT64 tempA[25];

    for(x=0; x<5; x++) for(y=0; y<5; y++)
        tempA[index(x, y)] = A[index(x, y)];
    for(x=0; x<5; x++) for(y=0; y<5; y++)
        A[index(0*x+1*y, 2*x+3*y)] = tempA[index(x, y)];
}

void chi(UINT64 *A)
{
    unsigned int x, y;
    UINT64 C[5];

    for(y=0; y<5; y++) {
        for(x=0; x<5; x++)
            C[x] = A[index(x, y)] ^ ((~A[index(x+1, y)]) & A[index(x+2, y)]);
        for(x=0; x<5; x++)
            A[index(x, y)] = C[x];
    }
}

void iota(UINT64 *A, const UINT64 round_constants[24], unsigned int indexRound)
{
    A[index(0, 0)] ^= round_constants[indexRound];
}

void kekka_coproc(UINT64 A[25], const UINT64 round_constants[24])
{
    unsigned int i;
    for(i=0; i<nrRounds; i++) {
        theta(A);
        rho(A);
        pi(A);
        chi(A);
        iota(A, round_constants, i);
    }
}
