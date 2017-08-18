#ifndef _POWER_HH_
#define _POWER_HH_

#include "Param.hh"
#include "PWPolynomial.hh"
#include "Util.hh"
#include "Operator.hpp"
#include "Component.hh"

using namespace std;



class Power {
public:
	Power(int d_, Param &p_);
	virtual ~Power();

	PWPolynomial *getErrPow();

	virtual double estimArea() = 0;
	virtual double estimDelay() = 0;

	virtual void mpEval(mpz_t mpR, long long int b) const = 0;

	virtual flopoco::Operator* toComponent(flopoco::Target* t, std::string name) = 0;

protected:
	int d;
	Param &p;

	PWPolynomial *errPow;
};

#endif // _POWER_HH_
