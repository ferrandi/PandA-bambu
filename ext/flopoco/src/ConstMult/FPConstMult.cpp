/*
   A multiplier by a floating-point constant for FloPoCo

  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.
*/


// TODO Case mantissa=1, wE_out>wE_in
// TODO pipeline, 
// TODO standard test vectors: 1, 0, various exn, xcut borders


#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../sollya.h"
#include "../Operator.hpp"
#include "FPConstMult.hpp"
#include "../FPNumber.hpp"

using namespace std;


namespace flopoco{

	// The expert version // TODO define correctRounding

	FPConstMult::FPConstMult(Target* target, int wE_in_, int wF_in_, int wE_out_, int wF_out_, int cstSgn_, int cst_exp_, mpz_class cstIntSig_):
		Operator(target), 
		wE_in(wE_in_), wF_in(wF_in_), wE_out(wE_out_), wF_out(wF_out_), 
		cstSgn(cstSgn_), cst_exp_when_mantissa_int(cst_exp_), cstIntSig(cstIntSig_), constant_is_zero(false)
	{
		srcFileName="FPConstMult";
		ostringstream name;
		name <<"FPConstMult_"<<(cstSgn==0?"":"M") <<mpz2string(cstIntSig)<<"b"<<(cst_exp_when_mantissa_int<0?"M":"")<<abs(cst_exp_when_mantissa_int)<<"_"<<wE_in<<"_"<<wF_in<<"_"<<wE_out<<"_"<<wF_out;
		uniqueName_=name.str();

		if(wE_in<3 || wE_out <3){
			ostringstream error;
			error << srcFileName << ": exponent size must be at least 3" <<endl;
			throw error.str();
		}

		if(cstIntSig==0) {
			REPORT(INFO, "building a multiplier by 0, it will be easy");
			constant_is_zero=true;
		}
		else {
			// Constant normalization
			while ((cstIntSig % 2) ==0) {
				REPORT(INFO, "Significand is even, normalising");
				cstIntSig = cstIntSig >>1;
				cst_exp_when_mantissa_int+=1;
			}
			mantissa_is_one = false;
			if(cstIntSig==1) {
				REPORT(INFO, "Constant mantissa is 1, multiplying by it will be easy"); 
				mantissa_is_one = true;
			}

			cstWidth = intlog2(cstIntSig);
			cst_exp_when_mantissa_1_2 = cst_exp_when_mantissa_int + cstWidth - 1; 

			// initialize mpfr constant
			mpfr_init2(cstSig, max(cstWidth, 2));
			mpfr_set_z(cstSig, cstIntSig.get_mpz_t(), GMP_RNDN); // exact op
			mpfr_mul_2si(cstSig, cstSig, -(cstWidth-1), GMP_RNDN);  // exact op, gets cstSig in 1..2
			
			mpfr_init(mpfrC);
			mpfr_set(mpfrC, cstSig, GMP_RNDN);
			mpfr_mul_2si(mpfrC, mpfrC, cst_exp_when_mantissa_1_2, GMP_RNDN);
			
			if(cstSgn==1)
				mpfr_neg(mpfrC,  mpfrC, GMP_RNDN);
			REPORT(INFO, "mpfrC = " << mpfr_get_d(mpfrC, GMP_RNDN));


			if(!mantissa_is_one) {
				// sub component
				icm = new IntConstMult(target, wF_in+1, cstIntSig);
				oplist.push_back(icm);
			}

		}
		// do all the declarations. Pushed into a method so that CRFPConstMult can inherit it
		buildVHDL();

	}













