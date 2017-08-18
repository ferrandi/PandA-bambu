/*
 * A floating-point faithful multiplier by a real constant, using a variation of the KCM method

 This file is part of the FloPoCo project developed by the Arenaire
 team at Ecole Normale Superieure de Lyon
  
 Author : Bogdan Pasca, Bogdan.Pasca@ens-lyon.org

 Initial software.
 Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
 2008-2010.
  All rights reserved.
*/


#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../sollya.h"
#include "../utils.hpp"
#include "../Operator.hpp"

#ifdef HAVE_SOLLYA
#include "FPRealKCM.hpp"
#include "../IntAdder.hpp"
#include "../IntMultiAdder.hpp"
#include "FixRealKCM.hpp"

using namespace std;

namespace flopoco{

	FPRealKCM::FPRealKCM(Target* target, int wE, int wF,  string constant, map<string, double> inputDelays) :
		Operator(target, inputDelays), wE(wE), wF(wF), constant(constant)
	{
		srcFileName="FPRealKCM";

		/* Convert the input string into a sollya evaluation tree */
		sollya_node_t node;
		node = parseString(constant.c_str());	/* If conversion did not succeed (i.e. parse error) */
		if (node == 0) {
			ostringstream error;
			error << srcFileName << ": Unable to parse string "<< constant << " as a numeric constant" <<endl;
			throw error.str();
		}

		mpfr_init2(mpC, 10000);
		setToolPrecision(10000);
		evaluateConstantExpression(mpC, node,  getToolPrecision());// should be enough for everybody

		if(mpfr_cmp_si(mpC, 0)<0)
			throw string("FPRealKCM: only positive constants are supported");

		REPORT(DEBUG, "Constant evaluates to " << mpfr_get_d(mpC, GMP_RNDN));
		REPORT(DEBUG, "Constant exponent is " << mpfr_get_exp(mpC) );

		// build the name
		ostringstream name; 
		name <<"FPRealKCM_" << vhdlize(wE)  << "_" << vhdlize(wF) << "_" << vhdlize(constant);
		setName(name.str());
				
		int iExp = mpfr_get_exp(mpC);// - 1;
		
		addFPInput("X", wE, wF);
		addFPOutput("R", wE, wF, 2); //faithful result
		
		vhdl << tab << declare("fracX",wF+1) << " <= \"1\" & X"<<range(wF-1,0)<<";"<<endl;
		vhdl << tab << declare("eX",wE) << " <= X"<<range(wE+wF-1, wF)<<";"<<endl;
		
		vhdl << tab << declare("exc",2) << "<= X"<<range(wE+wF+2, wE+wF+1)<<";"<<endl;
		vhdl << tab << declare("sign") << "<= X"<<of(wE+wF)<<";"<<endl;
		
		FixRealKCM *frkcm = new FixRealKCM( target, -wF, 0, 0, -wF+iExp-1, constant);
		oplist.push_back(frkcm);
		
		inPortMap(frkcm, "X", "fracX");
		outPortMap(frkcm, "R", "fracMultRes");
		vhdl << tab << instance( frkcm, "ConstMultKCM") << endl;
		syncCycleFromSignal("fracMultRes");
		setCriticalPath(frkcm->getOutputDelay("R"));
		
		//get number of bits of output
		//normalize
		vhdl << tab << declare("norm") << " <= fracMultRes"<<of(wF+1)<<";"<<endl;
		
		manageCriticalPath(target->localWireDelay() + target->adderDelay(wE+2));
		vhdl << tab << declare("nf",wF) << " <= fracMultRes"<<range(wF-1,0)<<" when norm='0' else fracMultRes"<<range(wF,1)<<";"<<endl;
		
		//update exponent
		vhdl << tab << declare("expOp1",wE+2) << " <= CONV_STD_LOGIC_VECTOR("<<iExp-1<<","<<wE+2<<");"<<endl;
		vhdl << tab << declare("finalExp",wE+2) << " <= (\"00\" & eX) + expOp1 + norm;"<<endl;
		
		manageCriticalPath(target->localWireDelay() + target->lutDelay());
		vhdl << tab << "with finalExp"<<range(wE+1,wE)<<" select "<<endl;
		vhdl << tab << declare("excUpdated", 2) << " <= exc when \"00\","<<endl;
		vhdl << tab << tab << "\"00\" when \"10\"|\"11\","<<endl;
		vhdl << tab << tab << "\"10\" when \"01\","<<endl;
		vhdl << tab << tab << " exc when others;"<<endl;
		
		manageCriticalPath(target->localWireDelay() + target->lutDelay());
		vhdl << tab << "with exc select "<<endl;
		vhdl << tab << declare("excUpdated2", 2) << " <= exc when \"00\"|\"10\"|\"11\","<<endl;
		vhdl << tab << tab << " excUpdated when \"01\","<<endl;
		vhdl << tab << tab << " exc when others;"<<endl;
		
		vhdl << tab << "R <= excUpdated2 & sign & finalExp"<<range(wE-1,0)<<" & nf;"<<endl;
		outDelayMap["R"] = getCriticalPath();
	}



	FPRealKCM::~FPRealKCM() {
		// TODO 
	}


	// To have MPFR work in fix point, we perform the multiplication in very large precision using RN,
	// and the RU and RD are done only when converting to an int at the end.
	void FPRealKCM::emulate(TestCase* tc){
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");
		
		/* Compute correct value */
		FPNumber fpx(wE, wF);
		fpx = svX;
		mpfr_t x, c, ru, rd;
		mpfr_init2(x, 1+wF);
		mpfr_init2(c, 1+wF);
		mpfr_init2(ru, 1+wF);
		mpfr_init2(rd, 1+wF);
		mpfr_set(c, mpC, GMP_RNDN);
		fpx.getMPFR(x);
		mpfr_mul(ru, x, c, GMP_RNDU);
		mpfr_mul(rd, x, c, GMP_RNDD);
		
		// Set outputs 
		FPNumber  fprd(wE, wF, rd);
		mpz_class svRd = fprd.getSignalValue();
		tc->addExpectedOutput("R", svRd);
		
		FPNumber  fpru(wE, wF, ru);
		mpz_class svRu = fpru.getSignalValue();
		tc->addExpectedOutput("R", svRu);
		
		// clean up
		mpfr_clears(x, c, rd, ru, NULL);
	}

	// void FPRealKCM::buildStandardTestCases(TestCaseList* tcl){

	// }






}




#endif //HAVE_SOLLYA
