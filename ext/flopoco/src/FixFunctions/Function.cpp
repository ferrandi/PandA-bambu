/*
  Function object for FloPoCo

  Authors: Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  

  All rights reserved.

*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SOLLYA

#include "Function.hpp"
#include <sstream>

namespace flopoco{

	Function::Function(string name_, double xmin, double xmax, double scale)
	{
		/* Convert the input string into a sollya evaluation tree */
		sollya_node_t sF = parseString(name_.c_str());

		// Name HAS to be unique!
		// will cause weird bugs otherwise
		ostringstream complete_name;
		complete_name << name_ << " * " << scale << " on [" << xmin << ", " << xmax << "]"; 
		name = complete_name.str();

		/* If conversion did not succeed (i.e. parse error)
		 * throw an exception */
		if (sF == 0)
			throw "Unable to parse input function.";
		
		/* Map the input to the [0,1[ range */
		// g(y) = scale * f(y * (xmax - xmin) + xmin)
		sollya_node_t sXW = makeConstantDouble(xmax - xmin);
		sollya_node_t sXMin = makeConstantDouble(xmin);
	
		sollya_node_t sX = makeAdd(makeMul(makeVariable(), sXW), sXMin);
		sollya_node_t sG = substitute(sF, sX);
	
		node = makeMul(makeConstantDouble(scale), sG);
		if (node == 0)
			throw "Sollya error when performing range mapping.";
	
		// No need to free memory for Sollya subexpressions (?)
	}

	Function::~Function()
	{
		free_memory(node);
	}

	string Function::getName() const
	{
		return name;
	}

	void Function::eval(mpfr_t r, mpfr_t x) const
	{
		evaluateFaithful(r, node, x, getToolPrecision());
	}

	double Function::eval(double x) const
	{
		mpfr_t mpX, mpR;
		double r;

		mpfr_inits(mpX, mpR, NULL);
		mpfr_set_d(mpX, x, GMP_RNDN);
		evaluateFaithful(mpR, node, mpX, getToolPrecision());
		r = mpfr_get_d(mpR, GMP_RNDN);
		mpfr_clears(mpX, mpR, NULL);

		return r;
	}

	sollya_node_t Function::getSollyaNode() const
	{
		return node;
	}

}
#endif //HAVE_SOLLYA	
