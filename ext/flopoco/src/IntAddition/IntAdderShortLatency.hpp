#ifndef INTADDERSSL_HPP
#define INTADDERSSL_HPP
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
	
	/** The IntAdderShortLatency class for experimenting with adders.
	*/
	class IntAdderShortLatency : public IntAdder {
		public:
			/**
			* The IntAdderShortLatency constructor
			* @param[in] target           the target device
			* @param[in] wIn              the with of the inputs and output
			* @param[in] inputDelays      the delays for each input
			* @param[in] optimizeType     the type optimization we want for our adder.
			*            0: optimize for logic (LUT/ALUT)
			*            1: optimize register count
			*            2: optimize slice/ALM count
			* @param[in] srl              optimize for use of shift registers
			**/
			IntAdderShortLatency ( Target* target, int wIn, map<string, double> inputDelays = emptyDelayMap, int optimizeType = SLICE, bool srl = true);
			
		
			/**
			* Returns the cost in LUTs of the Short-Latency implementation
			* @param[in] target            the target device
			* @param[in] wIn               the input width
			* @param[in] inputDelays       the map containing the input delays
			* @param[in] srl               optimize for use of shift registers
			* @return                      the number of LUTS
			*/
			int getLutCostShortLatency ( Target* target, int wIn, map<string, double> inputDelays, bool srl );
			

			
			/**
			* Returns the cost in Registers of the Short-Latency implementation
			* @param[in] target            the target device
			* @param[in] wIn               the input width
			* @param[in] inputDelays       the map containing the input delays
			* @param[in] srl               optimize for use of shift registers
			* @return                      the number of Registers
			*/
			int getRegCostShortLatency ( Target* target, int wIn, map<string, double> inputDelays, bool srl );
			
			/**
			* Returns the cost in Slices of the Short-Latency implementation
			* @param[in] target            the target device
			* @param[in] wIn               the input width
			* @param[in] inputDelays       the map containing the input delays
			* @param[in] srl               optimize for use of shift registers
			* @return                      the number of Slices
			*/
			int getSliceCostShortLatency ( Target* target, int wIn, map<string, double> inputDelays, bool srl );
			
//			/**
//			* Updates the parameters needed of architecture implementation: wIn is taken from class attributes
//			* @param[in]  target            the target device
//			* @param[out] alpha             the size of the chunk (except last chunk)
//			* @param[out] beta              the size of the last chunk
//			* @param[out] k                 the number of chunks
//			*/
//			void updateParameters ( Target* target, int &alpha, int &beta, int &k );
//			
//			/**
//			* Updates the parameters needed of architecture implementation: wIn is taken from class attributes
//			* @param[in]  target            the target device
//			* @param[in]  inputDelays       the map containing the input delays
//			* @param[out] alpha             the size of the chunk (except first and last chunk)
//			* @param[out] beta              the size of the last chunk
//			* @param[out] gamma             the size of the first
//			* @param[out] k                 the number of chunks
//			*/
//			void updateParameters ( Target* target, map<string, double> inputDelays, int &alpha, int &beta, int &gamma, int &k );
//			
//			/**
//			* Updates the parameters needed of architecture implementation: wIn is taken from class attributes
//			* @param[in]  target            the target device
//			* @param[in]  inputDelays       the map containing the input delays
//			* @param[out] alpha             the size of the chunk (except first and last chunk)
//			* @param[out] beta              the size of the last chunk
//			* @param[out] k                 the number of chunks
//			*/
//			void updateParameters ( Target* target, map<string, double> inputDelays, int &alpha, int &beta, int &k );
			
			/**
			* Returns the result of the optimization algorithm for the short-latency architecture on the input data.
			* @param[in] target            the target device
			* @param[in] wIn               the input width
			* @param[in] k                 the number of chunks
			*/
			void tryOptimizedChunkSplittingShortLatency ( Target* target, int wIn, int &k );
			void tryOptimizedChunkSplittingShortLatency ( Target* target, map<string, double> inputDelays, int wIn, int &k );
			
			/**
			*  Destructor
			*/
			~IntAdderShortLatency();
			
			/**
			* The emulate function.
			* @param[in] tc               a list of test-cases
			*/
			void emulate ( TestCase* tc );
			
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
	};
	
}
#endif
