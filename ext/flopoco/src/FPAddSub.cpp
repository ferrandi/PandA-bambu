/*
  Floating Point Adder for FloPoCo
 
  This file is part of the FloPoCo project 
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Authors:   Matei Istoan, Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, 2008-2012.
  All right reserved.

  */
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

#include "FPAddSub.hpp"

using namespace std;

//TODO +- inf for exponent => update exception

namespace flopoco{

#define DEBUGVHDL 0


FPAddSub::FPAddSub(Target* target, int wEX, int wFX, int wEY, int wFY, int wER, int wFR, map<string, double> inputDelays) :
		Operator(target), wEX(wEX), wFX(wFX), wEY(wEY), wFY(wFY), wER(wER), wFR(wFR) {

		srcFileName="FPAddSub";
			
		//parameter set up. For now all wEX=wEY=wER and the same holds for fractions
		wF = wFX;
		wE = wEX;
			
		ostringstream name;
		name<<"FPAddSub_"<<wE<<"_"<<wF<<"_uid"<<getNewUId(); 
		setName(name.str()); 

		setCopyrightString("Matei Istoan, Florent de Dinechin (2012)");		

		sizeRightShift = intlog2(wF+3);

		/* Set up the IO signals */
		/* Inputs: 2b(Exception) + 1b(Sign) + wEX bits (Exponent) + wFX bits(Fraction) */
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		addFPInput ("X", wE, wF);
		addFPInput ("Y", wE, wF);
		addFPOutput("Radd", wE, wF);
		addFPOutput("Rsub", wE, wF);

		//=========================================================================|
		//                          Swap/Difference                                |
		// ========================================================================|
		vhdl<<"-- Exponent difference and swap  --"<<endl;
		
		setCriticalPath(getMaxInputDelays(inputDelays));
		manageCriticalPath(target->localWireDelay(wE+wF+2) + target->lutDelay());

		vhdl << tab << declare("excExpFracX",2+wE+wF) << " <= X"<<range(wE+wF+2, wE+wF+1) << " & X"<<range(wE+wF-1, 0)<<";"<<endl;
		vhdl << tab << declare("excExpFracY",2+wE+wF) << " <= Y"<<range(wE+wF+2, wE+wF+1) << " & Y"<<range(wE+wF-1, 0)<<";"<<endl;
		
		setCriticalPath(getMaxInputDelays(inputDelays));
		manageCriticalPath(target->localWireDelay(wE+1) + target->adderDelay(wE+1));
		
		vhdl<< tab << declare("eXmeY",wE+1) << " <= (\"0\" & X"<<range(wE+wF-1,wF)<<") - (\"0\" & Y"<<range(wE+wF-1,wF)<<");"<<endl;
		vhdl<< tab << declare("eYmeX",wE+1) << " <= (\"0\" & Y"<<range(wE+wF-1,wF)<<") - (\"0\" & X"<<range(wE+wF-1,wF)<<");"<<endl;
		
		double cpeXmeY = getCriticalPath();
		
		setCriticalPath(getMaxInputDelays(inputDelays));
		
		if (wF < 30){
			manageCriticalPath(target->localWireDelay() + target->adderDelay(wE+wF+2)); //comparator delay implemented for now as adder
			vhdl<< tab << declare("swap")       << " <= '0' when excExpFracX >= excExpFracY else '1';"<<endl;
		}else{
			IntAdder *cmpAdder = new IntAdder(target, wE+wF+2+1, inDelayMap("X",getCriticalPath()));
			oplist.push_back(cmpAdder);
			
			vhdl << tab << declare("addCmpOp1",wE+wF+2+1) << "<= " << zg(1,0) << " & excExpFracX;"<<endl;
			vhdl << tab << declare("addCmpOp2",wE+wF+2+1) << "<= " << og(1,0) << " & not(excExpFracY);"<<endl;
			
			inPortMap(cmpAdder, "X", "addCmpOp1");
			inPortMap(cmpAdder, "Y", "addCmpOp2");
			inPortMapCst(cmpAdder, "Cin", "'1'");
			outPortMap (cmpAdder, "R", "cmpRes");
			
			vhdl << instance(cmpAdder, "cmpAdder") << endl;
			syncCycleFromSignal("cmpRes");
			setCriticalPath( cmpAdder->getOutputDelay("R") );
			vhdl<< tab << declare("swap")       << " <= cmpRes"<<of(wE+wF+2)<<";"<<endl;
		}
		
		double cpswap = getCriticalPath();
		
		manageCriticalPath(target->localWireDelay(wE+wF+3) + target->lutDelay());
		
		// depending on the value of swap, assign the corresponding values to the newX and newY signals 
		vhdl<<tab<<declare("newX",wE+wF+3) << " <= X     when swap = '0' else Y;"<<endl;
		vhdl<<tab<<declare("newY",wE+wF+3) << " <= Y     when swap = '0' else X;"<<endl;
		//break down the signals
		vhdl << tab << declare("expX",wE) << "<= newX"<<range(wE+wF-1,wF)<<";"<<endl;
		vhdl << tab << declare("excX",2)  << "<= newX"<<range(wE+wF+2,wE+wF+1)<<";"<<endl;
		vhdl << tab << declare("excY",2)  << "<= newY"<<range(wE+wF+2,wE+wF+1)<<";"<<endl;
		vhdl << tab << declare("signX")   << "<= newX"<<of(wE+wF)<<";"<<endl;
		vhdl << tab << declare("signY")   << "<= newY"<<of(wE+wF)<<";"<<endl;
		vhdl << tab << declare("diffSigns") << " <= signX xor signY;"<<endl;
		vhdl << tab << declare("sXsYExnXY",6) << " <= signX & signY & excX & excY;"<<endl; 
		//		vhdl << tab << declare("ExnXY",4) << " <= excX & excY;"<<endl; 
		
		manageCriticalPath(target->localWireDelay(wF+1) + target->lutDelay());
		
		vhdl << tab << declare("fracY",wF+1) << " <= "<< zg(wF+1)<<" when excY=\"00\" else ('1' & newY("<<wF-1<<" downto 0));"<<endl;
		double cpfracY = getCriticalPath();

		
		
		//exception bits
		manageCriticalPath(target->localWireDelay(6)+2*target->lutDelay());
		
		vhdl <<tab<<"with sXsYExnXY select "<<endl;
		vhdl <<tab<<declare("excRtRAdd",2) << " <= \"00\" when \"000000\"|\"010000\"|\"100000\"|\"110000\","<<endl
		<<tab<<tab<<"\"01\" when \"000101\"|\"010101\"|\"100101\"|\"110101\"|\"000100\"|\"010100\"|\"100100\"|\"110100\"|\"000001\"|\"010001\"|\"100001\"|\"110001\","<<endl
		<<tab<<tab<<"\"10\" when \"111010\"|\"001010\"|\"001000\"|\"011000\"|\"101000\"|\"111000\"|\"000010\"|\"010010\"|\"100010\"|\"110010\"|\"001001\"|\"011001\"|\"101001\"|\"111001\"|\"000110\"|\"010110\"|\"100110\"|\"110110\", "<<endl
		<<tab<<tab<<"\"11\" when others;"<<endl;
		
		vhdl <<tab<<"with sXsYExnXY select "<<endl;
		vhdl <<tab<<declare("excRtRSub",2) << " <= \"00\" when \"000000\"|\"010000\"|\"100000\"|\"110000\","<<endl
		<<tab<<tab<<"\"01\" when \"000101\"|\"010101\"|\"100101\"|\"110101\"|\"000100\"|\"010100\"|\"100100\"|\"110100\"|\"000001\"|\"010001\"|\"100001\"|\"110001\","<<endl
		<<tab<<tab<<"\"10\" when \"001000\"|\"011000\"|\"101000\"|\"111000\"|\"000010\"|\"010010\"|\"100010\"|\"110010\"|\"001001\"|\"011001\"|\"101001\"|\"111001\"|\"000110\"|\"010110\"|\"100110\"|\"110110\"|\"101010\"|\"011010\", "<<endl
		<<tab<<tab<<"\"11\" when others;"<<endl;
		
		manageCriticalPath(target->localWireDelay() + 2*target->lutDelay());
		
		
		vhdl <<tab<<declare("signRAdd") << "<= \'0\' when (sXsYExnXY=\"100000\" or sXsYExnXY=\"010000\") else signX;"<<endl;
		vhdl <<tab<<declare("signRSub") << "<= \'0\' when (sXsYExnXY=\"000000\" or sXsYExnXY=\"110000\") else (signX and (not swap)) or ((not signX) and swap);"<<endl;
		
		
		setCycleFromSignal("swap");;
		if ( getCycleFromSignal("eYmeX") == getCycleFromSignal("swap") )
			setCriticalPath(max(cpeXmeY, cpswap));
		else{
			if (syncCycleFromSignal("eYmeX"))
				setCriticalPath(cpeXmeY);
		}
		
		manageCriticalPath(target->localWireDelay(wE+1) + target->lutDelay());		//multiplexer
		
		vhdl<<tab<<declare("expDiff",wE+1) << " <= eXmeY when (swap = '0') else eYmeX;"<<endl; 
		
		manageCriticalPath(target->localWireDelay() + target->eqConstComparatorDelay(wE+1));
		
		vhdl<<tab<<declare("shiftedOut") << " <= '1' when (expDiff >= "<<wF+2<<") else '0';"<<endl;
		//shiftVal=the number of positions that fracY must be shifted to the right				
		
		
		if (wE>sizeRightShift) {
			manageCriticalPath(target->localWireDelay(sizeRightShift) + target->lutDelay());
			
			vhdl<<tab<<declare("shiftVal",sizeRightShift) << " <= expDiff("<< sizeRightShift-1<<" downto 0)"
				<< " when shiftedOut='0' else CONV_STD_LOGIC_VECTOR("<<wFX+3<<","<<sizeRightShift<<");" << endl; 
		}		
		else if (wE==sizeRightShift) {
			vhdl<<tab<<declare("shiftVal",sizeRightShift) << " <= expDiff;" << endl ;
		}
		else 	{ //  wE< sizeRightShift
			vhdl<<tab<<declare("shiftVal",sizeRightShift) << " <= CONV_STD_LOGIC_VECTOR(0,"<<sizeRightShift-wE <<") & expDiff;" <<	endl;
		}
		
		if ( getCycleFromSignal("fracY") == getCycleFromSignal("shiftVal") )
			setCriticalPath( max(cpfracY, getCriticalPath()) );
		else{
			if (syncCycleFromSignal("fracY"))
				setCriticalPath(cpfracY);
		}		
		
		// shift right the significand of new Y with as many positions as the exponent difference suggests (alignment) 
		rightShifter = new Shifter(target,wF+1,wF+3, Shifter::Right, inDelayMap("X",getCriticalPath()));
		rightShifter->changeName(getName()+"_RightShifter");
		oplist.push_back(rightShifter);
		inPortMap  (rightShifter, "X", "fracY");
		inPortMap  (rightShifter, "S", "shiftVal");
		outPortMap (rightShifter, "R","shiftedFracY");
		vhdl << instance(rightShifter, "RightShifterComponent");
		
		syncCycleFromSignal("shiftedFracY");
		setCriticalPath(rightShifter->getOutputDelay("R"));
		nextCycle();         
		setCriticalPath(0.0);
		double cpshiftedFracY = getCriticalPath();
		
		//sticky computation in parallel with addition, no need for manageCriticalPath
		//FIXME: compute inside shifter;
		//compute sticky bit as the or of the shifted out bits during the alignment //
		
		manageCriticalPath(target->localWireDelay() + target->eqConstComparatorDelay(wF+1));
		
		vhdl<<tab<< declare("sticky") << " <= '0' when (shiftedFracY("<<wF<<" downto 0)=CONV_STD_LOGIC_VECTOR(0,"<<wF<<")) else '1';"<<endl;
		
		double cpsticky = getCriticalPath();
		setCycleFromSignal("shiftedFracY");
		nextCycle();         
		setCriticalPath(0.0);
		setCriticalPath(cpshiftedFracY);
		
		//pad fraction of Y [overflow][shifted frac having inplicit 1][guard][round]
		vhdl<<tab<< declare("shiftedFracYext", wF+4) << " <= \"0\" & shiftedFracY("<<2*wF+3<<" downto "<<wF+1<<");"<<endl;	
		
		manageCriticalPath(target->localWireDelay(wF+4) + target->lutDelay());
		
		vhdl<<tab<< declare("fracYAdd", wF+4) << " <= shiftedFracYext;"<<endl;
		vhdl<<tab<< declare("fracYSub", wF+4) << " <= shiftedFracYext xor ( "<< wF+3 <<" downto 0 => \'1\');"<<endl;
		//pad fraction of X [overflow][inplicit 1][fracX][guard bits]				
		vhdl<<tab<< declare("fracX", wF+4) << " <= \"01\" & (newX("<<wF-1<<" downto 0)) & \"00\";"<<endl;
		
		if (getCycleFromSignal("sticky")==getCycleFromSignal("fracX"))
			setCriticalPath( max (cpsticky, getCriticalPath()) );
		else
			if (syncCycleFromSignal("sticky"))
				setCriticalPath(cpsticky);
		manageCriticalPath(target->localWireDelay(wF+5)+ target->lutDelay());
		
		vhdl << tab << declare("cInFracAdderSub") << " <= not sticky;" << endl;
		
		//mantisa addition
		fracAdder = new IntAdder(target,wF+4, inDelayMap("Y", getCriticalPath()));
		oplist.push_back(fracAdder);
		inPortMap  (fracAdder, "X", "fracX");
		inPortMap  (fracAdder, "Y", "fracYAdd");
		inPortMapCst  (fracAdder, "Cin", "\'0\'");
		outPortMap (fracAdder, "R","fracAdderResultAdd");
		vhdl << instance(fracAdder, "fracAdderAdd");
		
		//mantisa subtraction
		inPortMap  (fracAdder, "X", "fracX");
		inPortMap  (fracAdder, "Y", "fracYSub");
		inPortMap  (fracAdder, "Cin", "cInFracAdderSub");
		outPortMap (fracAdder, "R","fracAdderResultSub");
		vhdl << instance(fracAdder, "fracAdderSub");
		
		setCycleFromSignal("fracAdderResultSub");
		syncCycleFromSignal("fracAdderResultAdd");
		setCriticalPath(fracAdder->getOutputDelay("R"));
		
		if (getCycleFromSignal("sticky")==(getCycleFromSignal("fracAdderResultSub")>getCycleFromSignal("fracAdderResultAdd")?getCycleFromSignal("fracAdderResultSub"):getCycleFromSignal("fracAdderResultAdd")))
			setCriticalPath(max(cpsticky, getCriticalPath()));
		else{
			if (syncCycleFromSignal("sticky"))
				setCriticalPath(cpsticky);
		}	
		
		//incremented exponent.
		manageCriticalPath(target->localWireDelay(wF+2)+ target->adderDelay(wE+2));
		
		vhdl << tab << declare("extendedExp",wE+2) << "<= \"00\" & expX;"<<endl;
		vhdl << tab << declare("extendedExpInc",wE+2) << "<= (\"00\" & expX) + '1';"<<endl;
		
		syncCycleFromSignal("extendedExpInc");
		double cpextendedExp = getCriticalPath();
		
		

		
		//path for subtraction-------------------------------------------------------------------
		//shift in place
		manageCriticalPath(target->localWireDelay(wF+5)+ target->lutDelay());
		
		vhdl << tab << declare("fracGRSSub",wF+5) << "<= fracAdderResultSub & sticky; "<<endl;
				
		
		lzocs = new LZOCShifterSticky(target, wF+5, wF+5, intlog2(wF+5), false, 0, inDelayMap("I",getCriticalPath()));
		oplist.push_back(lzocs);
		inPortMap  (lzocs, "I", "fracGRSSub");
		outPortMap (lzocs, "Count","nZerosNew");
		outPortMap (lzocs, "O","shiftedFracSub");
		vhdl << instance(lzocs, "LZC_component");
		
		syncCycleFromSignal("shiftedFracSub");
		setCriticalPath(lzocs->getOutputDelay("O"));
		double cpshiftedFrac = getCriticalPath();
		
		//update exponent
		manageCriticalPath(target->localWireDelay(wF+2)+ target->adderDelay(wE+2) + target->lutDelay());
		
		vhdl << tab << declare("updatedExpSub",wE+2) << " <= extendedExpInc - (" << zg(wE+2-lzocs->getCountWidth(),0) <<" & nZerosNew);"<<endl;
		vhdl << tab << declare("eqdiffsign")<< " <= '1' when nZerosNew="<<og(lzocs->getCountWidth(),0)<<" else '0';"<<endl; 
		
		setCycleFromSignal("shiftedFracSub");
		
		//concatenate exponent with fraction to absorb the possible carry out
		manageCriticalPath(target->localWireDelay(wE+wF+3) + target->lutDelay());
		
		vhdl<<tab<<declare("expFracSub",wE+2+wF+1)<<"<= updatedExpSub & shiftedFracSub"<<range(wF+3,3)<<";"<<endl;
		double cpexpFrac = getCriticalPath();
		
		//at least in parallel with previous 2 statements
		setCycleFromSignal("shiftedFracSub");
		setCriticalPath(cpshiftedFrac);
		manageCriticalPath(target->localWireDelay() + target->lutDelay());
		
		vhdl<<tab<<declare("stkSub")<<"<= shiftedFracSub"<<of(1)<<" or shiftedFracSub"<<of(0)<<";"<<endl;
		vhdl<<tab<<declare("rndSub")<<"<= shiftedFracSub"<<of(2)<<";"<<endl;
		vhdl<<tab<<declare("grdSub")<<"<= shiftedFracSub"<<of(3)<<";"<<endl;
		vhdl<<tab<<declare("lsbSub")<<"<= shiftedFracSub"<<of(4)<<";"<<endl;
		
		setCycleFromSignal("shiftedFracSub");
		setCriticalPath(cpshiftedFrac);
		
		//decide what to add to the guard bit
		manageCriticalPath(target->localWireDelay() + 3*target->lutDelay());
		
		vhdl<<tab<<declare("addToRoundBitSub")<<"<= '0' when (lsbSub='0' and grdSub='1' and rndSub='0' and stkSub='0')  else '1';"<<endl;
				
		//round
		
		if (getCycleFromSignal("expFracSub") == getCycleFromSignal("addToRoundBitSub"))
			setCriticalPath(max(cpexpFrac, getCriticalPath()));
		else
			if (syncCycleFromSignal("expFracSub"))
				setCriticalPath(cpexpFrac);
		
		IntAdder *ra = new IntAdder(target, wE+2+wF+1, inDelayMap("X", getCriticalPath()));
		oplist.push_back(ra);
		
		inPortMap(ra,"X", "expFracSub");
		inPortMapCst(ra, "Y", zg(wE+2+wF+1,0) );
		inPortMap( ra, "Cin", "addToRoundBitSub");
		outPortMap( ra, "R", "RoundedExpFracSub");
		vhdl << instance(ra, "roundingAdderSub");
		
		syncCycleFromSignal("RoundedExpFracSub");
		setCriticalPath(ra->getOutputDelay("R"));
		
		//possible update to exception bits
		manageCriticalPath(target->localWireDelay(wE>wF?wE:wF) + target->lutDelay());
		
		vhdl << tab << declare("upExcSub",2)<<" <= RoundedExpFracSub"<<range(wE+wF+2,wE+wF+1)<<";"<<endl;
		vhdl << tab << declare("fracRSub",wF)<<" <= RoundedExpFracSub"<<range(wF,1)<<";"<<endl;
		vhdl << tab << declare("expRSub",wE) <<" <= RoundedExpFracSub"<<range(wF+wE,wF+1)<<";"<<endl;
		
		vhdl << tab << declare("excRtEffSub", 2) <<" <= excRtRAdd when (diffSigns=\'1\') else excRtRSub;"<<endl;

		manageCriticalPath(target->localWireDelay(2) + 2*target->lutDelay());
		
		vhdl << tab << declare("exExpExcSub",4) << " <= upExcSub & excRtEffSub;"<<endl;
		vhdl << tab << "with (exExpExcSub) select "<<endl;
		vhdl << tab << declare("excRt2Sub",2) << "<= \"00\" when \"0000\"|\"0100\"|\"1000\"|\"1100\"|\"1001\"|\"1101\","<<endl
		<<tab<<tab<<"\"01\" when \"0001\","<<endl
		<<tab<<tab<<"\"10\" when \"0010\"|\"0110\"|\"0101\","<<endl
		<<tab<<tab<<"\"11\" when others;"<<endl;
		
		syncCycleFromSignal("excRt2Sub");
		manageCriticalPath(target->localWireDelay(2) + target->lutDelay());
		
		vhdl<<tab<<declare("excRSub",2) << " <= \"00\" when (eqdiffsign='1') else excRt2Sub;"<<endl;
		
		// assign result 
		manageCriticalPath(target->localWireDelay(wE+wF) + target->lutDelay());
		
		vhdl<<tab<< declare("computedRSub",wE+wF) << " <= expRSub & fracRSub;"<<endl;
		vhdl << tab << "Rsub <= excRSub & signRSub & computedRSub when (diffSigns=\'0\') else excRAdd & signRSub & computedRAdd;"<<endl;
		
		double cpSub = getCriticalPath();
		//-------------------------------------------------------------------------------------
		
		
		
		
		
		
		
		
		
		//path for addition-----------------------------------------------------------------
		//shift in place
		setCriticalPath(cpextendedExp);
		setCycleFromSignal("extendedExp");
		
		manageCriticalPath(target->localWireDelay(wF+5)+ target->lutDelay());
		
		vhdl << tab << declare("fracGRSAdd",wF+5) << "<= fracAdderResultAdd & sticky; "<<endl;
		
		syncCycleFromSignal("extendedExpInc");
		
		vhdl << tab << declare("updatedFracAdd",wF+1) << " <= fracGRSAdd" << range(wF+3, 3) << " when (fracAdderResultAdd(" << wF+3 << ")=\'1\') else fracGRSAdd"<< range(wF+2, 2) <<";" << endl;
		
		//update exponent
		manageCriticalPath(target->localWireDelay(wE+2) + target->lutDelay());
		
		vhdl << tab << declare("updatedExpAdd",wE+2) << " <= extendedExpInc when (fracAdderResultAdd(" << wF+3 << ")=\'1\') else extendedExp;"<<endl;
		
		//concatenate exponent with fraction to absorb the possible carry out
		manageCriticalPath(target->localWireDelay(wE+wF+3) + target->lutDelay());
		
		vhdl<<tab<<declare("expFracAdd",wE+2+wF+1)<<"<= updatedExpAdd & updatedFracAdd;"<<endl;
		cpexpFrac = getCriticalPath();
		
		
		//at least in parallel with previous 2 statements
		setCycleFromSignal("fracGRSAdd");
		setCriticalPath(cpshiftedFrac);
		manageCriticalPath(target->localWireDelay() + target->lutDelay());
		
		vhdl<<tab<<declare("stkAdd")<<"<= fracGRSAdd"<<of(1)<<" or fracGRSAdd"<<of(0)<<";"<<endl;
		vhdl<<tab<<declare("rndAdd")<<"<= fracGRSAdd"<<of(2)<<";"<<endl;
		vhdl<<tab<<declare("grdAdd")<<"<= fracGRSAdd"<<of(3)<<";"<<endl;
		vhdl<<tab<<declare("lsbAdd")<<"<= fracGRSAdd"<<of(4)<<";"<<endl;
		
		//decide what to add to the guard bit
		manageCriticalPath(target->localWireDelay() + 2*target->lutDelay());
		
		vhdl<<tab<<declare("addToRoundBitAdd")<<"<= (grdAdd and rndAdd) or (grdAdd and (not rndAdd) and lsbAdd) or ((not grdAdd) and rndAdd and stkAdd) or (grdAdd and (not rndAdd) and stkAdd);"<<endl;
		
		//round
		
		if (getCycleFromSignal("expFracAdd") == getCycleFromSignal("addToRoundBitAdd"))
			setCriticalPath(max(cpexpFrac, getCriticalPath()));
		else
			if (syncCycleFromSignal("expFracAdd"))
				setCriticalPath(cpexpFrac);
		
		inPortMap(ra,"X", "expFracAdd");
		inPortMapCst(ra, "Y", zg(wE+2+wF+1,0) );
		inPortMap( ra, "Cin", "addToRoundBitAdd");
		outPortMap( ra, "R", "RoundedExpFracAdd");
		vhdl << instance(ra, "roundingAdderAdd");
		
		setCycleFromSignal("RoundedExpFracAdd");
		setCriticalPath(ra->getOutputDelay("R"));
		
		//possible update to exception bits
		
		manageCriticalPath(target->localWireDelay(wE>wF?wE:wF));
		
		vhdl << tab << declare("upExcAdd",2)<<" <= RoundedExpFracAdd"<<range(wE+wF+2,wE+wF+1)<<";"<<endl;
		vhdl << tab << declare("fracRAdd",wF)<<" <= RoundedExpFracAdd"<<range(wF,1)<<";"<<endl;
		vhdl << tab << declare("expRAdd",wE) <<" <= RoundedExpFracAdd"<<range(wF+wE,wF+1)<<";"<<endl;
		
		manageCriticalPath(target->localWireDelay(4) + 3*target->lutDelay());
		
		vhdl << tab << declare("excRtEffAdd", 2) <<" <= excRtRAdd when (diffSigns=\'0\') else excRtRSub;"<<endl;
		
		vhdl << tab << declare("exExpExcAdd",4) << " <= upExcAdd & excRtEffAdd;"<<endl;
		vhdl << tab << "with (exExpExcAdd) select "<<endl;
		vhdl << tab << declare("excRt2Add",2) << "<= \"00\" when \"0000\"|\"0100\"|\"1000\"|\"1100\"|\"1001\","<<endl
		<<tab<<tab<<"\"01\" when \"0001\","<<endl
		<<tab<<tab<<"\"10\" when \"0010\"|\"0110\"|\"0101\","<<endl
		<<tab<<tab<<"\"11\" when others;"<<endl;
		
		syncCycleFromSignal("excRt2Add");
		manageCriticalPath(target->localWireDelay(wE+wF+3) + target->lutDelay());
		
		vhdl<<tab<<declare("excRAdd",2) << " <=  excRt2Add;"<<endl;
		
		// assign result 
		vhdl<<tab<< declare("computedRAdd",wE+wF) << " <= expRAdd & fracRAdd;"<<endl;
		vhdl << tab << "Radd <= excRAdd & signRAdd & computedRAdd when (diffSigns=\'0\') else excRSub & signRAdd & computedRSub;"<<endl;
		
		double cpAdd = getCriticalPath();		
		//--------------------------------------------------------------------------------------
		
		if (getCycleFromSignal("computedRAdd") == getCycleFromSignal("computedRSub"))
			setCriticalPath(max(cpAdd, cpSub));
		else
			if (syncCycleFromSignal("computedRSub"))
				setCriticalPath(cpSub);
		
	}

