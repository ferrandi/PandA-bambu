/*
  The floating-point numbers used in for FloPoCo
 
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author: Cristian Klein, Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

 */

#include "FPNumber.hpp"
#include "utils.hpp"


namespace flopoco{


	FPNumber::FPNumber(int wE, int wF)
		: wE(wE), wF(wF)
	{
		if (wE > 30)
			throw std::string("FPNumber::FPNumber: Using exponents larger than 30 bits is not supported.");
	}

	FPNumber::FPNumber(int wE, int wF, mpfr_t m)
		: wE(wE), wF(wF)
	{
		if (wE > 30)
			throw std::string("FPNumber::FPNumber: Using exponents larger than 30 bits is not supported.");
		operator=(m);
	}


	FPNumber::FPNumber(int wE, int wF, mpz_class z)
		: wE(wE), wF(wF)
	{
		if (wE > 30)
			throw std::string("FPNumber::FPNumber: Using exponents larger than 30 bits is not supported.");
		operator=(z);
	}

	FPNumber::FPNumber(int wE, int wF, SpecialValue v)	
		: wE(wE), wF(wF)
	{
		if (wE > 30)
			throw std::string("FPNumber::FPNumber: Using exponents larger than 30 bits is not supported.");
		switch(v)  {
		case plusInfty: 
			exception = 2;
			sign = 0;
			exponent = getLargeRandom(wE);
			mantissa = getLargeRandom(wF);
			break;
		case minusInfty: 
			exception = 2;
			sign = 1;
			exponent = getLargeRandom(wE);
			mantissa = getLargeRandom(wF);
			break;
		case plusDirtyZero: 
			exception = 0;
			sign = 0;
			exponent = getLargeRandom(wE);
			mantissa = getLargeRandom(wF);
			break;
		case minusDirtyZero: 
			exception = 0;
			sign = 1;
			exponent = getLargeRandom(wE);
			mantissa = getLargeRandom(wF);
			break;
		case NaN: 
			exception = 3;
			sign = getLargeRandom(1);
			exponent = getLargeRandom(wE);
			mantissa = getLargeRandom(wF);
			break;
		case largestPositive: 
			exception = 1;
			sign = 0;
			exponent = (1<<wE)-1;
			mantissa = (1<<wF)-1;
			break;
		case smallestPositive: 
			exception = 1;
			sign = 0;
			exponent = 0;
			mantissa = 0;
			break;
		case largestNegative: 
			exception = 1;
			sign = 1;
			exponent = (1<<wE)-1;
			mantissa = (1<<wF)-1;
			break;
		case smallestNegative: 
			exception = 1;
			sign = 1;
			exponent = 0;
			mantissa = 0;
			break;
		}
	}


	mpz_class FPNumber::getMantissaSignalValue()
	{
		return mantissa;
	}

	mpz_class FPNumber::getExceptionSignalValue() { return exception; }

	mpz_class FPNumber::getSignSignalValue() { return sign; }
	mpz_class FPNumber::getExponentSignalValue()
	{
		return exponent;
	}

	mpz_class FPNumber::getFractionSignalValue()
	{
		return mantissa + (mpz_class(1)<<wF);
	}



	void FPNumber::getMPFR(mpfr_t mp)
	{

		/* NaN */
		if (exception == 3)
			{
				mpfr_set_nan(mp);
				return;
			}

		/* Infinity */
		if (exception == 2)	{
			mpfr_set_inf(mp, (sign == 1) ? -1 : 1);
			return;
		}

		/* Zero */
		if (exception == 0)	{
			mpfr_set_d(mp, (sign == 1) ? -0.0 : +0.0, GMP_RNDN);
			return;
		}
	
		/* „Normal” numbers
		 * mp = (-1) * (1 + (mantissa / 2^wF)) * 2^unbiased_exp
		 * unbiased_exp = exp - (1<<(wE-1)) + 1
		 */
		mpfr_set_prec(mp, wF+2);
		mpfr_set_z(mp, mantissa.get_mpz_t(), GMP_RNDN);
		mpfr_div_2si(mp, mp, wF, GMP_RNDN);
		mpfr_add_ui(mp, mp, 1, GMP_RNDN);
	
		mp_exp_t exp = exponent.get_si();
		exp -= ((1<<(wE-1))-1);
		mpfr_mul_2si(mp, mp, exp, GMP_RNDN);

		/* Sign */
		if (sign == 1)
			mpfr_neg(mp, mp, GMP_RNDN);
	}

