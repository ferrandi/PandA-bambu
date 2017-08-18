#ifndef INTADDERCLASSICALS_HPP
#define INTADDERCLASSICALS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"
#include "../IntAdder.hpp"

namespace flopoco {

#define PINF 16384
#define XILINX_OPTIMIZATION 1
	
	/** The IntAdderClassical class for experimenting with adders.
	*/
	class IntAdderClassical : public IntAdder{
		public:
			/**
			* The IntAdderClassical constructor
			* @param[in] target           the target device
			* @param[in] wIn              the with of the inputs and output
			* @param[in] inputDelays      the delays for each input
			* @param[in] optimizeType     the type optimization we want for our adder.
			*            0: optimize for logic (LUT/ALUT)
			*            1: optimize register count
			*            2: optimize slice/ALM count
			* @param[in] srl              optimize for use of shift registers
			**/
			IntAdderClassical ( Target* target, int wIn, map<string, double> inputDelays = emptyDelayMap, int optimizeType = 2, bool srl = true);
			
			/**
			* Returns the cost in LUTs of the Classical implementation
			* @param[in] target            the target device
			* @param[in] wIn               the input width
			* @param[in] inputDelays       the map containing the input delays
			* @param[in] srl               optimize for use of shift registers
			* @return                      the number of LUTS
			*/
			int getLutCostClassical ( Target* target, int wIn, map<string, double> inputDelays, bool srl );
			
			/**
			* Returns the cost in Registers of the Classical implementation
			* @param[in] target            the target device
			* @param[in] wIn               the input width
			* @param[in] inputDelays       the map containing the input delays
			* @param[in] srl               optimize for use of shift registers
			* @return                      the number of Registers
			*/
			int getRegCostClassical ( Target* target, int wIn, map<string, double> inputDelays, bool srl );
			
			/**
			* Returns the cost in Slices of the Classical implementation
			* @param[in] target            the target device
			* @param[in] wIn               the input width
			* @param[in] inputDelays       the map containing the input delays
			* @param[in] srl               optimize for use of shift registers
			* @return                      the number of Slices
			*/
			int getSliceCostClassical ( Target* target, int wIn, map<string, double> inputDelays, bool srl );
			
			/**
			*  Destructor
			*/
			~IntAdderClassical();
			
		protected:
			int wIn_;                         /**< the width for X, Y and R*/
			
		private:
			//		map<string, double> inputDelays_; /**< a map between input signal names and their maximum delays */
			double maxInputDelay;             /**< the maximum delay between the inputs present in the map*/
			int *cSize;                       /**< array containing the chunk sizes for all nbOfChunks*/
			int *cIndex;                      /**< array containing the indexes for all Chunks*/
			
			// new notations
			int alpha;                        /**< the chunk size */
			int beta;                         /**< the last chunk size */
			int gamma;                        /**< the first chunk size when slack is considered */
			int k;                            /**< the number of chunks */
			int w;                            /**< the addition width */
			int selectedDesign;               /**< one of the 3 possible implementations */
			int classicalSlackVersion;        /**< for the slack case, two architectures are possible in the classical case. */
			int alternativeSlackVersion;      /**< for the slack case, two architectures are possible in the alternative case. */
			
			int shortLatencyVersion;          /**< the short-latency has two options, one optimized and one defalut. The default one is selected if the optimization cannot take place */
			int shortLatencyKValue;          /**<  */
			
			int shortLatencyInputRegister;
			double objectivePeriod;           /**< the inverse of the frequency */
			
			bool inputsGotRegistered; 
			};
}
#endif
