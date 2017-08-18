#ifndef _PIECEWISEFUNCTION_HH_
#define _PIECEWISEFUNCTION_HH_

#include <string>
#include <iostream>
#include <vector>
#include "HOTBM/Util.hh"
#include "Function.hpp"
#include <stdlib.h>
using namespace std;


namespace flopoco{

	class PiecewiseFunction {
	public:
		PiecewiseFunction(string name_);
		virtual ~PiecewiseFunction();

		string getName() const;
		vector<Function*>getPiecewiseFunctionArray() const;
    Function* getPiecewiseFunctionArray(int i) const;
	private:
		string name;
		vector<Function*> fArray;
	}
		;
}
#endif // _PIECEWISEFUNCTION_HH_
