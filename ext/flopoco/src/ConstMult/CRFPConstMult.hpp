#ifndef CRFPCONSTMULT_HPP
#define CRFPCONSTMULT_HPP
#include "../Operator.hpp"

namespace flopoco{

	class CRFPConstMult : public FPConstMult
	{
	public:
		CRFPConstMult(Target* target, int wE_in, int wF_in, int wE_out, int wF_out, string constant);
		~CRFPConstMult();

		string constant;

		int cst_width;
		// No need for the other usual methods of Operators, use those of FPConstMult

		// except for test case generation: TODO

	};

}
#endif
