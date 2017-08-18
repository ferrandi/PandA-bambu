#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SOLLYA

#include "FixDCT.hpp"

using namespace std;

namespace flopoco{


	FixDCT2::FixDCT2(Target* target, int p_, int N_, int k_, bool signedInput_, map<string, double> inputDelays) :
		Operator(target, inputDelays), p(p_), N(N_), k(k_), signedInput(signedInput_)
	{
		srcFileName="FixDCT2";
					
		ostringstream name;
		name << "FixDCT2_" << p << "_uid" << getNewUId();
		setName(name.str()); 
	
		setCopyrightString("Florent de Dinechin, Matei Istoan (2013)");

		for (int i=0; i<N; i++)
			addInput(join("X",i), (signedInput ? 1 : 0)+p); // sign + p bits, from weights -1 to -p		

		// guard bits for a faithful result
		int g = 1 + intlog2(N-1);
		REPORT(INFO, "g = " << g);

		mpfr_t sumAbsCoeff;
		mpfr_init2 (sumAbsCoeff, 10*(1+p));
		mpfr_set_d (sumAbsCoeff, 0.0, GMP_RNDN);
		
		for(int i=0; i<N; i++)
		{
			long double tempCoeff_ld;
			mpfr_t tempCoeff;
			
			tempCoeff_ld = getDCT2FilterCoeff(i);
			
			string tempString;
			tempString = getDCT2FilterCoeffString(i);
			coeff.push_back(tempString);
			
			mpfr_init2	(tempCoeff, 10*(1+p));
			mpfr_init2	(mpcoeff[i], 10*(1+p));
			
			mpfr_set_ld	(tempCoeff, tempCoeff_ld, GMP_RNDN);
			mpfr_set	(mpcoeff[i], tempCoeff, GMP_RNDN);
			mpfr_abs(tempCoeff, tempCoeff, GMP_RNDN);
				
			// Accumulate the absolute values
			mpfr_add(sumAbsCoeff, sumAbsCoeff, tempCoeff, GMP_RNDU);
		}
		
		// now sumAbsCoeff is the max value that the filter can take.
		double sumAbs = mpfr_get_d(sumAbsCoeff, GMP_RNDU); // just to make the following loop easier
		int leadingBit=0;
		while(sumAbs >= 2.0)
		{
			sumAbs *= 0.5;
			leadingBit++;
		}
		while(sumAbs < 1.0)
		{
			sumAbs *= 2.0;
			leadingBit--;
		}
		REPORT(INFO, "Worst-case weight of MSB of the result is " << leadingBit);

		wO = 1+ (leadingBit - (-p)) + 1; //1 + sign  ; 

		addOutput("R", wO, 2); // sign + 

		int size = 1 + (leadingBit - (-p) +1) + g; // sign + overflow  bits on the left, guard bits on the right
		REPORT(INFO, "Sum size is: "<< size);
		
		//compute the guard bits from the KCM mulipliers
		int guardBitsKCM = 0;
		
		for(int i=0; i<N; i++)
		{
			int wIn = p + 1;	//p bits + 1 sign bit
			int lsbOut = -p-g;
			double targetUlpError = 1.0;
			int temp = FixRealKCM::neededGuardBits(target, wIn, lsbOut, targetUlpError);
			
			if(temp > guardBitsKCM)
				guardBitsKCM = temp;
		}
		
		size += guardBitsKCM; // sign + overflow  bits on the left, guard bits + guard bits from KCMs on the right
		REPORT(INFO, "Sum size with KCM guard bits is: "<< size);
		
		// Creating the FIR filter that will compute the DCT2
		FixFIR* filter = new FixFIR(target, 		// the target FPGA
														p, 			// the size of the inputs
														coeff,		// the constants
														true		// use bit heaps
													);
		addSubComponent(filter);
		for(int i=0; i<N; i++)
			inPortMap (filter, join("X", i), join("X", i));
		outPortMap(filter, "R", "R_int");
		vhdl << instance(filter, "FIR_filter");
		
		//manage the critical path
		setCycleFromSignal("R_int");
		
		vhdl << tab << "R" << " <= R_int;" << endl;			
	};
	
	
	//generate the i-th coefficient
	long double FixDCT2::getDCT2FilterCoeff(int i)
	{
		mpfr_t coeffValue, temp;
		long double result;
		string resultString;
		
		mpfr_init2(temp, 10*(1+p));
		mpfr_init2(coeffValue, 10*(1+p));
		
		mpfr_set_d(coeffValue, (i+0.5)*k, GMP_RNDN);
		mpfr_const_pi(temp, GMP_RNDN);
		mpfr_mul(coeffValue, coeffValue, temp, GMP_RNDN);
		mpfr_div_d(coeffValue, coeffValue, N, GMP_RNDN);
		mpfr_cos(coeffValue, coeffValue, GMP_RNDN);
		
		result = mpfr_get_ld(coeffValue, GMP_RNDN);
		
		mpfr_clears(temp, coeffValue, (mpfr_ptr)0);
		
		return result;
	};
	
