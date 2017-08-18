#ifndef _TERMPOWMULT_HH_
#define _TERMPOWMULT_HH_

#include "Term.hh"
#include "Power.hh"
#include "Polynomial.hh"
#include "Util.hh"



class TermPowMult : public Term {
public:
	TermPowMult(int d_, double *k, Param &p_);
	~TermPowMult();

	void roundTables(int g, bool full = true, double *kAdjust = NULL);

	double estimArea();
	double estimDelay();

	double eval(long long int a, long long int b) const;
	long long int evalRound(long long int a, long long int b) const;

	friend Component::Component (flopoco::Target*, TermPowMult, std::string);
	flopoco::Operator* toComponent(flopoco::Target* t, std::string name) {
		return new Component (t, *this, name);
	}

protected:
	PWPolynomial calcErrTab(int k_ = 0);
	PWPolynomial calcErrPow(int k_ = 0);

	TermPowMultParam &tp;
	double **kList;
	Power *pow;

	long long int **table;
	int *wTable;
	bool signTable;
};

#endif // _TERMPOWMULT_HH_
