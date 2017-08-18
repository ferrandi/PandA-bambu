/*
  Floating Point Square Root for FloPoCo
 
  Authors : 
  Jeremie Detrey, Florent de Dinechin (digit-recurrence version)

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

 */
 


#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "IntAdder.hpp"
#include "IntMultiplier.hpp"
#include "IntSquarer.hpp"
#include "FPSqrt.hpp"

using namespace std;

namespace flopoco{

#define DEBUGVHDL 0
	//#define LESS_DSPS

	FPSqrt::FPSqrt(Target* target, int wE, int wF) :
		Operator(target), wE(wE), wF(wF) {

		ostringstream name;

		name<<"FPSqrt_"<<wE<<"_"<<wF;

		uniqueName_ = name.str(); 

		// -------- Parameter set up -----------------

		addFPInput ("X", wE, wF);
		addFPOutput("R", wE, wF);


		// Digit-recurrence implementation recycled from FPLibrary
		//cout << "   DDDD" <<  target->adderDelay(10) << "  " <<  target->localWireDelay() << "  " << target->lutDelay();
		vhdl << tab << declare("fracX", wF) << " <= X" << range(wF-1, 0) << "; -- fraction"  << endl; 
		vhdl << tab << declare("eRn0", wE) << " <= \"0\" & X" << range(wE+wF-1, wF+1) << "; -- exponent" << endl;
		vhdl << tab << declare("xsX", 3) << " <= X"<< range(wE+wF+2, wE+wF) << "; -- exception and sign" << endl;

		vhdl << tab << declare("eRn1", wE) << " <= eRn0 + (\"00\" & " << rangeAssign(wE-3, 0, "'1'") << ") + X(" << wF << ");" << endl;

		vhdl << tab << declare(join("w",wF+3), wF+4) << " <= \"111\" & fracX & \"0\" when X(" << wF << ") = '0' else" << endl
		     << tab << "       \"1101\" & fracX;" << endl;
		//		vhdl << tab << declare(join("d",wF+3)) << " <= '0';" << endl;
		//		vhdl << tab << declare(join("s",wF+3)) << " <= '1';" << endl;

		double delay= target->lutDelay() + target->localWireDelay() + target->ffDelay(); // estimated delay so far (one mux)
		for(int step=1; step<=wF+2; step++) {
		  int i = wF+3-step; // to have the same indices as FPLibrary 
		  vhdl << tab << "-- Step " << i << endl;
		  string di = join("d", i);
		  string xi = join("x", i);
		  string wi = join("w", i);
		  string wip = join("w", i+1);
		  string si = join("s", i);
		  string sip = join("s", i+1);
		  //			string zs = join("zs", i);
		  string ds = join("ds", i);
		  string xh = join("xh", i);
		  string wh = join("wh", i);
		  vhdl << tab << declare(di) << " <= "<< wip << "("<< wF+3<<");" << endl;
		  vhdl << tab << declare(xi,wF+5) << " <= " << wip << " & \"0\";" << endl;
		  vhdl << tab << declare(ds,step+3) << " <=  \"0\" & ";
		  if (step>1)
		    vhdl 	<< sip << " & ";
		  vhdl << " (not " << di << ") & " << di << " & \"1\";" << endl;
		  vhdl << tab << declare(xh,step+3) << " <= " << xi << range(wF+4, wF+2-step) << ";" << endl;
		  vhdl << tab << "with " << di << " select" << endl
		       << tab << tab <<  declare(wh, step+3) << " <= " << xh << " - " << ds << " when '0'," << endl
		       << tab << tab << "      " << xh << " + " << ds << " when others;" << endl;
		  vhdl << tab << declare(wi, wF+4) << " <= " << wh << range(step+1,0);
		  if(step <= wF+1) 
		    vhdl << " & " << xi << range(wF+1-step, 0) << ";" << endl;  
		  else
		    vhdl << ";" << endl; 
		  vhdl << tab << declare(si, step) << " <= ";
		  if(step==1)
		    vhdl << "\"\" & (not " << di << ") ;"<< endl; 
		  else
		    vhdl << sip /*<< range(step-1,1)*/ << " & not " << di << ";"<< endl; 
				
		  // Pipeline management
		  double stageDelay= target->adderDelay(step) + target->localWireDelay() + 2*target->lutDelay();
		  delay += stageDelay;
		  if (verbose>=2) {
		    cout << "estimated delay for stage "<< step << " is " << stageDelay << "s" << endl;
		    cout << "   cumulated delay would be " << delay << "s,   target is " << 1/target->frequency()<< endl;
		  }
		  if(delay > 1/target->frequency()) {
		    // insert a pipeline register and reset the cumulated delay
		    nextCycle();
		    delay= target->ffDelay() + stageDelay;
		    if (verbose>=2) 
		      cout << "----inserted a register level" << endl;
		  }
		}
		vhdl << tab << declare("d0") << " <= w1(" << wF+3 << ") ;" << endl;
		vhdl << tab << declare("fR", wF+4) << " <= s1 & not d0 & '1';" << endl;

		// end of component FPSqrt_Sqrt in fplibrary
		vhdl << tab << "-- normalisation of the result, removing leading 1" << endl;
		vhdl << tab <<  "with fR(" << wF+3 << ") select" << endl
		     << tab << tab << declare("fRn1", wF+2) << " <= fR" << range(wF+2, 2) << " & (fR(1) or fR(0)) when '1'," << endl
		     << tab << tab << "        fR" <<range(wF+1, 0) << "                    when others;" << endl;
		vhdl << tab << declare("round") << " <= fRn1(1) and (fRn1(2) or fRn1(0)) ; -- round  and (lsb or sticky) : that's RN, tie to even" << endl;

		nextCycle();
		
		vhdl << tab << declare("fRn2", wF) << " <= fRn1" << range(wF+1, 2) <<" + (" << rangeAssign(wF-1, 1, "'0'") << " & round); -- rounding sqrt never changes exponents " << endl;
		vhdl << tab << declare("Rn2", wE+wF) << " <= eRn1 & fRn2;" << endl;
		
		vhdl << tab << "-- sign and exception processing" << endl;
		vhdl << tab <<  "with xsX select" << endl
		     << tab << tab << declare("xsR", 3) << " <= \"010\"  when \"010\",  -- normal case" << endl
		     << tab << tab <<  "       \"100\"  when \"100\",  -- +infty" << endl
		     << tab << tab <<  "       \"000\"  when \"000\",  -- +0" << endl
		     << tab << tab <<  "       \"001\"  when \"001\",  -- the infamous sqrt(-0)=-0" << endl
		     << tab << tab <<  "       \"110\"  when others; -- return NaN" << endl;
			
		vhdl << tab << "R <= xsR & Rn2; " << endl; 
	}
  
