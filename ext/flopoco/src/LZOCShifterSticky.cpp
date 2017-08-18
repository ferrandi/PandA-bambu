/*
  A leading zero/one counter + shifter + sticky bit computer for FloPoCo
 
  Authors: Florent de Dinechin, Bogdan Pasca

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
#include "LZOCShifterSticky.hpp"

using namespace std;



namespace flopoco{


	LZOCShifterSticky::LZOCShifterSticky(Target* target, int wIn, int wOut, int wCount, bool computeSticky, const int countType, map<string, double> inputDelays) :
		Operator(target, inputDelays), wIn_(wIn), wOut_(wOut), wCount_(wCount), computeSticky_(computeSticky), countType_(countType) {
	
		// -------- Parameter set up -----------------
		setEntityType( (countType_==-1?gen:spec) );
		srcFileName = "LZOCShifterSticky";
		
		REPORT( INFO, "wIn="<<wIn << " wOut="<<wOut << " wCount="<<wCount << " computeSticky=" << computeSticky  << " countType=" << countType); 

		ostringstream name; 
		name << "L" << (countType_<0?"ZO":((countType_>0)?"O":"Z")) << "CShifter"
			  << (computeSticky_?"Sticky":"") << "_" << wIn_ << "_to_"<<wOut_<<"_counting_"<<(1<<wCount_)<<"_uid"<<Operator::getNewUId();;
		setName(name.str());

		setCopyrightString("Florent de Dinechin, Bogdan Pasca (2007)");

	
		// -------- Parameter set up -----------------
		setEntityType( (countType_==-1?gen:spec) );
	
	
		addInput ("I", wIn_);
		if (entityType_==gen) addInput ("OZb"); /* if we generate a generic LZOC */
		addOutput("Count", wCount_);
		addOutput("O", wOut_);
		if (computeSticky_)   addOutput("Sticky"); /* if we require a sticky bit computation */

		// we consider that wOut <= wIn. We fix this at the end if not the case
		int wOut_true = wOut_;
		wOut_ = wOut > wIn_ ? wIn_ : wOut;

	
		vhdl << tab << declare(join("level",wCount_), wIn_) << " <= I ;"   <<endl; 
		if (entityType_==gen) vhdl << tab << declare("sozb") << "<= OZb;"<<endl;
		if ((computeSticky_)&&(wOut_<wIn))   vhdl << tab << declare(join("sticky",wCount_)) << " <= '0' ;"<<endl; //init sticky 
	
		int currLev=wIn, prevLev=0;

		setCriticalPath( getMaxInputDelays(inputDelays) );
		for (int i=wCount_-1; i>=0; i--){
			//int currLev = (wOut_>intpow2(i)?wOut_:intpow2(i));
			prevLev = currLev;
	
			// level(k) = max ( max (2^k, wOut) + 2^k -1) , wIn)
			currLev = (wOut_>intpow2(i)?wOut_:intpow2(i));
			currLev += (intpow2(i)-1); 
			currLev = (currLev > wIn_? wIn_: currLev);

			if (countType>=0)
				manageCriticalPath( target->localWireDelay() + target->eqConstComparatorDelay( intpow2(i) ) ) ;
			else
				manageCriticalPath( target->localWireDelay() + target->eqComparatorDelay( intpow2(i) ) ) ;
				
			vhdl << tab << declare(join("count",i)) << "<= '1' when " <<join("level",i+1)<<range(prevLev-1,prevLev - intpow2(i))<<" = "
				  <<"("<<prevLev-1<<" downto "<<prevLev - intpow2(i)<<"=>"<< (countType_==-1? "sozb": countType_==0?"'0'":"'1'")<<") else '0';"<<endl;

			manageCriticalPath( target->localWireDelay() + target->lutDelay() );
			vhdl << tab << declare(join("level",i),currLev) << "<= " << join("level",i+1)<<"("<<prevLev-1<<" downto "<< prevLev-currLev << ")"
				  << " when " << join("count",i) << "='0' else ";
			int l,r;
			l = prevLev - intpow2(i) - 1;
			r = (currLev < prevLev - intpow2(i) ? (prevLev - intpow2(i)) - currLev : 0 );
			if (l>=r)
				vhdl << join("level",i+1) << "("<<prevLev - intpow2(i) - 1 <<" downto "<< (currLev < prevLev - intpow2(i) ? (prevLev - intpow2(i)) - currLev : 0 ) <<")";
		     
			if (prevLev - intpow2(i) < currLev )
				vhdl << (l>=r?" & ":"") << rangeAssign(currLev -(prevLev - intpow2(i))-1,0,"'0'");
			vhdl << ";"<<endl;

			if ((computeSticky_)&&(wOut_<wIn)) {
				
				manageCriticalPath( compDelay( max( prevLev-currLev, (currLev < prevLev - intpow2(i) ? (prevLev - int(intpow2(i)) ) - currLev : 0 ))  ) ); 

				vhdl << tab << declare(join("sticky_high_",i)) << "<= '0'";
				if (prevLev-currLev > 0)
					vhdl << "when " << join("level",i+1)<<"("<<prevLev-currLev -1 <<" downto "<< 0 <<") = CONV_STD_LOGIC_VECTOR(0,"<< prevLev-currLev <<") else '1'";
				vhdl << ";"<<endl;

   			vhdl << tab << declare(join("sticky_low_",i)) << "<= '0'";
				if ((currLev < prevLev - intpow2(i) ? (prevLev - intpow2(i)) - currLev : 0 ) > 0)
					vhdl << "when " <<join("level",i+1)<<"("<<(currLev < prevLev - intpow2(i) ? (prevLev - intpow2(i)) - currLev : 0 ) -1 
						  <<" downto "<< 0 <<") = CONV_STD_LOGIC_VECTOR(0,"<< (currLev < prevLev - intpow2(i) ? (prevLev - intpow2(i)) - currLev : 0 ) <<") else '1'";
				vhdl << ";"<<endl;

				manageCriticalPath( muxDelay(currLev) );

				vhdl << tab << declare(join("sticky",i)) << "<= " << join("sticky",i+1) << " or " << join("sticky_high_",i) 
					  << " when " << join("count",i) << "='0' else " << join("sticky",i+1) << " or " << join("sticky_low_",i)<<";"<<endl;
			}
		
			vhdl <<endl;
		}    
		 
		//assign back the value to wOut_
		wOut_ =  wOut_true;
		vhdl << tab << "O <= "<< join("level",0)
			  << (wOut_<=wIn?"":join("&",rangeAssign(wOut_-wIn-1,0,"'0'")))<<";"<<endl;
	
		outDelayMap["O"] = getCriticalPath();
		outDelayMap["Count"] = getCriticalPath();


		vhdl << tab << declare("sCount",wCount_) <<(wCount_==1?"(0)":"")<<" <= ";
		for (int i=wCount_-1; i>=0; i--){
			vhdl <<join("count",i);
			vhdl << (i>0?" & ":join(";","\n"));
		} 

		if((1<<wCount_)-1 > wIn_) {
			vhdl << tab << "Count <= sCount;"<<endl;
			
			/*			vhdl << tab << "Count <= CONV_STD_LOGIC_VECTOR("<<wIn_<<","<<wCount_<<") when sCount=CONV_STD_LOGIC_VECTOR("<<intpow2(wCount_)-1<<","<<wCount_<<")"<<endl
				  << tab << tab << "else sCount;"<<endl;*/
		}
		else {
			vhdl << tab << "Count <= sCount;"<<endl;
		}



		if (computeSticky_){
			outDelayMap["Sticky"] = getCriticalPath();
			if (wOut_>=wIn)
				vhdl << tab << "Sticky <= '0';"<<endl; 
			else
				vhdl << tab << "Sticky <= sticky0;"<<endl;
		}
	}

	LZOCShifterSticky::~LZOCShifterSticky() {
	}

	void LZOCShifterSticky::setEntityType(entityType_t eType){
		entityType_ = eType; 
	}

	int LZOCShifterSticky::getCountWidth() const{
		return wCount_;
	}



	void LZOCShifterSticky::emulate(TestCase* tc)
	{
		mpz_class si   = tc->getInputValue("I");
	
		mpz_class sozb = 42; //dummy value
		if (countType_ == -1) 
			sozb = tc->getInputValue("OZb");
	
		int sticky=0;
		int j, icount;
	
		/* Count the leading zero/one s */
		mpz_class bit = (countType_ == -1) ? sozb : (countType_ == 0 ? 0 : 1); /* what are we counting in the specific case */
		/* from the MSB towards the LSB, check if current bit of input = to the bit we test against */
		for (j = wIn_-1; j >= 0; j--)
			if (mpz_tstbit(si.get_mpz_t(), j) != bit)
				break;
	
		/* the number of bits is then equal to:
			the index of the MSB - the index where we stoped previously */
		icount = (wIn_-1) - j;
		tc->addExpectedOutput("Count", icount);

		/* compute the max value on wOut_ bits */
		maxValue_ = mpzpow2(wOut_)-1;
		mpz_class inputValue = si;
		
		mpz_class stickyTest =  1 ;//(countType_==-1) ? (sozb==0?1:0) : (countType_ == 0 ? 1 : 0) ;
	
		//compute output value and sticky
		if ((countType_==0) || (sozb==0)){
 
			if (inputValue > 0) 
				while (!((inputValue <= maxValue_) && (2*inputValue > maxValue_)))
					if (inputValue>maxValue_){
						if(mpz_tstbit(inputValue.get_mpz_t(), 0)==stickyTest)
							sticky=1;
						inputValue=inputValue/2;
					}else
						inputValue=inputValue*2;
			else {}
		}
		else /* if we are counting ones */
			{
				int restOfBits = wIn_ - icount;
				if (icount>0){
					mpz_class ones = mpzpow2(icount)-1;
					ones *= mpzpow2(restOfBits);
			
					inputValue-=ones; // the input without the leading ones
				} 

				if ((wIn_<=wOut_) || ((wIn_>wOut_) && (restOfBits<wOut_) ))	//shift result in place	
					inputValue *=mpzpow2(wOut_-restOfBits);
				else
					for (int i=1;i<=restOfBits-wOut_;i++){
						if(mpz_tstbit(inputValue.get_mpz_t(), 0)==stickyTest) //FIXME What do we count out when we count ones, the one or the zero?
							sticky=1;
						inputValue=inputValue/2;
					}
			}
		tc->addExpectedOutput("O",inputValue);
				
		if (computeSticky_)
			tc->addExpectedOutput("Sticky",sticky);
	}


}
