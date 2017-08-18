#ifndef FunctionTable_HPP
#define FunctionTable_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>

#include "../Table.hpp"
#include "Function.hpp"

namespace flopoco{

	/** The FunctionTable class */
	class FunctionTable : public Table
	{
	public:
		/**
			 The FunctionTable constructor
			 @param[string] func    a string representing the function, input range should be [0,1]
			 @param[int]    wInX    input size, also opposite of input LSB weight
			 @param[int]    lsbOut  output LSB weight
			 @param[int]    msbOut  output MSB weight, used to determine wOut
		 */
		FunctionTable(Target* target, string func, int wInX, int lsbOut, int msbOut, int logicTable=1, map<string, double> inputDelays = emptyDelayMap);

		/**
		 * FunctionTable destructor
		 */
		~FunctionTable();
	
		mpz_class function(int x); // overloading Table method
		void emulate(TestCase * tc);


	protected:
		
		Function *f;
		unsigned wR;
		int wInX_;   
		int lsbOut_;
		int msbOut_;
		
	};

}

#endif
