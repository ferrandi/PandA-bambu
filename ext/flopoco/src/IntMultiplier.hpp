#ifndef IntMultiplierS_HPP
#define IntMultiplierS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "Table.hpp"
#include "BitHeap.hpp"

#include "IntMultipliers/MultiplierBlock.hpp"

namespace flopoco {

	/*
	  Definition of the DSP use threshold t:
	  Consider a submultiplier block, by definition smaller than (or equal to) a DSP in both dimensions
	  let r=(submultiplier area)/(DSP area); r is between 0 and 1
	  if r >= 1-t   then use a DSP for this block 
	  So: t=0 means: any submultiplier that does not fill a DSP goes to logic
        t=1 means: any submultiplier, even very small ones, go to DSP
	*/




	/** The IntMultiplier class, getting rid of Bogdan's mess.
	 */
	class IntMultiplier : public Operator {

	public:
		/** An elementary LUT-based multiplier, written as a table so that synthesis tools don't infer DSP blocks for it*/
		class SmallMultTable: public Table {
		public:
			int dx, dy, wO;
			bool negate, signedX, signedY;
			SmallMultTable(Target* target, int dx, int dy, int wO, bool negate=false, bool signedX=false, bool signedY=false );
			mpz_class function(int x);
		};

		/**
		 * The IntMultiplier constructor
		 * @param[in] target           the target device
		 * @param[in] wX             X multiplier size (including sign bit if any)
		 * @param[in] wY             Y multiplier size (including sign bit if any)
		 * @param[in] wOut           wOut size for a truncated multiplier (0 means full multiplier)
		 * @param[in] signedIO       false=unsigned, true=signed
		 * @param[in] DSPThreshold   DSP block use threshold, see its def above
		 **/
		IntMultiplier(Target* target, int wX, int wY, int wOut=0, bool signedIO = false,
		              float DSPThreshold = 1.0, map<string, double> inputDelays = emptyDelayMap,bool enableSuperTiles=false);


		/**
		 * The virtual IntMultiplier constructor adds all the multiplier bits to some bitHeap, but no more.
		 * @param[in] parentOp      the Operator to which VHDL code will be added
		 * @param[in] bitHeap       the BitHeap to which bits will be added
		 * @param[in] x            a Signal from which the x input should be taken
		 * @param[in] y            a Signal from which the y input should be taken
		 * @param[in] wX             X multiplier size (including sign bit if any)
		 * @param[in] wY             Y multiplier size (including sign bit if any)
		 * @param[in] wOut         wOut size for a truncated multiplier (0 means full multiplier)
		 * @param[in] lsbWeight     the weight, within this BitHeap, corresponding to the LSB of the multiplier output. 
		 *                          Note that there should be enough bits below for guard bits in case of truncation.
		 *                          The method neededGuardBits() provides this information.
		 *                          For a stand-alone multiplier lsbWeight=g, otherwise lsbWeight>=g
		 * @param[in] negate     if true, the multiplier result is subtracted from the bit heap 
		 * @param[in] signedIO     false=unsigned, true=signed
		 * @param[in] DSPThreshold            DSP block use ratio
		 **/
		IntMultiplier (Operator* parentOp, BitHeap* bitHeap,  Signal* x, Signal* y, 
		               int wX, int wY, int wOut, int lsbWeight, bool negate, bool signedIO, float DSPThreshold);

		/** How many guard bits will a truncated multiplier need? Needed to set up the BitHeap of an operator using the virtual constructor */
		static int neededGuardBits(int wX, int wY, int wOut);


		/**
		 *  Destructor
		 */
		~IntMultiplier();

		/**
		 * The emulate function.
		 * @param[in] tc               a test-case
		 */
		void emulate ( TestCase* tc );

		void buildStandardTestCases(TestCaseList* tcl);





	protected:
		// add a unique identifier for the multiplier, and possibly for the block inside the multiplier
		string addUID(string name, int blockUID=-1);


		string PP(int i, int j, int uid=-1);
		string PPTbl( int i, int j, int uid=-1);
		string XY(int i, int j, int uid=-1);


		/** Fill the bit heap with all the contributions from this multiplier */
		void fillBitHeap();


		void buildLogicOnly();
		void buildTiling();






		/**	builds the logic block ( smallMultTables) 
		 *@param topX, topY -top right coordinates 
		 *@param botX, botY -bottom left coordinates 
		 *@param uid is just a number which helps to form the signal names (for multiple calling of the method
		 )	*/
		void buildHeapLogicOnly(int topX, int topY, int botX, int botY, int uid=-1);

		/**	builds the heap using DSP blocks) 
		 */
		void buildXilinxTiling();

		void buildAlteraTiling( int blockTopX, int blockTopY, int blockBottomX, int blockBottomY, int dspIndex, bool signedX, bool signedY);

		void buildFancy41x41Tiling();


		/** is called when no more dsp-s fit in a row, because of the truncation line
		 *	checks the DSPThreshold, if only DSPs should be used, only logic, or maybe both, and applies it **/
		bool worthUsingOneDSP(int topX, int topY, int botX, int botY,int wxDSP,int wyDSP);
		void addExtraDSPs(int topX, int topY, int botX, int botY, int wxDSP, int wyDSP);
		int checkTiling(int wxDSP, int wyDSP, int& horDSP, int& verDSP);




		int wxDSP, wyDSP;               /**< the width for X/Y in DSP*/
		int wXdecl;                     /**< the width for X as declared*/
		int wYdecl;                     /**< the width for Y  as declared*/
		int wX;                         /**< the width for X after possible swap such that wX>wY */
		int wY;                         /**< the width for Y after possible swap such that wX>wY */
		int wOut;						/**< the size of the output*/
		int wFull;                      /**< size of the full product: wX+wY  */
		int wTruncated;                 /**< The number of truncated bits, wFull - wOut*/
		int g ;                         /**< the number of guard bits */
		int weightShift;                /**< the shift in weight for the LSB of a truncated multiplier compared to a full one,  wFull - (wOut+g)*/
		double DSPThreshold;					/**<   says what proportion of a DSP area is allowed to be lost */
		double maxError;     			/**< the max absolute value error of this multiplier, in ulps of the result. Should be 0 for untruncated, 1 or a bit less for truncated.*/  
		double initialCP;    			/**< the initial delay, getMaxInputDelays ( inputDelays_ ).*/  
	private:
		bool useDSP;
		Operator* parentOp;  			/**< For a virtual multiplier, adding bits to some external BitHeap, 
												this is a pointer to the Operator that will provide the actual vhdl stream etc. */
		BitHeap* bitHeap;    			/**< The heap of weighted bits that will be used to do the additions */
		int lsbWeight;       			/**< the weight in the bit heap of the lsb of the multiplier result ; equals g for standalone multipliers */
		//Plotter* plotter;
		// TODO the three following variable pairs seem uglily redundant
		Signal* x;
		Signal* y; 
		string xname;
		string yname;
		string inputName1;
		string inputName2;
		bool negate;                    /**< if true this multiplier computes -xy */
		int signedIO;                   /**< true if the IOs are two's complement */
		bool enableSuperTiles;     		/** if true, supertiles are built (fewer resources, longer latency */
		int multiplierUid;
		void initialize();     			/**< initialization stuff common to both constructors*/
		vector<MultiplierBlock*> localSplitVector;	
		vector<int> multWidths;	
		//vector<DSP*> dsps;
		//ofstream fig;

		Target* target;

	};

}
#endif
