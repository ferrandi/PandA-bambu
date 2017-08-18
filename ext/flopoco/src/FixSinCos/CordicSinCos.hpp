#ifndef CORDICSINCOS_HPP
#define CORDICSINCOS_HPP

#include "../Operator.hpp"
#include "../utils.hpp"
#include "../IntMultiplier.hpp"
#include "../IntAdder.hpp"
#include "../ConstMult/FixRealKCM.hpp"

//#include "CordicSinCosClassic.hpp"
// #include "CordicSinCosRedIter.hpp"

#include <vector>

namespace flopoco{ 


	class CordicSinCos : public Operator {
	  
	  public:
	  
		// constructor, defined there with two parameters (default value 0 for each)
		CordicSinCos(Target* target, int wIn, int wOut, int reducedIterations = 0, map<string, double> inputDelays = emptyDelayMap);

		// destructor
		~CordicSinCos();
		

		// Below all the functions needed to test the operator
		/* the emulate function is used to simulate in software the operator
		  in order to compare this result with those outputed by the vhdl opertator */
		void emulate(TestCase * tc);

		/* function used to create Standard testCase defined by the developper */
		void buildStandardTestCases(TestCaseList* tcl);

		
		mpz_class fp2fix(mpfr_t x, int wI, int wF);

	private:
		int wIn;                   /**< input precision, input being in [-1,1) (includes a sign bit of weight 2^0) */
		int wOut;                  /**< output precisions; output being in (-1,1) (includes a sign bit of weight 2^0) */
		int reducedIterations;     /**< 0 = normal CORDIC, 1=halved number of iterations */
		int w;                     /**< internal precision */
		int	maxIterations;         /**< index at which iterations stop */
		int g;                     /**< number of guard bits*/
		mpfr_t scale;              /**< 1-2^(wOut-1)*/
		mpfr_t constPi;
		mpfr_t kfactor;            /**< */
		vector<mpfr_t> atani;      /**< */

	};

}

#endif
