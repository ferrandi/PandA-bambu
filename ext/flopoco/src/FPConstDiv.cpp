/*
   A divider by a floating-point constant for FloPoCo

  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.
*/

 


#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "FPConstDiv.hpp"
#include "FPNumber.hpp"

using namespace std;


namespace flopoco{


	// The expert version 

	FPConstDiv::FPConstDiv(Target* target, int wE_in_, int wF_in_, int wE_out_, int wF_out_, int d_, int dExp_, int alpha_):
		Operator(target), 
		wE_in(wE_in_), wF_in(wF_in_), wE_out(wE_out_), wF_out(wF_out_), d(d_), dExp(dExp_), alpha(alpha_)
	{
		srcFileName="FPConstDiv";
		ostringstream name;
		name <<"FPConstDiv_"<<wE_in<<"_"<<wF_in<<"_"<<wE_out<<"_"<<wF_out<<"_"<<d<<"_";
		if(dExp>=0)
			name<<dExp<<"_";
		else
			name<<"M"<<-dExp<<"_";
		if(target->isPipelined()) 
			name << target->frequencyMHz() ;
		else
			name << "comb";
		uniqueName_ = name.str();
		
		uniqueName_=name.str();

		if(wE_in<3 || wE_out <3){
			ostringstream error;
			error << srcFileName << " (" << uniqueName_ << "): exponent size must be at least 3" <<endl;
			throw error.str();
		}

		if(d==0) {
			ostringstream o;
			o << srcFileName << " (" << uniqueName_ << "): ERROR in FPConstDiv, division by 0";
			throw o.str();
		}

			// Constant normalization
		while ((d % 2) ==0) {
				REPORT(DEBUG, "d is even, normalising");
				d = d >>1;
				dExp+=1;
		}
		mantissaIsOne = false;
		if(d==1) {
			REPORT(INFO, "Mantissa of d is 1, dividing by it will be easy"); 
			mantissaIsOne = true;
		}
		
		// Computing the binary64 value of the actual constant, taking exponent into account
		dd = d*intpow2(dExp);

		REPORT(INFO, "dividing by the constant " << dd);



		// Set up the IO signals
		addFPInput("X", wE_in, wF_in);
		addFPOutput("R", wE_out, wF_out);

		setCopyrightString("Florent de Dinechin (2007-2011)");

		int gamma = intlog2(d);
		int s = gamma-1;
		int h = d>>1; 
		int intDivSize = wF_in+1 + s+1;

		vhdl << tab << declare("x_exn",2) << " <=  X("<<wE_in<<"+"<<wF_in<<"+2 downto "<<wE_in<<"+"<<wF_in<<"+1);"<<endl;
		vhdl << tab << declare("x_sgn") << " <=  X("<<wE_in<<"+"<<wF_in<<");"<<endl;
		vhdl << tab << declare("x_exp", wE_in) << " <=  X("<<wE_in<<"+"<<wF_in<<"-1 downto "<<wF_in<<");"<<endl;
		vhdl << tab << declare("x_sig", wF_in+1) << " <= '1' & X("<<wF_in-1 <<" downto 0);"<<endl;

		manageCriticalPath(target->localWireDelay() + target->adderDelay(gamma+1));
		vhdl << tab << declare("Diffmd", gamma+1) << " <=  ('0' & x_sig" << range(wF_in, wF_in-gamma+1)<< ") - ('0' & CONV_STD_LOGIC_VECTOR(" << d << ", " << gamma <<")) ;" << endl;
		vhdl << tab << declare("mltd") << " <=   Diffmd("<< gamma<<");" << endl;

		int mltdCycle=getCurrentCycle();
		double mltdCP=getCriticalPath();

		if(d*intpow2(dExp) >1.0) { // only underflow possible
			vhdl <<endl << tab << "-- exponent processing. For this d we may only have underflow" << endl;
			
			manageCriticalPath(target->localWireDelay() + target->adderDelay(wE_out+1));
			vhdl << tab << declare("r_exp0", wE_out+1) << " <=  ('0' & x_exp) - ( CONV_STD_LOGIC_VECTOR(" << s+1+dExp << ", " << wE_out+1 <<")) + (not mltd);" << endl;
			
			vhdl << tab << declare("underflow") << " <=  r_exp0(" << wE_out << ");" << endl;
			vhdl << tab << declare("r_exp", wE_out) << " <=  r_exp0" << range(wE_out-1, 0) << ";" << endl;
			
			vhdl <<endl << tab << "-- exception flag processing"<<endl; 
			vhdl << tab << declare("r_exn", 2) << " <=  \"00\" when  x_exn=\"01\" and underflow='1' else x_exn" << ";" << endl;
		}
		else
			{
			vhdl <<endl << tab << "-- exponent processing. For this d we may only have overflow" << endl;
			
			manageCriticalPath(target->localWireDelay() + target->adderDelay(wE_out+1));
			vhdl << tab << declare("r_exp0", wE_out+1) << " <=  ('0' & x_exp) + ( CONV_STD_LOGIC_VECTOR(" << -(s+1+dExp) << ", " << wE_out+1 <<")) + (not mltd);" << endl;
			
			vhdl << tab << declare("overflow") << " <=  r_exp0(" << wE_out << ");" << endl;
			vhdl << tab << declare("r_exp", wE_out) << " <=  r_exp0" << range(wE_out-1, 0) << ";" << endl;
			
			vhdl <<endl << tab << "-- exception flag processing"<<endl; 
			vhdl << tab << declare("r_exn", 2) << " <=  \"10\" when  x_exn=\"01\" and overflow='1' else x_exn" << ";" << endl;
			}

		// Back to where we were after the computation of mldt
		setCycle(mltdCycle); 
		setCriticalPath(mltdCP);
		vhdl <<endl << tab << "-- significand processing"<<endl;
		if(mantissaIsOne) {
			vhdl << tab << declare("r_frac", wF_out) << " <= x_sig;"<<endl;
		}
		else {// Actual division
			// mux = diffusion of the control signal + 1 LUT
			manageCriticalPath(target->localWireDelay(wF_in) + target->lutDelay());
			vhdl << tab << declare("divIn0", intDivSize) << " <= '0' & x_sig & CONV_STD_LOGIC_VECTOR(" << h << ", " << s <<");" << endl;
			vhdl << tab << declare("divIn1", intDivSize) << " <= x_sig & '0' & CONV_STD_LOGIC_VECTOR(" << h << ", " << s <<");" << endl;
			vhdl << tab << declare("divIn", intDivSize) << " <= divIn1 when mltd='1' else divIn0;" << endl;
			
			icd = new IntConstDiv(target, intDivSize, d,   alpha, false, inDelayMap("X",target->localWireDelay()+getCriticalPath()));
			oplist.push_back(icd);
			
			inPortMap  (icd, "X", "divIn");
			outPortMap (icd, "Q","quotient");
			outPortMap (icd, "R","remainder");
			vhdl << instance(icd, "sig_div");
			
			setCycleFromSignal("remainder"); 
			setCriticalPath(icd->getOutputDelay("R"));
			
			vhdl << tab << declare("r_frac", wF_out) << " <= quotient" << range(wF_out-1, 0) << ";"<<endl;
			
		}
			vhdl << tab << "R <=  r_exn & x_sgn & r_exp & r_frac" << ";" << endl;
	}




