#ifndef DualTable2_HPP
#define DualTable2_HPP
#include <gmpxx.h>

#include "Operator.hpp"

/** A basic hardware look-up DualTable2 for FloPoCo. 

	 If the input to your DualTable2 are negative, etc, or if you want to
	 define errors, or... then derive a class from this one.

*/


namespace flopoco{


	class DualTable2 : public Operator
	{
	public:

		/** Input width (in bits)*/
		int wIn;

		/** Output width (in bits)*/
		int wOut;

		/** minimal input value (default 0) */
		int minIn; 

		/** maximal input value (default 2^wIn-1) */
		int maxIn; 
	
		/**
		 * The DualTable2 constructor
		 * @param[in] target the target device
		 * @param[in] wIn    the with of the input in bits
		 * @param[in] wOut   the with of the output in bits  
		 **/
		DualTable2(Target* target, int _wIn, int _wOut, int _minIn=0, int _maxIn=-1);

    DualTable2(Target* target);
     
		virtual ~DualTable2() {};



		/** The function that will define the values contained in the DualTable2
		 * @param[in] x  input to the DualTable2, an integer value between minIn and maxIn
		 * @return    an mpz integer  between 0 and 2^wOut-1 
		 */
		virtual mpz_class function(int x) =0;


		/** Overloading the method of Operator */
		void outputVHDL(ostream& o, string name);

		/** A function that translates an real value into an integer input.
			 This function should be overridden by an implementation of DualTable2.
			 It is optional.
		*/
		virtual int    double2input(double x);

		/** A function that translates an integer input value into its real semantics
			 This function should be overridden by an implementation of DualTable2.
			 It is optional.
		*/
		virtual double input2double(int x);

		/** A function that translates an real value into an integer output
			 This function should be overridden by an implementation of DualTable2
			 It is optional.
		*/
		virtual  mpz_class double2output(double x);

		/** A function that translates an integer output value into its real semantics
			 This function should be overridden by an implementation of DualTable2
			 It is optional.
		*/
		virtual double output2double(mpz_class x);
	
#if 0 // TODO some day
		/** A function that translates an real value into an integer output
			 This function should be overridden by an implementation of DualTable2
			 It is optional.
		*/
		virtual  mpz_class mpfr2output(double x);

		/** A function that translates an integer output value into its real semantics
			 This function should be overridden by an implementation of DualTable2
			 It is optional.
		*/
		virtual double output2mpfr(mpz_class x);
	
#endif

		/** A function that returns an estimation of the size of the DualTable2 in LUTs. Your mileage may vary thanks to boolean optimization */
		int size_in_LUTs();
	private:
		bool full; /**< true if there is no "don't care" inputs, i.e. minIn=0 and maxIn=2^wIn-1 */
	};

}
#endif
