/*
  An integer multi-operand adder for FloPoCo
 
  Authors : Bogdan Pasca
 
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

*/

#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"
#include "IntNAdder.hpp"

using namespace std;

namespace flopoco{

	IntNAdder::IntNAdder(Target* target, int wIn, int N, map<string, double> inputDelays, bool carryIn):
		IntMultiAdder(target, wIn, N, inputDelays, carryIn, true), wIn_(wIn), N_(N), carryIn_(carryIn) 
	{
	
		ostringstream name;
		name << "IntNAdder_" << wIn_<<"_op"<<N<<"_uid"<<Operator::getNewUId();
		srcFileName = "IntNAdder";
		setName(name.str());

		setCopyrightString("Bogdan Pasca (2009, 2010)");

		// Set up the IO signals
		for (int i=0; i<N; i++)
			addInput ( join("X",i) , wIn_, true);
	
		if (carryIn)
			addInput( "Cin");
	
		addOutput("R"  , wIn_, 1, true);

		int beta;
		int *cIndex; /**< array containing the indexes for all Chunks*/

		setCriticalPath( getMaxInputDelays(inputDelays) );
		if (! target->suggestSlackSubaddSize(alpha, wIn, target->ffDelay() + getMaxInputDelays(inputDelays))){
			nextCycle(); ///////////////////////////////////////////////////
			target->suggestSubaddSize(alpha, wIn);				
		}
			beta = (wIn % alpha == 0 ? alpha: wIn % alpha);	
		
		nbOfChunks = (wIn/alpha) + (wIn-alpha>0?1:0);

		//array containig the chunk sizes
		cSize = new int[nbOfChunks+1];
		for (int i=0; i<nbOfChunks; i++)
			cSize[i] = alpha;

		if (nbOfChunks > 1)
			cSize[nbOfChunks-1] = beta;
		
		//the indexes in the inputs of the chunks
		cIndex = new int[nbOfChunks];
		cIndex[0] = cSize[0];
		for (int i=1; i < nbOfChunks; i++)
			cIndex[i] = cIndex[i-1] + cSize[i];
		
		if (N>=2){
			//split the inputs
			for (int i=0;i<N;i++)
				for (int j=0; j<nbOfChunks; j++)
					vhdl << tab << declare ( join("sX",i,"_",j,"_l",0), cSize[j]+1) << " <=  \"0\" & "<<join("X",i)<<range(cIndex[j]-1, (j==0?0:cIndex[j-1]))<<";"<<endl;
	
			int currentLevel = 1;
			for (int l=1; l<N; l++){
				//perform addition round; there is only one additon per round
				manageCriticalPath(target->localWireDelay() + target->adderDelay(cSize[0]));
				 
				for (int j=0; j<nbOfChunks; j++){
					ostringstream dname, uname1, uname2, uname3;
					dname << "sX"<<j<<"_0_l"<<l;
					uname1 << "sX"<<j<<"_0_l"<<l-1;
					uname2 << "sX"<<j<<"_1_l"<<l-1;
					uname3 << "sX"<<j-1<<"_0_l"<<l-1;
					vhdl << tab << declare( join("sX0_",j,"_l",l), cSize[j]+1) << " <=  (\"0\" & " << join("sX0_",j,"_l",l-1)<<range(cSize[j]-1, 0) << ") + " << join("sX1_",j,"_l",l-1);
					if ( j==0 && l==1 && carryIn) 
						vhdl << " + Cin";
					if (j>0) 
						vhdl << " + " << join("sX0_",j-1,"_l",l-1) << of(cSize[j-1]);
					vhdl << ";" << endl;				
				}
		
				//from this point we just add two numbers with internal 
				//propagations, so perform addition and then take care of the propagations in a loop-like manner
				for (int propL=2; propL<=N-l; propL++)
					for (int j=0; j<nbOfChunks; j++)
						vhdl << tab << declare(join("sX",propL-1,"_",j,"_l",l), cSize[j]+1) << " <= " << join("sX",propL,"_",j,"_l",l-1) << ";" <<endl;
		
				currentLevel++;
			}

			if (nbOfChunks>1){
				vhdl << tab << "--final propagations " << endl; 
	
				for (int i=2; i<nbOfChunks+1; i++){
					manageCriticalPath(target->adderDelay(cSize[i]));
					for (int j=i-1; j< nbOfChunks ; j++){
						ostringstream dname, uname1, uname2;
						dname <<  "sX"<<j<<"_0_l"<<currentLevel;
						uname1 << "sX"<<j<<"_0_l"<<currentLevel-1;
						uname2 << "sX"<<j-1<<"_0_l"<<currentLevel-1;
						vhdl << tab << declare(join("sX0_",j,"_l",currentLevel), cSize[j]+1) << " <= (" << zg(1) << " & " << join("sX0_",j,"_l",currentLevel-1)<<range(cSize[j]-1,0) << ") + " 
								                                                            << join("sX0_",j-1,"_l",currentLevel-1) << of(cSize[j-1])<<";" <<endl;
					}
					currentLevel++;
				}
			}
			currentLevel--;
		
			outDelayMap["R"] = getCriticalPath();
			vhdl << tab << "R <= ";
			int k=0;

			for (int i=nbOfChunks-1; i>=0; i--){
				vhdl << join("sX0_",i,"_l",currentLevel-k) << range(cSize[i]-1,0);
				if (i > 0) vhdl << " & ";
				k++;
			}
			vhdl << ";" <<endl;
			
			setCriticalPath( target->adderDelay(beta) );
			outDelayMap["R"] = getCriticalPath(); 
			
		}else if (N==1 && carryIn){
			IntAdder *ia = new IntAdder(target, wIn, inDelayMap("X", getCriticalPath()));
			oplist.push_back(ia);
			
			inPortMap   ( ia, "X"  , "X0");
			inPortMapCst( ia, "Y"  , zg(wIn));
			inPortMap   ( ia, "Cin", "Cin");
			outPortMap  ( ia, "R"  , "S");
			
			vhdl << tab << instance(ia, "CarryPropagateAdder") << endl;		
			syncCycleFromSignal("S");
			setCriticalPath( ia->getOutputDelay("R") );
			vhdl << tab << "R <= S;"<<endl;
		}else if (N==1){
			vhdl << tab << "R <= X0;"<<endl;
		}	
		outDelayMap["R"] = getCriticalPath();
	}


	IntNAdder::~IntNAdder() {
	}


//	void IntNAdder::emulate(TestCase* tc)
//	{
//		mpz_class svX;
//		mpz_class svC;
//		
//		if (carryIn_)
//			svC =  tc->getInputValue("Cin");
//		else 
//			svC = 0;

//		mpz_class svR = svC;

//		for (int i=0; i<N_; i++){
//			ostringstream iName;
//			iName << "X"<<i;
//			svX = tc->getInputValue(iName.str());
//			svR = svR + svX;
//			mpz_clrbit(svR.get_mpz_t(),wIn_); 
//		}
//	
//		tc->addExpectedOutput("R", svR);
//	}
}
