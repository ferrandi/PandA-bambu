#ifndef FixXPow3Div6_HPP
#define FixXPow3Div6_HPP

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "../Operator.hpp"

#include "../BitHeap.hpp"
#include "../Signal.hpp"
#include "../ConstMult/IntConstDiv3.hpp"

#include "../utils.hpp"

namespace flopoco{



	class FixXPow3Div6 : public Operator
	{
	public:

		FixXPow3Div6(Target* target, int msbIn_, int lsbIn_, int msbOut_, int lsbOut_, bool signedInput_ = false, map<string, double> inputDelays = emptyDelayMap);
		
		FixXPow3Div6(Operator* parentOp, Target* target, Signal* multiplicandX, int msbIn_, int lsbIn_, int msbOut_, int lsbOut_,
							 BitHeap* bitheap,
							 bool signedInput_ = false, map<string, double> inputDelays = emptyDelayMap);
		~FixXPow3Div6();

		// Overloading the virtual functions of Operator
		void emulate(TestCase* tc);
		
		int msbIn;						/**< The MSB of the input */
		int lsbIn;						/**< The LSB of the input */
										/**< The MSB and the LSB of the input give the format of the number being input */
		int msbOut;						/**< The MSB of the output */
		int lsbOut;						/**< The LSB of the output */
										/**< The MSB and the LSB of the input give the format of the number being output */

		int		wIn;					/**< The input width */
		int		wOut;					/**< The output width */
		bool	signedInput;			/**< Signed or unsigned operator */		//For now, the input is always positive, so signedInput defaults FALSE
		
		int g;							/**< The number of guard bits used for the computations */
		
		BitHeap* bitHeap;    			/**< The heap of weighted bits that will be used to do the additions */
		Operator* parentOp;				/**< The operator which envelops this constant multiplier */

	};

}


#endif
