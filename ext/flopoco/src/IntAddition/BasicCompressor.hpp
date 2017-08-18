#ifndef BasicCompressor_HPP
#define BasicCompressor_HPP

#include "../Operator.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include "gmp.h"
#include "mpfr.h"
#include <vector>
#include <gmpxx.h>
#include <stdio.h>
#include <stdlib.h>
#include "../utils.hpp"

namespace flopoco
{

	/** The BasicCompressor class generates basic patterns for bit compressions
	 */


	class BasicCompressor:public Operator
	{
	public:
		vector<int> height; /** inverted input vector h **/
		int wOut; /** size of the output vector **/
		int param; /** computes the range of the output vector **/
		
		
		/** constructor **/	
		BasicCompressor(Target * target, vector<int> h);
	
	
		/** destructor**/
		~BasicCompressor();

		unsigned getColumnSize(int column);

		int getOutputSize();
	
	
		/** test case generator  **/
		void emulate(TestCase * tc);
	};
}
 
#endif
