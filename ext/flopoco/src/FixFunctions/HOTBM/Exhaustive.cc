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

#include "Util.hh"
#include "Exhaustive.hh"

namespace flopoco{


	bool Exhaustive::ltInstance::operator()(const tInstance &i1, const tInstance &i2) const
	{
		return i1.second < i2.second;
	}



	Exhaustive::Exhaustive(Function &f_, Param &p_)
		: f(f_), p(p_), instance(NULL)
	{
		HOTBMInstance::verbose = false;

		tInstance best(NULL, HUGE_VAL);
		int n = 0;
		bool done = false;
		for (int alpha = 0; (alpha <= p.wI-2) && (!done); alpha++) {
			Param p = this->p;
			tInstSet instSet;

			try {
				cerr << "Testing for (alpha, beta) = (" << alpha << ", " << (p.wI-alpha) << ")...";
				p.alpha = alpha;
				p.beta = p.wI-alpha;
				for (int j = 0; j <= p.n; j++)
					p.t[j] = new TermROMParam(p.alpha, j ? p.beta : 0);
				n++;
				HOTBMInstance *inst = new HOTBMInstance(f, p);
				instSet.insert(tInstance(inst, score(*inst)));
			}

			/* XXX: int exceptions are non-fatal
			 * const char * exceptions are fatal and pass through */
			catch (int) {
				cerr << " Failed" << endl;
				continue;
			}
			cerr << " OK" << endl;

			list<Param> pList;

			for (int i = 0; i <= p.n; i++) {
				cerr << "Term T" << i << "...";// << endl;

				tInstSet instSet_;

				instSet_ = instSet;
				for (tInstSet::iterator k = instSet_.begin(); k != instSet_.end(); k++) {
					Param p = (*k).first->getParam();
					TermParam &tp = *p.t[i];
					for (tp.alpha = p.alpha-1; tp.alpha >= 0; tp.alpha--)
						pList.push_back(p);
					n += process(pList, instSet, i < p.n ? -1 : 2);
					pList.clear();
				}
				skim(instSet, 4);

				cerr << " *";// << endl;
				if (!i)
					{
						cerr << endl;
						continue;
					}

				instSet_ = instSet;
				for (tInstSet::iterator k = instSet_.begin(); k != instSet_.end(); k++) {
					Param p = (*k).first->getParam();
					TermParam &tp = *p.t[i];
					for (tp.beta = p.beta-1; tp.beta >= 1; tp.beta--)
						pList.push_back(p);
					n += process(pList, instSet, i < p.n ? -1 : 4);
					pList.clear();
				}
				skim(instSet, 4);

				cerr << " *";// << endl;
				tInstSet termROMInstSet = instSet;

				instSet_ = instSet;
				instSet.clear();
				for (tInstSet::iterator k = instSet_.begin(); k != instSet_.end(); k++) {
					Param p = (*k).first->getParam();
					PowerROMParam pp(p.t[i]->beta, i*p.t[i]->beta);
					int alphas[1] = { p.t[i]->alpha };
					int sigmas[1] = { i*p.t[i]->beta };
					TermPowMultParam &tp = *(new TermPowMultParam(p.t[i]->alpha, &pp, 0, 1, alphas, sigmas));
					delete p.t[i];
					p.t[i] = &tp;
					for (tp.p->lambda = i*tp.beta; tp.p->lambda >= 1; tp.p->lambda--) {
						tp.sigmas[0] = tp.p->lambda;
						pList.push_back(p);
					}
					n += process(pList, instSet, i < p.n ? -1 : 4);
					pList.clear();
				}
				skim(instSet, 4);

				cerr << " *";// << endl;

				instSet_ = instSet;
				for (tInstSet::iterator k = instSet_.begin(); k != instSet_.end(); k++) {
					Param p = (*k).first->getParam();
					TermPowMultParam &tp = *(TermPowMultParam *)p.t[i];
					int mMax = (tp.p->lambda+5)/6;
					for (tp.mT = 2; tp.mT <= mMax; tp.mT++) {
						delete[] tp.alphas;
						delete[] tp.sigmas;
						delete[] tp.rhos;
						tp.alphas = new int[tp.mT];
						tp.sigmas = new int[tp.mT];
						tp.rhos = new int[tp.mT];
						for (int j = 0; j < tp.mT; j++) {
							tp.alphas[j] = tp.alpha;
							tp.sigmas[j] = !j ? tp.p->lambda+1-tp.mT : 1;
							tp.rhos[j] = !j ? 0 : tp.rhos[j-1]+tp.sigmas[j-1];
						}
						pList.push_back(p);
					}
					n += process(pList, instSet, -1);
					pList.clear();
				}

				cerr << " *";// << endl;

				instSet_ = instSet;
				instSet.clear();
				for (tInstSet::iterator k = instSet_.begin(); k != instSet_.end(); k++) {
					Param p = (*k).first->getParam();
					TermPowMultParam &tp = *(TermPowMultParam *)p.t[i];
					tInstSet localInstSet, localInstSet_;
					localInstSet.insert(*k);
					for (int j = 1; j < tp.mT; j++) {
						localInstSet_ = localInstSet;
						for (tInstSet::iterator k = localInstSet_.begin(); k != localInstSet_.end(); k++) {
							Param p = (*k).first->getParam();
							TermPowMultParam &tp = *(TermPowMultParam *)p.t[i];
							for (tp.alphas[j]--; tp.alphas[j] >= 0; tp.alphas[j]--) {
								for (int k = j+1; k < tp.mT; k++)
									tp.alphas[k] = tp.alphas[j];
								pList.push_back(p);						
							}
							n += process(pList, localInstSet, -1);
							pList.clear();
						}
					}
					instSet.insert(localInstSet.begin(), localInstSet.end());
				}

				cerr << " *";// << endl;

				instSet_ = instSet;
				instSet.clear();
				for (tInstSet::iterator k = instSet_.begin(); k != instSet_.end(); k++) {
					Param p = (*k).first->getParam();
					TermPowMultParam &tp = *(TermPowMultParam *)p.t[i];
					tInstSet localInstSet, localInstSet_;
					localInstSet.insert(*k);
					for (int j = 1; j < tp.mT; j++) {
						localInstSet_ = localInstSet;
						for (tInstSet::iterator k = localInstSet_.begin(); k != localInstSet_.end(); k++) {
							Param p = (*k).first->getParam();
							TermPowMultParam &tp = *(TermPowMultParam *)p.t[i];
							for (tp.sigmas[j]++; tp.sigmas[j-1] > 1; tp.sigmas[j]++) {
								tp.sigmas[j-1]--;
								tp.rhos[j]--;
								pList.push_back(p);						
							}
							n += process(pList, localInstSet, -1);
							pList.clear();
						}
					}
					instSet.insert(localInstSet.begin(), localInstSet.end());
				}

				cerr << " *";// << endl;

#if 0

				instSet_ = instSet;
				instSet.clear();
				for (tInstSet::iterator k = instSet_.begin(); k != instSet_.end(); k++) {
					Param p = (*k).first->getParam();
					TermPowMultParam &tp = *(TermPowMultParam *)p.t[i];
					tInstSet localInstSet, localInstSet_;
					localInstSet.insert(*k);
					for (int j = 1; j < tp.mT; j++) {
						localInstSet_ = localInstSet;
						for (tInstSet::iterator k = localInstSet_.begin(); k != localInstSet_.end(); k++) {
							Param p = (*k).first->getParam();
							TermPowMultParam &tp = *(TermPowMultParam *)p.t[i];
							for (tp.sigmas[j]++; tp.sigmas[j-1] > 1; tp.sigmas[j]++) {
								tp.sigmas[j-1]--;
								tp.rhos[j]--;
								pList.push_back(p);						
							}
							n += process(pList, localInstSet, i < p.n ? -1 : 4);
							pList.clear();
						}
					}
					instSet.insert(localInstSet.begin(), localInstSet.end());
				}
				skim(instSet, 16);

				cerr << " *" << endl;
#endif

				instSet_ = instSet;
				for (tInstSet::iterator k = instSet_.begin(); k != instSet_.end(); k++) {
					Param p = (*k).first->getParam();
					TermPowMultParam &tp = *(TermPowMultParam *)p.t[i];
					for (tp.mT--; tp.mT >= 0; tp.mT--) {
						tp.mM++;
						pList.push_back(p);
					}
					n += process(pList, instSet, -1);
					pList.clear();
				}
				skim(instSet, 4);

				cerr << " *";// << endl;

				instSet_ = instSet;
				for (tInstSet::iterator k = instSet_.begin(); k != instSet_.end(); k++) {
					Param p = (*k).first->getParam();
					TermPowMultParam &tp = *(TermPowMultParam *)p.t[i];
					PowerAdHocParam &pp = *(new PowerAdHocParam(tp.p->beta, i*tp.p->beta, tp.p->lambda));
					delete tp.p;
					tp.p = &pp;
					for (; pp.mu >= pp.lambda; pp.mu--)
						pList.push_back(p);
					n += process(pList, instSet, 2);
					pList.clear();
				}

				cerr << " *" << endl;

				instSet.insert(termROMInstSet.begin(), termROMInstSet.end());
				skim(instSet, 4);
			}

			skim(instSet, 1);
			cerr << instSet.begin()->second << " vs. " << best.second;
			if (instSet.begin()->second < best.second) {
				if (best.first)
					delete best.first;
				best = *instSet.begin();
			}
			else {
				delete instSet.begin()->first;
				done = true;
			}
			HOTBMInstance::freeTermCache();
			cerr << endl << endl;
		}

		cerr << "Tested " << n << " designs." << endl;
		instance = best.first;
	
		/* Nice error in case no design has been found */
		if (!instance)
			throw "No suitable design found.";

		cerr << "Best design: ";
		instance->getParam().print(cerr);
		cerr << endl << "Score: ";
		cerr << (long long int)best.second << endl;
	}

