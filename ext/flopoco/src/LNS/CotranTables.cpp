/*
  Helper table for LNS Cotransformation
 
  Author : Sylvain Collange

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

*/

#include "CotranTables.hpp"
#include <cmath>
#include <climits>

using namespace std;

namespace flopoco{

	double db2(double z)
	{
		// Assumes target precision is small enough for double precision to provide correct rounding...
		return log(abs(1. - pow(2, z)))/log(2.);
	}

	int CotranF1Table::addrLen(int wF, int j, int wE)
	{
		int k = min(int(log(wF+1)/log(2)+1), wE);
		return wF + k - j;
	}

	int CotranF1Table::dataLen(int wF, int j, int wE)
	{
		int k2 = int(log(wF+1)/log(2)+1);
		return wF + k2 + 1;
	}

	int CotranF1Table::esszero(int wF, int j, int wE)
	{
		double dh = 1. / (1 << (wF - j));
		int k = min(int(log(wF+1)/log(2)+1), wE);
		// negative number
		double esszero1 = log(1. - pow(2., -(pow(2., -wF)))) / log(2.0) - dh;
		//	cout << "esszero = " << esszero1 << endl;
	
		// 2's-complement
		return max(0, int(floor((1.+esszero1 / (1 << k)) * (1 << addrLen(wF, j, wE)))));
	}

	CotranF1Table::CotranF1Table(Target* target, int wF, int j, int wE) :
		Table(target, 
				addrLen(wF, j, wE),
				dataLen(wF, j, wE),
				esszero(wF, j, wE),
				(1 << addrLen(wF, j, wE)) - 2),
		wF(wF), j(j), k(int(log(wF+1)/log(2)+1)), wE(wE)
	{
		ostringstream name;
		name << "CotranF1Table_" << wE << "_" << wF<< "_" << j; 
		uniqueName_ = name.str(); 


		setCombinatorial();	

		dh = 1. / (1 << (wF - j));
	}


	mpz_class CotranF1Table::function(int x)
	{
		double y = ((double(x) / (1 << wIn)) - 1) * (1 << k);
	
		
		int decval = (int)rint(db2(-y-dh) * (1 << wF));

		// keep only low-order bits (2's-complement with wOut bits)
		decval &= ((1 << wOut) - 1);
		//	cout << "x=" << x << ", y=" << y << ", val=" << decval << endl;
		return mpz_class(decval);
	}


	/////////////////////////////////////////////////////

	int CotranF2Table::addrLen(int wF, int j)
	{
		return j;
	}

	int CotranF2Table::dataLen(int wF, int j)
	{
		return wF + int(log(wF+1)/log(2)+1) + 1;
	}


	CotranF2Table::CotranF2Table(Target* target, int wF, int j) :
		Table(target, 
				addrLen(wF, j),
				dataLen(wF, j)),
		wF(wF), j(j), k(int(log(wF+1)/log(2)+1))
	{
		ostringstream name;
		name << "CotranF2Table_"  << wF<< "_" << j; 
		uniqueName_ = name.str();

		setCombinatorial();	
 
		dh = 1. / (1 << (wF - j));
	}


	mpz_class CotranF2Table::function(int x)
	{
		double y = double(x) / (1 << wF);
	
		int decval = (int)rint(db2(y-dh) * (1 << wF));

		// keep only low-order bits (2's-complement with wOut bits)
		decval &= ((1 << wOut) - 1);
		//	cout << "x=" << x << ", y=" << y << ", val=" << decval << endl;
		return mpz_class(decval);
	}

	///////////////////////////////////////////////////


	int CotranF3Table::addrLen(int wF, int j)
	{
		return j + 2;
	}

	int CotranF3Table::dataLen(int wF, int j)
	{
		return wF + 1;
	}

	int two_compl(int i, int w)
	{
		return i & ((1 << w) - 1);
	}

	int sign_ext(int i, int w)
	{
		int shift_amount = (sizeof(int) * CHAR_BIT - w);
		return (i << shift_amount) >> shift_amount; 
	}

	int CotranF3Table::begin(int wF, int j)
	{
		double dh = 1. / (1 << (wF - j));
		return two_compl(floor(-2 * (dh * (1 << wF))), j + 2);
	}

	int CotranF3Table::end(int wF, int j)
	{
		double dh = 1. / (1 << (wF - j));
		return two_compl(ceil(-log(2 * pow(2., dh) - 1)/log(2) * (1 << wF)), j + 2);
	}

	CotranF3Table::CotranF3Table(Target* target, int wF, int j) :
		Table(target, 
				addrLen(wF, j),
				dataLen(wF, j),
				begin(wF, j),
				end(wF, j)),
		wF(wF), j(j), k(int(log(wF+1)/log(2)+1))
	{
		ostringstream name;
		name << "CotranF3Table_" << wF<< "_" << j; 
		uniqueName_ = name.str(); 
		dh = 1. / (1 << (wF - j));

		setCombinatorial();	
	
	}

	double sb2(double z)
	{
		return log(1 + pow(2., z))/log(2);
	}

	double CotranF3Table::SbArg(int z)
	{
		// Direct translation from a Perl script from a few years ago.
		// No more idea how it works, but supposed to work.
		// TODO: make sure it actually works
		double zh = (floor((double)z / (1 << j))) * (1 << j) / (1 << wF);
		double zl = (double)(z % (1 << j)) / (1 << wF);
		double f1 = db2(-zh - dh);
		double f2 = db2(zl - dh);
		return f2 - ((double)z / (1 << wF) + f1);
	}

	mpz_class CotranF3Table::function(int x)
	{
		int y = sign_ext(x, wIn);
		double sbarg = SbArg(y);
		int decval = (int)rint(sb2(sbarg) * (1 << wF));

		// keep only low-order bits (2's-complement with wOut bits)
		//	decval &= ((1 << wOut) - 1);
		//	cout << "x=" << x << ", val=" << decval << endl;
		return mpz_class(decval);
	}

}
