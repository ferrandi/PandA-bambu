#ifndef FixMultAddS_HPP
#define FixMultAddS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "IntMultiplier.hpp"
#include "BitHeap.hpp"
#include "Plotter.hpp"


namespace flopoco {

	/*
	  Definition of the DSP use threshold r:
	  Consider a submultiplier block, by definition smaller than (or equal to) a DSP in both dimensions
	  So: r=1 means any multiplier that does not fills a DSP goes to logic
	  r=0       any multiplier, even very small ones, go to DSP

	  (submultiplier area)/(DSP area) is between 0 and 1
	  if (submultiplier area)/(DSP area) is larger than r then use a DSP for it 
	*/




	/** The FixMultAdd class computes A+X*Y
	    X*Y may be placed anywhere with respect to A;
	    the product will be truncated when relevant.
	    The result is specified as its LSB, MSB.
	    
	*/
	class FixMultAdd : public Operator {

	public:
		/**
		 * The FixMultAdd generic constructor (TODO: provide simpler versions)
		 * @param[in] target            target device
		 * @param[in] wX                X multiplier size (including sign bit if any)
		 * @param[in] wY                Y multiplier size (including sign bit if any)
		 * @param[in] prodMSB           weight of the MSB of the product
		 * @param[in] resultLSB         weight of the LSB of the result
		 * @param[in] addendLSB         weight of the LSB of A
		 * @param[in] addendMSB         weight of the MSB of A
		 * @param[in] signedIO          false=unsigned, true=signed
		 * @param[in] ratio             DSP block use ratio
		 * @param[in] enableSuperTiles  if true, supertiles will decrease resource consumption but increase latency
		 **/
		FixMultAdd(Target* target, int wX, int wY, int wA, int wOut, int msbP, int lsbA, bool signedIO = true, 
		           float ratio = 0.7, bool enableSuperTiles=true, map<string, double> inputDelays = emptyDelayMap);


		/* TODO *
		 * The virtual FixMultAdd constructor adds all the multiplier bits to some bitHeap, but no more.
		 * @param[in] parentOp      the Operator to which VHDL code will be added
		 * @param[in] bitHeap       the BitHeap to which bits will be added
		 * @param[in] x            a Signal from which the x input should be taken
		 * @param[in] y            a Signal from which the y input should be taken
		 * @param[in] wX             X multiplier size (including sign bit if any)
		 * @param[in] wY             Y multiplier size (including sign bit if any)
		 * @param[in] wOut         wOut size for a truncated multiplier (0 means full multiplier)
		 * @param[in] lsbWeight     the weight, within this BitHeap, corresponding to the LSB of the multiplier output. 
		 *                          Note that there should be enough bits below for guard bits in case of truncation.
		 The method neededGuardBits() provides this information.
		 * @param[in] negate     if true, the multiplier result is subtracted from the bit heap 
		 * @param[in] signedIO     false=unsigned, true=signed
		 * @param[in] ratio            DSP block use ratio
		 **/
		//			FixMultAdd (Operator* parentOp, BitHeap* bitHeap,  Signal* x, Signal* y, 
		//			int wX, int wY, int wOut, int lsbWeight, bool negate, bool signedIO, float ratio);


		/**
		 *  Destructor
		 */
		~FixMultAdd();

		void fillBitHeap();
		/**
		 * The emulate function.
		 * @param[in] tc               a test-case
		 */
		void emulate ( TestCase* tc );

		void buildStandardTestCases(TestCaseList* tcl);



		int wX;                         /**< width of multiplicand X */
		int wY;                         /**< width of multiplicand Y */
		int wA;                     /**< width of addend A  */
		int wOut;                      /**< size of the result */

		// All the weights below are anchored on LSB of the result,  which has weight 0
		// In the bit heap it will have weight g. 
		int msbP;                    /**< weight +1 of the MSB product */
		int lsbPfull;               /** equal to msbP - wX -wY */
		int lsbA;                  /**< weight of the LSB of A */
		bool signedIO;               /**< if true, inputs and outputs are signed. */
		double ratio;               /**< between 0 and 1, the area threshhold for using DSP blocks versus logic*/
		bool enableSuperTiles;     /**< if true, supertiles are built (fewer resources, longer latency */
		string xname;              /**< VHDL name */
		string yname;              /**< VHDL name */
		string aname;              /**< VHDL name */
		int g ;                    /**< the number of guard bits if the product is truncated */
		int maxWeight;             /**< The max weight for the bit heap of this multiplier, wOut + g*/
		int wOutP;                 /**< size of the product (not counting the guard bits) */
		double maxError;     /**< the max absolute value error of this multiplier, in ulps of the result. Should be 0 for untruncated, 1 or a bit less for truncated.*/  
		double initialCP;     /**< the initial delay, getMaxInputDelays ( inputDelays_ ).*/  
		int possibleOutputs;  /**< 1 if the operator is exact, 2 if it is faithfully rounded */

	private:
		Operator* parentOp;  /**< For a virtual multiplier, adding bits to some external BitHeap, 
		                        this is a pointer to the Operator that will provide the actual vhdl stream etc. */
		BitHeap* bitHeap;    /**< The heap of weighted bits that will be used to do the additions */
		IntMultiplier *mult; /**< the virtual multiplier */
		Plotter* plotter;

	};

}
#endif
