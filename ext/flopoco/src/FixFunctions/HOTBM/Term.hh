#ifndef _TERM_HH_
#define _TERM_HH_

#include "../Function.hpp"
#include "Param.hh"
#include "PWPolynomial.hh"
#include "Util.hh"
#include "Operator.hpp"
#include "Component.hh"

using namespace std;



class Term {
public:
	Term(int d_, double *k_, Param &p_);
	virtual ~Term();

	PWPolynomial *getErrMethod();

	virtual void roundTables(int g, bool full = true, double *kAdjust = NULL) = 0;

	virtual double estimArea() = 0;
	virtual double estimDelay() = 0;

	virtual double eval(long long int a, long long int b) const = 0;
	virtual long long int evalRound(long long int a, long long int b) const = 0;

	virtual flopoco::Operator* toComponent(flopoco::Target* t, std::string name) = 0;

protected:
	int d;
	double *k;
	Param p;

	PWPolynomial *errMethod;

public:
	int refCount;
};

#endif // _TERM_HH_
