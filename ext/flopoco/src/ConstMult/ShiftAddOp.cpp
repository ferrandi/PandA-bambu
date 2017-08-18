/*
  Shift-and-add operators for integer constant multiplication

  This file is part of the FloPoCo project 
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author: Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, 2008-2010.
  All right reserved.

*/

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "IntConstMult.hpp"
#include "ShiftAddOp.hpp"
#include "ShiftAddDag.hpp"

using namespace std;


namespace flopoco{


	ShiftAddOp::ShiftAddOp(ShiftAddDag* impl, ShiftAddOpType op, ShiftAddOp* i, int s, ShiftAddOp* j) :
		impl(impl), op(op), i(i), j(j), s(s) {
		n=impl->computeConstant(op, i, s, j);
		// now compute the size according to this constant, as the log2 of the max value the result can take
		if (n >= 0)
			size = intlog2(n * ((mpz_class(1)<<impl->icm->xsize)-1));
		else
			size = 1 + intlog2(-n* ((mpz_class(1)<<impl->icm->xsize)-1));  // we need a sign bit
		if(n==1)
			size=impl->icm->xsize;

		// compute the cost in terms of full adders of this node
		switch(op) {
		case X:
			cost_in_full_adders = 0;   break;
		case Add:      
			if (s >= j->size) // no overlap of bits of Vi<<s and Vj
				cost_in_full_adders = 0;
			else
				cost_in_full_adders = size - s - 1; // -1 because the cout bit is for free    
			break;
		case Sub:      
			cost_in_full_adders = size - 1; // -1 because the cout bit is for free    
			break;
		case RSub:      
			if (s >= j->size) // no overlap of bits of Vi<<s and Vj
				cost_in_full_adders = i->size - 1; // still need to negate i
			else
				cost_in_full_adders = size - s - 1; // -1 because the cout bit is for free    
			break;
		case Shift:    cost_in_full_adders = 0; 
			break;
		case Neg:      cost_in_full_adders = size -1; // -1 because the cout bit is for free
			break;
		}   
	
		// build the variable name
		ostringstream o;
		
		if(n==1)         o <<"X"; 
		else  if(n>=0)   o<<"P"<<mpz2string(n)<<"X";  
		else             o<<"M"<<mpz2string(-n)<<"X";
		name =  o.str();


		// and add this object to the dictionary of the implementation
		if (op!=X)
			impl->saolist.push_back(this);

	}


}
