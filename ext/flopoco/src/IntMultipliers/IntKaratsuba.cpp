/*
  An integer multiplier for FloPoCo using the Karatsuba method
 
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
#include "IntKaratsuba.hpp"

using namespace std;

namespace flopoco{

//#define OPT 0
#define NADD


	IntKaratsuba:: IntKaratsuba(Target* target, int wIn, map<string, double> inputDelays) :
		Operator(target), wIn_(wIn), wOut_(2*wIn){

		ostringstream name;
		name << "IntKaratsuba_" << wIn_<<"_f"<<target->frequencyMHz();
		setCopyrightString("Bogdan Pasca (2008-2010)");
		setName(name.str());
	
		/* Set up the IO signals
		 * X and Y have wInX_ and wInY_ bits respectively 
		 * R has wOut_ bits where wOut_ = (wInX_ + wInY_) bits
		 */
		addInput ("X", wIn_);
		addInput ("Y", wIn_);
		addOutput("R", wOut_);
	
		//TODO replace 17 with a generic multiplierWidth()
	
		int chunks = ( wIn % 17 ==0 ? wIn/17 : ceil( double(wIn)/double(17)) );
		setCriticalPath(getMaxInputDelays(inputDelays));
		
		int chunkSize_ = 17;

		if (chunks == 1){
			/* no need for karatsuba here */
			vhdl << tab << "R <= X * Y;" << endl;
		}else if (chunks == 2){
			//pad inputs to 34 bits
			vhdl << tab << declare ("sX", 34) << " <= X & " << zg(34-wIn, 0) << ";" << endl;
			vhdl << tab << declare ("sY", 34) << " <= Y & " << zg(34-wIn, 0) << ";" << endl;
			//chunk splitting
			vhdl << tab << declare ("x0", 18) << " <= \"0\" & sX" << range(16,0)  << ";" << endl;
			setSignalDelay("x0", getCriticalPath());
			vhdl << tab << declare ("x1", 18) << " <= \"0\" & sX" << range(33,17) << ";" << endl;
			vhdl << tab << declare ("y0", 18) << " <= \"0\" & sY" << range(16,0)  << ";" << endl;
			vhdl << tab << declare ("y1", 18) << " <= \"0\" & sY" << range(33,17) << ";" << endl;

			manageCriticalPath( target->adderDelay(18));
			vhdl << tab << declare ("dx", 18) << " <= x1 - x0;" << endl;
			vhdl << tab << declare ("dy", 18) << " <= y1 - y0;" << endl;
			setSignalDelay("dx",getCriticalPath());
			setSignalDelay("dy",getCriticalPath());

			setCycleFromSignal("x0", getSignalDelay("x0"), false);
			manageCriticalPath(target->LogicToDSPWireDelay() + target->DSPMultiplierDelay());
			vhdl << tab << declare ("p0", 36) << " <= x0 * y0;" << endl;
			setSignalDelay("p0", getCriticalPath());

			setCycleFromSignal("dx", getSignalDelay("dx"));
			manageCriticalPath(target->LogicToDSPWireDelay() + target->DSPMultiplierDelay());
			vhdl << tab << declare ("tp1", 36) << " <= dx * dy;"<<endl;
			syncCycleFromSignal("p0", getSignalDelay("p0"));
			manageCriticalPath(target->DSPAdderDelay());		
			vhdl << tab << declare ("p1", 36) << " <= p0 - tp1;"<<endl;
			setSignalDelay("p1", getCriticalPath());

			setCycleFromSignal("x0", getSignalDelay("x0"));
			manageCriticalPath(target->LogicToDSPWireDelay() + target->DSPMultiplierDelay());
			vhdl << tab << declare ("tp2", 36) << " <= x1 * y1;"<<endl;
			syncCycleFromSignal("p1", getSignalDelay("p1"));
			manageCriticalPath(target->DSPAdderDelay());		
			vhdl << tab << declare ("p2", 36) << " <= p1 + tp2;"<<endl;

			manageCriticalPath(target->DSPToLogicWireDelay() + target->adderDelay(36));
//			manageCriticalPath(target->adderDelay(36));		
			vhdl << tab << declare ("t2", 36) << " <= p2 - p1;" << endl;		

			vhdl << tab << declare ("t0", 36) << " <= p0;" << endl;
			vhdl << tab << declare ("t1", 36) << " <= p2;" << endl;		

			vhdl << tab << declare ("a1", 51) << " <= t2"<<range(33,0) << " & t0" <<range(33,17) << ";" << endl;
			vhdl << tab << declare ("b1", 51) << " <= " << zg(15,0) << " & p2;" << endl;

			IntAdder *ia = new IntAdder(target, 51, inDelayMap("X", target->localWireDelay() + getCriticalPath()));
			oplist.push_back(ia);
			
			inPortMap(ia, "X", "a1");
			inPortMap(ia, "Y", "b1");
			inPortMapCst(ia, "Cin", "'0'");
			outPortMap(ia, "R", "a1pb1");
			
			vhdl<< tab << instance(ia, "finalAdder");
			syncCycleFromSignal("a1pb1");
			setCriticalPath( ia->getOutputDelay("R"));
			
			if (68 - 2*wIn < 17 )
				vhdl << tab << "R <= a1pb1 & t0"<<range(16, (68 - 2*wIn)) << ";" << endl;
			else{
				vhdl << tab << "R <= a1pb1" << range(50, 68-2*wIn-17) << ";" << endl;
			}
		}else if (chunks == 3){
			//pad inputs to 51 bits
			vhdl << tab << declare ("sX", 51) << " <= X & " << zg(51-wIn, 0) << ";" << endl;
			vhdl << tab << declare ("sY", 51) << " <= Y & " << zg(51-wIn, 0) << ";" << endl;
			//chunk splitting
			vhdl << tab << declare ("x0", 18) << " <= \"0\" & sX" << range(16,0)  << ";" << endl;
			vhdl << tab << declare ("x1", 18) << " <= \"0\" & sX" << range(33,17) << ";" << endl;
			vhdl << tab << declare ("x2", 18) << " <= \"0\" & sX" << range(50,34) << ";" << endl;
			vhdl << tab << declare ("y0", 18) << " <= \"0\" & sY" << range(16,0)  << ";" << endl;
			vhdl << tab << declare ("y1", 18) << " <= \"0\" & sY" << range(33,17) << ";" << endl;
			vhdl << tab << declare ("y2", 18) << " <= \"0\" & sY" << range(50,34) << ";" << endl;
			nextCycle();////////////////////////////////////////////////////////////
			//precomputing
			vhdl << tab << declare ("dx10", 18) << " <= x1 - x0;" << endl;
			vhdl << tab << declare ("dx20", 18) << " <= x2 - x0;" << endl;
			vhdl << tab << declare ("dx21", 18) << " <= x2 - x1;" << endl;
			vhdl << tab << declare ("dy10", 18) << " <= y1 - y0;" << endl;
			vhdl << tab << declare ("dy20", 18) << " <= y2 - y0;" << endl;
			vhdl << tab << declare ("dy21", 18) << " <= y2 - y1;" << endl;
			//computing
			vhdl << tab << declare("p00",36) << " <= x0 * y0;" << endl;
			vhdl << tab << declare("p11",36) << " <= x1 * y1;" << endl;
			vhdl << tab << declare("p22",36) << " <= x2 * y2;" << endl;
			nextCycle();////////////////////////////////////////////////////////////
			vhdl << tab << declare("tk_0", 36) << " <= p00 - (dx10 * dy10);"<<endl;
			vhdl << tab << declare("t2k_0",36) << " <= p11 - (dx20 * dy20);"<<endl;
			vhdl << tab << declare("t3k_0",36) << " <= p22 - (dx21 * dy21);"<<endl;
			nextCycle();////////////////////////////////////////////////////////////
			nextCycle();////////////////////////////////////////////////////////////
			vhdl << tab << declare("tk"   ,36) << " <= tk_0  + p11;" << endl;
			vhdl << tab << declare("t2k_1",36) << " <= t2k_0 + p00;" << endl;
			vhdl << tab << declare("t3k"  ,36) << " <= t3k_0 + p11;" << endl;
			nextCycle();////////////////////////////////////////////////////////////
			vhdl << tab << declare("s1_1",34) << " <= p22"<<range(33,0)<< " + (\"0\" & t3k"<<range(34,17)<<");"<<endl;
			vhdl << tab << declare("t2k" ,36) << " <= t2k_1 + p22;" << endl;
			vhdl << tab << declare("s0"  ,35) << " <= ( \"0\" & p00"<<range(33,17)<<") + tk"<<range(34,0) << ";" << endl;
			vhdl << tab << declare("s1"  ,51) << " <= s1_1 & t3k"<<range(16,0) << ";" << endl;
			vhdl << tab << declare("finalSumLow_0",17) << " <= p00" << range(16,0) << ";" << endl;
			nextCycle();////////////////////////////////////////////////////////////
			vhdl << tab << declare("finalSumLow_1",17) << " <= s0" << range(16,0) << ";" << endl;
			vhdl << tab << declare("l5_0",18) << " <= ( \"0\" & s0"<<range(33,17)<<") + (\"0\" & t2k"<<range(16,0)<<");"<<endl;
			vhdl << tab << declare("l5_1",27) << " <= ( \"0\" & s1"<<range(25,0)<<") + ("<< zg(7,0) << " & t2k"<<range(35,17)<< ") + s0"<< of(34) <<";"<<endl;
			vhdl << tab << declare("l5_2",25) << " <= s1" << range(50,26) << ";" << endl;
			nextCycle();////////////////////////////////////////////////////////////
			vhdl << tab << declare("finalSumLow_2",17) << " <= l5_0" << range(16,0) << ";" << endl;
			vhdl << tab << declare("l6_0",27) << " <= l5_1 + (\"0\" & l5_0"<<range(17,17) << ");" << endl;		
			vhdl << tab << declare("l6_1",25) << " <= l5_2;" << endl;
			nextCycle();////////////////////////////////////////////////////////////
				
			vhdl << tab << declare("finalSumLow_3",26) << " <= l6_0"<<range(25,0) << ";" << endl;
			vhdl << tab << declare("finalSumLow_4",25) << " <= l6_1 + l6_0"<<of(26) << ";" << endl;
		
			if (102-2*wIn < 17){                        
				vhdl << tab << " R <= finalSumLow_4 & finalSumLow_3 & finalSumLow_2 & finalSumLow_1 & finalSumLow_0"<<range(16, 102-2*wIn) << ";" << endl;
			}else{
				vhdl << tab << " R <= finalSumLow_4 & finalSumLow_3 & finalSumLow_2 & finalSumLow_1" << range(16, 102-2*wIn-17) << ";" << endl;
			}
		}else if (chunks == 4){
			//pad inputs to 68 bits
			vhdl << tab << declare ("sX", 68) << " <= X & " << zg(68-wIn, 0) << ";" << endl;
			vhdl << tab << declare ("sY", 68) << " <= Y & " << zg(68-wIn, 0) << ";" << endl;
			//chunk splitting
			vhdl << tab << declare ("x0", 18) << " <= \"0\" & sX" << range(16,0)  << ";" << endl;
			vhdl << tab << declare ("x1", 18) << " <= \"0\" & sX" << range(33,17) << ";" << endl;
			vhdl << tab << declare ("x2", 18) << " <= \"0\" & sX" << range(50,34) << ";" << endl;
			vhdl << tab << declare ("x3", 18) << " <= \"0\" & sX" << range(67,51) << ";" << endl;
			vhdl << tab << declare ("y0", 18) << " <= \"0\" & sY" << range(16,0)  << ";" << endl;
			vhdl << tab << declare ("y1", 18) << " <= \"0\" & sY" << range(33,17) << ";" << endl;
			vhdl << tab << declare ("y2", 18) << " <= \"0\" & sY" << range(50,34) << ";" << endl;
			vhdl << tab << declare ("y3", 18) << " <= \"0\" & sY" << range(67,51) << ";" << endl;
			//precomputing
			vhdl << tab << declare ("dx10", 18) << " <= x1 - x0;" << endl;
			vhdl << tab << declare ("dx20", 18) << " <= x2 - x0;" << endl;
			vhdl << tab << declare ("dx21", 18) << " <= x2 - x1;" << endl;
			vhdl << tab << declare ("dx30", 18) << " <= x3 - x0;" << endl;
			vhdl << tab << declare ("dx31", 18) << " <= x3 - x1;" << endl;
			vhdl << tab << declare ("dx32", 18) << " <= x3 - x2;" << endl;

			vhdl << tab << declare ("dy10", 18) << " <= y1 - y0;" << endl;
			vhdl << tab << declare ("dy20", 18) << " <= y2 - y0;" << endl;
			vhdl << tab << declare ("dy21", 18) << " <= y2 - y1;" << endl;
			vhdl << tab << declare ("dy30", 18) << " <= y3 - y0;" << endl;
			vhdl << tab << declare ("dy31", 18) << " <= y3 - y1;" << endl;
			vhdl << tab << declare ("dy32", 18) << " <= y3 - y2;" << endl;
			

			//computing
			vhdl << tab << declare("p00",36) << " <= x0 * y0;" << endl;
			nextCycle();////////////////////////////////////////////////////////////

			vhdl << tab << declare("p10",36) << " <= dx10 * dy10;" << endl;
			vhdl << tab << declare("p20",36) << " <= dx20 * dy20;" << endl;
			vhdl << tab << declare("p21",36) << " <= dx21 * dy21;" << endl;
			vhdl << tab << declare("p31",36) << " <= dx31 * dy31;" << endl;
			vhdl << tab << declare("p32",36) << " <= dx32 * dy32;" << endl;
			vhdl << tab << declare("p30",36) << " <= dx30 * dy30;" << endl;

			vhdl << tab << declare("t1k_0",36) << " <= p00 + (x1 * y1);"<<endl;

			nextCycle();////////////////////////////////////////////////////////////
			vhdl << tab << declare("t2k_0",36) << " <= t1k_0 + (x2 * y2);"<<endl;
//			vhdl << tab << declare("p30",36)   << " <= p21 + (dx30 * dy30);" << endl;
			vhdl << tab << declare("t3k_x",36)   << " <= p21 + p30;" << endl;

			nextCycle();////////////////////////////////////////////////////////////
			vhdl << tab << declare("t3k_0",36) << " <= t2k_0 + (x3 * y3);"<<endl;
			nextCycle();////////////////////////////////////////////////////////////
			vhdl << tab << declare("t2k_1",37) << " <= (t2k_0 & \"0\") - (p20 & \"0\");"<<endl;
			vhdl << tab << declare("t1k_1",37) << " <= (t1k_0 & \"0\") - (p10 & \"0\");"<<endl;

			nextCycle();////////////////////////////////////////////////////////////

			vhdl << tab << declare("t3k_1",37) << " <= (t3k_0 & \"0\") - (t3k_x & \"0\");"<<endl;
			vhdl << tab << declare("t4k_0",37) << " <= (t3k_0 & \"0\") - (p00 & \"0\");"<<endl;
			vhdl << tab << declare("t5k_0",37) << " <= (t3k_0 & \"0\") - (t1k_0 & \"0\");"<<endl;
			vhdl << tab << declare("t6k_0",37) << " <= (t3k_0 & \"0\") - (t2k_0 & \"0\");"<<endl;
			nextCycle();////////////////////////////////////////////////////////////

			vhdl << tab << declare("t4k_1",37) << " <= (t4k_0(36 downto 1) & \"0\") - (p31 & \"0\");"<<endl;
			vhdl << tab << declare("t5k_1",37) << " <= (t5k_0(36 downto 1) & \"0\") - (p32 & \"0\");"<<endl;
			nextCycle();////////////////////////////////////////////////////////////

			IntMultiAdder* c = new IntMultiAdder(target, 119, 7); //68*2 - 17
			oplist.push_back(c);

			vhdl << tab << declare("op0", 119) << " <= " << zg(100,0) << " & p00"<<range(35, 17) << ";" << endl;
			vhdl << tab << declare("op1", 119) << " <= " << zg(83,0) << " & t1k_1"<<range(36, 1) << ";" << endl;
			vhdl << tab << declare("op2", 119) << " <= " << zg(66,0) << " & t2k_1"<<range(36, 1) << " & " << zg(17,0) << ";" << endl;
			vhdl << tab << declare("op3", 119) << " <= " << zg(49,0) << " & t3k_1"<<range(36, 1) << " & " << zg(34,0) << ";" << endl;
			vhdl << tab << declare("op4", 119) << " <= " << zg(32,0) << " & t4k_1"<<range(36, 1) << " & " << zg(51,0) << ";" << endl;
			vhdl << tab << declare("op5", 119) << " <= " << zg(15,0) << " & t5k_1"<<range(36, 1) << " & " << zg(68,0) << ";" << endl;
			vhdl << tab << declare("op6", 119) << " <= t6k_0"<<range(34, 1) << " & " << zg(85,0) << ";" << endl;

			inPortMap( c, "X0", "op0");
			inPortMap( c, "X1", "op1");
			inPortMap( c, "X2", "op2");
			inPortMap( c, "X3", "op3");
			inPortMap( c, "X4", "op4");
			inPortMap( c, "X5", "op5");
			inPortMap( c, "X6", "op6");

			outPortMap(c, "R", "addRes");
			
			vhdl << instance(c, "finalAdder");
			syncCycleFromSignal("addRes");
//			nextCycle();///////////////////////////////////////////////////////

			if (136-2*wIn < 17){                        
				vhdl << tab << " R <= addRes & p00"<<range(16, 136-2*wIn) << ";" << endl;
			}else{
				vhdl << tab << " R <= addRes" << range(118, 136-2*wIn-17) << ";" << endl;
			}
		}else{
			//N-way katastuba
			int g=5; //accumulation guard bits
			
			//expand inputs to multiple of chunk size
			//pad inputs to 34 bits
			vhdl << tab << declare ("sX", chunks*chunkSize_) << " <= X & " << zg(chunks*chunkSize_-wIn, 0) << ";" << endl;
			vhdl << tab << declare ("sY", chunks*chunkSize_) << " <= Y & " << zg(chunks*chunkSize_-wIn, 0) << ";" << endl;
			//step 1 compute differences 
			
			setCriticalPath( getMaxInputDelays(inputDelays) );

			setSignalDelay("sX", getCriticalPath());

			manageCriticalPath( target->adderDelay(chunkSize_+1)); 
			for (int i=1; i< chunks; i++){
				for (int j=0; j<i; j++){
					vhdl << tab << declare( join("dx",i,"_",j), chunkSize_+1 ) << "<= (\"0\" & sX"<<range( (i+1)*chunkSize_-1, i*chunkSize_ ) << ") - (\"0\" & sX"<<range( (j+1)*chunkSize_-1, j*chunkSize_ ) << ");"<<endl;				
					vhdl << tab << declare( join("dy",i,"_",j), chunkSize_+1 ) << "<= (\"0\" & sY"<<range( (i+1)*chunkSize_-1, i*chunkSize_ ) << ") - (\"0\" & sY"<<range( (j+1)*chunkSize_-1, j*chunkSize_ ) << ");"<<endl;				
					setSignalDelay(join("dx",i,"_",j), getCriticalPath());
					setSignalDelay(join("dy",i,"_",j), getCriticalPath());
				}
			}
			
//			setCycleFromSignal(join("dx",1,"_",0), getSignalDelay(join("dx",1,"_",0) );
			//now the accumulations to be performed inside DSPs
			for (int u=1; u<2*(chunks-1); u++){
				bool chainIn=false;
				int chainIn_i=-1, chainIn_j=-1;
				for (int i=0;i<chunks;i++)
					for (int j=0; j<chunks; j++){
						if (i!=j && i>j && i+j==u){
							//perform multiplication
							if (!chainIn)
								setCycleFromSignal( join("dx",i,"_",j), getSignalDelay(join("dx",i,"_",j))); //second operand is identical
							else{
								int p = getCycleFromSignal( join("dx",i,"_",j) );
								int q = getCycleFromSignal( join("P",chainIn_i,chainIn_j));
								setCycle( max(p, q-1) );
							}
							
							manageCriticalPath( target->LogicToDSPWireDelay() + target->DSPMultiplierDelay());
							vhdl << tab << declare( join( (!chainIn?"P":"tP"),i,j), 2*(chunkSize_+1) )<< " <= "<< join("dx",i,"_",j)<<" * " << join("dy",i,"_",j)<<" ;"<<endl;//todo extend accumulation
							setSignalDelay(join((!chainIn?"P":"tP"),i,j), getCriticalPath());
							if (chainIn){
								//sync with the other input
								syncCycleFromSignal( join("P",chainIn_i,chainIn_j), getSignalDelay(join("P",chainIn_i,chainIn_j)));
								manageCriticalPath( target->DSPAdderDelay() );
								//perform addition 
								vhdl << tab << declare( join("P",i,j), 2*(chunkSize_+1) + g) << " <= " << signExtend( join("tP",i,j), 2*(chunkSize_+1) + g) << " + " << signExtend(join("P",chainIn_i,chainIn_j),2*(chunkSize_+1) + g)<<";"<<endl; 
								setSignalDelay( join("P",i,j), getCriticalPath());
							}
							chainIn = true;
							chainIn_i=i;
							chainIn_j=j;
						}
					}
			}	

			//kernel multiplications 
			for (int i=0; i<chunks; i++){
				if (i==0){
					setCycleFromSignal("sX", getSignalDelay("sX"));
				}else{
					int p = getCycleFromSignal("sX");
					int q = getCycleFromSignal(join("s",i-1));
					setCycle( max(p, q-1) );
					if (i==1)
						nextCycle();
				}
				manageCriticalPath(target->LogicToDSPWireDelay() + target->DSPMultiplierDelay());
				vhdl << tab << declare( join("P",i,i), 2*(chunkSize_+1) )<< " <= (\"0\" & sX"<<range( (i+1)*chunkSize_-1, i*chunkSize_ ) << ") * (\"0\" & sY"<<range( (i+1)*chunkSize_-1, i*chunkSize_ ) << ");"<<endl;
				
				if (i>0){
					syncCycleFromSignal( join("s", i-1), getSignalDelay( join("s",i-1)) );
					manageCriticalPath( target->localWireDelay() + target->adderDelay(2*(chunkSize_+1) + g)); //TODO replace with IntAdder
					vhdl << tab << declare( join("s",i), 2*(chunkSize_+1) + g ) << " <= " << signExtend( join("s",i-1), 2*(chunkSize_+1) + g) << " + " << signExtend( join("P",i,i), 2*(chunkSize_+1) + g)<<";"<<endl;
					setSignalDelay(join("s",i), getCriticalPath());
				}else{
					vhdl << tab << declare("s0", 2*(chunkSize_+1))  << " <= P00;"<<endl;
					setSignalDelay("s0", getCriticalPath());
				}
			}
			
//			//now we describe the adder tree that produces the S_k
//			setCycleFromSignal( "P00", getSignalDelay("P00"));
//			for (int i=0; i<chunks-1; i++){
//				manageCriticalPath( target->localWireDelay() + target->adderDelay(2*(chunkSize_+1) + g)); //TODO replace with IntAdder
//				vhdl << tab << declare( join("s",i+1), 2*(chunkSize_+1) + g ) << " <= " << signExtend( (i==0? "P00": join("s",i)), 2*(chunkSize_+1) + g) << " + " << signExtend( join("P",i+1,i+1), 2*(chunkSize_+1) + g)<<";"<<endl;
//				setSignalDelay(join("s",i+1), getCriticalPath());
//			}

			//now for the subtractions
			//these go all in parallel
			manageCriticalPath( target->localWireDelay() + target->adderDelay(2*(chunkSize_+1) + g));
			for (int i=chunks; i<2*chunks-1; i++){
				vhdl << tab << declare( join("s",i), 2*(chunkSize_+1) + g ) << " <= " << signExtend( join("s",chunks-1), 2*(chunkSize_+1) + g) << " - " << signExtend( (i==chunks? "P00": join("s",i-chunks)), 2*(chunkSize_+1) + g)<<";"<<endl;
				setSignalDelay( join("s",i), getCriticalPath());
			}

			manageCriticalPath( target->localWireDelay() + target->adderDelay(2*(chunkSize_+1) + g));
			//form the s'
			for (int i=0; i<2*chunks-1; i++){
				if (i==0){
					vhdl << tab << declare( join("sp",i), 3*chunkSize_) << " <= " << zeroExtend( "s0", 3*chunkSize_) << ";"<<endl;
				}else if (i==2*chunks-2){
					vhdl << tab << declare( join("sp",i), 3*chunkSize_ ) << " <= " << zeroExtend( join("s",i), 3*chunkSize_) << ";"<<endl;
				}else{
					// we need one more subtraction
					int p = (i < chunks? i: chunks-1); 
					int q = (i < chunks? 0: i-chunks+1);
					
					vhdl << tab << declare( join("tsp",i), 2*(chunkSize_+1) + g ) << " <= " << signExtend( join("s",i), 2*(chunkSize_+1) + g ) << " - " << signExtend( join("P",p,q) , 2*(chunkSize_+1) + g ) << ";" << endl;
					vhdl << tab << declare( join("sp",i), 3*chunkSize_) << " <= " << zeroExtend( join("tsp",i), 3*chunkSize_) <<";"<<endl; 
				}
			}
			
			//form the 3 operands to go in the multioperand adder
			
			//first determine how many will go into the first, so to get the signal size
			int size = 0;
			int i=0;
			while (i < 2*chunks-1){
				i+=3;
				size++;
			}
			int startIndex = i-3;
			cout << "Operand 0 startIndex="<<startIndex<<" and size is " << size<<endl;
			
			vhdl << tab << declare( "top0",  size*(3*chunkSize_)+ (startIndex==2*(chunks-1)?-chunkSize_:0) ) << " <= ";
			for (int i=startIndex; i>0; i=i-3){
					vhdl << join("sp",i)<< (startIndex==2*(chunks-1) && i==startIndex?range(2*chunkSize_-1,0):"") << " & "; 			
			}
			vhdl << join("sp",0)<<";"<<endl;
			
			//now this one is extended to output width size
			vhdl << tab << declare("op0", 2* (chunkSize_*chunks))<< "<= " << zeroExtend("top0", 2* (chunkSize_*chunks))<<";"<<endl;
			
			//now for the second one
			size = 0;
			i=1;
			while (i < 2*chunks-1){
				i+=3;
				size++;
			}
			startIndex = i-3;
			cout << "Operand 0 startIndex="<<startIndex<<" and size is " << size<<endl;

				
			vhdl << tab << declare( "top1",  size*(3*chunkSize_) + chunkSize_ + (startIndex==2*(chunks-1)?-chunkSize_:0) ) << " <= ";
			for (int i=startIndex; i>1; i=i-3){
					vhdl << join("sp",i)<< (startIndex==2*(chunks-1) && i==startIndex?range(2*chunkSize_-1,0):"") << " & "; 			
			}
			vhdl << join("sp",1)<<" & "<< zg(chunkSize_) << ";"<<endl;
			
			//now this one is extended to output width size
			vhdl << tab << declare("op1", 2* (chunkSize_*chunks))<< "<= " << zeroExtend("top1", 2* (chunkSize_*chunks))<<";"<<endl;
			

			//now for the second one
			size = 0;
			i=2;
			while (i < 2*chunks-1){
				i+=3;
				size++;
			}
			startIndex = i-3;
			cout << "Operand 0 startIndex="<<startIndex<<" and size is " << size<<endl;

				
			vhdl << tab << declare( "top2",  size*(3*chunkSize_) + 2*chunkSize_ + (startIndex==2*(chunks-1)?-chunkSize_:0)) << " <= ";
			for (int i=startIndex; i>2; i=i-3){
					vhdl << join("sp",i)<< (startIndex==2*(chunks-1) && i==startIndex?range(2*chunkSize_-1,0):"") << " & "; 			
			}
			vhdl << join("sp",2) <<" & "<< zg(2*chunkSize_) <<";"<<endl;
			
			//now this one is extended to output width size
			vhdl << tab << declare("op2", 2* (chunkSize_*chunks))<< "<= " << zeroExtend("top2", 2* (chunkSize_*chunks))<<";"<<endl;

			IntMultiAdder *ina = new IntMultiAdder( target, 2* (chunkSize_*chunks), 3, inDelayMap("X0", target->localWireDelay() + getCriticalPath()) );
			oplist.push_back(ina);
			
			inPortMap(ina, "X0", "op0");
			inPortMap(ina, "X1", "op1");
			inPortMap(ina, "X2", "op2");
			inPortMapCst(ina, "Cin", "'0'");
			
			outPortMap(ina, "R", "res");
			vhdl << tab << instance(ina, "FinalAdder") << endl;
			syncCycleFromSignal("res");
			setCriticalPath( ina->getOutputDelay("R"));
			
			vhdl << "R <= res;" << endl;
			
			outDelayMap["R"] = getCriticalPath();
		}
	}


	void IntKaratsuba::outputVHDL(std::ostream& o, std::string name) {
		licence(o);
		o << "library ieee; " << endl;
		o << "use ieee.std_logic_1164.all;" << endl;
		o << "use ieee.std_logic_arith.all;" << endl;
		o << "use ieee.std_logic_signed.all;" << endl;
		o << "library work;" << endl;
		outputVHDLEntity(o);
		newArchitecture(o,name);
		o << buildVHDLComponentDeclarations();	
		o << buildVHDLSignalDeclarations();
		beginArchitecture(o);		
		o<<buildVHDLRegisters();
		o << vhdl.str();
		endArchitecture(o);
	}


	IntKaratsuba::~IntKaratsuba() {
	}


	void IntKaratsuba::emulate(TestCase* tc){
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");
		mpz_class svR = svX * svY;
		tc->addExpectedOutput("R", svR);
	}

}
