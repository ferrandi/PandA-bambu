#ifndef IntMultiAdderS_HPP
#define IntMultiAdderS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

namespace flopoco {
	
	/** The IntMultiAdder class for experimenting with adders.
	 */
	class IntMultiAdder : public Operator {
	public:

		/**
		 * The IntMultiAdder constructor
		 * @param[in] target           the target device
		 * @param[in] wIn              the with of the inputs and output
		 * @param[in] N                the number of operands
		 * @param[in] inputDelays      the delays for each input
		 * @param[in] carryIn          denotes is the operator will have a Cin input
 		 * @param[in] ambiguity        this constructor will be called only from the classes inheriting IntMultiAdder;
 		 *                             this parameter is used for disambiguate between these scenarios; 
		 **/
		IntMultiAdder ( Target* target, int wIn, int N, map<string, double> inputDelays, bool carryIn, bool ambiguity):
			Operator(target, inputDelays), wIn_(wIn), N_(N), carryIn_(carryIn){
		}
	
		/**
		 * The IntMultiAdder constructor
		 * @param[in] target           the target device
		 * @param[in] wIn              the with of the inputs and output
		 * @param[in] N                the number of operands
		 * @param[in] inputDelays      the delays for each input
		 * @param[in] carryIn          denotes is the operator will have a Cin input 
		 **/
		IntMultiAdder ( Target* target, int wIn, int N, map<string, double> inputDelays = emptyDelayMap, bool carryIn = false);

		/**
		 *  Destructor
		 */
		~IntMultiAdder();
		
		/**
		 * The emulate function.
		 * @param[in] tc               a list of test-cases
		 */
		void emulate ( TestCase* tc );
				
	protected:
		int wIn_;        /**> the width of the operands */
		int N_;          /**> the number of operands    */
		bool carryIn_;   /**> if true, the operator will have a carry-in input */
	};
	
}
#endif
