#ifndef SHIFTADDDAG_HPP
#define SHIFTADDDAG_HPP
#include <vector>
#include <sstream>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "ShiftAddOp.hpp"
#include "../Operator.hpp"


namespace flopoco{

	class IntConstMult;
	class ShiftAddOp;

	class ShiftAddDag {
	public:
		IntConstMult* icm;
		vector<ShiftAddOp*> saolist;  // the shift-and-add operators
		ShiftAddOp* PX;
	
		ShiftAddDag(IntConstMult* icm) : icm(icm) {
			//initialize with  the  ShiftAddOp that computes X
			PX = new ShiftAddOp(this, X);  
		};
	
		~ShiftAddDag() {
			delete PX;
			/* TODO with an iterator for (int i=0; i<sao.size; i++) delete sao[i];*/};
	
		// This method looks up in the current Dag if the required op
		// exists, and either returns a pointer to it, or creates the
		// corresponding node.
		ShiftAddOp* provideShiftAddOp(ShiftAddOpType op, ShiftAddOp* i, int s, ShiftAddOp* j=NULL);

		mpz_class computeConstant(ShiftAddOpType op, ShiftAddOp* i, int s, ShiftAddOp* j);


		ShiftAddOp* result; 

	};
}

#endif
