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
/* vim: set tabstop=8 softtabstop=2 shiftwidth=2: */
#include <fstream>
#include <sstream>

#include "Minimax.hh"
#include "TermROM.hh"
#include "TermPowMult.hh"
#include "HOTBMInstance.hh"
#include "../HOTBM.hpp"

namespace flopoco{

	string hexencode(string s)
	{
		string r;
		string::iterator it;
		for (it = s.begin(); it != s.end(); it++)
			{
				char h;
				h = (*it / 16);
				h += (h <= 9) ? '0' : 'A'-10;
				r += h;
				h = (*it % 16);
				h += (h <= 9) ? '0' : 'A'-10;
				r += h;
			}
		return r;
	}

	bool HOTBMInstance::ltParam::operator()(const tParam &p1, const tParam &p2) const
	{
		int nameCmp = p1.first.compare(p2.first);

		if (nameCmp)
			return nameCmp < 0;

		int n1 = (p1.second.wI << 16) | (p1.second.alpha << 6) | (p1.second.n);
		int n2 = (p2.second.wI << 16) | (p2.second.alpha << 6) | (p2.second.n);
		return n1 < n2;
	}

	bool HOTBMInstance::ltTermParam::operator()(const tTermParam &p1, const tTermParam &p2) const
	{
		bool paramCmp1 = ltParam()(p1.first, p2.first);
		bool paramCmp2 = ltParam()(p2.first, p1.first);

		if (paramCmp1 || paramCmp2)
			return paramCmp1;

		if (p1.second != p2.second)
			return p1.second < p2.second;

		int i = p1.second;
		int n1 = (p1.first.second.t[i]->alpha << 20) | (p1.first.second.t[i]->beta << 6) | (p1.first.second.t[i]->type);
		int n2 = (p2.first.second.t[i]->alpha << 20) | (p2.first.second.t[i]->beta << 6) | (p2.first.second.t[i]->type);
		if (n1 != n2)
			return n1 < n2;

		if (p1.first.second.t[i]->type == TERM_TYPE_POWMULT) {
			TermPowMultParam &tp1 = *((TermPowMultParam *)p1.first.second.t[i]);
			TermPowMultParam &tp2 = *((TermPowMultParam *)p2.first.second.t[i]);
			n1 = (tp1.p->beta << 20) | (tp1.p->lambda << 10) | (tp1.p->type << 6) | (tp1.mM << 3) | (tp1.mT);
			n2 = (tp2.p->beta << 20) | (tp2.p->lambda << 10) | (tp2.p->type << 6) | (tp2.mM << 3) | (tp2.mT);

			if (n1 != n2)
				return n1 < n2;

			for (int j = 0; j < tp1.mM+tp1.mT; j++) {
				n1 = (tp1.alphas[j] << 16) | (tp1.sigmas[j]);
				n2 = (tp2.alphas[j] << 16) | (tp2.sigmas[j]);
				if (n1 != n2)
					return n1 < n2;
			}

			if (tp1.p->type == POWER_TYPE_ADHOC)
				return ((PowerAdHocParam *)tp1.p)->mu < ((PowerAdHocParam *)tp2.p)->mu;
			else
				return false;
		}
		else
			return false;
	}



	bool HOTBMInstance::verbose = true;
	bool HOTBMInstance::force = false;

	HOTBMInstance::tApproxCache HOTBMInstance::approxCache;
	HOTBMInstance::tTermCache HOTBMInstance::termCache;



	HOTBMInstance::HOTBMInstance(Function &f_, Param &p_)
		: f(f_), p(p_), tList(NULL), kAdjust(NULL)
	{
		computeApprox();

		buildTerms();

		roundTables(false);
	}

	HOTBMInstance::~HOTBMInstance()
	{
		if (tList) {
			for (int i = 0; i <= p.n; i++)
				tList[i]->refCount--;
			delete[] tList;
		}
		if (kAdjust)
			delete[] kAdjust;
	}

	Param &HOTBMInstance::getParam()
	{
		return p;
	}

