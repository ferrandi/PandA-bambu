/*
  A FIXME for FloPoCo
 
  It may be pipelined to arbitrary frequency.
  Also useful to derive the carry-propagate delays for the subclasses of Target
 
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

#include "LongIntAdderAddAddMuxGen1.hpp"

#define XILINX_OPTIMIZATION 0

//#define MAXSIZE
using namespace std;
namespace flopoco{

	LongIntAdderAddAddMuxGen1::LongIntAdderAddAddMuxGen1(Target* target, int wIn, map<string, double> inputDelays):
		Operator(target), wIn_(wIn), inputDelays_(inputDelays) 
	{
		srcFileName="LongIntAdderAddAddMuxGen1";
		setName(join("LongIntAdderAddAddMuxGen1_", wIn_));
		int version = 1; /* this will go into the parameters */
		
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

			if (version == 1){
				//compute the maximum input delay
				maxInputDelay = getMaxInputDelays(inputDelays);
				
				
				if (false){
					if (verbose)
						cout << "The maximum input delay is "<<	maxInputDelay<<endl;
					
					cSize = new int[2000];
					REPORT(3, "-- The new version: direct mapping without 0/1 padding, IntAdders instantiated");
					double	objectivePeriod = double(1) / target->frequency();
					REPORT(2, "Objective period is "<< objectivePeriod <<" at an objective frequency of "<<target->frequency());
					target->suggestSubaddSize(chunkSize_ ,wIn_);
					REPORT(2, "The chunkSize for first two chunks is: " << chunkSize_ );
					
					if (2*chunkSize_ >= wIn_){
						cerr << "ERROR FOR NOW -- instantiate int adder, dimmension too small for LongIntAdderAddAddMuxGen1" << endl;
						exit(0);
					}
					
					cSize[0] = chunkSize_;
					cSize[1] = chunkSize_;
					
					bool finished = false; /* detect when finished the first the first
					phase of the chunk selection algo */
					int width = wIn_ - 2*chunkSize_; /* remaining size to split into chunks */
					int propagationSize = 0; /* carry addition size */
					int chunkIndex = 2; /* the index of the chunk for which the size is
					to be determined at the current step */
					bool invalid = false; /* the result of the first phase of the algo */
					
					/* FIRST PHASE */
					REPORT(3, "FIRST PHASE chunk splitting");
					while (not (finished))	 {
						REPORT(2, "The width is " << width);
						propagationSize+=2;
						double delay = objectivePeriod - target->adderDelay(width)- target->adderDelay(propagationSize); //2*target->localWireDelay()  -
						REPORT(2, "The value of the delay at step " << chunkIndex << " is " << delay);
						if ((delay > 0) || (width < 4)) {
							REPORT(2, "finished -> found last chunk of size: " << width);
							cSize[chunkIndex] = width;
							finished = true;
						}else{
							REPORT(2, "Found regular chunk ");
							int cs; 
							double slack =  target->adderDelay(propagationSize) ; //+ 2*target->localWireDelay()
							REPORT(2, "slack is: " << slack);
							REPORT(2, "adderDelay of " << propagationSize << " is " << target->adderDelay(propagationSize) );
							target->suggestSlackSubaddSize( cs, width, slack);
							REPORT(2, "size of the regular chunk is : " << cs);
							width = width - cs;
							cSize[chunkIndex] = cs;
							
							if ( (cSize[chunkIndex-1]<=2) && (cSize[chunkIndex-1]<=2) && ( invalid == false) ){
								REPORT(1, "[WARNING] Register level inserted after carry-propagation chain");
								invalid = true; /* invalidate the current splitting */
							}
							chunkIndex++; /* as this is not the last pair of chunks,
							pass to the next pair */
						}
					}
					REPORT(2, "First phase return valid result: " << invalid);
					
					/* SECOND PHASE: 
					only if first phase is cannot return a valid chunk size
					decomposition */
					if (invalid){
						REPORT(2,"SECOND PHASE chunk splitting ...");
						target->suggestSubaddSize(chunkSize_ ,wIn_);
						lastChunkSize_ = (wIn_% chunkSize_ ==0 ? chunkSize_ :wIn_% chunkSize_);
						
						/* the index of the last chunk pair */
						chunkIndex = (wIn_% chunkSize_ ==0 ? ( wIn_ / chunkSize_) - 1 :  (wIn_-lastChunkSize_) / chunkSize_ ); 								
						for (int i=0; i < chunkIndex; i++)
							cSize[i] = chunkSize_;
						/* last chunk is handled separately  */
						cSize[chunkIndex] = lastChunkSize_;
					}
					
					/* VERIFICATION PHASE: check if decomposition is correct */		
					REPORT(2, "found " << chunkIndex + 1  << " chunks ");
					nbOfChunks = chunkIndex + 1; 
					int sum = 0;
					ostringstream chunks;
					for (int i=chunkIndex; i>=0; i--){
						chunks << cSize[i] << " ";
						sum+=cSize[i];
					}
					chunks << endl;
					REPORT(2, "Chunks are: " << chunks.str());
					REPORT(2, "The chunk size sum is " << sum << " and initial width was " << wIn_);
					if (sum != wIn_){
						cerr << "ERROR: check the algo" << endl; /*should never get here ... */
						exit(0);
					}
				}
				
				
				int ll,l0;
				double xordelay;
				// double dcarry, muxcystoo;
				if (target->getID()=="Virtex5"){
					xordelay = 0.300e-9;
					// dcarry = 0.023e-9;
					// muxcystoo = 0.305e-9;
					
				}else{ 
					if (target->getID()=="Virtex6"){
						xordelay = 0.180e-9;
						// dcarry = 0.015e-9;
						// muxcystoo =	0.219e-9;
					}else{ 
						if (target->getID()=="Virtex4"){
							xordelay = 0.273e-9;
							// dcarry = 0.034e-9;
							// muxcystoo = 0.278e-9;
						}
					}
				}
				int lkm1;
				
#ifdef MAXSIZE
for (int aa=25; aa<=400; aa+=25){
	target->setFrequency(double(aa)*1000000.0);

#endif

double t = 1.0 / target->frequency();

				if (!target->suggestSlackSubaddSize(lkm1, wIn, target->localWireDelay() + target->lutDelay()))
					REPORT(INFO, "Impossible 1");
				
				target->suggestSlackSubaddSize(ll, wIn, (2*target->localWireDelay() + target->lutDelay() + xordelay + target->lutDelay()) );
				
				target->suggestSlackSubaddSize(l0, wIn, t - (2*target->lutDelay()+ xordelay) );
				
				REPORT(INFO, "l0="<<l0);
				
				
				int maxAdderSize = lkm1 + ll*(ll+1)/2 + l0;
				REPORT(INFO, "ll="<<ll);
				REPORT(INFO, "max adder size is="<< maxAdderSize);
				
				if (wIn>maxAdderSize) 
					REPORT(INFO, "cannot do proper chunk splitting");

#ifdef MAXSIZE
cout << " f="<<aa<<" s="<<maxAdderSize<<endl;
}
exit(1);
#endif					
				cSize = new int[100];

				int td = wIn;
				cSize[0] = l0;
				cSize[1] = 1;
				td -= (l0+1);
				nbOfChunks = 2;
				while (td > 0){
// 					cout <<"running";
					int nc = cSize[nbOfChunks-1] + 1;
					int nnc = lkm1;
					
					REPORT(INFO,"nc="<<nc);
					REPORT(INFO,"nnc="<<nnc);
					
					if (nc + nnc >= td){
						REPORT(INFO, "Finish");
						//we can finish it now;
						if (nc>=td)
							nc = td-1;
						cSize[nbOfChunks] = nc;
						nbOfChunks++;
						td-=nc;
						cSize[nbOfChunks] = td;
						nbOfChunks++;
						td=0;
					}else{
						REPORT(INFO, "run");
						//not possible to finish chunk splitting now
						cSize[nbOfChunks] = nc;
						nbOfChunks++;
						td-=nc;
					}
				}
				
				
				for (int i=0; i<nbOfChunks; i++)
					REPORT(INFO, "cSize["<<i<<"]="<<cSize[i]);
				
		
				//=================================================
				//split the inputs ( this should be reusable )
				vhdl << tab << "--split the inputs into chunks of bits depending on the frequency" << endl;
				for (int i=0;i<2;i++)
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
						vhdl << tab << declare (name.str(),cSize[j]+1) << " <=  \"0\" & X"<<i<<range(high-1,low)<<";"<<endl;
					}
				vhdl << tab << declare("scIn") << " <= Cin;"<<endl;
			
				int l=1;
				for (int j=0; j<nbOfChunks; j++){

#ifdef XILINX_OPTIMIZATION
					{ //code for adder instantiation to stop ise from "optimizing"
					int tp = target->isPipelined();
					target->setNotPipelined();
					IntAdder *adder = new IntAdder(target, cSize[j]+1);
					oplist.push_back(adder);
					if (tp) target->setPipelined();
				
					if (j>0){ //for all chunks greater than zero we perform this additions
						inPortMap(adder, "X", join("sX",j,"_0_l",l-1) );
						inPortMap(adder, "Y", join("sX",j,"_1_l",l-1) );
						inPortMapCst(adder, "Cin", "'0'");
						outPortMap(adder, "R", join("sX",j,"_0_l",l,"_Zero") );
						vhdl << instance(adder, join("adderZ",j) );

						inPortMapCst(adder, "Cin", "'1'");
						outPortMap(adder, "R", join("sX",j,"_0_l",l,"_One"));
						vhdl << instance( adder, join("adderO",j) );
						
					}else{
						vhdl << tab << "-- the carry resulting from the addition of the chunk + Cin is obtained directly" << endl;
						vhdl << tab << declare(join("sX",j,"_0_l",l,"_Cin"),cSize[j]+1) << " <= " << join("sX",j,"_0_l",l-1)<<" + "<<join("sX",j,"_1_l",l-1)<<" + scIn;"<<endl;
					}
				}
#else					
					if (j>0){ //for all chunks greater than zero we perform this additions
						vhdl<<tab<<declare(join("sX",j,"_0_l",l,"_Zero"),cSize[j]+1)<< " <= "<< join("sX",j,"_0_l",l-1)<< " + "<<join("sX",j,"_1_l",l-1)<<";"<<endl;
						vhdl<<tab<<declare(join("sX",j,"_0_l",l,"_One"),cSize[j]+1)<< "  <= "<< join("sX",j,"_0_l",l-1)<< " + "<<join("sX",j,"_1_l",l-1)<<" + '1';"<<endl;
					}else{
						vhdl<<tab<< "-- the carry resulting from the addition of the chunk + Cin is obtained directly" << endl;
						vhdl<<tab<<declare(join("sX",j,"_0_l",l,"_Cin"),cSize[j]+1)<< "  <= " << join("sX",j,"_0_l",l-1)<<" + "<<join("sX",j,"_1_l",l-1)<<" + scIn;"<<endl;
					}
#endif		
				}
			
				vhdl << tab <<"--form the two carry string"<<endl;
				vhdl << tab << declare("carryStringZero",nbOfChunks-2) << " <= "; 
				for (int i=nbOfChunks-3; i>=0; i--) {
					vhdl << "sX"<<i+1<<"_0_l"<<l<<"_Zero" << of(cSize[i+1]) << (i>0?" & ":";") ;
				} vhdl << endl;
	
				vhdl << tab << declare("carryStringOne",  nbOfChunks-2) << "  <= "; 
				for (int i=nbOfChunks-3; i>=0; i--) {
					vhdl << "sX"<<i+1<<"_0_l"<<l<<"_One" << " " <<of(cSize[i+1]) << (i>0?" & ":";");
				} vhdl << endl;

