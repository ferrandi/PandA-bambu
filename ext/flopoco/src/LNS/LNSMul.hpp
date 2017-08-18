#ifndef LNSMUL_HPP
#define LNSMUL_HPP

#include "../Operator.hpp"

namespace flopoco{
	struct LNSMul : Operator
	{
		LNSMul(Target * target, int wE, int wF);
		virtual ~LNSMul();

		virtual void emulate(TestCase * tc);
	
	private:
		int wE;
		int wF;
	};
}
#endif
