/*
  An integer comparator unit
 
  Author: Bogdan Pasca

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.

*/

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "IntComparator.hpp"

using namespace std;



namespace flopoco{


	IntComparator::IntComparator(Target* target, int wIn, int criteria, bool constant, int constValue, map<string, double> inputDelays) :
		Operator(target, inputDelays), wIn_(wIn), criteria_(criteria) {
	
		// -------- Parameter set up -----------------
		srcFileName = "IntComaprator";
		if (criteria<-2 && criteria>2)
			criteria_=0;
		
		ostringstream name;
		switch(criteria){
			case -2: name << "IntComparator_"<< wIn<<"_"<<"less";   break;
			case -1: name << "IntComparator_"<< wIn<<"_"<<"leq";    break;
			case  0: name << "IntComparator_"<< wIn<<"_"<<"eq";     break;
			case  1: name << "IntComparator_"<< wIn<<"_"<<"geq";    break;
			case  2: name << "IntComparator_"<< wIn<<"_"<<"greater";break;
			default: name << "IntComparator_"<< wIn<<"_"<<"eq";
		}
		
		name << "_uid"<<getNewUId();
		setName(name.str());
		setCopyrightString("Bogdan Pasca (2010)");

		addInput ("X", wIn_,true);
		addInput ("Y", wIn_,true);
		addOutput("R",true); 

		switch(criteria){
			case -2: vhdl << tab << "R <= '1' when (X <  Y) else '0';"<<endl; break;
			case -1: vhdl << tab << "R <= '1' when (X <= Y) else '0';"<<endl; break;
			case  0:{ 

				//determine chunk size
				int cs;
				setCriticalPath(getMaxInputDelays(inputDelays));
				if (!target->suggestSlackSubcomparatorSize(cs, wIn_, getCriticalPath() + target->localWireDelay() + target_->ffDelay(), constant)){
					REPORT(INFO, "Extra reg level inserted here!");
					nextCycle();
					setCriticalPath(0.0);
					target->suggestSlackSubcomparatorSize(cs, wIn, target->localWireDelay() + target_->ffDelay(), constant);
				}	
				REPORT(INFO, "The suggested chunk size for the first splitting was:"<<cs);
				//number of chunks
				int k = ( wIn % cs ==0? wIn/cs : wIn/cs + 1);
				if (k > 1){
					//first checks in parallel all chunks for equality
					REPORT(INFO, "Cp before "<<getCriticalPath());
					REPORT(INFO, "comp delay = " << target->eqComparatorDelay(cs));
					manageCriticalPath( target->eqComparatorDelay(cs) );
					REPORT(INFO, "Cp after"<< getCriticalPath());
					
					for (int i=0; i<k; i++){
						vhdl <<tab << declare(join("b",i,"l",0))<<" <= '1' when X"<<range(min((i+1)*cs-1,wIn-1),i*cs);
						if (!constant)
							vhdl<<"=Y"<<range(min((i+1)*cs-1,wIn-1),i*cs)<<" else '0';"<<endl;
						else
							vhdl<<"=conv_std_logic_vector("<<constValue<<","<< wIn<<") else '0';"<<endl;
					}
					//form a new std logic vector of k bits that will be checked against '11...111'
					int l = 0;
					int ibits = k;
					//determine number of chunks needed for this comparisson. In most cases this will be 1
					if (!target->suggestSlackSubcomparatorSize(cs, ibits, getCriticalPath() + target->localWireDelay() + target_->ffDelay(), true)){
						nextCycle();
						setCriticalPath(0.0);
						target->suggestSlackSubcomparatorSize(cs, ibits, target->localWireDelay() + target_->ffDelay(), true);
					}	
					REPORT(INFO, "The number of ibits is "<<ibits<<" cs="<<cs);
					k = ( ibits % cs ==0? ibits/cs : ibits/cs + 1);
					while (ibits>1){
						l++;
						//join bits in a vector
						vhdl << tab << declare(join("lev",l),ibits) << " <= ";
						for (int i=0; i<ibits;i++){
							if (i==ibits-1)
								vhdl << join("b",i,"l",l-1) << ";"<<endl;
							else
								vhdl << join("b",i,"l",l-1) << " & ";
						}	
						manageCriticalPath(target->eqConstComparatorDelay(ibits));
						for (int i=0; i<k; i++){
							vhdl <<tab << declare(join("b",i,"l",l))<<" <= '1' when lev"<<l<<range(min((i+1)*cs-1,ibits-1),i*cs)
							<<"="<< og(  min((i+1)*cs-1,ibits-1)+1 -i*cs ,0 )<<" else '0';"<<endl;
						}
						
						if (!target->suggestSlackSubcomparatorSize(cs, k, getCriticalPath() + target->localWireDelay() + target_->ffDelay() , true)){
							nextCycle();
							setCriticalPath(0.0);
							target->suggestSlackSubcomparatorSize(cs, k, 0.0 + target->localWireDelay() + target_->ffDelay(), true);
						}
						ibits = k;
						k = ( ibits % cs ==0? ibits/cs : ibits/cs + 1);
					}
					//assign R
					vhdl << tab << "R <= " <<join("b",0,"l",l)<<";"<<endl;
					outDelayMap["R"] = getCriticalPath();
					
				}else{
					//if we can do it all in one cycle
					if (!constant)
						vhdl << tab << "R <= '1' when X=Y else '0';"<<endl;
					else
						vhdl << tab << "R <= '1' when X=conv_std_logic_vector("<<constValue<<","<< wIn<<") else '0';"<<endl;
				}
			} break;
			case  1: vhdl << tab << "R <= '1' when (X >=  Y) else '0';"<<endl; break;
			case  2: vhdl << tab << "R <= '1' when (X > Y) else '0';"<<endl; break;
			default:;
		}
		
		
		}

	IntComparator::~IntComparator() {
	}
	
	void IntComparator::emulate(TestCase* tc){
		mpz_class svX = tc->getInputValue ( "X" );
		mpz_class svY = tc->getInputValue ( "Y" );
		
		mpz_class svR;
		
		switch(criteria_){
			case -2: if ( svX <  svY ) svR = 1; else svR = 0; break;
			case -1: if ( svX <= svY ) svR = 1; else svR = 0; break;
			case  0: if ( svX == svY ) svR = 1; else svR = 0; break;
			case  1: if ( svX >= svY ) svR = 1; else svR = 0; break;
			case  2: if ( svX > svY  ) svR = 1; else svR = 0; break;
			default:; 
		}
		
		tc->addExpectedOutput ( "R", svR );
	}


}
