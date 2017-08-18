#ifndef FPSQRT_HPP
#define FPSQRT_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "Operator.hpp"
#include "FPNumber.hpp"

namespace flopoco{
	/** The FPSqrt class */
	class FPSqrt : public Operator
	{
	public:
		/**
		 * The FPSqrt constructor
		 * @param[in]		target		the target device
		 * @param[in]		wE			the the with of the exponent for the f-p number X
		 * @param[in]		wF			the the with of the fraction for the f-p number X
		 */
		FPSqrt(Target* target, int wE, int wF);

		/**
		 * FPSqrt destructor
		 */
		~FPSqrt();



		/**
		 * Emulate a correctly rounded square root using MPFR.
		 * @param tc a TestCase partially filled with input values 
		 */
		void emulate(TestCase * tc);

		/* Overloading the Operator method to limit testing of NaNs and negative numbers*/
		TestCase* buildRandomTestCase(int i);
		//	void buildStandardTestCases(TestCaseList* tcl);

	
	private:
		/** The width of the exponent for the input X */
		int wE; 
		/** The width of the fraction for the input X */
		int wF; 
		/** A boolean selecting between the old digit recurrence implementation, or the polynomial-based one */
		bool useDSP;
		/** A boolean selecting between IEEE-compliant correct rounding
			 or faithful (last-bit accurate) result  */
		bool correctRounding;
	}
		;
}
#endif //FPSQRT_HPP
