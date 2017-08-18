/*
  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon
  
  Authors :  Radu Tudoran, Bogdan Pasca

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.
 
*/

#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <string.h>

#include <gmp.h>


#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

#include "Fix2FP.hpp"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <math.h>
#include <locale>

#include <stdio.h>
#include <mpfr.h>

using namespace std;

namespace flopoco{

#define DEBUGVHDL 0


	Fix2FP::Fix2FP(Target* target, int LSBI, int MSBI, int Signed,int wER, int wFR) :
		Operator(target), MSBI(MSBI), LSBI(LSBI), Signed(Signed),wER(wER), wFR(wFR) {

		ostringstream name;
	
		if ((MSBI < LSBI)){
			cerr << " Fix2FP: Input constraint LSB <= MSB not met."<<endl;
			exit (EXIT_FAILURE);
		}
	
		mpz_class maxExpWE = mpz_class(1)<<(wER-1);
		mpz_class minExpWE = 1 - maxExpWE;
	
		if (( maxExpWE < MSBI ) || ( minExpWE > LSBI)){
			cerr << " The exponent is too small for full coverage. Try increasing the exponent !"<<endl;
			exit (EXIT_FAILURE);
		}
	

		int absMSB = MSBI>=0?MSBI:-MSBI;
		int absLSB = LSBI>=0?LSBI:-LSBI;
		name<<"Fix2FP_"<< (LSBI<0?"M":"")<<absLSB<<"_"<<(MSBI<0?"M":"")<<absMSB <<"_"<< (Signed==1?"S":"US") << "_" <<wER<<"_"<<wFR; 
		setName(name.str()); 

		setCopyrightString("Radu Tudoran, Bogdan Pasca (2009)");		

		//parameter set up
		wF = wFR;
		wE = wER;

		if(MSBI>0)
			MSB=MSBI+1;
		else
			MSB=MSBI;

		LSB=LSBI;
				
		/* Set up the IO signals */
		
		addInput ("I", MSB-LSB);
		addFPOutput("O", wE,wF);
	
		/*	VHDL code description	*/
	
		inputWidth=MSB-LSB;
	
		vhdl << tab << declare("input",inputWidth) << " <= I;"<<endl;
	
		// code for the LZOCShifter part
	
		if(Signed!=0)
			vhdl << tab << declare("signSignal")<<"<=input"<<of(MSB-1-LSB)<<";"<<endl;
		else
			vhdl << tab << declare("signSignal")<<"<= '0';"<<endl;
	
		vhdl << tab << declare("passedInput",inputWidth)<<"<=input"<<range(MSB-1 -LSB,0)<<";"<<endl;
		vhdl << tab << declare("input2LZOC",inputWidth-1)<<"<=passedInput"<<range(MSB-2 -LSB,0)<<";"<<endl;
	
		if ( MSB-LSB > wF ){ //ROUNDING NEEDED
		
			//code for the case when a rounding is required
			int maximalOutputValue = (MSB-LSB)>wF+4? MSB-LSB:wF+4;
	
			if(Signed!=0){	
				lzocs = new LZOCShifterSticky(target,inputWidth-1 , maximalOutputValue, intlog2(inputWidth-1), 0, -1);
				lzocs->changeName(getName()+"_LZOCS");
				oplist.push_back(lzocs);

				inPortMap  (lzocs, "I", "input2LZOC");
				inPortMap  (lzocs,"OZb","signSignal");
				outPortMap (lzocs, "Count","temporalExponent");
				outPortMap (lzocs, "O","temporalFraction");
				vhdl << instance(lzocs, "LZOC_component");
	
				sizeExponentValue=lzocs->getCountWidth();
			}else{
		
				lzcs = new LZOCShifterSticky(target, inputWidth , maximalOutputValue, intlog2(inputWidth), 0, 0);
				lzcs->changeName(getName()+"_LZCS");
				oplist.push_back(lzcs);
			
				inPortMap  (lzcs, "I", "passedInput");
				outPortMap (lzcs, "Count","temporalExponent");
				outPortMap (lzcs, "O","temporalFraction");
				vhdl << instance(lzcs, "LZC_component");
	
				sizeExponentValue=lzcs->getCountWidth();
			}
			sizeFractionPlusOne=wF+1;
		
			syncCycleFromSignal("temporalExponent");
	
			//Code for creating the exponent
			if(Signed!=0)
				vhdl << tab << declare("MSB2Signal",wE)<<"<=CONV_STD_LOGIC_VECTOR("<<MSB-2<<","<<wE<<");"<<endl;
			else
				vhdl << tab << declare("MSB2Signal",wE)<<"<=CONV_STD_LOGIC_VECTOR("<<MSB-1<<","<<wE<<");"<<endl;
	
			if(Signed!=0)
				vhdl << tab << declare("zeroPadding4Exponent",wE- intlog2(inputWidth-1),true)<<"<=CONV_STD_LOGIC_VECTOR(0,"<<wE- intlog2(inputWidth-1)<<");"<<endl;
			else
				vhdl << tab << declare("zeroPadding4Exponent",wE- intlog2(inputWidth),true)<<"<=CONV_STD_LOGIC_VECTOR(0,"<<wE- intlog2(inputWidth)<<");"<<endl;
	
			vhdl << tab << declare("valueExponent",wE)<<"<= not (zeroPadding4Exponent & temporalExponent );"<<endl;
	
			exponentConversion = new IntAdder(target,wE);
			exponentConversion->changeName(getName()+"exponentConversion");
			oplist.push_back(exponentConversion);
			inPortMap  (exponentConversion, "X", "MSB2Signal");
			inPortMap  (exponentConversion, "Y", "valueExponent");
			inPortMapCst(exponentConversion, "Cin", "'1'");
			outPortMap (exponentConversion, "R","partialConvertedExponent");
			vhdl << instance(exponentConversion, "exponentConversion");
	
			syncCycleFromSignal("partialConvertedExponent");
	
			vhdl << tab << declare("biassOfOnes",wE-1)<<"<=CONV_STD_LOGIC_VECTOR("<<pow(double(2),wE)-1<<","<<wE-1<<");"<<endl;
			vhdl << tab << declare("biassSignal",wE)<<"<='0' & biassOfOnes;"<<endl;
			vhdl << tab << declare("biassSignalBit",wE+1)<<"<='0' & biassSignal;"<<endl;
			vhdl << tab << declare("partialConvertedExponentBit",wE+1)<<"<= '0' & partialConvertedExponent;"<<endl;
			vhdl << tab << declare("sign4OU")<<"<=partialConvertedExponent"<<of(wE-1)<<";"<<endl;
	
			exponentFinal = new IntAdder(target,wE+1);
			exponentFinal->changeName(getName()+"exponentFinal");
			oplist.push_back(exponentFinal);
			inPortMap  (exponentFinal, "X", "partialConvertedExponentBit");
			inPortMap  (exponentFinal, "Y", "biassSignalBit");
			inPortMapCst(exponentFinal, "Cin", "'0'");
			outPortMap (exponentFinal, "R","convertedExponentBit");
			vhdl << instance(exponentFinal, "exponentFinal");
	
			syncCycleFromSignal("convertedExponentBit");
	
			vhdl << tab << declare("convertedExponent",wE)<<"<= convertedExponentBit"<<range(wE-1,0)<<";"<<endl;
	
			vhdl << tab << declare("underflowSignal")<<"<= '1' when (sign4OU='1' and convertedExponentBit"<<range(wE,wE-1)<<" = \"01\" ) else '0' ;"<<endl;
			vhdl << tab << declare("overflowSignal")<<"<= '1' when (sign4OU='0' and convertedExponentBit"<<range(wE,wE-1)<<" = \"10\" ) else '0' ;"<<endl;
	
			//code for verifing if the number is zero
	
			setCycleFromSignal("passedInput");
	
			if(Signed!=0){
				vhdl << tab << declare("minusOne4ZD",MSB -LSB)<<"<=CONV_STD_LOGIC_VECTOR("<<-1<<","<<MSB-LSB<<");"<<endl;
	
				zeroD = new IntAdder(target, MSB-LSB );
				zeroD->changeName(getName()+"zeroD");
				oplist.push_back(zeroD);
				inPortMap  (zeroD, "X", "passedInput");
				inPortMap  (zeroD, "Y", "minusOne4ZD");
				inPortMapCst(zeroD, "Cin", "'0'");
				outPortMap (zeroD, "R","zeroDS");
				vhdl << instance(zeroD, "zeroD");
	
				syncCycleFromSignal("zeroDS");
	
				vhdl << tab << declare("zeroInput")<<"<= zeroDS"<<of(MSB-LSB-1)<<" and not(signSignal);"<<endl;
			}else{

				vhdl << tab << declare("minusOne4ZD",MSB -LSB+1)<<"<=CONV_STD_LOGIC_VECTOR("<<-1<<","<<MSB-LSB+1<<");"<<endl;
				vhdl << tab << declare("passedInputBit",MSB-LSB+1)<<"<= '0' & passedInput;"<<endl;
				zeroD = new IntAdder(target, MSB-LSB +1);
				zeroD->changeName(getName()+"zeroD");
				oplist.push_back(zeroD);
				inPortMap  (zeroD, "X", "passedInputBit");
				inPortMap  (zeroD, "Y", "minusOne4ZD");
				inPortMapCst(zeroD, "Cin", "'0'");
				outPortMap (zeroD, "R","zeroDS");
				vhdl << instance(zeroD, "zeroD");
	
				setCycleFromSignal("zeroDS");
	
				vhdl << tab << declare("zeroInput")<<"<= zeroDS"<<of(MSB-LSB)<<" and not (signSignal);"<<endl;
			}

			//code for the Convertion of the fraction
			setCycleFromSignal("temporalFraction");
	
			if(Signed!=0){
	
				vhdl << tab << declare("sign2vector",maximalOutputValue)<<"<=(others => signSignal);"<<endl;
				vhdl << tab << declare("tempConvert",maximalOutputValue)<<"<= sign2vector xor temporalFraction;"<<endl;
				vhdl << tab << declare("tempConvert0",maximalOutputValue+1)<<"<= '0' & tempConvert;"<<endl;
				vhdl << tab << declare("tempPaddingAddSign",maximalOutputValue)<<"<=(others=>'0');"<<endl;
				vhdl << tab << declare("tempAddSign",maximalOutputValue+1)<<"<=tempPaddingAddSign & signSignal;"<<endl;

				//Integer adder for obtaining the fraction value
				fractionConvert = new IntAdder(target,maximalOutputValue+1);
				fractionConvert->changeName(getName()+"_fractionConvert");
				oplist.push_back(fractionConvert);
				inPortMap  (fractionConvert, "X", "tempConvert0");
				inPortMap  (fractionConvert, "Y", "tempAddSign");
				inPortMapCst(fractionConvert, "Cin", "'0'");
				outPortMap (fractionConvert, "R","tempFractionResult");
				vhdl << instance(fractionConvert, "fractionConverter");
	
				syncCycleFromSignal("tempFractionResult");
			}else{
				vhdl << tab << declare("tempFractionResult",maximalOutputValue+1)<<"<= '0' & temporalFraction;"<<endl;
			}
	
			vhdl << tab << declare("correctingExponent")<<"<=tempFractionResult"<<of(maximalOutputValue)<<";"<<endl;
			vhdl << tab << declare("fractionConverted",wF)<<"<=tempFractionResult"<<range(maximalOutputValue-2,maximalOutputValue -wF-1)<<";"<<endl;
	
			vhdl << tab << declare("firstBitofRest")<<"<=tempFractionResult"<<of(maximalOutputValue-wF-2)<<";"<<endl;
	
			//selection of mux 3
			vhdl << tab << declare("lastBitOfFraction")<<"<=tempFractionResult"<<of(maximalOutputValue-wF-1)<<";"<<endl;
	
			//Zero Compare of the bits from the remainder of magnitude
	
			int sizeOfRemainder=maximalOutputValue-sizeFractionPlusOne-1;
	
			setCycleFromSignal("tempFractionResult");
			vhdl << tab << declare("minusOne",sizeOfRemainder)<<"<=CONV_STD_LOGIC_VECTOR("<<-1<<","<<sizeOfRemainder<<");"<<endl;
			vhdl << tab << declare("fractionRemainder",sizeOfRemainder)<<"<= tempFractionResult"<<range(sizeOfRemainder-1,0)<<";"<<endl;
			oneSubstracter = new IntAdder(target,sizeOfRemainder);
			oneSubstracter->changeName(getName()+"_oneSubstracter");
			oplist.push_back(oneSubstracter);
			inPortMap  (oneSubstracter, "X", "fractionRemainder");
			inPortMap  (oneSubstracter, "Y", "minusOne");
			inPortMapCst(oneSubstracter, "Cin", "'0'");
			outPortMap (oneSubstracter, "R","zeroFractionResult");
			vhdl << instance(oneSubstracter, "oneSubstracter");
	
			syncCycleFromSignal("zeroFractionResult");
	
			vhdl << tab << declare("zeroRemainder")<<"<= not( not (tempFractionResult"<<of(sizeOfRemainder-1)<<") and zeroFractionResult"<<of(sizeOfRemainder-1)<<");"<<endl;
	
			// signals for Muxes
	
			if(inputWidth>32&&target->frequencyMHz()>=250)
				nextCycle();
	
			//selection of mux 3
			vhdl << tab << declare("outputOfMux3")<<"<=lastBitOfFraction;"<<endl;
			vhdl << tab << "with zeroRemainder select "<<endl
			     << tab << declare("outputOfMux2")<<" <= outputOfMux3 when '0', '1' when others;"<<endl;
			vhdl << tab << "with firstBitofRest select "<<endl
			     << tab << declare("outputOfMux1")<<" <= outputOfMux2 when '1', '0' when others;"<<endl;
			vhdl << tab << declare("possibleCorrector4Rounding",wF+wE+1)<<"<=CONV_STD_LOGIC_VECTOR(0,"<<wE<<") & correctingExponent & CONV_STD_LOGIC_VECTOR(0,"<<wF<<");"<<endl;
			vhdl << tab << declare("concatenationForRounding",wE+wF+1)<<"<= '0' & convertedExponent & fractionConverted;"<<endl;
	
			vhdl << tab << declare("testC",wE+wF+1)<<"<= concatenationForRounding;"<<endl;
			vhdl << tab << declare("testR",wE+wF+1)<<"<= possibleCorrector4Rounding;"<<endl;
			vhdl << tab << declare("testM")<<"<= outputOfMux1;"<<endl;
	
	
			roundingAdder = new IntAdder(target,wF+wE+1);
			roundingAdder->changeName(getName()+"roundingAdder");
			oplist.push_back(roundingAdder);
			inPortMap  (roundingAdder, "X", "concatenationForRounding");
			inPortMap  (roundingAdder, "Y", "possibleCorrector4Rounding");
			inPortMap  (roundingAdder, "Cin", "outputOfMux1");
			outPortMap (roundingAdder, "R","roundedResult");
			vhdl << instance(roundingAdder, "roundingAdder");
	
			syncCycleFromSignal("roundedResult");
	
	
			vhdl << tab << declare("convertedExponentAfterRounding",wE)<<"<= roundedResult"<<range(wE+wF-1,wF)<<";"<<endl;
			vhdl << tab << declare("convertedFractionAfterRounding",wF)<<"<= roundedResult"<<range(wF-1,0)<<";"<<endl;
	
			vhdl << tab << declare("MSBSelection")<<"<= overflowSignal or roundedResult"<<of(wF+wE)<<";"<<endl;
			vhdl << tab << declare("LSBSelection")<<"<= not(underflowSignal and not(zeroInput));"<<endl;
			vhdl << tab << declare("Selection",2)<<"<= MSBSelection & LSBSelection when zeroInput='0' else \"00\";"<<endl;
			vhdl << tab << declare("specialBits",2)<<" <= Selection;"<<endl;
	
			//assembling the result

			vhdl << tab << "O<= specialBits & signSignal & convertedExponentAfterRounding & convertedFractionAfterRounding;"<<endl;
		} 
		//==========================================================================
		//==========================================================================
		else //ROUNDING NOT NEEDED
			{
	
				int maximalOutputValue = wF+1;
		
				//code for zero detector of the input

				if(Signed!=0){
					vhdl << tab << declare("minusOne4ZD",MSB -LSB)<<"<=CONV_STD_LOGIC_VECTOR("<<-1<<","<<MSB-LSB<<");"<<endl;
	
					zeroD = new IntAdder(target, MSB-LSB );
					zeroD->changeName(getName()+"zeroD");
					oplist.push_back(zeroD);
					inPortMap  (zeroD, "X", "passedInput");
					inPortMap  (zeroD, "Y", "minusOne4ZD");
					inPortMapCst(zeroD, "Cin", "'0'");
					outPortMap (zeroD, "R","zeroDS");
					vhdl << instance(zeroD, "zeroD");
	
					setCycleFromSignal("zeroDS");
	
					vhdl << tab << declare("zeroInput")<<"<= zeroDS"<<of(MSB-LSB-1)<<" and not (signSignal);"<<endl;
				}else{
					vhdl << tab << declare("minusOne4ZD",MSB -LSB+1)<<"<=CONV_STD_LOGIC_VECTOR("<<-1<<","<<MSB-LSB+1<<");"<<endl;
					vhdl << tab << declare("passedInputBit",MSB-LSB+1)<<"<= '0' & passedInput;"<<endl;
					zeroD = new IntAdder(target, MSB-LSB +1);
					zeroD->changeName(getName()+"zeroD");
					oplist.push_back(zeroD);
					inPortMap  (zeroD, "X", "passedInputBit");
					inPortMap  (zeroD, "Y", "minusOne4ZD");
					inPortMapCst(zeroD, "Cin", "'0'");
					outPortMap (zeroD, "R","zeroDS");
					vhdl << instance(zeroD, "zeroD");
	
					setCycleFromSignal("zeroDS");
	
					vhdl << tab << declare("zeroInput")<<"<= zeroDS"<<of(MSB-LSB)<<" and not (signSignal);"<<endl;
				}

				//code for the leading zeros/ones
				setCycleFromSignal("input2LZOC");
	
				if(Signed!=0){
					lzocs		= new LZOCShifterSticky(target,inputWidth-1 , maximalOutputValue, intlog2(inputWidth-1), 0, -1);
					lzocs->changeName(getName()+"_LZCS");
					oplist.push_back(lzocs);
					inPortMap  (lzocs, "I", "input2LZOC");
					inPortMap	(lzocs,"OZb","signSignal");
					outPortMap (lzocs, "Count","temporalExponent");
					outPortMap (lzocs, "O","temporalFraction");
					vhdl << instance(lzocs, "LZOC_component");
	
					sizeExponentValue=lzocs->getCountWidth();
				}else{
					lzcs = new LZOCShifterSticky(target, inputWidth , maximalOutputValue, intlog2(inputWidth), 0, 0);
					lzcs->changeName(getName()+"_LZCS");
					oplist.push_back(lzcs);
					inPortMap  (lzcs, "I", "passedInput");
					outPortMap (lzcs, "Count","temporalExponent");
					outPortMap (lzcs, "O","temporalFraction");
					vhdl << instance(lzcs, "LZC_component");
	
					sizeExponentValue=lzcs->getCountWidth();
				}	
				sizeFractionPlusOne=wF+1;
	
				syncCycleFromSignal("temporalExponent");

				//code for the fraction
				setCycleFromSignal("temporalFraction");
	
				int sizeFractionPlusOne=wF+1;

				if(Signed!=0){
					vhdl << tab << declare("tfr",sizeFractionPlusOne) <<"<= temporalFraction"<<range(maximalOutputValue-1,maximalOutputValue-sizeFractionPlusOne)<<";"<<endl;
		
					vhdl << tab << declare("sign2vector",sizeFractionPlusOne)<<"<=(others=>signSignal);"<<endl;
					vhdl << tab << declare("tempConvert",sizeFractionPlusOne)<<"<=sign2vector xor tfr;"<<endl;
					vhdl << tab << declare("tempPaddingAddSign",sizeFractionPlusOne)<<"<=(others=>'0');"<<endl;
					vhdl << tab << declare("tempAddSign",sizeFractionPlusOne+1)<<"<=tempPaddingAddSign & signSignal;"<<endl;
					vhdl << tab << declare("tempConvert0",sizeFractionPlusOne+1)<<"<= '0' & tempConvert;"<<endl;
	
					//Integer adder for obtaining the fraction value
	
					fractionConvert = new IntAdder(target,sizeFractionPlusOne+1);
					fractionConvert->changeName(getName()+"_fractionConvert");
					oplist.push_back(fractionConvert);
					inPortMap  (fractionConvert, "X", "tempConvert0");
					inPortMap  (fractionConvert, "Y", "tempAddSign");
					inPortMapCst(fractionConvert, "Cin", "'0'"); 
					outPortMap (fractionConvert, "R","tempFractionResult");
					vhdl << instance(fractionConvert, "fractionConverter");
	
					syncCycleFromSignal("tempFractionResult");
				}else{
					vhdl << tab << declare("tempFractionResult",maximalOutputValue+1)<<"<= '0' & temporalFraction;"<<endl;
				}
	
				vhdl << tab << declare("correctingExponent")<<"<=tempFractionResult"<<of(sizeFractionPlusOne)<<";"<<endl;
				vhdl << tab << declare("convertedFraction",wF)<<"<=tempFractionResult"<<range(wF-1,0)<<";"<<endl;
	
				//code for creating the exponent
				setCycleFromSignal("temporalExponent");
				if(Signed!=0)
					vhdl << tab << declare("MSB2Signal",wE,true)<<"<= CONV_STD_LOGIC_VECTOR("<<MSB-2<<","<<wE<<");"<<endl;
				else
					vhdl << tab << declare("MSB2Signal",wE,true)<<"<= CONV_STD_LOGIC_VECTOR("<<MSB-1<<","<<wE<<");"<<endl;
	
				if(Signed!=0)
					vhdl << tab << declare("zeroPadding4Exponent",wE- intlog2(inputWidth-1),true)<<"<= CONV_STD_LOGIC_VECTOR(0,"<<wE- intlog2(inputWidth-1)<<");"<<endl;
				else
					vhdl << tab << declare("zeroPadding4Exponent",wE- intlog2(inputWidth),true)<<"<= CONV_STD_LOGIC_VECTOR(0,"<<wE- intlog2(inputWidth)<<");"<<endl;
	
				vhdl << tab << declare("valueExponent",wE,true)<<"<= not(zeroPadding4Exponent & temporalExponent);"<<endl;
	
				exponentConversion = new IntAdder(target,wE);
				exponentConversion->changeName(getName()+"exponentConversion");
				oplist.push_back(exponentConversion);
				inPortMap  (exponentConversion, "X", "MSB2Signal");
				inPortMap  (exponentConversion, "Y", "valueExponent");
				inPortMapCst(exponentConversion, "Cin", "'1'");
				outPortMap (exponentConversion, "R","partialConvertedExponent");
				vhdl << instance(exponentConversion, "exponentConversion");
	
				syncCycleFromSignal("partialConvertedExponent");
	
				vhdl << tab << declare("biassOfOnes",wE-1)<<"<=CONV_STD_LOGIC_VECTOR("<<pow(double(2),wE)-1<<","<<wE-1<<");"<<endl;
				vhdl << tab << declare("biassSignal",wE)<<"<='0' & biassOfOnes;"<<endl;
				vhdl << tab << declare("biassSignalBit",wE+1)<<"<='0' & biassSignal;"<<endl;
				vhdl << tab << declare("zeroBitExponent")<<"<='0';"<<endl;
				vhdl << tab << declare("partialConvertedExponentBit",wE+1)<<"<= '0' & partialConvertedExponent;"<<endl;
				vhdl << tab << declare("sign4OU")<<"<= partialConvertedExponent"<<of(wE-1)<<";"<<endl;
	
				exponentFinal = new IntAdder(target,wE+1);
				exponentFinal->changeName(getName()+"exponentFinal");
				oplist.push_back(exponentFinal);
				inPortMap  (exponentFinal, "X", "partialConvertedExponentBit");
				inPortMap  (exponentFinal, "Y", "biassSignalBit");
				inPortMapCst(exponentFinal, "Cin", "'0'");
				outPortMap (exponentFinal, "R","convertedExponentBit");
				vhdl << instance(exponentFinal, "exponentFinal");
	
				syncCycleFromSignal("convertedExponentBit");
	
				vhdl << tab << declare("OUflowSignal1",2)<<"<= convertedExponentBit"<<range(wE,wE-1)<<";"<<endl;
	
				vhdl << tab << declare("underflowSignal")<<"<= '1' when (sign4OU='1' and OUflowSignal1=\"01\" ) else '0';"<<endl;
	
				vhdl << tab << declare("overflowSignal1")<<"<= '1' when (sign4OU='0' and OUflowSignal1=\"10\" ) else '0';"<<endl;
	
				syncCycleFromSignal("correctingExponent");
	
				vhdl << tab << declare("zeroInput4Exponent",wE+1)<<"<=(others=>'0');"<<endl;
				vhdl << tab << declare("possibleConvertedExponent2",wE)<<"<= convertedExponentBit"<<range(wE-1,0)<<";"<<endl;
				vhdl << tab << declare("possibleConvertedExponent20",wE+1)<<"<= '0' & possibleConvertedExponent2;"<<endl;
				vhdl << tab << declare("sign4OU2")<<"<= possibleConvertedExponent2"<<of(wE-1)<<";"<<endl;
	
				expCorrect = new IntAdder(target,wE+1);
				expCorrect->changeName(getName()+"expCorrect");
				oplist.push_back(expCorrect);
				inPortMap  (expCorrect, "X",   "possibleConvertedExponent20");
				inPortMap  (expCorrect, "Y",   "zeroInput4Exponent");
				inPortMap  (expCorrect, "Cin", "correctingExponent");
				outPortMap (expCorrect, "R",   "finalConvertedExponent");
				vhdl << instance(expCorrect, "expCorrect");
	
				syncCycleFromSignal("finalConvertedExponent");
	
				vhdl << tab << declare("convertedExponent",wE)<<"<= finalConvertedExponent"<<range(wE-1,0)<<";"<<endl;
				vhdl << tab << declare("overflowSignal2")<<"<= '1' when (sign4OU2='0' and finalConvertedExponent"<<range(wE,wE-1)<<" = \"10\" ) else '0' ;"<<endl;
				vhdl << tab << declare("overflowSignal")<<"<= overflowSignal2 or overflowSignal1;"<<endl;

				//code for the special bits
	
				vhdl << tab << declare("MSBSelection")<<"<= overflowSignal;"<<endl;
				vhdl << tab << declare("LSBSelection")<<"<= not(underflowSignal or zeroInput);"<<endl;
				vhdl << tab << declare("Selection",2)<<"<= MSBSelection & LSBSelection when zeroInput='0' else \"00\";"<<endl;
				vhdl << tab << declare("specialBits",2)<<" <= Selection;"<<endl;
		
				//assembling the result
				vhdl << tab << "O<= specialBits & signSignal & convertedExponent & convertedFraction;"<<endl;
			}
	}


