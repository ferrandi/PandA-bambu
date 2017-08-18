/*
  Floating Point Square Root, polynomial version, for FloPoCo
 
  Authors : Mioara Joldes, Bogdan Pasca (2010)

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
#include <math.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

#ifdef HAVE_SOLLYA
#include "IntAdder.hpp"
#include "IntMultiplier.hpp"
#include "IntSquarer.hpp"
#include "FPSqrtPoly.hpp"

#define KEEP_HANDCRAFTED_VERSION 0

using namespace std;

namespace flopoco{

	FPSqrtPoly::FPSqrtPoly(Target* target, int wE, int wF, bool correctlyRounded, int degree):
		Operator(target), wE(wE), wF(wF), correctRounding(correctlyRounded) {

		ostringstream name;
		name<<"FPSqrtPoly_"<<wE<<"_"<<wF;
		uniqueName_ = name.str(); 

		// -------- Parameter set up -----------------
		addFPInput ("X", wE, wF);
		addFPOutput("R", wE, wF);

		vhdl << "--Split the Floating-Point input"<<endl;
		vhdl << tab << declare("fracX",wF) << " <= X" << range(wF-1, 0) << ";"  << endl; 
		vhdl << tab << declare("expX", wE)<< "  <= X" << range(wE+wF-1, wF) << ";"  << endl; 
		
		vhdl << "--If the real exponent is odd"<<endl;
		vhdl << tab << declare("OddExp")   << " <= not(expX(0));"  << endl;  
		

#if KEEP_HANDCRAFTED_VERSION
		if ((wE==8) && (wF==23) && (degree==2)) {
			////////////////////////////////////////////////////////////////////////////////////
			//      Original hand-crafted polynomial version, beats the automatically generated one 

			/*These are the amount of shifts with respect to 0 that the coefficients a2 a1 and a0 are shifted */
			int coeff_msb[3];
			coeff_msb[0] = 0; //a0
			coeff_msb[1] =-1; //a1
			coeff_msb[2] =-3; //a2
	
			int coeffStorageSizes[3];
			coeffStorageSizes[0] = 27; 
			coeffStorageSizes[1] = 17;
			coeffStorageSizes[2] = 9;
	
			int coeffTableWidth = coeffStorageSizes[0] + coeffStorageSizes[1] + coeffStorageSizes[2];
			int msb_x = -6; //number of zeros after the dot
			int tableAddressWidth = 1 + 7; //the +1 comes from the LSB of the exponent
			/* due to the fact that we use the same evaluator for both odd and even exponents 
			the datapath width has to be 1 bith wider 
			even case   0XXXXXX
			odd case    XXXXXX0
			*/
			int sizeOfX = 16; 
			
			/* number of bits kept to the right of the LSB of a1 when performing 
			the addition a1 + a2x; the value of this variable determines the size 
			of the truncation of the a2x product */
			int keepBitsRightOfA1; 

			/* how many extra bits I keep to the right of the LSB of a1 after 
			the addition a1 + a2x which extended a1 to the right with keepBitsRightOfA1
			bits; we need these bits for perfroming correct rounding */
			int keepBitsRightOfA1BeforeFinalMult;
	
			if (correctRounding){
				keepBitsRightOfA1 = 17;
				keepBitsRightOfA1BeforeFinalMult = 2;
			}else{ //faithful rounding 
				keepBitsRightOfA1 = 1;
				keepBitsRightOfA1BeforeFinalMult = 0;
			}
	
			vhdl << "--A concatenation of the exception bits and the sign bit"<<endl;
			vhdl << tab << declare("excsX", 3) << " <= X" << range(wE+wF+2, wE+wF) << ";"  << endl; 

	
			//first estimation of the exponent
			vhdl << tab << declare("expBiasPostDecrement", wE+1) << " <= CONV_STD_LOGIC_VECTOR("<< (1<<(wE-1))-2 <<","<<wE+1<<");"<<endl;
			vhdl << tab << declare("expPostBiasAddition", wE+1) << " <= ( \"0\" & expX) + expBiasPostDecrement + not(OddExp);"<<endl;
	
			//the addres bits for the coefficient ROM; the MSB of address is the LSB of the exponent
			vhdl << tab << declare("address", tableAddressWidth) << " <= OddExp & X" << range(wF-1, wF-tableAddressWidth+1) << ";"  << endl; 

			//get the correct size of x for the multiplication
			vhdl << tab << declare("lowX", sizeOfX + 1) << " <= (\"0\" & X"<<range(sizeOfX-1,0) << ") when OddExp='0' else "
			                                            << "(X"<<range(sizeOfX-1,0) << " & \"0\");"<<endl<<endl;

			//instantiate the coefficient table
			Table* t;
			if (correctRounding)
				t = new PolynomialTableCorrectRounded(target, tableAddressWidth, coeffTableWidth);
			else //faithful rounding 
				t = new PolynomialTable(target, tableAddressWidth, coeffTableWidth);
			oplist.push_back(t);

			nextCycle();//// this pipeline level is needed in order to infer a BRAM here
	
			inPortMap    (t, "X", "address");
			outPortMap   (t, "Y", "data");
			vhdl << instance(t, "SQRT_Coeffs_Table");
			syncCycleFromSignal("data");

			nextCycle();// The Coefficent ROM has a registered output TODO upgrade to FloPoCo 2.0
                                      
			/* get a2 from memory; 
				a2 is negative for this handcrafted version but is stored 
				positive in the BRAM so we don't have to do 2's complement 
				multiplications*/ 
		
			vhdl <<endl << tab << declare("a2", coeffStorageSizes[2]) << "<= data"<<range(coeffTableWidth-1, coeffTableWidth-coeffStorageSizes[2]) <<";"<<endl;
			// perform (-a2)*x, each term is <= than 17 bits so * will be mapped into one DSP block;
			/* this should work rather well for altera as well, but the unsigned 18-bit multipliers might reduce BRAM cost TODO explore this */ 
			vhdl << tab << declare("prodA2X",coeffStorageSizes[2] + sizeOfX + 1) << " <= lowX  * a2 ;" << endl;

			nextCycle();// Will be absorbed by the DSP macro
	
			//get a1 from memory
			vhdl <<endl << tab << declare("a1",coeffStorageSizes[1]) << " <= data"<<range(coeffStorageSizes[0] + coeffStorageSizes[1] - 1, coeffStorageSizes[0]) << ";" <<endl;

			/* next operation should be a1 + a2x, but at previous step we computed -a2*x 
			so therefore we now need to compute a1 - (-a2x) */
			
			/* the addition (well subtraction) will be perfromed on less bits, discarding 
			some of the lower bits of a2x */
			vhdl << tab << declare("signExtA1ZeroPad", 1 + coeffStorageSizes[1] + keepBitsRightOfA1) << " <= \"0\" & a1 & " << zg(keepBitsRightOfA1, 0) << ";" << endl;
			/* alignment of the previously computed -a2*x product and truncation of some of the lower bits */
			vhdl << tab << declare("signExtAlignedProdA2X", 1 + coeffStorageSizes[1] + keepBitsRightOfA1) << " <= \"0\" & " << zg(coeff_msb[1]-(coeff_msb[2]+msb_x),0) << " & " 
				  << "prodA2X"<< range(coeffStorageSizes[2] + sizeOfX, coeffStorageSizes[2] + sizeOfX - (1 + coeffStorageSizes[1] + keepBitsRightOfA1 + coeff_msb[2]+msb_x )+1) << ";"<<endl;

			/* subtraction a-b = a + (not b) + 1 */
			vhdl << tab << declare("negSignExtAlignedProdA2X", 1 + coeffStorageSizes[1] + keepBitsRightOfA1) << " <= not(signExtAlignedProdA2X);"<<endl;

			/* operands have no combinatorial delays */
			IntAdder* add1 = new IntAdder(target, 1 + coeffStorageSizes[1] + keepBitsRightOfA1);
			oplist.push_back(add1);
	
			inPortMap(add1,"X", "signExtA1ZeroPad");
			inPortMap(add1,"Y", "negSignExtAlignedProdA2X");
			inPortMapCst(add1,"Cin", "'1'");
			outPortMap(add1,"R", "a1pxa2");
			vhdl << instance(add1, "Adder_a1_prod_x_a2");
	
			syncCycleFromSignal("a1pxa2"); 
			setCriticalPath( add1->getOutputDelay("R") );
			nextCycle();////////////////// The tiling multiplier does not yet support inDelayMap
	
			//perform the multiplication between x and ( a1 + a2x )  
			//TODO pipeline if keepBitsRightOfA1BeforeFinalMult > 0  
			//FIXME for now we instantiate an int Multiplier, but we can do better

			/* I guess that a1pxa2 does not overflow ?! TODO:check*/
			
			/* at this point we need to multiply x, 17-bit wide with a1pxa2*/
			/* we choose 34 bits for a1pxa2 (so we take just two multipliers)
			and we keep keepBitsRightOfA1BeforeFinalMult */ 				
			if (correctRounding)
				vhdl << tab << declare("a1pxa2truncated",17+keepBitsRightOfA1BeforeFinalMult) << " <= a1pxa2"<<range(coeffStorageSizes[1] + keepBitsRightOfA1-1, keepBitsRightOfA1 - keepBitsRightOfA1BeforeFinalMult) << ";" << endl;
			else
				vhdl << tab << declare("a1pxa2truncated",17) << " <= a1pxa2"<<range(coeffStorageSizes[1] + keepBitsRightOfA1-1, keepBitsRightOfA1) << ";" << endl;				

			IntMultiplier * mult_x_a1pa2x = new IntMultiplier(target, (sizeOfX+1), (correctRounding?17+keepBitsRightOfA1BeforeFinalMult:17), inDelayMap("X", target->localWireDelay() + getCriticalPath()), 0, 0.9);
			oplist.push_back(mult_x_a1pa2x);

			inPortMap (mult_x_a1pa2x, "X", "lowX");
			inPortMap (mult_x_a1pa2x, "Y", "a1pxa2truncated");
			outPortMap(mult_x_a1pa2x, "R", "prodXA1sumA2X_large");
			vhdl << tab << instance(mult_x_a1pa2x,"Multiplier_x_a1pxa2");
			syncCycleFromSignal("prodXA1sumA2X_large"); 

			/* discard the msb 15 bits as they are all zeros. 
			FIXME replace this by one DSP an one adder */
			vhdl << tab << declare("prodXA1sumA2X",(correctRounding?34+keepBitsRightOfA1BeforeFinalMult:34)) << " <= prodXA1sumA2X_large"<<range((correctRounding?35:33),0) << ";" << endl;

			/* compose the operands for the addition a0 + [ prev_computation ] */
			/* fetch a0 from memory */
			vhdl << endl << tab << declare("a0",coeffStorageSizes[0]) << " <= data" << range(coeffStorageSizes[0]-1,0) << ";" << endl;
			
			/* one half ulp is already added to c0 to provide for the final rounding
			therefore, after the final addition we can just truncate */
			vhdl << tab << declare ("ovfGuardA0", 1 + coeffStorageSizes[0]) << " <=  \"0\" & a0;" << endl; 

			/* this gets aligned and truncated to the LSB of a0 */
			vhdl << tab << declare ("ovfGuardAlignProdXA1sumA2X", 1 + coeffStorageSizes[0]) << " <=  \"0\" & " << zg((1+coeff_msb[0])+1-msb_x , 0)  << " & "
					  << "prodXA1sumA2X"<<range((sizeOfX+1)+ coeffStorageSizes[1] -1 + keepBitsRightOfA1BeforeFinalMult, keepBitsRightOfA1BeforeFinalMult + (sizeOfX+1)+ coeffStorageSizes[1] - (coeffStorageSizes[0]- (-msb_x+1+(1+ coeff_msb[0])))) << ";" <<endl; 

			IntAdder * adder_a0px_a1pxa2 =  new IntAdder(target, 1 + coeffStorageSizes[0]);
			oplist.push_back(adder_a0px_a1pxa2);
		
			inPortMap   (adder_a0px_a1pxa2, "X", "ovfGuardA0");
			inPortMap   (adder_a0px_a1pxa2, "Y", "ovfGuardAlignProdXA1sumA2X");
			inPortMapCst(adder_a0px_a1pxa2, "Cin", "'1'");

			outPortMap(adder_a0px_a1pxa2,   "R", "sumA0ProdXA1sumA2X");
			vhdl << instance(adder_a0px_a1pxa2, "Adder_a0px_a1pxa2");
			syncCycleFromSignal("sumA0ProdXA1sumA2X");
			setCriticalPath( adder_a0px_a1pxa2->getOutputDelay("R"));
			 
			/* we performed rounding by adding one half ulp to the final result directly 
			in the addition a0 + ... */

			if (!correctlyRounded){
				/* the final addition cannot overflow as sqrt(11.111111....) is smaller than 10.0000..0 */
				
				vhdl << tab << declare("finalFrac", wF) << " <= sumA0ProdXA1sumA2X" << range(coeffStorageSizes[0]-2, coeffStorageSizes[0]-wF-1) << ";" << endl;
				vhdl << tab << declare("finalExp", wE) << " <= expPostBiasAddition" << range(wE,1) <<";"<<endl;

				vhdl << tab << "-- sign/exception handling" << endl;
				vhdl << tab << "with excsX select" <<endl
				     << tab << tab <<  declare("exnR", 2) << " <= \"01\" when \"010\", -- positive, normal number" << endl
				     << tab << tab << "excsX" << range(2, 1) << " when \"001\" | \"000\" | \"100\", " << endl
				     << tab << tab << "\"11\" when others;"  << endl;
				vhdl << tab << "R <= exnR & excsX(0) & finalExp & finalFrac;" << endl; 
				outDelayMap["R"] = getCriticalPath();
			}else{ /* correctly rounded version; this is rather costly; one should 
			consider either using faithful rounding with one bit more of mantisa for the 
			same numbrical quality, either use the muliplicative version FPSqrt */
			
				vhdl << tab << declare("resRoundedOnWFp1",1+wF+1)<<" <= sumA0ProdXA1sumA2X"<<range(coeffStorageSizes[0]-1,coeffStorageSizes[0]-(1+wF+1))<<";"<<endl;
				
				/* addOneHalf ulp in parallel with squaring */
				IntAdder *ohu = new IntAdder(target, 1+wF+1, inDelayMap("X",target->localWireDelay() + getCriticalPath()));
				oplist.push_back(ohu);
			
				inPortMapCst( ohu, "X", "resRoundedOnWFp1");
				inPortMapCst( ohu, "Y", zg(2+wF));
				inPortMapCst( ohu, "Cin", "'1'");
				outPortMap  ( ohu, "R", "resRoundedphu");
				vhdl << tab << instance(ohu, "AdderOfOneHalfUlpToRoundedResult");

				vhdl << tab << declare("negXext", 1+2*(wF+2)) << " <= (\"10\" & not(fracX) & "<<og(3+wF,0)<<") when OddExp='1' else (\"110\" & not(fracX) & "<<og(2+wF,0)<<");"<<endl;
							
				//done in parallel
				/* compute the square of sumA0ProdXA1sumA2X */
				IntSquarer *cr_squarer = new IntSquarer(target, 1+wF+1, inDelayMap("X",target->localWireDelay() + getCriticalPath()) );
				oplist.push_back(cr_squarer);
			
				inPortMapCst(cr_squarer, "X", "resRoundedOnWFp1");
				outPortMap  (cr_squarer, "R", "squaredResult");
				vhdl << tab << instance(cr_squarer, "Squarer_CorrectRounding");
				syncCycleFromSignal("squaredResult");
				setCriticalPath( cr_squarer->getOutputDelay("R"));
			
				/* the results is on 2*(2+wF) bits */

				/* extend squaredResult with one bit to accomodate for the sign of the final comparisson
				implemented as a subtraction */
				vhdl << tab << declare("extSquaredResult",1+2*(wF+2))<< "<= \"0\" & squaredResult;"<<endl; 
				/* extend x with zeros to the right and left and then negate*/
			
				IntAdder *cr_subtracter = new IntAdder(target, 1+2*(wF+2), inDelayMap("X",target->localWireDelay() + getCriticalPath()) );
				oplist.push_back(cr_subtracter);
			
				inPortMap   ( cr_subtracter, "X", "extSquaredResult");
				inPortMap   ( cr_subtracter, "Y", "negXext");
				inPortMapCst( cr_subtracter, "Cin", "'1'");
				outPortMap  ( cr_subtracter, "R", "sqr_of_sqrtx_m_x"); 
				vhdl << tab << instance(cr_subtracter, "cr_subtracter");
				syncCycleFromSignal("sqr_of_sqrtx_m_x");
				setCriticalPath( cr_subtracter->getOutputDelay("R"));
	
				manageCriticalPath(target->lutDelay() + target->localWireDelay());
				vhdl << tab << declare("finalFrac", wF) << " <= resRoundedOnWFp1" << range(wF,1) << " when sqr_of_sqrtx_m_x"<<of(2*(wF+2))<<"='0' else "
				                                        << "resRoundedphu"<< range(wF, 1)<<";"<<endl;
				vhdl << tab << declare("finalExp", wE) << " <= expPostBiasAddition" << range(wE,1) <<";"<<endl;

				vhdl << tab << "-- sign/exception handling" << endl;
				vhdl << tab << "with excsX select" <<endl
				     << tab << tab <<  declare("exnR", 2) << " <= \"01\" when \"010\", -- positive, normal number" << endl
				     << tab << tab << "excsX" << range(2, 1) << " when \"001\" | \"000\" | \"100\", " << endl
				     << tab << tab << "\"11\" when others;"  << endl;
				vhdl << tab << "R <= exnR & excsX(0) & finalExp & finalFrac;" << endl; 
				outDelayMap["R"] = getCriticalPath();
			}
		}else{
#endif // KEEP_HANDCRAFTER_VERSION

		vhdl << tab << declare("excsX",3) << " <= X"<<range(wE+wF+2,wE+wF)<<";"<<endl;
		vhdl << tab << declare("sX") << "  <= X"<<of(wE+wF)<<";"<<endl;
		
		//first estimation of the exponent
		vhdl << tab << declare("expBiasPostDecrement", wE+1) << " <= CONV_STD_LOGIC_VECTOR("<< (1<<(wE-1))-2 <<","<<wE+1<<");"<<endl;
		vhdl << tab << declare("expPostBiasAddition", wE+1) << " <= ( \"0\" & expX) + expBiasPostDecrement + not(OddExp);"<<endl;
	
		vhdl << tab << "-- sign/exception handling" << endl;
		vhdl << tab << "with excsX select" <<endl
			  << tab << tab <<  declare("exnR", 2) << " <= \"01\" when \"010\", -- positive, normal number" << endl
			  << tab << tab << "excsX" << range(2, 1) << " when \"001\" | \"000\" | \"100\", " << endl
			  << tab << tab << "\"11\" when others;"  << endl;

//		FunctionEvaluator *fixpsqrt = new FunctionEvaluator(target, "sqrt(2+2*x),0,1,1;sqrt(1+x),0,1,1", wF+1, wF, degree); //TODO
//		oplist.push_back(fixpsqrt);

//***************************************************************************
		PiecewiseFunction  *pf = new PiecewiseFunction("sqrt(2+2*x),0,1,1;sqrt(1+x),0,1,1");
		PolyCoeffTable *tg = new PolyCoeffTable(target, pf, wF+2+(correctlyRounded?1:0), degree);
		oplist.push_back(tg);
		
		int addrLinesPolyOdd, addrLinesPolyEven; 
		
		/* the number of address lines needed to store the intervals for the odd case*/		
		addrLinesPolyOdd = (tg->getNrIntArray())[0];
		/* the number of address lines needed to store the intervals for the even case*/		
		addrLinesPolyEven = (tg->getNrIntArray())[1];

		
		/* the numbger of address lines of the table */
		int addrLinesTable = tg->wIn;

		/* this will be different than 0 if for we require more intervals to get 
		the same precision for the odd case, The difference is usually 1*/
		int largerTableForOdd = addrLinesPolyOdd-addrLinesPolyEven;
		
		vhdl << tab << declare("tableAddress", addrLinesTable) << " <= (expX(0) & X"<<range(wF-1,wF-addrLinesPolyOdd)<<") when expX(0)='0' else "<<endl
		                                                           << "(expX(0) & " <<zg(largerTableForOdd,0) << " & X"<<range(wF-1,wF-addrLinesPolyEven)<<");"<<endl;

		vhdl << tab << declare("theFrac",wF-addrLinesPolyEven) << " <= ("<< zg(largerTableForOdd,0) <<" & X"<<range(wF-addrLinesPolyOdd-1,0)<<") when expX(0)='0' else "<<endl
		                                                            << "X"<<range(wF-addrLinesPolyEven-1,0)<<";"<<endl;
		
		YVar* y = new YVar(wF-addrLinesPolyEven, -addrLinesPolyEven);
		
		PolynomialEvaluator *pe = new PolynomialEvaluator(target, tg->getCoeffParamVector(), y, wF+2+(correctlyRounded?1:0), tg->getMaxApproxError() );
		oplist.push_back(pe);
		
		nextCycle();// The Coefficent ROM has a registered iunput
		
		inPortMap  ( tg, "X", "tableAddress");
		outPortMap ( tg, "Y", "Coef");
		vhdl << instance ( tg, "GeneratedTable" );
		syncCycleFromSignal("Coef");
		nextCycle();// The Coefficent ROM has a registered output
		
		vhdl << tab << declare ("y",y->getSize()) << " <= theFrac;" << endl;
		
		/* get the coefficients */
		int lsb = 0, sizeS = 0;
		for (uint32_t i=0; i< pe->getCoeffParamVector().size(); i++){
			lsb += sizeS;
			sizeS = pe->getCoeffParamVector()[i]->getSize()+1;
			vhdl << tab << declare(join("a",i), sizeS ) << "<= Coef"<< range (lsb+sizeS-1, lsb) << ";" << endl;
		}
		
		inPortMap( pe, "Y", "y");
		for (uint32_t i=0; i< pe->getCoeffParamVector().size(); i++){
			inPortMap(pe,  join("a",i), join("a",i));
		}
		outPortMap( pe, "R", "rfx");
		vhdl << instance( pe, "PolynomialEvaluator");
		syncCycleFromSignal("rfx");
		setCriticalPath( pe->getOutputDelay("R"));		                  

		if (!correctlyRounded){
			vhdl << tab << declare("extentedf", 1 + wF + 1) << " <= rfx"<<range(pe->getRWidth()-pe->getRWeight()-1, pe->getRWidth()-(pe->getRWeight()+wF)-2)<<";"<<endl; 
		                             
			IntAdder *roundingAdder = new IntAdder(target, 1 + wF + 1, inDelayMap("X", target->localWireDelay() + getCriticalPath()));
			oplist.push_back(roundingAdder);
		
			inPortMap    ( roundingAdder, "X"  , "extentedf");
			inPortMapCst ( roundingAdder, "Y"  , zg(1 + wF + 1,0) );
			inPortMapCst ( roundingAdder, "Cin", "'1'");
			outPortMap   ( roundingAdder, "R"  , "fPostRound");
			vhdl << instance( roundingAdder, "Rounding_Adder");

			syncCycleFromSignal("fPostRound");
			vhdl << tab << " R <= exnR & sX & expPostBiasAddition"<<range(wE,1)<<" & fPostRound"<<range(wF, 1)<<";"<<endl;
		}else{ /* correctly rounded version */
			/* round on one bit more */ //TODO this rounding should be saved once we can directly feed TableGenerator the 1/2 ulps to add to c0
			vhdl << tab << declare("extentedf", 1 + wF + 2) << " <= rfx"<<range(pe->getRWidth()-pe->getRWeight()-1, pe->getRWidth()-(pe->getRWeight()+wF)-3)<<";"<<endl; 
		                             
			IntAdder *roundingAdder = new IntAdder(target, 1 + wF + 2, inDelayMap("X", target->localWireDelay() + getCriticalPath()));
			oplist.push_back(roundingAdder);
		
			inPortMap    ( roundingAdder, "X"  , "extentedf");
			inPortMapCst ( roundingAdder, "Y"  , zg(1 + wF + 2,0) );
			inPortMapCst ( roundingAdder, "Cin", "'1'");
			outPortMap   ( roundingAdder, "R"  , "fPostRound");
			vhdl << instance( roundingAdder, "Rounding_Adder");
			syncCycleFromSignal("fPostRound");
			setCriticalPath( roundingAdder->getOutputDelay("R"));
			
			vhdl << tab << declare("resRoundedOnWFp1",1+wF+1)<<" <= fPostRound"<<range(1+wF+1,1)<<";"<<endl;
				
			/* addOneHalf ulp in parallel with squaring */
			IntAdder *ohu = new IntAdder(target, 1+wF+1, inDelayMap("X", target->localWireDelay() + getCriticalPath()));
			oplist.push_back(ohu);
		
			inPortMapCst( ohu, "X", "resRoundedOnWFp1");
			inPortMapCst( ohu, "Y", zg(2+wF));
			inPortMapCst( ohu, "Cin", "'1'");
			outPortMap  ( ohu, "R", "resRoundedphu");
			vhdl << tab << instance(ohu, "AdderOfOneHalfUlpToRoundedResult");
			syncCycleFromSignal("resRoundedphu");

			/* extend x with zeros to the right and left and then negate*/
			vhdl << tab << declare("negXext", 1+2*(wF+2)) << " <= (\"10\" & not(fracX) & "<<og(3+wF,0)<<") when OddExp='1' else (\"110\" & not(fracX) & "<<og(2+wF,0)<<");"<<endl;
			//x fraction alignment is done in parallel with squaring
		
			/* compute the square of sumA0ProdXA1sumA2X */
			IntSquarer *cr_squarer = new IntSquarer(target, 1+wF+1, inDelayMap("X", target->localWireDelay() + getCriticalPath()));
			oplist.push_back(cr_squarer);
		
			inPortMapCst(cr_squarer, "X", "resRoundedOnWFp1");
			outPortMap  (cr_squarer, "R", "squaredResult");
			vhdl << tab << instance(cr_squarer, "Squarer_CorrectRounding");
			syncCycleFromSignal("squaredResult"); //this will be the longest path between the two 
			setCriticalPath( cr_squarer->getOutputDelay("R") );
			/* the results is on 2*(2+wF) bits */

			/* extend squaredResult with one bit to accomodate for the sign of the final comparisson
			implemented as a subtraction */
			vhdl << tab << declare("extSquaredResult",1+2*(wF+2))<< "<= \"0\" & squaredResult;"<<endl; 
		
			IntAdder *cr_subtracter = new IntAdder(target, 1+2*(wF+2), inDelayMap("X", target->localWireDelay() + getCriticalPath()));
			oplist.push_back(cr_subtracter);
		
			inPortMap   ( cr_subtracter, "X", "extSquaredResult");
			inPortMap   ( cr_subtracter, "Y", "negXext");
			inPortMapCst( cr_subtracter, "Cin", "'1'");
			outPortMap  ( cr_subtracter, "R", "sqr_of_sqrtx_m_x"); 
			vhdl << tab << instance(cr_subtracter, "cr_subtracter");
			syncCycleFromSignal("sqr_of_sqrtx_m_x");
			setCriticalPath(cr_subtracter->getOutputDelay("R"));
				
			manageCriticalPath( target->localWireDelay() + getCriticalPath());	
			vhdl << tab << declare("finalFrac", wF) << " <= resRoundedOnWFp1" << range(wF,1) << " when sqr_of_sqrtx_m_x"<<of(2*(wF+2))<<"='0' else "
			                                        << "resRoundedphu"<< range(wF, 1)<<";"<<endl;
			vhdl << tab << declare("finalExp", wE) << " <= expPostBiasAddition" << range(wE,1) <<";"<<endl;

			vhdl << tab << " R <= exnR & sX & expPostBiasAddition"<<range(wE,1)<<" & finalFrac;"<<endl;
			outDelayMap["R"]=getCriticalPath();
		}
		
#if KEEP_HANDCRAFTED_VERSION
		}
