
#ifndef FPFMACC_HPP
#define FPFMACC_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>


#include "../Operator.hpp"
#include "../FPAdderSinglePath.hpp"
#include "../FPMultiplier.hpp"

namespace flopoco{

	/** The FPFMAcc class.  */
	class FPFMAcc : public Operator
	{
	public:

		/**
		 * The FPFMAcc constructor
		 * @param[in]		target   the target device
		 * @param[in]		wE       the width of the exponent for the inputs X and Y
		 * @param[in]		wF      the width of the fraction for the input X
		 **/ 
		FPFMAcc(Target* target, int wE, int wF, int adderLatency = -1);

		/**
		 * FPFMAcc destructor
		 */
		~FPFMAcc();
	
	protected:
		
		int wE;     /**< The width of the exponent for the inputs X and Y*/ 
		int wF;     /**< The width of the fraction for the input X */

	private:
		
//		FPTruncMult* fpTruncMultiplier; /**< instance of a FPMultiplier */
//		FPAdder*     longAcc;      /**< instance of a FPAdder */
		
	};
}
#endif