	Fix2FP::~Fix2FP() {
	}


	void Fix2FP::emulate(TestCase * tc)
	{
	
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("I");
 	
		mpz_class tmpSUB = (mpz_class(1) << (MSBI-LSBI+1));
		mpz_class tmpCMP = (mpz_class(1)  << (MSBI-LSBI))-1;

		if (Signed != 0)
			if (svX > tmpCMP){ //negative number 
				svX = svX - tmpSUB;
			}

		mpfr_t x;
		mpfr_init2(x, 10000); //init to infinite prec
		mpfr_set_z(x, svX.get_mpz_t(), GMP_RNDN);

		mpfr_t cst, tmp2;
		mpfr_init2(cst, 10000); //init to infinite prec
		mpfr_init2(tmp2, 10000); //init to infinite prec


		mpfr_set_ui(cst, 2 , GMP_RNDN);
		mpfr_set_si(tmp2, LSBI , GMP_RNDN);
		mpfr_pow(cst, cst, tmp2, GMP_RNDN);

		mpfr_mul(x, x, cst, GMP_RNDN);

		mpfr_t myFP;
		mpfr_init2(myFP, wFR+1);
		mpfr_set(myFP, x, GMP_RNDN);

		FPNumber  fpr(wER, wFR, myFP);
		mpz_class svR = fpr.getSignalValue();
		tc->addExpectedOutput("O", svR);

		// clean-up
		mpfr_clears(x, myFP, NULL);

	}


	void Fix2FP::buildStandardTestCases(TestCaseList* tcl){
	
	}

}
