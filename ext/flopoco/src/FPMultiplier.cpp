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
#include "FPMultiplier.hpp"
#include "FPNumber.hpp"

using namespace std;

namespace flopoco{

	FPMultiplier::FPMultiplier(Target* target, int wEX, int wFX, int wEY, int wFY, int wER, int wFR, 
	                           bool norm, bool correctlyRounded, double ratio, int maxTimeInMinutes, map<string, double> inputDelays) :
		Operator(target), wEX_(wEX), wFX_(wFX), wEY_(wEY), wFY_(wFY), wER_(wER), wFR_(wFR), normalized_(norm), correctlyRounded_(correctlyRounded)  {

		ostringstream name;
		name << "FPMultiplier_"<<wEX_<<"_"<<wFX_<<"_"<<wEY_<<"_"<<wFY_<<"_"<<wER_<<"_"<<wFR_<<"_uid"<<getNewUId(); 
		setName(name.str());
		setCopyrightString("Bogdan Pasca, Florent de Dinechin 2008-2011");

	
		addFPInput ("X", wEX_, wFX_);
		addFPInput ("Y", wEY_, wFY_);
		if(normalized_) 
			addFPOutput ("R"   , wER , wFR);  
		else{
			wFR_ = 2 + wFX_ + wFY_;
			addOutput  ("ResultExponent"   , wER_    );  
			addOutput ("ResultSignificand", wFR_    );
			addOutput ("ResultException"  , 2      );
			addOutput("ResultSign"       ); 
		}

		setCriticalPath(getMaxInputDelays(inputDelays));

		/* Sign Handling -- no need to count it in the critical path */
		vhdl << tab << declare("sign") << " <= X" << of(wEX_+wFX) << " xor Y" << of(wEY_+wFY) << ";" << endl;

		/* Exponent Handling */
		vhdl << tab << declare("expX", wEX_) << " <= X"<< range(wEX_ + wFX_ -1, wFX_) << ";" << endl; 
		vhdl << tab << declare("expY", wEY_) << " <= Y"<< range(wEY_ + wFY_ -1, wFY_) << ";" << endl; 

		//Add exponents and substract bias
		manageCriticalPath(target->localWireDelay() + target->adderDelay(wEX+2));
		vhdl << tab << declare("expSumPreSub", wEX_+2) << " <= (\"00\" & expX) + (\"00\" & expY);" << endl; 
		vhdl << tab << declare("bias", wEX_+2) << " <= CONV_STD_LOGIC_VECTOR(" << intpow2(wER-1)-1 << ","<<wEX_+2<<");"<< endl; 

		manageCriticalPath(target->localWireDelay() + target->adderDelay(wEX+2));
		vhdl << tab << declare("expSum",wEX+2) << " <= expSumPreSub - bias;" << endl;   
		double exponentCriticalPath=getCriticalPath();
	

		/* Significand Handling */
		setCycle(0); 
		setCriticalPath(0.0);
		vhdl << tab << declare("sigX",1 + wFX_) << " <= \"1\" & X" << range(wFX_-1,0) << ";" << endl;
		vhdl << tab << declare("sigY",1 + wFY_) << " <= \"1\" & Y" << range(wFY_-1,0) << ";" << endl;


		int sigProdSize;
		int g=3; // number of guard bits needed in case of faithful rounding
		if(correctlyRounded_) 
			sigProdSize = wFX_+1 + wFY_+1;
		else
			// faithful rounding will be computed by IntTruncMultiplier
			// but we still  have to re-round behind 
			sigProdSize = wFR_+g; 
#if 1
		IntMultiplier* intmult_ = new IntMultiplier(target, wFX_+1, wFY_+1, sigProdSize, false /*signedIO*/);
#else
		int useLimits=1; // TODO WTF is it? 
		IntTruncMultiplier* intmult_ = new IntTruncMultiplier(target, wFX_+1, wFY_+1, sigProdSize, ratio, useLimits, maxTimeInMinutes,
		                                                     false, /* interactive */
		                                                     false, /* signed */ 
		                                                     false  /* roundCompensate*/);
#endif

		oplist.push_back(intmult_);

		inPortMap( intmult_, "X", "sigX");
		inPortMap( intmult_, "Y", "sigY");
		outPortMap(intmult_, "R", "sigProd");
		vhdl << instance(intmult_, "SignificandMultiplication");
		syncCycleFromSignal("sigProd");
		setCriticalPath( intmult_->getOutputDelay("R"));
		double significandCriticalPath=getCriticalPath();
		

		/* Exception Handling, assumed to be faster than both exponent and significand computations */
		setCycle(0);
		vhdl << tab << declare("excSel",4) <<" <= X"<<range(wEX_ + wFX_ +2, wEX_ + wFX_ + 1) << " & Y"<<range(wEY_ + wFY_ +2, wEY_ + wFY_ +1) << ";" << endl;
		
		vhdl << tab << "with excSel select " << endl;
		vhdl << tab << declare("exc",2) << " <= \"00\" when  \"0000\" | \"0001\" | \"0100\", " << endl;
		vhdl << tab << "       \"01\" when \"0101\","<<endl;
		vhdl << tab << "       \"10\" when \"0110\" | \"1001\" | \"1010\" ,"<<endl;		
		vhdl << tab << "       \"11\" when others;"<<endl;		

		//synchronization

		syncCycleFromSignal("expSum", exponentCriticalPath,false);	
		syncCycleFromSignal("sigProd", significandCriticalPath,true); 

		
		if (normalized_){
		/******************************************************************/
				
			vhdl << tab<< declare("norm") << " <= sigProd" << of(sigProdSize -1) << ";"<<endl;
			
			manageCriticalPath(target->localWireDelay() + target->adderDelay(wEX+2));
			double expPostNormCriticalPath=getCriticalPath();
			vhdl << tab<< "-- exponent update"<<endl;
			vhdl << tab<< declare("expPostNorm", wEX_+2) << " <= expSum + (" << zg(wEX_+1,0) << " & norm);"<<endl;

			//  exponent update is in parallel to the mantissa shift, so get back there
			setCycleFromSignal("expSum", exponentCriticalPath, false);	
			syncCycleFromSignal("sigProd", significandCriticalPath, true); 
		
			//check is rounding is needed
			if (1+wFR_ >= wFX_+wFY_+2) {  
				/* => no rounding needed - possible padding; 
				   in this case correctlyRounded_ is irrelevant: result is exact  */
				vhdl << tab << declare("resSig", wFR_) << " <= sigProd" << range(wFX_+wFY_,0) << " & " <<   zg(1+wFR_ - (wFX_+wFY_+2) , 0)<<" when norm='1' else"<<endl;
				vhdl << tab <<"                      sigProd" << range(wFX_+wFY_-1,0) << " & " << zg(1+wFR_ - (wFX_+wFY_+2) + 1 , 0) << ";"<<endl;

				manageCriticalPath(target->localWireDelay() + target->lutDelay());
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
				vhdl << tab<< "-- significand normalization shift"<<endl;
				manageCriticalPath(target->localWireDelay() + target->lutDelay());
				vhdl << tab << declare("sigProdExt", sigProdSize) << " <= sigProd" << range(sigProdSize-2, 0) << " & " << zg(1,0) <<" when norm='1' else"<<endl;
				vhdl << tab << "                      sigProd" << range(sigProdSize-3, 0) << " & " << zg(2,0) << ";"<<endl;

				
				syncCycleFromSignal("expPostNorm", expPostNormCriticalPath, true); 
				vhdl << tab << declare("expSig", 2 + wER_ + wFR_) << " <= expPostNorm & sigProdExt" << range(sigProdSize-1,  sigProdSize-wFR_) << ";" << endl;
				
				if(correctlyRounded_) {
					vhdl << tab << declare("sticky") << " <= sigProdExt" << of(wFX_+wFY + 1 - wFR) << ";" << endl;
					
					if(wFX_+wFY + 1 - wFR>0) // otherwise the user has been stupid anyway
						manageCriticalPath(target->localWireDelay() + target->eqConstComparatorDelay(sigProdSize-1 - wFR));
					vhdl << tab << declare("guard") << " <= '0' when sigProdExt" << range(wFX_+wFY + 1 - wFR - 1,0) << "=" << zg(wFX_+wFY + 1 - wFR - 1 +1,0) <<" else '1';" << endl;
					
					manageCriticalPath(target->localWireDelay() + target->lutDelay());
					vhdl << tab << declare("round") << " <= sticky and ( (guard and not(sigProdExt" << of(wFX_+wFY + 1 - wFR+1) <<")) or (" 
					     << "sigProdExt" << of(wFX_+wFY + 1 - wFR+1) << " ))  ;" << endl;
				}
				else
					{
					vhdl << tab << declare("round") << " <= '1' ;" << endl;
				}
				IntAdder* intadd_ = new IntAdder(target, 2 + wER_ + wFR_, inDelayMap("X", getCriticalPath() ));
				oplist.push_back(intadd_);
					
				inPortMap    (intadd_, "X",   "expSig");
				inPortMapCst (intadd_, "Y",   zg(2 + wER_ + wFR_,0));
				inPortMap    (intadd_, "Cin", "round");
				outPortMap   (intadd_, "R", "expSigPostRound");
				
				vhdl << tab << instance( intadd_, "RoundingAdder");
				syncCycleFromSignal("expSigPostRound");
				setCriticalPath(intadd_->getOutputDelay("R"));
					


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
			// TODO: modern pipeline framework
			// TODO: manage the faithful case
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

	FPMultiplier::~FPMultiplier() {
	}



	// TODO the unnormalized case is not emulated
	void FPMultiplier::emulate(TestCase * tc)
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
		if(correctlyRounded_){
			mpfr_mul(r, x, y, GMP_RNDN);
			// Set outputs 
			FPNumber  fpr(wER_, wFR_, r);
			mpz_class svR = fpr.getSignalValue();
			tc->addExpectedOutput("R", svR);
		}
		else{
			// round down
			mpfr_mul(r, x, y, GMP_RNDD);
			FPNumber  fprd(wER_, wFR_, r);
			mpz_class svRd = fprd.getSignalValue();
			tc->addExpectedOutput("R", svRd);
			// round up
			mpfr_mul(r, x, y, GMP_RNDU);
			FPNumber  fpru(wER_, wFR_, r);
			mpz_class svRu = fpru.getSignalValue();
			tc->addExpectedOutput("R", svRu);
		}
		// clean up
		mpfr_clears(x, y, r, NULL);
	}


}
