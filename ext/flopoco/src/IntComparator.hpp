#ifndef IntComparator_HPP
#define IntComparator_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

namespace flopoco{

	/** 
	 * An integer comparator for FloPoCo
	 */ 
	class IntComparator : public Operator
	{
	public:
	
		/** 
		 *  IntComparator constructor
		 * @param[in] target the target device for this operator
		 * @param[in] wIn the width of the mantissa input
		 */
		IntComparator(Target* target, int wIn, int criteria, bool constant, int constValue, map<string, double> inputDelays = emptyDelayMap);
	
		/** The IntComparator destructor */
		~IntComparator();

		void emulate(TestCase* tc);
	
	private:
	
		int          wIn_;                   /**< The number of bits of the input */
		int          criteria_;              /**< comparisson criteria. -2 -1 0 1 2 = { <, <=, =, >=, >} */

	};
}
#endif
