#ifndef INPUTIEEE_HPP
#define INPUTIEEE_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "Operator.hpp"
#include "FPNumber.hpp"
#include "LZOCShifterSticky.hpp"

namespace flopoco{

	/** The InputIEEE class */
	class InputIEEE : public Operator
	{
	public:
		/**
		 * The InputIEEE constructor
		 * @param[in]		target		the target device
		 * @param[in]		wE			the the with of the exponent for the f-p number X
		 * @param[in]		wF			the the with of the fraction for the f-p number X
		 */
		InputIEEE(Target* target, int wEI, int wFI, int wEO, int wFO, bool flushToZero=true);

		/**
		 * InputIEEE destructor
		 */
		~InputIEEE();



		// overloading functions from Operator

		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);

	private:
		/** The width of the exponent for the input X */
		int wEI; 
		/** The width of the fraction for the input X */
		int wFI; 
		/** The width of the exponent for the output R */
		int wEO; 
		/** The width of the fraction for the output R */
		int wFO; 
		/** used only when wEI>wEO: minimal exponent representable in output format, biased with input bias */
		int underflowThreshold;
		/** used only when wEI>wEO: maximal exponent representable in output format, biased with input bias */
		int overflowThreshold;
		/** if false, convert subnormals if possible (needs more hardware). If true, always flush them to zero */
		bool flushToZero;
		/** The shifter used to normalize subnormal numbers */
		LZOCShifterSticky* shifter;
	};

}
#endif //INPUTIEEE_HPP
