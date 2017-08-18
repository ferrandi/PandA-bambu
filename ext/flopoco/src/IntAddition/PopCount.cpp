#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"
#include <gmpxx.h>

#include "PopCount.hpp"

using namespace std;


// personalized parameter
//string PopCount::operatorInfo = "UserDefinedInfo param0 param1 <options>";



PopCount::PopCount(Target * target, int w)
	:Operator(target), w(w)
{
	ostringstream name;
	name << "PopCount_" << w;
	setName(name.str());
	setCopyrightString("Guillaume Sergent 2012");

	int wOut = intlog2(w);

	addInput("X", w);
	addOutput("R", wOut);

	vhdl << "with X select R <= ";
	for (mpz_class i = 0; i < (1 << w); i++) {
		vhdl << "\"" << unsignedBinary(popcnt(i),wOut) << "\" when \""
		     << unsignedBinary(i,w) << "\", ";
	}
	vhdl << "\"" << std::string (wOut, '-') << "\" when others;" << endl;
};


void PopCount::emulate(TestCase * tc)
{
	mpz_class sx = tc->getInputValue("X");
	tc->addExpectedOutput("R", popcnt(sx));
}


