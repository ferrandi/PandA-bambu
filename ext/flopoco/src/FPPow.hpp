#ifndef __FPPOW_HPP
#define __FPPOW_HPP
#include <vector>
#include <sstream>

#include "Operator.hpp"
#include "LZOC.hpp"
#include "LZOCShifterSticky.hpp"
#include "Shifters.hpp"


namespace flopoco{


	class FPPow : public Operator
	{
	public:
		FPPow(Target* target, int wE, int wF, int type, int logTableSize=0, int expTableSize=0, int expDegree=0);
		~FPPow();

		void compute_error(mpfr_t & r, mpfr_t &epsE, mpfr_t& epsM, mpfr_t& epsL );

		//		Overloading the virtual functions of Operator
		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);
		/**Overloading the function of Operator */
		TestCase* buildRandomTestCase(int n); 

		int wE, wF;

		int type;      /**< 0: pow; 1: powr */
	};
}
#endif
