#include "Operator.hpp"

#include "utils.hpp"

using namespace flopoco;


class PopCount:public Operator {
	public:
	static string operatorInfo;
	int w;


	public:
	PopCount(Target * target, int w);

	~PopCount() {
	};

	void emulate(TestCase * tc);
};
