#ifndef _POLYNOMIAL_HH_
#define _POLYNOMIAL_HH_

#include <set>

#include "Util.hh"

using namespace std;



class Polynomial {
public:
	Polynomial();
	Polynomial(int d_, double *k_);
	Polynomial(int d_, double k_);
	Polynomial(const Polynomial &p);
	~Polynomial();

	int getD() const;
	double getK(int i) const;

	double eval(double x, int n = 0) const;

	Polynomial operator=(const Polynomial &p);
	bool operator==(const Polynomial &p) const;
	Polynomial operator+(const Polynomial &p2) const;
	Polynomial operator-(const Polynomial &p2) const;
	Polynomial operator+(double x0) const;
	Polynomial operator-(double x0) const;
	Polynomial operator*(double k0) const;
	Polynomial operator>>(double x0) const;
	Polynomial operator<<(double x0) const;
	Polynomial operator^(double k0) const;

	set<double> solve(double ia, double ib, int n = 0) const;
	double max(double ia, double ib) const;
	double min(double ia, double ib) const;

	void print(ostream &os) const;

private:
	double max_(double ia, double ib, bool max = true) const;

	int d;
	double *k;
};

#endif // _POLYNOMIAL_HH_
