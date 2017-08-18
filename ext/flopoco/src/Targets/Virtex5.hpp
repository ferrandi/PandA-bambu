#ifndef VIRTEX5_HPP
#define VIRTEX5_HPP
#include "../Target.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>


namespace flopoco{

	/** Class for representing an Virtex5 target */
	class Virtex5 : public Target
	{
	public:
		/** The default constructor. */  
		Virtex5() : Target()	{
			id_             		= "Virtex5";
			vendor_         		= "Xilinx";
			sizeOfBlock_ 			= 36 << 10 ;	// the size of a primitive block is 36Kbit
			maxFrequencyMHz_		= 500;
			// all these values are set more or less randomly, to match  virtex 5 more or less
			fastcarryDelay_ 		= 0.023e-9; //s   
			elemWireDelay_  		= 0.436e-9;
			lutDelay_       		= 0.086e-9; 
			multXInputs_    		= 25;
			multYInputs_    		= 18;
			// all these values are set precisely to match the Virtex5
			fdCtoQ_         		= 0.396e-9; //the deterministic delay + an approximate NET delay
			lut2_           		= 0.086e-9;
			lut3_           		= 0.086e-9; //TODO
			lut4_           		= 0.086e-9; //TODO
			muxcyStoO_      		= 0.305e-9;
			muxcyCINtoO_    		= 0.023e-9;
			ffd_            		= 0.022e-9;
			muxf5_          		= 0.291e-9;
			slice2sliceDelay_		= 0.436e-9;
			xorcyCintoO_    		= 0.300e-9;
			lutInputs_ 				= 6;
			nrDSPs_ 				= 160; // XC5VLX30 has 1 column of 32 DSPs
			dspFixedShift_ 			= 17; 
			
			DSPMultiplierDelay_		= 2.387e-9;
			DSPAdderDelay_			= 1.889e-9;
			DSPCascadingWireDelay_	= 0.235e-9;
			DSPToLogicWireDelay_	= 0.436e-9;

			RAMDelay_				= 1.750e-9; 
			RAMToLogicWireDelay_	= 0.436e-9;

			hasFastLogicTernaryAdders_ = false;
			
			//---------------Floorplanning related----------------------
			multiplierPosition.push_back(47);
			multiplierPosition.push_back(51);
			multiplierPosition.push_back(75);
			multiplierPosition.push_back(79);
			
			memoryPosition.push_back(7);
			memoryPosition.push_back(19);
			memoryPosition.push_back(31);
			memoryPosition.push_back(43);
			memoryPosition.push_back(55);
			memoryPosition.push_back(71);
			memoryPosition.push_back(83);
			memoryPosition.push_back(95);
			memoryPosition.push_back(111);
			
			topSliceX = 111;
			topSliceY = 159;
			
			lutPerSlice = 4;
			ffPerSlice = 4;
			
			dspHeightInLUT = 3;		// actually 2.5
			ramHeightInLUT = 5;
			
			dspPerColumn = 63;
			ramPerColumn = 31;
			//----------------------------------------------------------
		}

		/** The destructor */
		virtual ~Virtex5() {}

		/** overloading the virtual functions of Target
		 * @see the target class for more details 
		 */
		double carryPropagateDelay();
		double adderDelay(int size);
		double adder3Delay(int size){return 0;}; // currently irrelevant for Xilinx
		double eqComparatorDelay(int size);
		double eqConstComparatorDelay(int size);
		
		double DSPMultiplierDelay(){ return DSPMultiplierDelay_;}
		double DSPAdderDelay(){ return DSPAdderDelay_;}
		double DSPCascadingWireDelay(){ return DSPCascadingWireDelay_;}
		double DSPToLogicWireDelay(){ return DSPToLogicWireDelay_;}
		double LogicToDSPWireDelay(){ return DSPToLogicWireDelay_;}
		void   delayForDSP(MultiplierBlock* multBlock, double currentCp, int& cycleDelay, double& cpDelay);
		
		double RAMDelay() { return RAMDelay_; }
		double RAMToLogicWireDelay() { return RAMToLogicWireDelay_; }
		double LogicToRAMWireDelay() { return RAMToLogicWireDelay_; }
		
		void   getAdderParameters(double &k1, double &k2, int size);
		double localWireDelay(int fanout = 1);
		double lutDelay();
		double ffDelay();
		double distantWireDelay(int n);
		bool   suggestSubmultSize(int &x, int &y, int wInX, int wInY);
		bool   suggestSubaddSize(int &x, int wIn);
		bool   suggestSubadd3Size(int &x, int wIn){return 0;}; // currently irrelevant for Xilinx
		bool   suggestSlackSubaddSize(int &x, int wIn, double slack);
		bool   suggestSlackSubadd3Size(int &x, int wIn, double slack){return 0;}; // currently irrelevant for Xilinx
		bool   suggestSlackSubcomparatorSize(int &x, int wIn, double slack, bool constant);
		
		int    getIntMultiplierCost(int wInX, int wInY);
		long   sizeOfMemoryBlock();
		DSP*   createDSP(); 
		int    getEquivalenceSliceDSP();
		int    getNumberOfDSPs();
		void   getDSPWidths(int &x, int &y, bool sign = false);
		int    getIntNAdderCost(int wIn, int n);	
	
	private:
		double fastcarryDelay_; /**< The delay of the fast carry chain */
		double lutDelay_;       /**< The delay between two LUTs */
		double elemWireDelay_;  /**< The elementary wire dealy (for computing the distant wire delay) */
	
		double fdCtoQ_;         /**< The delay of the FlipFlop. Also contains an approximate Net Delay experimentally determined */
		double lut2_;           /**< The LUT delay for 2 inputs */
		double lut3_;           /**< The LUT delay for 3 inputs */
		double lut4_;           /**< The LUT delay for 4 inputs */
		double muxcyStoO_;      /**< The delay of the carry propagation MUX, from Source to Out*/
		double muxcyCINtoO_;    /**< The delay of the carry propagation MUX, from CarryIn to Out*/
		double ffd_;            /**< The Flip-Flop D delay*/
		double muxf5_;          /**< The delay of the almighty mux F5*/
		double slice2sliceDelay_;       /**< This is approximate. It approximates the wire delays between Slices */
		double xorcyCintoO_;    /**< the S to O delay of the xor gate */
		int nrDSPs_;			/**< Number of available DSPs on this target */
		int dspFixedShift_;		/**< The amount by which the DSP block can shift an input to the ALU */
		
		double DSPMultiplierDelay_;
		double DSPAdderDelay_;
		double DSPCascadingWireDelay_;
		double DSPToLogicWireDelay_;
		
		double RAMDelay_;
		double RAMToLogicWireDelay_;

	};

}
#endif
