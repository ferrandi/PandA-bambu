/*
  A correctly-rounded multiplier by an arbitrary real constant for FloPoCo
 
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
#include "../sollya.h"
#include "../utils.hpp"
#include "../Operator.hpp"
#ifdef HAVE_SOLLYA
#include "FPConstMult.hpp"
#include "CRFPConstMult.hpp"
#include "../FPNumber.hpp"

using namespace std;

namespace flopoco{


	CRFPConstMult::CRFPConstMult(Target* target, int wE_in, int wF_in, int wE_out, int wF_out, string _constant):
		FPConstMult(target, wE_in, wF_in, wE_out, wF_out), 
		constant (_constant) 
	{
#if 0
		sollya_node_t node;
		mpfr_t mpR;
		mpz_t zz;

		srcFileName="CRFPConstMult";
		/* Convert the input string into a sollya evaluation tree */
		node = parseString(constant.c_str());	/* If conversion did not succeed (i.e. parse error) */
		if (node == 0) {
			ostringstream error;
			error << srcFileName << ": Unable to parse string "<< constant << " as a numeric constant" <<endl;
			throw error.str();
		}

		mpfr_inits(mpR, NULL);
		evaluateConstantExpression(mpR, node,  getToolPrecision());


		if(verbose){
			double r;
			r = mpfr_get_d(mpR, GMP_RNDN);
			cout << "  Constant evaluates to " <<r <<endl; 
		}
		// compute the precision -- TODO with NWB

		cst_width =  2*wF_in+4;
		REPORT(0, "***** WARNING Taking constant with 2*wF_in+4 bits. Correct rounding is not yet guaranteed. This is being implemented." );


		REPORT(INFO, "Required significand precision to reach correct rounding is " << cst_width  ); 

		evaluateConstantExpression(mpR, node,  cst_width);

		// Now convert mpR into exponent + integral significand


		// sign
		cst_sgn = mpfr_sgn(mpR);
		if(cst_sgn<0)
			mpfr_neg(mpR, mpR, GMP_RNDN);

		// compute exponent and mantissa
		cst_exp_when_mantissa_1_2 = mpfr_get_exp(mpR) - 1; //mpfr_get_exp() assumes significand in [1/2,1)  
		cst_exp_when_mantissa_int = cst_exp_when_mantissa_1_2 - cst_width + 1; 
		mpfr_init2(mpfr_cst_sig, cst_width);
		mpfr_div_2si(mpfr_cst_sig, mpR, cst_exp_when_mantissa_1_2, GMP_RNDN);
		REPORT(INFO, "mpfr_cst_sig  = " << mpfr_get_d(mpfr_cst_sig, GMP_RNDN));


		// Build the corresponding FPConstMult.

	
		// initialize mpfr_xcut_sig = 2/cst_sig, will be between 1 and 2
		mpfr_init2(mpfr_xcut_sig, 32*(cst_width+wE_in+wE_out)); // should be accurate enough
		mpfr_set_d(mpfr_xcut_sig, 2.0, GMP_RNDN);               // exaxt op
		mpfr_div(mpfr_xcut_sig, mpfr_xcut_sig, mpfr_cst_sig, GMP_RNDD);

		// now  round it down to wF_in+1 bits 
		mpfr_t xcut_wF;
		mpfr_init2(xcut_wF, wF_in+1);
		mpfr_set(xcut_wF, mpfr_xcut_sig, GMP_RNDD);
		mpfr_mul_2si(xcut_wF, xcut_wF, wF_in, GMP_RNDN);

		// It should now be an int; cast it into a mpz, then a mpz_class 
		mpz_init2(zz, wF_in+1);
		mpfr_get_z(zz, xcut_wF, GMP_RNDN);
		xcut_sig_rd = mpz_class(zz);
		mpz_clear(zz);

		REPORT(DETAILED, "mpfr_xcut_sig = " << mpfr_get_d(mpfr_xcut_sig, GMP_RNDN) );

		// Now build the mpz significand
		mpfr_mul_2si(mpfr_cst_sig,  mpfr_cst_sig, cst_width, GMP_RNDN);
   
		// It should now be an int; cast it into a mpz, then a mpz_class 
		mpz_init2(zz, cst_width);
		mpfr_get_z(zz, mpfr_cst_sig, GMP_RNDN);
		cst_sig = mpz_class(zz);
		mpz_clear(zz);
		REPORT(DETAILED, "mpzclass cst_sig = " << cst_sig);


		// build the name
		ostringstream name; 
		name <<"FPConstMult_"<<(cst_sgn==0?"":"M") <<cst_sig<<"b"
			  <<(cst_exp_when_mantissa_int<0?"M":"")<<abs(cst_exp_when_mantissa_int)
			  <<"_"<<wE_in<<"_"<<wF_in<<"_"<<wE_out<<"_"<<wF_out;
		uniqueName_=name.str();


		// cleaning up
		mpfr_clears(mpR, mpfr_xcut_sig, xcut_wF, mpfr_cst_sig, NULL);

		icm = new IntConstMult(target, wF_in+1, cst_sig);
		oplist.push_back(icm);

		buildVHDL();

#endif
	}



	CRFPConstMult::~CRFPConstMult() {
		// TODO but who cares really
	}


}
#endif //HAVE_SOLLYA
