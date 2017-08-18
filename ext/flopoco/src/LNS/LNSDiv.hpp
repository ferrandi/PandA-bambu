#ifndef LNSDIV_HPP
#define LNSDIV_HPP

#include "../Operator.hpp"

namespace flopoco{

	struct LNSDiv : Operator
	{
		LNSDiv(Target * target, int wE, int wF);
		virtual ~LNSDiv();

		virtual void outputVHDL(std::ostream& o, std::string name);
	
	private:
		int wE;
		int wF;
	};
}
#endif
