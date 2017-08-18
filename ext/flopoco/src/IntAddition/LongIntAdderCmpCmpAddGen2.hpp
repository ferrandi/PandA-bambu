#ifndef LongIntAdderCmpCmpAddGen2CmpCmpAdd_HPP
#define LongIntAdderCmpCmpAddGen2CmpCmpAdd_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../Operator.hpp"
#include "IntAdderSpecific.hpp"
#include "CarryGenerationCircuit.hpp"
#include "IntComparatorSpecific.hpp"

namespace flopoco{

	/** The LongIntAdderCmpCmpAddGen2 class for experimenting with adders. 
	 */
	class LongIntAdderCmpCmpAddGen2 : public Operator
	{
	public:
		/**
		 * The LongIntAdderCmpCmpAddGen2 constructor
		 * @param[in] target the target device
		 * @param[in] wIn    the with of the inputs and output
		 * @param[in] inputDelays the delays for each input
		 **/
		LongIntAdderCmpCmpAddGen2(Target* target, int wIn, map<string, double> inputDelays = emptyDelayMap);
		/*LongIntAdderCmpCmpAddGen2(Target* target, int wIn);
		  void cmn(Target* target, int wIn, map<string, double> inputDelays);*/
	
		/**
		 *  Destructor
		 */
		~LongIntAdderCmpCmpAddGen2();
		void outputVHDL(std::ostream& o, std::string name);

		void emulate(TestCase* tc);

	protected:
		int wIn_;                         /**< the width for X, Y and R*/

	private:
		map<string, double> inputDelays_; /**< a map between input signal names and their maximum delays */
		int bufferedInputs;               /**< variable denoting an initial buffering of the inputs */
		double maxInputDelay;             /**< the maximum delay between the inputs present in the map*/
		int nbOfChunks;                   /**< the number of chunks that the addition will be split in */
		int chunkSize_;                   /**< the suggested chunk size so that the addition can take place at the objective frequency*/
		int lastChunkSize_;               /**< */
		int *cSize;                       /**< array containing the chunk sizes for all nbOfChunks*/

	};
}
#endif
