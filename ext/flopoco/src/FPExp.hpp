/*
  An FP exponential for FloPoCo
  
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

*/
#ifndef __FPEXP_HPP
#define __FPEXP_HPP
#include <vector>
#include <sstream>

#include "Operator.hpp"
#include "Table.hpp"
#include "DualTable.hpp"

class Fragment;


namespace flopoco{


	class FPExp : public Operator
	{
	public:

		/** The magic dual table, that holds either (e^A, e^Z-1) or (e^A, e^Z-Z-1) 
		       |.....e^A....||...expZpart.....|
		       <--sizeExpA--><--sizeExpZPart-->
*/
		       
		class magicTable: public DualTable {
		public:
			magicTable(Target* target, int sizeExpA_, int sizeExpZPart_, bool storeExpZmZm1_);
			mpz_class function(int x);
			int sizeExpA;
			int sizeExpZPart; /** sometimes e^Z-1, sometimes e^Z-Z-1*/
			bool storeExpZmZm1;
		};


		class ExpYTable: public Table {
		public:
			ExpYTable(Target* target, int wIn, int wOut);
			mpz_class function(int x);
		};




		/** The constructor with manual control of all options
		    * @param wE exponent size
		    * @param wF fraction size
		    * @param k size of the input to the first table 
		    * @param d  degree of the polynomial approximation (if k=d=0, the constructor tries to compute sensible values)
		    * @param guardBits number of gard bits. If -1, a default value (that depends of the size)  is computed inside the constructor.  
		    * @param fullInput boolean, if true input mantissa is of size wE+wF+1, so that input shift doesn't padd it with 0s (useful for FPPow)
		    * @param DSP_threshold is the DSP use threshold to be used by all the internal multipliers. Formal definition in IntMultiplier.hpp   
		    */

		FPExp(Target* target, int wE, int wF, int k, int d, int guardBits=-1, bool fullInput=false, float DSP_threshold=0.7f,  map<string, double> inputDelays = emptyDelayMap);
		~FPExp();
		
		// Overloading the virtual functions of Operator
		// void outputVHDL(std::ostream& o, std::string name);
		
		void emulate(TestCase * tc);
		void buildStandardTestCases(TestCaseList* tcl);
		TestCase* buildRandomTestCase(int i);

	private:
		int wE; /**< Exponent size */
		int wF; /**< Fraction size */
		int k;  /**< Size of the address bits for the first table  */
		int d;  /**< Degree of the polynomial approximation */
		int g;  /**< Number of guard bits */
		float DSPThreshold;
	};

}
#endif
