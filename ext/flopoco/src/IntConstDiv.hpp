/*
  Integer division by a small constant.
  
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

*/
#ifndef __INTCONSTDIV_HPP
#define __INTCONSTDIV_HPP
#include <vector>
#include <sstream>

#include "Operator.hpp"
#include "Target.hpp"
#include "Table.hpp"


namespace flopoco{


	class IntConstDiv : public Operator
	{
	public:

		/** This table inputs a number X on alpha + gammma bits, and computes its Euclidean division by d: X=dQ+R, which it returns as the bit string Q & R */ 
		class EuclideanDivTable: public Table {
		public:
			int d;
			int alpha;
			int gamma;
			EuclideanDivTable(Target* target, int d_, int alpha_, int gamma_);
			mpz_class function(int x);
		};




		/** The constructor 
		    * @param d The divisor.
		    * @param n The size of the input X.
		    * @param alpha The size of the chunk, or, use radix 2^alpha
		    */

		IntConstDiv(Target* target, int wIn, int d, int alpha=-1, bool remainderOnly=false, map<string, double> inputDelays = emptyDelayMap);
		~IntConstDiv();
		
		// Overloading the virtual functions of Operator
		// void outputVHDL(std::ostream& o, std::string name);
		
		void emulate(TestCase * tc);

	public:
		int quotientSize();   /**< Size in bits of the quotient output */
		int remainderSize();  /**< Size in bits of a remainder; gamma=ceil(log2(d-1)) */

	private:
		int d; /**<  Divisor*/
		int wIn;  /**<  Size in bits of the input X */
		int alpha; /**< Size of the chunk (should be between 1 and 16)*/
		bool remainderOnly; /**< if true, only the remainder will be computed. If false, quotient will be computed */
		int gamma;  /**< Size in bits of a remainder; gamma=ceil(log2(d-1)) */
		int qSize;   /**< Size in bits of the quotient output */
		
	};

}
#endif
