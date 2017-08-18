#ifndef FIXEDPOINTSINORCOS_HPP
#define FIXEDPOINTSINORCOS_HPP

#include "../Operator.hpp"
#include "../utils.hpp"
#include "../FixFunctions/FunctionEvaluator.hpp"

namespace flopoco{ 


	class FixSinOrCos : public Operator {
	  
	  public:
		int w;
		int degree;
		mpfr_t scale;
	  
		// constructor, defined there with two parameters (default value 0 for each)
		FixSinOrCos(Target* target, int w, int degree, map<string, double> inputDelays = emptyDelayMap);

		// destructor
		~FixSinOrCos();
		
		void changeName(std::string operatorName);


		// Below all the functions needed to test the operator
		/* the emulate function is used to simulate in software the operator
		  in order to compare this result with those outputed by the vhdl opertator */
		void emulate(TestCase * tc);

		/* function used to create Standard testCase defined by the developper */
		void buildStandardTestCases(TestCaseList* tcl);

	};

}

#endif
