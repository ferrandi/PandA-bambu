#ifndef LNS_LOGSINCOS_HPP
#define LNS_LOGSINCOS_HPP

#include "../Operator.hpp"
#include "../FixFunctions/GenericEvaluator.hpp"

namespace flopoco{

	struct LogSinCos : Operator
	{
		LogSinCos(Target * target, int fL, int fTheta, int o, EvaluationMethod method = Polynomial);
		virtual ~LogSinCos();

		int fL;
		int fTheta;
		int order;
	
	private:
		EvaluationMethod method_;
		Operator * logcos;
		Operator * logsinrcp;
		Operator * log;
	};
}
#endif