	FPNumber& FPNumber::operator=(mpfr_t mp_)
	{
		mpfr_t mp;
		mpfr_init2(mp, mpfr_get_prec(mp_));
		mpfr_set(mp, mp_, GMP_RNDN);

		/* NaN */
		if (mpfr_nan_p(mp))
			{
				exception = 3;
				sign = 0;
				exponent = 0;
				mantissa = 0;
				return *this;
			}

		/* Inf */
		if (mpfr_inf_p(mp))
			{
				exception = 2;
				sign = mpfr_sgn(mp) > 0 ? 0 : 1;
				exponent = 0;
				mantissa = 0;
				return *this;
			}

		/* Zero */
		if (mpfr_zero_p(mp))
			{
				exception = 0;
				sign = mpfr_signbit(mp) == 0 ? 0 : 1;
				exponent = 0;
				mantissa = 0;
				return *this;
			}

		/* Normal numbers */
		exception = 1;
		sign = mpfr_sgn(mp) > 0 ? 0 : 1;
		mpfr_abs(mp, mp, GMP_RNDN);

		/* Get exponent
		 * mpfr_get_exp() return exponent for significant in [1/2,1)
		 * but we require [1,2). Hence the -1.
		 */
		mp_exp_t exp = mpfr_get_exp(mp)-1;

		/* Extract mantissa */
		mpfr_div_2si(mp, mp, exp, GMP_RNDN);
		mpfr_sub_ui(mp, mp, 1, GMP_RNDN);
		mpfr_mul_2si(mp, mp, wF, GMP_RNDN);
		mpfr_get_z(mantissa.get_mpz_t(), mp,  GMP_RNDN);


		// Due to rounding, the mantissa might overflow (i.e. become bigger
		// then we expect).
		if (mantissa == mpz_class(1) << wF)
			{
				exp++;
				mantissa = 0;
			}

		if (mantissa >= mpz_class(1) << wF)
			throw std::string("Mantissa is too big after conversion to VHDL signal.");
		if (mantissa < 0)
			throw std::string("Mantissa is negative after conversion to VHDL signal.");

		/* Bias and store exponent */
		exp += ((1<<(wE-1))-1);
		exponent = exp;

		/* Handle underflow */
		if (exponent < 0)
			{
				exception = 0;
				exponent = 0;
				mantissa = 0;
			}

		/* Handle overflow */
		if (exponent >= (1<<(wE)))
			{
				exception = 2;
				exponent = 0;
				mantissa = 0;
			}

		mpfr_clear(mp);

		return *this;
	}

	FPNumber& FPNumber::operator=(mpz_class s)
	{
		mantissa = s & ((mpz_class(1) << wF) - 1); s = s >> wF;
		exponent = s & ((mpz_class(1) << wE) - 1); s = s >> wE;
		sign = s & mpz_class(1); s = s >> 1;
		exception = s & mpz_class(3); s = s >> 2;

		if (s != 0)
			throw std::string("FPNumber::operator= s is bigger than expected.");

		return *this;
	}

	mpz_class FPNumber::getSignalValue()
	{

		/* Sanity checks */
		if ((sign != 0) && (sign != 1))
			throw std::string("FPNumber::getSignal: sign is invalid.");
		if ((exception < 0) || (exception > 3))
			throw std::string("FPNumber::getSignal: exception is invalid.");
		if ((exponent < 0) || (exponent >= (1<<wE)))
			throw std::string("FPNumber::getSignal: exponent is invalid.");
		if ((mantissa < 0) || (mantissa >= (mpz_class(1)<<wF)))
			throw std::string("FPNumber::getSignal: mantissa is invalid.");
		return (((((exception << 1) + sign) << wE) + exponent) << wF) + mantissa;
	}

	FPNumber& FPNumber::operator=(FPNumber fp)
	{

		/* Pass this through MPFR to lose precision */
		mpfr_t mp;
		mpfr_init(mp);	// XXX: Precision set in operator=
		fp.getMPFR(mp);
		operator=(mp);
		mpfr_clear(mp);

		return *this;
	}



	void FPNumber::getPrecision(int &wE, int &wF)
	{
		wE = this->wE;
		wF = this->wF;
	}

	FPNumber& FPNumber::operator=(double x)
	{
		mpfr_t mp;
		mpfr_init2(mp, 53);
		mpfr_set_d(mp, x, GMP_RNDN);

		operator=(mp);

		mpfr_clear(mp);

		return *this;
	}

	FPNumber& FPNumber::operator--(int)
	{
		/* Decrementation from special values is hard-coded */
		if (exception == 3)
			{
				if (sign == 0) /* before +NaN follows +inf */
					exception = 2;
				else /* before -NaN follows +NaN */
					sign = 0;
			}
		else if (exception == 2) /* inf */
			{
				if (sign == 0)	/* before +inf comes max */
					{
						exception = 1;
						exponent = (mpz_class(1) << wE) - 1;
						mantissa = (mpz_class(1) << wF) - 1;
					}
				else /* before -inf comes -NaN */
					exception = 3;
			}
		else if (exception == 0) /* zero */
			{
				if (sign == 0)	/* before +0 comes -0 */
					sign = 1;
				else /* before -0 comes -min */
					{
						exception = 1;
						exponent = 0;
						mantissa = 0;
					}
			}
		else /* other numbers */
			{
				/* Combine exception, exponent & mantissa, for nice trick */
				mpz_class tz = (((exception << wE) + exponent) << wF) + mantissa;
				if (sign == 0)
					tz--;
				else
					tz++;
				mantissa = tz & ((mpz_class(1) << wF) - 1); tz = tz >> wF;
				exponent = tz & ((mpz_class(1) << wE) - 1); tz = tz >> wE;
				exception = tz & mpz_class(3);
			}

		return *this;
	}

	FPNumber& FPNumber::operator++(int)
	{
		/* Negate number */
		sign = 1 - sign;
		/* Decrement it */
		operator--(0);
		/* Negate it again */
		sign = 1 - sign;

		return *this;
	}

	FPNumber& FPNumber::operator-=(int x)
	{
		for ( ; x < 0; x++)
			operator++(0);
		for ( ; x > 0; x--)
			operator--(0);
		return *this;
	}

	FPNumber& FPNumber::operator+=(int x)
	{
		return operator-=(-x);
	}

}
