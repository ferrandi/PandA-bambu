#ifndef PolyCoeffTable_HPP
#define PolyCoeffTable_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include <cstdlib>

#include "../Operator.hpp"
#include "../Table.hpp"
#include "PolynomialEvaluator.hpp"

#include "Function.hpp"
#include "PiecewiseFunction.hpp"
#include "HOTBM/MPPolynomial.hh"
#include "../UtilSollya.hh"

namespace flopoco{

	/** The PolyCoeffTable class.  */
	class PolyCoeffTable : public Table {

	public:
		PolyCoeffTable(Target* target, PiecewiseFunction* pf,  int wOutX, int n);
		PolyCoeffTable(Target* target, string func,  int wOutX, int n);
		/* TODO: Doxygen parameters*/ 

		/**
		 * PolyCoeffTable destructor
		 */
		~PolyCoeffTable();
			
		MPPolynomial* getMPPolynomial(sollya_node_t t);
		vector<FixedPointCoefficient*> getPolynomialCoefficients(sollya_node_t t, sollya_chain_t c);
		vector<FixedPointCoefficient*> getPolynomialCoefficients(sollya_node_t t, int* sizeList);
		vector<vector<FixedPointCoefficient*> > getPolynomialCoefficientsVector();
		void printPolynomialCoefficientsVector();
		void updateMinWeightParam(int i, FixedPointCoefficient* zz);
		vector<FixedPointCoefficient*> getCoeffParamVector();
		void printCoeffParamVector();
		mpfr_t *getMaxApproxError();
		void generateDebug();
		void generateDebugPwf();
		sollya_chain_t makeIntPtrChainCustomized(int m, int n, int precshift, int msize);
		vector<int> getNrIntArray();

		/************************************************/
		/********Virtual methoods from class Table*******/
		mpz_class function(int x);

		int    double2input(double x);
		double input2double(int x);
		mpz_class double2output(double x);
		double output2double(mpz_class x);
		/************************************************/
	protected:
		void buildActualTable();
		int wOutX_;  /**< Output precision required from this polynomial. The output interval is assumed to be [0,1], so wOutX will actually determine all the coefficient sizes */
		Function *f;
		vector< vector<FixedPointCoefficient*> > polyCoeffVector;
		vector<FixedPointCoefficient*> coeffParamVector; /**< This is a vector of coefficient parameters: for each degree, the size and weight of the corresponding coeff */
		mpfr_t *maxError;
		PiecewiseFunction *pwf;
		vector <int> nrIntervalsArray; /**< A vector containing as many entries as functions (size is usually 1, but 2 for the polynomials used in FPSqrtPoly). Each entry is an integer that gives the number of subintervals in which the corresponding function has been split. */
		vector <mpz_class> actualTable; /**< The final compact coefficient table: one entry per polynomial/interval, each entry is the concatenation of the bit vectors of all the coefficients.*/
	};
}
#endif
