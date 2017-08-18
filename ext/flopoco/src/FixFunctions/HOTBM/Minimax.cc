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

#include <mpfr.h>
#include "Minimax.hh"
#include "sollya.h"

#define EPSILON 1.0e-12

namespace flopoco{

	/*
	 * Uses libsollya to compute the minimax polynomial
	 * using the remez algorithm
	 */
	Minimax::Minimax(Function &f_, double ia_, double ib_, int d)
		: mpP(NULL)
	{
		/* Nice progress message */
		cerr << "*";

		/* Convert parameters to their required type */
		mpfr_t ia, ib, prec;
		mpfr_inits(ia, ib, prec, NULL);
		mpfr_set_d(ia, ia_, GMP_RNDN);
		mpfr_set_d(ib, ib_, GMP_RNDN);
		mpfr_set_d(prec, EPSILON, GMP_RNDN);

		/* Create input functions */
		sollya_node_t f = copyTree(f_.getSollyaNode());
		sollya_node_t w = parseString("1");

		/* Other paramters */
		sollya_chain_t monoms = makeIntPtrChainFromTo(0,d);

		/* Call remez */
		sollya_node_t nRemez = remez(f, w, monoms, ia, ib, &prec, getToolPrecision());

		/* Extract coefficients */
		int degree;
		sollya_node_t *nCoef;
		mpfr_t *coef;

		getCoefficients(&degree, &nCoef, nRemez);
		coef = new mpfr_t[degree+1];
		int i;
		for (i = 0; i <= degree; i++)
			{
				mpfr_init(coef[i]);
				evaluateConstantExpression(coef[i], nCoef[i], getToolPrecision());
			}

		/* Create the long-awaited polynomial */
		mpP = new MPPolynomial(degree, coef);

		/* Compute the error */
		sollya_node_t nDiff = makeSub(f, nRemez);
		mpfr_init(mpErr);
		uncertifiedInfnorm(mpErr, nDiff, ia, ib, 501/*default in sollya*/, getToolPrecision()); 

		/* Cleanup */
		for (i = 0; i <= degree; i++)
			mpfr_clear(coef[i]);
		delete coef;
		free_memory(nDiff);	/* XXX: also frees nRemez and f */
		free_memory(w);
		mpfr_clear(ia);
		mpfr_clear(ib);
		mpfr_clear(prec);
		freeChain(monoms, freeIntPtr);
	}


	Minimax::~Minimax()
	{
		if (mpP)
			delete mpP;
		mpfr_clear(mpErr);
		mpfr_free_cache();
	}

	MPPolynomial &Minimax::getMPP() const
	{
		return *mpP;
	}

	void Minimax::getMPErr(mpfr_t mpErr_) const
	{
		mpfr_set(mpErr_, mpErr, GMP_RNDN);
	}
}
