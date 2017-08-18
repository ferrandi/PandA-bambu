#ifndef Spartan3_HPP
#define Spartan3_HPP
#include "../Target.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>


namespace flopoco{

	/** Class for representing an Spartan3 target */
	class Spartan3 : public Target
	{
	public:
		/** The default constructor. */  
		Spartan3() : Target()	{
			id_             		= "Spartan3";
			vendor_         		= "Xilinx";
			maxFrequencyMHz_		= 300;
			sizeOfBlock_ 			= 18432;	// to set TODO
			// all these values are set more or less randomly, to match  Spartan 3 more or less
			fastcarryDelay_ 		= 0.034e-9; //s   
			elemWireDelay_  		= 0.436e-9;
			lutDelay_       		= 0.15e-9; 
			multXInputs_    		= 18;
			multYInputs_    		= 18;
			// all these values are set precisely to match the Spartan3
			fdCtoQ_         		= 0.626e-9; //the deterministic delay + an approximate NET delay
			lut2_           		= 0.479e-9;
			lut3_           		= 0.479e-9; //TODO
			lut4_           		= 0.479e-9; //TODO
			muxcyStoO_      		= 0.435e-9;
			muxcyCINtoO_    		= 0.056e-9;
			ffd_            		= 0.176e-9;
			muxf5_          		= 0.291e-9; //TODO
			slice2sliceDelay_		= 0.976e-9;
			xorcyCintoO_    		= 0.786e-9;
		
			lutInputs_ 				= 4;
			
			DSPMultiplierDelay_     = 2.970e-9;
			DSPAdderDelay_          = 1.820e-9;
			DSPCascadingWireDelay_	= 0.266e-9;
			DSPToLogicWireDelay_	= 0.436e-9;

			RAMDelay_ 				= 2.970e-9; //TODO
			RAMToLogicWireDelay_	= 0.436e-9; //TODO


			hasFastLogicTernaryAdders_ = false;
			
			//---------------Floorplanning related----------------------
			multiplierPosition.push_back(3);
			multiplierPosition.push_back(35);
			
			memoryPosition.push_back(3);
			memoryPosition.push_back(35);
			
			topSliceX = 39;
			topSliceY = 47;
			
			lutPerSlice = 2;
			ffPerSlice = 2;
			
			dspHeightInLUT = 8;
			ramHeightInLUT = 8;
			
			dspPerColumn = 6;
			ramPerColumn = 6;
			//----------------------------------------------------------

		}

		/** The destructor */
		virtual ~Spartan3() {}

		/** overloading the virtual functions of Target
		 * @see the target class for more details 
		 */
		double carryPropagateDelay();
		double adderDelay(int size);
		double adder3Delay(int size);
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
		bool   suggestSubadd3Size(int &x, int wIn);
		bool   suggestSlackSubaddSize(int &x, int wIn, double slack);
		bool   suggestSlackSubadd3Size(int &x, int wIn, double slack);
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
		
		double DSPMultiplierDelay_;
		double DSPAdderDelay_;
		double DSPCascadingWireDelay_;
		double DSPToLogicWireDelay_;
		
		double RAMDelay_;
		double RAMToLogicWireDelay_;

	};

}

#endif
