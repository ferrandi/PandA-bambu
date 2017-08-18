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
#include "TermROM.hh"

#include <iostream>

TermROM::TermROM(int d_, double *k_, Param &p_)
	: Term(d_, k_, p_), tp(*(TermROMParam *)p.t[d]), pList(new Polynomial[P(tp.alpha)]), table(NULL)
{
	int n = P(p.alpha-tp.alpha);
	for (long long int i = 0; i < P(tp.alpha); i++) {
		Polynomial p_(d, (k[i*n] + k[(i+1)*n-1]) * .5);
		pList[i] = (p_ + (p_ >> (E(-p.alpha-tp.beta)-E(-p.wI)))) * .5;
	}

	PWPolynomial errP[2];
	errP[0] = calcErrTab(0);
	errP[1] = calcErrTab(E(-p.alpha-tp.beta) - E(-p.wI));

	errMethod[0] = PWPolynomial::max(errP[0], errP[1]);
	errMethod[1] = PWPolynomial::min(errP[0], errP[1]);
}

TermROM::~TermROM()
{
	delete[] pList;
	if (table)
		delete[] table;
}

void TermROM::roundTables(int g, bool full, double *kAdjust)
{
	p.g = g;
	
	int beta_ = tp.beta ? tp.beta-1 : 0;

	if (full) {
		if (table)
			delete[] table;
		table = new long long int[P(tp.alpha+beta_)];
	}

	long long int tMin = P(60), tMax = -P(60);
	for (long long int a = 0; a < P(tp.alpha); a += full ? 1 : MAX(P(tp.alpha)-1,1)) {
		Polynomial p_ = pList[a] + (!d && kAdjust ? kAdjust[a] : 0);
		for (long long int b = 0; b < P(beta_); b += full ? 1 : MAX(P(beta_)-1,1)) {
			long long int r;
			if (tp.beta)
				r = (long long int)floor(p_.eval(b * E(-p.alpha-tp.beta) + E(-p.wI-1)) * P(p.wO+p.g));
			else
				r = (long long int)floor(p_.eval(-E(-p.alpha-1) + E(-p.wI-1)) * P(p.wO+p.g));
			if (full)
				table[a*P(beta_)+b] = r;
			if (r < tMin)
				tMin = r;
			if (r > tMax)
				tMax = r;
		}
	}

	if(tMin < 0) {
		if(tMax >= 0) {
			// Both positive and negative values: use 2's-complement encoding
			signTable = SignMixed;
		}
		else {
			signTable = SignNegative;
		}
	}
	else {
		signTable = SignPositive;
	}
	
	if(signTable == SignMixed) {
		// 2's-complement
		tMax = std::max(tMax, -tMin-1);
		wTable = !tMax ? 1 : (int)floor(log2(tMax))+2;
	}
	else {
		// 2's-complement with implicit sign bit
		if(signTable == SignNegative)
			tMax = -tMin-1;
		wTable = !tMax ? 1 : (int)floor(log2(tMax))+1;
	}
}

double TermROM::estimArea()
{
	return Estim::xorArea(tp.beta-1)
			 + Estim::romArea(tp.alpha + (tp.beta?tp.beta-1:0), wTable)
			 + Estim::xorArea(wTable);
}

double TermROM::estimDelay()
{
	return Estim::xorDelay(tp.beta-1)
			 + Estim::romDelay(tp.alpha + (tp.beta?tp.beta-1:0), wTable)
			 + Estim::xorDelay(wTable);
}

double TermROM::eval(long long int a, long long int b) const
{
	if (tp.beta) {
		bool sign = !I(b,tp.beta,1);
		b = I(b,tp.beta-1,tp.beta-1) ^ (sign * (P(tp.beta-1)-1));
		double r = pList[a].eval(b * E(-p.alpha-tp.beta) + E(-p.wI-1));
		return (!(d % 2) || !sign) ? r : -r;
	}
	else
		return pList[a].eval(-E(-p.alpha-1) + E(-p.wI-1));
}

long long int TermROM::evalRound(long long int a, long long int b) const
{
	if (tp.beta) {
		bool sign = !I(b,tp.beta,1);
		b = I(b,tp.beta-1,tp.beta-1) ^ (sign * (P(tp.beta-1)-1));
		long long int r = table[a*P(tp.beta-1)+b];
		return (!(d % 2) || !sign) ? r : -r-1;
	}
	else
		return table[a];
}

