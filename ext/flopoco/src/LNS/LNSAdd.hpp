#ifndef LNSADD_HPP
#define LNSADD_HPP

#include "../Operator.hpp"
#include "../FixFunctions/GenericEvaluator.hpp"


namespace flopoco{

	// LNSAdd: implements f+(z) = log2(1 + 2^z)
	// z in fixed-point, wE integral bits, wF fractional bits
	struct LNSAdd : Operator
	{
		LNSAdd(Target * target, int wE, int wF, int o, EvaluationMethod method = Polynomial);
		virtual ~LNSAdd();


		//virtual void fillTestCase(mpz_class a[]);

		int wE;
		int wF;
		int order;
	private:
		Operator * t[3];
	};

}
#endif