//				nextCycle();/////////////////////

				vhdl << tab << "--perform the short carry additions" << endl;
				ostringstream unameCin;
				unameCin  << "sX"<<0<<"_0_l"<<l<<"_Cin";
				vhdl << tab << declare("rawCarrySum",nbOfChunks-2) << " <= carryStringOne + carryStringZero + " << unameCin.str() <<of(cSize[0])<< ";" << endl;



//				vhdl << tab << declare("manipulatedSum",nbOfChunks-2) << "<= carryStringOne AND ( (not(rawCarrySum) AND not(carryStringZero)) OR carryStringZero);" << endl; //strike of genious
				vhdl << tab << declare("manipulatedSum",nbOfChunks-2) << "<= (not(rawCarrySum) AND carryStringOne) OR carryStringZero;" << endl; //strike of genious

//				if (invalid)
//					nextCycle();/////////////////////
				
				vhdl << tab <<"--get the final pipe results"<<endl;
				for ( int i=0; i<nbOfChunks; i++){
					ostringstream unameZero, unameOne, unameCin;
					unameZero << "sX"<<i<<"_0_l"<<l<<"_Zero";
					unameOne  << "sX"<<i<<"_0_l"<<l<<"_One";
					unameCin  << "sX"<<0<<"_0_l"<<l<<"_Cin";
					if (i==0) 
						vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << unameCin.str()<< range(cSize[0]-1,0) <<  ";" << endl;
					else {
						if (i==1) vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << unameZero.str() << range(cSize[i]-1,0) << " when " << unameCin.str()<<of(cSize[0])<<"='0' else "<<unameOne.str()<< range(cSize[i]-1,0)<<";"<<endl;
						else      vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << unameZero.str() << range(cSize[i]-1,0) << " when manipulatedSum"<<of(i-2)<<"='0' else "<<unameOne.str()<<range(cSize[i]-1,0)<<";"<<endl;
//						else      vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << unameZero.str() << range(cSize[i]-1,0) << " when ((not(rawCarrySum("<<i-2<<")) AND carryStringZero("<<i-2<<")) OR carryStringOne("<<i-2<<"))='0' else "<<unameOne.str()<<range(cSize[i]-1,0)<<";"<<endl;

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
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////
			} else if (version == 2){

				//THE new version that works well with VHDL (well, not that well finally )
				cSize = new int[2000];
				REPORT(3, "-- The new version: direct mapping without 0/1 padding, IntAdders instantiated");
				double	objectivePeriod = double(1) / target->frequency();
				REPORT(2, "Objective period is "<< objectivePeriod <<" at an objective frequency of "<<target->frequency());
				target->suggestSubaddSize(chunkSize_ ,wIn_);
				REPORT(2, "The chunkSize for first two chunks is: " << chunkSize_ );
			
				if (2*chunkSize_ >= wIn_){
					cerr << "ERROR FOR NOW -- instantiate int adder, dimmension too small for LongIntAdderAddAddMuxGen1" << endl;
					exit(0);
				}
			
				cSize[0] = chunkSize_;
				cSize[1] = chunkSize_;
			
				bool finished = false; /* detect when finished the first the first
					                   phase of the chunk selection algo */
				int width = wIn_ - 2*chunkSize_; /* remaining size to split into chunks */
				int propagationSize = 0; /* carry addition size */
				int chunkIndex = 2; /* the index of the chunk for which the size is
					                to be determined at the current step */
				bool invalid = false; /* the result of the first phase of the algo */
			
				/* FIRST PHASE */
				REPORT(3, "FIRST PHASE chunk splitting");
				while (not (finished))	 {
					REPORT(2, "The width is " << width);
					propagationSize+=1;
					double delay = objectivePeriod - target->adderDelay(width)- 2*target->localWireDelay()  - target->adderDelay(propagationSize); 
					REPORT(2, "The value of the delay at step " << chunkIndex << " is " << delay);
					if ((delay > 0) || (width < 4)) {
						REPORT(2, "finished -> found last chunk of size: " << width);
						cSize[chunkIndex] = width;
						finished = true;
					}else{
						REPORT(2, "Found regular chunk ");
						int cs; 
						double slack =  target->adderDelay(propagationSize);
						REPORT(2, "slack is: " << slack);
						REPORT(2, "adderDelay of " << propagationSize << " is " << target->adderDelay(propagationSize) );
						target->suggestSlackSubaddSize( cs, width, slack);
						REPORT(2, "size of the regular chunk is : " << cs);
						width = width - cs;
						cSize[chunkIndex] = cs;

						if ( (cSize[chunkIndex-1]==cSize[chunkIndex]) && (cSize[chunkIndex-1]==2) && ( invalid == false) ){
							REPORT(1, "[WARNING] Register level inserted after carry-propagation chain");
							invalid = true; /* invalidate the current splitting */
						}
						chunkIndex++; /* as this is not the last pair of chunks,
							          pass to the next pair */
				}
					}
				REPORT(2, "First phase return valid result: " << invalid);
			
				/* SECOND PHASE: 
				only if first phase is cannot return a valid chunk size
				decomposition */
				if (invalid){
					REPORT(2,"SECOND PHASE chunk splitting ...");
					target->suggestSubaddSize(chunkSize_ ,wIn_);
					lastChunkSize_ = (wIn_% chunkSize_ ==0 ? chunkSize_ :wIn_% chunkSize_);

					/* the index of the last chunk pair */
					chunkIndex = (wIn_% chunkSize_ ==0 ? ( wIn_ / chunkSize_) - 1 :  (wIn_-lastChunkSize_) / chunkSize_ ); 								
					for (int i=0; i < chunkIndex; i++)
						cSize[i] = chunkSize_;
					/* last chunk is handled separately  */
					cSize[chunkIndex] = lastChunkSize_;
				}

				/* VERIFICATION PHASE: check if decomposition is correct */		
				REPORT(2, "found " << chunkIndex + 1  << " chunks ");
				nbOfChunks = chunkIndex + 1; 
				int sum = 0;
				ostringstream chunks;
				for (int i=chunkIndex; i>=0; i--){
					chunks << cSize[i] << " ";
					sum+=cSize[i];
				}
				chunks << endl;
				REPORT(2, "Chunks are: " << chunks.str());
				REPORT(2, "The chunk size sum is " << sum << " and initial width was " << wIn_);
				if (sum != wIn_){
					cerr << "ERROR: check the algo" << endl; /*should never get here ... */
					exit(0);
				}
			
				/* input splitting and padding with one 0 for following operations*/
				for (int i=0;i<2;i++)
					for (int j=0; j<nbOfChunks; j++){
						/* sXj_i: j=chunk index, i=input index*/
						int low=0, high=0;
						for (int k=0;k<=j;k++)
							high+=cSize[k];
						for (int k=0;k<=j-1;k++)
							low+=cSize[k];
						vhdl << tab << declare (join("sX",j,"_",i),cSize[j]+1) << " <=  \"0\" & X"<<i<<range(high-1,low)<<";"<<endl;
					}
				vhdl << tab << declare("scIn") << " <= Cin;"<<endl;

				for (int j=0; j<nbOfChunks; j++){
					REPORT(3, "ITERATION " << j);
					ostringstream dnameZero, dnameOne, uname1, uname2, dnameCin;
					dnameZero << "sX"<<j<<"_Zero";
					dnameOne  << "sX"<<j<<"_One";
					dnameCin  << "sX"<<j<<"_Cin";
		
					uname1 << "sX"<<j<<"_0";
					uname2 << "sX"<<j<<"_1";


#ifdef XILINX_OPTIMIZATION
	// the xst synthetsies x+y and x+y+1 slower if this optimization is not used				
					bool pipe = target->isPipelined();
					target->setNotPipelined();

					IntAdder *adder = new IntAdder(target, cSize[j]+1);
					ostringstream a;
					a.str("");

					vhdl << tab << declare( uname1.str()+"ext", cSize[j]+1 ) << " <= \"0\" & "  <<  use(uname1.str()) << range(cSize[j]-1,0) << ";" << endl;
					vhdl << tab << declare( uname2.str()+"ext", cSize[j]+1 ) << " <= \"0\" & "  <<  use(uname2.str()) << range(cSize[j]-1,0) << ";" << endl;
			
					if (j>0){ //for all chunks greater than zero we perform this additions
							bool found = false;
							for(unsigned k=0; k<oplist.size(); k++) {
								if  ( (oplist[k]->getName()).compare(adder->getName()) ==0 ){
									REPORT(3, "found in opList ... not adding");
									found = true;
								}
							}
						if (found == false)						{
							REPORT(3, "Not found in list, adding " << adder->getName());
							oplist.push_back(adder);
						}
				
						inPortMapCst(adder, "X", uname1.str()+"ext" );
						inPortMapCst(adder, "Y", uname2.str()+"ext" );
						inPortMapCst(adder, "Cin", "'0'");
						outPortMap(adder, "R", dnameZero.str());
						a<< "Adder" << cSize[j]+1 << "Zero" << j;
						vhdl << instance( adder, a.str() );
				
						if (j<nbOfChunks-1){
					
							inPortMapCst(adder, "X", uname1.str()+"ext" );
							inPortMapCst(adder, "Y", uname2.str()+"ext" );
							inPortMapCst(adder, "Cin", "'1'");
							outPortMap(adder, "R", dnameOne.str());
							a.str("");
							a<< "Adder" << cSize[j]+1 << "One" << j;
							vhdl << instance( adder, a.str() );
						}
					if (pipe)
						target->setPipelined();
					else
						target->setNotPipelined();
					}else{
						vhdl << tab << "-- the carry resulting from the addition of the chunk + Cin is obtained directly" << endl;
						vhdl << tab << declare(dnameCin.str(),cSize[j]+1) << "  <= (\"0\" & "<< use(uname1.str())<<range(cSize[j]-1,0) <<") +  (\"0\" & "<< use(uname2.str())<<range(cSize[j]-1,0)<<") + scIn;"<<endl;
					}
#else
/* this is the same, but no combinatorial IntAdders are defined for the additions*/		
					if (j>0){ //for all chunks greater than zero we perform this additions
						vhdl << tab << declare(dnameZero.str(),cSize[j]+1) << " <= (\"0\" & "<< use(uname1.str())<<range(cSize[j]-1,0) <<") +  (\"0\" & "<< use(uname2.str())<<range(cSize[j]-1,0)<<");"<<endl;
						if (j<nbOfChunks-1)
						vhdl << tab << declare(dnameOne.str(),cSize[j]+1) << "  <= (\"0\" & "<< use(uname1.str())<<range(cSize[j]-1,0) <<") +  (\"0\" & "<< use(uname2.str())<<range(cSize[j]-1,0)<<") + '1';"<<endl;
					}else{
						vhdl << tab << "-- the carry resulting from the addition of the chunk + Cin is obtained directly" << endl;
						vhdl << tab << declare(dnameCin.str(),cSize[j]+1) << "  <= (\"0\" & "<< use(uname1.str())<<range(cSize[j]-1,0) <<") +  (\"0\" & "<< use(uname2.str())<<range(cSize[j]-1,0)<<") + scIn;"<<endl;
					}

#endif
				}

				vhdl << tab << declare("carryStringZero",nbOfChunks-2,true) << " <= "; 
				for (int i=nbOfChunks-2; i>=1; i--) {
					vhdl << " " << use(join("sX",i,"_Zero")) << range(cSize[i],cSize[i]);
					if (i>1) vhdl << " & ";
					else     vhdl << " ; ";
				} vhdl << endl;
				REPORT(3, "Created ZERO carry string");
				
				vhdl << tab << declare("carryStringOne",nbOfChunks-2,true) << " <= "; 
				for (int i=nbOfChunks-2; i>=1; i--) {
					vhdl << " " << use(join("sX",i,"_One")) << range(cSize[i],cSize[i]);
					if (i>1) vhdl << " & ";
					else     vhdl << " ; ";
				} vhdl << endl;
				REPORT(3, "Created ONE carry string");

				nextCycle(); //////////////////////////////////////////////////////

				for (int i=1; i<=nbOfChunks-2; i++){
					vhdl << tab << declare(join("s",i-1),2) << " <=  (\"0\" & carryStringZero" << range(i-1,i-1) << ") + " 
						  << " (\"0\" & carryStringOne" << range(i-1,i-1) << ") + ";
					if (i==1)
						vhdl << "sX0_Cin" << of(cSize[0])<<";"<<endl;
					else
						vhdl << use(join("s",i-2))<<"("<<1<<");"<<endl;
				}
				REPORT(3, "Performed SUMs");
		
				vhdl << tab << declare("carrySum",nbOfChunks-2,true) << " <= ";
				for (int i=nbOfChunks-3;i>=0;i--){
					vhdl << use(join("s",i))<< of(1) << (i==0?";":" & ");
				} vhdl << endl;

				if (invalid)
					nextCycle(); //////////////////////////////////////////////////////
	
					//get the final pipe results;
					for ( int i=1; i< nbOfChunks; i++){
						if (i==1) 
							vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << use(join("sX",i,"_Zero")) << range(cSize[i]-1,0) << " + " 
									 << "sX0_Cin" << of(cSize[0]) << ";" << endl;
						else 
							vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << use(join("sX",i,"_Zero")) << range(cSize[i]-1,0) << " + " 
									 << "carrySum" << of(i-2)<<";"<<endl;
					}
	
					vhdl << tab << "R <= ";
					for (int i=nbOfChunks-1; i>=1; i--){
						vhdl << use(join("res",i));
						if (i > 0) vhdl << " & ";
					}
			
					vhdl << "sX0_Cin" << range(cSize[0]-1,0)<< ";" <<endl;
			}
		}else
			vhdl << tab << " R <= X0 + X1 + Cin;"<<endl;
	}

	LongIntAdderAddAddMuxGen1::~LongIntAdderAddAddMuxGen1() {
	}


	void LongIntAdderAddAddMuxGen1::emulate(TestCase* tc)
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
