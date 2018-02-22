#ifndef __KECCAK_H__
#define __KECCAK_H__

typedef unsigned char UINT8;
typedef unsigned long long int UINT64;
#define nrRounds 24

#define nrLanes 25

#define index(x, y) (((x)%5)+5*((y)%5))
#define ROL64(a, offset) ((offset != 0) ? ((((UINT64)a) << offset) ^ (((UINT64)a) >> (64-offset))) : a)


#endif
