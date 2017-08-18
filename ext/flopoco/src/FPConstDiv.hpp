#ifndef FPCONSTDIV_HPP
#define FPCONSTDIV_HPP
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "Operator.hpp"
#include "IntConstDiv.hpp"


namespace flopoco{

	class FPConstDiv : public Operator
	{
	public:
		/** The generic constructor */
		FPConstDiv(Target* target, int wE_in, int wF_in, int wE_out, int wF_out, int d, int dExp=0, int alpha=-1);
		
		
		~FPConstDiv();

		int wE_in; 
		int wF_in; 
		int wE_out; 
		int wF_out; 




		void emulate(TestCase *tc);
		void buildStandardTestCases(TestCaseList* tcl);


	private:
		int d;
		int dExp;
		int alpha;
		IntConstDiv *icd;
		bool mantissaIsOne;
		double dd; // the value of the actual constant in double: equal to d*2^dExp
		// TODO replace the above with the mpd that we have in emulate
	};

}
#endif