	void HOTBMInstance::freeApproxCache()
	{
		for (tApproxCache::iterator p = approxCache.begin(); p != approxCache.end(); p++) {
			for (int i = 0; i <= p->first.second.n; i++)
				delete[] p->second.first[i];
			delete[] p->second.first;
		}
		approxCache.clear();
	}

	void HOTBMInstance::freeTermCache(bool full)
	{
		tTermCache termCache_;
		int n = 0;
		for (tTermCache::iterator p = termCache.begin(); p != termCache.end(); p++) {
			if (full || !p->second->refCount) {
				delete p->second;
				n++;
			}
			else
				termCache_.insert(*p);
		}
		termCache = termCache_;
	}

	void HOTBMInstance::tune()
	{
		int nextG = 1;
		bool check = false;
		do {
			cerr << "Testing with " << nextG << " guard bits...";
			double kAdjust_ = (nT-1) * E(-p.wO-1) * (E(-nextG) - E(-p.g));
			p.g = nextG;
			for (int i = 0; i < P(p.t[0]->alpha); i++)
				kAdjust[i] += kAdjust_;
			roundTables();
			check = checkError();
			cerr << (check ? " OK" : " Failed") << endl;
		} while (!check && (++nextG <= gMax));

		if (!check)
			throw "HOTBMInstance::tune: Not enough guard bits.";
	}

	void HOTBMInstance::dump(flopoco::FlopocoStream& vhdl, list<int> &dumpList)
	{
		vhdl.vhdlCodeBuffer.precision(50);
		for (long long int i = 0; i < P(p.wI); i++) {
			double x = i * E(-p.wI);

			for (list<int>::iterator k = dumpList.begin(); k != dumpList.end(); k++) {
				if (k != dumpList.begin())
					vhdl << " ";

				switch (*k) {
				case HOTBM_DUMP_X:
					vhdl << x;
					break;

				case HOTBM_DUMP_FUNCTION:
					//vhdl << f.eval(x);
					break;

				case HOTBM_DUMP_APPROX: {
					Polynomial p_;
					for (int j = 0; j <= p.n; j++)
						p_ = p_ + Polynomial(j, kList[j][I(i,p.wI,p.alpha)]);
					vhdl << p_.eval(I(i,p.beta,p.beta) * E(-p.wI) - (E(-p.alpha)-E(-p.wI))/2);
					break;
				}

				case HOTBM_DUMP_METHOD:
					vhdl << eval(i);
					break;

				case HOTBM_DUMP_ROUND:
					vhdl << (evalRound(i) * E(-p.wO));
					break;

				case HOTBM_DUMP_ERR_POLY:
					vhdl << errPoly.eval(x);
					break;

				case HOTBM_DUMP_ERR_METHOD:
					vhdl << errMethod[0].eval(x) << " " << errMethod[1].eval(x);
					break;

				case HOTBM_DUMP_ERR_ROUND: {
					double err = nT * E(-p.wO-p.g-1) + E(-p.wO-1) * (1 - E(-p.g));
					vhdl << err << " " << -err;
					break;
				}

				case HOTBM_DUMP_ERR_MAX:
					vhdl << E(-p.wO) << " " << (-E(-p.wO));
					break;

				case HOTBM_DUMP_INPUT_WORD:
					vhdl << i;
					break;

				case HOTBM_DUMP_OUTPUT_WORD:
					vhdl << evalRound(i);
					break;

				default:
					break;
				}
			}

			vhdl << endl;
		}
	}

