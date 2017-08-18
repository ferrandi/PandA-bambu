#ifndef LONGACC2FP_HPP
#define LONGACC2FP_HPP
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "Operator.hpp"
#include "Shifters.hpp"
#include "LZOCShifterSticky.hpp"
#include "IntAdder.hpp"

namespace flopoco{

	/** Operator which converts the output of the long accumulator to the desired FP format
	 */
	class LongAcc2FP : public Operator
	{
	public:

		/** Constructor
		 * @param target the target device
		 * @param MaxMSBX the weight of the MSB of the expected exponent of X
		 * @param LSBA the weight of the least significand bit of the accumulator
		 * @param MSBA the weight of the most significand bit of the accumulator
		 * @param wEOut the width of the output exponent 
		 * @param eFOut the width of the output fractional part
		 */ 
		LongAcc2FP(Target* target, int LSBA, int MSBA, int wEOut, int wFOut);

		/** Destructor */
		~LongAcc2FP();
		
		void emulate(TestCase * tc);
		
		void buildStandardTestCases(TestCaseList* tcl);
		
		TestCase* buildRandomTestCase(int i);



	protected:
		int LSBA_;    /**< the weight of the least significand bit of the accumulator */
		int MSBA_;    /**< the weight of the most significand bit of the accumulator */
		int wEOut_;   /**< the width of the output exponent */
		int wFOut_;   /**< the width of the output fractional part */

	private:
		Target* ownTarget_;
		IntAdder* adder_;
		LZOCShifterSticky* lzocShifterSticky_;   
		int      sizeAcc_;       /**< The size of the accumulator  = MSBA-LSBA+1; */
		int      expBias_;       /**< the exponent bias value */
		int      countWidth_;    /**< the number of bits that the leading zero/one conunter outputs the result on */

	};
}
#endif
