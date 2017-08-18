/*
  A table of reciprocals for the argument reduction of the floating-point logarithm 
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
using namespace std;



namespace flopoco{


	FirstInvTable::FirstInvTable(Target* target, int wIn, int wOut) : 
		Table(target, wIn, wOut)
	{
		ostringstream name; 
		name <<"InvTable_0_"<<wIn<<"_"<<wOut;
		setName(name.str());

	}

	FirstInvTable::~FirstInvTable() {}
  

	int    FirstInvTable::double2input(double x){
		int result;
		cerr << "??? FirstInvTable::double2input not yet implemented ";
		exit(1);
		return result;
	}


	double FirstInvTable::input2double(int x) {
		double y;
		if(x>>(wIn-1)) //MSB of input
			y= ((double)(x+(1<<wIn))) //      11xxxx (binary)
				/  ((double)(1<<(wIn+1))); // 0.11xxxxx (binary)
		else
			y= ((double)(x+(1<<wIn))) //   10xxxx (binary)
				/  ((double)(1<<(wIn))); // 1.0xxxxx (binary)
		return(y);
	}

	mpz_class FirstInvTable::double2output(double x){
		mpz_class result;
		result =  mpz_class(floor(x*((double)(1<<(wOut-1)))));
		return result;
	}

	double FirstInvTable::output2double(mpz_class x) {
		double y=((double)x.get_si()) /  ((double)(1<<(wOut-1)));
		return(y);
	}


	mpz_class FirstInvTable::function(int x)
	{
		double d;
		mpz_class r;
		d=input2double(x);
		r =  ceil( ((double)(1<<(wOut-1))) / d); // double rounding, but who cares really
		// The following line allows us to prove that the case a0=5 is lucky enough to need one bit less than the general case
		//cout << "FirstInvTable> x=" << x<< "  r=" <<r << "  error=" << ceil( ((double)(1<<(wOut-1))) / d)  - ( ((double)(1<<(wOut-1))) / d) << endl;
		return r;
	}



#if 0
	int FirstInvTable::check_accuracy(int wF) {
		int i;
		mpz_class j;
		double x1,x2,y,e1,e2;
		double maxerror=0.0;
		double prod=0.0;

		maxMulOut=0;
		minMulOut=2;

		for (i=minIn; i<=maxIn; i++) {
			// x1 and x2 are respectively the smallest and largest FP possible
			// values leading to input i
			x1=input2double(i); 
			if(i>>(wIn-1)) //MSB of input
				x2= - negPowOf2(wF)          // <--wF --->
					+ ((double)(i+1+(1<<wIn))) //   11 11...11 (binary)
					/ ((double)(1<<(wIn+1))); // 0.11 11...11 (binary)
			else
				x2= - negPowOf2(wF-1) 
					+ ((double)(i+1+(1<<wIn))) //  10 11...11 (binary)
					/ ((double)(1<<(wIn))); // 1.0 11...11 (binary)
			j=function(i);
			y=output2double(j);
			if(verbose)
				cout << "i="<<i<< " ("<<input2double(i)<<") j="<<j
					  <<" min="<< x1*y <<" max="<< x2*y<< endl;
			prod=x1*y; if (prod<minMulOut) minMulOut=prod;
			prod=x2*y; if (prod>maxMulOut) maxMulOut=prod;
			e1= fabs(x1*y-1); if (e1>maxerror) maxerror=e1;
			e2= fabs(x2*y-1); if (e2>maxerror) maxerror=e2;
		} 
		cout << "FirstInvTable: Max error=" <<maxerror << "  log2=" << log2(maxerror) <<endl; 
		cout << "               minMulOut=" <<minMulOut << " maxMulOut=" <<maxMulOut  <<endl; 

		printf("%1.30e\n", log2(maxerror));

		return (int) (ceil(log2(maxerror)));
	}

#endif
}
