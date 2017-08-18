#ifndef _FIXSINCOS_H
#define _FIXSINCOS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// works only with Sollya
#ifdef HAVE_SOLLYA

#include <gmpxx.h>

#include "Operator.hpp"

#include "utils.hpp"

using namespace flopoco;




class FixSinCos: public Operator {
public:
	class SinCosTable: public Table {
	public:
		/**< Builds a table returning  sin(Addr) and cos(Addr) accurate to 2^(-lsbOut-g)
			 (beware, lsbOut is positive) where Addr is on a bits. 
			 If g is not null, a rounding bit is added at weight 2^((-lsbOut-1)
			 argRedCase=1 for full table, 4 for quadrant reduction, 8 for octant reduction.
			 Result is signed if argRedCase=1 (msbWeight=0=sign bit), unsigned positive otherwise (msbWeight=-1).
			 */
		SinCosTable(Target* target, int a, int lsbOut, int g, int argRedCase_, FixSinCos* parent);
		~SinCosTable();
	private:
		mpz_class function(int x);
		int lsbOut;
		int g;
		int argRedCase;     /**< argRedCase=1 for full table, 4 for quadrant, 8 for octant reduction */
		FixSinCos* parent;
	};


public:
	
	FixSinCos(Target * target, int w, float ratio=0.5);
	
	~FixSinCos();
	
	
	void emulate(TestCase * tc);
	
	void buildStandardTestCases(TestCaseList * tcl);
	

private:
	int w;
	mpfr_t scale;              /**< 1-2^(wOut-1)*/
	mpfr_t constPi;
	SinCosTable* scT;
	Operator *pi_mult; // may be a FixRealKCM or an IntConstMult
};

#endif // HAVE_SOLLYA

#endif // header guard