PWPolynomial TermROM::calcErrTab(double shift)
{
	PWPolynomial errP;

	for (long long int i = 0; i < P(p.alpha); i++) {
		long long int a = I(i,p.alpha,tp.alpha);
		Polynomial p_ = Polynomial(d, k[i]);
		double ia =  i    * E(-p.alpha);
		double ib = (i+1) * E(-p.alpha);
		double ib_ = ib - E(-p.wI);
		double delta = E(-p.alpha-tp.beta);

		if (shift > 0)
			errP.set(ia, ia+shift, (p_ - pList[a].eval(-E(-p.alpha-1) + E(-p.wI-1))) << ((ia+ib_)/2));

		if (shift < delta-E(-p.wI))
			errP.set(ib+shift-(delta-E(-p.wI)), ib, (p_ - pList[a].eval(E(-p.alpha-1)-delta + E(-p.wI-1))) << ((ia+ib_)/2));

		errP.set(ia+shift, ib+shift-(delta-E(-p.wI)), (p_ - (pList[a] << shift)) << ((ia+ib_)/2));
	}

	return errP;
}

Component::Component (flopoco::Target* t, TermROM tr, std::string name)
	:Operator (t)
{
	TermROMParam& tp = tr.tp;
	int d = tr.d;
	int wTable = tr.wTable;
	TermROM::TableSign& signTable = tr.signTable;
	long long int* table = tr.table;
	setName (name);
	Param& p = tr.p;
	using namespace flopoco;

	int beta_ = tp.beta ? tp.beta-1 : 0;

	vhdl << "--------------------------------------------------------------------------------" << endl;
	vhdl << "-- TermROM instance for order-" << d << " term." << endl;
	vhdl << "-- Decomposition:" << endl;
	vhdl << "--   alpha_" << d << " = " << tp.alpha << "; ";
	if (tp.beta)
		vhdl << "beta_" << d << " = " << tp.beta << " (1+" << (tp.beta-1) << ")" << "; ";
	else
		vhdl << "beta_" << d << " = 0; ";
	vhdl << "wO_" << d << " = " << wTable << "; " << "wO = " << p.wO << "; " << "g = " << p.g << "." << endl;
	vhdl << endl;

	if (tp.alpha)
		addInput ("a", tp.alpha);
	if (tp.beta)
		addInput ("b", tp.beta);
	addOutput ("r", p.wO+p.g+1);

	if (tp.beta)
		declare ("sign");
	if (beta_)
		declare ("b0", beta_);
	if (beta_ || tp.alpha)
		declare ("x0", tp.alpha + beta_);
	declare ("r0", wTable);

	if (tp.beta)
		vhdl << "  sign <= not b" << of(tp.beta-1) << ";" << endl;
	if (beta_) {
		vhdl << "  b0 <= b" << range(beta_-1, 0) << " xor " << rangeAssign(beta_-1, 0, "sign") << ";" << endl;
		if (tp.alpha)
			vhdl << "  x0 <= a & b0;" << endl;
		else
			vhdl << "  x0 <= b0;" << endl;
	}
	else if (tp.alpha)
		vhdl << "  x0 <= a;" << endl;
	if (tp.beta || tp.alpha)
		vhdl << endl;

	if (signTable == TermROM::SignMixed)
		vhdl << "  -- Table in 2's-complement" << endl;
	if (beta_ || tp.alpha)
		VHDLGen::genROM(vhdl, table, tp.alpha+beta_, wTable, "x0", "r0");
	else {
		vhdl << "  r0 <= ";
		VHDLGen::genInteger(vhdl, table[0], wTable);
		vhdl << "; -- t = " << table[0] << endl;
	}
	vhdl << endl;

	if ((d%2) && tp.beta){
		vhdl << "  r" << range(wTable-1, 0) << " <= r0 xor " << rangeAssign(wTable-1, 0, "sign") << ";" << endl;
		// Do NOT negate sign when signTable==SignNegative here
	}
	else
		vhdl << "  r" << range(wTable-1, 0) << " <= r0;" << endl;

	if (p.wO+p.g+1 > wTable) {
		vhdl << "  -- Sign extension" << endl;
		vhdl << "  r(" << (p.wO+p.g) << " downto " << wTable << ") <= (" << (p.wO+p.g) << " downto " << wTable << " => ("
			 << (signTable == TermROM::SignNegative ? "not " : "");
			 
		if(signTable == TermROM::SignMixed) {
			if ((d%2) && tp.beta)
				vhdl << "sign xor ";
			vhdl << "r0(" << (wTable-1) << ")";	// cannot use r as an input
		}
		else {
			vhdl << (((d%2) && tp.beta) ? "sign" : "'0'");
		}
		vhdl << "));" << endl;
	}
}
