
/*
   A multiply-and-add in a single bit heap

Author:  Florent de Dinechin

This file is part of the FloPoCo project
developed by the Arenaire team at Ecole Normale Superieure de Lyon

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
2012-2013.
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
#include "Operator.hpp"
#include "FixMultAdd.hpp"
#include "IntMultiplier.hpp"

using namespace std;

namespace flopoco {

#define vhdl parentOp->vhdl
#define declare parentOp->declare
#define inPortMap parentOp->inPortMap
#define outPortMap parentOp->outPortMap
#define instance parentOp->instance
#define manageCriticalPath parentOp->manageCriticalPath
#define getCriticalPath parentOp->getCriticalPath
#define setCycle parentOp->setCycle
#define oplist parentOp->getOpListR()




#if 0
 	// The virtual constructor TODO
	FixMultAdd::FixMultAdd (Operator* parentOp_, BitHeap* bitHeap_, Signal* x_, Signal* y_, int wX_, 
			int wY_, int wOut_, int lsbWeight_, bool negate_, bool signedIO_, float ratio_):
		Operator ( parentOp_->getTarget()), 
		wxDSP(0), wyDSP(0), wXdecl(wX_), wYdecl(wY_), wX(0), wY(0), wOut(wOut_), ratio(ratio_),  maxError(0.0), 
		parentOp(parentOp_), bitHeap(bitHeap_), lsbWeight(lsbWeight_),
		x(x_), y(y_), negate(negate_), signedIO(signedIO_) 
	{

		isOperator=false;
		// leave the compression to the parent op
	}

#endif





	// The constructor for a stand-alone operator
	// The parameters are quite messy, yes.
	// All the lsbs and msbs are given with respect to the lsb of the result 
	// (0 means: aligned with the LSB of the result) 
	FixMultAdd::FixMultAdd(Target* target, int wX_, int wY_, int wA_, int wOut_, 
	                       int msbP_, int lsbA_, 
	                       bool signedIO_,
	                       float ratio_, bool enableSuperTiles_, 
	                       map<string, double> inputDelays_):
		Operator ( target, inputDelays_ ),
		wX(wX_), wY(wY_), wA(wA_), wOut(wOut_),
		msbP(msbP_),
		lsbPfull(msbP - wX -wY),
 		lsbA(lsbA_),
		signedIO(signedIO_),
		ratio(ratio_), 
		enableSuperTiles(enableSuperTiles_) 
{

		srcFileName="FixMultAdd";
		setCopyrightString ( "Florent de Dinechin, 2012" );

		{
			ostringstream name;
			name <<"FixMultAdd";
			name << wX << "x" << wY << "p" << wA << "r" << wOut << "" << (signedIO?"s":"u") << "uid"<<Operator::getNewUId();
			setName ( name.str() );
			REPORT(INFO, "Building " << name.str() );
		}

		parentOp=this;

		// TODO the following assumes that lsbA is positive
		if(lsbA<0){
			THROWERROR("lsbA<0: this is currently unsupported, if you ask nicely we could fix it")
		}

		if(lsbPfull >= 0) { // no truncation needed, we can add all the bits of the product 
			wOutP = wX+wY; // the parameter we'll pass to IntMultiplier
			g=0;
			possibleOutputs=1; // No faithful rounding
			REPORT(DETAILED, " Exact architecture" )
		}
		else { // there is a truncation of the product
			// we will add g guard bits to the bit heap
			wOutP=msbP;
			g = IntMultiplier::neededGuardBits(wX, wY, wOutP); 
			possibleOutputs=2; // No faithful rounding
			REPORT(DETAILED, " Faithfully rounded architecture" )
		}

		// Set up the IO signals
		xname="X";
		yname="Y";
		aname="A";

		addInput ( xname  , wX, true );
		addInput ( yname  , wY, true );
		addInput ( aname  , wA, true );
		addOutput ( "R"  , wOut, possibleOutputs , true );


		// Set up the VHDL library style
		if(signedIO)
			useStdLogicSigned();
		else
			useStdLogicUnsigned();

		// The bit heap
		bitHeap = new BitHeap(this, wOut+g, enableSuperTiles);


		// TODO should be a parameter to the bit heap constructor
		bitHeap->setSignedIO(signedIO);


		// initialize the critical path
		setCriticalPath(getMaxInputDelays ( inputDelays_ ));



		// TODO if it fits in a DSP block just write A*B+C
		
		

		fillBitHeap();

		bitHeap -> generateCompressorVHDL();			
		vhdl << tab << "R" << " <= " << bitHeap-> getSumName() << range(wOut+g-1, g) << ";" << endl;
	}




	FixMultAdd::~FixMultAdd() {
		free(mult);
	}


	void FixMultAdd::fillBitHeap() {
		//  throw the addend to the bit heap
		bitHeap -> addSignedBitVector(lsbA+g, aname, wA);
		
		//  throw the product to the bit heap
		int lsbWeight = msbP+1 - wOutP;
		// TODO we could read wX and wY from the signal.
		mult = new IntMultiplier (this, bitHeap, getSignalByName("X"), getSignalByName("Y"), wX, wY, wOutP, lsbWeight, false /*negate*/, signedIO, 0.75);
	}




	void FixMultAdd::emulate ( TestCase* tc ) {
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");
		mpz_class svA = tc->getInputValue("A");
		mpz_class svP, svR;
		mpz_class twoToWR = (mpz_class(1) << (wOut));
		mpz_class twoToWRm1 = (mpz_class(1) << (wOut-1));

		if (! signedIO){
			svP = svX * svY;
			svR = svA<<lsbA;
			//align the product
			if(lsbPfull>=0)
				svR += svP << lsbPfull;
			else 
				svR += (svP >>(-lsbPfull));

			tc->addExpectedOutput("R", svR);
			if(possibleOutputs==2) {
				svR++;
				svR &= (twoToWR -1);
				tc->addExpectedOutput("R", svR);
			}
		}

		else{ // Manage signed digits
			mpz_class twoToWX = (mpz_class(1) << (wX));
			mpz_class twoToWXm1 = (mpz_class(1) << (wX-1));
			mpz_class twoToWY = (mpz_class(1) << (wY));
			mpz_class twoToWYm1 = (mpz_class(1) << (wY-1));
			mpz_class twoToWA = (mpz_class(1) << (wA));
			mpz_class twoToWAm1 = (mpz_class(1) << (wA-1));

			if (svX >= twoToWXm1)
				svX -= twoToWX;

			if (svY >= twoToWYm1)
				svY -= twoToWY;

			if (svA >= twoToWAm1)
				svA -= twoToWA;

			svP = svX * svY; //signed
			if(lsbPfull>=0) { // no truncation
				svR = (svA << lsbA) + (svP << lsbPfull); // signed
				// manage two's complement at output
				if ( svR < 0)
					svR += twoToWR; 
				tc->addExpectedOutput("R", svR);
				return;
			}
			else {//lsbPfull<0
				int shift=-lsbPfull;
				// fully accurate result, product-anchored
				svR = (svA<<(lsbA + shift)) + svP; 
				// manage its two's complement
				if ( svR < 0)
					svR += (mpz_class(1) << (wOut+shift));
				// shift back to place: truncation
				svR = svR >> shift;
				tc->addExpectedOutput("R", svR); // this was rounded down
				svR++;
				svR &= (twoToWR -1);
				tc->addExpectedOutput("R", svR);
			}
		}
	}



	void FixMultAdd::buildStandardTestCases(TestCaseList* tcl)
	{
#if 0
		TestCase *tc;

		mpz_class x, y;

		// 1*1
		x = mpz_class(1); 
		y = mpz_class(1); 
		tc = new TestCase(this); 
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);

		// -1 * -1
		x = (mpz_class(1) << wXdecl) -1; 
		y = (mpz_class(1) << wYdecl) -1; 
		tc = new TestCase(this); 
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);

		// The product of the two max negative values overflows the signed multiplier
		x = mpz_class(1) << (wXdecl -1); 
		y = mpz_class(1) << (wYdecl -1); 
		tc = new TestCase(this); 
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);
#endif
	}




}
