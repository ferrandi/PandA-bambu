/*
  The High Order Table Based Method for hardware function evaluation

  Author: Jérémie Detrey, FloPoCoized by Christian Klein and F. de Dinechin

  (developed for the polynomial-based square root)

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2005-2011  

  All rights reserved.

*/
#include "Polynomial.hh"



Polynomial::Polynomial()
	: d(0), k(new double[1])
{
	k[0] = 0.0;
}

Polynomial::Polynomial(int d_, double *k_)
	: d(d_), k(new double[d+1])
{
	for (int i = 0; i <= d; i++)
		k[i] = k_[i];
}

Polynomial::Polynomial(int d_, double k_)
	: d(d_), k(new double[d+1])
{
	for (int i = 0; i <= d; i++)
		k[i] = (i < d) ? 0 : k_;
}

Polynomial::Polynomial(const Polynomial &p)
	: d(p.d), k(new double[d+1])
{
	for (int i = 0; i <= d; i++)
		k[i] = p.k[i];
}

Polynomial::~Polynomial()
{
	delete[] k;
}

int Polynomial::getD() const
{
	return d;
}

double Polynomial::getK(int i) const
{
	return k[i];
}

double Polynomial::eval(double x, int n) const
{
	double r = 0;

	for (int i = d; i >= n; i--) {
		r *= x;
		double m = 1;
		for (int j = 0; j < n; j++)
			m *= i-j;
		r += m * k[i];
	}

	return r;
}

Polynomial Polynomial::operator=(const Polynomial &p)
{
	if (this != &p) {
		delete[] k;
		d = p.d;
		k = new double[d+1];
		for (int i = 0; i <= d; i++)
			k[i] = p.k[i];
	}

	return *this;
}

bool Polynomial::operator==(const Polynomial &p) const
{
	if (d != p.d)
		return false;
	for (int i = 0; i <= d; i++)
		if (k[i] != p.k[i])
			return false;
	return true;
}

Polynomial Polynomial::operator+(const Polynomial &p2) const
{
	const Polynomial &p1 = *this;
	Polynomial p((p1.d > p2.d) ? p1.d : p2.d, 0.);

	for (int i = 0; i <= p.d; i++)
		p.k[i] = ((i <= p1.d) ? p1.k[i] : 0) + ((i <= p2.d) ? p2.k[i] : 0);

	return p;
}

Polynomial Polynomial::operator-(const Polynomial &p2) const
{
	const Polynomial &p1 = *this;
	Polynomial p((p1.d > p2.d) ? p1.d : p2.d, 0.);

	for (int i = 0; i <= p.d; i++)
		p.k[i] = ((i <= p1.d) ? p1.k[i] : 0) - ((i <= p2.d) ? p2.k[i] : 0);

	return p;
}

Polynomial Polynomial::operator+(double x0) const
{
	Polynomial p(d, k);
	p.k[0] += x0;
	return p;
}

Polynomial Polynomial::operator-(double x0) const
{
	Polynomial p(d, k);
	p.k[0] -= x0;
	return p;
}

Polynomial Polynomial::operator*(double k0) const
{
	Polynomial p(d, 0.);

	for (int i = 0; i <= d; i++)
		p.k[i] = k[i] * k0;

	return p;
}

Polynomial Polynomial::operator>>(double x0) const
{
	Polynomial p(d, 0.);

	for (int i = 0; i <= d; i++) {
		p.k[i] = 0;
		for (int j = i; j <= d; j++)
			p.k[i] += k[j] * bin(j, i) * pow(x0, j-i);
	}

	return p;
}

Polynomial Polynomial::operator<<(double x0) const
{
	return operator>>(-x0);
}

Polynomial Polynomial::operator^(double k0) const
{
	Polynomial p(d, 0.);

	double kpow = 1.;
	for (int i = 0; i <= d; i++, kpow *= k0)
		p.k[i] = k[i] / kpow;

	return p;
}

set<double> Polynomial::solve(double ia, double ib, int n) const
{
	set<double> r;

	if (n < d) {
		set<double> dr = solve(ia, ib, n+1);
		dr.insert(ia);
		dr.insert(ib);

		set<double>::iterator i = dr.begin();
		set<double>::iterator j = dr.begin();

		for (j++; j != dr.end(); i++, j++) {
			double a = *i;
			double b = *j;

			double pa = eval(a, n);
			double pb = eval(b, n);

			if (pa * pb >= 0)
				continue;

			double m = (a+b)/2;
			while ((m != a) && (m != b)) {
				double pm = eval(m, n);
				if (pa * pm <= 0) {
					b = m;
					pb = pm;
				}
				else {
					a = m;
					pa = pm;
				}
				m = (a+b)/2;
			}
			r.insert(m);
		}
	}

	return r;
}

double Polynomial::max(double ia, double ib) const
{
	return max_(ia, ib, true);
}

double Polynomial::min(double ia, double ib) const
{
	return max_(ia, ib, false);
}

double Polynomial::max_(double ia, double ib, bool max) const
{
	double m = max ? -HUGE_VAL : HUGE_VAL;

	set<double> r = solve(ia, ib, 1);
	r.insert(ia);
	r.insert(ib);

	for (set<double>::iterator i = r.begin(); i != r.end(); i++) {
		double m_ = eval(*i);
		if (max ? (m_ > m) : (m_ < m))
			m = m_;
	}

	return m;
}

void Polynomial::print(ostream &os) const
{
	bool first = true;

	for (int i = 0; i <= d; i++) {
		if (k[i] == 0)
			continue;
		if (!first)
			os << "  " << (k[i] > 0 ? '+' : '-') << "  ";
		os << (first ? k[i] : fabs(k[i]));
		if (i)
			os << " * x";
		if (i > 1)
			os << "^" << i;
		first = false;
	}

	if (first)
		os << 0;
}



// int main()
// {
//   double k[] = { 0, 24, -50, 35, -10, 1 };
//   Polynomial p(5, k);

//   set<double> r = p.solve(-10, 10);

//   cerr.precision(40);
//   for (set<double>::iterator i = r.begin(); i != r.end(); i++)
//     cerr << *i << endl;

//   return 0;
// }
