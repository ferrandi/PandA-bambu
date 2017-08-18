#ifndef LZOCShifterSticky_HPP
#define LZOCShifterSticky_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

namespace flopoco{

	/** 
	 * A leading zero/one counter + shifter + sticky bit computer for FloPoCo
	 */ 
	class LZOCShifterSticky : public Operator
	{
	public:
		/** The different entity types for this operator */
		typedef enum {
			gen, /**< generic entity type. The OZB: in std_logic is present in entity */
			spec /**< specific entity type. The OZB: in std_logic is NOT present in entity */
		} entityType_t;

	
		/** 
		 *  LZOCShifterSticky constructor, used in FPLog
		 * @param[in] target the target device for this operator
		 * @param[in] wIn the width of the mantissa input
		 * @param[in] wOut the width of the mantissa output
		 * @param[in] countSize the numbers of bits to count, often equal to wIn but sometimes less (see FPLog)
		 */
		LZOCShifterSticky(Target* target, int wIn, int wOut, int wCount, bool compute_sticky, const int countType=-1, map<string, double> inputDelays = emptyDelayMap);
	
		/** The LZOCShifterSticky destructor */
		~LZOCShifterSticky();

		/** Sets the type of the entity for this operator
		 * @param eType the type of the entity for this operator. Can be
		 *              generic or specific
		 */
		void setEntityType(entityType_t eType);
	
		/** Returns the number of bits of the count
		 * @return the number of bits of the count
		 */
		int getCountWidth() const;
	
	
	
	
		void emulate(TestCase* tc);
	
		double compDelay(int n){
			if ( countType_ == -1 )
				return target_->localWireDelay() + target_->adderDelay(n/2); 
			else{
				if (n <= target_->lutInputs())
					return target_->localWireDelay() + target_->lutDelay();
				else if ( n<= target_->lutInputs()*target_->lutInputs() )
						return 2*target_->localWireDelay() + 2*target_->lutDelay();
				else if ( n< target_->lutInputs()*target_->lutInputs()*target_->lutInputs() )
						return 3*target_->localWireDelay() + 3*target_->lutDelay();
				else					
					return target_->localWireDelay() + target_->adderDelay(n/4); 
			}
		}

		double muxDelay(int selFanout){
			return target_->localWireDelay(selFanout) + target_->lutDelay(); 
		}


	private:
	
		int          wIn_;                   /**< The number of bits of the input */
		int          wOut_;                  /**< The number of bits of the shifted output */
		int          wCount_;                /**< The number of bits of the count */
		bool         computeSticky_;         /**< If true, compute the sticky bit. If false, save this hardware */
		int          countType_;             /**< -1|0|1. If -1 is present then generic LZOC is instatiated */
		string       level_[42];             /**< The names of the signals, just to make code more readable */ 
		string       leveld_[42];            /**< Same but possibly delayed  */
		int          size_[42];              /**< Their size. Do we need to count more than 2^42 bits in FloPoCo? */      
		entityType_t entityType_;            /**< Entity type. Can be either generic or specific */
		bool         levelRegistered_ [42]; /**< if boolean true, the corresponding level signal is registered*/ 
		int          countDepth_[42];        /**< the depths for the levels of the architecture */	
		mpz_class    maxValue_;              /**< utilitary var */
	};
}
#endif
