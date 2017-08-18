/*
 * An FP collision operator for FloPoCo

This is mostly an example of a coarse-grain, nonstandard operator.

A 3D collision detector inputs 3 FP coordinates X,Y and Z and
the square of a radius, R2, and computes a boolean predicate which is
true iff X^2 + Y^2 + Z^2 < R2

There are two versions, selectable by the useFPOperators parameter.
One combines existing FloPoCo floating-point operators, and the other
one is a specific datapath designed in the FloPoCo philosophy.

As this is a floating-point operator, each versions has its "grey
area", when X^2+Y^2+Z^2 is very close to R2.  In this case the
predicate may be wrong with respect to the infinitely accurate result.

The grey area of the combination of FP operators is about 2.5 units in
the last place of R2.  The pure FloPoCo version (which is a lot
smaller and faster) is more accurate, with a grey area smaller than 1
ulp of R2.

 
  Author : Florent de Dinechin
 
  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon.

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, 2008-2010.
  All right reserved.
 
*/
#ifndef __COLLISION_HPP
#define __COLLISION_HPP
#include <vector>
#include <sstream>

#include "../Operator.hpp"
#include "../Shifters.hpp"
#include "../LZOC.hpp"
#include "../LZOCShifterSticky.hpp"
#include "../IntAdder.hpp"
#include "../IntMultiplier.hpp"
#include "../IntSquarer.hpp"
#include "../FPMultiplier.hpp"
#include "../FPAdderSinglePath.hpp"

namespace flopoco{

	class Collision : public Operator
	{
	public:
		Collision(Target* target, int wE, int wF, int optimize);
		~Collision();

		void emulate(TestCase * tc);

		void buildRandomTestCases(TestCaseList* tcl, int n);

		int wE, wF;

	};
}
#endif
