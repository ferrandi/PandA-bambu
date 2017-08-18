/*
  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  
  All rights reserved.
*/

#include <iostream>
#include <math.h>
#include <cstdlib>
#include "../utils.hpp"
#include "KCMTable.hpp"
using namespace std;


namespace flopoco{

	// A table for the KCM Multiplication
	// the size of the input to the table will be the number of inputs of a LUT
	KCMTable::KCMTable(Target* target, int wIn, int wOut, mpz_class C, bool inputSigned, map<string, double> inputDelays) : 
	Table(target, wIn, wOut, 0, -1, true, inputDelays),  C_(C), inputSigned_(inputSigned)
	{
		ostringstream name; 
		srcFileName="KCMTable";
		name <<"KCMTable_" << wIn << "_" << C << (inputSigned_?"_signed":"_unsigned");
		setName(name.str());

	}

	KCMTable::~KCMTable() {}

	mpz_class KCMTable::function(int x) {
		mpz_class result;
  
		if(inputSigned_) {
			int signedX = x - ((x >> (wIn-1))<<wIn); // two's compliment
			result = mpz_class(signedX) * C_;

			if (result < 0 )
				result = (mpz_class(1)<<(wOut)) + result; //this is how we pass negatives to 2's complement
		} 
		else {
			result = mpz_class(x) * C_;
		}
		return  result;
	}

}
