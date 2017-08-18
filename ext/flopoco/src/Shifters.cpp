/*
  A barrel shifter for FloPoCo
 
  Authors: Florent de Dinechin, Bogdan Pasca

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
#include <gmp.h>
#include <mpfr.h>
#include <stdio.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

#include "Shifters.hpp"

using namespace std;

// TODO there is a small inefficiency here, as most bits of s don't need to be copied all the way down

namespace flopoco{


	Shifter::Shifter(Target* target, int wIn, int maxShift, ShiftDirection direction, map<string, double> inputDelays) :
		Operator(target, inputDelays), wIn_(wIn), maxShift_(maxShift), direction_(direction) {
	
		setCopyrightString ( "Bogdan Pasca, Florent de Dinechin (2008-2011)" );	
		setOperatorName();
		srcFileName = (direction == Right?  "RightShifter": "LeftShifter");
		
		REPORT( INFO, " wIn="<<wIn<<" maxShift="<<maxShift<<" direction="<< (direction == Right?  "RightShifter": "LeftShifter") );
		
		// -------- Parameter set up -----------------
		wOut_         = wIn_ + maxShift_;
		wShiftIn_     = intlog2(maxShift_);
		maxInputDelay_ = getMaxInputDelays(inputDelays); 

		addInput ("X", wIn_);
		addInput ("S", wShiftIn_);  
		addOutput("R", wOut_);

		vhdl << tab << declare("level0",wIn_  ) << "<= X;" <<endl;
		vhdl << tab << declare("ps", wShiftIn_) << "<= S;" <<endl;
	
		// local variables
		int    lastRegLevel = -1;
		int    unregisteredLevels = 0;
		int    dep = 0;
		setCriticalPath( getMaxInputDelays( inputDelays) );

		for (int currentLevel=0; currentLevel<wShiftIn_; currentLevel++){
			//compute current level delay
			unregisteredLevels = currentLevel - lastRegLevel;
			if ( intpow2(unregisteredLevels-1) > wIn_+currentLevel+1 ) 
				dep = wIn + currentLevel + unregisteredLevels;
			else
				dep = intpow2(unregisteredLevels-1);
		
			if (verbose)
				cerr<<"> Shifters\t depth = "<<dep<<" at i="<<currentLevel<<endl;
				
			double wireD = target->localWireDelay(2*wIn);//the delay is unusually high
			REPORT(DEBUG, " wire delay is " << wireD << " and unregisteredLevels="<<unregisteredLevels);
			if (manageCriticalPath( intlog( mpz_class(target->lutInputs()/2), mpz_class(dep)) * target->lutDelay() + (intlog( mpz_class(target->lutInputs()/2), mpz_class(dep))-1)*target->localWireDelay()+ wireD)){
				lastRegLevel = currentLevel;
				REPORT(DEBUG, tab << "REG LEVEL current delay is:" << getCriticalPath());
			}
//			vhdl << "--Estimated delay is:" << getCriticalPath() << endl;
			if (currentLevel<wShiftIn_-1)	
				setCriticalPath(0.0);

			ostringstream currentLevelName, nextLevelName;
			currentLevelName << "level"<<currentLevel;
			nextLevelName << "level"<<currentLevel+1;
			if (direction==Right){
				vhdl << tab << declare(nextLevelName.str(),wIn+intpow2(currentLevel+1)-1 ) 
					  <<"<=  ("<<intpow2(currentLevel)-1 <<" downto 0 => '0') & "<<currentLevelName.str()<<" when ps";
				if (wShiftIn_ > 1) 
					vhdl << "(" << currentLevel << ")";
				vhdl << " = '1' else "
					  << tab << currentLevelName.str() <<" & ("<<intpow2(currentLevel)-1<<" downto 0 => '0');"<<endl;
			}else{
				vhdl << tab << declare(nextLevelName.str(),wIn+intpow2(currentLevel+1)-1 )
					  << "<= " << currentLevelName.str() << " & ("<<intpow2(currentLevel)-1 <<" downto 0 => '0') when ps";
				if (wShiftIn_>1) 
					vhdl << "(" << currentLevel<< ")";
				vhdl << "= '1' else "
					  << tab <<" ("<<intpow2(currentLevel)-1<<" downto 0 => '0') & "<< currentLevelName.str() <<";"<<endl;
			}
			
		}
		//update the output slack
		outDelayMap["R"] = getCriticalPath();
		REPORT(DETAILED, "Delay at output " << getCriticalPath() );
		
	
		ostringstream lastLevelName;
		lastLevelName << "level"<<wShiftIn_;
		if (direction==Right)
			vhdl << tab << "R <= "<<lastLevelName.str()<<"("<< wIn + intpow2(wShiftIn_)-1-1 << " downto " << wIn_ + intpow2(wShiftIn_)-1 - wOut_ <<");"<<endl;
		else
			vhdl << tab << "R <= "<<lastLevelName.str()<<"("<< wOut_-1 << " downto 0);"<<endl;
		
	}

	Shifter::~Shifter() {
	}

	void Shifter::setOperatorName(){
		ostringstream name;
		if(direction_==Left) name <<"LeftShifter_";
		else                 name <<"RightShifter_";
		name<<wIn_<<"_by_max_"<<maxShift_<<"_uid"<<Operator::getNewUId();;
		uniqueName_=name.str();
	}

	void Shifter::emulate(TestCase* tc)
	{
		mpz_class sx = tc->getInputValue("X");
		mpz_class ss = tc->getInputValue("S");
		mpz_class sr ;

		mpz_class shiftedInput = sx;
		int i;
	
		if (direction_==Left){
			mpz_class shiftAmount = ss;
			for (i=0;i<shiftAmount;i++)
				shiftedInput=shiftedInput*2;
		
			for (i= wIn_+intpow2(wShiftIn_)-1-1; i>=wOut_;i--)
				if ( mpzpow2(i) <= shiftedInput )
					shiftedInput-=mpzpow2(i);
		}else{
			mpz_class shiftAmount = maxShift_-ss;

			if (shiftAmount > 0){
				for (i=0;i<shiftAmount;i++)
					shiftedInput=shiftedInput*2;
			}else{
				for (i=0;i>shiftAmount;i--)
					shiftedInput=shiftedInput/2;
			}
		}

		sr=shiftedInput;
		tc->addExpectedOutput("R", sr);
	}



}
