/*
  Conversion from IEEE-like compact floating-point format to FloPoCo format
  
  Author:  Florent de Dinechin

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
#include "InputIEEE.hpp"

using namespace std;


namespace flopoco{


#define DEBUGVHDL 0

	InputIEEE::InputIEEE(Target* target, int wEI, int wFI, int wEO, int wFO, bool flushToZero) :
		Operator(target), wEI(wEI), wFI(wFI), wEO(wEO), wFO(wFO), flushToZero(flushToZero){


		setCopyrightString("Florent de Dinechin (2008)");		

		ostringstream name;

		name<<"InputIEEE_"<<wEI<<"_"<<wFI<<"_to_"<<wEO<<"_"<<wFO;

		uniqueName_ = name.str(); 

		// -------- Parameter set up -----------------

		addIEEEInput ("X", wEI,wFI);
		addFPOutput("R", wEO, wFO);

		vhdl << tab << declare("expX", wEI) << "  <= X" << range(wEI+wFI-1, wFI) << ";" << endl;
		vhdl << tab << declare("fracX", wFI) << "  <= X" << range(wFI-1, 0) << ";" << endl;
		vhdl << tab << declare("sX") << "  <= X(" << wEI+wFI << ");" << endl;	

		// There are three exponent cases to consider:
		// wEI==wEO is the easiest case, with one subtelty:  
		//    we have two more exponent values than IEEE (field 0...0, value -(1<<wEI)+1, and field 11..11, value 1<<wEI),
		//    we may thus convert into normal numbers input values whose mantissa field begins with a 1
		//    Other subnormals are flushed to zero
		// wEI > wEO (range downgrading) is probably the most useful, as we want to minimize the precision of the FPGA computation
		//    with respect to a software implementation 
		//    Anyway it is fairly easy to implement, too: subnormals are all flushed to zero, and some normal numbers may overflow or underflow.
		// wEI < wEO (range widening) may be useful to some, but I don't see whom yet :) mail me if you want it implemented.
		//    It will  be costly, since all input subnormals will need to be converted into normal numbers by means of a barrel shifter.

		// In each exponent case, the mantissa may be copied (if wFI<=wFO -- taking care of the above subtlety,
		// or it must be rounded if wFI>wFO (again, probably the most useful case)
		// If wEI=WEO, the rounding may not lead to an overflow since we have this one more large exponent value, 
		// If wEI<WEO, the rounding may lead to an overflow 
		// This is reasonably cheap.

		// analyze the input exponent
		manageCriticalPath(target->localWireDelay() + target->lutDelay());
		vhdl << tab << declare("expZero") << "  <= '1' when expX = " << rangeAssign(wEI-1,0, "'0'") << " else '0';" << endl;
		vhdl << tab << declare("expInfty") << "  <= '1' when expX = " << rangeAssign(wEI-1,0, "'1'") << " else '0';" << endl;
		vhdl << tab << declare("fracZero") << " <= '1' when fracX = " << rangeAssign(wFI-1,0, "'0'") << " else '0';" << endl;


		if(wEI==wEO){ 
			vhdl << tab << declare("reprSubNormal") << " <= fracX(" << wFI-1 << ");" << endl;
			vhdl << tab << "-- since we have one more exponent value than IEEE (field 0...0, value emin-1)," << endl 
				  << tab << "-- we can represent subnormal numbers whose mantissa field begins with a 1" << endl;
		        manageCriticalPath(target->localWireDelay() + target->lutDelay());
			vhdl << tab << declare("sfracX",wFI) << " <= fracX" << range(wFI-2,0) << " & '0' when (expZero='1' and reprSubNormal='1')    else fracX;" << endl;

			if(wFO>=wFI){
				vhdl << tab << declare("fracR",wFO) << " <= " << "sfracX";
				if(wFO>wFI) // need to pad with 0s
					vhdl << " & CONV_STD_LOGIC_VECTOR(0," << wFO-wFI <<");" << endl;
				else 
					vhdl << ";" << endl;
				vhdl << tab << "-- copy exponent. This will be OK even for subnormals, zero and infty since in such cases the exn bits will prevail" << endl;
				vhdl << tab << declare("expR", wEO) << " <= expX;" << endl;
			}
			else { // wFI > wFO, wEI==wEO
				vhdl << tab << "-- wFO < wFI, need to round fraction" << endl;
				vhdl << tab << declare("resultLSB") << " <= sfracX("<< wFI-wFO <<");" << endl;
				vhdl << tab << declare("roundBit") << " <= sfracX("<< wFI-wFO-1 <<");" << endl;
				// need to define a sticky bit
				vhdl << tab << declare("sticky") << " <= ";
				if(wFI-wFO>1){
				        manageCriticalPath(target->localWireDelay() + target->lutDelay());
					vhdl<< " '0' when sfracX" << range(wFI-wFO-2, 0) <<" = CONV_STD_LOGIC_VECTOR(0," << wFI-wFO-2 <<")   else '1';"<<endl;
				}
				else {
					vhdl << "'0';" << endl; 
				} // end of sticky computation
				manageCriticalPath(target->localWireDelay() + target->lutDelay());
				vhdl << tab << declare("round") << " <= roundBit and (sticky or resultLSB);"<<endl;

				vhdl << tab << "-- The following addition will not overflow since FloPoCo format has one more exponent value" <<endl; 
				manageCriticalPath(target->localWireDelay() + target->adderDelay(wEO+wFO));
				vhdl << tab << declare("expfracR0", wEO+wFO) << " <= (expX & sfracX" << range(wFI-1, wFI-wFO) << ")  +  (CONV_STD_LOGIC_VECTOR(0," << wEO+wFO-1 <<") & round);"<<endl;
				vhdl << tab << declare("fracR",wFO) << " <= expfracR0" << range(wFO-1, 0) << ";" << endl;
				vhdl << tab << declare("expR",wEO) << " <= expfracR0" << range(wFO+wEO-1, wFO) << ";" << endl;
			}

			vhdl << tab << declare("infinity") << " <= expInfty and fracZero;" << endl;
			vhdl << tab << declare("zero")     << " <= expZero and not reprSubNormal;" << endl;
			vhdl << tab << declare("NaN")      << " <= expInfty and not fracZero;" << endl;

			vhdl << tab << declare("exnR",2) << " <= " << endl
				  << tab << tab << "     \"00\" when zero='1' " << endl
				  << tab << tab << "else \"10\" when infinity='1' " << endl
				  << tab << tab << "else \"11\" when NaN='1' " << endl
				  << tab << tab << "else \"01\" ;  -- normal number" << endl;
		}



		else if (wEI<wEO) { // No overflow possible. Subnormal inputs need to be normalized
			REPORT(INFO, "Warning: subnormal inputs would be representable in the destination format,\n   but will be flushed to zero anyway (TODO). Please complain to the FloPoCo team if you need correct rounding");
			int32_t biasI = (1<<(wEI-1)) -1;
			int32_t eMaxO = (1<<(wEO-1)); // that's our maximal exponent, one more than IEEE's
			vhdl << tab << declare("overflow") << " <= '0';--  overflow never happens for these (wE_in, wE_out)"  << endl;
			vhdl << tab << declare("underflow") << " <= '0';--  underflow never happens for these (wE_in, wE_out)" << endl;
			// We have to compute ER = E_X - bias(wE_in) + bias(wE_R)
			// Let us pack all the constants together
			mpz_class expAddend = -biasI + eMaxO-1;
			manageCriticalPath(target->localWireDelay() + target->adderDelay(wEO));
			vhdl << tab << declare("expR",    wEO) << " <= "
					 << "(" << rangeAssign(wEO-1, wEI, "'0'") << "  & expX) + "
					 << "\"" << unsignedBinary(expAddend, wEO) << "\""
					 << ";"<<endl;
			if(wFO>=wFI){ // no rounding needed
				vhdl << tab << declare("fracR",wFO) << " <= " << "fracX";
				if(wFO>wFI) // need to pad with 0s
					vhdl << " & CONV_STD_LOGIC_VECTOR(0," << wFO-wFI <<");" << endl;
				else 
					vhdl << ";" << endl;
				vhdl << tab << declare("roundOverflow") << " <= '0';" << endl;
			}
			else
				throw  string("InputIEEE not yet implemented for wEI<wEO and wFI>wFO, send us a mail if you need it");

			vhdl << tab << declare("NaN") << " <= expInfty and not fracZero;" << endl;
			vhdl << tab << declare("infinity") << " <= (expInfty and fracZero) or (not NaN and (overflow or roundOverflow));" << endl;
			vhdl << tab << declare("zero") << " <= expZero or underflow;" << endl ;

			vhdl << tab << declare("exnR",2) << " <= " << endl
				  << tab << tab << "     \"11\" when NaN='1' " << endl
				  << tab << tab << "else \"10\" when infinity='1' " << endl
				  << tab << tab << "else \"00\" when zero='1' " << endl
				  << tab << tab << "else \"01\" ;  -- normal number" << endl;
		}



		else { // wEI>wEO: some exponents will lead to over/underflow. All subnormals are flushed to zero
			int32_t eMinO = -((1<<(wEO-1))-1); // that's our minimal exponent, equal to -bias
			int32_t biasI = (1<<(wEI-1)) -1;
			underflowThreshold = eMinO+biasI; // positive since wEI>wEO.
			int32_t eMaxO = (1<<(wEO-1)); // that's our maximal exponent, one more than IEEE's
			overflowThreshold = eMaxO+biasI;
			vhdl << tab << "-- min exponent value without underflow, biased with input bias: " << underflowThreshold << endl ;
			manageCriticalPath(target->localWireDelay() + target->adderDelay(wEI+1));
			vhdl << tab << declare("unSub",wEI+1) << " <= ('0' & expX) - CONV_STD_LOGIC_VECTOR(" << underflowThreshold << "," << wEI+1 <<");" << endl;
			vhdl << tab << declare("underflow") << " <= unSub(" << wEI << ");" << endl;

			manageCriticalPath(target->localWireDelay() + target->adderDelay(wEI+1));
			vhdl << tab << "-- max exponent value without overflow, biased with input bias: " << overflowThreshold << endl ;
			vhdl << tab << declare("ovSub",wEI+1) << " <= CONV_STD_LOGIC_VECTOR(" << overflowThreshold << "," << wEI+1 <<")  -  ('0' & expX);" << endl;
			vhdl << tab << declare("overflow") << " <= ovSub(" << wEI << ");" << endl;
			vhdl << tab << "-- copy exponent. Result valid only in the absence of ov/underflow" << endl;
			// wEI>wEO therefore biasI>biasO
			// have to compute expR = ((expX-biasI) + biasO) (wEO-1 downto 0)
			// but the wEO-1 LSB bits of both biases are identical, therefore simply copy 
			// and the remaining MSB are 1 for input and 0 for output, therefore subtract the leading 1 by inverting it
			manageCriticalPath(target->localWireDelay() + target->lutDelay());
			vhdl << tab << declare("expXO", wEO) << " <= (not expX(" << wEO-1 << ")) & expX" << range(wEO-2, 0) << ";" << endl;

			if(wFO>=wFI){ // no rounding needed
				vhdl << tab << declare("fracR",wFO) << " <= " << "fracX";
				if(wFO>wFI) // need to pad with 0s
					vhdl << " & CONV_STD_LOGIC_VECTOR(0," << wFO-wFI <<");" << endl;
				else 
					vhdl << ";" << endl;
				vhdl << tab << declare("expR",wEO) << " <= expXO;" << endl;
				vhdl << tab << declare("roundOverflow") << " <= '0';" << endl;
			}
			else { // wFI > wFO, wEI>wEO

				vhdl << tab << "-- wFO < wFI, need to round fraction" << endl;
				vhdl << tab << declare("resultLSB") << " <= fracX("<< wFI-wFO <<");" << endl;
				vhdl << tab << declare("roundBit") << " <= fracX("<< wFI-wFO-1 <<");" << endl;
				// need to define a sticky bit
				vhdl << tab << declare("sticky") << " <= ";
				if(wFI-wFO>1)
				{
					manageCriticalPath(target->localWireDelay() + target->lutDelay());
					vhdl<< " '0' when fracX" << range(wFI-wFO-2, 0) <<" = CONV_STD_LOGIC_VECTOR(0," << wFI-wFO-2 <<")   else '1';"<<endl;
				}
				else 
					vhdl << "'0';" << endl;
				manageCriticalPath(target->localWireDelay() + target->lutDelay());
				vhdl << tab << declare("round") << " <= roundBit and (sticky or resultLSB);"<<endl;

				vhdl << tab << "-- The following addition may overflow" <<endl;
				manageCriticalPath(target->localWireDelay() + target->adderDelay(wEO+wFO+1));
				vhdl << tab << declare("expfracR0", wEO+wFO+1) << " <= ('0' & expXO & fracX" << range(wFI-1, wFI-wFO) << ")  +  (CONV_STD_LOGIC_VECTOR(0," << wEO+wFO <<") & round);"<<endl;
				vhdl << tab << declare("roundOverflow") << " <= expfracR0(" << wEO+wFO << ");" << endl;

				vhdl << tab << declare("fracR",wFO) << " <= expfracR0" << range(wFO-1, 0) << ";" << endl;
				vhdl << tab << declare("expR",wEO) << " <= expfracR0" << range(wFO+wEO-1, wFO) << ";" << endl;
			}

			vhdl << tab << declare("NaN") << " <= expInfty and not fracZero;" << endl;
			vhdl << tab << declare("infinity") << " <= (expInfty and fracZero) or (not NaN and (overflow or roundOverflow));" << endl;
			vhdl << tab << declare("zero") << " <= expZero or underflow;" << endl ;

			vhdl << tab << declare("exnR",2) << " <= " << endl
				  << tab << tab << "     \"11\" when NaN='1' " << endl
				  << tab << tab << "else \"10\" when infinity='1' " << endl
				  << tab << tab << "else \"00\" when zero='1' " << endl
				  << tab << tab << "else \"01\" ;  -- normal number" << endl;
		}

		vhdl << tab << "R <= exnR & sX & expR & fracR; " << endl; 

	}

	InputIEEE::~InputIEEE() {
	}






	void InputIEEE::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");
		mpz_class sgnX = (svX >> (wFI+wEI));
		mpz_class expX = (svX >> wFI) & ((mpz_class(1)<<wEI)-1);
		mpz_class fracX = svX & ((mpz_class(1)<<wFI)-1);
		mpfr_t x;
		mpfr_init2(x, 1+wFI);
	
		if (wEI!=11 || wFI!=52)
			throw string("InputIEEE::emulate(): only double-precision inputs supported");
		else {
			union {double d; uint32_t i[2];} xx;
#if 1 // little-endian
			mpz_class t;
			t = (svX >> 32);
			xx.i[1] = t.get_ui();
			t = svX - (mpz_class(xx.i[1])<<32);
			xx.i[0] = t.get_ui() ;
#else //big-endian
			t = (svX >> 32);
			xx.i[0] = t.get_ui();
			t = svX - (mpz_class(xx.i[0])<<32);
			xx.i[1] = t.get_ui() ;
#endif
			//cout << "Double input to emulate: " << xx.d << endl;

			mpfr_set_d(x, xx.d, GMP_RNDN);

			mpfr_t r;
			mpfr_init2(r, 1+wFO); 
			mpfr_set(r, x, GMP_RNDN);
			FPNumber  fpr(wEO, wFO, r);

			/* Set outputs */
			mpz_class svr= fpr.getSignalValue();
			tc->addExpectedOutput("R", svr);

			mpfr_clears(x, r, NULL);
		}
	}

	void InputIEEE::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;
		mpz_class x, r;

		tc = new TestCase(this); 
		tc->addComment("a typical normal number: 1.0");
		x = ((mpz_class(1) << 10)-1) << 52 ;
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addComment("another one: -1.0");
		x += (mpz_class(1) << 63);
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addComment("a subnormal that is converted to a normal number");
		x = mpz_class(1) << 51;
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);	

		if(wFO==52 && wEO==11) {
			tc = new TestCase(this);
			tc->addComment("the same, but defined explicitely (to check emulate())");
			x = mpz_class(1) << 51;
			tc->addInput("X", x);
			r = mpz_class(1) << 64; // normal number, exp=0, mantissa=1
			tc->addExpectedOutput("R", r);
			tcl->add(tc);
		}

		tc = new TestCase(this);
		tc->addComment("the same, negative");
		x += (mpz_class(1) << 63);
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addComment("a subnormal that is flushed to zero");
		x = mpz_class(1) << 50;
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addComment("the same, negative");
		x += (mpz_class(1) << 63);
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addComment("another subnormal that is flushed to zero");
		x = mpz_class(1) << 49;
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addComment("the same, negative");
		x += (mpz_class(1) << 63);
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addComment("The largest finite number. ");
		x = (((mpz_class(1) << 11)-1) << 52) -1 ;
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		if (wEI>wEO && wFI>wFO) {
			tc = new TestCase(this);
			tc->addComment("a number whose rounding will trigger an overflow");
			x =  overflowThreshold << wFI; // maximal exponent
			x += ((mpz_class(1) << wFI)-1); // largest mantissa
			tc->addInput("X", x);
			emulate(tc);
			tcl->add(tc);

			tc = new TestCase(this);
			tc->addComment("just to check: the previous input minus one ulp");
			x -= 1; 
			tc->addInput("X", x);
			emulate(tc);
			tcl->add(tc);
		}

		tc = new TestCase(this);
		tc->addComment("the same, negative");
		x += (mpz_class(1) << 63);
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addComment("Plus infty");
		x = ((mpz_class(1) << 11)-1) << 52 ;
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addComment("Minus infty");
		x += (mpz_class(1) << 63);
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this);
		tc->addComment("NaN");
		x = (((mpz_class(1) << 11)-1) << 52 ) + 12;
		tc->addInput("X", x);
		emulate(tc);
		tcl->add(tc);


	}


}
