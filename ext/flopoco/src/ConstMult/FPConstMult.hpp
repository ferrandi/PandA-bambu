#ifndef FPCONSTMULT_HPP
#define FPCONSTMULT_HPP
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../Operator.hpp"
#include "IntConstMult.hpp"


namespace flopoco{

	class FPConstMult : public Operator
	{
	public:
		/** The generic constructor */
		FPConstMult(Target* target, int wE_in, int wF_in, int wE_out, int wF_out, int cst_sgn, int cst_exp, mpz_class cst_sig);
		
		/** A constructor for rational constants */
		FPConstMult(Target* target, int wE_in, int wF_in, int wE_out, int wF_out, int a, int b);
		
		/** An empty constructor,  used by CRFPConstMult */
		FPConstMult(Target* target, int wE_in, int wF_in, int wE_out, int wF_out);
		
#ifdef HAVE_SOLLYA
		/** A constructor that parses an expression for the constant */
		FPConstMult(Target* target, int wE_in, int wF_in, int wE_out, int wF_out, int wF_C, string constant);
#endif //HAVE_SOLLYA
		~FPConstMult();

		int wE_in; 
		int wF_in; 
		int wE_out; 
		int wF_out; 
		mpfr_t mpfrC; 
		int cstWidth;
		bool correctRounding;

		int cstSgn;
		int cst_exp_when_mantissa_1_2;
		int cst_exp_when_mantissa_int;
		mpz_class cstIntSig;
		string cst_name;

		mpfr_t cstSig; // between 1 and 2, high accuracy
		mpfr_t mpfr_xcut_sig; // between 1 and 2

		mpz_class xcut_sig_rd; // an int on wF_in+1 bits, which is mpfr_xcut_sig rounded down 

		bool mantissa_is_one; /**< is the mantissa equal to 1? */
		bool constant_is_zero; /**< is the constant equal to 0? */

		IntConstMult *icm;
		int icm_depth;


		/** to avoid code duplication. */
		void setupSgnAndExpCases();

		void computeExpSig();

		void computeIntExpSig();

		void computeXCut();

		void normalizeCst();


		/** The method that declares all the signals and sets up the pipeline.
			 It is called by the constructors of FPConstMult and CRFPConstMult to avoid code duplication */
		void buildVHDL();

		void emulate(TestCase *tc);

		/* The value of the constant multiplicand */
		mpfr_t mpY;

		void fillTestCase(mpz_class a[]);
	};

}
#endif
