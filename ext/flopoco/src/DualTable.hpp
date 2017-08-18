#ifndef DUALTABLE_HPP
#define DUALTABLE_HPP
#include <gmpxx.h>

#include "Operator.hpp"
#include "utils.hpp"
#include <math.h>
#include <sstream>
#include <cstdlib>
#include <iostream>


namespace flopoco{

	/** A basic hardware look-up table for FloPoCo. 

		 If the input to your table are negative, etc, or if you want to
		 define errors, or... then derive a class from this one.

	*/

	using namespace std;

	class DualTable : public Operator
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
		
		Target *target;
 
		/**
		 * The Table constructor
		 * @param[in] target the target device
		 * @param[in] wIn    the with of the input in bits
		 * @param[in] wOut   the with of the output in bits  
		 **/
		DualTable(Target* target, int _wIn, int _wOut, int _minIn=0, int _maxIn=-1);
		
		DualTable(Target* target);

		virtual ~DualTable() {};

	
		/** The function that will define the values contained in the table
		 * @param[in] x  input to the table, an integer value between minIn and maxIn
		 * @return    an mpz integer  between 0 and 2^wOut-1 
		 */
	
		virtual mpz_class function(int x) =0;


		/** Do the useful work */
		void fillTable();

		/** overload the virtual method of Operator to have access to the virtual function() */
		void outputVHDL(std::ostream& o, std::string name);

		/** A function that translates an real value into an integer input
			 This function should be overridden by an implementation of Table
		*/
		virtual int    double2input(double x);

		/** A function that translates an integer input value into its real semantics
			 This function should be overridden by an implementation of Table
		*/
		virtual double input2double(int x);

		/** A function that translates an real value into an integer output
			 This function should be overridden by an implementation of Table
		*/
		virtual  mpz_class double2output(double x);

		/** A function that translates an integer output value into its real semantics
			 This function should be overridden by an implementation of Table
		*/
		virtual double output2double(mpz_class x);
	
		/** A function that returns an estimation of the size of the table in LUTs. Your mileage may vary thanks to boolean optimization */
		int size_in_LUTs();
	

	private:		
	
		/**
			The inner class of the DualTable which is the fundamental entity which is recognized as a dual port memory.
		*/
		class primitiveDualMemory: public Operator{
		
		public:
		
			/** Input width (in bits)*/
			int wIn;

			/** Output width (in bits)*/
			int wOut;

			/** minimal input value (default 0) */
			int minIn; 

			/** maximal input value (default 2^wIn-1) */
			int maxIn; 
	
			stringstream inputData;
		
			void setInputData(std::ostringstream&  data);				
			primitiveDualMemory(Target* target, int _wIn, int _wOut, int _minIn, int _maxIn);
			~primitiveDualMemory() {}
		
		
		private:
			
		
		};


		bool full; /**< true if there is no "don't care" inputs, i.e. minIn=0 and maxIn=2^wIn-1 */
		int nrOfMemBlocks;
		int limitSingleMemory ;
	
		primitiveDualMemory * primitiveBlock;
		primitiveDualMemory ** primitiveBlocks ;
	
	};

}
#endif
