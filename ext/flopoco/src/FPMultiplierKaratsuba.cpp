/*
  Floating Point Multiplier for FloPoCo
 
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author: Bogdan Pasca

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
#include "FPMultiplierKaratsuba.hpp"
#include "FPNumber.hpp"

using namespace std;

namespace flopoco{

	FPMultiplierKaratsuba::FPMultiplierKaratsuba(Target* target, int wEX, int wFX, int wEY, int wFY, int wER, int wFR, int norm) :
		Operator(target), wEX_(wEX), wFX_(wFX), wEY_(wEY), wFY_(wFY), wER_(wER), wFR_(wFR) {

		ostringstream name;
		name << "FPMultiplierKaratsuba_"<<wEX_<<"_"<<wFX_<<"_"<<wEY_<<"_"<<wFY_<<"_"<<wER_<<"_"<<wFR_; 
		setName(name.str());

		if (wFX!=wFY){
			cerr << "karatsuba, equal sizes for inputs pls";
			throw "karatsuba, equal sizes for inputs pls";
		}


		/* set if operator outputs a normalized_ result */
		normalized_ = (norm==0?false:true);
	
		addFPInput ("X", wEX_, wFX_);
		addFPInput ("Y", wEY_, wFY_);
		if(normalized_==true) 
			addFPOutput ("R"   , wER_, wFR    );  
		else{
			wFR_ = 2 + wFX_ + wFY_;
			addOutput  ("ResultExponent"   , wER_    );  
			addOutput ("ResultSignificand", wFR_    );
			addOutput ("ResultException"  , 2      );
			addOutput("ResultSign"       ); 
		}

		/* Sing Handling */
		vhdl << tab << declare("sign") << " <= X" << of(wEX_+wFX) << " xor Y" << of(wEY_+wFY) << ";" << endl;

		/* Exponent Handling */
		vhdl << tab << declare("expX", wEX_) << " <= X"<< range(wEX_ + wFX_ -1, wFX_) << ";" << endl; 
		vhdl << tab << declare("expY", wEY_) << " <= Y"<< range(wEY_ + wFY_ -1, wFX_) << ";" << endl; 

		//Add exponents and substract bias
		vhdl << tab << declare("expSumPreSub", wEX_+2) << " <= (\"00\" & expX) + (\"00\" & expY);" << endl; 
		nextCycle(); ///////////////////////////////////////////////////////
		vhdl << tab << declare("bias", wEX_+2) << " <= CONV_STD_LOGIC_VECTOR(" << intpow2(wER-1)-1 << ","<<wEX_+2<<");"<< endl; 
		vhdl << tab << declare("expSum",wEX+2) << " <= expSumPreSub - bias;" << endl;   
	
		/* Significand Handling */
		setCycle(0); 
		vhdl << tab << declare("sigX",1 + wFX_) << " <= \"1\" & X" << range(wFX_-1,0) << ";" << endl;
		vhdl << tab << declare("sigY",1 + wFY_) << " <= \"1\" & Y" << range(wFY_-1,0) << ";" << endl;

		intmult_ = new IntKaratsuba(target, wFX_+1);
		oplist.push_back(intmult_);

		inPortMap( intmult_, "X", "sigX");
		inPortMap( intmult_, "Y", "sigY");
		outPortMap(intmult_, "R", "sigProd");
		vhdl << instance(intmult_, "SignificandMultiplication");

		/* Exception Handling */
		setCycle(0);
		vhdl << tab << declare("excSel",4) <<" <= X"<<range(wEX_ + wFX_ +2, wEX_ + wFX_ + 1) << " & Y"<<range(wEY_ + wFY_ +2, wEY_ + wFY_ +1) << ";" << endl;
		
		vhdl << tab << "with excSel select " << endl;
		vhdl << tab << declare("exc",2) << " <= \"00\" when  \"0000\" | \"0001\" | \"0100\", " << endl;
		vhdl << tab << "       \"01\" when \"0101\","<<endl;
		vhdl << tab << "       \"10\" when \"0110\" | \"1001\" | \"1010\" ,"<<endl;		
		vhdl << tab << "       \"11\" when others;"<<endl;		

		//syncronization
		syncCycleFromSignal("sigProd");
		syncCycleFromSignal("expSum");
		nextCycle();///////////////////////////////	
		
		if (normalized_==true){
		/******************************************************************/
				
			vhdl << tab<< declare("norm") << " <= sigProd" << of(wFX_+wFY_+1) << ";"<<endl;
			vhdl << tab<< declare("expPostNorm", wEX_+2) << " <= expSum + (" << zg(wEX_+1,0) << " & norm);"<<endl;

			//check is rounding is needed
			if (1+wFR_ >= wFX_+wFY_+2) {  /* => no rounding needed - possible padding */
				vhdl << tab << declare("resSig", wFR_) << " <= sigProd" << range(wFX_+wFY_,0) << " & " <<   zg(1+wFR_ - (wFX_+wFY_+2) , 0)<<" when norm='1' else"<<endl;
				vhdl << tab <<"                      sigProd" << range(wFX_+wFY_-1,0) << " & " << zg(1+wFR_ - (wFX_+wFY_+2) + 1 , 0) << ";"<<endl;

				vhdl << tab <<"with expPostNorm" << range(wER_+1, wER_) << " select"<<endl;		
				vhdl << tab << declare("excPostNorm",2) << " <=  \"01\"  when  \"00\","<<endl;
				vhdl << tab <<"                            \"10\"             when \"01\", "<<endl;
				vhdl << tab <<"                            \"00\"             when \"11\"|\"10\","<<endl;
				vhdl << tab <<"                            \"11\"             when others;"<<endl;						
				
				vhdl << tab << "with exc select " << endl;
				vhdl << tab << declare("finalExc",2) << " <= exc when  \"11\"|\"10\"|\"00\"," <<endl;
				vhdl << tab << "                    excPostNorm when others; " << endl;
		
				vhdl << tab << "R <= finalExc & sign & expPostNorm" << range(wER_-1, 0) << " & resSig;"<<endl;
			}
			else{
				nextCycle();////
//				vhdl << tab << declare("resSig", wFR_) << " <= sigProd" << range(wFX_+wFY_, wFX_+wFY_ - wFR_+1) <<" when norm='1' else"<<endl;
//				vhdl << tab <<"                      sigProd" << range(wFX_+wFY_-1, wFX_+wFY_ - wFR_) << ";"<<endl;

				vhdl << tab << declare("sigProdExt", wFX_+wFY_+ 1 + 1) << " <= sigProd" << range(wFX_+wFY_, 0) << " & " << zg(1,0) <<" when norm='1' else"<<endl;
				vhdl << tab << "                      sigProd" << range(wFX_+wFY_-1, 0) << " & " << zg(2,0) << ";"<<endl;
							
				vhdl << tab << declare("expSig", 2 + wER_ + wFR_) << " <= expPostNorm & sigProdExt" << range(wFX_+wFY_+ 1,wFX_+wFY_+ 2 -wFR_) << ";" << endl;

		
				vhdl << tab << declare("sticky") << " <= sigProdExt" << of(wFX_+wFY + 1 - wFR) << ";" << endl;
				vhdl << tab << declare("guard") << " <= '0' when sigProdExt" << range(wFX_+wFY + 1 - wFR - 1,0) << "=" << zg(wFX_+wFY + 1 - wFR - 1 +1,0) <<" else '1';" << endl;
				vhdl << tab << declare("round") << " <= sticky and ( (guard and not(sigProdExt" << of(wFX_+wFY + 1 - wFR+1) <<")) or (" 
				 << "sigProdExt" << of(wFX_+wFY + 1 - wFR+1) << " ))  ;" << endl;
				                                                                      
				nextCycle();////			
				intadd_ = new IntAdder(target, 2 + wER_ + wFR_);
				oplist.push_back(intadd_);
				
				inPortMap    (intadd_, "X",   "expSig");
				inPortMapCst (intadd_, "Y",   zg(2 + wER_ + wFR_,0));
				inPortMap    (intadd_, "Cin", "round");
				outPortMap   (intadd_, "R", "expSigPostRound");
				
				vhdl << tab << instance( intadd_, "RoundingAdder");
				syncCycleFromSignal("expSigPostRound");
				nextCycle();
				vhdl << tab <<"with expSigPostRound" << range(wER_+wFR_+1, wER_+wFR_) << " select"<<endl;		
				vhdl << tab << declare("excPostNorm",2) << " <=  \"01\"  when  \"00\","<<endl;
				vhdl << tab <<"                            \"10\"             when \"01\", "<<endl;
				vhdl << tab <<"                            \"00\"             when \"11\"|\"10\","<<endl;
				vhdl << tab <<"                            \"11\"             when others;"<<endl;						
			
				vhdl << tab << "with exc select " << endl;
				vhdl << tab << declare("finalExc",2) << " <= exc when  \"11\"|\"10\"|\"00\"," <<endl;
				vhdl << tab << "                    excPostNorm when others; " << endl;
	
				vhdl << tab << "R <= finalExc & sign & expSigPostRound" << range(wER_+wFR_-1, 0)<<";"<<endl;
			
			}
		}else{ //the non-normalized version for DotProduct
				vhdl << tab <<"with expSum" << range(wER_+1, wER_) << " select"<<endl;		
				vhdl << tab << declare("excPostProc",2) << " <=  \"01\"  when  \"00\","<<endl;
				vhdl << tab <<"                            \"10\"             when \"01\", "<<endl;
				vhdl << tab <<"                            \"00\"             when \"11\"|\"10\","<<endl;
				vhdl << tab <<"                            \"11\"             when others;"<<endl;						
				
				vhdl << tab << "with exc select " << endl;
				vhdl << tab << declare("finalExc",2) << " <= exc when  \"11\"|\"10\"|\"00\"," <<endl;
				vhdl << tab << "                    excPostProc when others; " << endl;
				
				vhdl << tab << "ResultExponent <= expSum" << range(wER_-1, 0) << ";" << endl;
				vhdl << tab << "ResultSignificand <= sigProd;" << endl;
				vhdl << tab << "ResultException <= finalExc;" << endl;
				vhdl << tab << "ResultSign <= sign;" << endl;
		}
	} // end constructor

	FPMultiplierKaratsuba::~FPMultiplierKaratsuba() {
	}

	// FIXME: the following is only functional for a correctly rounded multiplier.
	// The old code for the non-normalized case is commented out below, just in case.
	void FPMultiplierKaratsuba::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");

		/* Compute correct value */
		FPNumber fpx(wEX_, wFX_), fpy(wEY_, wFY_);
		fpx = svX;
		fpy = svY;
		mpfr_t x, y, r;
		mpfr_init2(x, 1+wFX_);
		mpfr_init2(y, 1+wFY_);
		mpfr_init2(r, 1+wFR_); 
		fpx.getMPFR(x);
		fpy.getMPFR(y);
		mpfr_mul(r, x, y, GMP_RNDN);

		// Set outputs 
		FPNumber  fpr(wER_, wFR_, r);
		mpz_class svR = fpr.getSignalValue();
		tc->addExpectedOutput("R", svR);

		// clean up
		mpfr_clears(x, y, r, NULL);
	}


}
