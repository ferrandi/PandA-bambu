#ifndef LONGACC_HPP
#define LONGACC_HPP
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "Operator.hpp"
#include "Shifters.hpp"
#include "FPNumber.hpp"
#include "utils.hpp"

namespace flopoco{

	/** Implements a long, fixed point accumulator for accumulating floating point numbers
	 */
	class LongAcc : public Operator
	{
	public:
		/** Constructor
		 * @param target the target device
		 * @param wEX the width of the exponent 
		 * @param eFX the width of the fractional part
		 * @param MaxMSBX the weight of the MSB of the expected exponent of X
		 * @param LSBA the weight of the least significand bit of the accumulator
		 * @param MSBA the weight of the most significand bit of the accumulator
		 */ 
		LongAcc(Target* target, int wEX, int wFX, int MaxMSBX, int LSBA, int MSBA, map<string, double> inputDelays = emptyDelayMap, bool forDotProd = false, int wFY = -1);
	
		/** Destructor */
		~LongAcc();
	
		void test_precision(int n); /**< Undocumented */
		void test_precision2(); /**< Undocumented */
	
		/**
		 * Gets the correct value associated to one or more inputs.
		 * @param a the array which contains both already filled inputs and
		 *          to be filled outputs in the order specified in getTestIOMap.
		 */
		void fillTestCase(mpz_class a[]);

		void emulate(TestCase* tc);

		TestCase* buildRandomTestCase(int i);


		mpz_class mapFP2Acc(FPNumber X);
	
		mpz_class sInt2C2(mpz_class X, int width);

	protected:
		int wEX_;     /**< the width of the exponent  */
		int wFX_;     /**< the width of the fractional part */
		int MaxMSBX_; /**< the weight of the MSB of the expected exponent of X */
		int LSBA_;    /**< the weight of the least significand bit of the accumulator */
		int MSBA_;    /**< the weight of the most significand bit of the accumulator */

		mpz_class AccValue_;
		int currentIteration;
		int xOvf;	

	private:
		Shifter* shifter_;          /**<Shifter object for shifting X in place */
		int      sizeAcc_;          /**<The size of the accumulator  = MSBA-LSBA+1; */
		int      sizeAccL_;         /**< used only for carry-select implementation */
		int      sizeSummand_;      /**< the maximum size of the summand  = MaxMSBX-LSBA+1; */
		int      sizeShiftedFrac_;  /**< size of ths shifted frac  = sizeSummand + wFX;  to accomodate very small summands */
		int      maxShift_;         /**< maximum shift ammount */
		int      E0X_;              /**< the bias value */
		int      sizeShift_;        /**< the shift size */
		string   summand2cname_;    /**< ??? */
		int      c2ChunkSize_;      /**< for c2 addition */
		int      c2PipelineDepth_;  /**< for c2 addition */

		int additionNumberOfChunks_;         /**< Number of chunks of the accumulation addition */
		int rebalancedAdditionChunkSize_;    /**< The chunk size after rebalancing */
		int rebalancedAdditionLastChunkSize_;/**< The size of the last chunk after rebalancing */ 

	};

}

#endif
