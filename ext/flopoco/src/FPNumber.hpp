#ifndef __FPNumber_HPP
#define __FPNumber_HPP

#include <gmpxx.h>
#include <mpfr.h>


namespace flopoco{

	/**
	 * Flopoco internal Floating Point. Defines an
	 * abstraction on which arithmetic operations can easily be applied
	 * but at the same times can easily be converted to VHDL signals.
	 * Used for TestBench generation.
	 */
	class FPNumber
	{
	public:

		/** Several possible special values */
		typedef enum {
			plusInfty,                   /**< A positive infinity with random non-zero exponent and fraction bits  */
			minusInfty,                  /**< A negative infinity with random non-zero exponent and fraction bits  */
			plusDirtyZero,               /**< A zero with non-zero exponent and fraction bits */
			minusDirtyZero,              /**< A zero with non-zero exponent and fraction bits */
			NaN,                         /**< A NaN */
			largestPositive,                 /**< The largest positive FPNumber  */
			smallestPositive,                 /**< The smallest positive FPNumber  */
			largestNegative,                 /**< The largest (in magnitude) negative FPNumber  */
			smallestNegative                  /**< The smallest (in magnitude) negative FPNumber*/
		} SpecialValue;

		/**
		 * Constructs a new FPNumber.
		 * @param wE the width of the exponent
		 * @param wF the width of the significant
		 */
		FPNumber(int wE, int wF);

		/**
		 * Constructs a new FPNumber.
		 * @param wE the width of the exponent
		 * @param wF the width of the significant
		 * @param v a special value
		 */
		FPNumber(int wE, int wF, SpecialValue v);

		/**
		 * Constructs a new initialised FPNumber.
		 * @param wE the width of the exponent
		 * @param wF the width of the significant
		 * @param m the initial value.
		 */
		FPNumber(int wE, int wF, mpfr_t m);

		/**
		 * Constructs a new initialised FPNumber.
		 * @param wE the width of the exponent
		 * @param wF the width of the significant
		 * @param z the initial value, given as an mpz holding the bits of the FPNumber.
		 */
		FPNumber(int wE, int wF, mpz_class z);

		/**
		 * Retrieves the significant.
		 * @return Returns an mpz_class, representing the
		 * VHDL signal of the mantissa, without leading 1.
		 */
		mpz_class getMantissaSignalValue();

		/**
		 * Retrieves the fraction.
		 * @return An mpz_class, representing the VHDL
		 * signal of the mantissa, plus leading 1.
		 */
		mpz_class getFractionSignalValue();

		/**
		 * Retrieves the two exception bits.
		 * @return the two exception bits as VHDL signals.
		 */
		mpz_class getExceptionSignalValue();

		/**
		 * Retrives the sign.
		 * @return the sign as a VHDL signal.
		 */
		mpz_class getSignSignalValue();

		/**
		 * Retrieves the exponent.
		 * @return the exponent as a VHDL signal.
		 */
		mpz_class getExponentSignalValue();


		/**
		 * Converts the currently stored FPNumber to an mpfr_t
		 * @param[out] m a preinitialized mpfr_t where to store the floating point
		 */
		void getMPFR(mpfr_t m);

		/**
		 * converts an mpfr_t into an FPNumber.
		 * @param m the mpfr_t to convert.
		 */
		FPNumber &operator=(mpfr_t m);



		/**
		 * Assignes a signal value. Converts the signal value to the
		 * relevant FPNumber fields.
		 * @param s the signal value to assign.
		 */
		FPNumber &operator=(mpz_class s);

		/**
		 * Retrieved the VHDL signal representation of this floating point.
		 * @return a VHDL signal stored as mpz_class.
		 */
		mpz_class getSignalValue();

		/**
		 * Equality operator. Everything does through MPFR to make sure
		 * correct rounding occurs.
		 */
		FPNumber &operator=(FPNumber fp);



		/**
		 * Returns wE and wF.
		 * @param[out] wE exponent precision
		 * @param[out] wF fraction precision
		 */
		void getPrecision(int &wE, int &wF);

		/**
		 * Assigns a double.
		 */
		FPNumber &operator=(double x);

		/**
		 * Changes this FPNumber, so that it is decremented with
		 * one ULP. Postfix form.
		 */
		FPNumber &operator--(int);

		/**
		 * Changes this FPNumber, so that it is incremented with
		 * one ULP. Postfix form.
		 */
		FPNumber &operator++(int);

		/**
		 * Changes this FPNumber, so that it is decremented with
		 * the given number of ULPs.
		 * @param x how many ULPs to decrement it with.
		 */
		FPNumber &operator-=(int x);

		/**
		 * Changes this FPNumber, so that it is incremented with
		 * the given number of ULPs.
		 * @param x how many ULPs to increment it with.
		 */
		FPNumber &operator+=(int x);

	private:
		/** The width of the exponent */
		int wE;

		/** The width of the significant (without leading zero) */
		int wF;

		/** The value of the sign */
		mpz_class sign;
	
		/** The value of the exception */
		mpz_class exception;

		/** The value of the exponent */
		mpz_class exponent;

		/** The value of the mantissa (without leading one) */
		mpz_class mantissa;

	};

}

#endif

