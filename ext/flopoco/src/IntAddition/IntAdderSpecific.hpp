#ifndef __IntAdderSpecific_HPP
#define __IntAdderSpecific_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../Operator.hpp"
#include "../utils.hpp"

namespace flopoco{

	/** The IntAdderSpecific class for experimenting with adders. 
	 */
	class IntAdderSpecific : public Operator
	{
	public:
		/**
		 * The IntAdderSpecific constructor
		 * @param[in] target the target device
		 * @param[in] wIn    the with of the inputs and output
		 * @param[in] inputDelays the delays for each input
		 **/
		IntAdderSpecific(Target* target, int wIn, map<string, double> inputDelays = emptyDelayMap);
		/*IntAdderSpecific(Target* target, int wIn);
		  void cmn(Target* target, int wIn, map<string, double> inputDelays);*/
	
		/**
		 *  Destructor
		 */
		~IntAdderSpecific();

		void outputVHDL(std::ostream& o, std::string name);
		void emulate(TestCase* tc);

	protected:
		int wIn_;                         /**< the width for X, Y and R*/
	private:
		map<string, double> inputDelays_; /**< a map between input signal names and their maximum delays */

	};
}
#endif
