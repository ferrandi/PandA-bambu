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
#include "FirstInvTable.hpp"
#include "FirstLogTable.hpp"
using namespace std;


namespace flopoco{


	FirstLogTable::FirstLogTable(Target *target, int wIn, int wOut, FirstInvTable* fit, FPLog* op_) : 
		Table(target, wIn, wOut), fit(fit), op(op_)  
	{
		ostringstream name; 
		name <<"LogTable_0_"<<wIn<<"_"<<wOut;
		setName(name.str());

		minIn = 0;
		maxIn = (1<<wIn) -1;
		if (wIn!=fit->wIn) {
			cerr<< "FirstLogTable::FirstLogTable should use same wIn as FirstInvTable"<<endl;
			exit(1);
		}
	}

	FirstLogTable::~FirstLogTable() {}


	int    FirstLogTable::double2input(double x){
		int result;
		cerr << "??? FirstLogTable::double2input not yet implemented ";
		exit(1);
		//   result = (int) floor(x*((double)(1<<(wIn-1))));
		//   if( result < minIn || result > maxIn) {
		//     cerr << "??? FirstLogTable::double2input:  Input "<< result <<" out of range ["<<minIn<<","<<maxIn<<"]";
		//     exit(1);
		//  }
		return result;
	}

	double FirstLogTable::input2double(int x) {
		return(fit->input2double(x));
	}

	mpz_class    FirstLogTable::double2output(double y){
		//Here y is between -0.5 and 0.5 strictly, whatever wIn.  Therefore
		//we multiply y by 2 before storing it, so the table actually holds
		//2*log(1/m)
		double z = floor(2*y*((double)(1<<(wOut-1)))); 

		// otherwise, signed arithmetic on wOut bits
		if(z>=0)
			return (mpz_class) z;
		else
			return (z + (double)(1<<wOut));
    
	}

	double FirstLogTable::output2double(mpz_class x) {
		cerr<<" FirstLogTable::output2double TODO"; exit(1);
		//  double y=((double)x) /  ((double)(1<<(wOut-1)));
		//return(y);
	}


	mpz_class FirstLogTable::function(int x)
	{ 
		mpz_class result;
		double apprinv;
		mpfr_t i,l;
		mpz_t r;

		mpfr_init(i);
		mpfr_init2(l,wOut);
		mpz_init2(r,400);
		apprinv = fit->output2double(fit->function(x));;
		// result = double2output(log(apprinv));
		mpfr_set_d(i, apprinv, GMP_RNDN);
		mpfr_log(l, i, GMP_RNDN);
		mpfr_neg(l, l, GMP_RNDN);

		// Remove the sum of small offsets that are added to the other log tables
		for(int j=1; j<=op->stages; j++){
			mpfr_set_d(i, 1.0, GMP_RNDN);
			int pi=op->p[j];
			mpfr_mul_2si(i, i, -2*pi, GMP_RNDN);
			mpfr_sub(l, l, i,  GMP_RNDN);
		}

 

		// code the log in 2's compliment
		mpfr_mul_2si(l, l, wOut, GMP_RNDN);
		mpfr_get_z(r, l, GMP_RNDN);
		result = mpz_class(r); // signed

		// This is a very inefficient way of converting
		mpz_class t = mpz_class(1) << wOut; 
		result = t+result;
		if(result>t) result-=t;

		//  cout << "x="<<x<<" apprinv="<<apprinv<<" logapprinv="<<log(apprinv)<<" result="<<result<<endl;
		mpfr_clear(i);
		mpfr_clear(l);
		mpz_clear(r);
		return  result;
	}


}
//void FirstLogTable::check_accuracy() {}
	
