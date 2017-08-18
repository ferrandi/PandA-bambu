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
#include "Term.hh"



Term::Term(int d_, double *k_, Param &p_)
	: d(d_), k(k_), p(p_), errMethod(new PWPolynomial[2]), refCount(0)
{
}

Term::~Term()
{
	delete[] errMethod;
}

PWPolynomial *Term::getErrMethod()
{
	return errMethod;
}
