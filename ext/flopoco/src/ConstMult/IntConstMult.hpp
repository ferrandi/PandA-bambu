#ifndef INTCONSTMULT_HPP
#define INTCONSTMULT_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>

#include "../Operator.hpp"
#include "ShiftAddOp.hpp"
#include "ShiftAddDag.hpp"
#include "../IntAdder.hpp"

/**
	Integer constant multiplication.

	See also ShiftAddOp, ShiftAddDag
	ShiftAddOp defines a shift-and-add operation for IntConstMult.

	ShiftAddDag deals with the implementation of an IntConstMult as a
	vector of ShiftAddOp. It defines the intermediate variables with
	their bit sizes and provide methods for evaluating the cost of an
	implementation.

*/


namespace flopoco{

	class IntConstMult : public Operator
	{
	public:
		/** The standard constructor, inputs the number to implement */ 
		IntConstMult(Target* target, int xsize, mpz_class n);

		/** A constructor for constants defined as a header and a period (significands of rational constants).
		    The actual periodic pattern is given as (period << periodMSBZeroes)
				Parameters i and j are such that the period must be repeated 2^i + 2^j times. 
				If j==-1, just repeat the period 2^i times
		 */
		IntConstMult(Target* _target, int _xsize, mpz_class n, 
		             mpz_class period, int periodMSBZeroes, int periodSize, 
		             mpz_class header, int headerSize, 
		             int i, int j);

		~IntConstMult();

		mpz_class n;  /**< The constant */ 
		int xsize;   
		int rsize;   
		ShiftAddDag* implementation;




		// Overloading the virtual functions of Operator

		void emulate(TestCase* tc);
		void buildStandardTestCases(TestCaseList* tcl);

		/** Recodes input n; returns the number of non-zero bits */
		int recodeBooth(mpz_class n, int* BoothCode);

		// void buildMultBooth();      /**< Build a rectangular (low area, long latency) implementation */
		ShiftAddOp* buildMultBoothTree(mpz_class n);  /**< Build a balanced tree implementation as per the ASAP 2008 paper */ 

		/** Build an optimal tree for rational constants
		 Parameters are such that n = headerSize + (2^i + 2^j)periodSize */ 
		void buildTreeForRational(mpz_class header, mpz_class period, int headerSize, int periodSize, int i, int j);  

	private:
		void build_pipeline(ShiftAddOp* sao, double& delay);
		string printBoothCode(int* BoothCode, int size);
		void showShiftAddDag();
		void optimizeLefevre(const vector<mpz_class>& constants);

	};
}
#endif
