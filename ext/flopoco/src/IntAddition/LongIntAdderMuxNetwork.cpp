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
#include "LongIntAdderMuxNetwork.hpp"


using namespace std;
namespace flopoco{

	LongIntAdderMuxNetwork::LongIntAdderMuxNetwork(Target* target, int wIn, map<string, double> inputDelays, int regular):
		Operator(target), wIn_(wIn), inputDelays_(inputDelays) 
	{
		srcFileName="LongIntAdderMuxNetwork";
		setName(join("LongIntAdderMuxNetwork_", wIn_));

		// Set up the IO signals
		for (int i=0; i<2; i++)
			addInput ( join("X",i) , wIn_, true);
		addInput("Cin");
		addOutput("R"  , wIn_, true, 1);

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
						cerr << "ERROR FOR NOW -- instantiate int adder, dimmension too small for LongIntAdderMuxNetwork" << endl;
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
				// double xordelay;
				// double dcarry;
				// double muxcystoo;
				// double fdcq;
				double muxcystooOut;

				int fanOutWeight;

				if (target->getID()=="Virtex5"){
					// fdcq = 0.396e-9; 
					// xordelay = 0.300e-9;
					// dcarry = 0.023e-9;
					// muxcystoo = 0.305e-9;
					muxcystooOut = 0.504e-9; 
					fanOutWeight = 45;					
				}else{ 
					if (target->getID()=="Virtex6"){
						// fdcq = 0.280e-9;
						// xordelay = 0.180e-9;
						// dcarry = 0.015e-9;
						// muxcystoo =	0.219e-9;
						muxcystooOut = 0.373e-9;
						fanOutWeight = 51;
					}else{ 
						if (target->getID()=="Virtex4"){
							// fdcq = 0.272e-9;
							// xordelay = 0.273e-9;
							// dcarry = 0.034e-9;
							// muxcystoo = 0.278e-9;
							muxcystooOut = 0.524e-9;
							fanOutWeight = 60;
						}
					}
				}
				int lkm1;
				

	double iDelay = getMaxInputDelays(inputDelays);
				
#ifdef MAXSIZE
	for (int aa=25; aa<=500; aa+=5){
		target->setFrequency(double(aa)*1000000.0);

#endif
bool nogo = false;
double t = 1.0 / target->frequency();

				if (!target->suggestSlackSubaddSize(lkm1, wIn, iDelay /*fdcq + target->localWireDelay()*/ + target->localWireDelay() + target->lutDelay())){
//					cerr << "Impossible 1" << endl;
					nogo = true;
				}
//				cout << "lkm1 = " << lkm1 << endl;
				
				double z =				iDelay +
										/*fdcq + target->localWireDelay() +*/
										target->lutDelay() + //xordelay +
										muxcystooOut + // the select to output line of the carry chain multiplexer. 
													// usually this delay for the 1-bit addition which is not overlapping   
										target->localWireDelay() + 
										target->localWireDelay(fanOutWeight) + //final multiplexer delay. Fan-out of the CGC bits is accounted for
										target->lutDelay();
#ifdef DEBUGN
				cerr << "lut             delay = " << target->lutDelay() << endl;
				cerr << "muxcystooOut    delay = " << muxcystooOut << endl;
				cerr << "localWireDelay  delay = " << target->localWireDelay() << endl;
				cerr << "localWireDelay2 delay = " << target->localWireDelay(fanOutWeight) << endl;
				cerr << "z slack = " << z << endl;
#endif
				nogo = nogo | (!target->suggestSlackSubaddSize(ll, wIn, z));
#ifdef DEBUGN
				cerr << "ll is = "<<ll << endl;
#endif				
				/*nogo = nogo | (!*/target->suggestSlackSubaddSize(l0, wIn, t - (2*target->lutDelay()+ muxcystooOut/* xordelay*/)); //);
				
				REPORT(INFO, "l0="<<l0);
				
				
				int maxAdderSize = lkm1 + ll*(ll+1)/2 + l0;
				if (nogo) 
					maxAdderSize = -1;
				REPORT(INFO, "ll="<<ll);
				REPORT(INFO, "max adder size is="<< maxAdderSize);
				

#ifdef MAXSIZE
cout << " f="<<aa<<" s="<<maxAdderSize<<endl;
}
exit(1);
#endif					
				cSize = new int[100];
				
				if (regular>0) {
					int c = regular;
					cout << "c="<<c<<endl; 
					int s = wIn_;
					int j=0;
					while (s>0){
						if (s-c>0){
							cSize[j]=c;
							s-=c;
						}else{
							cSize[j]=s;
							s=0;	
						}
						j++;	
					}
					nbOfChunks = j;	
				}else{
					int td = wIn;
					cSize[0] = l0;
					cSize[1] = 1;
					td -= (l0+1);
					nbOfChunks = 2;
					while (td > 0){
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
				}	
				
				for (int i=0; i<nbOfChunks; i++)
					REPORT(INFO, "cSize["<<i<<"]="<<cSize[i]);
				
//#define test512
#ifdef test512				
				nbOfChunks = 16;
				for (int i=1;i<=16;i++)
					cSize[i-1]=32;
		
#endif		
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
						vhdl << tab << declare (name.str(),cSize[j],true) << " <=  X"<<i<<range(high-1,low)<<";"<<endl;
					}
			
				int l=1;
				for (int j=0; j<nbOfChunks; j++){
					//code for adder instantiation to stop ise from "optimizing"
					IntAdderSpecific *adder = new IntAdderSpecific(target, cSize[j]);
					oplist.push_back(adder);

					if (j>0){ //for all chunks greater than zero we perform this additions
						inPortMap(adder, "X", join("sX",j,"_0_l",l-1) );
						inPortMap(adder, "Y", join("sX",j,"_1_l",l-1) );
						inPortMapCst(adder, "Cin", "'0'");
						outPortMap(adder, "R",    join("sX",j,"_0_l",l,"_Zero") );
						outPortMap(adder, "Cout", join("coutX",j,"_0_l",l,"_Zero") );
						vhdl << instance(adder, join("adderZ",j) );

						inPortMapCst(adder, "Cin", "'1'");
						outPortMap(adder, "R", join("sX",j,"_0_l",l,"_One"));
						outPortMap(adder, "Cout", join("coutX",j,"_0_l",l,"_One"));
						vhdl << instance( adder, join("adderO",j) );
					}else{
						vhdl << tab << "-- the carry resulting from the addition of the chunk + Cin is obtained directly" << endl;
						inPortMap(adder, "X", join("sX",j,"_0_l",l-1) );
						inPortMap(adder, "Y", join("sX",j,"_1_l",l-1) );
						inPortMapCst(adder, "Cin", "Cin");
						outPortMap(adder, "R",    join("sX",j,"_0_l",l,"_Cin") );
						outPortMap(adder, "Cout", join("coutX",j,"_0_l",l,"_Cin") );
						vhdl << instance(adder, join("adderCin",j) );
					}
				}
			
				vhdl << tab <<"--form the two carry string"<<endl;
				vhdl << tab << declare("carryStringZero",nbOfChunks-2) << " <= "; 
				for (int i=nbOfChunks-3; i>=0; i--) {
					vhdl << "coutX"<<i+1<<"_0_l"<<l<<"_Zero"<< (i>0?" & ":";") ;
				} vhdl << endl;
	
				vhdl << tab << declare("carryStringOne",  nbOfChunks-2) << "  <= "; 
				for (int i=nbOfChunks-3; i>=0; i--) {
					vhdl << "coutX"<<i+1<<"_0_l"<<l<<"_One" << " " << (i>0?" & ":";");
				} vhdl << endl;

				//multiplexer network
				for (int i=0; i<=nbOfChunks-3; i++){
					if (i==0)	
						vhdl << tab << declare( join("c",i+1) ) << " <= carryStringOne"<<of(i)<<" when Cin='1' else carryStringZero"<<of(i)<<";"<<endl; 
					else
						vhdl << tab << declare( join("c",i+1) ) << " <= carryStringOne"<<of(i)<<" when "<<join("c",i)<<"='1' else carryStringZero"<<of(i)<<";"<<endl;
				}
				
				for (int i=0; i< nbOfChunks; i++){
					if (i==0)
						vhdl << tab << declare( join("res",i), cSize[i],true) << " <= " << join("sX",i,"_0_l",1,"_Cin") << ";" << endl;
					else if (i==1)
						vhdl << tab << declare( join("res",i), cSize[i],true) << " <= " << join("sX",i,"_0_l",1,"_Zero") << " when "<<join("coutX",0,"_0_l",1,"_Cin")<<"='0' else "<< join("sX",i,"_0_l",1,"_One") << ";" << endl;
					else
						vhdl << tab << declare( join("res",i), cSize[i],true) << " <= " << join("sX",i,"_0_l",1,"_Zero") << " when "<<join("c",i-1)<<"='0' else "<< join("sX",i,"_0_l",1,"_One") << ";" << endl;	
				}
				

//				if (target->getVendor()== "Xilinx"){
//					//////////////////////////////////////////////////////
//					vhdl << tab << "--perform the short carry additions" << endl;
//					CarryGenerationCircuit *cgc = new CarryGenerationCircuit(target,nbOfChunks-2);
//					oplist.push_back(cgc);
//				
//					inPortMap(cgc, "X", "carryStringZero" );
//					inPortMap(cgc, "Y", "carryStringOne" );
//					inPortMapCst(cgc, "Cin", join("coutX",0,"_0_l",1,"_Cin"));
//					outPortMap(cgc, "R",    "rawCarrySum" );
//					vhdl << instance(cgc, "cgc");

//				
//					vhdl << tab <<"--get the final pipe results"<<endl;
//					for ( int i=0; i<nbOfChunks; i++){
//						if (i==0) 
//							vhdl << tab << declare(join("res",i),cSize[i],true) << " <= sX0_0_l1_Cin;" << endl;
//						else {
//							if (i==1) vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << join("sX",i,"_0_l",l,"_Zero") << " when " << join("coutX",0,"_0_l",l,"_Cin")<<"='0' else "<<join("sX",i,"_0_l",l,"_One")<<";"<<endl;
//							else      vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << join("sX",i,"_0_l",l,"_Zero") << " when rawCarrySum"<<of(i-2)<<"='0' else "<<join("sX",i,"_0_l",l,"_One")<<";"<<endl;
//						}
//					}
//			
//				}else{ //Altera /////////////////////////////////////////////////////////////////////
//					vhdl << tab << "--perform the short carry additions" << endl;
//					IntAdderSpecific *cgc = new IntAdderSpecific(target,nbOfChunks-2);
//					oplist.push_back(cgc);
//				
//					inPortMap(cgc, "X", "carryStringZero" );
//					inPortMap(cgc, "Y", "carryStringOne" );
//					inPortMapCst(cgc, "Cin", join("coutX",0,"_0_l",1,"_Cin"));
//					outPortMap(cgc, "R",    "rawCarrySum" );
//					outPortMap(cgc, "Cout", "cgcCout");
//					vhdl << instance(cgc, "cgc");

//					vhdl << tab <<"--get the final pipe results"<<endl;
//					for ( int i=0; i<nbOfChunks; i++){
//						if (i==0) 
//							vhdl << tab << declare(join("res",i),cSize[i],true) << " <= sX0_0_l1_Cin;" << endl;
//						else {
//							if (i==1) vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << join("sX",i,"_0_l",l,"_Zero") << " when " << join("coutX",0,"_0_l",l,"_Cin")<<"='0' else "<<join("sX",i,"_0_l",l,"_One")<<";"<<endl;
//							else      vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << join("sX",i,"_0_l",l,"_One") << " when  ((not(rawCarrySum"<<of(i-2)<<") and carryStringOne"<<of(i-2)<<") or carryStringZero"<<of(i-2)<<")='1' else "<<join("sX",i,"_0_l",l,"_Zero")<<";"<<endl;
//						}
//					}
//				}
				
				vhdl << tab << "R <= ";
				int k=0;
				for (int i=nbOfChunks-1; i>=0; i--){
					vhdl << join("res",i);
					if (i > 0) vhdl << " & ";
					k++;
				}
				vhdl << ";" <<endl;


			///////////////////////////////////////////////////////////////////////////////////////////////////////////////
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////
			
	}

	LongIntAdderMuxNetwork::~LongIntAdderMuxNetwork() {
	}


	void LongIntAdderMuxNetwork::emulate(TestCase* tc)
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
