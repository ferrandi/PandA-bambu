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
#include <sstream>

#include "PowerROM.hh"
#include "PowerAdHoc.hh"
#include "TermPowMult.hh"
#include "Operator.hpp"



TermPowMult::TermPowMult(int d_, double *k_, Param &p_)
	: Term(d_, k_, p_), tp(*(TermPowMultParam *)p.t[d]), kList(new double *[tp.mM+tp.mT]),
		table(NULL), wTable(new int[tp.mM+tp.mT])

{
	for (int i = 0; i < tp.mM+tp.mT; i++) {
		kList[i] = new double[P(tp.alphas[i])];
		int n = P(p.alpha-tp.alphas[i]);
		for (long long int j = 0; j < P(tp.alphas[i]); j++)
			kList[i][j] = (k[j*n] + k[(j+1)*n-1]) * .5;
	}

	if (tp.p->type == POWER_TYPE_ROM)
		pow = new PowerROM(d, p);
	else if (tp.p->type == POWER_TYPE_ADHOC)
		pow = new PowerAdHoc(d, p);

	PWPolynomial errTab[2], errPow[2];
	for (int i = 0; i < 2; i++) {
		errTab[i] = calcErrTab(i);
		errPow[i] = calcErrPow(i);
	}

	errMethod[0] = PWPolynomial::max(errTab[0], errTab[1]) + PWPolynomial::max(errPow[0], errPow[1]);
	errMethod[1] = PWPolynomial::min(errTab[0], errTab[1]) + PWPolynomial::min(errPow[0], errPow[1]);
}

TermPowMult::~TermPowMult()
{
	delete pow;
	for (int i = 0; i < tp.mM+tp.mT; i++)
		delete[] kList[i];
	delete[] kList;

	if (table) {
		for (int i = 0; i < tp.mM+tp.mT; i++)
			delete[] table[i];
		delete[] table;
	}
	delete[] wTable;
}

void TermPowMult::roundTables(int g, bool full, double *kAdjust)
{
	p.g = g;

	if (full) { 
		if (table) {
			for (int i = 0; i < tp.mM+tp.mT; i++)
				delete[] table[i];
			delete[] table;
		}
		table = new long long int *[tp.mM+tp.mT];
	}

	for (int i = 0; i < tp.mM+tp.mT; i++) {
		if (i < tp.mM) {
			long long int tMin = P(60), tMax = -P(60);
			if (full)
				table[i] = new long long int[P(tp.alphas[i])];
			for (long long int a = 0; a < P(tp.alphas[i]); a += full ? 1 : MAX(P(tp.alphas[i])-1,1)) {
				long long int r = (long long int)round(kList[i][a] * E(p.wO+p.g-d*(p.alpha+1)-tp.rhos[i]));
				if (full)
					table[i][a] = r < 0 ? -r : r;
				if (r < tMin)
					tMin = r;
				if (r > tMax)
					tMax = r;
			}

			if ((tMin < 0) && (tMax > 0))
				throw "TermPowMult::roundTables: Non-constant sign.";
			if (!i)
				signTable = tMin < 0;
			tMax = signTable ? -tMin : tMax;
			wTable[i] = !tMax ? 1 : (int)floor(log2(tMax))+1;
		}
		else {
			long long int tMin = P(60), tMax = -P(60);
			if (full)
				table[i] = new long long int[P(tp.alphas[i]+tp.sigmas[i]-1)];
			for (long long int a = 0; a < P(tp.alphas[i]); a += full ? 1 : MAX(P(tp.alphas[i])-1,1)) {
				for (long long int s = 0; s < P(tp.sigmas[i]-1); s += full ? 1 : MAX(P(tp.sigmas[i]-1)-1,1)) {
					long long int r = (long long int)floor(kList[i][a] * (s+.5) *
									 E(p.wO+p.g-d*(p.alpha+1)+1-tp.rhos[i]-tp.sigmas[i]));
					if (full)
						table[i][a*P(tp.sigmas[i]-1)+s] = r < 0 ? -r-1 : r;
					if (r < tMin)
						tMin = r;
					if (r > tMax)
						tMax = r;
				}
			}

			if ((tMin < 0) && (tMax >= 0))
				throw "TermPowMult::roundTables: Non-constant sign.";
			if (!i)
				signTable = tMin < 0;
			tMax = signTable ? -tMin-1 : tMax;
			wTable[i] = !tMax ? 1 : (int)floor(log2(tMax))+1;
		}
	}
}

