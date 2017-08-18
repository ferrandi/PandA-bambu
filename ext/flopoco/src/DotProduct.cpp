/*
  Dot Product unit for FloPoCo

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
#include <time.h>
#include <gmp.h>
#include <mpfr.h>
#include <cstdlib>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"

#include "DotProduct.hpp"
#include "FPMultiplier.hpp"


using namespace std;

namespace flopoco{

	DotProduct::DotProduct(Target* target, int wE, int wFX, int wFY, int MaxMSBX, int LSBA, int MSBA, double ratio,  map<string, double> inputDelays):
		Operator(target), wE(wE), wFX(wFX), wFY(wFY), MaxMSBX(MaxMSBX), LSBA(LSBA), MSBA(MSBA)  {
	
		ostringstream name;

		srcFileName="DotProduct";
		name <<"DotProduct_"<<wE<<"_"<<wFX<<"_"<<wFY<<"_"
			  <<(MaxMSBX>=0?"":"M")<<abs(MaxMSBX)<<"_"
			  <<(LSBA>=0?"":"M")<<abs(LSBA)<<"_"
			  <<(MSBA>=0?"":"M")<<abs(MSBA) ;

		if(target->isPipelined()) 
			name << target->frequencyMHz() ;
		else
			name << "comb";
		setName(name.str()); 

		setCopyrightString("Bogdan Pasca, Florent de Dinechin (2008-2011)");		

	
		/* Set up the I/O signals of of the entity */
		addFPInput ("X", wE, wFX);
		addFPInput ("Y", wE, wFY);
		addInput   ("newDataSet");
		
		addOutput  ("A", MSBA-LSBA+1); //the width of the output represents the accumulator size
		addOutput  ("C", MSBA-LSBA+1); //the width of the output represents the accumulator size
		
		addOutput  ("XOverflow");  
		addOutput  ("XUnderflow");  
		addOutput  ("AccOverflow");  
		
		
 		sizeAcc_ = MSBA-LSBA+1;

		vhdl << tab << declare( "sX" ) << " <= X"<<of(wE+wFX+2)<<";"<<endl;
		vhdl << tab << declare( "sY" ) << " <= Y"<<of(wE+wFY+2)<<";"<<endl;
		
		vhdl << tab << declare( "excX", 2 ) << " <= X"<<range(wE+wFX+1,wE+wFX)<<";"<<endl;
		vhdl << tab << declare( "excY", 2 ) << " <= Y"<<range(wE+wFY+1,wE+wFY)<<";"<<endl;
		
		vhdl << tab << declare( "expX", wE ) << " <= X"<<range(wE+wFX-1,wFX)<<";"<<endl;
		vhdl << tab << declare( "expY", wE ) << " <= Y"<<range(wE+wFY-1,wFY)<<";"<<endl;
				
		vhdl << tab << declare("fracX", wFX+1) << " <= \"1\" & X"<<range(wFX-1,0)<<";"<<endl;
		vhdl << tab << declare("fracY", wFY+1) << " <= \"1\" & Y"<<range(wFY-1,0)<<";"<<endl;

		setCriticalPath( getMaxInputDelays(inputDelays) );

		/* sign */
		vhdl << tab << declare("signP") << " <= sX xor sY;"<<endl;

		/* multiply mantissas */
		IntMultiplier *mMult = new IntMultiplier(target, 1+wFX, 1+wFY, 
		                                         0 /* untruncated*/,  
		                                         false /*unsigned*/,  
		                                         ratio,
		                                         inDelayMap("X", target->localWireDelay() + getCriticalPath()));
		oplist.push_back(mMult); 

		inPortMap ( mMult, "X", "fracX");
		inPortMap ( mMult, "Y", "fracY");
		outPortMap( mMult, "R", "mFrac");
		
		vhdl << tab << instance( mMult, "MantisaMultiplier");
		setSignalDelay("mFrac", mMult->getOutputDelay("R"));

		/*in parallel manage exponents */
		manageCriticalPath( target->localWireDelay() + target->adderDelay(wE+1));		
		vhdl << tab << declare("sumExp",wE+1) << " <= (\"0\" & expX) + (\"0\" & expY);"<<endl;
		
		/*check overflow*/
		vhdl << tab << declare("overflow") << " <= sumExp"<<of(wE)<<";"<<endl;
		
		/* subtract bias */
		manageCriticalPath( target->localWireDelay() + target->adderDelay(wE+1));		
		vhdl << tab << declare("sumExpMBias",wE+1) << " <= sumExp - (\"0\" & CONV_STD_LOGIC_VECTOR("<<intpow2(wE-1)-1<<","<<wE<<"));"<<endl;
		
		/*check underflow*/
		vhdl << tab << declare("underflow") << " <= sumExpMBias"<<of(wE)<<";"<<endl;
		
		/*set exceptions */
		vhdl << tab << declare("excConcat",4) << "<= excX & excY;"<<endl;
		
		manageCriticalPath( target->localWireDelay() + target->lutDelay());
		vhdl << tab << " with excConcat select " << endl;
		vhdl << tab << declare( "excPhase1",2 ) << " <=  \"00\" when \"0000\","<<endl
		     << tab << tab << "\"01\" when \"0101\","<<endl
		     << tab << tab << "\"10\" when \"1001\"|\"0110\"|\"1010\","<<endl
		     << tab << tab << "\"11\" when others;"<<endl;

		vhdl << tab << declare("excConcatOU",4) << " <= excPhase1 & overflow & underflow;"<<endl;

		manageCriticalPath( target->localWireDelay() + target->lutDelay());
		vhdl << tab << " with excConcatOU select " << endl;
		vhdl << tab << declare( "exc",2 ) << " <=  \"00\" when \"0101\","<<endl
		     << tab << tab << "\"10\" when \"0110\","<<endl
		     << tab << tab << "excPhase1 when others;"<<endl;
		
		setSignalDelay("exc", getCriticalPath());
		
		syncCycleFromSignal("mFrac", getSignalDelay("mFrac"));			

		/* now we instantiate the accumulator */
		LongAcc* la = new LongAcc(target, wE, wFX, MaxMSBX, LSBA, MSBA, inDelayMap("sigX_dprod",target->localWireDelay() + getCriticalPath()),true, wFY);
		oplist.push_back(la);
		
		inPortMap( la, "sigX_dprod", "signP");
		inPortMap( la, "excX_dprod", "exc");
		inPortMap( la, "fracX_dprod", "mFrac");
		inPortMapCst( la, "expX_dprod", "sumExpMBias"+range(wE-1,0));

		inPortMap( la, "newDataSet", "newDataSet");
		outPortMap(la, "ready", "accReady");
		outPortMap(la, "A", "accA");  
		outPortMap(la, "C", "accC");
		outPortMap(la, "XOverflow", "accXOverflow");  
		outPortMap(la, "XUnderflow", "accXUnderflow");  
		outPortMap(la, "AccOverflow", "accAccOverflow");  

		vhdl << tab << instance(la, "Accumulator");
		syncCycleFromSignal("A");
		
		vhdl << tab << "A <= accA;"<<endl;
		vhdl << tab << "C <= accC;"<<endl;
		vhdl << tab << "XOverflow <= accXOverflow;"<<endl;
		vhdl << tab << "XUnderflow <= accXUnderflow;"<<endl;
		vhdl << tab << "AccOverflow <= accAccOverflow;"<<endl;
	}

	DotProduct::~DotProduct() {
	}

	void DotProduct::test_precision(int n) {
		mpfr_t ref_acc, long_acc, fp_acc, x, y, r_mul_longAcc, r_mul_fp, d, one, two, msb;
		double sum, error;

		// initialisation
		mpfr_init2(ref_acc, 10000);
		mpfr_init2(long_acc, sizeAcc_+1);
//#define DPACC

#define QPACC

#ifdef DPACC
		mpfr_init2(fp_acc, 53);
#else

#ifdef QPACC
		mpfr_init2(fp_acc, 113);
#else
		mpfr_init2(fp_acc, wFX+1);
#endif
#endif
		mpfr_init2(x, wFX+1);
		mpfr_init2(y, wFY+1);
	
		mpfr_init2(r_mul_longAcc, 2*(wFX+1) );
		mpfr_init2(r_mul_fp, wFX+1 );

		mpfr_init2(d, 10000);
		mpfr_init2(one, 100);
		mpfr_init2(two, 100);
		mpfr_init2(msb, 10);
		mpfr_set_d(one, 1.0, GMP_RNDN);
		mpfr_set_d(two, 2.0, GMP_RNDN);
		mpfr_set_d(msb, (double)(1<<(MSBA+1)), GMP_RNDN); // works for MSBA_<32
		mpfr_mul_2si(msb, one, MSBA+1, GMP_RNDN); // works for MSBA_<32

		cout <<endl;
		mpfr_out_str(stdout, 2, sizeAcc_, msb, GMP_RNDN);
		cout <<endl;

		//cout<<"%-------Acc. of positive numbers--------------- "<<endl;
		cout<<"-------Dot-prod of positive numbers unif [0,1] --------- "<<endl;
		mpfr_set_d(ref_acc, 0.0, GMP_RNDN);
		mpfr_set_d(fp_acc, 0.0, GMP_RNDN);
		mpfr_set_d(long_acc, 0.0, GMP_RNDN);
		//put a one in the MSBA_+1 bit will turn all the subsequent additions
		// into fixed-point ones
		mpfr_add(long_acc, long_acc, msb, GMP_RNDN);

		gmp_randstate_t state;
		gmp_randinit_default(state);

		for(int i=0; i<n; i++){
			mpfr_urandomb(d,state); // deprecated function; r is [0,1]
			mpfr_set(x,d,GMP_RNDN);		
			mpfr_urandomb(d,state); // deprecated function; r is [0,1]
			mpfr_set(y,d,GMP_RNDN);		

			mpfr_mul(r_mul_longAcc,x,y,GMP_RNDN);//    mpfr_add(r, one, r, GMP_RNDN); // r in [1 2[
			mpfr_mul(r_mul_fp     ,x,y,GMP_RNDN);//    mpfr_add(r, one, r, GMP_RNDN); // r in [1 2[
		
			mpfr_add(fp_acc, fp_acc, r_mul_fp, GMP_RNDN);
			mpfr_add(ref_acc, ref_acc, r_mul_longAcc, GMP_RNDN);		
			mpfr_add(long_acc, long_acc, r_mul_longAcc, GMP_RNDZ);
			
		}
		cout <<endl;
		mpfr_out_str(stdout, 2, sizeAcc_+1, long_acc, GMP_RNDN);
		cout <<endl;
		mpfr_out_str(stdout, 2, sizeAcc_+1, ref_acc, GMP_RNDN);
		cout <<endl;

		// remove the leading one from long acc
		mpfr_sub(long_acc, long_acc, msb, GMP_RNDN);

		cout <<endl;
		cout << "Long accumulator result:"<<endl;
		mpfr_out_str(stdout, 2, sizeAcc_+1, long_acc, GMP_RNDN);
		cout <<endl;
		cout << "Reference accumulator result:"<<endl;
		mpfr_out_str(stdout, 2, sizeAcc_+1, ref_acc, GMP_RNDN);
		cout <<endl;

		cout << "*************----***********"<<endl;		
		sum=mpfr_get_d(ref_acc, GMP_RNDN);
		cout  << "% unif[0 1] :sum="<< sum;
		sum=mpfr_get_d(fp_acc, GMP_RNDN);
		cout << "   FPAcc="<< sum;
		sum=mpfr_get_d(long_acc, GMP_RNDN);
		cout << "   LongAcc="<< sum;

		cout <<endl << n << " & ";
		// compute the error for the FP adder
		mpfr_sub(d, fp_acc, ref_acc, GMP_RNDN);
		mpfr_div(d, d, ref_acc, GMP_RNDN);
		error=mpfr_get_d(d, GMP_RNDN);
		cout << " Relative error between fp_acc and ref_acc is: "<<endl;
		cout << scientific << setprecision(2)  << error << endl;

		// compute the error for the long acc
		mpfr_sub(d, long_acc, ref_acc, GMP_RNDN);
		mpfr_div(d, d, ref_acc, GMP_RNDN);
		error=mpfr_get_d(d, GMP_RNDN);
		cout << "Relative error between long_acc and ref_acc is: "<<endl;
		cout << scientific << setprecision(2)  << error << endl;

		cout<<"-------Dot-prod of positive numbers unif [-1,1] --------- "<<endl;

		/* initialize random seed: */
		srand ( time(NULL) );

		mpfr_set_d(ref_acc, 0.0, GMP_RNDN);
		mpfr_set_d(fp_acc, 0.0, GMP_RNDN);
		//put a one in the MSBA_+1 bit will turn all the subsequent additions
		// into fixed-point ones
		mpfr_set_d(long_acc, (double)(1<<(MSBA+1)), GMP_RNDN);

		int sign;
		for(int i=0; i<n; i++){
			mpfr_urandomb(d,state); 
			mpfr_set(x,d,GMP_RNDN);		
			mpfr_urandomb(d,state); 
			mpfr_set(y,d,GMP_RNDN);		

			mpfr_mul(r_mul_fp, x, y,GMP_RNDN);
			mpfr_mul(r_mul_longAcc, x, y,GMP_RNDN);
			
			/* generate sign: */
	 		sign = rand() % 2;
			if (sign){
				mpfr_add(fp_acc, fp_acc, r_mul_fp, GMP_RNDN);
				mpfr_add(ref_acc, ref_acc, r_mul_longAcc, GMP_RNDN);
				mpfr_add(long_acc, long_acc, r_mul_longAcc, GMP_RNDN);
			}else{
				mpfr_sub(fp_acc, fp_acc, r_mul_fp, GMP_RNDN);
				mpfr_sub(ref_acc, ref_acc, r_mul_longAcc, GMP_RNDN);
				mpfr_sub(long_acc, long_acc, r_mul_longAcc, GMP_RNDN);
			}
		}

		// remove the leading one from long acc
		mpfr_sub(long_acc, long_acc, msb, GMP_RNDN);

		// compute the error for the FP adder
		mpfr_sub(d, fp_acc, ref_acc, GMP_RNDN);
		mpfr_div(d, d, ref_acc, GMP_RNDN);
		error=mpfr_get_d(d, GMP_RNDN);
		cout << "Relative error between fp_acc and ref_acc is :"<<endl;
		cout << scientific << setprecision(2) << error << endl;

		// compute the error for the long acc
		mpfr_sub(d, long_acc, ref_acc, GMP_RNDN);
		mpfr_div(d, d, ref_acc, GMP_RNDN);
		error=mpfr_get_d(d, GMP_RNDN);
		cout << "Relative error between long_acc and ref_acc is " << endl;
		cout << scientific << setprecision(2)  << error <<endl;

		sum=mpfr_get_d(ref_acc, GMP_RNDN);
		cout << "% unif[-1 1] : sum="<< sum;
		sum=mpfr_get_d(fp_acc, GMP_RNDN);
		cout << "   FPAcc="<< sum;
		sum=mpfr_get_d(long_acc, GMP_RNDN);
		cout << "   LongAcc="<< sum;
		cout <<endl;
	}
}
