/*
  A wrapper generator for FloPoCo. 
 
  A wrapper is a VHDL entity that places registers before and after
  an operator, so that you can synthesize it and get delay and area,
  without the synthesis tools optimizing out your design because it
  is connected to nothing.
 
  Author: Florent de Dinechin

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.
 */

#include <iostream>
#include <sstream>
#include "Operator.hpp"
#include "Wrapper.hpp"

namespace flopoco{

	Wrapper::Wrapper(Target* target, Operator *op):
		Operator(target), op_(op)
	{
		setCopyrightString("Florent de Dinechin (2007)");
		/* the name of the Wrapped operator consists of the name of the operator to be 
			wrapped followd by _Wrapper */
		setName(op_->getName() + "_Wrapper");
		
		//this operator is a sequential one	even if Target is unpipelined
		setSequential();	
	

		// Copy the signals of the wrapped operator 
		// This replaces addInputs and addOutputs
		for(int i=0; i < op->getIOListSize(); i++)	{
			Signal* s = op->getIOListSignal(i);
			if(s->type() == Signal::in) 
				addInput(s->getName(), s->width(), s->isBus());
			if(s->type() == Signal::out) 
				addOutput(s->getName(), s->width(), s->isBus());
			
		}

		
 		string idext;

		// copy inputs
		for(int i=0; i < op->getIOListSize(); i++){
			Signal* s = op->getIOListSignal(i);
			 if(s->type() == Signal::in) {
				 idext = "i_"+s->getName();
				 vhdl << tab << declare(idext, s->width(), s->isBus()) << " <= " << s->getName() << ";" << endl;
			}
		}		

		// register inputs
		setCycle(1);

		for(int i=0; i < op->getIOListSize(); i++){
			Signal* s = op->getIOListSignal(i);
			 if(s->type() == Signal::in) {
				 idext = "i_"+s->getName();
				 inPortMap (op, s->getName(), idext);
			 }
		}		


		// port map the outputs
		for(int i=0; i < op->getIOListSize(); i++){
			Signal* s = op->getIOListSignal(i);
			if(s->type() == Signal::out) {
				idext = "o_" + s->getName();
				outPortMap (op, s->getName(), idext);
			}
		}

		// The VHDL for the instance
		vhdl << instance(op, "test");

		// Advance cycle to the cycle of the outputs
		syncCycleFromSignal(idext, false); // this is the last output
		nextCycle();

		// copy the outputs
		for(int i=0; i < op->getIOListSize(); i++){
			Signal* s = op->getIOListSignal(i);
			if(s->type() == Signal::out) {
				string idext = "o_" + s->getName();
				vhdl << tab << s->getName() << " <= " << use(idext) << ";" <<endl;
			}
		}
		
	}

	Wrapper::~Wrapper() {
	}


}