	// The rational version
	FPConstMult::FPConstMult(Target* target, int wE_in_, int wF_in_, int wE_out_, int wF_out_, int a_, int b_):
		Operator(target), 
		wE_in(wE_in_), wF_in(wF_in_), wE_out(wE_out_), wF_out(wF_out_), constant_is_zero(false)
	{

		int a=a_;
		int b=b_;
		srcFileName="FPConstMult (rational)";

		if(b<=0){
			ostringstream error;
			error << srcFileName << ": b must be strictly positive" <<endl;
			throw error.str();
		}
		if(wE_in<3 || wE_out <3){
			ostringstream error;
			error << srcFileName << ": exponent size must be at least 3" <<endl;
			throw error.str();
		}

		//build mpfrC out of the initials a and b, it's safer
		mpfr_t mpa, mpb;
		mpfr_inits(mpfrC,mpa,mpb, NULL);
		mpfr_set_si(mpa, a, GMP_RNDN);
		mpfr_set_si(mpb, b, GMP_RNDN);
		mpfr_set_prec(mpfrC, 10*(wE_in+wF_in+wE_out+wF_out)); // should be enough for anybody
		mpfr_div(mpfrC, mpa, mpb, GMP_RNDN);
		
		REPORT(DEBUG, "Constant evaluates as " << mpfr_get_d( mpfrC, GMP_RNDN) );

		if(a<0){
			cstSgn=1;
			a=-a;
		}
		

		// First simplify the fraction: compute GCD by Euclide, then divide a and b by it
		int ae=max(a,b);
		int be=min(a,b);
		int re=ae%be;
		while(re!=0) {
			ae=be;
			be=re;
			re=ae%be;
		}
		a=a/be;
		b=b/be;
		REPORT(DEBUG, "GCD of a and b is " << be << ", simplified fraction to " << a << "/" << b);

		// Now get rid of powers of two in both a and b.
		// we transform them into (a',b',expUpdate) such that a' and b' are both odd
		// and a/b = a'/b'.2^expUpdate
		int expUpdate=0; 
		while((a&1)==0) {
			a=a>>1;
			expUpdate++;
		}
		while((b&1)==0) {
			b=b>>1;
			expUpdate--;
		}
		
		if(b==1) {
			throw("This fraction does not have an infinite binary representation -- not implemented yet");
		}

		mantissa_is_one = (a==b); // fairly different architecture in this case

		if(mantissa_is_one){
			mpfr_set_d(mpfrC, 1.0, GMP_RNDN);
			mpfr_mul_2si(mpfrC,mpfrC, expUpdate, GMP_RNDN);
			cst_exp_when_mantissa_1_2 = expUpdate;
		}
		else
			{
		
			// Now look for a period
			int periodSize, headerSize; 
			mpz_class periodicPattern, header; // The two values to look for
			mpfr_set_si(mpa, a, GMP_RNDN);
			mpfr_set_si(mpb, b, GMP_RNDN);


			mpz_class aa, bb, cc;
			aa=a; 
			bb=b;

			// First euclidean division of a by b
			header = aa/bb;
			headerSize=intlog2(header);

			cc = aa - header*bb; // remainder
			REPORT(DEBUG, "fraction " << a_ << "/" << b_ << " rewritten as " << header << "+" << cc << "/" << bb );
			// Now look for the order of 2 modulo bb
			periodSize=1;
			mpz_class twotoperiodSize=2;
			while ((twotoperiodSize % bb) != 1){
				periodSize++;
				twotoperiodSize = twotoperiodSize <<1;
			}
			// and the period is 2^periodSize/b
			periodicPattern = cc*twotoperiodSize/b;


			// The period may begin with zeroes. 
			// Example: 1/3, or any other smaller than 0.5
			// but the following will generate optimal code
			// 


			REPORT(DETAILED, "Found header " << header.get_str(2) << " of size "<< headerSize 
						 << " and period " << periodicPattern.get_str(2) << " of size " << periodSize);
	 
			int wC;

			if(wF_out >= wF_in) {
				correctRounding=true;
				wC = wF_out + 1  +1 + intlog2(b);
				REPORT(INFO, "Building a correctly rounded multiplier");
			} 
			else {
				correctRounding=false;
				wC = wF_out + 3;
				REPORT(INFO, "wF_out < <F_in, building a faithful multiplier");
			}

			int r = ceil(   ((double)(wC-headerSize)) / ((double)periodSize)   ); // Needed repetitions
			REPORT(DETAILED, "target wC=" << wC << ", need to repeat the period " << r << " times");
			int i = intlog2(r) -1; // 2^i < r < 2^{i+1}
			int rr = r - (1<<i);
			int j;
			if (rr==0)
				j=-1;
			else
				j= intlog2(rr-1);

			// now round up the number of needed repetitions
			if(j==-1) {
				r=(1<<i);
				REPORT(DETAILED, "... Will repeat 2^i with i=" << i);
			}
			else{
				r=(1<<i)+(1<<j);
				REPORT(DETAILED, "... Will repeat 2^i+2^j = " << (1<<i) + (1<<j) << " with i=" << i << " and j=" << j);
			}


			// Rebuild the integer mantissa, just to check
			cstIntSig = header;
			for(int k=0; k<r; k++)
				cstIntSig = (cstIntSig<<periodSize) + periodicPattern;
			REPORT(DEBUG, "Constant mantissa rebuilt as " << cstIntSig << " ==  " << cstIntSig.get_str(2) );
			// Beware, this may not be normalized (example 1/3 begins with a zero)

			cstWidth = headerSize  + r*periodSize; // may be a few bits too many if the period has MSB 0s, eg 1/3

			REPORT(DETAILED, "Final constant precision is " << cstWidth << " bits");

			int patternLeadingZeroes, patternLSBZeroes;
			if(header==0) {
			 	// Do we have leading zeroes in the pattern?
			 	patternLeadingZeroes = periodSize - intlog2(periodicPattern);
				REPORT(DETAILED, "Null header, and period starting with  " << patternLeadingZeroes << " zero(s)...");
				cstWidth -= patternLeadingZeroes;
				REPORT(DETAILED, "   ... so the practical size of the constant is " << cstWidth 
							 << " bits, and it does provide "<< cstWidth + patternLeadingZeroes << " bits of accuracy");
			}
			

			cst_exp_when_mantissa_1_2 = mpfr_get_exp(mpfrC) - 1; //mpfr_get_exp() assumes significand in [1/2,1)  
			cst_exp_when_mantissa_1_2 += expUpdate;
			cst_exp_when_mantissa_int = cst_exp_when_mantissa_1_2 - cstWidth;

			// Do we have trailing zeroes in the pattern ?
			patternLSBZeroes=0;
			while ((cstIntSig % 2) ==0) {
				REPORT(DEBUG, "Significand is even, normalising");
				cstIntSig = cstIntSig >>1;
				periodicPattern = periodicPattern >>1;
				cst_exp_when_mantissa_int++;
				patternLSBZeroes++;
			}
			REPORT(DETAILED, "Periodic pattern has " << patternLSBZeroes << " zero(s) at the LSB");


			icm = new IntConstMult(target, wF_in+1, cstIntSig, periodicPattern, patternLSBZeroes, periodSize, header, headerSize, i, j);
			oplist.push_back(icm);

		}
		
		
		// build the name
		ostringstream name; 
		name <<"FPConstMultRational_"
				 <<wE_in<<"_"<<wF_in<<"_"<<wE_out<<"_"<<wF_out<<"_"
				 <<(cstSgn==0?"":"M") <<a<<"div"<<b<<"_"<<(correctRounding?"CR":"faithful");
		uniqueName_=name.str();
		
		buildVHDL();

 		mpfr_clears(mpa, mpb, NULL);
	}







#ifdef HAVE_SOLLYA


