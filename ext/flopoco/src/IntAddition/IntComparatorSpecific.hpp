#ifndef __IntComparatorSpecific_HPP
#define __IntComparatorSpecific_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../Operator.hpp"
#include "../utils.hpp"

namespace flopoco{

	/** The IntComparatorSpecific class for experimenting with adders. 
	 */
	class IntComparatorSpecific : public Operator
	{
	public:
		/**
		 * The IntComparatorSpecific constructor
		 * @param[in] target the target device
		 * @param[in] wIn    the with of the inputs and output
		 * @param[in] inputDelays the delays for each input
		 **/
		IntComparatorSpecific(Target* target, int wIn, int type, map<string, double> inputDelays = emptyDelayMap);
		/*IntComparatorSpecific(Target* target, int wIn);
		  void cmn(Target* target, int wIn, map<string, double> inputDelays);*/
	
		/**
		 *  Destructor
		 */
		~IntComparatorSpecific();

		void outputVHDL(std::ostream& o, std::string name);
		void emulate(TestCase* tc);

	protected:
		int wIn_;                         /**< the width for X, Y and R*/
		int type_;
	private:
		map<string, double> inputDelays_; /**< a map between input signal names and their maximum delays */

	};
}
#endif