double TermPowMult::estimArea()
{
	double area = Estim::xorArea(tp.beta-1);

	area += pow->estimArea();

	for (int i = 0; i < tp.mM+tp.mT; i++) {
		area += Estim::xorArea(tp.sigmas[i]-1);
		if (i < tp.mM)
			area += Estim::romArea(tp.alphas[i], wTable[i]) + Estim::multiplierArea(tp.sigmas[i], wTable[i]);
		else
			area += Estim::romArea(tp.alphas[i]+tp.sigmas[i]-1, wTable[i]);
		area += Estim::xorArea(wTable[i]);
	}

	area += Estim::adderArea(p.wO+p.g+1, tp.mM+tp.mT);

	return area;
}

double TermPowMult::estimDelay()
{
	double delay = 0;

	for (int i = 0; i < tp.mM+tp.mT; i++) {
		double delay_ = Estim::xorDelay(tp.sigmas[i]-1);
		if (i < tp.mM)
			delay_ += Estim::romDelay(tp.alphas[i], wTable[i]) + Estim::multiplierDelay(tp.sigmas[i], wTable[i]);
		else
			delay_ += Estim::romArea(tp.alphas[i]+tp.sigmas[i]-1, wTable[i]);
		delay_ += Estim::xorDelay(wTable[i]);
		if (delay_ > delay)
			delay = delay_;
	}

	delay += Estim::xorDelay(tp.beta-1) + pow->estimDelay() + Estim::adderDelay(p.wO+p.g+1, tp.mM+tp.mT);

	return delay;
}

double TermPowMult::eval(long long int a, long long int b) const
{
	bool sign = !I(b,tp.beta,1);

	b = I(b,tp.beta-1,tp.beta-1) ^ (sign * (P(tp.beta-1)-1));

	mpz_t mpS, mpTmp;
	mpz_init(mpS);
	mpz_init(mpTmp);

	pow->mpEval(mpS, b);

	double r = 0;

	for (int i = 0; i < tp.mM+tp.mT; i++) {
		long long int ai = I(a,tp.alpha,tp.alphas[i]);

		mpz_set(mpTmp, mpS);
		mpz_fdiv_q_2exp(mpTmp, mpTmp, tp.p->lambda-tp.rhos[i]-tp.sigmas[i]);
		mpz_fdiv_r_2exp(mpTmp, mpTmp, tp.sigmas[i]);

		long long int s = mpz_get_lli(mpTmp);
		bool signS = !I(s,tp.sigmas[i],1);
		s = I(s,tp.sigmas[i]-1,tp.sigmas[i]-1) ^ (signS * (P(tp.sigmas[i]-1)-1));

		double r_ = kList[i][ai] * (s+.5) * E(-d*(p.alpha+1)+1-tp.rhos[i]-tp.sigmas[i]);
		r_ = !signS ? r_ : -r_;
		r += (!(d % 2) || !sign) ? r_ : -r_; 
	}

	mpz_clear(mpS);
	mpz_clear(mpTmp);

	return r;
}

long long int TermPowMult::evalRound(long long int a, long long int b) const
{
	bool sign = !I(b,tp.beta,1);

	b = I(b,tp.beta-1,tp.beta-1) ^ (sign * (P(tp.beta-1)-1));

	mpz_t mpS, mpTmp;
	mpz_init(mpS);
	mpz_init(mpTmp);

	pow->mpEval(mpS, b);

	long long int r = 0;

	for (int i = 0; i < tp.mM+tp.mT; i++) {
		long long int ai = I(a,tp.alpha,tp.alphas[i]);

		mpz_set(mpTmp, mpS);
		mpz_fdiv_q_2exp(mpTmp, mpTmp, tp.p->lambda-tp.rhos[i]-tp.sigmas[i]);
		mpz_fdiv_r_2exp(mpTmp, mpTmp, tp.sigmas[i]);

		long long int s = mpz_get_lli(mpTmp);
		bool signS = !I(s,tp.sigmas[i],1);
		s = I(s,tp.sigmas[i]-1,tp.sigmas[i]-1) ^ (signS * (P(tp.sigmas[i]-1)-1));

		long long int r_;
		if (i < tp.mM)
			r_ = (table[i][ai] * ((s<<1)+1)) >> tp.sigmas[i];
		else
			r_ = table[i][ai*P(tp.sigmas[i]-1)+s];
		r_ = !(signS ^ signTable) ? r_ : -r_-1;
		r += (!(d % 2) || !sign) ? r_ : -r_-1;
	}

	mpz_clear(mpS);
	mpz_clear(mpTmp);

	return r;
}

PWPolynomial TermPowMult::calcErrTab(int k_)
{
	PWPolynomial errP;

	for (long long int i = 0; i < P(p.alpha); i++) {
		Polynomial errP_ = Polynomial(d, k[i] - kList[0][I(i,p.alpha,tp.alphas[0])]) << ((E(-p.alpha)-E(-p.wI))*.5);
		double err = 0;
		for (int j = 1; j < tp.mM+tp.mT; j++)
			err += fabs(kList[0][I(i,p.alpha,tp.alphas[0])] - kList[j][I(i,p.alpha,tp.alphas[j])]) *
				(P(tp.sigmas[j])-1) * E(-d*(p.alpha+1)+1-tp.rhos[j]-tp.sigmas[j]-1);
		errP_ = errP_ + (!k_ ? err : -err);
		errP.set(i * E(-p.alpha), (i+1) * E(-p.alpha), errP_ << (i * E(-p.alpha)));
	}

	return errP;
}

