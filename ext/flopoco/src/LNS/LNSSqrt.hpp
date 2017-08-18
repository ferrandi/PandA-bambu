#ifndef LNSSQRT_HPP
#define LNSSQRT_HPP

#include "../Operator.hpp"

namespace flopoco{

	struct LNSSqrt : Operator
	{
		LNSSqrt(Target * target, int wE, int wF);
		virtual ~LNSSqrt();

		virtual void outputVHDL(std::ostream& o, std::string name);
		virtual void setOperatorName();	
	
	private:
		int wE;
		int wF;
	};

}
#endif
