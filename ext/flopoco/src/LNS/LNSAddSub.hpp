#ifndef LNSADDSUB_HPP
#define LNSADDSUB_HPP

#include "../Operator.hpp"
#include "LNSAdd.hpp"
#include "CotranHybrid.hpp"

namespace flopoco{

	struct LNSAddSub : Operator
	{
		LNSAddSub(Target * target, int wE, int wF);
		virtual ~LNSAddSub();

		virtual void outputVHDL(std::ostream& o, std::string name);
	
	private:
		int wE;
		int wF;
		int j;
	
		CotranHybrid * addsub;
	};
}
#endif
