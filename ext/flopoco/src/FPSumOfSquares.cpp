/*

  A sum of three squares  for FloPoCo

  There are two versions, selectable by the useFPOperators parameter.
  The main one is a specific datapath designed in the FloPoCo
  philosophy, the other one combines existing FloPoCo floating-point
  operators for comparison purposes.
 
  Authors: Florent de Dinechin, Bogdan Pasca
 
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

 */
#include <fstream>
#include <sstream>
#include <math.h>	// for NaN
#include "FPSumOfSquares.hpp"


using namespace std;

namespace flopoco{


	FPSumOfSquares::FPSumOfSquares(Target* target, int wE, int wF, int optimize)
		: Operator(target), wE(wE), wF(wF)
	{
		setCopyrightString("F. de Dinechin, Bogdan Pasca (2011)");
		srcFileName="FPSumOfSquares";
		ostringstream o;
		o << "FPSumOfSquares_" << wE << "_" << wF;
		if(!optimize)
			o << "_FP";
		setName(o.str());

		addFPInput("X", wE, wF);
		addFPInput("Y", wE, wF);
		addFPInput("Z", wE, wF);
		addFPOutput("R", wE, wF, 2); // This 2 means: we will allow two possible inputs (faithful rounding)

		if(!optimize) {
			//////////////////////////////////////////////////////////////////:
			//            A version that assembles FP operators             //
			//////////////////////////////////////////////////////////////////:

			FPMultiplier* mult = new FPMultiplier(target, wE, wF, wE, wF, wE, wF, 1);
			oplist.push_back(mult);
			FPAdderSinglePath* add =  new FPAdderSinglePath(target, wE, wF, wE, wF, wE, wF);
			oplist.push_back(add);
		
			inPortMap (mult, "X", "X");
			inPortMap (mult, "Y", "X");
			outPortMap(mult, "R", "X2");
			vhdl << instance(mult, "multx");
		
			inPortMap (mult, "X", "Y");
			inPortMap (mult, "Y", "Y");
			outPortMap(mult, "R", "Y2");
			vhdl << instance(mult, "multy");
		
			inPortMap (mult, "X", "Z");
			inPortMap (mult, "Y", "Z");
			outPortMap(mult, "R", "Z2");
			vhdl << instance(mult, "multz");
		
			syncCycleFromSignal("Z2", false);
			nextCycle(); 
		
			inPortMap (add, "X", "X2");
			inPortMap (add, "Y", "Y2");
			outPortMap(add, "R", "X2PY2");
			vhdl << instance(add, "add1");
		
			syncCycleFromSignal("X2PY2", false);
			nextCycle(); 
		
			inPortMap (add, "X", "X2PY2");
			inPortMap (add, "Y", "Z2");
			outPortMap(add, "R", "X2PY2PZ2");
			vhdl << instance(add, "add2");
		
			syncCycleFromSignal("X2PY2PZ2", false);
			setCriticalPath(add->getOutputDelay("R"));
			vhdl << tab << "R <= X2PY2PZ2;"<<endl;
			outDelayMap["R"]=getCriticalPath();
		}
		else { ////////////////// here comes the FloPoCo version	//////////////////////////:
			// Error analysis
			// 3 ulps(wF+g) in the multiplier truncation
			// Again 2 ulps(wF+g) in the shifter output truncation
			// Normalisation truncation: either 0 (total 5), or 1 ulp(wF+g) but dividing the previous by 2 (total 3.5)
			// Total max 5 ulps, we're safe with 3 guard bits

			// guard bits for a faithful result
			int g=3; 

			// The exponent datapath

			// setCriticalPath( getMaxInputDelays(inputDelays) + target->localWireDelay());
			setCriticalPath(0);

			manageCriticalPath(  target->adderDelay(wE+1) // subtractions 
													 + target->localWireDelay(wE) // fanout of XltY etc
													 + target->lutDelay()         // & and mux
													 );

			//---------------------------------------------------------------------
			// extract the three biased exponents. 
			vhdl << tab << declare("EX", wE) << " <=  X" << range(wE+wF-1, wF) << ";" << endl;
			vhdl << tab << declare("EY", wE) << " <=  Y" << range(wE+wF-1, wF) << ";" << endl;
			vhdl << tab << declare("EZ", wE) << " <=  Z" << range(wE+wF-1, wF) << ";" << endl;
		
			// determine the max of the exponents
			vhdl << tab << declare("DEXY", wE+1) << " <=   ('0' & EX) - ('0' & EY);" << endl;
			vhdl << tab << declare("DEYZ", wE+1) << " <=   ('0' & EY) - ('0' & EZ);" << endl;
			vhdl << tab << declare("DEXZ", wE+1) << " <=   ('0' & EX) - ('0' & EZ);" << endl;
			vhdl << tab << declare("XltY") << " <=   DEXY("<< wE<<");" << endl;
			vhdl << tab << declare("YltZ") << " <=   DEYZ("<< wE<<");" << endl;
			vhdl << tab << declare("XltZ") << " <=   DEXZ("<< wE<<");" << endl;
		
			// rename the exponents  to A,B,C with A>=(B,C)
			vhdl << tab << declare("EA", wE)  << " <= " << endl
				  << tab << tab << "EZ when (XltZ='1') and (YltZ='1')  else " << endl
				  << tab << tab << "EY when (XltY='1') and (YltZ='0')  else " << endl
				  << tab << tab << "EX; " << endl;
			vhdl << tab << declare("EB", wE)  << " <= " << endl
				  << tab << tab << "EX when (XltZ='1') and (YltZ='1')  else " << endl
				  << tab << tab << "EZ when (XltY='1') and (YltZ='0')  else " << endl
				  << tab << tab << "EY; " << endl;
			vhdl << tab << declare("EC", wE)  << " <= " << endl
				  << tab << tab << "EY when (XltZ='1') and (YltZ='1')  else " << endl
				  << tab << tab << "EX when (XltY='1') and (YltZ='0')  else " << endl
				  << tab << tab << "EZ; " << endl;
		
			//---------------------------------------------------------------------
			// Now recompute our two shift values -- they were already computed at cycle 0 but it is cheaper this way, otherwise we have to register, negate and mux them.
			manageCriticalPath(  target->adderDelay(wE-1) );

			vhdl << tab << declare("fullShiftValB", wE) << " <=  (EA" << range(wE-2,0) << " - EB" << range(wE-2,0) << ") & '0' ; -- positive result, no overflow " << endl;
			vhdl << tab << declare("fullShiftValC", wE) << " <=  (EA" << range(wE-2,0) << " - EC" << range(wE-2,0) << ") & '0' ; -- positive result, no overflow " << endl;
	
			double cpfullShiftValC = getCriticalPath();
			//---------------------------------------------------------------------
			Shifter* rightShifterDummy = new Shifter(target,wF+g+2, wF+g+2, Shifter::Right);
			int sizeRightShift = rightShifterDummy->getShiftInWidth(); 

			//-- Manage the shift value of the mantissa of B --------
			manageCriticalPath( target->localWireDelay() + target->lutDelay());
			vhdl<<tab<<declare("shiftedOutB") << " <= "; 
			if (wE>sizeRightShift){
				for (int i=wE-1;i>=sizeRightShift;i--) {
					vhdl<< "fullShiftValB("<<i<<")";
					if (i>sizeRightShift)
						vhdl<< " or ";
				}
				vhdl<<";"<<endl;
			}
			else
				vhdl<<tab<<"'0';"<<endl; 
			
			if (wE>sizeRightShift) {
				manageCriticalPath( target->localWireDelay() + target->lutDelay());
				vhdl<<tab<<declare("shiftValB",sizeRightShift) << " <=  fullShiftValB("<< sizeRightShift-1<<" downto 0)"
					  << " when shiftedOutB='0'"<<endl
					  <<tab << tab << "    else CONV_STD_LOGIC_VECTOR("<<wF+g+1<<","<<sizeRightShift<<") ;" << endl; 
			}else if (wE==sizeRightShift) {
				vhdl<<tab<<declare("shiftValB",sizeRightShift) << " <= fullShiftValB;" << endl ;
			}else { //  wE< sizeRightShift
				vhdl<<tab<<declare("shiftValB",sizeRightShift) << " <= CONV_STD_LOGIC_VECTOR(0,"<<sizeRightShift-wE <<") & fullShiftValB;" <<	endl;
			}
			double cpshiftValB = getCriticalPath();
			
			//-- Manage the shift value of the mantissa of C --------
			manageCriticalPath( target->localWireDelay() + target->lutDelay()); 
			//FIXME possible fixme needed when or does not fit on lut
			vhdl<<tab<<declare("shiftedOutC") << " <= "; 
			if (wE>sizeRightShift){
				for (int i=wE-1;i>=sizeRightShift;i--) {
					vhdl<< "fullShiftValC("<<i<<")";
					if (i>sizeRightShift)
						vhdl<< " or ";
				}
				vhdl<<";"<<endl;
			}
			else
				vhdl<<tab<<"'0';"<<endl; 
		
			setCycleFromSignal("fullShiftValC",cpfullShiftValC);
			if (wE>sizeRightShift) {
				manageCriticalPath( target->localWireDelay() + target->lutDelay());//the mux delay
				vhdl<<tab<<declare("shiftValC",sizeRightShift) << " <= fullShiftValC("<< sizeRightShift-1<<" downto 0)"
					  << " when shiftedOutC='0'"<<endl
					  <<tab << tab << "    else CONV_STD_LOGIC_VECTOR("<<wF+g+1<<","<<sizeRightShift<<") ;" << endl; 
			} else if (wE==sizeRightShift) {
				vhdl<<tab<<declare("shiftValC",sizeRightShift) << " <= fullShiftValC;" << endl ;
			} else 	{ //  wE< sizeRightShift
				vhdl<<tab<<declare("shiftValC",sizeRightShift) << " <= CONV_STD_LOGIC_VECTOR(0,"<<sizeRightShift-wE <<") & fullShiftValC;" <<	endl;
			}

			// Back to cycle 0 for the significand datapath
			setCycle(0);
			//FIXME add inDelayMap for use within hierarchies of components 
			// Square the significands 
#define USE_SQUARER 1
#if  USE_SQUARER
			IntSquarer* mult = new IntSquarer(target,  1+ wF);
#else
			IntMultiplier* mult = new IntMultiplier(target, 1+ wF, 1+ wF);
#endif
			oplist.push_back(mult);
		
			vhdl << tab << declare("mX", wF+1)  << " <= '1' & X" << range(wF-1, 0) << "; " << endl;
		
			inPortMap (mult, "X", "mX");
#if  !USE_SQUARER
			inPortMap (mult, "Y", "mX");
#endif
			outPortMap(mult, "R", "mX2");
			vhdl << instance(mult, "multx");
	
			vhdl << tab << declare("mY", wF+1)  << " <= '1' & Y" << range(wF-1, 0) << "; " << endl;

			inPortMap (mult, "X", "mY");
#if  !USE_SQUARER	
			inPortMap (mult, "Y", "mY");
#endif
			outPortMap(mult, "R", "mY2");
			vhdl << instance(mult, "multy");
		
			vhdl << tab << declare("mZ", wF+1)  << " <= '1' & Z" << range(wF-1, 0) << "; " << endl;
		
			inPortMap (mult, "X", "mZ");
#if  !USE_SQUARER	
			inPortMap (mult, "Y", "mZ");
#endif
			outPortMap(mult, "R", "mZ2");
			vhdl << instance(mult, "multz");

			syncCycleFromSignal("mZ2", false);
			setCriticalPath(mult->getOutputDelay("R"));
			// truncate the three results to wF+g+2
			int prodsize = 2+2*wF;
			vhdl << tab << declare("X2t", wF+g+2)  << " <= mX2" << range(prodsize-1, prodsize - wF-g-2) << "; " << endl;
			vhdl << tab << declare("Y2t", wF+g+2)  << " <= mY2" << range(prodsize-1, prodsize - wF-g-2) << "; " << endl;
			vhdl << tab << declare("Z2t", wF+g+2)  << " <= mZ2" << range(prodsize-1, prodsize - wF-g-2) << "; " << endl;
	
			// Now we have our three FP squares, we rename them to A,B,C with A>=(B,C) 
			// only 3 3-muxes
			manageCriticalPath(target->localWireDelay(wF) + target->lutDelay());  
			vhdl << tab << declare("MA", wF+g+2)  << " <= " << endl
				  << tab << tab << "Z2t when (XltZ='1') and (YltZ='1')  else " << endl
				  << tab << tab << "Y2t when (XltY='1') and (YltZ='0')  else " << endl
				  << tab << tab << "X2t; " << endl;
			vhdl << tab << declare("MB", wF+g+2)  << " <= " << endl
				  << tab << tab << "X2t when (XltZ='1') and (YltZ='1')  else " << endl
				  << tab << tab << "Z2t when (XltY='1') and (YltZ='0')  else " << endl
				  << tab << tab << "Y2t; " << endl;
			vhdl << tab << declare("MC", wF+g+2)  << " <= " << endl
				  << tab << tab << "Y2t when (XltZ='1') and (YltZ='1')  else " << endl
				  << tab << tab << "X2t when (XltY='1') and (YltZ='0')  else " << endl
				  << tab << tab << "Z2t; " << endl;
			
			//Synchronize exponent and significand datapath
			syncCycleFromSignal("shiftValB", cpshiftValB, false);

			// B and C right shifters are the same
			Shifter* rightShifter = new Shifter(target,wF+g+2, wF+g+2, Shifter::Right, inDelayMap("X",target->localWireDelay()+getCriticalPath())); 
			oplist.push_back(rightShifter);

			inPortMap  (rightShifter, "X", "MB");
			inPortMap  (rightShifter, "S", "shiftValB");
			outPortMap (rightShifter, "R","shiftedB");
			vhdl << instance(rightShifter, "ShifterForB");

			inPortMap  (rightShifter, "X", "MC");
			inPortMap  (rightShifter, "S", "shiftValC");
			outPortMap (rightShifter, "R","shiftedC");
			vhdl << instance(rightShifter, "ShifterForC");
		
			// superbly ignore the bits that are shifted out
			syncCycleFromSignal("shiftedB", false);
			setCriticalPath( rightShifter->getOutputDelay("R"));
			
			int shiftedB_size = getSignalByName("shiftedB")->width();
			vhdl << tab << declare("alignedB", wF+g+2)  << " <= shiftedB" << range(shiftedB_size-1, shiftedB_size -(wF+g+2)) << "; " << endl;
			vhdl << tab << declare("alignedC", wF+g+2)  << " <= shiftedC" << range(shiftedB_size-1, shiftedB_size -(wF+g+2)) << "; " << endl;
		
			vhdl << tab << declare("paddedA", wF+g+4)  << " <= \"00\" & MA; " << endl;
			vhdl << tab << declare("paddedB", wF+g+4)  << " <= \"00\" & alignedB; " << endl;
			vhdl << tab << declare("paddedC", wF+g+4)  << " <= \"00\" & alignedC; " << endl;
		
			IntMultiAdder* adder = new IntMultiAdder(target,wF+g+4, 3, inDelayMap("X0", target->localWireDelay() + getCriticalPath() ));
			oplist.push_back(adder);

			inPortMap   (adder, "X0", "paddedA");
			inPortMap   (adder, "X1", "paddedB");
			inPortMap   (adder, "X2", "paddedC");
			inPortMapCst(adder, "Cin", "'0'"); // a 1 would compensate the two truncations in the worst case -- to explore
			outPortMap  (adder, "R","sum");
			vhdl << instance(adder, "adder1");

			syncCycleFromSignal("sum", false);
			setCriticalPath(adder->getOutputDelay("R"));

			manageCriticalPath(target->localWireDelay() + target->lutDelay());
			// Possible 3-bit normalisation, with a truncation
			vhdl << tab << declare("finalFraction", wF+g)  << " <= " << endl
				  << tab << tab << "sum" << range(wF+g+2,3) << "   when sum(" << wF+g+3 << ")='1'    else " << endl
				  << tab << tab << "sum" << range(wF+g+1, 2) <<  "   when (sum" << range(wF+g+3, wF+g+2) << "=\"01\")     else " << endl
				  << tab << tab << "sum" << range(wF+g, 1) <<  "   when (sum" << range(wF+g+3, wF+g+1) << "=\"001\")     else " << endl
				  << tab << tab << "sum" << range(wF+g-1, 0) << "; " << endl;

			// Exponent datapath. We have to compute 2*EA - bias + an update corresponding to the normalisatiobn
			// since (1.m)*(1.m) = xx.xxxxxx sum is xxxx.xxxxxx
			// All the following ignores overflows, infinities, zeroes, etc for the sake of simplicity.
			manageCriticalPath(target->localWireDelay() + target->lutDelay());
			int bias = (1<<(wE-1))-1;
			vhdl << tab << declare("exponentUpdate", wE+1)  << " <= " << endl
				  << tab << tab << "CONV_STD_LOGIC_VECTOR(" << bias-3 << ", "<< wE+1 <<")  when sum(" << wF+g+3 << ")='1'    else " << endl
				  << tab << tab << "CONV_STD_LOGIC_VECTOR(" << bias-2 << ", "<< wE+1 <<")  when (sum" << range(wF+g+3, wF+g+2) << "=\"01\")     else " << endl
				  << tab << tab << "CONV_STD_LOGIC_VECTOR(" << bias-1 << ", "<< wE+1 <<")  when (sum" << range(wF+g+3, wF+g+1) << "=\"001\")     else " << endl
				  << tab << tab << "CONV_STD_LOGIC_VECTOR(" << bias   << ", "<< wE+1 <<")  ; " << endl;
		
			manageCriticalPath( target->localWireDelay() + target->adderDelay(wE+1));
			vhdl << tab << declare("finalExp", wE+1)  << " <= (EA & '0') - exponentUpdate ; " << endl;
			
			IntAdder *roundingAdder = new IntAdder(target, wE +1 + wF);
			oplist.push_back(roundingAdder);
			
			vhdl << tab << declare("roundingOp",wE+1 + wF) << "<= finalExp & finalFraction"<<range(wF+g-1,g)<<";"<<endl;
			
			inPortMap     ( roundingAdder, "X", "roundingOp");
			inPortMapCst  ( roundingAdder, "Y", zg(wE+1+wF));
			inPortMapCst   ( roundingAdder, "Cin", "'1'");
			outPortMap    ( roundingAdder, "R", "expFrac");
			vhdl << tab << instance( roundingAdder, "RoundingAdder"); 
			syncCycleFromSignal("expFrac");
			setCriticalPath( roundingAdder->getOutputDelay("R"));
				
			//TODO 		
			vhdl << tab << declare("rExc",2) << " <= \"01\" when  expFrac"<<of(wE+wF)<<"='0' else \"10\";"<<endl;
			vhdl << tab << "R <= rExc & '0' & expFrac"<<range(wE+1 + wF-2,0)<<";"<<endl;
		}
	
	}	


