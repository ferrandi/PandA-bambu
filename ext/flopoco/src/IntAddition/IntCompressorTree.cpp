/*
  An integer compressor tree for FloPoCo
  
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
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"
#include "../IntAdder.hpp"
#include "IntCompressorTree.hpp"

using namespace std;


namespace flopoco{


	IntCompressorTree::IntCompressorTree(Target* target, int wIn, int N, map<string, double> inputDelays):
		IntMultiAdder(target, wIn, N, inputDelays, false, true), wIn_(wIn), N_(N), inputDelays_(inputDelays) 
	{
		ostringstream name;
		name << "IntCompressorTree_" << wIn_<<"_"<<N_<<"_uid"<<Operator::getNewUId();
		setName(name.str());
		srcFileName = "IntCompressorTree";
		setCopyrightString("Bogdan Pasca (2009-2011)");

		// Set up the IO signals
		for (int i=0; i<N; i++){
			name.str(""); //init a ostringstream variable
			name << "X"<<i; 
			addInput (name.str() , wIn_, true);
		}

		addOutput("R"  , wIn_, 1, true);
		REPORT(DEBUG, "delay for X0 is   "<< inputDelays["X0"]);	
		
		setCriticalPath(getMaxInputDelays(inputDelays));

		int lutSize = target->lutInputs();
		bool processing = true;
		int nbOfInputs = N_;
		int tLev = 1;

		//for homogeneous signal names
		for (int j=0; j<nbOfInputs;j++){
			vhdl << tab << declare( join("l_",tLev-1,"_s_",j), wIn_, true) << " <= " << join("X",j) << ";" << endl;
		}
	
		while (processing){
			if ((nbOfInputs == 1) || (wIn == 1)){
				manageCriticalPath(target->lutDelay());
				vhdl << tab << "R <= ";
				for (int i=N-1; i>=0; i--){
					if (i>0)
						vhdl << "X"<<i<< " + ";
					else
						vhdl << "X"<<i<< ";"<<endl;
				} 
				processing = false;
				outDelayMap["R"] = getCriticalPath();
			}else if (nbOfInputs == 2){
				IntAdder *finalAdder = new IntAdder(target, wIn_, inDelayMap("X", target->localWireDelay()+ getCriticalPath() + (getCriticalPath()==0? target->ffDelay():0 )));
				oplist.push_back(finalAdder);
				REPORT(INFO, "Finished 2 input adder instantiation");	

				inPortMap(finalAdder,"X",join("l_",tLev-1,"_s_",0));
				inPortMap(finalAdder,"Y",join("l_",tLev-1,"_s_",1));
				inPortMapCst(finalAdder,"Cin","'0'");
				outPortMap(finalAdder,"R","myR");
				vhdl << instance(finalAdder,"FinalAdder_CompressorTree") << endl;
				REPORT(DEBUG, "Instantiation Succeed");
				syncCycleFromSignal("myR");
			
				vhdl << tab << "R <= myR;" << endl;
				outDelayMap["R"] = finalAdder->getOutputDelay("R");
				vhdl << " -- delay at adder output " <<  outDelayMap["R"] << endl;
				processing = false;
				REPORT(DEBUG, "Should have finished ...");
			}else{
				int a[16];    //possibly in the future LUTS may have up to 16 inputs
				int sol[16];
				int bestSol[16];
	
				for (int i=0;i <= lutSize; i++){
					sol[i]=0;
					bestSol[i]=0;
				}
				bestSol[0]=999;
	
				for (int i=lutSize; i>=1 ; i--){
					a[i]= intlog2(double(i));
				} 
				a[0]=0;
	
				bt(1, lutSize, nbOfInputs, sol, a, nbOfInputs, bestSol);
	
				REPORT(DEBUG, "BACKTRACKING FINISHED");
				REPORT(DEBUG, " Solution = ");
				
				ostringstream tmp;
				for (int i=1; i <=lutSize; i++)
					tmp << bestSol[i] << ", ";
				tmp << " having score " << bestSol[0];
				REPORT(DEBUG, tmp.str());
				
				manageCriticalPath( target->lutDelay() + target->localWireDelay( target->lutInputs() ));
				REPORT(DEBUG, "Delay in compressor tree @ some level " << getCriticalPath());
			
				int cMap = 0;
				int cOutp = 0;
				int cComp = 0;
				for (int i=lutSize; i>=1; i--){
					REPORT(DEBUG, "mapping compressors " << i << " to " << intlog2(i));
					int sumSize = intlog2(i);			
					for (int h=1; h<=bestSol[i]; h++){
						REPORT(DEBUG, tab << "number " << h);
						if (i>2){
							for (int j=0; j < wIn_; j++){ //do the compressor computation
								vhdl << tab << declare( join("sell_",tLev,"_c_",cComp,"_cl_",j) , i, true) << " <= ";
								for (int k=cMap; k<cMap+i; k++) {
									vhdl << join("l_",tLev-1,"_s_",k) << of(j);
									if (k < cMap+i-1)
										vhdl << " & ";
								}vhdl << ";" << endl;
								vhdl << tab << " with "<<join("sell_",tLev,"_c_",cComp,"_cl_",j)<<" select" << endl;
								vhdl << tab << declare( join("l_",tLev,"_c_",cComp,"_cl_",j) , sumSize, true) << " <= "<<endl;
								for (int ii=0;ii<(1<<i); ii++)
									if (ii==0)
										vhdl << tab << "\""<<unsignedBinary(0,sumSize)<<"\" when \""<< unsignedBinary(0,i)<<"\",";
									else
										vhdl << tab << "\""<<unsignedBinary(coutOnes(ii),sumSize)<<"\" when \""<< unsignedBinary(ii,i)<<"\",";
								vhdl << tab << "\""<<unsignedBinary(0,sumSize)<<"\" when others;"<<endl;
							}
							cComp++;
						}
						if (i == 2){
							vhdl << tab << declare(join("l_",tLev,"_c_",cComp,"_cl_",0), wIn_, true) << " <= " << join("l_",tLev-1,"_s_",cMap)<<";"<<endl;
							vhdl << tab << declare(join("l_",tLev,"_c_",cComp,"_cl_",1), wIn_, true) << " <= " << join("l_",tLev-1,"_s_",cMap+1)<<";"<<endl;
							cComp++;
						}
						if (i == 1){
							vhdl << tab << declare(join("l_",tLev,"_c_",cComp,"_cl_",0), wIn_, true) << " <= " << join("l_",tLev-1,"_s_",cMap)<<";"<<endl;
							cComp++;
						}
						//form the summs for the current compressor
						if (i>2){
							for (int m=0; m < sumSize; m++){
								vhdl << tab << declare (join("l_",tLev,"_s_",cOutp + m), wIn_, true) << " <= ";
								for (int k=wIn_-1-m; k >= 0; k--)	{							
									vhdl << join("l_",tLev,"_c_",cComp-1,"_cl_",k)<<range(m,m) << (k!=0?" & ":"");
								}
								if (m>0)
									vhdl << " & " << zg(m,0);
								vhdl << ";" << endl;
							}
						}else{
							if (i==2){
								vhdl << tab << declare (join("l_",tLev,"_s_",cOutp  ), wIn_, true) << " <= " << join("l_",tLev,"_c_",cComp-1,"_cl_",0)<<";"<<endl;
								vhdl << tab << declare (join("l_",tLev,"_s_",cOutp+1), wIn_, true) << " <= " << join("l_",tLev,"_c_",cComp-1,"_cl_",1)<<";"<<endl;
							}else if (i==1){
								vhdl << tab << declare (join("l_",tLev,"_s_",cOutp  ), wIn_, true) << " <= " << join("l_",tLev,"_c_",cComp-1,"_cl_",0)<<";"<<endl;
							}
						}
						cMap += i;
						cOutp += intlog2(i);
					}
				}
				//form the sums
			
				nbOfInputs = bestSol[0];
				tLev++;
			}
			REPORT(DEBUG, "Finished IntCompressor");
		}
	}		


	bool IntCompressorTree::solution(int k, int n, int targetSum, int * sol, int * coef){
		int val=0;
		for (int i=1; i<=n; i++)
			val+=sol[i]*i;
		if ((k==n+1) && ( val==targetSum))
			return true;
		else
			return false;
	}

	void IntCompressorTree::printSolution(int n, int * sol, int * coef, int *bestSol){
		ostringstream tmp;			
			tmp <<  "solution !!! :"; 
			for (int i=1; i<=n; i++)
				tmp << sol[i] << ", ";

		REPORT(DEBUG, tmp.str());
		int val=0;
		for (int i=1; i<=n; i++)
			val+=sol[i]*coef[i];
		
		
		tmp.str("");
		tmp << "score = " << val << " coeficinets=";
	
		if (val < bestSol[0]){
			for (int i=1; i<=n; i++)
				bestSol[i] = sol[i];
			bestSol[0]=val;
		}
	
		if (verbose)
			for (int i=1; i<=n ; i ++)
				tmp << coef[i] << "::";	

		REPORT(DEBUG, tmp.str());

	}

	bool IntCompressorTree::successor( int k, int sum, int * sol){
		if (sol[k] < sum) {
			sol[k]++;
			return true;
		}else
			return false;
	}

	bool IntCompressorTree::valid(int k, int sum, int n, int * sol, int * coef){
		int val = 0;
		for (int i=1; i<= (k>n?n:k); i++)  
			val+=sol[i]*coef[i];
	
		if ((val <= sum ) && (k<=n+1))
			return true;
		else
			return false;
	}

	void IntCompressorTree::bt(int k, int n, int sum, int* sol, int* coef, int targetSum, int * bestSol){
	
		if (solution(k, n, targetSum, sol, coef))
			printSolution(n, sol, coef, bestSol);
		else{
			sol[k]=-1;
			while (successor(k, sum, sol)) { //TODO optimize 
				if (valid(k,sum, n, sol, coef)){ 
					bt(k+1, n, sum, sol, coef, targetSum, bestSol);
				}
			}
		}
	}

	IntCompressorTree::~IntCompressorTree() {
	}


//	void IntCompressorTree::emulate(TestCase* tc)
//	{
//		mpz_class svX;
//		mpz_class svR = 0;

//		for (int i=0; i<N_; i++){
//			ostringstream iName;
//			iName << "X"<<i;
//			svX = tc->getInputValue(iName.str());
//			svR = svR + svX;
//			mpz_clrbit(svR.get_mpz_t(),wIn_); 
//		}
//		tc->addExpectedOutput("R", svR);

//	}

}