	FPConstDiv::~FPConstDiv() {
		// TODO but who cares really
		//		free(icd); TODO but what's the syntax?
	}





	void FPConstDiv::emulate(TestCase *tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");

		/* Compute correct value */
		FPNumber fpx(wE_in, wF_in);
		fpx = svX;
		mpfr_t x, mpd, r;
		mpfr_init2(x, 1+wF_in);
		mpfr_init2(r, 1+wF_out); 
		mpfr_init(mpd); // Should be enough for everybody 
		mpfr_set_d(mpd, dd, GMP_RNDN);
		fpx.getMPFR(x);
		mpfr_div(r, x, mpd, GMP_RNDN);		
		// Set outputs 
		FPNumber  fpr(wE_out, wF_out, r);
		mpz_class svRN = fpr.getSignalValue();
		tc->addExpectedOutput("R", svRN);
		// clean up
		mpfr_clears(x, r, mpd, NULL);
		}


	void FPConstDiv::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;
		mpz_class x;
	
		tc = new TestCase(this); 
		tc->addFPInput("X", 1.0);
		tc->addComment("1.0");
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", 3.0);
		tc->addComment("3.0");
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", 9.0);
		tc->addComment("9.0");
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", 27.0);
		tc->addComment("27.0");
		emulate(tc);
		tcl->add(tc);
	}

}