	FPSumOfSquares::~FPSumOfSquares()
	{
	}





	/* We compute the acceptable values according to the FP operator
		combination, using interval arithmetic, so emulate() is not stricter
		when the operator is the optimized one (TODO). All it checks is that
		the optimized version is at least as accurate as the FP one. */

	void FPSumOfSquares::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");
		mpz_class svZ = tc->getInputValue("Z");
		mpz_class svR2 = tc->getInputValue("R2");

		/* Compute correct value */
		FPNumber fpx(wE, wF);
		fpx = svX;
		FPNumber fpy(wE, wF);
		fpy = svY;
		FPNumber fpz(wE, wF);
		fpz = svZ;
		FPNumber fpr2(wE, wF);
		fpr2 = svR2;

		mpfr_t x,y,z, r2, ru, rd;
		mpfr_init2(x,  1+wF);
		mpfr_init2(y,  1+wF);
		mpfr_init2(z,  1+wF);
		mpfr_init2(r2, 1+wF);
		mpfr_init2(ru, 1+wF);
		mpfr_init2(rd, 1+wF);


		fpr2.getMPFR(r2); 

		fpx.getMPFR(x); 
		fpy.getMPFR(y); 
		fpz.getMPFR(z);

		mpfr_mul(x,x,x, GMP_RNDU);
		mpfr_mul(y,y,y, GMP_RNDU);
		mpfr_mul(z,z,z, GMP_RNDU);
		mpfr_add(x,x,y, GMP_RNDU);
		mpfr_add(ru,x,z, GMP_RNDU); // x^2+y^2+r^2, all rounded up
 
