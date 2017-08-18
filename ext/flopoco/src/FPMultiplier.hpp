#ifndef FPMULTIPLIERS_HPP
#define FPMULTIPLIERS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "Operator.hpp"
#include "IntMultiplier.hpp"
#include "IntAdder.hpp"

namespace flopoco{

	/** The FPMultiplier class */
	class FPMultiplier : public Operator
	{
	public:
	
		/**
		 * The FPMutliplier constructor
		 * @param[in]		target		the target device
		 * @param[in]		wEX			the the with of the exponent for the f-p number X
		 * @param[in]		wFX			the the with of the fraction for the f-p number X
		 * @param[in]		wEY			the the with of the exponent for the f-p number Y
		 * @param[in]		wFY			the the with of the fraction for the f-p number Y
		 * @param[in]		wER			the the with of the exponent for the multiplication result
		 * @param[in]		wFR			the the with of the fraction for the multiplication result
		 **/
		FPMultiplier(Target* target, int wEX, int wFX, int wEY, int wFY, int wER, int wFR, 
		             bool norm = true, bool correctlyRounded=true, double ratio=1, int maxTimeInMinutes=1, map<string, double> inputDelays = emptyDelayMap);

		/**
		 * FPMultiplier destructor
		 */
		~FPMultiplier();

		/**
		 * Emulate the operator using MPFR.
		 * @param tc a TestCase partially filled with input values 
		 */
		void emulate(TestCase * tc);


	protected:
	
		int  wEX_;                  /**< The width of the exponent for the input X */
		int  wFX_;                  /**< The width of the fraction for the input X */
		int  wEY_;                  /**< The width of the exponent for the input Y */
		int  wFY_;                  /**< The width of the fraction for the input Y */
		int  wER_;                  /**< The width of the exponent for the output R */
		int  wFR_;                  /**< The width of the fraction for the output R */
		bool normalized_;	          /**< Signal if the output of the operator is to be or not normalized*/
		bool correctlyRounded_;	    /**< true: operator computes correct rounding; false: operator computes faithful rounding */

 
	};
}

#endif
