#ifndef _MPPOLYNOMIAL_HH_
#define _MPPOLYNOMIAL_HH_

#include "Util.hh"

using namespace std;



class MPPolynomial {
public:
	MPPolynomial();
	MPPolynomial(int d_, mpfr_t *mpK_);
	MPPolynomial(const MPPolynomial &mpP);
	~MPPolynomial();

	int getD() const;
	void getMPK(mpfr_t mpK_, int i) const;

	MPPolynomial operator=(const MPPolynomial &mpP);
	MPPolynomial operator>>(double x0) const;
	MPPolynomial operator<<(double x0) const;

	void eval(mpfr_t mpR, mpfr_t mpX, int n = 0) const;

private:
	int d;
	mpfr_t *mpK;
};

#endif // _MPPOLYNOMIAL_HH_