		fpx.getMPFR(x); 
		fpy.getMPFR(y); 
		fpz.getMPFR(z);

		mpfr_mul(x,x,x, GMP_RNDD);
		mpfr_mul(y,y,y, GMP_RNDD);
		mpfr_mul(z,z,z, GMP_RNDD);
		mpfr_add(x,x,y, GMP_RNDD);
		mpfr_add(rd,x,z, GMP_RNDD); // x^2+y^2+r^2, all rounded down
 
#if 0
		mpfr_out_str (stderr, 10, 30, x, GMP_RNDN); cerr << " ";
		mpfr_out_str (stderr, 10, 30, rd, GMP_RNDN); cerr << " ";
		mpfr_out_str (stderr, 10, 30, ru, GMP_RNDN); cerr << " ";
		cerr << endl;
#endif
		mpz_class predfalse = 0;
		mpz_class predtrue =  1;

		if(mpfr_cmp(r2, rd)<0) { // R2< X^2+Y^2+Z^2
			tc->addExpectedOutput("P", predfalse);
			tc->addExpectedOutput("P", predfalse);
		}
		else if(mpfr_cmp(r2, ru)>0) {
			tc->addExpectedOutput("P", predtrue);
			tc->addExpectedOutput("P", predtrue);
		}
		else{ // grey area
			tc->addExpectedOutput("P", predtrue);
			tc->addExpectedOutput("P", predfalse);
		}

