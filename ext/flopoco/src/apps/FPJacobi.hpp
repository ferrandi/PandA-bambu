#ifndef FPJACOBI_HPP
#define FPJACOBI_HPP
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

	/** The FPJacobi class.  */
	class FPJacobi : public Operator
	{
	public:

		/**
		 * The FPJacobi constructor
		 * @param[in]		target   the target device
		 * @param[in]		wE       the width of the exponent for the inputs X and Y
		 * @param[in]		wF      the width of the fraction for the input X
		 **/ 
		FPJacobi(Target* target, int wE, int wF, int l1, int l2, int l3, int version);

		/**
		 * FPJacobi destructor
		 */
		~FPJacobi();
	
	protected:
		
		int wE;     /**< The width of the exponent for the inputs X and Y*/ 
		int wF;     /**< The width of the fraction for the input X */

	private:
		
//		FPTruncMult* fpTruncMultiplier; /**< instance of a FPMultiplier */
//		FPAdder*     longAcc;      /**< instance of a FPAdder */
		
	};
}
#endif
