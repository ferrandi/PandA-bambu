#ifndef STRATIXIV_HPP
#define  STRATIXIV_HPP
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../Target.hpp"


namespace flopoco{

	/** Class for representing an StratixIV target */
	class StratixIV : public Target
	{
	public:

		/** The default constructor. */  
		StratixIV() : Target()	{
			id_							= "StratixIV";
			vendor_						= "Altera";
			hasFastLogicTernaryAdders_	= true;
			maxFrequencyMHz_			= 600;
			sizeOfBlock_				= 9216;			// the size of a (the largest) primitive block is 2^10 * 9
			
			fastcarryDelay_				= 0.009e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			elemWireDelay_				= 0.206e-11;	// *obtained from Quartus 2 Chip Planner 11.1
			lut2lutDelay_				= 0.450e-9;		// ???
			lutDelay_					= 0.460e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			ffDelay_					= 0.079e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			
			multXInputs_				= 36;
			multYInputs_				= 36;
			lutInputs_					= 6;
			almsPerLab_					= 10;			// there are 10 ALMs per LAB
			// all these values are set precisely to match the Stratix 4
			lut2_						= 0.184e-9; 	// *obtained from Quartus 2 Chip Planner
			lut3_						= 0.228e-9; 	// *obtained from Quartus 2 Chip Planner
			lut4_						= 0.235e-9; 	// *obtained from Handbook
			
			innerLABcarryDelay_			= 0.065e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			interLABcarryDelay_			= 0.148e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			shareOutToCarryOut_			= 0.275e-9; 	// obtained from Quartus 2 Chip Planner 11.1
			muxStoO_					= 0.189e-9; 	// TODO
			fdCtoQ_						= 0.394e-9; 	// *obtained from Quartus 2 Chip Planner 11.1
			carryInToSumOut_			= 0.297e-9;		// *obtained from Quartus 2 Chip Planner 11.1
			
			// DSP parameters
			totalDSPs_					= 161;		
			nrConfigs_					= 4;			// StratixIV has 9, 12, 18, 36 bit multipliers by default
		
			multiplierWidth_[0]			= 9;
			multiplierWidth_[1]			= 12;
			multiplierWidth_[2]			= 18;
			multiplierWidth_[3]			= 36;
			
			multiplierDelay_[0]			= 2.173e-9; 	// *obtained experimentaly from Quartus 2
			multiplierDelay_[1]			= 2.132e-9; 	// *obtained experimentaly from Quartus 2
			multiplierDelay_[2]			= 1.823e-9; 	// *obtained experimentaly from Quartus 2
			multiplierDelay_[3]			= 1.800e-9; 	// *obtained experimentaly from Quartus 2
			
			DSPMultiplierDelay_			= 1.823e-9;
			DSPAdderDelay_				= 1.420e-9;
			DSPCascadingWireDelay_		= 0.266e-9;		//?
			DSPToLogicWireDelay_		= 0.266e-9;		//1.619e-9;
			
			RAMDelay_					= 1.590e-9; 	// *obtained experimentaly from Quartus 2
			RAMToLogicWireDelay_		= 0.439e-9; 	//TODO
			
		}
	
		/** The destructor */
		virtual ~StratixIV() {}

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
		int 	multiplierWidth_[4];	/**< The multiplier width available */
		double  multiplierDelay_[4];	/**< The corresponding delay for each multiplier width available */
		double inputRegDelay_[4];		/**< The input register delay to DSP block for each multiplier width available */
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