PWPolynomial TermPowMult::calcErrPow(int k_)
{
	PWPolynomial errP;
	PWPolynomial errPow_[2];

	errPow_[0] = pow->getErrPow()[k_];
	errPow_[1] = pow->getErrPow()[d%2 ? 1-k_ : k_] * (d%2 ? -1 : 1);
	errPow_[1].set(-E(-p.beta+1), 0, errPow_[1].get(0));
	errPow_[1] = (errPow_[1] ^ (-1)) >> E(-p.beta+1);

	PWPolynomial errPow;

	errPow.set( 0, 1, errPow_[0]);
	errPow.set(-1, 0, errPow_[1]);

	for (long long int i = 0; i < P(p.alpha); i++) {
		errP.set(i * E(-p.alpha), (i+1) * E(-p.alpha),
						 (((errPow * kList[0][I(i,p.alpha,tp.alphas[0])] * E(-d*(p.alpha+1))) ^ E(-p.alpha-1))
							<< (i * E(-p.alpha) + E(-p.alpha-1))));
	}

	return errP;
}

class TermPowMultTableInstance: public flopoco::Operator {
	public:
	TermPowMultTableInstance (flopoco::Target* target, int d, int mM,
	                          int mT,
	                          int alpha, int sigma, int wTable, int i,
	                          long long int table[], std::string name)
		:Operator (target) {
		setName (name);
		vhdl << "--------------------------------------------------------------------------------" << endl;
		vhdl << "-- TermPowMult::Table instance for order-" << d << " term Q_" << (i+1) << "." << endl;
		vhdl << "-- Decomposition:" << endl;
		vhdl << "--   alpha_" << d << "," << (i+1) << " = " << alpha << "; ";
		if (i >= mM)
			vhdl << "sigma'_" << d << "," << (i+1) << " = " << (sigma-1) << "; ";
		vhdl << "wO_" << d << "," << (i+1) << " = " << wTable << "." << endl;
		vhdl << endl;

		if (alpha)
			addInput ("a", alpha);
		if ((i >= mM) && (sigma > 1))
			addInput ("s", sigma-1);
		addOutput ("r", wTable);

		int wI = i < mM ? alpha : alpha+sigma-1;
		if (wI)
			declare ("x", wI);

		if (wI) {
			if ((i < mM) || (sigma <= 1))
				vhdl << "  x <= a;" << endl;
			else if (!alpha)
				vhdl << "  x <= s;" << endl;
			else
				vhdl << "  x <= a & s;" << endl;
			vhdl << endl;
			VHDLGen::genROM(vhdl, table, wI, wTable, "x", "r");
		}
		else {
			vhdl << "  r <= ";
			VHDLGen::genInteger(vhdl, table[0], wTable);
			vhdl << ";" << endl;
		}

		vhdl << endl << endl;
	}
	~TermPowMultTableInstance() {
	}
};

