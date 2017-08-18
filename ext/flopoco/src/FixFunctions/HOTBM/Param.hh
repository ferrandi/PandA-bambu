#ifndef _PARAM_HH_
#define _PARAM_HH_

#include "Util.hh"

using namespace std;

#define POWER_TYPE_ROM   1
#define POWER_TYPE_ADHOC 2

#define TERM_TYPE_ROM     1
#define TERM_TYPE_POWMULT 2



class PowerParam {
public:
	PowerParam(int type_, int beta_, int lambda_);
	virtual ~PowerParam() {};

	virtual PowerParam *clone() = 0;
	virtual void print(ostream &os) = 0;

	int type;
	int beta, lambda;
};

class PowerROMParam : public PowerParam {
public:
	PowerROMParam(int beta_, int lambda_);

	virtual PowerROMParam *clone();
	virtual void print(ostream &os);
};

class PowerAdHocParam : public PowerParam {
public:
	PowerAdHocParam(int beta_, int mu_, int lambda_);

	virtual PowerAdHocParam *clone();
	virtual void print(ostream &os);

	int mu;
};



class TermParam {
public:
	TermParam(int type_, int alpha_, int beta_);
	virtual ~TermParam() {};

	virtual TermParam *clone() = 0;
	virtual void print(ostream &os) = 0;

	int type;
	int alpha, beta;
};

class TermROMParam : public TermParam {
public:
	TermROMParam(int alpha_, int beta_);

	virtual TermROMParam *clone();
	virtual void print(ostream &os);
};

class TermPowMultParam : public TermParam {
public:
	TermPowMultParam(int alpha_, PowerParam *p_, int mM_, int mT_, int *alphas_, int *sigmas_);
	virtual ~TermPowMultParam();

	virtual TermPowMultParam *clone();
	virtual void print(ostream &os);

	PowerParam *p;
	int mM, mT;
	int *alphas, *rhos, *sigmas;
};



class Param {
public:
	Param(int wI_, int wO_, int n_);
	Param(const Param &p);
	~Param();

	void print(ostream &os);

	int wI, wO, n;
	int alpha, beta;
	TermParam **t;
	int g;
};

#endif // _PARAM_HH_