	void HOTBMInstance::computeApprox()
	{
		tParam paramKey(f.getName(), p);
		for (int i = 0; i <= p.n; i++) {
			delete paramKey.second.t[i];
			paramKey.second.t[i] = NULL;
		}

		if (approxCache.count(paramKey)) {
			tApprox approx = approxCache[paramKey];
			kList = approx.first;
			errPoly = approx.second.first;
			errBound[0] = approx.second.second;
			errBound[1] = -errBound[0];
		}
		else {
			fstream *fCache;
			bool inCache = false;
			{
				ostringstream buf;
				buf << "cache/" << hexencode(f.getName()) << "_" << p.wI << "_" << p.alpha << "_" << p.n << ".minimax";
				fCache = new fstream(buf.str().c_str(), fstream::in);
				inCache = fCache->is_open();
				if (!inCache) {
					delete fCache;
					fCache = new fstream(buf.str().c_str(), fstream::out);
				}
			}

			kList = new double *[p.n+1];
			for (int i = 0; i <= p.n; i++)
				kList[i] = new double[P(p.alpha)];

			if (!inCache) {
				if (verbose)
					cerr << "Computing minimax approximations..." << endl;
				int w = (int)(1 + p.alpha / log2(10));

				for (long long int i = 0; i < P(p.alpha); i++) {
					if (!(i % 16)) {
						cerr.width(w); cerr << i << " / ";
						cerr.width(w); cerr << P(p.alpha) << "   ";
					}

					double ia =  i    * E(-p.alpha);
					double ib = (i+1) * E(-p.alpha);
					double ib_ = ib - E(-p.wI);

					Minimax *m;
					try {
						m = new Minimax(f, ia, ib_, p.n);

						MPPolynomial mpP = m->getMPP() >> ((ia+ib_)/2);
						mpfr_t mpTmp;
						mpfr_init(mpTmp);

						char buf[100];
						mp_exp_t exp;
						for (int j = 0; j <= p.n+1; j++) {
							if (j <= p.n) {
								mpP.getMPK(mpTmp, j);
								kList[j][i] = mpfr_get_d(mpTmp, GMP_RNDN);
							}
							else {
								m->getMPErr(mpTmp);
								errPoly.set(ia, ib, mpfr_get_d(mpTmp, GMP_RNDN));
							}

							mpfr_get_str(buf, &exp, 2, 80, mpTmp, GMP_RNDN);
							*fCache << buf << "e" << (exp-80) << endl;
						}

						mpfr_clear(mpTmp);
						delete m;
					}
					catch (const char *s) {
						throw (string(s) + "\nMinimax exited abruptly. Memory might leak.").c_str();
					}

					cerr << " ";
					if (!((i+1) % 16) || (i+1 == P(p.alpha)))
						;//cerr << endl;
				}
			}
			else {
				if (verbose)
					cerr << "Reading minimax approximations from cache..." << endl;
				for (long long int i = 0; i < P(p.alpha); i++) {
					double ia =  i    * E(-p.alpha);
					double ib = (i+1) * E(-p.alpha);

					char buf[100];
					mpfr_t mpTmp;
					mpfr_init(mpTmp);

					for (int j = 0; j <= p.n+1; j++) {
						*fCache >> buf;
						mpfr_set_str(mpTmp, buf, 2, GMP_RNDN);

						if (j <= p.n)
							kList[j][i] = mpfr_get_d(mpTmp, GMP_RNDN);
						else
							errPoly.set(ia, ib, mpfr_get_d(mpTmp, GMP_RNDN));
					}

					mpfr_clear(mpTmp);
				}
			}

			fCache->close();
			delete fCache;

			/*
			  for (long long int i = 0; i < P(p.alpha); i++) {
			  double ia =  i    * E(-p.alpha);
			  double ib = (i+1) * E(-p.alpha);
			  double ib_ = ib - E(-p.wI);

			  Taylor taylor(f, 16, (ia+ib_)/2);
			  Polynomial pTaylorDiff = taylor.getPoly();
			  for (int j = 0; j <= p.n; j++)
			  pTaylorDiff = pTaylorDiff - Polynomial(j, kList[j][i]);
			  pTaylorDiff = pTaylorDiff << ((ia+ib_)/2);

			  errPoly.set(ia, ib, pTaylorDiff);
			  }
			*/

			errBound[0] = errPoly.max(0, 1, E(-p.wI));
			errBound[1] = -errBound[0];

			approxCache[paramKey] = tApprox(kList, tApproxError(errPoly, errBound[0]));
		}

		if (verbose)
			cerr << "Minimax error (errPoly): " << errBound[0] << " " << errBound[1] << endl;
		if (errBound[0] >= E(-p.wO-1))
			throw 0;
		// XXX: const char * exceptions are fatal
		//throw "HOTBMInstance::computeApprox: Minimax error is too large for required precision.";
	}

