#ifndef FPADDSUB_HPP
#define FPADDSUB_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "Operator.hpp"
#include "LZOC.hpp"
#include "Shifters.hpp"
#include "FPNumber.hpp"
#include "IntAdder.hpp"
#include "IntDualSub.hpp"
#include "LZOCShifterSticky.hpp"

namespace flopoco{

	/** The FPAddSub class */
	class FPAddSub : public Operator
	{
	public:
		/**
		 * The FPAddSub constructor
		 * @param[in]		target		the target device
		 * @param[in]		wEX			the the with of the exponent for the f-p number X
		 * @param[in]		wFX			the the with of the fraction for the f-p number X
		 * @param[in]		wEY			the the with of the exponent for the f-p number Y
		 * @param[in]		wFY			the the with of the fraction for the f-p number Y
		 * @param[in]		wER			the the with of the exponent for the addition result
		 * @param[in]		wFR			the the with of the fraction for the addition result
		 */
		FPAddSub(Target* target, int wEX, int wFX, int wEY, int wFY, int wER, int wFR, map<string, double> inputDelays = emptyDelayMap);

		/**
		 * FPAddSub destructor
		 */
		~FPAddSub();


		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);
		TestCase* buildRandomTestCase(int i);



	private:
		/** The width of the exponent for the input X */
		int wEX; 
		/** The width of the fraction for the input X */
		int wFX; 
		/** The width of the exponent for the input Y */
		int wEY; 
		/** The width of the fraction for the input Y */
		int wFY; 
		/** The width of the exponent for the output R */
		int wER; 
		/** The width of the fraction for the output R */
		int wFR;
		/** Signal if the output of the operator is to be or not normalized*/

		/** The combined leading zero counter and shifter for the close path */
		LZOCShifterSticky* lzocs; 
		/** The integer adder object for subtraction in the close path */
		IntAdder *fracSubClose; 
		/** The dual subtractor for the close path */
		IntDualSub *dualSubClose;
		/** The fraction adder for the far path */
		IntAdder *fracAdder;
		/** The adder that does the final rounding */
		IntAdder *finalRoundAdd; 
		/** The right shifter for the far path */
		Shifter* rightShifter;	


		int wF;
		int wE;
		int sizeRightShift;
	
	};

}

#endif
