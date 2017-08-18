#ifndef _UTIL_HH_
#define _UTIL_HH_

#include <iostream>
#include <mpfr.h>
#include "FlopocoStream.hpp"

extern "C" {
#include <math.h>
}

using namespace std;

void handleSignals();

#define P(n) (((long long int)1) << (n))
#define E(x) (exp2(x))
#define T(i,p,w) ((i) & ((P(w)-1) << ((p)-(w))))
#define I(i,p,w) (((i) >> ((p)-(w))) & (P(w)-1))

#define MAX(x,y) ((x) >= (y) ? (x) : (y))
#define MIN(x,y) ((x) <= (y) ? (x) : (y))

void mpz_set_lli(mpz_t rop, unsigned long long int op);
unsigned long long int mpz_get_lli(mpz_t op);

double fact(int n);
double bin(int n, int k);



class VHDLGen {
public:
	static void genInteger(flopoco::FlopocoStream& vhdl, long long int x, int w);
	static void genInteger(flopoco::FlopocoStream& vhdl, mpz_t mpX, int w);

	static void genROM(flopoco::FlopocoStream& vhdl, long long int *t, int wX, int wR, string x, string r);
	static void genROM(flopoco::FlopocoStream& vhdl, mpz_t *mpT, int wX, int wR, string x, string r);
};



class Estim {
public:
	static double xorArea(int w);
	static double xorDelay(int w);

	static double adderArea(int w, int n = 2);
	static double adderDelay(int w, int n = 2);

	static double multiplierArea(int wX, int wY);
	static double multiplierDelay(int wX, int wY);

	static double romArea(int wX, int wR);
	static double romDelay(int wX, int wR);
};

#endif // _UTIL_HH_