		mpfr_clears(x,y,z,r2, ru, rd, NULL);

	}
 


	// This method is cloned from Operator, just resetting sign and exception bits
	// (because we don't have any exception support in this toy example)
	TestCase* FPSumOfSquares::buildRandomTestCase(int i){

		TestCase *tc;
		/* Generate test cases using random input numbers */
			tc = new TestCase(this); // TODO free all this memory when exiting TestBench
			/* Fill inputs */
			for (unsigned int j = 0; j < ioList_.size(); j++) {
				Signal* s = ioList_[j]; 
				if (s->type() == Signal::in) {
					// ****** Modification: positive normal numbers with small exponents 
					mpz_class m = getLargeRandom(wF);
					mpz_class bias = (mpz_class(1)<<(wE-1)) - 1; 
					mpz_class e = getLargeRandom(wE-2) - (mpz_class(1)<<(wE-3)) + bias; // ensure no overflow
					mpz_class a = (mpz_class(1)<<(wE+wF+1)) // 01 to denote a normal number
						+ (e<<wF) + m;
					tc->addInput(s->getName(), a);
				}
			}
			/* Get correct outputs */
			emulate(tc);

			//		cout << tc->getInputVHDL();
			//    cout << tc->getExpectedOutputVHDL();

			return tc;
	}
}