	void HOTBMInstance::buildTerms()
	{
		tList = new Term *[p.n+1];
		nM = 0;
		nT = 0;
		if (verbose)
			cerr << "Building terms...";

		for (int i = 0; i <= p.n; i++) {
			tTermParam paramKey(tParam(f.getName(), p), i);
			for (int j = 0; j <= p.n; j++) {
				if (j == i)
					continue;
				delete paramKey.first.second.t[j];
				paramKey.first.second.t[j] = NULL;
			}

			if (termCache.count(paramKey))
				tList[i] = termCache[paramKey];
			else {
				if (p.t[i]->type == TERM_TYPE_ROM)
					tList[i] = new TermROM(i, kList[i], paramKey.first.second);
				else if (p.t[i]->type == TERM_TYPE_POWMULT)
					tList[i] = new TermPowMult(i, kList[i], paramKey.first.second);
				termCache[paramKey] = tList[i];
			}

			if (p.t[i]->type == TERM_TYPE_ROM)
				nT++;
			else if (p.t[i]->type == TERM_TYPE_POWMULT) {
				nM += ((TermPowMultParam *)p.t[i])->mM;
				nT += ((TermPowMultParam *)p.t[i])->mM + ((TermPowMultParam *)p.t[i])->mT;
			}

			PWPolynomial *errP = tList[i]->getErrMethod();
			for (int j = 0; j < 2; j++)
				errMethod[j] = errMethod[j] + errP[j];

			tList[i]->refCount++;
			if (verbose)
				cerr << " T" << i;
		}
		if (verbose)
			cerr << endl;

		PWPolynomial errTotal[2];
		for (int i = 0; i < 2; i++) {
			//errTotal[i] = errPoly + errMethod[i];
			errTotal[i] = errMethod[i] + errBound[i];
			errBound[i] = i ? HUGE_VAL : -HUGE_VAL;
		}

		kAdjust = new double[P(p.t[0]->alpha)];
		PWPolynomial pAdjust;
		for (long long int i = 0; i < P(p.t[0]->alpha); i++) {
			double ia =  i    * E(-p.t[0]->alpha);
			double ib = (i+1) * E(-p.t[0]->alpha);
			double max = errTotal[0].max(ia, ib, E(-p.wI));
			double min = errTotal[1].min(ia, ib, E(-p.wI));

			kAdjust[i] = (max+min) / 2;
			pAdjust.set(ia, ib, kAdjust[i]);

			max -= kAdjust[i];
			min -= kAdjust[i];

			if (errBound[0] < max)
				errBound[0] = max;
			if (errBound[1] > min)
				errBound[1] = min;
		}
		for (int i = 0; i < 2; i++)
			errMethod[i] = errMethod[i] - pAdjust;

		if (verbose)
			cerr << "Method error: " << errBound[0] << " " << errBound[1] << endl;

		if ((errBound[0] < E(-p.wO-1)) && (errBound[1] > -E(-p.wO-1)))
			gMax = (int)ceil(log2((nM+nT-1) / (E(-p.wO-1) - MAX(errBound[0],-errBound[1]))) - p.wO - 1);
		else if (!force) {
			for (int i = 0; i <= p.n; i++)
				tList[i]->refCount--;
			delete[] tList;
			delete[] kAdjust;

			throw "HOTBMInstance::buildTerms: Method error is too large for required precision.";
		}
		else
			gMax = 8;

		if (verbose)
			cerr << "Guard bits: " << gMax << endl;
		p.g = gMax;

		double kAdjust_ = (nT-1) * E(-p.wO-p.g-1) + E(-p.wO-1);
		for (int i = 0; i < P(p.t[0]->alpha); i++)
			kAdjust[i] += kAdjust_;
	}

	void HOTBMInstance::roundTables(bool full)
	{
		for (int i = 0; i <= p.n; i++)
			tList[i]->roundTables(p.g, full, !i ? kAdjust : NULL);
	}

	bool HOTBMInstance::checkError()
	{
		for (long long int i = 0; i < P(p.wI); i++) {
			if (::fabs(f.eval(i*E(-p.wI)) - evalRound(i)*E(-p.wO)) >= E(-p.wO))
				return false;
		}
		return true;
	}

