#ifndef FixedComplexMultiplier_HPP
#define FixedComplexMultiplier_HPP
#include <vector>
#include <sstream>

#include "../Operator.hpp"
#include "../IntMultiplier.hpp"
#include "../IntAdder.hpp"

namespace flopoco{
	
	/**
	 * Multiplier for complex numbers
	 * Depending on the value of threeMultiplications, the multiplication
	 * (a+jb)*(c+jd) can be either
	 * 		Re(z)=a*c+-b*d
	 * 		Im(z)=a*d+b*c, with 4 multiplications and 3 additions
	 * or
	 * 		m1=(a+b)*c
	 * 		m2=(d+c)*b
	 * 		m3=(d-c)*a
	 * 		Re(z)=m1-m2
	 * 		Im(z)=m1+m3, with 3 multiplications and 5 additions
	 * @param[int] wI input size
	 * @param[int] wO output size
 	 */
	class FixedComplexMultiplier : public Operator
	{
	public:
		FixedComplexMultiplier(Target* target, int wI, int wO, float threshold, bool signedOperator = true, bool threeMultiplications = false);
		~FixedComplexMultiplier();
		
		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);

		int wI, wO;
		bool signedOperator;
		float threshold; 

	};
}
#endif
