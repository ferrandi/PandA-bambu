#ifndef IntNAdderS_HPP
#define IntNAdderS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../Operator.hpp"
#include "../IntMultiAdder.hpp"
#include "../IntAdder.hpp"

namespace flopoco{
	/** The IntNAdder class for experimenting with adders. 
	 */
	class IntNAdder : public IntMultiAdder
	{
	public:
		/**
		 * The IntNAdder constructor
		 * @param[in] target      the target device
		 * @param[in] wIn         the with of the inputs and output
		 * @param[in] inputDelays the delays for each input
		 * @param[in] carryIn     true if the oprator accepts a carry-in 
		 **/
		IntNAdder(Target* target, int wIn, int N, map<string, double> inputDelays = emptyDelayMap, bool carryIn = false);
	
		/**
		 *  Destructor
		 */
		~IntNAdder();


//		void emulate(TestCase* tc);

	protected:
		int                 wIn_;         /**< the width for X, Y and R*/
		int                 N_;           /**< number of oprands */
		bool                carryIn_;     /**< true if this operator will have a carry-in input */

	private:
		int    nbOfChunks;                   /**< the number of chunks that the addition will be split in */

		int    alpha;                        /**< the chunk size*/
		int    beta;                         /**< the last chunk size*/

		int    *cSize;                       /**< array containing the chunk sizes for all nbOfChunks*/
	};
}
#endif
