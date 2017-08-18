#ifndef _POWERROM_HH_
#define _POWERROM_HH_

#include "Power.hh"
#include "Util.hh"



class PowerROM : public Power {
public:
	PowerROM(int d_, Param &p_);
	~PowerROM();

	double estimArea();
	double estimDelay();

	void mpEval(mpz_t mpR, long long int b) const;

	friend Component::Component (flopoco::Target*, PowerROM, std::string);
	flopoco::Operator* toComponent(flopoco::Target* t, std::string name) {
		return new Component (t, *this, name);
	}

protected:
	PWPolynomial calcErrTab(double shift = 0);

	PowerROMParam &pp;
};

#endif // _POWERROM_HH_
