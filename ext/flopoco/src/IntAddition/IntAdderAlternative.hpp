#ifndef INTADDERSALTRNATIVE_HPP
#define INTADDERSALTRNATIVE_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"
#include "../IntAdder.hpp"
namespace flopoco {
	
#define LOGIC      0
#define REGISTER   1
#define SLICE      2
#define LATENCY    3

#define PINF 16384
#define XILINX_OPTIMIZATION 1
	
	/** The IntAdderAlternative class for experimenting with adders.
	*/
	class IntAdderAlternative : public IntAdder {
		public:
			/**
			* The IntAdderAlternative constructor
			* @param[in] target           the target device
			* @param[in] wIn              the with of the inputs and output
			* @param[in] inputDelays      the delays for each input
			* @param[in] optimizeType     the type optimization we want for our adder.
			*            0: optimize for logic (LUT/ALUT)
			*            1: optimize register count
			*            2: optimize slice/ALM count
			* @param[in] srl              optimize for use of shift registers
			**/
			IntAdderAlternative ( Target* target, int wIn, map<string, double> inputDelays = emptyDelayMap, int optimizeType = 2, bool srl = true);
			
			/**
			* Returns the cost in LUTs of the Alternative implementation
			* @param[in] target            the target device
			* @param[in] wIn               the input width
			* @param[in] inputDelays       the map containing the input delays
			* @param[in] srl               optimize for use of shift registers
			* @return                      the number of LUTS
			*/
			int getLutCostAlternative ( Target* target, int wIn, map<string, double> inputDelays, bool srl );
			
			/**
			* Returns the cost in Registers of the Alternative implementation
			* @param[in] target            the target device
			* @param[in] wIn               the input width
			* @param[in] inputDelays       the map containing the input delays
			* @param[in] srl               optimize for use of shift registers
			* @return                      the number of Registers
			*/
			int getRegCostAlternative ( Target* target, int wIn, map<string, double> inputDelays, bool srl );
			
			/**
			* Returns the cost in Slices of the Alternative implementation
			* @param[in] target            the target device
			* @param[in] wIn               the input width
			* @param[in] inputDelays       the map containing the input delays
			* @param[in] srl               optimize for use of shift registers
			* @return                      the number of Slices
			*/
			int getSliceCostAlternative ( Target* target, int wIn, map<string, double> inputDelays, bool srl );
			
			/**
			*  Destructor
			*/
			~IntAdderAlternative();
				
		protected:
			int wIn_;                         /**< the width for X, Y and R*/
			
		private:
			double maxInputDelay;             /**< the maximum delay between the inputs present in the map*/
			int *cSize;                       /**< array containing the chunk sizes for all nbOfChunks*/
			int *cIndex;                      /**< array containing the indexes for all Chunks*/

			// new notations
			int alpha;                        /**< the chunk size */
			int beta;                         /**< the last chunk size */
			int gamma;                        /**< the first chunk size when slack is considered */
			int k;                            /**< the number of chunks */
			int w;                            /**< the addition width */
			int alternativeSlackVersion;      /**< for the slack case, two architectures are possible in the alternative case. */
			
			double objectivePeriod;           /**< the inverse of the frequency */
			bool inputsGotRegistered; 
	};
	
}
#endif
