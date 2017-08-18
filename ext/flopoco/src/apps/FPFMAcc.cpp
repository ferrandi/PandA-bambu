/*
  Floating-Point Fused Multiply Accumulate unit for FloPoCo

  This file is part of the FloPoCo project 
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author :   Bogdan Pasca

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <string.h>
#include <gmp.h>
#include <mpfr.h>
#include <cstdlib>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"

#include "FPFMAcc.hpp"
#include "../IntMultiplier.hpp"


using namespace std;

namespace flopoco{

	FPFMAcc::FPFMAcc(Target* target, int wE, int wF, int adderLatency):
		Operator(target), wE(wE), wF(wF) {
		srcFileName="FPFMAcc1";
		ostringstream name;

		name <<"FPFMAcc_"<<wE<<"_"<<wF<<"_f"<<target->frequencyMHz()<<"_uid"<<getNewUId();
		setName(name.str()); 

		setCopyrightString("Bogdan Pasca (2010)");		

		/* Set up the I/O signals of of the entity */
		addFPInput ("X", wE, wF);
		addFPInput ("Y", wE, wF);
		addInput("S");
		addFPOutput("A", wE, wF ); 
		
		FPMultiplier *fmt = new FPMultiplier(target, wE, wF, wE, wF, wE, wF);//, 1, 0.6, -1);
		oplist.push_back(fmt);
		
		inPortMap(fmt, "X", "X");
		inPortMap(fmt, "Y", "Y");
		outPortMap(fmt, "R", "rmult");
		vhdl << tab << instance( fmt, "InputMultiplier") << endl;
		syncCycleFromSignal("rmult");
		
		//minimize mux size: fp format is [exc][sign][exp][frac]. 0 is represented 00 0
//		vhdl << tab << declare("addOperandExcSign",3) << " <= "<<zg(3,0)<<" when S='0' else accOut"<<range(wE+wF+2,wE+wF)<<";"<<endl;
//		vhdl << tab << declare("addOperand", wE+wF+3) << " <= addOperandExcSign & accOut"<<range(wE+wF-1, 0)<<";"<<endl;

		vhdl << tab << declare("addOperand", wE+wF+3) << " <= "<<zg( wE+wF+3,0)<<" when S='0' else accOut;"<<endl;


		nextCycle();
		
		FPAdderSinglePath *fas = new FPAdderSinglePath(target, wE, wF, wE, wF, wE, wF);
		oplist.push_back(fas);
		
		inPortMap   (fas, "X", "rmult");
		inPortMap   (fas, "Y", "addOperand");
		outPortMap  (fas, "R", "racc");
		vhdl << tab << instance( fas, "FloatingPointAccumulators") << endl;
		syncCycleFromSignal("racc");
		
		if (adderLatency > 0){
			if (fas->getPipelineDepth()+2<=adderLatency){
				cout << "delaying signals ..." << endl;
				setCycle( getCycleFromSignal("racc") + adderLatency - 2 - fas->getPipelineDepth());
			}else{
				cout << "WARNING: adder delay " << fas->getPipelineDepth() << " is larger than the requested adder delay" << adderLatency << endl;
			}
		}
		
		nextCycle(); //register output for now
		vhdl << tab << declare("accOut", wE+wF+3) << " <= racc;" << endl; 

		vhdl << tab << "A <= accOut;" << endl;
		
	}

	FPFMAcc::~FPFMAcc() {
	}

}