	// The parser version
	FPConstMult::FPConstMult(Target* target, int wE_in_, int wF_in_, int wE_out_, int wF_out_, int wF_C, string constant):
		Operator(target), 
		wE_in(wE_in_), wF_in(wF_in_), wE_out(wE_out_), wF_out(wF_out_), cstWidth(wF_C), mantissa_is_one(false), constant_is_zero(false)
	{
		sollya_node_t node;

		srcFileName="FPConstMult";
		/* Convert the input string into a sollya evaluation tree */
		node = parseString(constant.c_str());	/* If conversion did not succeed (i.e. parse error) */
		if (node == 0) {
			ostringstream error;
			error << srcFileName << ": Unable to parse string "<< constant << " as a numeric constant" <<endl;
			throw error.str();
		}
		//     flopoco -verbose=1 FPConstMultParser 8 23 8 23 30 "1/7"


		mpfr_inits(mpfrC, NULL);
		evaluateConstantExpression(mpfrC, node,  getToolPrecision());
		REPORT(DEBUG, "Constant evaluates to " << mpfr_get_d(mpfrC, GMP_RNDN));



			// Nonperiodic version

			if(wF_C==0) //  means: please compute wF_C for faithful rounding
				cstWidth=wF_out+3;
			else
				cstWidth=wF_C;
			
			mpfr_set_prec(mpfrC, wF_out+3);
			evaluateConstantExpression(mpfrC, node,  cstWidth);


			setupSgnAndExpCases();
			computeExpSig();
			computeIntExpSig();
			normalizeCst();

			if(!constant_is_zero && !mantissa_is_one) {
				icm = new IntConstMult(target, wF_in+1, cstIntSig);
				oplist.push_back(icm);
			}
			
		
		
		// build the name
		ostringstream name; 
		name <<"FPConstMult_"<<(cstSgn==0?"":"M") <<cstIntSig<<"b"
				 <<(cst_exp_when_mantissa_int<0?"M":"")<<abs(cst_exp_when_mantissa_int)
				 <<"_"<<wE_in<<"_"<<wF_in<<"_"<<wE_out<<"_"<<wF_out;
		uniqueName_=name.str();
		
		
		buildVHDL();
	}

#endif //HAVE_SOLLYA









