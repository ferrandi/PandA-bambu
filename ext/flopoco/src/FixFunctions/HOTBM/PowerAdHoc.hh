#ifndef _POWERADHOC_HH_
#define _POWERADHOC_HH_

#include "Power.hh"
#include "Util.hh"



class PowerAdHoc : public Power {
public:
	PowerAdHoc(int d_, Param &p_);
	~PowerAdHoc();

	double estimArea();
	double estimDelay();

	void mpEval(mpz_t mpR, long long int b) const;

	friend Component::Component (flopoco::Target*, PowerAdHoc, std::string);
	flopoco::Operator* toComponent(flopoco::Target* t, std::string name) {
		return new Component (t, *this, name);
	}

protected:
	PWPolynomial calcErrTab(double shift = 0);

	typedef set<int> tPPElement;
	typedef set<tPPElement> tPPLine;
	typedef map<int, tPPLine> tPPArray;

	tPPArray ppAdd(const tPPArray &ppa1, const tPPArray &ppa2);
	tPPArray ppShift(const tPPArray &ppa, int n);
	tPPArray ppMul(const tPPArray &ppa, const tPPElement &pp);
	tPPArray ppMul(const tPPArray &ppa1, const tPPArray &ppa2);
	tPPArray ppPow(const tPPArray &ppa, int d);
	void ppPrint(flopoco::FlopocoStream& vhdl, const tPPArray &ppa);

	PowerAdHocParam &pp;
	tPPArray ppa;
	int nPPLine;
};

#endif // _POWERADHOC_HH_
