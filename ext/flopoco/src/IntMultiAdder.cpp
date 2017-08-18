/*
An multioperand integer adder wrapper class for FloPoCo

It may be pipelined to arbitrary frequency.
Also useful to derive the carry-propagate delays for the subclasses of Target

Authors:  Bogdan Pasca

This file is part of the FloPoCo project
developed by the Arenaire team at Ecole Normale Superieure de Lyon

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
2008-2010.
  All rights reserved.
*/

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "IntMultiAdder.hpp"

#include "IntAddition/IntNAdder.hpp"
#include "IntAddition/IntCompressorTree.hpp"
 
using namespace std;
namespace flopoco {
	
	IntMultiAdder::IntMultiAdder (Target* target, int wIn, int N, map<string, double> inputDelays, bool carryIn):
	Operator ( target, inputDelays), wIn_ ( wIn ), N_(N), carryIn_(carryIn)  {
		ostringstream name;
		srcFileName="IntMultiAdder";
		setCopyrightString ( "Bogdan Pasca 2011" );
		
		name << "IntMultiAdder_" << wIn_<<"_op"<<N<<"_f"<<target->frequencyMHz()<<"_uid"<<getNewUId();
		setName ( name.str() );
		
		// Set up the IO signals
		for (int i=0; i<N; i++)
			addInput ( join("X",i) , wIn_, true );

		addOutput ( "R"  , wIn_, 1 , true );
		
		REPORT(DETAILED, "Implementing IntMultiAdder " << wIn);
		
		Operator* IntMultiAdderInstantiation;
		
		if (( target->getVendor()=="Altera" || target->getID()=="Virtex5" || target->getID()=="Virtex6" ) && !carryIn)
			IntMultiAdderInstantiation = new IntCompressorTree( target, wIn, N, inputDelays);
		else
			IntMultiAdderInstantiation = new IntNAdder( target, wIn, N, inputDelays, carryIn);
		
		cloneOperator( IntMultiAdderInstantiation );
		changeName ( name.str() );
	}
	
	/**************************************************************************/
	IntMultiAdder::~IntMultiAdder() {
	}
	
	/******************************************************************************/
	void IntMultiAdder::emulate ( TestCase* tc ) {
		
		mpz_class svR = 0.0;
		if (carryIn_)
			svR = svR + tc->getInputValue ( "Cin" );
		
		for (int i=0; i<N_; i++)
			svR = svR + tc->getInputValue ( join("X",i) );
		
		svR = svR % (mpz_class(1)<<wIn_);
		tc->addExpectedOutput ( "R", svR );
	}
}


