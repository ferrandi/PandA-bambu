/*
  Conversion from  FloPoCo format to IEEE-like compact floating-point format

  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon
  
  Author : Fabrizio Ferrandi ferrandi@elet.polimi.it

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, 2009. All right reserved.

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
#include "OutputIEEE.hpp"

using namespace std;

namespace flopoco{

#define DEBUGVHDL 0

	OutputIEEE::OutputIEEE(Target* target, int wEI, int wFI, int wEO, int wFO, bool onlyPositiveZeroes) :
		Operator(target), wEI(wEI), wFI(wFI), wEO(wEO), wFO(wFO), onlyPositiveZeroes(onlyPositiveZeroes)  {

		setCopyrightString("F. Ferrandi  (2009-2012)");

		ostringstream name;

		name<<"OutputIEEE_"<<wEI<<"_"<<wFI<<"_to_"<<wEO<<"_"<<wFO;

		uniqueName_ = name.str(); 

		// -------- Parameter set up -----------------

		addFPInput ("X", wEI, wFI);
		addOutput("R", wEO+wFO+1);

		vhdl << tab << declare("expX", wEI) << "  <= X" << range(wEI+wFI-1, wFI) << ";" << endl;
		vhdl << tab << declare("fracX", wFI) << "  <= X" << range(wFI-1, 0) << ";" << endl;
		vhdl << tab << declare("exnX",2) << "  <= X" << range(wEI+wFI+2, wEI+wFI+1) << ";" << endl;
		if(onlyPositiveZeroes)
		        vhdl << tab << declare("sX") << "  <= X(" << wEI+wFI << ") when (exnX = \"01\" or exnX = \"10\") else '0';" << endl;
		else
		        vhdl << tab << declare("sX") << "  <= X(" << wEI+wFI << ") when (exnX = \"01\" or exnX = \"10\" or exnX = \"00\") else '0';" << endl;
		manageCriticalPath(target->localWireDelay() + target->lutDelay());
		vhdl << tab << declare("expZero") << "  <= '1' when expX = " << rangeAssign(wEI-1,0, "'0'") << " else '0';" << endl;
		if(wEI==wEO){ 
			vhdl << tab << "-- since we have one more exponent value than IEEE (field 0...0, value emin-1)," << endl 
				  << tab << "-- we can represent subnormal numbers whose mantissa field begins with a 1" << endl;

			if(wFO>=wFI){
				manageCriticalPath(target->localWireDelay() + target->lutDelay());
				vhdl << tab << declare("sfracX",wFI) << " <= " << endl
						<< tab << tab << rangeAssign(wFI-1,0, "'0'") << " when (exnX = \"00\") else" << endl
						<< tab << tab << "'1' & fracX" << range(wFI-1,1) << " when (expZero = '1' and exnX = \"01\") else" << endl
						<< tab << tab << "fracX when (exnX = \"01\") else " << endl
						<< tab << tab << rangeAssign(wFI-1,1, "'0'") << " & exnX(0);" << endl;
				vhdl << tab << declare("fracR",wFO) << " <= " << "sfracX";
				if(wFO>wFI) // need to pad with 0s
					vhdl << " & CONV_STD_LOGIC_VECTOR(0," << wFO-wFI <<");" << endl;
				else 
					vhdl << ";" << endl;
				vhdl << tab << declare("expR",wEO) << " <=  " << endl
						<< tab << tab << rangeAssign(wEO-1,0, "'0'") << " when (exnX = \"00\") else" << endl
						<< tab << tab << "expX when (exnX = \"01\") else " << endl
						<< tab << tab << rangeAssign(wEO-1,0, "'1'") << ";" << endl;

			}
			else { // wFI > wFO, wEI==wEO
				manageCriticalPath(target->localWireDelay() + target->lutDelay());
				vhdl << tab << declare("sfracX",wFI) << " <= '1' & fracX" << range(wFI-1,1) << " when (expZero = '1' and exnX = \"01\") else fracX;" << endl;
				vhdl << tab << "-- wFO < wFI, need to round fraction" << endl;
				vhdl << tab << declare("resultLSB") << " <= sfracX("<< wFI-wFO <<");" << endl;
				vhdl << tab << declare("roundBit") << " <= sfracX("<< wFI-wFO-1 <<");" << endl;
				// need to define a sticky bit
				vhdl << tab << declare("sticky") << " <= ";
				if(wFI-wFO>1){
					manageCriticalPath(target->localWireDelay() + target->lutDelay());
					vhdl<< " '0' when sfracX" << range(wFI-wFO-2, 0) <<" = CONV_STD_LOGIC_VECTOR(0," << wFI-wFO-2 <<") else '1';"<<endl;
				}
				else {
					vhdl << "'0';" << endl; 
				} // end of sticky computation
				manageCriticalPath(target->localWireDelay() + target->lutDelay());
				vhdl << tab << declare("round") << " <= roundBit and (sticky or resultLSB);"<<endl;

				vhdl << tab << "-- The following addition will not overflow since FloPoCo format has one more exponent value" <<endl;
				manageCriticalPath(target->localWireDelay() + target->adderDelay(wEO+wFO));
				vhdl << tab << declare("expfracR0", wEO+wFO) << " <= (expX & sfracX" << range(wFI-1, wFI-wFO) << ")  +  (CONV_STD_LOGIC_VECTOR(0," << wEO+wFO-1 <<") & round);"<<endl;

				manageCriticalPath(target->localWireDelay() + target->lutDelay());
				vhdl << tab << declare("fracR",wFO) << " <= " << endl
						<< tab << tab << rangeAssign(wFO-1,0, "'0'") << " when (exnX = \"00\") else" << endl
						<< tab << tab << "expfracR0" << range(wFO-1, 0) << " when (exnX = \"01\") else " << endl
						<< tab << tab << rangeAssign(wFO-1,1, "'0'") << " & exnX(0);" << endl;

				vhdl << tab << declare("expR",wEO) << " <=  " << endl
						<< tab << tab << rangeAssign(wEO-1,0, "'0'") << " when (exnX = \"00\") else" << endl
						<< tab << tab << "expfracR0" << range(wFO+wEO-1, wFO) << " when (exnX = \"01\") else " << endl
						<< tab << tab << rangeAssign(wEO-1,0, "'1'") << ";" << endl;
			}

		}



		else if (wEI<wEO) { // No overflow possible. Subnormal inputs need to be normalized
			throw  string("OutputIEEE not yet implemented for wEI<wEO, send us a mail if you need it");
			// cout << "Warning: subnormal inputs would be representable in the destination format,\n   but will be flushed to zero anyway (TODO)" << endl;
		}



		else {
			throw  string("OutputIEEE not yet implemented for wEI>wEO, send us a mail if you need it");
		}
	
		vhdl << tab << "R <= sX & expR & fracR; " << endl; 

	}

	OutputIEEE::~OutputIEEE() {
	}






	void OutputIEEE::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");

		/* Compute correct value */
		FPNumber fpx(wEI, wFI, svX);
		mpfr_t x, r;
		mpfr_init2(x, 1+wFI);
		mpfr_init2(r, 1+wFO); 
		fpx.getMPFR(x);

		mpfr_set(r, x, GMP_RNDN); ///TODO probably not enough
		FPNumber  fpr(wEO, wFO, r);

		/* Set outputs */
		mpz_class svr= fpr.getSignalValue();
		// Remove FloPoCo encoding
		mpz_class negative  = mpz_class(1)<<(wEO+wFO);
		if((((mpz_class(1)<<(wEO+wFO+2)) & svr) != 0) && (((mpz_class(1)<<(wEO+wFO+1)) & svr) != 0))
		{
		        svr = (((mpz_class(1)<<wEO)-1) << wFO) + 1;
		}
		else if((mpz_class(1)<<(wEO+wFO+2) & svr) != 0)
		{
		        svr = (svr & negative) + (((mpz_class(1)<<wEO)-1) << wFO);
		}
		else if((mpz_class(1)<<(wEO+wFO+1) & svr) != 0)
		        svr = svr & ((mpz_class(1)<<(1+wFO+wEO))-1);
		else if(onlyPositiveZeroes)
		        svr = svr & ((mpz_class(1)<<(wFO+wEO))-1);
		else
		        svr = svr & ((mpz_class(1)<<(1+wFO+wEO))-1);

		tc->addExpectedOutput("R", svr);

		mpfr_clears(x, r, NULL);
	}

}

