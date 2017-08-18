#ifndef SHIFTERS_HPP
#define SHIFTERS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "utils.hpp"

#include "Operator.hpp"


namespace flopoco{

	/** The Shifter class. Left and right shifters are perfectly
		 symmetrical, so both are instances of the Shifter class. Only the
		 name of the VHDL instance changes */
	class Shifter : public Operator
	{
	public:

		/** The types of shifting */
		typedef enum {
			Left, /**< Left Shifter */
			Right /**< Right Shifter */
		} ShiftDirection;
		
		/**
		 * The Shifter constructor
		 * @param[in]		target		the target device
		 * @param[in]		wIn			  the with of the input
		 * @param[in]		maxShift	the maximum shift amount
		 * @param[in]		direction	can be either Left of Right. Determines the shift direction
		 **/
		Shifter(Target* target, int wIn, int maxShift, ShiftDirection dir, map<string, double> inputDelays = emptyDelayMap);


		/** Destructor */
		~Shifter();


		/** 
		 * Sets the default name of this operator
		 */
		void setOperatorName(); 

		/**
		 * Emulate a correctly rounded division using MPFR.
		 * @param tc a TestCase partially filled with input values 
		 */
		void emulate(TestCase * tc);

		/** Returns the number of bits of the sift amount 
		 *@return number of bits of the shift amount
		 */
		int getShiftInWidth(){
			return wShiftIn_;
		}
	protected:
		int wIn_;          /**< the width of the input*/
		int maxShift_;     /**< the maximum shift amount*/
		int wOut_;         /**< the width of the output */
		int wShiftIn_; 	 /**< the number of bits of the input which determines the shift amount*/

	private:
		ShiftDirection direction_;  /**< determines the shift direction. can be Left or Right */
		double maxInputDelay_;      /**< the maximum delay found in the input map */
	};
}

#endif
