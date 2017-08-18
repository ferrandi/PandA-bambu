/*
  The High Order Table Based Method for hardware function evaluation

  Author: Jérémie Detrey, FloPoCoized by Christian Klein and F. de Dinechin

  (developed for the polynomial-based square root)

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2005-2011  

  All rights reserved.

*/
#include "MPPolynomial.hh"



MPPolynomial::MPPolynomial()
	: d(0), mpK(new mpfr_t[1])
{
	mpfr_init(mpK[0]);
	mpfr_set_ui(mpK[0], 0, GMP_RNDN);
}

MPPolynomial::MPPolynomial(int d_, mpfr_t *mpK_)
	: d(d_), mpK(new mpfr_t[d+1])
{
	for (int i = 0; i <= d; i++) {
		mpfr_init(mpK[i]);
		mpfr_set(mpK[i], mpK_[i], GMP_RNDN);
	}
}

MPPolynomial::MPPolynomial(const MPPolynomial &mpP)
	: d(mpP.d), mpK(new mpfr_t[d+1])
{
	for (int i = 0; i <= d; i++) {
		mpfr_init(mpK[i]);
		mpfr_set(mpK[i], mpP.mpK[i], GMP_RNDN);
	}
}

MPPolynomial::~MPPolynomial()
{
	for (int i = 0; i <= d; i++)
		mpfr_clear(mpK[i]);
	delete[] mpK;
}

int MPPolynomial::getD() const
{
	return d;
}

void MPPolynomial::getMPK(mpfr_t mpK_, int i) const
{
	mpfr_set(mpK_, mpK[i], GMP_RNDN);
}

void MPPolynomial::eval(mpfr_t mpR, mpfr_t mpX, int n) const
{
	mpfr_t mpTmp;

	mpfr_init(mpTmp);
	mpfr_set_ui(mpR, 0, GMP_RNDN);

	for (int i = d; i >= n; i--) {
		mpfr_mul(mpR, mpR, mpX, GMP_RNDN);
		mpfr_set(mpTmp, mpK[i], GMP_RNDN);
		for (int j = 0; j < n; j++)
			mpfr_mul_ui(mpTmp, mpTmp, i-j, GMP_RNDN);
		mpfr_add(mpR, mpR, mpTmp, GMP_RNDN);
	}

	mpfr_clear(mpTmp);
}

MPPolynomial MPPolynomial::operator=(const MPPolynomial &mpP)
{
	if (this != &mpP) {
		for (int i = 0; i <= d; i++)
			mpfr_clear(mpK[i]);
		delete[] mpK;
		d = mpP.d;
		mpK = new mpfr_t[d+1];
		for (int i = 0; i <= d; i++) {
			mpfr_init(mpK[i]);
			mpfr_set(mpK[i], mpP.mpK[i], GMP_RNDN);
		}
	}

	return *this;
}

MPPolynomial MPPolynomial::operator>>(double x0) const
{
	MPPolynomial mpP(d, mpK);

	mpfr_t mpX0, mpTmp;
	mpz_t mpBin;

	mpfr_inits(mpX0, mpTmp, NULL);
	mpz_init(mpBin);

	mpfr_set_d(mpX0, x0, GMP_RNDN);

	for (int i = 0; i <= d; i++) {
		mpfr_set_ui(mpP.mpK[i], 0, GMP_RNDN);
		for (int j = i; j <= d; j++) {
			mpfr_pow_ui(mpTmp, mpX0, j-i, GMP_RNDN);
			mpz_bin_uiui(mpBin, j, i);
			mpfr_mul_z(mpTmp, mpTmp, mpBin, GMP_RNDN);
			mpfr_mul(mpTmp, mpTmp, mpK[j], GMP_RNDN);
			mpfr_add(mpP.mpK[i], mpP.mpK[i], mpTmp, GMP_RNDN);
		}
	}

	mpfr_clears(mpX0, mpTmp, NULL);
	mpz_clear(mpBin);

	return mpP;
}

MPPolynomial MPPolynomial::operator<<(double x0) const
{
	return operator>>(-x0);
}