	FPAddSub::~FPAddSub() {
	}


	void FPAddSub::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");
	
		/* Compute correct value */
		FPNumber fpx(wEX, wFX), fpy(wEY, wFY);
		fpx = svX;
		fpy = svY;
		mpfr_t x, y, radd, rsub;
		mpfr_init2(x, 1+wFX);
		mpfr_init2(y, 1+wFY);
		mpfr_init2(radd, 1+wFR); 
		mpfr_init2(rsub, 1+wFR); 
		fpx.getMPFR(x);
		fpy.getMPFR(y);
		mpfr_add(radd, x, y, GMP_RNDN);
		mpfr_sub(rsub, x, y, GMP_RNDN);

		// Set outputs 
		FPNumber  fpradd(wER, wFR, radd);
		mpz_class svRadd = fpradd.getSignalValue();
		tc->addExpectedOutput("Radd", svRadd);
		
		FPNumber  fprsub(wER, wFR, rsub);
		mpz_class svRsub = fprsub.getSignalValue();
		tc->addExpectedOutput("Rsub", svRsub);

		// clean up
		mpfr_clears(x, y, radd, rsub, NULL);
	}





	void FPAddSub::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		// Regression tests 
		tc = new TestCase(this); 
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", -1.0);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", FPNumber::plusDirtyZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", 1.0);
		tc->addFPInput("Y", FPNumber::minusDirtyZero);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", FPNumber::plusInfty);
		tc->addFPInput("Y", FPNumber::minusInfty);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", FPNumber::plusInfty);
		tc->addFPInput("Y", FPNumber::plusInfty);
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addFPInput("X", FPNumber::minusInfty);
		tc->addFPInput("Y", FPNumber::minusInfty);
		emulate(tc);
		tcl->add(tc);
	
	}



	TestCase* FPAddSub::buildRandomTestCase(int i){

		TestCase *tc;
		mpz_class x,y;
		mpz_class normalExn = mpz_class(1)<<(wE+wF+1);
		mpz_class negative  = mpz_class(1)<<(wE+wF);

		tc = new TestCase(this); 
		/* Fill inputs */
		if ((i & 7) == 0) {// cancellation, same exponent
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn;
			y  = getLargeRandom(wF) + (e << wF) + normalExn + negative;
		}
		else if ((i & 7) == 1) {// cancellation, exp diff=1
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn;
			e++; // may rarely lead to an overflow, who cares
			y  = getLargeRandom(wF) + (e << wF) + normalExn + negative;
		}
		else if ((i & 7) == 2) {// cancellation, exp diff=1
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn + negative;
			e++; // may rarely lead to an overflow, who cares
			y  = getLargeRandom(wF) + (e << wF) + normalExn;
		}
		else if ((i & 7) == 3) {// alignment within the mantissa sizes
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn + negative;
			e +=	getLargeRandom(intlog2(wF)); // may lead to an overflow, who cares
			y  = getLargeRandom(wF) + (e << wF) + normalExn;
		}
		else if ((i & 7) == 4) {// subtraction, alignment within the mantissa sizes
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn;
			e +=	getLargeRandom(intlog2(wF)); // may lead to an overflow
			y  = getLargeRandom(wF) + (e << wF) + normalExn + negative;
		}
		else if ((i & 7) == 5 || (i & 7) == 6) {// addition, alignment within the mantissa sizes
			mpz_class e = getLargeRandom(wE);
			x  = getLargeRandom(wF) + (e << wF) + normalExn;
			e +=	getLargeRandom(intlog2(wF)); // may lead to an overflow
			y  = getLargeRandom(wF) + (e << wF) + normalExn;
		}
		else{ //fully random
			x = getLargeRandom(wE+wF+3);
			y = getLargeRandom(wE+wF+3);
		}
		// Random swap
		mpz_class swap = getLargeRandom(1);
		if (swap == mpz_class(0)) {
			tc->addInput("X", x);
			tc->addInput("Y", y);
		}
		else {
			tc->addInput("X", y);
			tc->addInput("Y", x);
		}
		/* Get correct outputs */
		emulate(tc);
		return tc;
	}

}
