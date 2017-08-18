/*
  FIXME for FloPoCo

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
#ifdef HAVE_SOLLYA

#include "FPJacobi.hpp"
#include "../FPAdder3Input.hpp"
#include "../FPAdderSinglePath.hpp"
#include "../ConstMult/FPRealKCM.hpp"


using namespace std;

namespace flopoco{

	FPJacobi::FPJacobi(Target* target, int wE, int wF, int l0, int l1, int l2, int version):
		Operator(target), wE(wE), wF(wF) {
		srcFileName="FPJacobi1";
		ostringstream name;

		name <<"FPJacobi_"<<wE<<"_"<<wF<<"_f"<<target->frequencyMHz()<<"_uid"<<getNewUId();
		setName(name.str()); 

		setCopyrightString("Bogdan Pasca (2010)");		

		/* Set up the I/O signals of of the entity */
		addFPInput ("X", wE, wF);
		addFPInput ("Y", wE, wF);
		addFPInput ("Z", wE, wF);
		addFPInput ("Q", wE, wF);
		
		addInput("S");
		addInput("oS");
		addFPOutput("R", wE, wF);
		
		//input muxes
		nextCycle(); // buffer inputs
		
		vhdl << tab << declare("opX",wE+wF+3) << " <= X when S='0' else opOutNear_d1;"<<endl;
		vhdl << tab << declare("opY",wE+wF+3) << " <= Y when S='0' else opOutMedium_d1;"<<endl;
		vhdl << tab << declare("opZ",wE+wF+3) << " <= Z when S='0' else opOutFarReg;"<<endl;
		
		nextCycle(); // register level
		
		if ( version == 0){
			FPAdder3Input *fpa3in = new FPAdder3Input( target, wE, wF);
			oplist.push_back(fpa3in);
			
			inPortMap (fpa3in, "X", "opX");
			inPortMap (fpa3in, "Y", "opY");
			inPortMap (fpa3in, "Z", "opZ");
			outPortMap(fpa3in, "R", "addRes");
			vhdl << tab << instance( fpa3in, "InputAdder") << endl;
			syncCycleFromSignal("addRes");
			nextCycle();
			
		}else{
			FPAdderSinglePath *fpa1 = new FPAdderSinglePath(target, wE, wF, wE, wF, wE, wF);
			oplist.push_back(fpa1);
			
			inPortMap (fpa1, "X", "opX");
			inPortMap (fpa1, "Y", "opY");
			outPortMap(fpa1, "R", "addRes_1");
			vhdl << tab << instance( fpa1, "InputAdder1") << endl;
			syncCycleFromSignal("addRes_1");
			nextCycle();

			FPAdderSinglePath *fpa2 = new FPAdderSinglePath( target, wE, wF, wE, wF, wE, wF);
			oplist.push_back(fpa2);
			
			inPortMap (fpa1, "X", "addRes_1");
			inPortMap (fpa1, "Y", "opZ");
			outPortMap(fpa1, "R", "addRes");
			vhdl << tab << instance( fpa1, "InputAdder2") << endl;
			syncCycleFromSignal("addRes");
			nextCycle();
		}
		
		
		FPRealKCM *cm = new FPRealKCM( target, wE, wF, "1/3");
		oplist.push_back(cm);
		
		inPortMap (cm, "X", "addRes");
		outPortMap(cm, "R", "multRes");
		vhdl << tab << instance( cm, "ConstantMultiplier") << endl;
		syncCycleFromSignal("multRes");
					
		//<---------------------- parameter latency
		int pipelineLevels0 = l0 - (getCycleFromSignal("multRes")+2);
		
		if (pipelineLevels0 >= 0)
			setCycle( getCycleFromSignal("multRes") + pipelineLevels0  );
		else{
			cout << "WARNING: you asked l1="<<l0<<" and we got "<< (getCycleFromSignal("multRes")+2) << endl;
		}
		
		cout << "INFO: you asked l1="<<l0<<" and we got "<< (getCycleFromSignal("multRes")+2) << ". Padding with registers." << endl;
		
		nextCycle();
		vhdl << tab << declare( "outMuxOut", wE+wF+3) << " <= Q when oS='1' else multRes;"<<endl;
		nextCycle();
		
		vhdl << tab << declare( "opOutNear", wE+wF+3) << " <= outMuxOut;" << endl;
		setCycle(getCycleFromSignal("opOutNear")+l1);
		vhdl << tab << declare( "opOutMedium", wE+wF+3) << " <= opOutNear;" << endl;
		setCycle(getCycleFromSignal("opOutMedium")+l2);
		vhdl << tab << declare( "opOutFar", wE+wF+3) << " <= opOutMedium;" << endl;
		nextCycle();
		vhdl << tab << declare( "opOutFarReg", wE+wF+3) << " <= opOutFar;"<<endl;
		setCycleFromSignal("opOutNear");
		vhdl << tab << "R <= opOutNear;" << endl;
	}

	FPJacobi::~FPJacobi() {
	}

}
#endif //HAVE_SOLLYA
