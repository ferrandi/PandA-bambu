#ifndef __IEEENUMBER_HPP
#define __IEEENUMBER_HPP

#include <gmpxx.h>
#include <mpfr.h>


namespace flopoco{

	/**
	 * An abstraction of IEEE FP numbers. Defines an
	 * abstraction on which arithmetic operations can easily be applied
	 * but at the same times can easily be converted to VHDL signals.
	 * Used for TestBench generation.
	 */
	class IEEENumber
	{
	public:

		/** Several possible special values */
		typedef enum {
			plusInfty,     /**< A positive infinity with random non-zero exponent and fraction bits  */
			minusInfty,    /**< A negative infinity with random non-zero exponent and fraction bits  */
			plusZero,      /**< A positive zero */
			minusZero,     /**< A negative zero  */
			NaN            /**< Not A Number */
		} SpecialValue;

		/**
		 * Constructs a new IEEENumber.
		 * @param wE the width of the exponent
		 * @param wF the width of the significant
		 */
		IEEENumber(int wE, int wF);

		/**
		 * Constructs a new IEEENumber.
		 * @param wE the width of the exponent
		 * @param wF the width of the significant
		 * @param v a special value
		 */
		IEEENumber(int wE, int wF, SpecialValue v);

		/**
		 * Constructs a new initialised IEEENumber.
		 * @param wE the width of the exponent
		 * @param wF the width of the significant
		 * @param m the initial value.
		 */
		IEEENumber(int wE, int wF, mpfr_t m);

		/**
		 * Retrieves the significant.
		 * @return Returns an mpz_class, representing the
		 * VHDL signal of the mantissa, without leading 1.
		 */
		mpz_class getMantissaSignalValue();



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
		 * Converts the currently stored IEEENumber to an mpfr_t
		 * @param[out] m a preinitialized mpfr_t where to store the floating point
		 */
		void getMPFR(mpfr_t m);

		/**
		 * Stores an mpfr_t as an internal representation of Flopoco.
		 * @param m the mpfr_t to store.
		 */
		IEEENumber &operator=(mpfr_t m);

		/**
		 * Assignes a signal value. Converts the signal value to the
		 * relevant IEEENumber fields.
		 * @param s the signal value to assign.
		 */
		IEEENumber &operator=(mpz_class s);

		/**
		 * Retrieved the VHDL signal representation of this floating point.
		 * @return a VHDL signal stored as mpz_class.
		 */
		mpz_class getSignalValue();

		/**
		 * Equality operator. Everything does through MPFR to make sure
		 * correct rounding occurs.
		 */
		IEEENumber &operator=(IEEENumber fp);



		/**
		 * Returns wE and wF.
		 * @param[out] wE exponent precision
		 * @param[out] wF fraction precision
		 */
		void getPrecision(int &wE, int &wF);

		/**
		 * Assigns a double.
		 */
		IEEENumber &operator=(double x);


	private:
		/** The width of the exponent */
		int wE;

		/** The width of the significant (without leading zero) */
		int wF;

		/** The value of the sign field */
		mpz_class sign;

		/** The value of the exponent field */
		mpz_class exponent;

		/** The value of the mantissa field  */
		mpz_class mantissa;

	};

}

#endif

