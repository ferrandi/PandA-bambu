#ifndef _PWPOLYNOMIAL_HH_
#define _PWPOLYNOMIAL_HH_

#include <map>

#include "Polynomial.hh"
#include "Util.hh"

using namespace std;



class PWPolynomial {
public:
	PWPolynomial();
	PWPolynomial(const PWPolynomial &p);
	~PWPolynomial();

	double eval(double x, int n = 0) const;

	void set(double ia, double ib, const Polynomial &p);
	void set(double ia, double ib, double x0);
	void set(double ia, double ib, const PWPolynomial &p);

	Polynomial get(double x) const;

	PWPolynomial operator+(const Polynomial &p2) const;
	PWPolynomial operator-(const Polynomial &p2) const;

	PWPolynomial operator+(double x0) const;
	PWPolynomial operator-(double x0) const;

	PWPolynomial operator+(const PWPolynomial &p2) const;
	PWPolynomial operator-(const PWPolynomial &p2) const;

	PWPolynomial operator*(double k0) const;
	PWPolynomial operator>>(double x0) const;
	PWPolynomial operator<<(double x0) const;
	PWPolynomial operator^(double k0) const;

	double max(double ia = -HUGE_VAL, double ib = HUGE_VAL, double delta = 0) const;
	double min(double ia = -HUGE_VAL, double ib = HUGE_VAL, double delta = 0) const;

	static PWPolynomial max(const PWPolynomial &p1, const PWPolynomial &p2);
	static PWPolynomial min(const PWPolynomial &p1, const PWPolynomial &p2);

	void print(ostream &os) const;

private:
	typedef pair<double, double> tInterval;
	struct ltInterval {
		bool operator()(const tInterval i1, const tInterval i2) const;
	};
	typedef map<tInterval, Polynomial, ltInterval> tPWPoly;
	typedef tPWPoly::iterator tSegment;
	typedef tPWPoly::const_iterator tConstSegment;

	PWPolynomial op(const Polynomial &p2, Polynomial (Polynomial::*op)(const Polynomial &) const) const;
	PWPolynomial op(const PWPolynomial &p2, Polynomial (Polynomial::*op)(const Polynomial &) const) const;
	double max_(double ia, double ib, double delta, bool max = true) const;
	static PWPolynomial max_(const PWPolynomial &p1, const PWPolynomial &p2, bool max = true);

	tSegment find(double x);
	tConstSegment find(double x) const;
	pair<tSegment, tSegment> find(double ia, double ib);
	pair<tConstSegment, tConstSegment> find(double ia, double ib) const;

	tPWPoly pwP;
};

#endif // _PWPOLYNOMIAL_HH_
