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
		~FPPow() override;

		void compute_error(mpfr_t & r, mpfr_t &epsE, mpfr_t& epsM, mpfr_t& epsL );

		//		Overloading the virtual functions of Operator
		void emulate(TestCase * tc) override;
		void buildStandardTestCases(TestCaseList* tcl) override;
		/**Overloading the function of Operator */
		TestCase* buildRandomTestCase(int n) override; 

		int wE, wF;

		int type;      /**< 0: pow; 1: powr */
	};
}
#endif
