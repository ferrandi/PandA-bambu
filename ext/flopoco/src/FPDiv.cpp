/*
  Floating Point Divider for FloPoCo

  
  This file is part of the FloPoCo project 
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Authors: Jeremie Detrey, Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

 */


// TODO Test even and odd significands

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

#include "FPDiv.hpp"

using namespace std;

namespace flopoco{

#define DEBUGVHDL 0


	FPDiv::FPDiv(Target* target, int wE, int wF) :
		Operator(target), wE(wE), wF(wF) {

		int i;
		ostringstream name;

		srcFileName="FPDiv";
		name<<"FPDiv_"<<wE<<"_"<<wF; 
		uniqueName_ = name.str(); 

		
		// -------- Parameter set up -----------------
		nDigit = (wF+6) >> 1; 

		addFPInput ("X", wE, wF);
		addFPInput ("Y", wE, wF);
		addFPOutput("R", wE, wF);
		
		vhdl << tab << declare("fX",wF+1) << " <= \"1\" & X(" << wF-1 << " downto 0);" << endl;
		vhdl << tab << declare("fY",wF+1) << " <= \"1\" & Y(" << wF-1 << " downto 0);" << endl;

		vhdl << tab << "-- exponent difference, sign and exception combination computed early, to have less bits to pipeline" << endl;
	 
		vhdl << tab << declare("expR0", wE+2) << " <= (\"00\" & X(" << wE+wF-1 << " downto " << wF << ")) - (\"00\" & Y(" << wE+wF-1 << " downto " << wF<< "));" << endl;
		vhdl << tab << declare("sR") << " <= X(" << wE+wF << ") xor Y(" << wE+wF<< ");" << endl;
		vhdl << tab << "-- early exception handling " <<endl;
		vhdl << tab << declare("exnXY",4) << " <= X(" << wE+wF+2 << " downto " << wE+wF+1  << ") & Y(" << wE+wF+2 << " downto " << wE+wF+1 << ");" <<endl;
		vhdl << tab << "with exnXY select" <<endl;
		vhdl << tab << tab << declare("exnR0", 2) << " <= " << endl;
		vhdl << tab << tab << tab << "\"01\"  when \"0101\",                   -- normal" <<endl;
		vhdl << tab << tab << tab << "\"00\"  when \"0001\" | \"0010\" | \"0110\", -- zero" <<endl;
		vhdl << tab << tab << tab << "\"10\"  when \"0100\" | \"1000\" | \"1001\", -- overflow" <<endl;
		vhdl << tab << tab << tab << "\"11\"  when others;                   -- NaN" <<endl;
		vhdl << tab << " -- compute 3Y" << endl;
		vhdl << tab << declare("fYTimes3",wF+3) << " <= (\"00\" & fY) + (\"0\" & fY & \"0\");" << endl; // TODO an IntAdder here
	
		ostringstream wInit;
		wInit << "w"<<nDigit-1;
		vhdl << tab << declare(wInit.str(), wF+3) <<" <=  \"00\" & fX;" << endl;

		nextCycle();/////////////////////////////////////////////////////////////
		setCriticalPath(0);

		double srt4stepdelay =  2*target->lutDelay() + 2*target->localWireDelay() + target->adderDelay(wF+4);

		for(i=nDigit-1; i>=1; i--) {
			manageCriticalPath(srt4stepdelay);

			ostringstream wi, qi, wim1, seli, qiTimesD, wipad, wim1full;
			wi << "w" << i;
			qi << "q" << i;
			wim1 << "w" << i-1;
			seli << "sel" << i;
			qiTimesD << "q" << i << "D";
			wipad << "w" << i << "pad";
			wim1full << "w" << i-1 << "full";
	
			vhdl << tab << declare(seli.str(),5) << " <= " << wi.str() << range( wF+2, wF-1)<<" & fY"<<of(wF-1)<<";" << endl; 
			vhdl << tab << "with " << seli.str() << " select" << endl;
			vhdl << tab << declare(qi.str(),3) << " <= " << endl;
			vhdl << tab << tab << "\"001\" when \"00010\" | \"00011\"," << endl;
			vhdl << tab << tab << "\"010\" when \"00100\" | \"00101\" | \"00111\"," << endl;
			vhdl << tab << tab << "\"011\" when \"00110\" | \"01000\" | \"01001\" | \"01010\" | \"01011\" | \"01101\" | \"01111\"," << endl;
			vhdl << tab << tab << "\"101\" when \"11000\" | \"10110\" | \"10111\" | \"10100\" | \"10101\" | \"10011\" | \"10001\"," << endl;
			vhdl << tab << tab << "\"110\" when \"11010\" | \"11011\" | \"11001\"," << endl;
			vhdl << tab << tab << "\"111\" when \"11100\" | \"11101\"," << endl;
			vhdl << tab << tab << "\"000\" when others;" << endl;
			vhdl << endl;
			vhdl << tab << "with " << qi.str() << " select" << endl;
			vhdl << tab << tab << declare(qiTimesD.str(),wF+4) << " <= "<< endl ;
			vhdl << tab << tab << tab << "\"000\" & fY            when \"001\" | \"111\"," << endl;
			vhdl << tab << tab << tab << "\"00\" & fY & \"0\"     when \"010\" | \"110\"," << endl;
			vhdl << tab << tab << tab << "\"0\" & fYTimes3             when \"011\" | \"101\"," << endl;
			vhdl << tab << tab << tab << "(" << wF+3 << " downto 0 => '0') when others;" << endl;
			vhdl << endl;
			vhdl << tab << declare(wipad.str(), wF+4) << " <= " << wi.str() << " & \"0\";" << endl;
			vhdl << tab << "with " << qi.str() << "(2) select" << endl;
			vhdl << tab << declare(wim1full.str(), wF+4) << "<= " << wipad.str() << " - " << qiTimesD.str() << " when '0'," << endl;
			vhdl << tab << "      " << wipad.str() << " + " << qiTimesD.str() << " when others;" << endl;
			vhdl << endl;
			vhdl << tab << declare(wim1.str(),wF+3) << " <= " << wim1full.str()<<range(wF+1,0)<<" & \"0\";" << endl;
		}
 
 
		manageCriticalPath(srt4stepdelay);
	
		vhdl << tab << declare("q0",3) << "(2 downto 0) <= \"000\" when  w0 = (" << wF+2 << " downto 0 => '0')" << endl;
		vhdl << tab << "             else w0(" << wF+2 << ") & \"10\";" << endl;

		for(i=nDigit-1; i>=1; i--) {
			ostringstream qi, qPi, qMi;
			qi << "q" << i;
			qPi << "qP" << i;
			qMi << "qM" << i;
			vhdl << tab << declare(qPi.str(), 2) <<" <=      " << qi.str() << "(1 downto 0);" << endl;
			vhdl << tab << declare(qMi.str(), 2)<<" <=      " << qi.str() << "(2) & \"0\";" << endl;
		}

		vhdl << tab << declare("qP0", 2) << " <= q0(1 downto 0);" << endl;
		vhdl << tab << declare("qM0", 2) << " <= q0(2)  & \"0\";" << endl;

		vhdl << tab << declare("qP", 2*nDigit) << " <= qP" << nDigit-1;
		for (i=nDigit-2; i>=0; i--)
			vhdl << " & qP" << i;
		vhdl << ";" << endl;

		vhdl << tab << declare("qM", 2*nDigit) << " <= qM" << nDigit-1 << "(0)";
		for (i=nDigit-2; i>=0; i--)
			vhdl << " & qM" << i;
		vhdl << " & \"0\";" << endl;


		// TODO an IntAdder here
		vhdl << tab << declare("fR0", 2*nDigit) << " <= qP - qM;" << endl;

		nextCycle();///////////////////////////////////////////////////////////////////////
	

		vhdl << tab << declare("fR", wF+4) << " <= "; 
		if (1 == (wF & 1) ) // odd wF
			vhdl << "fR0(" << 2*nDigit-1 << " downto 1);  -- odd wF" << endl;
		else 
			vhdl << "fR0(" << 2*nDigit-1 << " downto 3)  & (fR0(2) or fR0(1));  -- even wF, fixing the round bit" << endl;


		vhdl << tab << "-- normalisation" << endl;
		vhdl << tab << "with fR(" << wF+3 << ") select" << endl;

		vhdl << tab << tab << declare("fRn1", wF+2) << " <= fR(" << wF+2 << " downto 2) & (fR(1) or fR(0)) when '1'," << endl;
		vhdl << tab << tab << "        fR(" << wF+1 << " downto 0)                    when others;" << endl;

		vhdl << tab << declare("expR1", wE+2) << " <= expR0" 
			  << " + (\"000\" & (" << wE-2 << " downto 1 => '1') & fR(" << wF+3 << ")); -- add back bias" << endl;



		vhdl << tab << declare("round") << " <= fRn1(1) and (fRn1(2) or fRn1(0)); -- fRn1(0) is the sticky bit" << endl;

		nextCycle();///////////////////////////////////////////////////////////////////////
		vhdl << tab << "-- final rounding" <<endl;
		vhdl << tab <<  declare("expfrac", wE+wF+2) << " <= " 
			 << "expR1 & fRn1(" << wF+1 << " downto 2) ;" << endl;
		vhdl << tab << declare("expfracR", wE+wF+2) << " <= " 
			 << "expfrac + ((" << wE+wF+1 << " downto 1 => '0') & round);" << endl;
		vhdl << tab <<  declare("exnR", 2) << " <=      \"00\"  when expfracR(" << wE+wF+1 << ") = '1'   -- underflow" <<endl;
		vhdl << tab << "        else \"10\"  when  expfracR(" << wE+wF+1 << " downto " << wE+wF << ") =  \"01\" -- overflow" <<endl;
		vhdl << tab << "        else \"01\";      -- 00, normal case" <<endl;


		vhdl << tab << "with exnR0 select" <<endl;
		vhdl << tab << tab << declare("exnRfinal", 2) << " <= " <<endl;
		vhdl << tab << tab << tab << "exnR   when \"01\", -- normal" <<endl;
		vhdl << tab << tab << tab << "exnR0  when others;" <<endl;
		vhdl << tab << "R <= exnRfinal & sR & " 
			 << "expfracR(" << wE+wF-1 << " downto 0);" <<endl;

	}

	FPDiv::~FPDiv() {
	}



	void FPDiv::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");

		/* Compute correct value */
		FPNumber fpx(wE, wF), fpy(wE, wF);
		fpx = svX;
		fpy = svY;
		mpfr_t x, y, r;
		mpfr_init2(x, 1+wF);
		mpfr_init2(y, 1+wF);
		mpfr_init2(r, 1+wF); 
		fpx.getMPFR(x); // fake 0
		fpy.getMPFR(y);
		mpfr_div(r, x, y, GMP_RNDN);
		FPNumber  fpr(wE, wF, r);

		/* Set outputs */
		mpz_class svR = fpr.getSignalValue();
		tc->addExpectedOutput("R", svR);
		mpfr_clears(x, y, r, NULL);
	}



	void FPDiv::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		// Regression tests 
		tc = new TestCase(this); 
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", FPNumber::plusDirtyZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", FPNumber::minusDirtyZero);
		tc->addFPInput("Y", FPNumber::plusInfty);
		emulate(tc);
		tcl->add(tc);

	
	}

}
