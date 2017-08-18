#ifndef IntDualSub_HPP
#define IntDualSub_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "Operator.hpp"


namespace flopoco{

	/** The IntDualSub class computes both X-Y and Y-X, or both X-Y and X+Y 
	 */
	class IntDualSub : public Operator
	{
	public:
		/**
		 * The IntDualSub constructor
		 * @param[in] target the target device
		 * @param[in] wIn    the with of the inputs and output
		 * @param[in] opType:  if 1, compute X-Y and X+Y; if 0, compute X-Y and Y-X
		 * @param[in] inputDelays the delays for each input
		 **/
		IntDualSub(Target* target, int wIn, int opType, map<string, double> inputDelays = emptyDelayMap);
		/*IntDualSub(Target* target, int wIn);
		  void cmn(Target* target, int wIn, map<string, double> inputDelays);*/
	
		/**
		 *  Destructor
		 */
		~IntDualSub();

		void emulate(TestCase* tc);
		void buildStandardTestCases(TestCaseList* tcl);


	 
	protected:
		int wIn_;                         /**< the width for X, Y and the results */
		int opType_;					  /**< the operation type. if 0, op type is x-y y-x; if 1 op_type is x-y x+y */
		string son_;			   	  /**< second output name; can be yMx or xPy */

	private:
		map<string, double> inputDelays_; /**< a map between input signal names and their maximum delays */
		int bufferedInputs;               /**< variable denoting an initial buffering of the inputs */
		double maxInputDelay;             /**< the maximum delay between the inputs present in the map*/
		int nbOfChunks;                   /**< the number of chunks that the addition will be split in */
		int chunkSize_;                   /**< the suggested chunk size so that the addition can take place at the objective frequency*/
		int *cSize;                       /**< array containing the chunk sizes for all nbOfChunks*/

	};
}
#endif
