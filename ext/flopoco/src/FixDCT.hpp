#ifndef FIXDCT_HPP
#define FIXDCT_HPP

#include "Operator.hpp"
#include "utils.hpp"

#include <iostream>
#include <sstream>
#include <vector>

#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <string.h>

#include "sollya.h"

#include "Operator.hpp"

#include "BitHeap.hpp"
#include "ConstMult/FixRealKCM.hpp"

#include "FixFIR.hpp"

namespace flopoco{

	class FixDCT2 : public Operator
	{
	public:

		FixDCT2(Target* target, int p_, int N_, int k_, bool signedInput_ = false, map<string, double> inputDelays = emptyDelayMap);
		
		/*
		FixDCT(Operator* parentOp, Target* target, int p_, int N_, int k_,
							 BitHeap* bitheap,
							 bool signedInput_ = false, map<string, double> inputDelays = emptyDelayMap);
		*/
		~FixDCT2(){};

		// Overloading the virtual functions of Operator
		/* the emulate function is used to simulate in software the operator
		   in order to compare this result with those outputed by the vhdl opertator */
		void emulate(TestCase* tc);
		
		/* function used to create Standard testCase defined by the developper */
		void buildStandardTestCases(TestCaseList* tcl);
		
		//TODO: revert back to returning an mpfr_t
		long double getDCT2FilterCoeff(int i);
		string getDCT2FilterCoeffString(int i);
		
		
		int p;  					/**< The precision of inputs and outputs */ 
		int N;  					/**< The number of taps */
		int k;  					/**< The number of the current sample */
		
		bool signedInput;			/**< The inputs are signed, or not */
		
		mpfr_t mpcoeff[10000];  	/**< The coefficients of the DCT2 filter, as MPFR numbers */
		vector<string> coeff;		/**< the coefficients as strings */

		int wO;  					/**< output size, will be computed out of the constants */
		int g;						/**< The number of guard bits used for the computations */
	};

}


#endif
