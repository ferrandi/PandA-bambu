/*
  Polynomial Function Evaluator for FloPoCo

  Authors: Bogdan Pasca, Mioara Joldes

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
#include "../utils.hpp"


#include "FunctionEvaluator.hpp"

#ifdef HAVE_SOLLYA

using namespace std;

namespace flopoco{

#define DEBUGVHDL 0


	FunctionEvaluator::FunctionEvaluator(Target* target, string func, int wInX, int lsbOut, int n, bool finalRounding, map<string, double> inputDelays):
		Operator(target, inputDelays), wInX_(wInX), lsbOut_(lsbOut), finalRounding_(finalRounding){

		ostringstream name;
		srcFileName="FunctionEvaluator";
		
		name<<"FunctionEvaluator_"<<getNewUId(); 
		setName(name.str()); 

		setCopyrightString("Bogdan Pasca, Mioara Joldes, Florent de Dinechin (2010)");
		
		REPORT(INFO, "Will try to implement the function: " << func );
		REPORT(INFO, "The input precision of x is: " << wInX << " with x in [0,1[");
		REPORT(INFO, "The LSB weight of the output is: "<<lsbOut);
		REPORT(INFO, "The degree of the polynomial used to approximate this function is: " << n );
		
		pf = new PiecewiseFunction(func);
		tg = new PolyCoeffTable(target, pf, lsbOut+1, n);
		oplist.push_back(tg);
		
		REPORT(INFO, "The number of intervals of the function: "<< intpow2(tg->wIn));
		REPORT(INFO, "The number of bits used for y: "<< wInX-tg->wIn << " and its weight is: " << -tg->wIn);
		
			
		/* the nmber of bits of the address */
		int addrSize = tg->wIn;
		/* the number of bits of y is */
		int ySize = wInX - tg->wIn;
		/* the weight of y considering that y is in 1.XXXX */
		int yWeight = -tg->wIn;
		
		YVar* y = new YVar(ySize, yWeight);
		setCriticalPath(0.0);
		pe = new PolynomialEvaluator(target, tg->getCoeffParamVector(), y, lsbOut+1, tg->getMaxApproxError() , inDelayMap("Y",getCriticalPath()));
		oplist.push_back(pe);


		wR = pe->getRWidth();
		weightR = pe->getRWeight()+1;
		
		REPORT(INFO, "The width of the polynomial evaluator output is: "<< wR);
		/* number of digits to the left of the . */
		REPORT(INFO, "The weight of the PE output is: "<<weightR);


		addInput ("X", wInX);
		vhdl << tab << declare("addr", addrSize,true) << " <= X"<<range(wInX-1, wInX-addrSize)<<";"<<endl;

		/* Instantiate the table generator */