	// Set up the various constants out of an MPFR constant
	// The constant precision must be set up properly in 
	void FPConstMult::setupSgnAndExpCases()
	{

		if(mpfr_zero_p(mpfrC)) {
			REPORT(INFO, "building a multiplier by 0, it will be easy");
			constant_is_zero=true;
			return;
		}

		// sign
		if(mpfr_sgn(mpfrC)<0) {
			mpfr_neg(mpfrC, mpfrC, GMP_RNDN);
			cstSgn=1;
		} 
		else {
			cstSgn=0;
		}
	}


	// Needs: cstWidth, mpfrC
	// Provides: 
	void FPConstMult::computeExpSig()
	{
		// compute exponent and mantissa
		cst_exp_when_mantissa_1_2 = mpfr_get_exp(mpfrC) - 1; //mpfr_get_exp() assumes significand in [1/2,1)  
		mpfr_init2( cstSig, cstWidth);
		mpfr_div_2si(cstSig, mpfrC, cst_exp_when_mantissa_1_2, GMP_RNDN);
		REPORT(INFO, "cstSig  = " << mpfr_get_d(cstSig, GMP_RNDN));		
	}


	// Needs: cstWidth, cstSig
	// Obsolete
	void FPConstMult::computeXCut()
	{
		mpz_t zz;

		// initialize mpfr_xcut_sig = 2/cstIntSig, will be between 1 and 2
		mpfr_init2(mpfr_xcut_sig, 32*(cstWidth+wE_in+wE_out)); // should be accurate enough
		mpfr_set_d(mpfr_xcut_sig, 2.0, GMP_RNDN);               // exaxt op
		mpfr_div(mpfr_xcut_sig, mpfr_xcut_sig, cstSig, GMP_RNDD);

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
	}


	void FPConstMult::computeIntExpSig()
	{
		cst_exp_when_mantissa_int = mpfr_get_z_exp(cstIntSig.get_mpz_t(), mpfrC);
		REPORT(DETAILED, "mpzclass cstIntSig = " << cstIntSig);
	}