	double HOTBMInstance::eval(long long int x)
	{
		double r = kAdjust[I(x,p.wI,p.t[0]->alpha)] - (nT-1) * E(-p.wO-p.g-1) - E(-p.wO-1);

		for (int i = 0; i <= p.n; i++) {
			long long int a = I(x,p.wI  ,p.t[i]->alpha);
			long long int b = I(x,p.beta,p.t[i]->beta );
			r += tList[i]->eval(a, b);
		}

		return r;
	}

	long long int HOTBMInstance::evalRound(long long int x)
	{
		long long int r = 0;

		for (int i = 0; i <= p.n; i++) {
			long long int a = I(x,p.wI  ,p.t[i]->alpha);
			long long int b = I(x,p.beta,p.t[i]->beta );
			r += tList[i]->evalRound(a, b);
		}

		r >>= p.g;

		return r;
	}

	double HOTBMInstance::estimArea(int t) const
	{
		double area = 0;

		if (t == -1) {
			for (int i = 0; i <= p.n; i++)
				area += tList[i]->estimArea();
		}
		else
			area = tList[t]->estimArea();

		area += Estim::adderArea(p.wO+p.g+1, p.n+1);

		return area;
	}

	double HOTBMInstance::estimDelay(int t) const
	{
		double delay = 0;

		if (t == -1) {
			for (int i = 0; i <= p.n; i++) {
				double delay_ = tList[i]->estimDelay();
				if (delay_ > delay)
					delay = delay_;
			}
		}
		else
			delay = tList[t]->estimDelay();

		delay += Estim::adderDelay(p.wO+p.g+1, p.n+1);

		return delay;
	}

	void flopoco::HOTBM::genVHDL()
	{
		Param& p = inst->p;
		Term** tList = inst->tList;

		vhdl << "--------------------------------------------------------------------------------" << endl;
		vhdl << "--------------------------------------------------------------------------------" << endl;
		vhdl << "-- HOTBM instance for function " << f.getName() << "." << endl;
		vhdl << "-- wI = " << p.wI << "; wO = " << p.wO << "." << endl;
		vhdl << "-- Order-" << p.n << " polynomial approximation." << endl;
		vhdl << "-- Decomposition:" << endl;
		vhdl << "--   alpha = " << p.alpha << "; beta = " << p.beta << ";" << endl;
		for (int i = 0; i <= p.n; i++) {
			vhdl << "--   T_" << i << " " << (p.t[i]->type == TERM_TYPE_ROM ? "(ROM):    " : "(PowMult):")
				<< " alpha_" << i << " = " << p.t[i]->alpha << "; beta_" << i << " = " << p.t[i]->beta
				<< (i == p.n ? "." : ";") << endl;
		}
		vhdl << "-- Guard bits: g = " << p.g << "." << endl;
		vhdl << "-- Command line: " << f.getName() << " ";
		{
			ostringstream os;
			p.print(os);
			vhdl << os.str();
		}
		vhdl << endl;

		for (int i = 0; i <= p.n; i++) {
			if (p.t[i]->alpha)
				vhdl << declare (join("a_",i),p.t[i]->alpha) << " <= x"
				     << range (p.wI-1, p.wI-p.t[i]->alpha) << ";\n";
			if (p.t[i]->beta)
				vhdl << declare (join("b_",i),p.t[i]->beta) << " <= x"
				     << range (p.beta-1, p.beta-p.t[i]->beta) << ";\n";

			ostringstream buf;
			buf << getName() << "_t" << i;
			Operator* op = tList[i]->toComponent (getTarget(), buf.str());

			outPortMap (op, "r", join("r_",i));
			if (p.t[i]->beta)
				inPortMap (op, "b", join("b_",i));
			if (p.t[i]->alpha)
				inPortMap (op, "a", join("a_",i));

			oplist.push_back(op);
			vhdl << instance(op, join("t_",i));
		}
		vhdl << declare ("sum", p.wO+p.g+1) << " <= ";
		for (int i = 0; i <= p.n; i++)
			vhdl << (i ? " + " : "") << "r_" << i;
		vhdl << ";" << endl;
		vhdl << "  r <= sum" << range (p.wO+p.g, p.g) << ";" << endl;
	}
}