	Exhaustive::~Exhaustive()
	{
	}

	HOTBMInstance *Exhaustive::getInstance()
	{
		return instance;
	}

	double Exhaustive::score(const HOTBMInstance &inst)
	{
		double area	= inst.estimArea();
		double delay = inst.estimDelay();

		return area*area * delay;
	}

	int Exhaustive::process(list<Param> &pList, tInstSet &instSet, int nMax)
	{
		int n = 0;
		int count = 0;

		if (nMax > 0)
			pList.reverse();

		for (list<Param>::iterator p = pList.begin(); (p != pList.end()) && ((nMax < 0) || (count < nMax)); p++) {
			try {
				//(*p).print(cerr);
				n++;

				HOTBMInstance *inst = new HOTBMInstance(f, *p);
				instSet.insert(tInstance(inst, score(*inst)));
				count++;

				//cerr << "\033[100C\033[20D" << (long long int)score(*inst) << endl;
			}
			catch (const char *s) {
				//cerr << "\033[100CX" << endl;
				if (nMax == -1)
					break;
			}
		}

		return n;
	}

	void Exhaustive::skim(tInstSet &instSet, int nMax)
	{
		while (instSet.size() > (unsigned)nMax) {
			tInstSet::iterator k = --instSet.end();
			delete (*k).first;
			instSet.erase(k);
		}
		HOTBMInstance::freeTermCache();
	}

}