	void FPConstMult::normalizeCst()
	{
		// Constant normalization
		while ((cstIntSig % 2) ==0) {
			REPORT(DETAILED, "Significand is even, normalising");
			cstIntSig = cstIntSig >>1;
			cst_exp_when_mantissa_int += 1;
			cstWidth -= 1;
		}

		if(cstIntSig==1) {
			REPORT(INFO, "Constant mantissa is 1, multiplying by it will be easy"); 
			mantissa_is_one = true;
			return;
		}
		return; // normal constant
	}
	




	FPConstMult::FPConstMult(Target* target, int wE_in, int wF_in, int wE_out, int wF_out):
		Operator(target),
		wE_in(wE_in), wF_in(wF_in), wE_out(wE_out), wF_out(wF_out) 
	{
	}


	FPConstMult::~FPConstMult() {
		// TODO but who cares really
		// clean up memory -- with ifs, because in some cases they have not been init'd
		if(mpfrC) mpfr_clear(mpfrC);
		if(cstSig) mpfr_clear(cstSig);
		if(mpfr_xcut_sig) mpfr_clear(mpfr_xcut_sig);
	}





	void FPConstMult::buildVHDL() {

		// Set up the IO signals
		addFPInput("X", wE_in, wF_in);
		addFPOutput("R", wE_out, wF_out);

		setCopyrightString("Florent de Dinechin (2007-2011)");

		if(constant_is_zero) {
			vhdl << tab << declare("x_exn",2) << " <=  X("<<wE_in<<"+"<<wF_in<<"+2 downto "<<wE_in<<"+"<<wF_in<<"+1);"<<endl;
			vhdl << tab << declare("x_sgn") << " <=  X("<<wE_in<<"+"<<wF_in<<");"<<endl;
			
			vhdl << tab << declare("r_exn", 2) << " <=      \"00\" when ((x_exn = \"00\") or (x_exn = \"01\"))  -- zero"<<endl 
					 << tab << "         else \"11\" ;-- 0*inf = 0*NaN = NaN" << endl;

			vhdl  << tab << "R <= r_exn & x_sgn & " << rangeAssign(wE_out+wF_out-1, 0, "'0'") << ";"<<endl;
			
			return;
		}

		// bit width of constant exponent
		int wE_cst=intlog2(abs(cst_exp_when_mantissa_1_2));
		REPORT(DEBUG, "wE_cst = " << wE_cst);
	
		// We have to compute Er = E_X - bias(wE_in) + E_C + bias(wE_R)
		// Let us pack all the constants together
		mpz_class expAddend = -bias(wE_in) + cst_exp_when_mantissa_1_2  + bias(wE_out);
		int expAddendSign=0;
		if (expAddend < 0) {
			expAddend = -expAddend;
			expAddendSign=1;
		}
		int wE_sum; // will be the max size of all the considered  exponents
		wE_sum = intlog2(expAddend);
		if(wE_in > wE_sum)
			wE_sum = wE_in;
		if(wE_out > wE_sum) 
			wE_sum = wE_out;
		REPORT(DEBUG, "expAddend: " << expAddendSign << " " << expAddend << "   wE_sum " << wE_sum);

		vhdl << tab << declare("x_exn",2) << " <=  X("<<wE_in<<"+"<<wF_in<<"+2 downto "<<wE_in<<"+"<<wF_in<<"+1);"<<endl;
		vhdl << tab << declare("x_sgn") << " <=  X("<<wE_in<<"+"<<wF_in<<");"<<endl;
		vhdl << tab << declare("x_exp", wE_in) << " <=  X("<<wE_in<<"+"<<wF_in<<"-1 downto "<<wF_in<<");"<<endl;
		vhdl << tab << declare("x_sig", wF_in+1) << " <= '1' & X("<<wF_in-1 <<" downto 0);"<<endl;

		vhdl <<endl << tab << "-- significand processing"<<endl;

		if(mantissa_is_one) {			
			vhdl << tab << "-- The mantissa of the constant is  1" << endl;
			if(wF_out == wF_in) {
				vhdl << tab << declare("r_frac", wF_out) << " <= X("<<wF_in-1 <<" downto 0);"<<endl;
			}
			else if(wF_out > wF_in){
				vhdl << tab << declare("r_frac", wF_out) << " <= X("<<wF_in-1 <<" downto 0)  &  " << rangeAssign(wF_out-wF_in-1, 0, "'0'") << ";"<<endl;
				}
			else{ // wF_out < wF_in, this is a rounding of the mantissa TODO
				REPORT(INFO, "rounding of multiplication by a power of two  not implemented properly when  wF_out < wF_in, truncating. Please complain to the FloPoCo team if you need correct rounding");
				vhdl << tab << declare("r_frac", wF_out) << " <= X" << range(wF_in-1, wF_out -wF_in) << ";"<<endl;
			}
			vhdl << tab << declare("norm") << " <= '0';"<<endl;
			setSignalDelay("norm", 0.0); // save the delay for later
		}



		else{ // normal case, mantissa is not one
			inPortMap  (icm, "X", "x_sig");
			outPortMap (icm, "R","sig_prod");
			vhdl << instance(icm, "sig_mult");
			
			setCycleFromSignal("sig_prod"); 
			setCriticalPath(icm->getOutputDelay("R"));
			vhdl << tab << declare("norm") << " <= sig_prod" << of(icm->rsize -1) << ";"<<endl;
			setSignalDelay("norm", getCriticalPath()); // save the delay for later
			
			// one mux controlled by the diffusion of the "norm" bit
			manageCriticalPath(target_->localWireDelay(wF_out+1) + target_->lutDelay());
			
			vhdl << tab << declare("shifted_frac",    wF_out+1) << " <= sig_prod("<<icm->rsize -2<<" downto "<<icm->rsize - wF_out-2 <<")  when norm = '1'"<<endl
			     << tab << "           else sig_prod("<<icm->rsize -3<<" downto "<<icm->rsize - wF_out - 3<<");"<<endl;  
		}
		
		// Here if mantissa was 1 critical path is 0. Otherwise we want to reset critical path to the norm bit
		setCycleFromSignal("norm", getSignalDelay("norm"));
		
		manageCriticalPath(target_->localWireDelay() + target_->adderDelay(wE_sum+1));
		vhdl <<endl << tab << "-- exponent processing"<<endl;
		
		vhdl << tab << declare("abs_unbiased_cst_exp",wE_sum+1) << " <= \""
		     << unsignedBinary(expAddend, wE_sum+1) << "\";" << endl;
		vhdl << tab << declare("r_exp_br",    wE_out+1) << " <= "
		     << "((" << wE_sum << " downto " << wE_in << " => '0')  & x_exp)  "
		     << (expAddendSign==0 ? "+" : "-" ) << "  abs_unbiased_cst_exp"
		     << "  +  (("<<wE_sum<<" downto 1 => '0') & norm);"<<endl;



		vhdl <<endl << tab << "-- final rounding"<<endl;
		
		if(mantissa_is_one) {			
			vhdl << tab << declare("expfrac_rnd",   wE_out+1+wF_out) << " <= r_exp_br & r_frac;"<<endl;
		} 
		else {
			vhdl << tab << declare("expfrac_br",   wE_out+1+wF_out+1) << " <= r_exp_br & shifted_frac;"<<endl;
			// add the rounding bit //TODO: No  round to nearest here. OK for faithful. For CR, does this case ever appear?
			vhdl << tab << declare("expfrac_rnd1",  wE_out+1+wF_out+1) << " <= (("<<wE_out+1+wF_out <<" downto 1 => '0') & '1') + expfrac_br;"<<endl;
			vhdl << tab << declare("expfrac_rnd", wE_out+1+wF_out) << " <= expfrac_rnd1("<< wE_out+1+wF_out <<" downto  1);"<<endl;
		}

		// Handling signs is trivial
		if(cstSgn==0)
			vhdl << tab << declare("r_sgn") << " <= x_sgn; -- positive constant"<<endl;
		else
			vhdl << tab << declare("r_sgn") << " <= not x_sgn; -- negative constant"<<endl;


		// overflow handling
		vhdl << tab << declare("overflow") << " <= " ;
		if (maxExp(wE_in) + cst_exp_when_mantissa_1_2 + 1 < maxExp(wE_out)) // no overflow can ever happen
			vhdl << "'0'; --  overflow never happens for this constant and these (wE_in, wE_out)" << endl;
		else 
			vhdl <<  "expfrac_rnd(" << wE_out+wF_out << ");" << endl;

		// underflow handling
		vhdl << tab << declare("r_exp_rnd",    wE_out+1) << " <= expfrac_rnd" << range(wE_out+wF_out, wF_out) << ";"<<endl;
		vhdl << tab << declare("underflow") << " <= " ;
		if (minExp(wE_in) + cst_exp_when_mantissa_1_2 >= minExp(wE_out)) // no underflow can ever happen
			vhdl << "'0'; --  underflow never happens for this constant and these (wE_in, wE_out)" << endl;
		else 
			vhdl <<  "r_exp_rnd(" << wE_sum << ");" << endl;
			 
	
		//		vhdl << tab << declare("r_exp", wE_out) << " <= r_exp_br("<<wE_out-1<<" downto 0) ;"<<endl;

		vhdl << tab << declare("r_exn", 2) << " <=      \"00\" when ((x_exn = \"00\") or (x_exn = \"01\" and underflow='1'))  -- zero"<<endl 
			  << tab << "         else \"10\" when ((x_exn = \"10\") or (x_exn = \"01\" and overflow='1'))   -- infinity" << endl
			  << tab << "         else \"11\" when  (x_exn = \"11\")                      -- NaN" << endl
			  << tab << "         else \"01\";                                          -- normal number" << endl;

		vhdl  << tab << "r <= r_exn & r_sgn & (expfrac_rnd"<< range(wE_out+wF_out-1,0) <<");"<<endl;


	}






