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
#include "Param.hh"



PowerParam::PowerParam(int type_, int beta_, int lambda_)
	: type(type_), beta(beta_), lambda(lambda_)
{
}



PowerROMParam::PowerROMParam(int beta_, int lambda_)
	: PowerParam(POWER_TYPE_ROM, beta_, lambda_)
{
}

PowerROMParam *PowerROMParam::clone()
{
	return new PowerROMParam(beta, lambda);
}

void PowerROMParam::print(ostream &os)
{
	os << "rom " << lambda;
}



PowerAdHocParam::PowerAdHocParam(int beta_, int mu_, int lambda_)
	: PowerParam(POWER_TYPE_ADHOC, beta_, lambda_), mu(mu_)
{
}

PowerAdHocParam *PowerAdHocParam::clone()
{
	return new PowerAdHocParam(beta, mu, lambda);
}

void PowerAdHocParam::print(ostream &os)
{
	os << "adhoc " << mu << " " << lambda;
}



TermParam::TermParam(int type_, int alpha_, int beta_)
	: type(type_), alpha(alpha_), beta(beta_)
{
}



TermROMParam::TermROMParam(int alpha_, int beta_)
	: TermParam(TERM_TYPE_ROM, alpha_, beta_)
{
}

TermROMParam *TermROMParam::clone()
{
	return new TermROMParam(alpha, beta);
}

void TermROMParam::print(ostream &os)
{
	os << "rom " << alpha << " " << beta;
}



TermPowMultParam::TermPowMultParam(int alpha_, PowerParam *p_, int mM_, int mT_, int *alphas_, int *sigmas_)
	: TermParam(TERM_TYPE_POWMULT, alpha_, p_->beta), p(p_->clone()), mM(mM_), mT(mT_),
		alphas(new int[mM+mT]), rhos(new int[mM+mT]), sigmas(new int[mM+mT])
{
	for (int j = 0; j < mM+mT; j++) {
		alphas[j] = alphas_[j];
		sigmas[j] = sigmas_[j];
		rhos[j] = !j ? 0 : rhos[j-1] + sigmas[j-1];
	}
}

TermPowMultParam::~TermPowMultParam()
{
	delete p;
	delete[] alphas;
	delete[] rhos;
	delete[] sigmas;
}

TermPowMultParam *TermPowMultParam::clone()
{
	return new TermPowMultParam(alpha, p, mM, mT, alphas, sigmas);
}

void TermPowMultParam::print(ostream &os)
{
	os << "powmult " << alpha << " " << beta << "  ";
	p->print(os);
	os << "  " << mM << " " << mT;
	for (int i = 0; i < mM+mT; i++)
		os << "  " << alphas[i] << " " << sigmas[i];
}



Param::Param(int wI_, int wO_, int n_)
	: wI(wI_), wO(wO_), n(n_), t(new TermParam *[n+1])
{
	for (int i = 0; i <= n; i++)
		t[i] = NULL;
}

Param::Param(const Param &p)
	: wI(p.wI), wO(p.wO), n(p.n), alpha(p.alpha), beta(p.beta), t(new TermParam *[n+1]), g(p.g)
{
	for (int i = 0; i <= n; i++)
		t[i] = p.t[i] ? p.t[i]->clone() : NULL;
}

Param::~Param()
{
	for (int i = 0; i <= n; i++) {
		if (t[i])
			delete t[i];
	}
	delete[] t;
}

void Param::print(ostream &os)
{
	os << wI << " " << wO << " " << n << "   " << alpha;
	for (int i = 0; i <= n; i++) {
		os << "   ";
		if (t[i])
			t[i]->print(os);
		else
			os << "NULL";
	}
}
