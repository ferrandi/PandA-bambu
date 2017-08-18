#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"

#include "IntPower.hpp"
#include "FormalBinaryProduct.hpp"

//std::string IntPower::operatorInfo = "UserDefinedInfo param0 param1 <options>";

using namespace flopoco;

IntPower::IntPower(Target* target,
                   size_t wIn, size_t n,
		   std::map<std::string,double> inputDelays)
	:GenericBinaryPolynomial(target,
			ProductIR::identity(wIn).toPow(n).simplifyInPlace(),
			inputDelays), wIn(wIn), n(n) {
};

	
void IntPower::emulate(TestCase * tc) {
	mpz_class x = tc->getInputValue ("X"), res(1);
	for (size_t i = 0; i < n; i++) {
		res *= x;
	}
	res &= ((mpz_class(1) << p.data.size()) - 1);
	tc->addExpectedOutput ("R", res);
}

void IntPower::buildStandardTestCases(TestCaseList * tcl) {
}

void IntPower::buildRandomTestCases(TestCaseList *  tcl, int n) {
}

TestCase* IntPower::buildRandomTestCases(int i) {
  TestCase* tc = new TestCase(this);
  return tc;
}