#endif			
		
	}



	FPSqrtPoly::~FPSqrtPoly() {
	}

	void FPSqrtPoly::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");

		/* Compute correct value */
		FPNumber fpx(wE, wF);
		fpx = svX;
		mpfr_t x, r;
		mpfr_init2(x, 1+wF);
		mpfr_init2(r, 1+wF); 
		fpx.getMPFR(x);

		if(correctRounding) {
			mpfr_sqrt(r, x, GMP_RNDN);
			FPNumber  fpr(wE, wF, r);
			/* Set outputs */
			mpz_class svr= fpr.getSignalValue();
			tc->addExpectedOutput("R", svr);
		}
		else { // faithful rounding 
			mpfr_sqrt(r, x, GMP_RNDU);
			FPNumber  fpru(wE, wF, r);
			mpz_class svru = fpru.getSignalValue();
			tc->addExpectedOutput("R", svru);

			mpfr_sqrt(r, x, GMP_RNDD);
			FPNumber  fprd(wE, wF, r);
			mpz_class svrd = fprd.getSignalValue();
			/* Set outputs */
			tc->addExpectedOutput("R", svrd);
		}

		mpfr_clears(x, r, NULL);
	}




	// One test out of 4 fully random (tests NaNs etc)
	// All the remaining ones test positive numbers.
	TestCase* FPSqrtPoly::buildRandomTestCase(int i){

		TestCase *tc;
		mpz_class a;

		tc = new TestCase(this); 
		/* Fill inputs */
		if ((i & 3) == 0)
			a = getLargeRandom(wE+wF+3);
		else
			a  = getLargeRandom(wE+wF) + (mpz_class(1)<<(wE+wF+1)); // 010xxxxxx
		tc->addInput("X", a);

		/* Get correct outputs */
		emulate(tc);

		return tc;
	}
}

#endif //HAVE_SOLLYA

