#ifndef StratixV_HPP
#define  StratixV_HPP
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../Target.hpp"


namespace flopoco{

	/** Class for representing an StratixV target */
	class StratixV : public Target
	{
	public:

		/** The default constructor. */  
		StratixV() : Target()	{
			id_							= "StratixV";
			vendor_						= "Altera";
			hasFastLogicTernaryAdders_	= true;
			maxFrequencyMHz_			= 717;
			sizeOfBlock_				= 20480; 		// the size of a primitive block is 2^11 * 10
			
			fastcarryDelay_				= 0.026e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			elemWireDelay_				= 0.110e-9;		// *obtained from Quartus 2 Chip Planner 11.1
			lut2lutDelay_ 				= 0.410e-9;		// *obtained from Quartus 2 Chip Planner 11.1 - emulates R4+C3+Lab_Line
			lutDelay_					= 0.433e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			ffDelay_					= 0.156e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			
			multXInputs_				= 36;
			multYInputs_				= 36;
			lutInputs_					= 6;
			almsPerLab_					= 10;			// there are 10 ALMs per LAB
			// all these values are set precisely to match the Stratix 5
			lut2_						= 0.298e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			lut3_						= 0.298e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			lut4_						= 0.298e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			
			innerLABcarryDelay_			= 0.109e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			interLABcarryDelay_			= 0.231e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			shareOutToCarryOut_			= 0.287e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			muxStoO_					= 0.193e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			fdCtoQ_						= 0.110e-9; 	// TODO : check validity
			carryInToSumOut_			= 0.116e-9;		// *obtained from Quartus 2 Chip Planner 11.1
			
			// DSP parameters
			totalDSPs_					= 256;		
			nrConfigs_					= 5;			// StratixV has 9, 16, 18, 27, 36 bit multipliers by default
			
			multiplierWidth_[0]			= 9;
			multiplierWidth_[1]			= 16;
			multiplierWidth_[2]			= 18;
			multiplierWidth_[3]			= 27;
			multiplierWidth_[4]			= 36;

			// contains the delay of the DSP register = 0.745ns (for 36x36 bits )
			multiplierDelay_[0]			= 1.875e-9; 	// *obtained experimentaly from Quartus 2 11.1
			multiplierDelay_[1]			= 1.875e-9; 	// *obtained experimentaly from Quartus 2 11.1
			multiplierDelay_[2]			= 1.875e-9; 	// *obtained experimentaly from Quartus 2 11.1
			multiplierDelay_[3]			= 2.500e-9; 	// *obtained experimentaly from Quartus 2 11.1
			multiplierDelay_[4]			= 2.905e-9; 	// *obtained experimentaly from Quartus 2 11.1
			
			DSPMultiplierDelay_			= 1.875e-9;
			DSPAdderDelay_				= 1.030e-9;
			DSPCascadingWireDelay_		= 0.266e-9;		// TODO: update
			DSPToLogicWireDelay_		= 0.266e-9;		// TODO: update
			
			RAMDelay_					= 1.197e-9; 	// *obtained experimentaly from Quartus 2 11.1
			RAMToLogicWireDelay_		= 0.090e-9; 	// *obtained experimentaly from Quartus 2 11.1 - TODO: - check validity
			
		}
	
		/** The destructor */
		virtual ~StratixV() {}

		/** overloading the virtual functions of Target
		 * @see the target class for more details 
		 */
		double carryPropagateDelay();
		double adderDelay(int size);
		double adder3Delay(int size);
		double eqComparatorDelay(int size);
		double eqConstComparatorDelay(int size);
		
		double DSPMultiplierDelay(){ return DSPMultiplierDelay_;}
		double DSPAdderDelay(){ return DSPAdderDelay_;} //TODO
		double DSPCascadingWireDelay(){ return DSPCascadingWireDelay_;}//TODO
		double DSPToLogicWireDelay (){ return DSPToLogicWireDelay_;}	
		double LogicToDSPWireDelay (){ return DSPToLogicWireDelay_;}
		void delayForDSP(MultiplierBlock* multBlock, double currentCp, int& cycleDelay, double& cpDelay);
		
		
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
		int*   getDSPMultiplierWidths(){return multiplierWidth_;};
		int    getNrDSPMultiplier(){return nrConfigs_;};	
	
	private:

		double fastcarryDelay_; 		/**< The delay of the fast carry chain */
		double lut2lutDelay_;   		/**< The delay between two LUTs */
		double ffDelay_;   				/**< The delay between two flipflops (not including elemWireDelay_) */
		double elemWireDelay_;  		/**< The elementary wire dealy (for computing the distant wire delay) */
		double lutDelay_;      	 		/**< The LUT delay (in seconds)*/
	
		// Added by Sebi
		double lut2_;           		/**< The LUT delay for 2 inputs */
		double lut3_;           		/**< The LUT delay for 3 inputs */
		double lut4_;           		/**< The LUT delay for 4 inputs */
		double innerLABcarryDelay_;		/**< The wire delay between the upper and lower parts of a LAB --> R4 & C4 interconnects */	
		double interLABcarryDelay_;		/**< The approximate wire between two LABs --> R24 & C16 interconnects */	
		double shareOutToCarryOut_;		/**< The delay between the shared arithmetic out of one LAB and the carry out of the following LAB */	
		double muxStoO_;				/**< The delay of the MUX right after the 3-LUT of a LAB */	
		double fdCtoQ_;					/**< The delay of the FlipFlop. Also contains an approximate Net Delay experimentally determined */	
		double carryInToSumOut_;		/**< The delay between the carry in and the adder outup of one LAB */
		int    almsPerLab_;				/**< The number of ALMs contained by a LAB */
	
		// DSP parameters
		int		totalDSPs_;				/**< The total number of DSP blocks available on this target */	
		int		nrConfigs_;				/**< The number of distinct predefinded multiplier widths */
		int 	multiplierWidth_[5];	/**< The multiplier width available */
		double multiplierDelay_[5];		/**< The corresponding delay for each multiplier width available */
		double inputRegDelay_[5];		/**< The input register delay to DSP block for each multiplier width available */
		double pipe2OutReg2Add; 		/**< The DPS block pipeline register to output register delay in two-multipliers adder mode */
		double pipe2OutReg4Add; 		/**< The DPS block pipeline register to output register delay in four-multipliers adder mode */
	
		double DSPMultiplierDelay_;
		double DSPAdderDelay_;
		double DSPCascadingWireDelay_;
		double DSPToLogicWireDelay_;
		
		double RAMDelay_;
		double RAMToLogicWireDelay_;
	
	};
}
#endif
