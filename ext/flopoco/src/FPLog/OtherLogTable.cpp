/*
  A table of logs for the argument reduction of the floating-point logarithm 
  Author:  Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.

 */
#include <iostream>
#include <math.h>
#include <cstdlib>
#include "../utils.hpp"
#include "OtherLogTable.hpp"
using namespace std;



namespace flopoco{

	// A table of log 
	// -log(1-x) where x < 2^-p and x on a+1 bits.
	// the log is then smaller than 2^-p+1
	//  outputPrecision is the weight of the last bit in the real domain
	OtherLogTable::OtherLogTable(Target* target, int wIn, int outputPrecision, int which, int ai, int pi) : 
		Table(target, wIn, outputPrecision, 0, -1, 1),  which(which), ai(ai), pi(pi)
	 // TODO this forces a logic-based table

	{
		ostringstream name; 
		name <<"LogTable_"<<which<<"_"<<wIn<<"_"<<wOut;
		setName(name.str());
	}

	OtherLogTable::~OtherLogTable() {}


	int    OtherLogTable::double2input(double x){
		int result;
		cerr << "??? OtherLogTable::double2input not yet implemented ";
		exit(1);
		return result;
	}


	double OtherLogTable::input2double(int x) {
		double d; 
		// function uses log1p, so we prepare d for that 
		// computation in double is exact as long as we don't want a quad
		// operator...

		double Ei;
		if((which>1) || (which==1 && (1==(x>>(wIn-1)))) ) 
			Ei = 1.0 / ((double) (((uint64_t) 1)<<(2*pi)));
		else
			Ei = 1.0 / ((double) (((uint64_t) 1)<<(2*pi+1)));
	  
		d = ((double) (-x))   /   ((double) (((uint64_t) 1)<<(pi+wIn)));

		//cout << endl << d << " " << Ei << "   " ;
		d += Ei;
		//cout << d;
		return d; 
	}





	mpz_class    OtherLogTable::double2output(double y){
		mpz_class z; 
		z = (mpz_class) (  y *  ((double)(((int64_t)1)<< outputPrecision)) );
		// TODO fix for more than 64-bit
		//  z = (mpz_class(1)<< outputPrecision)  (  y *  ((double)(((int64_t)1)) );
		if (0 != z>>wOut) {
			cerr<<"OtherLogTable::double2output: output does not fit output format"<<endl; 
		}
		return z; 
	}



	double OtherLogTable::output2double(mpz_class x) {
		cerr<<" OtherLogTable::output2double TODO"; exit(1);
		//double y=((long double)x) /  ((long double)(1<<outputPrecision));
		//return(y);
	}





	mpz_class OtherLogTable::function(int x) {
		mpz_class result;
		double apprinv;
		mpfr_t i,l;
		mpz_t r;

		mpfr_init(i);
		mpfr_init2(l,wOut);
		mpz_init2(r,400);


		apprinv = input2double(x);
		mpfr_set_d(i, apprinv, GMP_RNDN);
		mpfr_log1p(l, i, GMP_RNDN);
		mpfr_neg(l, l, GMP_RNDN);
		// cout << "which=" << which <<  " div" << (pi+wIn+1) << "  x=" << x << "  apprinv=" << apprinv << "  l=" << mpfr_get_d(l, GMP_RNDN) << endl; 
		// Add the small offset that ensures that it never gets negative (the sum of these offsets will be subtracted to L0)
		mpfr_set_d(i, 1.0, GMP_RNDN);
		mpfr_mul_2si(i, i, -2*pi, GMP_RNDN);
		mpfr_add(l, l, i,  GMP_RNDN);

		mpfr_mul_2si(l, l, pi+wOut, GMP_RNDN);
		mpfr_get_z(r, l, GMP_RNDN);
		result=mpz_class(r);
		mpfr_clear(i);
		mpfr_clear(l);
		mpz_clear(r);
		return  result;
	}

}
