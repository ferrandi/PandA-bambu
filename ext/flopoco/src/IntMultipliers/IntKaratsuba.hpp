#ifndef IntKaratsuba_HPP
#define IntKaratsuba_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "../Operator.hpp"
#include "../IntMultiAdder.hpp"
#include "../IntAdder.hpp"


namespace flopoco{

	/** 
	 * The Integer Multiplier class. Receives at input two numbers of 
	 * wInX and wInY widths and outputs a result having the width wOut=wInX+wInY 
	 **/
	class IntKaratsuba : public Operator
	{
	public:
		/** 
		 * The constructor of the IntKaratsuba class
		 * @param target argument of type Target containing the data for which this operator will be optimized
		 * @param wInX integer argument representing the width in bits of the input X 
		 * @param wInY integer argument representing the width in bits of the input Y
		 **/
		IntKaratsuba(Target* target, int wIn, map<string, double> inputDelays = emptyDelayMap);

		void outputVHDL(std::ostream& o, std::string name);
		/**
		 * IntKaratsuba destructor
		 */
		~IntKaratsuba();

		/**
		 * Emulates an multiplier
		 */
		void emulate(TestCase* tc);
	protected:	
	
		int wIn_; /**< the width (in bits) of the input X  */
		int wOut_; /**< the width (in bits) of the output R  */

	private:
	};

}
#endif
