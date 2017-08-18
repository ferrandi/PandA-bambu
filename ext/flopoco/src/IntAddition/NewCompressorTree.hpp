#include "Operator.hpp"
#include "utils.hpp"

#include <vector>

using namespace flopoco;


class NewCompressorTree : public Operator {
public:
	static string operatorInfo;
	unsigned inSize; /**< size of the input vector */
	unsigned wOut; /**< bit size of the result */
	std::vector<unsigned> vops;  /**< at index i [little endian], contains
	                                the number of bits of weight i to be added*/


	public:

		NewCompressorTree(Target* target,
		                  std::vector<unsigned> inputVectorSizes);

		~NewCompressorTree() {};


		void emulate(TestCase * tc);

};