Component::Component (flopoco::Target* t, TermPowMult tpm, std::string name)
	:Operator (t)
{
	int d = tpm.d;
	int* wTable = tpm.wTable;
	long long int** table = tpm.table;
	TermPowMultParam& tp = tpm.tp;
	bool signTable = tpm.signTable;
	Param& p = tpm.p;
	Power* pow = tpm.pow;
	setName (name);
	using namespace flopoco;

	for (int i = 0; i < tp.mM+tp.mT; i++) {
	}

	vhdl << "--------------------------------------------------------------------------------" << endl;
	vhdl << "-- TermPowMult instance for order-" << d << " term." << endl;
	vhdl << "-- Decomposition:" << endl;
	vhdl << "--   alpha_" << d << " = " << tp.alpha << "; " << "beta_" << d << " = " << tp.beta << "; "
		 << "lambda_" << d << " = " << tp.p->lambda << "; " << " m_" << d << " = " << (tp.mM+tp.mT) << ";" << endl;
	vhdl << "--   Pow   " << (tp.p->type == POWER_TYPE_ROM ? "(ROM)" : "(AdHoc)") << ";" << endl;
	for (int i = 0; i < tp.mM+tp.mT; i++) {
		vhdl << "--   Q_" << d << "," << (i+1) << " " << (i < tp.mM ? "(Mult):" : "(ROM): ")
			 << " alpha_" << d << "," << (i+1) << " = " << tp.alphas[i] << "; "
			 << "rho_" << d << "," << (i+1) << " = " << tp.rhos[i] << "; "
			 << "sigma_" << d << "," << (i+1) << " = " << tp.sigmas[i] << "; "
			 << "wO_" << d << "," << (i+1) << " = " << wTable[i] << (i < tp.mM+tp.mT-1 ? ";" : ".") << endl;
	}
	vhdl << endl;

	if (tp.alpha)
		addInput ("a", tp.alpha);
	addInput ("b", tp.beta);
	addOutput ("r", p.wO+p.g+1);

	declare ("sign");
	if(tp.beta >= 2)
		declare ("b0", tp.beta-1);

	for (int i = 0; i < tp.mM+tp.mT; i++) {
		vhdl << endl;
		if (tp.alphas[i])
			declare (join("a_",i+1), tp.alphas[i]);
		declare (join("sign_",i+1));
		if (tp.sigmas[i] > 1)
			declare (join("s_",i+1), tp.sigmas[i]-1);
		// exclude outPortMapped r0's:
		if (i < tp.mM)
			declare (join("r0_",i+1), tp.sigmas[i]+wTable[i]);
		declare (join("r_",i+1), p.wO+p.g+1);
	}

	vhdl << "  sign <= not b(" << (tp.beta-1) << ");" << endl;
	if(tp.beta >= 2)
		vhdl << "  b0 <= b(" << (tp.beta-2) << " downto 0) xor (" << (tp.beta-2) << " downto 0 => sign);" << endl;
	vhdl << endl;

	{
		ostringstream buf;
		buf << name << "_pow";
		Operator* op = pow->toComponent(t, buf.str());
		oplist.push_back (op);

		outPortMap (op, "r", "s");
		if (tp.beta >= 2)
			inPortMap (op, "x", "b0");

		vhdl << instance (op, "pow");
	}

	for (int i = 0; i < tp.mM+tp.mT; i++) {
		if (tp.alphas[i])
			vhdl << "  a_" << (i+1) << " <= a" << range(tp.alpha-1, tp.alpha-tp.alphas[i]) << ";" << endl;
		vhdl << "  sign_" << (i+1) << " <= not s" << of(tp.p->lambda-tp.rhos[i]-1) << ";" << endl;
		if (tp.sigmas[i] > 1) {
			vhdl << "  s_" << (i+1) << " <= s" << range(tp.p->lambda-tp.rhos[i]-2, tp.p->lambda-tp.rhos[i]-tp.sigmas[i])
			     << " xor " << rangeAssign(tp.p->lambda-tp.rhos[i]-2, tp.p->lambda-tp.rhos[i]-tp.sigmas[i], join("sign_",i+1))
			     << ";" << endl;
		}

		{
			TermPowMultTableInstance* op = new TermPowMultTableInstance
				(t, d, tp.mM, tp.mT, tp.alphas[i], tp.sigmas[i], wTable[i], i, table[i], getName() + flopoco::join("_t",i+1));
			oplist.push_back (op);

			outPortMap (op, "r", join(i < tp.mM ? "k_" : "r0_",i+1));
			if ((i >= tp.mM) && (tp.sigmas[i] > 1))
				inPortMap (op, "s", join("s_",i+1));
			if (tp.alphas[i])
				inPortMap (op, "a", join("a_",i+1));

			vhdl << instance (op, join("t_",i+1));
		}

		if (i < tp.mM)
			vhdl << "  r0_" << (i+1) << " <= k_" << (i+1) << " * (s_" << (i+1) << " & \"1\");" << endl;
		vhdl << "  r_" << (i+1) << "(" << (wTable[i]-1) << " downto 0) <=" << endl << "    r0_" << (i+1);
		if (i < tp.mM)
			vhdl << "(" << (tp.sigmas[i]+wTable[i]-1) << " downto " << tp.sigmas[i] << ")";
		vhdl << " xor (";
		if (i < tp.mM)
			vhdl << (tp.sigmas[i]+wTable[i]-1) << " downto " << tp.sigmas[i];
		else
			vhdl << (wTable[i]-1) << " downto 0";
		vhdl << " => (" << (signTable ? "not " : "") << "(" << (d%2 ? "sign xor " : "") << "sign_" << (i+1) << ")));" << endl;

		if (p.wO+p.g+1 > wTable[i]) {
			vhdl << "  r_" << (i+1) << "(" << (p.wO+p.g) << " downto " << wTable[i] << ") <= ("
				 << (p.wO+p.g) << " downto " << wTable[i] << " => (" << (signTable ? "not " : "")
				 << "(" << (d%2 ? "sign xor " : "") << "sign_" << (i+1) << ")));" << endl;
		}
		vhdl << endl;
	}

	vhdl << "  r <= ";
	for (int i = 0; i < tp.mM+tp.mT; i++)
		vhdl << (i ? " + " : "") << "r_" << (i+1);
	vhdl << ";" << endl;
}