	// Computes multiplication by mpfrC, which is a high-precision value of the constant
	void FPConstMult::emulate(TestCase *tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");

		/* Compute correct value */
		FPNumber fpx(wE_in, wF_in);
		fpx = svX;
		mpfr_t x, ru,rd,rn;
		mpfr_init2(x, 1+wF_in);
		fpx.getMPFR(x);
		if(correctRounding){
			mpfr_init2(rn, 1+wF_out); 
			mpfr_mul(rn, x, mpfrC, GMP_RNDN);

			// Set outputs 
			FPNumber  fprn(wE_out, wF_out, rn);
			mpz_class svRN = fprn.getSignalValue();
			tc->addExpectedOutput("R", svRN);
			// clean up
			mpfr_clears(x, rn, NULL);
		}
		else{
			mpfr_init2(ru, 1+wF_out); 
			mpfr_init2(rd, 1+wF_out); 
			mpfr_mul(ru, x, mpfrC, GMP_RNDU);
			mpfr_mul(rd, x, mpfrC, GMP_RNDD);

			// Set outputs 
			FPNumber  fpru(wE_out, wF_out, ru);
			mpz_class svRU = fpru.getSignalValue();
			tc->addExpectedOutput("R", svRU);
			FPNumber  fprd(wE_out, wF_out, rd);
			mpz_class svRD = fprd.getSignalValue();
			tc->addExpectedOutput("R", svRD);
			// clean up
			mpfr_clears(x, ru, rd, NULL);
		}
	}

}

#if 0



#endif
