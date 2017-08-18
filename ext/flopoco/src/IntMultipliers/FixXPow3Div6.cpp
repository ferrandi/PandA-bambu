/*
 * Operator computing (X^3)/6, for X being a signed two's complement number
 
 Assumptions: 0<=X<1

 This file is part of the FloPoCo project developed by the Arenaire/ARIC
 team at Ecole Normale Superieure de Lyon
  
 Author : Florent de Dinechin, Matei Istoan

 Initial software.
 Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
 2008-2013.
  All rights reserved.
*/

#include "FixXPow3Div6.hpp"


using namespace std;

namespace flopoco{


	//FIXME: for now, the output width (wOut) is computed inside the operator, 
	//		and not from the parameters given to the constructor
	
	//standalone operator 
	FixXPow3Div6::FixXPow3Div6(Target* target, int msbIn_, int lsbIn_, int msbOut_, int lsbOut_, bool signedInput_, map<string, double> inputDelays) :
		Operator(target, inputDelays), msbIn(msbIn_), lsbIn(lsbIn_), msbOut(msbOut_), lsbOut(lsbOut_), signedInput(signedInput_)
	{
		srcFileName="FixXPow3Div6";

		if(lsbIn > msbIn)
			throw string("FixXPow3Div6: Error, lsbIn should not be greater than msbIn");
    
		if(lsbIn > msbIn)
			throw string("FixXPow3Div6: Error, lsbOut should not be greater than msbOut");
		
		wIn = msbIn - lsbIn + 1;
		wOut = 3*msbIn - 3*lsbIn + 1 + 1;				//one more bit to cover all the range of produced bits
		
		// build the name
		ostringstream name; 
		name <<"FixXPow3Div6_" << vhdlize(wIn) << "_" << vhdlize(wOut) << "_" << (signedInput ? "signed" : "unsigned");
		setName(name.str());
		
		//create the input and the output
		addInput("X", wIn);
		addOutput("R", wOut);
		
		//create the bitheap that computes the sum
		bitHeap = new BitHeap(this, wOut);
		
		//add the terms corresponding to sum_{i=imin}^imax(2^(3i-1)*x_i)
		IntConstDiv3 *divider;
		divider = new IntConstDiv3(target, wIn, 3, -1, 2);
		addSubComponent(divider);

		//manageCriticalPath(divider->getOutputDelay("Q"));
  
		inPortMap (divider , "X", "X");
		outPortMap(divider , "Q", "XZeroIntDiv3");
		vhdl << instance(divider , "Divider");
		
		for(int i=0; i<=((msbIn-lsbIn+1)*3-2)-1; i++)
		{
			stringstream s;
			
			s << "XZeroIntDiv3" << of(i);
			
			bitHeap->addBit(i, s.str());
		}
		
		//add the terms corresponding to sum_i_j_imin^imax(2^(i+2j-1)*x_i*x_j)
		for(int i=lsbIn; i<=msbIn; i++)
			for(int j=lsbIn; j<=msbIn; j++)
			{
				//add the bit only if i != j
				if(i == j)
					continue;
				
				stringstream s;
				
				vhdl << tab << declare(join("X_temp_", (lsbIn<0 ? i-lsbIn : i), "_", (lsbIn<0 ? j-lsbIn : j))) 
					<< " <= " <<  "X" << of(lsbIn<0 ? i-lsbIn : i) << " and X" << of(lsbIn<0 ? j-lsbIn : j) << ";" << endl;
							
				s << join("X_temp_", (lsbIn<0 ? i-lsbIn : i), "_", (lsbIn<0 ? j-lsbIn : j));
				
				bitHeap->addBit((lsbIn<0 ? (i-lsbIn)+2*(j-lsbIn)-1 : i+2*j-1) + 1, s.str());
			}
			
		//add the terms corresponding to sum_i_j_k_imin^imax(2^(i+j+k)*x_i*x_j*x_k)
		for(int i=lsbIn; i<=msbIn; i++)
			for(int j=i+1; j<=msbIn; j++)
				for(int k=j+1; k<=msbIn; k++)
				{
					//i < j < k
					stringstream s;					
					
					vhdl << tab << declare(join("X_temp2_", (lsbIn<0 ? i-lsbIn : i), "_", (lsbIn<0 ? j-lsbIn : j), "_", (lsbIn<0 ? k-lsbIn : k))) 
						<< " <= X_temp_" << (lsbIn<0 ? i-lsbIn : i) << "_" << (lsbIn<0 ? j-lsbIn : j) << " and X" << of(lsbIn<0 ? k-lsbIn : k) << ";" << endl;
					
					s << join("X_temp2_", (lsbIn<0 ? i-lsbIn : i), "_", (lsbIn<0 ? j-lsbIn : j), "_", (lsbIn<0 ? k-lsbIn : k));
					
					bitHeap->addBit((lsbIn<0 ? (i-lsbIn)+(j-lsbIn)+(k-lsbIn) : i+j+k) + 1, s.str());
				}
				
		//compress the bitheap
		bitHeap -> generateCompressorVHDL();
		
		//generate the final result
		vhdl << tab << "R" << " <= " << bitHeap-> getSumName() << range(wOut-1, 0) << ";" << endl;
						
	}
	
	
	//operator incorporated into a global compression
	//	for use as part of a bigger operator
	FixXPow3Div6::FixXPow3Div6(Operator* parentOp_, Target* target, Signal* multiplicandX, int msbIn_, int lsbIn_, int msbOut_, int lsbOut_, BitHeap* bitHeap_, bool signedInput_, map<string, double> inputDelays) :
		Operator(target, inputDelays), msbIn(msbIn_), lsbIn(lsbIn_), msbOut(msbOut_), lsbOut(lsbOut_), signedInput(signedInput_),
		wIn(msbIn_-lsbIn+1), wOut(msbOut_-lsbOut+1),
		parentOp(parentOp_), bitHeap(bitHeap_) 
	{
		srcFileName="FixXPow3Div6";

		// build the name
		ostringstream name; 
		name <<"FixXPow3Div6_" << vhdlize(wIn) << "_" << vhdlize(wOut) << "_" << (signedInput?"_signed":"_unsigned");
		setName(name.str()); 

		
	}




	FixXPow3Div6::~FixXPow3Div6()
	{
		// TODO 
	}


	void FixXPow3Div6::emulate(TestCase* tc)
	{
		// get I/O values
		mpz_class svX = tc->getInputValue("X");
		mpz_class svR;
		
		//compute the value of the result
		svR = svX*svX*svX;
		svR = svR/6;
		
		// add the result
		tc->addExpectedOutput("R", svR);
	}

	/*
	void FixXPow3Div6::buildStandardTestCases(TestCaseList* tcl)
	{

	}
	*/

}




