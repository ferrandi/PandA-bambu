#ifndef IntIntKCMS_HPP
#define IntIntKCMS_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../IntMultiAdder.hpp"
#include "../Operator.hpp"

#include "../BitHeap.hpp"
#include "../Signal.hpp"

namespace flopoco{
/** The IntIntKCM class. 
*/
class IntIntKCM : public Operator
{
	public:
		/**
		 * The IntIntKCM constructor
		 * @param[in] target the target device
		 * @param[in] wIn    the with of the input
		 * @param[in] C      the constant
		 * @param[in] inputDelays the delays for each input
		 **/
		IntIntKCM(Target* target, int wIn, mpz_class C, bool inputTwosComplement=false, bool useBitheap = false, map<string, double> inputDelays = emptyDelayMap);
		
		IntIntKCM(Operator* parentOp_, Target* target, Signal* multiplicandX, int wIn, mpz_class C, bool inputTwosComplement=false, BitHeap* bitheap_ = NULL, map<string, double> inputDelays = emptyDelayMap);

		/**
		 *  Destructor
		 */
		~IntIntKCM();

		void emulate(TestCase* tc);
		
		int getOutputWidth();				/**< returns the number of bits of the output */

	protected:
		int wIn_;							/**< the width for the input X*/
		bool signedInput_; 	
		int chunkSize_;						/**< the size of the lut> **/
		mpz_class C_;						/**< the constant to be used for the multiplication*/
		int wOut_;
		
		bool useBitheap;
		
		BitHeap*	bitHeap;    			/**< The heap of weighted bits that will be used to do the additions */
		Operator*	parentOp;				/**< The operator which envelops this constant multiplier */
		
	private:
		map<string, double> inputDelays_;	/**< a map between input signal names and their maximum delays */

};

}
#endif
