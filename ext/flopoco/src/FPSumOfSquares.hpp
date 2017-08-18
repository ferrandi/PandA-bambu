#ifndef __FPSumOfSquares_HPP
#define __FPSumOfSquares_HPP
#include <vector>
#include <sstream>

#include "Operator.hpp"
#include "Shifters.hpp"
#include "LZOC.hpp"
#include "LZOCShifterSticky.hpp"
#include "IntAdder.hpp"
#include "IntMultiAdder.hpp"
#include "IntMultiplier.hpp"
#include "IntSquarer.hpp"
#include "FPMultiplier.hpp"
#include "FPAdderSinglePath.hpp"
#include "FPNumber.hpp"
#include "utils.hpp"

namespace flopoco{

	class FPSumOfSquares : public Operator
	{
	public:
		FPSumOfSquares(Target* target, int wE, int wF, int optimize);
		~FPSumOfSquares();

		void emulate(TestCase * tc);

		TestCase* buildRandomTestCase(int i);

		int wE, wF;

	};
}
#endif
