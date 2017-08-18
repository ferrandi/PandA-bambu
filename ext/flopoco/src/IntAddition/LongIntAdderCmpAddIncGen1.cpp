/*
  A FIXME for FloPoCo. 
 
   Author: Bogdan Pasca

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.

 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "LongIntAdderCmpAddIncGen1.hpp"


#define XILINX_OPTIMIZATION 0
// #define MAXSIZE
// #define RCATEST

using namespace std;
namespace flopoco{

	LongIntAdderCmpAddIncGen1::LongIntAdderCmpAddIncGen1(Target* target, int wIn, map<string, double> inputDelays):
		Operator(target), wIn_(wIn), inputDelays_(inputDelays) 
	{
		srcFileName="LongIntAdderCmpAddIncGen1";
		setName(join("LongIntAdderCmpAddIncGen1_", wIn_));
//		int version = 1; /* this will go into the parameters */
		
		// Set up the IO signals
		for (int i=0; i<2; i++)
			addInput ( join("X",i) , wIn_);
		addInput("Cin");
		addOutput("R"  , wIn_);

		if (verbose){
			cout <<"delay for X is   "<< inputDelays["X"]<<endl;	
			cout <<"delay for Y is   "<< inputDelays["Y"]<<endl;
			cout <<"delay for Cin is "<< inputDelays["Cin"]<<endl;
		}

		if (isSequential()){

			double xordelay;
			double dcarry;
			double muxcystoo;
			if (target->getID()=="Virtex5"){
				xordelay = 0.300e-9;
				dcarry = 0.023e-9;
				muxcystoo = 0.305e-9;
			}else{ 
				if (target->getID()=="Virtex6"){
					xordelay = 0.180e-9;
					dcarry = 0.015e-9;
					muxcystoo =	0.219e-9;
				}else{ 
					if (target->getID()=="Virtex4"){
						xordelay = 0.273e-9;
						dcarry = 0.034e-9;
						muxcystoo = 0.278e-9;
					}
				}
			}
			
// 			double t;
			int ll,l0,l1,maxAdderSize;
			
			
#ifdef RCATEST
	for (int aa=25; aa<=400; aa+=25){
		target->setFrequency(double(aa)*1000000.0);
		int y;
		target->suggestSubaddSize(y, 10000);
		cout << "f="<<aa<<" s="<<y<<endl;
	}
	exit(-1);
#endif			
			
#ifdef MAXSIZE
for (int aa=25; aa<=400; aa+=25){
	target->setFrequency(double(aa)*1000000.0);
#endif

double t = 1.0 / target->frequency();				

ll = (1.0/2.0)* ((t - 3*target->lutDelay()-3*xordelay-3*muxcystoo-2*target->localWireDelay())/dcarry + 2);
l1=ll-1;



// 			double c = (2 * target->localWireDelay()) + target->lutDelay()+ xordelay; 
// 			target->suggestSlackSubaddSize(ll, wIn, (t+c)/2);
// 			
// 			l1=ll-1;
// 			
			target->suggestSlackSubaddSize(l0, wIn, t - (target->lutDelay()+ target->adderDelay(l1)) );
			
			maxAdderSize =  l0+l1+ll*(ll+1)/2;
						
			REPORT(INFO, "ll="<<ll);
			REPORT(INFO, "max adder size is="<< maxAdderSize);

#ifdef MAXSIZE
cout << " f="<<aa<<" s="<<maxAdderSize<<endl;
}
exit(1);
#endif
/*			exit(-1);*/
			
			cSize = new int[1000];
			
			cSize[0]=l0;
			cSize[1]=l1;
			cSize[2]=ll;
			
			int td = wIn;
			td -= (l0+l1+ll);
			
			if ((td < 0) || (wIn>maxAdderSize)){
				cout << "OOOups ..." <<endl;
				exit(-1);
			}
			
			nbOfChunks = 3;
			while (td>0){
				int nc = cSize[nbOfChunks-1] -1;
				if (nc >= td){
					//finish
					cSize[nbOfChunks] = td;
					td = 0;
					nbOfChunks++;
				}else{
					cSize[nbOfChunks]= cSize[nbOfChunks-1]-1;
					td-=cSize[nbOfChunks];
					nbOfChunks++;
				}
			}
				
				
			
/*			//declare the constant
			double c = (2 * target->localWireDelay()) + target->adderDelay(1); 
			double t = 1.0 / target->frequency();
			//compute l
			int ll;
			target->suggestSlackSubaddSize(ll, wIn, t - (t-c)/2);
			
			int maxAdderSize =  2*ll+ll*(ll+1)/2;
			REPORT(INFO, "ll="<<ll);
			REPORT(INFO, "max adder size is="<< maxAdderSize);
			
			if (wIn>maxAdderSize) 
				REPORT(INFO, "cannot do proper chunk splitting");
			
			cSize = new int[100];
						
			int td = wIn;
			cSize[0] = ll;
			cSize[1] = ll;
			td -= 2*ll;
			nbOfChunks = 2;
			while (td > 0){
				nbOfChunks++;
				cSize[nbOfChunks-1] = min(ll, td);
				td -= min(ll, td);
				ll--;
			}*/
					
			//=================================================
			//split the inputs ( this should be reusable )
			vhdl << tab << "--split the inputs into chunks of bits depending on the frequency" << endl;
			for (int i=0;i<2;i++){
				for (int j=0; j<nbOfChunks; j++){
					ostringstream name;
					//the naming standard: sX j _ i _ l
					//j=the chunk index i is the input index and l is the current level
					name << "sX"<<j<<"_"<<i<<"_l"<<0;
					int low=0, high=0;
					for (int k=0;k<=j;k++)
						high+=cSize[k];
					for (int k=0;k<=j-1;k++)
						low+=cSize[k];
					if (j==0)
						vhdl << tab << declare (name.str(),cSize[j]+1, true) << " <=  \"0\" & X"<<i<<range(high-1,low)<<";"<<endl;
					else
						vhdl << tab << declare (name.str(),cSize[j], true) << " <= X"<<i<<range(high-1,low)<<";"<<endl;
				}
			}	
			vhdl << tab << declare("scIn") << " <= Cin;"<<endl;
			
			int l=1;
			for (int j=0; j<nbOfChunks; j++){
				if (j>0){ //for all chunks greater than zero we perform this comparissons
#ifndef XILINX_OPTIMIZATION	
					vhdl<<tab<<declare(join("sX",j,"_0_l",l,"_Zero"), cSize[j]+1, true)<< " <= (\"0\" & " << join("sX",j,"_0_l",l-1)
					<< ") + (\"0\" & "<<join("sX",j,"_1_l",l-1)<<");"<<endl;
					vhdl<<tab<<declare(join("sX",j,"_0_l",l,"_One"))<< "  <= '1' when "<< join("sX",j,"_0_l",l-1)<< " >= not("<<join("sX",j,"_1_l",l-1)<<") else '0';"<<endl;
#else 
int tp = target->isPipelined();
target->setNotPipelined();
IntAdder *addZero = new IntAdder(target, cSize[j]+1);
IntComparator *compOne = new IntComparator(target, cSize[j], 1, false, 0);

oplist.push_back(addZero);
oplist.push_back(compOne);

if (tp) target->setPipelined();

vhdl << tab << declare(join("opX",j), cSize[j]+1) << " <= \"0\" & " << join("sX",j,"_0_l",l-1)<<";"<<endl;
vhdl << tab << declare(join("opY",j), cSize[j]+1) << " <= \"0\" & " << join("sX",j,"_1_l",l-1)<<";"<<endl;


inPortMap(addZero, "X", join("opX",j) );
inPortMap(addZero, "Y", join("opY",j) );
inPortMapCst(addZero, "Cin", "'0'");
outPortMap(addZero, "R", join("sX",j,"_0_l",l,"_Zero") );
vhdl << instance(addZero, join("addZ",j) );

vhdl << tab << declare(join("nsX",j,"_1_l",l-1), cSize[j]) << " <= not("<<join("sX",j,"_1_l",l-1)<<");"<<endl;

inPortMap(compOne, "X", join("sX",j,"_0_l",l-1) );
inPortMap(compOne, "Y", join("nsX",j,"_1_l",l-1) );
outPortMap(compOne, "R", join("sX",j,"_0_l",l,"_One") );
vhdl << instance(compOne, join("cmpO",j) );


#endif
				
				
				}else{
					//for the zero chunk we directly perform the addition
					vhdl<<tab<< "-- the carry resulting from the addition of the chunk + Cin is obtained directly" << endl;
					vhdl<<tab<<declare(join("sX",j,"_0_l",l,"_Cin"),cSize[j]+1, true)<< "  <= " << join("sX",j,"_0_l",l-1)<<" + "<<join("sX",j,"_1_l",l-1)<<" + scIn;"<<endl;
				}
			}
			
			vhdl << tab <<"--form the two carry string"<<endl;
			vhdl << tab << declare("carryStringZero",nbOfChunks-2, true) << " <= "; 
			for (int i=nbOfChunks-3; i>=0; i--) {
				vhdl << "sX"<<i+1<<"_0_l"<<l<<"_Zero"<<of(cSize[i+1])<< (i>0?" & ":";") ;
			} vhdl << endl;
			
			vhdl << tab << declare("carryStringOne",  nbOfChunks-2, true) << "  <= "; 
			for (int i=nbOfChunks-3; i>=0; i--) {
				vhdl << "sX"<<i+1<<"_0_l"<<l<<"_One" << (i>0?" & ":";");
			} vhdl << endl;
			
			//				nextCycle();/////////////////////
			
			vhdl << tab << "--perform the short carry additions" << endl;
			ostringstream unameCin;
			unameCin  << "sX"<<0<<"_0_l"<<l<<"_Cin";
			vhdl << tab << declare("rawCarrySum",nbOfChunks-2,true) << " <= carryStringOne + carryStringZero + " << unameCin.str() <<of(cSize[0])<< ";" << endl;
			
			
			
			//				vhdl << tab << declare("manipulatedSum",nbOfChunks-2) << "<= carryStringOne AND ( (not(rawCarrySum) AND not(carryStringZero)) OR carryStringZero);" << endl; //strike of genious
			vhdl << tab << declare("manipulatedSum",nbOfChunks-2, true) << "<= (not(rawCarrySum) AND carryStringOne) OR carryStringZero;" << endl; //strike of genious
			
			//				if (invalid)
			//					nextCycle();/////////////////////
			
			vhdl << tab <<"--get the final pipe results"<<endl;
			for ( int i=0; i<nbOfChunks; i++){
				ostringstream unameZero, unameOne, unameCin;
				unameZero << "sX"<<i<<"_0_l"<<l<<"_Zero";
				unameOne  << "sX"<<i<<"_0_l"<<l<<"_One";
				unameCin  << "sX"<<0<<"_0_l"<<l<<"_Cin";
				if (i==0) 
					vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << "sX0_0_l1_Cin"<< range(cSize[0]-1,0) <<  ";" << endl;
				else {
					if (i==1){ 
						vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << join("sX",i,"_0_l1_Zero")<<range(cSize[i]-1,0) 
// 						<< " + " << join("sX",i,"_1_l0")<<range(cSize[i]-1,0) 
						<< " + sX0_0_l1_Cin"<<of(cSize[i-1]) <<";"<<endl;
					}else{      
						vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << join("sX",i,"_0_l1_Zero")<<range(cSize[i]-1,0) 
// 						<< " + " << join("sX",i,"_1_l0")<<range(cSize[i]-1,0) 
						<< " + manipulatedSum"<<of(i-2) <<";"<<endl;
					}
				}
			}
			
			vhdl << tab << "R <= ";
			int k=0;
			for (int i=nbOfChunks-1; i>=0; i--){
				vhdl << use(join("res",i));
				if (i > 0) vhdl << " & ";
				k++;
			}
			vhdl << ";" <<endl;
			
		}else
			vhdl << tab << " R <= X0 + X1 + Cin;"<<endl;
		
	}

	LongIntAdderCmpAddIncGen1::~LongIntAdderCmpAddIncGen1() {
	}


	void LongIntAdderCmpAddIncGen1::emulate(TestCase* tc)
	{
		mpz_class svX[2];
		for (int i=0; i<2; i++){
			ostringstream iName;
			iName << "X"<<i;
			svX[i] = tc->getInputValue(iName.str());
		}
		mpz_class svC =  tc->getInputValue("Cin");

		mpz_class svR = svX[0] + svC;
		mpz_clrbit(svR.get_mpz_t(),wIn_); 
		for (int i=1; i<2; i++){
			svR = svR + svX[i];
			mpz_clrbit(svR.get_mpz_t(),wIn_); 
		}

		tc->addExpectedOutput("R", svR);

	}


}