  FPSqrt::~FPSqrt() {
  }






		void FPSqrt::emulate(TestCase * tc)
		{
			/* Get I/O values */
			mpz_class svX = tc->getInputValue("X");

			/* Compute correct value */
			FPNumber fpx(wE, wF);
			fpx = svX;
			mpfr_t x, r;
			mpfr_init2(x, 1+wF);
			mpfr_init2(r, 1+wF); 
			fpx.getMPFR(x);

			if(correctRounding) {
				mpfr_sqrt(r, x, GMP_RNDN);
				FPNumber  fpr(wE, wF, r);
				/* Set outputs */
				mpz_class svr= fpr.getSignalValue();
				tc->addExpectedOutput("R", svr);
			}
			else { // faithful rounding 
				mpfr_sqrt(r, x, GMP_RNDU);
				FPNumber  fpru(wE, wF, r);
				mpz_class svru = fpru.getSignalValue();
				tc->addExpectedOutput("R", svru);

				mpfr_sqrt(r, x, GMP_RNDD);
				FPNumber  fprd(wE, wF, r);
				mpz_class svrd = fprd.getSignalValue();
				/* Set outputs */
				tc->addExpectedOutput("R", svrd);
			}

			mpfr_clears(x, r, NULL);
		}




		// One test out of 4 fully random (tests NaNs etc)
		// All the remaining ones test positive numbers.
		TestCase* FPSqrt::buildRandomTestCase(int i){

			TestCase *tc;
			mpz_class a;

			tc = new TestCase(this); 
			/* Fill inputs */
			if ((i & 3) == 0)
				a = getLargeRandom(wE+wF+3);
			else
				a  = getLargeRandom(wE+wF) + (mpz_class(1)<<(wE+wF+1)); // 010xxxxxx
			tc->addInput("X", a);

			/* Get correct outputs */
			emulate(tc);

			return tc;
		}
	}
