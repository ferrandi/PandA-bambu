#ifndef LongIntAdderMuxNetworkS_HPP
#define LongIntAdderMuxNetworkS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../Operator.hpp"
#include "../utils.hpp"
#include "../Operator.hpp"

#include "IntAdderSpecific.hpp"
#include "CarryGenerationCircuit.hpp"

namespace flopoco{

	/** The LongIntAdderMuxNetwork class for experimenting with long fast adders. */
	class LongIntAdderMuxNetwork : public Operator
	{
	public:
		/**
		 * The LongIntAdderMuxNetwork constructor
		 * @param[in] target the target device
		 * @param[in] wIn    the with of the inputs and output
		 * @param[in] inputDelays the delays for each input
		 * @param[in] regular defaults to 0. A value of 32 for example forces the chunk-size to be 32
		 **/
		LongIntAdderMuxNetwork(Target* target, int wIn, map<string, double> inputDelays = emptyDelayMap, int regular = 0);
	
		/**
		 *  Destructor
		 */
		~LongIntAdderMuxNetwork();

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
