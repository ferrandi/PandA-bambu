#ifndef CycloneV_HPP
#define  CycloneV_HPP
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../Target.hpp"


namespace flopoco{

	/** Class for representing an CycloneV target */
	class CycloneV : public Target
	{
	public:

		/** The default constructor. */  
		CycloneV() : Target()	{
			id_						= "CycloneV";
			vendor_					= "Altera";
			hasFastLogicTernaryAdders_ = true;
			maxFrequencyMHz_		= 550;
			sizeOfBlock_			= 10240;		// the size of a primitive block is 2^10 * 10
			
			fastcarryDelay_			= 0.014e-9;		// obtained from Quartus 2 Chip Planner 11.1
			elemWireDelay_			= 0.180e-9;		// obtained from Quartus 2 Chip Planner 11.1 - local line delay
			lut2lutDelay_			= 0.450e-9;		// ???
			
			lutDelay_				= 0.800e-9;		// obtained from Quartus 11.1
			ffDelay_				= 0.055e-9;		// obtained from Quartus 11.1
			
			multXInputs_			= 27;
			multYInputs_			= 27;
			lutInputs_				= 6;
			almsPerLab_				= 10;			// there are 10 ALMs per LAB
			// all these values are set precisely to match the Cyclone V
			lut2_					= 0.306e-9;		// obtained from Quartus 2 Chip Planner 11.1
			lut3_					= 0.306e-9;		// obtained from Quartus 2 Chip Planner 11.1
			lut4_					= 0.306e-9;		// obtained from Quartus 2 Chip Planner 11.1
			
			innerLABcarryDelay_		= 0.056e-9;		// obtained from Quartus 2 Chip Planner 11.1 
			interLABcarryDelay_		= 0.074e-9;		// obtained from Quartus 2 Chip Planner 11.1
			shareOutToCarryOut_		= 0.385e-9;		// obtained from Quartus 2 Chip Planner 11.1
			muxStoO_				= 0.189e-9;		// TODO
			fdCtoQ_					= 0.352e-9;		// TODO
			carryInToSumOut_		= 0.327e-9;		// obtained from Quartus 2 Chip Planner 11.1
			
			// DSP parameters
			totalDSPs_				= 342;
			nrConfigs_				= 3;			// CycloneV has 9, 18, 27 bit multipliers by default
		
			multiplierWidth_[0]		= 9;
			multiplierWidth_[1]		= 18;
			multiplierWidth_[2]		= 27;
			
			multiplierDelay_[0]		= 2.378e-9;		// obtained experimentaly from Quartus 2 11.1
			multiplierDelay_[1]		= 2.378e-9;		// obtained experimentaly from Quartus 2 11.1
			multiplierDelay_[2]		= 2.838e-9;		// obtained experimentaly from Quartus 2 11.1
			
			RAMDelay_				= 2.311e-9;		// obtained experimentaly from Quartus 2 11.1
			RAMToLogicWireDelay_	= 0.908e-9;		// obtained experimentaly from Quartus 2 11.1
		}
	
		/** The destructor */
		virtual ~CycloneV() {}

		/** overloading the virtual functions of Target
		 * @see the target class for more details 
		 */
		double carryPropagateDelay();
		double adderDelay(int size);
		double adder3Delay(int size);
		double eqComparatorDelay(int size);
		double eqConstComparatorDelay(int size);

		double DSPMultiplierDelay(){ return multiplierDelay_[1];}
		double DSPAdderDelay(){ return 0.500e-9;}
		double DSPCascadingWireDelay(){ return 0.378e-9;}	// TODO
		double DSPToLogicWireDelay(){ return 0.724e-9;}	// TODO
		double LogicToDSPWireDelay(){ return 0.724e-9;}	// TODO
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
		int 	multiplierWidth_[3];		/**< The multiplier width available */
		double multiplierDelay_[3];		/**< The corresponding delay for each multiplier width available */
		
		double RAMDelay_;
		double RAMToLogicWireDelay_;
		
	};


}
#endif
