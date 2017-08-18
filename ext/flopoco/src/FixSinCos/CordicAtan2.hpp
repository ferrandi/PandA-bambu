#ifndef CORDICATAN2_HPP
#define CORDICATAN2_HPP

#include "../Operator.hpp"
#include "../utils.hpp"
#include "../IntMultiplier.hpp"
#include "../IntAdder.hpp"
#include "../ConstMult/FixRealKCM.hpp"


#include <vector>

namespace flopoco{ 

	
	class CordicAtan2 : public Operator {
	  
	  public:
	  
		// Possible TODO: add an option to obtain angles in radians or whatever

		/** Constructor: w is the input and output size, all signed fixed-point number. 
		 Angle is output as a signed number between 00...00 and 11...1111, for 2pi(1-2^-w)
		      pi is 0100..00, etc.
		Actual position of the fixed point in the inputs doesn't matter */
		CordicAtan2(Target* target, int w, map<string, double> inputDelays = emptyDelayMap);

		// destructor
		~CordicAtan2();
		

		// Below all the functions needed to test the operator
		/* the emulate function is used to simulate in software the operator
		  in order to compare this result with those outputed by the vhdl opertator */
		void emulate(TestCase * tc);

		/* function used to create Standard testCase defined by the developper */
		void buildStandardTestCases(TestCaseList* tcl);


	private:
		int w;                     /**< input and output size (two's complement each, including a sign bit) */
		int	maxIterations;         /**< index at which iterations stop */
		int gXY;                   /**< number of guard bits on the (X,Y) datapath */
		int gA;                    /**< number of guard bits on the Angle datapath */
		mpfr_t scale;              /**< 1-2^(wOut-1)*/
		mpfr_t constPi;
		mpfr_t kfactor;            /**< */
		vector<mpfr_t> atani;      /**< */

	};

}

#endif
