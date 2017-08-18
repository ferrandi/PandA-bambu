#ifndef LZOC_HPP
#define LZOC_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "Operator.hpp"


namespace flopoco{

	/** The Leading zero/one counter class.  
	 * Recursive structure with intlog2(wIn) stages.
	 */
	class LZOC : public Operator{

	public:
		/** The LZOC constructor
		 * @param[in] target the target device for this operator
		 * @param[in] wIn the width of the input
		 */
		LZOC(Target* target, int wIn, map<string, double> inputDelays = emptyDelayMap);
	
		/** The LZOC destructor	*/
		~LZOC();


		/** 
		 * Sets the default name of this operator
		 */
		void setOperatorName();


		/**
		 * Emulate a correctly rounded division using MPFR.
		 * @param tc a TestCase partially filled with input values 
		 */
		void emulate(TestCase * tc);
	
	protected:

		int wIn_;    /**< The width of the input */
		int wOut_;   /**< The width of the output */
		int p2wOut_; /**< The value of 2^wOut, which is computed as 1<<wOut */

	};

}
#endif
