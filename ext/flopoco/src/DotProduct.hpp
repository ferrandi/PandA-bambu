#ifndef DOTPRODUCT_HPP
#define DOTPRODUCT_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>

#include "Operator.hpp"
#include "IntMultiplier.hpp"
#include "LongAcc.hpp"

namespace flopoco{

	/** The DotProduct class.  */
	class DotProduct : public Operator
	{
	public:

		/**
		 * The DotProduct constructor
		 * @param[in]		target	the target device
		 * @param[in]		wE			the width of the exponent for the inputs X and Y
		 * @param[in]		wFX     the width of the fraction for the input X
		 * @param[in]		wFY     the width of the fraction for the input Y
		 * @param[in]		MaxMSBX	maximum expected weight of the MSB of the summand
		 * @param[in]		LSBA    The weight of the LSB of the accumulator; determines the final accuracy of the result
		 * @param[in]		MSBA    The weight of the MSB of the accumulator; has to greater than that of the maximal expected result
		 **/ 
		DotProduct(Target* target, int wE, int wFX, int wFY, int MaxMSBX, int LSBA, int MSBA, double ratio = 0.9, map<string, double> inputDelays = emptyDelayMap);

		/**
		 * DotProduct destructor
		 */
		~DotProduct();

	
		/**
		 * Tests the operator accuracy and relative error
		 */
		void test_precision(int n);
	
	protected:
		/** The width of the exponent for the inputs X and Y*/
		int wE; 
		/** The width of the fraction for the input X */
		int wFX;
		/** The width of the fraction for the input Y */
		int wFY; 
		/** Maximum expected weight of the MSB of the summand */
		int MaxMSBX; 
		/** The weight of the LSB of the accumulator; determines the final accuracy of the result.*/	
		int LSBA;
		/** The weight of the MSB of the accumulator; has to greater than that of the maximal expected result*/
		int MSBA;
		/** The width in bits of the accumulator*/
		int sizeAcc_;

	private:
	};
}
#endif