//		nextCycle();/////////////////////////////////// The Coefficent ROM has a registered input
		inPortMap ( tg, "X", "addr");
		outPortMap ( tg, "Y", "Coef");
		vhdl << instance ( tg, "GeneratedTable" );
		syncCycleFromSignal("Coef");
		nextCycle();/////////////////////////////////// The Coefficent ROM has a registered output
		
		setCriticalPath(0.0);
			
		vhdl << tab << declare ("y",ySize) << " <= X"<<range(ySize-1 ,0) << ";" << endl;
		
		/* get the coefficients */
		int lsb = 0, sizeS = 0;
		for (uint32_t i=0; i< pe->getCoeffParamVector().size(); i++){
			lsb += sizeS;
			sizeS = pe->getCoeffParamVector()[i]->getSize()+1;
			vhdl << tab << declare(join("a",i), sizeS ) << "<= Coef"<< range (lsb+sizeS-1, lsb) << ";" << endl;
		}
		
		/* Instantiate the polynomial evaluator generator*/
		inPortMap( pe, "Y", "y");
		for (uint32_t i=0; i< pe->getCoeffParamVector().size(); i++){
			inPortMap(pe,  join("a",i), join("a",i));
		}
		
		outPortMap( pe, "R", "Rpe");
		vhdl << instance( pe, "PolynomialEvaluator");

		syncCycleFromSignal("Rpe");
		setCriticalPath(pe->getOutputDelay("R"));
		
		if (finalRounding_){
//			/* number of bits to recover is */
			int recover = weightR + lsbOut;
			vhdl << tab << "-- weight of poly result is : " << weightR << endl;
//			manageCriticalPath(target->adderDelay(pe->getRWidth()-(recover)-2));
//			vhdl << tab << 	declare("sticky") << " <= '0' when Rpe"<<range(pe->getRWidth()-(recover)-2,0) <<" = " 
//		                                                        <<   zg(pe->getRWidth()-(recover)-1,0) << " else '1';"<<endl;
//			vhdl << tab << declare("extentedf", recover + 1) << " <= Rpe"<<range(pe->getRWidth()-1, pe->getRWidth()-(recover+1))<<";"<<endl; 
//		                  
////			nextCycle();                              
//			IntAdder *a = new IntAdder(target, recover + 1, inDelayMap("X",getCriticalPath()));
//			oplist.push_back(a);
//	
//			inPortMap(a, "X", "extentedf");
//			inPortMapCst(a, "Y", zg(recover + 1,0) );
//			inPortMapCst(a, "Cin", "sticky");
//			outPortMap(a, "R", "fPostRound");
//			vhdl << instance(a, "Rounding_Adder");

//			syncCycleFromSignal("fPostRound");
		
			addOutput("R", recover);
//			outDelayMap["R"]=a->getOutputDelay("R");
//			vhdl << tab << " R <= fPostRound"<<range(recover, 1)<<";"<<endl;
			vhdl << tab << " R <= Rpe"<<range(wR-1, wR-recover)<<";"<<endl;
			outDelayMap["R"] = getCriticalPath();
		}else{
			outDelayMap["R"] = getCriticalPath();
			addOutput("R", pe->getRWidth());
			vhdl << tab << "R <= Rpe;" << endl;  
		}
			
	}

	FunctionEvaluator::~FunctionEvaluator() {
	}
	
	
	void FunctionEvaluator::emulate(TestCase* tc){
		if (!finalRounding_)
			throw("Cannot generate testCases for non-rounded case");
			
		setToolPrecision(165);
		int  nrFunctions=(pf->getPiecewiseFunctionArray()).size();
		
		Function *f = pf->getPiecewiseFunctionArray(0);

		if (nrFunctions !=1) {
			cerr<<"Warning: we are dealing with a piecewise function; only the first branch will be evaluated"<<endl;    
		}

		mpz_class rd, ru; // rounded down

		mpz_class svX = tc->getInputValue("X");
		mpfr_t mpX, mpR;
		mpfr_init2(mpX,wInX_+2);
		mpfr_init2(mpR,10*lsbOut_);
		//int outSign = 0;
		/* Convert a random signal to an mpfr_t in [0,1[ */
		mpfr_set_z(mpX, svX.get_mpz_t(), GMP_RNDN);
		mpfr_div_2si(mpX, mpX, wInX_, GMP_RNDN);
	
		/* Compute the function */
		f->eval(mpR, mpX);
		REPORT(FULL,"emulate() input is:"<<sPrintBinary(mpX)); 
		REPORT(FULL,"emulate() output before rounding is:"<<sPrintBinary(mpR)); 
		/* Compute the signal value */
		if (mpfr_signbit(mpR))
			{
				//outSign = 1;
				mpfr_abs(mpR, mpR, GMP_RNDN);
			}
		mpfr_mul_2si(mpR, mpR, lsbOut_, GMP_RNDN);

		/* Function Evaluator only guarantees faithful rounding, so we
		 *  round down here, then add one ulp to obtain rounded up value.
		 */
		mpfr_get_z(rd.get_mpz_t(), mpR, GMP_RNDD);
		ru = rd + 1;
		REPORT(FULL,"emulate() output rd="<<rd <<"  ru="<< ru); 

		tc->addExpectedOutput("R", rd);
		tc->addExpectedOutput("R", ru);
		mpfr_clear(mpX);
		mpfr_clear(mpR);
	}

}
	
#endif //HAVE_SOLLYA	
