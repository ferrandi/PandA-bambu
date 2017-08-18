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
#include "PWPolynomial.hh"



bool PWPolynomial::ltInterval::operator()(const tInterval i1, const tInterval i2) const
{
	if (i1.first == i1.second)
		return i1.first < i2.second;
	else if (i2.first == i2.second)
		return i1.second < i2.first;
	else
		return i1.first < i2.first;
}



PWPolynomial::PWPolynomial()
{
	pwP[tInterval(-HUGE_VAL, HUGE_VAL)] = Polynomial();
}

PWPolynomial::PWPolynomial(const PWPolynomial &p)
{
	for (tConstSegment seg = p.pwP.begin(); seg != p.pwP.end(); seg++)
		pwP[seg->first] = seg->second;
}

PWPolynomial::~PWPolynomial()
{
}

double PWPolynomial::eval(double x, int n) const
{
	return find(x)->second.eval(x, n);
}

void PWPolynomial::set(double ia, double ib, const Polynomial &p)
{
	pair<tSegment, tSegment> seg = find(ia, ib);
	pair<tInterval, Polynomial> pa = *(seg.first);
	pair<tInterval, Polynomial> pb = *(seg.second);

	pwP.erase(seg.first, ++seg.second);

	if ((ia == pa.first.first) || (p == pa.second))
		ia = pa.first.first;
	else {
		pa.first.second = ia;
		pwP[pa.first] = pa.second;
	}

	if ((ib == pb.first.second) || (p == pb.second))
		ib = pb.first.second;
	else {
		pb.first.first = ib;
		pwP[pb.first] = pb.second;
	}

	pwP[tInterval(ia, ib)] = p;
}

void PWPolynomial::set(double ia, double ib, double x0)
{
	set(ia, ib, Polynomial(0, x0));
}

void PWPolynomial::set(double ia, double ib, const PWPolynomial &p)
{
	for (tConstSegment seg = p.pwP.begin(); seg != p.pwP.end(); seg++) {
		double ia_ = seg->first.first;
		double ib_ = seg->first.second;

		if (ia_ < ia)
			ia_ = ia;
		if (ib_ > ib)
			ib_ = ib;

		if (ia_ < ib_)
			set(ia_, ib_, seg->second);
	}
}

Polynomial PWPolynomial::get(double x) const
{
	return find(x)->second;
}

PWPolynomial PWPolynomial::operator+(const Polynomial &p2) const
{
	return op(p2, &Polynomial::operator+);
}

PWPolynomial PWPolynomial::operator-(const Polynomial &p2) const
{
	return op(p2, &Polynomial::operator-);
}

PWPolynomial PWPolynomial::operator+(double x0) const
{
	return operator+(Polynomial(0, x0));
}

PWPolynomial PWPolynomial::operator-(double x0) const
{
	return operator-(Polynomial(0, x0));
}

PWPolynomial PWPolynomial::op(const Polynomial &p2, Polynomial (Polynomial::*op)(const Polynomial &) const) const
{
	PWPolynomial p;

	for (tConstSegment seg = pwP.begin(); seg != pwP.end(); seg++) {
		double ia = seg->first.first;
		double ib = seg->first.second;

		if ((ia > -HUGE_VAL) && (ib < HUGE_VAL))
			p.set(ia, ib, (seg->second.*op)(p2));
	}

	return p;
}

PWPolynomial PWPolynomial::operator+(const PWPolynomial &p2) const
{
	return op(p2, &Polynomial::operator+);
}

PWPolynomial PWPolynomial::operator-(const PWPolynomial &p2) const
{
	return op(p2, &Polynomial::operator-);
}

