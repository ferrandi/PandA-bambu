#ifndef FPADDER3INPUT_HPP
#define FPADDER3INPUT_HPP
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

	/** The FPAdder3Input class */
	class FPAdder3Input : public Operator
	{
	public:
		/**
		 * The FPAdder3Input constructor
		 * @param[in]		target		the target device
		 * @param[in]		wEX			the the with of the exponent for the f-p number X
		 * @param[in]		wFX			the the with of the fraction for the f-p number X
		 * @param[in]		wEY			the the with of the exponent for the f-p number Y
		 * @param[in]		wFY			the the with of the fraction for the f-p number Y
		 * @param[in]		wER			the the with of the exponent for the addition result
		 * @param[in]		wFR			the the with of the fraction for the addition result
		 */
		FPAdder3Input(Target* target, int wE, int wF, map<string, double> inputDelays = emptyDelayMap);

		/**
		 * FPAdder3Input destructor
		 */
		~FPAdder3Input();


		void emulate(TestCase * tc);
// 		void buildStandardTestCases(TestCaseList* tcl);
// 		TestCase* buildRandomTestCase(int i);



	private:


		/** The combined leading zero counter and shifter for the close path */
		LZOCShifterSticky* lzocs; 
		/** The integer adder object for subtraction in the close path */
		IntAdder *fracSubClose; 
		/** The dual subtractor for the close path */
		IntDualSub *dualSubClose;
		/** The fraction adder for the far path */
		IntAdder *fracAddFar; 
		/** The adder that does the final rounding */
		IntAdder *finalRoundAdd; 
		/** The right shifter for the far path */
		Shifter* rightShifter;	


		int sizeRightShift;
	
		int wE;
		int wF;
	};

}

#endif
