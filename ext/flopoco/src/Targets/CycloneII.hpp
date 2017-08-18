#ifndef CycloneII_HPP
#define  CycloneII_HPP
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../Target.hpp"


namespace flopoco{

	/** Class for representing an CycloneII target */
	class CycloneII : public Target
	{
	public:

		/** The default constructor. */  
		CycloneII() : Target()	{
			id_						= "CycloneII";
			vendor_					= "Altera";
			maxFrequencyMHz_		= 400;
			sizeOfBlock_			= 4608;			// the size of a primitive block is 2^9 * 9
			
			fastcarryDelay_			= 0.071e-9;		// obtained from Quartus 2 Chip Planner 11.1
			elemWireDelay_			= 0.278e-9;		// obtained from Quartus 2 Chip Planner 11.1 - local line delay
			//lut2lutDelay_			= 1.5e-10;		// ???
			lut2lutDelay_			= 0.450e-9;		// ???
			
			lutDelay_				= 0.419e-9;		// obtained from Quartus 11.1
			ffDelay_				= 0.084e-9;		// obtained from Quartus 11.1
			
			multXInputs_			= 18;
			multYInputs_			= 18;
			lutInputs_				= 4;
			almsPerLab_				= 16;			// there are 16 LEs (not exactly ALMs, but will remain like this for compatibility) per LAB
			// all these values are set precisely to match the Cyclone II
			lut2_					= 0.413e-9;		// obtained from Quartus 2 Chip Planner 11.1
			lut3_					= 0.275e-9;		// obtained from Quartus 2 Chip Planner 11.1
			lut4_					= 0.342e-9;		// obtained from Quartus 2 Chip Planner 11.1
			
			innerLABcarryDelay_		= 0.159e-9;		// obtained from Quartus 2 Chip Planner 11.1
			interLABcarryDelay_		= 0.146e-9;		// obtained from Quartus 2 Chip Planner 11.1
			shareOutToCarryOut_		= 0.000e-9;		// the Cyclone architecture does not have a share out chain
			muxStoO_				= 0.275e-9;		// obtained from Quartus 2 Chip Planner 11.1
			fdCtoQ_					= 0.352e-9;		// TODO
			carryInToSumOut_		= 0.410e-9;		// obtained from Quartus 2 Chip Planner 11.1
			
			// DSP parameters
			totalDSPs_				= 150;
			nrConfigs_				= 2;			// CycloneII has 9, 18 bit multipliers by default
		
			multiplierWidth_[0]		= 9;
			multiplierWidth_[1]		= 18;
			
			// contains the DSP output register = 0.104ns
			multiplierDelay_[0]		= 2.181e-9;		// obtained experimentaly from Quartus 2 11.1
			multiplierDelay_[1]		= 2.499e-9;		// obtained experimentaly from Quartus 2 11.1
			
			RAMDelay_				= 3.245e-9;		// obtained experimentaly from Quartus 2 11.1
			RAMToLogicWireDelay_	= 0.351e-9;		// obtained experimentaly from Quartus 2 11.1
		}
	
		/** The destructor */
		virtual ~CycloneII() {}

		/** overloading the virtual functions of Target
		 * @see the target class for more details 
		 */
		double carryPropagateDelay();
		double adderDelay(int size);
		double adder3Delay(int size){return 0;}; // currently irrelevant for this architecture
		double eqComparatorDelay(int size);
		double eqConstComparatorDelay(int size);

		double DSPMultiplierDelay(){ return multiplierDelay_[1];}
		double DSPAdderDelay(){ return 0.000e-9;} // Feature not available for this FPGA family
		double DSPCascadingWireDelay(){ return 0.378e-9;}	// TODO
		double DSPToLogicWireDelay(){ return 0.765e-9;}	// TODO
		double LogicToDSPWireDelay(){ return 0.765e-9;}	// TODO
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
		bool   suggestSubadd3Size(int &x, int wIn){return 0;}; // currently irrelevant for this architecture
		bool   suggestSlackSubaddSize(int &x, int wIn, double slack);
		bool   suggestSlackSubadd3Size(int &x, int wIn, double slack){return 0;}; // currently irrelevant for this architecture
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

		double fastcarryDelay_; 			/**< The delay of the fast carry chain */
		double lut2lutDelay_;   			/**< The delay between two LUTs */
		double ffDelay_;   				/**< The delay between two flipflops (not including elemWireDelay_) */
		double elemWireDelay_;  			/**< The elementary wire dealy (for computing the distant wire delay) */
		double lutDelay_;       			/**< The LUT delay (in seconds)*/
	
		double lut2_;           			/**< The LUT delay for 2 inputs */
		double lut3_;           			/**< The LUT delay for 3 inputs */
		double lut4_;           			/**< The LUT delay for 4 inputs */
		double innerLABcarryDelay_;		/**< The wire delay between the upper and lower parts of a LAB --> R4 & C4 interconnects */	
		double interLABcarryDelay_;		/**< The approximate wire between two LABs --> R24 & C16 interconnects */	
		double shareOutToCarryOut_;		/**< The delay between the shared arithmetic out of one LAB and the carry out of the following LAB */	
		double muxStoO_;					/**< The delay of the MUX right after the 3-LUT of a LAB */	
		double fdCtoQ_;					/**< The delay of the FlipFlop. Also contains an approximate Net Delay experimentally determined */	
		double carryInToSumOut_;			/**< The delay between the carry in and the adder outup of one LAB */
		int    almsPerLab_;					/**< The number of LEs contained by a LAB */
	
		// DSP parameters
		int 	totalDSPs_;					/**< The total number of DSP blocks available on this target */
		int	    nrConfigs_;					/**< The number of distinct predefinded multiplier widths */
		int 	multiplierWidth_[2];		/**< The multiplier width available */
		double multiplierDelay_[2];		/**< The corresponding delay for each multiplier width available */
		
		double RAMDelay_;
		double RAMToLogicWireDelay_;
		
	};


}
#endif
