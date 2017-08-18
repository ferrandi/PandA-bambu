/*
  Function Table for FloPoCo

  Authors: Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  

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

#include "FunctionTable.hpp"

#ifdef HAVE_SOLLYA

using namespace std;

namespace flopoco{

	FunctionTable::FunctionTable(Target* target, string func, int wInX, int lsbOut, int msbOut, int logicTable, map<string, double> inputDelays):
		Table(target, wInX, msbOut-lsbOut+1, 0, -1, logicTable), wInX_(wInX), lsbOut_(lsbOut), msbOut_(msbOut){

		ostringstream name;
		srcFileName="FunctionTable";
		
		name<<"FunctionTable_"<<getNewUId(); 
		setName(name.str()); 

		setCopyrightString("Florent de Dinechin (2010)");
		
		REPORT(INFO, "Table-based implementation of : " << func );
		REPORT(INFO, "The input precision of x is: " << wInX << " with x in [0,1[");
		REPORT(INFO, "The LSB weight of the output is: "<<lsbOut);
		
		f = new Function(func);
	 			
	}

	FunctionTable::~FunctionTable() {
	}
	
	
	mpz_class FunctionTable::function(int x){
		int precision=10*(wIn+wOut);
		setToolPrecision(precision);
		mpz_class r ; 

		mpz_class svX = x;
		mpfr_t mpX, mpR;
		mpfr_init2(mpX,wInX_+2);
		mpfr_init2(mpR,precision);

		/* Convert x to an mpfr_t in [0,1[ */
		mpfr_set_z(mpX, svX.get_mpz_t(), GMP_RNDN);
		mpfr_div_2si(mpX, mpX, wInX_, GMP_RNDN);
	
		/* Compute the function */
		f->eval(mpR, mpX);
		//		REPORT(FULL,"function() input is:"<<sPrintBinary(mpX)); 
		//		REPORT(FULL,"function() output before rounding is:"<<sPrintBinary(mpR)); 
		/* Compute the signal value */
		mpfr_mul_2si(mpR, mpR, -lsbOut_, GMP_RNDN);

		/* Almost guarantees correct rounding
		 */
		mpfr_get_z(r.get_mpz_t(), mpR, GMP_RNDN);
		REPORT(FULL,"function() output r="<<r); 
		mpfr_clear(mpX);
		mpfr_clear(mpR);
		return r;
	}


	void FunctionTable::emulate(TestCase* tc){
		mpz_class svX = tc->getInputValue("X");

		if(svX<0 || (svX>(mpz_class(1)<<31)))
			throw("FunctionTable::emulate input too large");

		int x =  svX.get_ui();		

		mpz_class r = function(x);

		tc->addExpectedOutput("Y", r);
	}

}
	
#endif //HAVE_SOLLYA	