PWPolynomial PWPolynomial::op(const PWPolynomial &p2, Polynomial (Polynomial::*op)(const Polynomial &) const) const
{
	const PWPolynomial &p1 = *this;
	PWPolynomial p;

	tConstSegment seg1 = p1.pwP.begin();
	tConstSegment seg2 = p2.pwP.begin();

	double ia = seg1->first.first;
	double ib;
	while (seg1 != p1.pwP.end()) {
		ib = (seg1->first.second <= seg2->first.second) ? seg1->first.second : seg2->first.second;

		if ((ia > -HUGE_VAL) && (ib < HUGE_VAL))
			p.set(ia, ib, (seg1->second.*op)(seg2->second));

		if (seg1->first.second <= ib)
			seg1++;
		if (seg2->first.second <= ib)
			seg2++;
		ia = ib;
	}

	return p;
}

PWPolynomial PWPolynomial::operator*(double k0) const
{
	PWPolynomial p;

	for (tConstSegment seg = pwP.begin(); seg != pwP.end(); seg++) {
		double ia = seg->first.first;
		double ib = seg->first.second;

		if ((ia > -HUGE_VAL) && (ib < HUGE_VAL))
			p.set(ia, ib, seg->second * k0);
	}
	
	return p;
}

PWPolynomial PWPolynomial::operator>>(double x0) const
{
	PWPolynomial p;

	for (tConstSegment seg = pwP.begin(); seg != pwP.end(); seg++) {
		double ia = seg->first.first;
		double ib = seg->first.second;

		if ((ia > -HUGE_VAL) && (ib < HUGE_VAL))
			p.set(ia-x0, ib-x0, seg->second >> x0);
	}

	return p;
}

PWPolynomial PWPolynomial::operator<<(double x0) const
{
	return operator>>(-x0);
}

PWPolynomial PWPolynomial::operator^(double k0) const
{
	PWPolynomial p;

	for (tConstSegment seg = pwP.begin(); seg != pwP.end(); seg++) {
		double ia = seg->first.first;
		double ib = seg->first.second;

		if ((ia > -HUGE_VAL) && (ib < HUGE_VAL)) {
			double ia_ = (k0 >= 0 ? ia : ib) * k0;
			double ib_ = (k0 >= 0 ? ib : ia) * k0;
			p.set(ia_, ib_, seg->second ^ k0);
		}
	}

	return p;
}

double PWPolynomial::max(double ia, double ib, double delta) const
{
	return max_(ia, ib, delta, true);
}

double PWPolynomial::min(double ia, double ib, double delta) const
{
	return max_(ia, ib, delta, false);
}

double PWPolynomial::max_(double ia, double ib, double delta, bool max) const
{
	double m = max ? -HUGE_VAL : HUGE_VAL;

	pair<tConstSegment, tConstSegment> s = find(ia, ib);
	s.second++;

	for (tConstSegment seg = s.first; seg != s.second; seg++) {
		double ia_ = seg->first.first;
		double ib_ = seg->first.second;

		ia_ = ia_ >= ia ? ia_ : ia;
		ib_ = ib_ <= ib ? ib_ : ib;

		if ((ia_ <= ib_-delta) && (ia_ > -HUGE_VAL) && (ib_ < HUGE_VAL)) {
			ib_ -= delta;
			double m_ = max ? seg->second.max(ia_, ib_) : seg->second.min(ia_, ib_);
			if (max ? (m_ > m) : (m_ < m))
				m = m_;
		}
	}

	return m;
}

PWPolynomial PWPolynomial::max(const PWPolynomial &p1, const PWPolynomial &p2)
{
	return max_(p1, p2, true);
}

PWPolynomial PWPolynomial::min(const PWPolynomial &p1, const PWPolynomial &p2)
{
	return max_(p1, p2, false);
}

