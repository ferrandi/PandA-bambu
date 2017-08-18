#ifndef IntCompressorTree_HPP
#define IntCompressorTree_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <gmpxx.h>
#include "../Operator.hpp"
#include "../IntMultiAdder.hpp"

namespace flopoco{

	/** The IntCompressorTree class for experimenting with adders. 
	 */
	class IntCompressorTree : public IntMultiAdder
	{
	public:
		/**
		 * The IntCompressorTree constructor
		 * @param[in] target the target device
		 * @param[in] wIn    the with of the inputs and output
		 * @param[in] inputDelays the delays for each input
		 **/
		IntCompressorTree(Target* target, int wIn, int N, map<string, double> inputDelays = emptyDelayMap);

		bool solution(int k, int n, int targetSum, int * sol, int * coef);
		void printSolution(int n, int * sol, int * coef, int *bestSol);
		bool successor( int k, int sum, int * sol);
		bool valid(int k, int sum, int n, int * sol, int * coef);
		void bt(int k, int n, int sum, int* sol, int* coef, int targetSum, int *bestSol);
	
		int coutOnes(int k){
			int n = 0;
			while (k!=0){
				n+=(k%2);
				k=(k-(k%2))/2;
			}
			return n;
		}
		/**
		 *  Destructor
		 */
		~IntCompressorTree();


//		void emulate(TestCase* tc);

	protected:
		int wIn_;                         /**< the width for X_{0}..X_{n-1} and R*/
		int N_;                           /**< number of operands */

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
