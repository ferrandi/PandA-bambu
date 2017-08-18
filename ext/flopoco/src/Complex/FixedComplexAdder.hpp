#ifndef FixedComplexAdder_HPP
#define FixedComplexAdder_HPP
#include <vector>
#include <sstream>

#include "../Operator.hpp"
#include "../IntAdder.hpp"

namespace flopoco{
	
	/**
	 * Complex adder for fixed point numbers
	 */
	class FixedComplexAdder : public Operator
	{
	public:
		FixedComplexAdder(Target* target, int wI, int wF, bool signedOperator = true, map<string, double> inputDelays = emptyDelayMap);
		~FixedComplexAdder();
		
		void emulate(TestCase * tc);

		int wI, wF, w;
		bool signedOperator;

	};
}
#endif