	//create the string that generates the i-th coefficient using Sollya
	string FixDCT2::getDCT2FilterCoeffString(int i)
	{
		ostringstream result;
		
		result << "cos(" << "pi/" << N << "*(" << i << "+0.5)*" << k << ")";
		
		return result.str();
	};

	
	//TODO: redo the emulate function
	void FixDCT2::emulate(TestCase * tc)
	{
		// Not completely safe: we compute everything on 10 times the required precision, and hope that rounding this result is equivalent to rounding the exact result

		mpfr_t x, t, s, rd, ru;
		mpfr_init2 (x, 1+p);
		mpfr_init2 (t, 10*(1+p));
		mpfr_init2 (s, 10*(1+p));
		mpfr_init2 (rd, 1+p);
		mpfr_init2 (ru, 1+p);
	
		mpfr_set_d(s, 0.0, GMP_RNDN); // initialize s to 0

		for (int i=0; i<N; i++)
		{
			mpz_class sx = tc->getInputValue(join("X", i));			// get the input bit vector as an integer
			sx = bitVectorToSigned(sx, 1+p);						// convert it to a signed mpz_class
			mpfr_set_z (x, sx.get_mpz_t(), GMP_RNDD);				// convert this integer to an MPFR; this rounding is exact
			mpfr_div_2si (x, x, p, GMP_RNDD);						// multiply this integer by 2^-p to obtain a fixed-point value; this rounding is again exact

			mpfr_mul(t, x, mpcoeff[i], GMP_RNDN);					// Here rounding possible, but precision used is ridiculously high so it won't matter
			
			//if(coeffsign[i]==1)
			//	mpfr_neg(t, t, GMP_RNDN); 

			mpfr_add(s, s, t, GMP_RNDN); 							// same comment as above
		}

		// now we should have in s the (exact in most cases) sum
		// round it up and down

		// make s an integer -- no rounding here
		mpfr_mul_2si (s, s, p, GMP_RNDN);

		mpz_class rdz, ruz;

		mpfr_get_z (rdz.get_mpz_t(), s, GMP_RNDD);					// there can be a real rounding here
		rdz=signedToBitVector(rdz, wO);
		tc->addExpectedOutput ("R", rdz);

		mpfr_get_z (ruz.get_mpz_t(), s, GMP_RNDU);					// there can be a real rounding here	
		ruz=signedToBitVector(ruz, wO);
		tc->addExpectedOutput ("R", ruz);
		
		mpfr_clears (x, t, s, rd, ru, NULL);
	};



	// please fill me with regression tests or corner case tests
	void FixDCT2::buildStandardTestCases(TestCaseList * tcl)
	{
		TestCase *tc;

		// first few cases to check emulate()
		// All zeroes
		tc = new TestCase(this);
		for(int i=0; i<N; i++)
			tc->addInput(join("X",i), mpz_class(0) );
		emulate(tc);
		tcl->add(tc);

		// All ones (0.11111)
		tc = new TestCase(this);
		for(int i=0; i<N; i++)
			tc->addInput(join("X",i), (mpz_class(1)<<p)-1 );
		emulate(tc);
		tcl->add(tc);

		// n cases with one 0.5 and all the other 0s
		for(int i=0; i<N; i++){
			tc = new TestCase(this);
			for(int j=0; j<N; j++){
				if(i==j)
					tc->addInput(join("X",j), (mpz_class(1)<<(p-1)) );
				else
					tc->addInput(join("X",j), mpz_class(0) );					
			}
			emulate(tc);
			tcl->add(tc);
		}
	};
}


#endif //have sollya