PWPolynomial PWPolynomial::max_(const PWPolynomial &p1, const PWPolynomial &p2, bool max)
{
	PWPolynomial p;

	tConstSegment seg1 = p1.pwP.begin();
	tConstSegment seg2 = p2.pwP.begin();

	double ia = seg1->first.first;
	double ib;
	while (seg1 != p1.pwP.end()) {
		ib = (seg1->first.second <= seg2->first.second) ? seg1->first.second : seg2->first.second;

		if ((ia > -HUGE_VAL) && (ib < HUGE_VAL)) {
			Polynomial dp = seg1->second - seg2->second;
			std::set<double> r = dp.solve(ia, ib);
			r.insert(ia);
			r.insert(ib);

			std::set<double>::iterator i = r.begin();
			std::set<double>::iterator j = r.begin();
			for (j++; j != r.end(); i++, j++) {
				double dpm = dp.eval((*i + *j) / 2);
				p.set(*i, *j, (max ? (dpm >= 0) : (dpm <= 0)) ? seg1->second : seg2->second);
			}
		}

		if (seg1->first.second <= ib)
			seg1++;
		if (seg2->first.second <= ib)
			seg2++;
		ia = ib;
	}

	return p;
}

void PWPolynomial::print(ostream &os) const
{
	for (tConstSegment seg = pwP.begin(); seg != pwP.end(); seg++) {
		os << "[ " << seg->first.first << " ; " << seg->first.second << " [ : ";
		seg->second.print(os);
		os << endl;
	}
}

PWPolynomial::tSegment PWPolynomial::find(double x)
{
	return pwP.upper_bound(tInterval(x, x));
}

PWPolynomial::tConstSegment PWPolynomial::find(double x) const
{
	return pwP.upper_bound(tInterval(x, x));
}

pair<PWPolynomial::tSegment, PWPolynomial::tSegment> PWPolynomial::find(double ia, double ib)
{
	return pair<tSegment, tSegment>(pwP.lower_bound(tInterval(ia, ia)),
							  pwP.upper_bound(tInterval(ib, ib)));
}

pair<PWPolynomial::tConstSegment, PWPolynomial::tConstSegment> PWPolynomial::find(double ia, double ib) const
{
	return pair<tConstSegment, tConstSegment>(pwP.lower_bound(tInterval(ia, ia)),
								    pwP.upper_bound(tInterval(ib, ib)));
}



// int main()
// {
//   double k1[] = { 0, 0, 1 };
//   double k2[] = { 2, 0, -1 };
//   PWPolynomial p1, p2;

//   p1.set(-2, 2, Polynomial(2, k1));
//   p2.set(-2, 2, Polynomial(2, k2));

//   PWPolynomial pmx = PWPolynomial::max(p1, p2);
//   PWPolynomial pmn = PWPolynomial::min(p1, p2);

//   p1.print(cout);
//   cout << "---" << endl;
//   p2.print(cout);
//   cout << "---" << endl;
//   pmx.print(cout);
//   cout << "---" << endl;
//   pmn.print(cout);
//   cout << "---" << endl;
//   (pmx+pmn).print(cout);
//   cout << "---" << endl;
//   (pmx-pmn).print(cout);

//   PWPolynomial p;

//   p.print(cout);
//   cout << "---" << endl;
//   p.set(0, 1, Polynomial(0, 1));
//   p.print(cout);
//   cout << "---" << endl;
//   p.set(.25, .5, Polynomial(0, 2));
//   p.print(cout);
//   cout << "---" << endl;
//   p.set(.75, 1, Polynomial(0, 3));
//   p.print(cout);
//   cout << "---" << endl;
//   p.set(.625, .875, Polynomial(0, 4));
//   p.print(cout);
//   cout << "---" << endl;
//   p.set(.125, .9375, Polynomial(0, 5));
//   p.print(cout);
//   cout << "---" << endl;
//   p.set(.25, .75, Polynomial(0, 6));
//   p.print(cout);
//   cout << "---" << endl;
//   p.set(.25, .75, Polynomial(0, 6));
//   p.print(cout);
//   cout << "---" << endl;
//   p.set(.25, .5, Polynomial(0, 5));
//   p.print(cout);
//   cout << p.eval(0.75) << " " << p.eval(0.9) << endl;

//   return 0;
// }
