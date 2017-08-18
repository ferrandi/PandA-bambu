/*
  A multiplier by an integer constant for FloPoCo

  This file is part of the FloPoCo project developed by the Arenaire
  team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.

*/

#include <iostream>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"

#include "ShiftAddOp.hpp"
#include "ShiftAddDag.hpp"

#include "IntConstMult.hpp"
//#include "rigo.h"

using namespace std;

namespace flopoco{

	void reset_visited(ShiftAddOp* sao) {
		if (sao!=NULL) {
			sao->already_visited=false;
			switch(sao->op) {
			case X:
				break;
			case Add:
			case Sub:
			case RSub:
				reset_visited(sao->i);
				reset_visited(sao->j);
				break;
			case Shift:
			case Neg:
				reset_visited(sao->i);
				break;
			}
		}
	}





	// No longer used

	int compute_tree_depth(ShiftAddOp* sao) {
		int ipd, jpd;
		if (sao==NULL)
			return 0;
		else {
			switch(sao->op) {
			case X:
				return 0;
			case Add:
			case Sub:
			case RSub:
				ipd = compute_tree_depth(sao->i);
				jpd = compute_tree_depth(sao->j);
				return 1+max(ipd, jpd);
			case Shift:
				ipd = compute_tree_depth(sao->i);
				return ipd;
			case Neg:
				ipd = compute_tree_depth(sao->i);
				return 1+ipd;
			}
			return 0;
		}
	}





	// Do not forget to call reset_visited before calling this one.
	int compute_total_cost_rec(ShiftAddOp* sao) {
		if (sao==NULL || sao->already_visited)
			return 0;
		else {
			sao->already_visited=true;
			switch(sao->op) {
			case X:
				return 0;
			case Add:
			case Sub:
			case RSub:
				return sao->cost_in_full_adders + compute_total_cost_rec(sao->i) + compute_total_cost_rec(sao->j);
			case Shift:
			case Neg:
				return sao->cost_in_full_adders + compute_total_cost_rec(sao->i);
			}
		}
		throw string("This exception in IntConstMult::compute_total_cost_rec should never happen");
	}





	int compute_total_cost(ShiftAddOp* sao) {
		reset_visited(sao);
		return compute_total_cost_rec(sao);
	}



	/**
	 * Depth-first traversal of the DAG to build the pipeline.
	 * @param partial_delay accumulates the delays of several stages

	 Starting from the leaves, we accumulate partial delays until target_period is reached.
	 Then pipeline level will be inserted.
 
	*/

	void IntConstMult::build_pipeline(ShiftAddOp* sao, double &partial_delay) {
		string iname, jname, isignal, jsignal;
		double idelay=0,jdelay=0, max_children_delay;
		int size, isize, jsize, shift, adder_size; 
		bool use_pipelined_adder;
		IntAdder* adder=0;
	
		if (sao==NULL)
			return;
		else {

			// First check that the sub-DAG has not been already visited

			bool already_visited=true;
			try { 
				getSignalByName(sao->name);
			} catch (std::string s) {
				already_visited=false;
			}
			if(already_visited)
				return;

			// A few variables to make the code below easier to read
			ShiftAddOpType op = sao->op;
			size = sao->size;
			shift = sao->s;
			
			switch(op) {
			case X:
				partial_delay=0;
				setCycle(0, false);
				return;

			case Add:
			case Sub:
			case RSub:
				// A few variables to make the code below easier to read
				isize = sao->i->size;
				jsize = sao->j->size;

				build_pipeline(sao->i, idelay);
				if(sao->i != sao->j) {
					build_pipeline(sao->j, jdelay);
				}
				iname = sao->i->name; 
				jname = sao->j->name; 

				adder_size = sao->cost_in_full_adders+1;
				vhdl << endl << tab << "-- " << *sao <<endl; // comment what we're doing
				setCycleFromSignal(iname, false);
				syncCycleFromSignal(jname, false);

				max_children_delay = max(idelay,jdelay);

				// Now decide what kind of adder we will use, and compute the remaining delay
				use_pipelined_adder=false;
				if (isSequential()) {
					// First case: using a plain adder fits within the current pipeline level
					double tentative_delay = max_children_delay + target_->adderDelay(adder_size) + target_->localWireDelay();
					if(tentative_delay <= 1./target_->frequency()) {
						use_pipelined_adder=false;
						partial_delay = tentative_delay;					
					}
					else { 
						// register the children 
						nextCycle();
						// Is a standard adder OK ?
						tentative_delay = target_->ffDelay() + target_->localWireDelay() + target_->adderDelay(adder_size);
						if(tentative_delay <= 1./target_->frequency()) {
							use_pipelined_adder=false;
							partial_delay = tentative_delay;					
						}
						else { // Need to instantiate an IntAdder
							use_pipelined_adder=true;
							adder = new IntAdder(target_, adder_size);
							adder->changeName(getName() + "_" + sao->name + "_adder");
							oplist.push_back(adder);

							partial_delay =  (adder->getOutDelayMap())["R"]; //  target_->adderDelay(adder->getLastChunkSize());
						}
					}
				}
				// Now generate VHDL

				if(shift==0) { // Add with no shift -- this shouldn't happen with current DAGs so te following code is mostly untested
					if(op==Sub || op==RSub)
						throw string("In IntConstMult::build_pipeline, Sub and RSub with zero shift currently unimplemented"); // TODO
					isignal = sao->name + "_High_L";  
					jsignal = sao->name + "_High_R"; 

					// The i part
					vhdl << tab << declare(isignal, size) << " <= ";
					if(size>isize+1) // need to sign-extend x
						vhdl <<"(" << size-1 << " downto " << isize <<" => '" << (sao->i->n >= 0 ? "0" : "1" ) << "') & ";
					vhdl << iname << ";" << endl;
					// the y part
					vhdl << tab << declare(jsignal, size) << " <= ";
					if(size>jsize) // need to sign-extend y
						vhdl << "(" << size-1 <<" downto " << jsize <<" => '" << (sao->j->n >= 0 ? "0" : "1" ) << "') & ";
					vhdl << jname << ";" << endl;

					if(use_pipelined_adder) { // Need to use an IntAdder subcomponent
						inPortMap  (adder, "X", isignal);
						inPortMap  (adder, "Y", jsignal);
						inPortMapCst  (adder, "Cin", "'0'");
						outPortMap (adder, "R",sao->name);
						vhdl << instance(adder, sao->name + "_adder");
					}
					else
						vhdl << tab << declare(sao->name, size) << " <= " << isignal << " + " << jsignal << ";" << endl;
				}


				else { // Add with actual shift
					if(op == Add || op==RSub) {
						if(shift >= jsize) { // Simpler case when the two words to add are disjoint; size=isize+s+1
							//                        jjjjjj
							//             +/-  iiii
							// TODO perf: use an IntAdder here when needed
							// The lower bits of the sum are those of y, possibly sign-extended but otherwise untouched
							vhdl << tab << declare(sao->name, sao->size) << "("<< shift - 1 <<" downto 0) <= " ;
							if(shift>jsize) {
								vhdl << "(" <<  shift-1 <<" downto " << jsize << " => ";
								if(sao->j->n >= 0)  vhdl << "'0'"; // pad with 0s 
								else                vhdl << jname << "(" << jsize-1 << ")";// sign extend
								vhdl << ") & ";
							}
							vhdl << jname << ";   -- lower bits untouched"<<endl;

							if(op == Add) {
								// The higher bits (size-1 downto s) of the result are those of x, possibly plus 11...1 if y was negative
								vhdl << tab << sao->name << "("<<sao->size-1<<" downto "<< shift<<") <= " << iname ;
								if(sao->j->n < 0) { 
									vhdl <<" + (" << sao->size-1 <<" downto " <<  shift <<" => " << jname << "(" << jsize-1 << ")) "
										  << ";   -- sum of higher bits"<<endl;
								}
								else 
									vhdl << ";   -- higher bits also untouched"<<endl;
							}
							else {// op == RSub
								// The higher bits (size-1 downto s) of the result are those of -x, possibly plus 11...1 if y was negative
								vhdl << tab << sao->name << "("<<sao->size-1<<" downto "<< shift<<") <= " ;
								if(sao->j->n < 0) 
									vhdl <<"(" << sao->size-1 << " downto " <<  shift <<" => " << jname << "(" << jsize-1 << ")) ";
								else
									vhdl <<"(" << sao->size-1 << " downto " <<  shift <<" => '0') ";
								vhdl << " - " << iname << ";   -- sum of higher bits"<<endl;
							}
						} // end if (shift >= jsize)
						else{ 
							// jsize>s.        Cases:      xxxxx              xxxxxx
							//                                yyyyyyyyyy             yyyyyyyyyyyy
							// so we may need to sign-extend Vx, or Vy, or even both.
							// The higher bits of the result are sum/diff
							isignal = sao->name + "_High_L";  
							jsignal = sao->name + "_High_R"; 
							// The x part
							vhdl << tab << declare(isignal,  size - shift) << " <= ";
							if(size >= isize +  shift +1) { // need to sign-extend vx. If the constant is positive, padding with 0s is enough
								vhdl <<" (" << size-1 << " downto " << isize +  shift <<" => ";
								if(sao->i->n >= 0)   vhdl << "'0'";// pad with 0s 
								else                 vhdl << iname << "(" << isize-1 << ")"; // sign extend
								vhdl << ") & ";
							}
							vhdl << iname << "("<< isize -1 <<" downto 0) ;" << endl;
							// the y part
							vhdl << tab << declare(jsignal,  size - shift) << " <= ";
							if(size >= jsize+1) {// need to sign-extend vy. If the constant is positive padding with 0s is enough
								vhdl <<" (" << size-1 << " downto " << jsize <<" => ";
								if(sao->j->n >= 0)  vhdl << "'0'"; // pad with 0s 
								else                vhdl << jname << "(" << jsize-1 << ")";// sign extend
								vhdl << ") & ";
							}
							vhdl << jname << "("<< jsize -1 <<" downto " <<  shift << "); " << endl;
					
							// do the sum
							if(use_pipelined_adder) {
								inPortMap  (adder, "X", jsignal);
								if(op==Add) {
									inPortMap  (adder, "Y", isignal);
									inPortMapCst  (adder, "Cin", "'0'");
								} 
								else { // RSub
									string isignalneg = isignal+"_neg"; 
									vhdl << declare(isignalneg, size - shift) << " <= not " << isignal;
									inPortMap  (adder, "Y", isignal);
									inPortMapCst  (adder, "Cin", "'1'");
								}
								string resname=sao->name+"_h";
								outPortMap (adder, "R",resname);
								vhdl << instance(adder, sao->name + "_adder");

								syncCycleFromSignal(resname, false);
								//nextCycle();
								vhdl << tab << declare(sao->name, sao->size) << "("<<size-1<<" downto " <<  shift << ") <= " << resname + ";" << endl;
							}
							else
								vhdl << tab << declare(sao->name, sao->size) << "("<<size-1<<" downto " <<  shift << ") <= " // vz (size-1 downto s)
									  << jsignal << (op==Add ? " + " : "-") << isignal << ";   -- sum of higher bits" << endl; 
			
							// In both cases the lower bits of the result (s-1 downto 0) are untouched
							vhdl << tab << sao->name << "("<<shift-1<<" downto 0) <= " << jname <<"("<< shift-1<<" downto 0);   -- lower bits untouched"<<endl;

						} // end if (shift >= jsize) else
					} // end if(op == Add || op == RSub) 
					else { // op=Sub 
						// Do a normal subtraction of size size
						isignal = sao->name + "_L";  
						jsignal = sao->name + "_R"; 
						vhdl << tab << declare(isignal,  size) << " <= ";
						if(size > isize+shift) {// need to sign-extend vx. If the constant is positive padding with 0s is enough
							vhdl <<" (" << size-1 << " downto " << isize+shift <<" => ";
							if(sao->i->n >= 0)   vhdl << "'0'";// pad with 0s 
							else                 vhdl << iname << "(" << isize-1 << ")";// sign extend
							vhdl << ") & ";
						}
						vhdl << iname << " & (" << shift-1 << " downto 0 => '0');" << endl;

						vhdl << tab << declare(jsignal,  size) << " <= ";
						vhdl <<" (" << size-1 << " downto " << jsize <<" => ";
						if(sao->j->n >= 0)   vhdl << "'0'";// pad with 0s 
						else                 vhdl << jname << "(" << jsize-1 << ")";// sign extend
						vhdl << ") & ";
						vhdl << jname << ";" << endl;
					
						// do the subtraction
						if(use_pipelined_adder) {
							string jsignalneg = jsignal+"_neg"; 
							vhdl << declare(jsignalneg, size) << " <= not " << jsignal;
							inPortMap  (adder, "X", isignal);
							inPortMap  (adder, "Y", jsignalneg);
							inPortMapCst  (adder, "Cin", "'1'");
							string resname=sao->name+"_h";
							outPortMap (adder, "R",resname);
							vhdl << instance(adder, sao->name + "_adder");

							syncCycleFromSignal(resname, false);
							//nextCycle();
							vhdl << tab << declare(sao->name, size) << " <=  " << resname + ";" << endl;
						
						}
					}
				}
			
				return;

				// shift and neg almost identical
			case Shift:
			case Neg:
				isize = sao->i->size;

				double local_delay;
				if(op == Neg){   
					local_delay = target_->adderDelay(sao->cost_in_full_adders);
				}
				else 
					local_delay=0;

				build_pipeline(sao->i, idelay);

				iname = sao->i->name; 
				setCycleFromSignal(iname, false);

				if(isSequential() 
					&& idelay +  target_->localWireDelay() + local_delay > 1./target_->frequency()
					&& sao->i->op != X) {
					// This resets the partial delay to that of this ShiftAddOp
					nextCycle();
					partial_delay =  target_->ffDelay() + target_->adderDelay(sao->cost_in_full_adders);
				}
				else{ // this ShiftAddOp and its child will be in the same pipeline level
					partial_delay = idelay + target_->localWireDelay() + local_delay;
				}
				vhdl << tab << declare(sao->name, size) << " <= " ;
				// TODO use a pipelined IntAdder when necessary
				if(op == Neg)   
					vhdl << "("<< size -1 <<" downto 0 => '0') - " << iname <<";"<<endl; 
				else { // Shift
					if (shift == 0) 
						vhdl << iname <<";"<<endl; 
					else
						vhdl << iname <<" & ("<< shift - 1 <<" downto 0 => '0');"<<endl;
				}
				break;

			}   
		}
	}











	IntConstMult::IntConstMult(Target* _target, int _xsize, mpz_class n) :
		Operator(_target), n(n), xsize(_xsize){
		ostringstream name; 

		srcFileName="IntConstMult";
		setCopyrightString("Florent de Dinechin (2007-2011)");

		//C++ wrapper for GMP does not work properly on win32, using mpz2string
		name <<"IntConstMult_"<<xsize<<"_"<<mpz2string(n);
		setName(name.str());

		implementation = new ShiftAddDag(this);

		rsize = intlog2(n * ((mpz_class(1)<<xsize)-1));

		addInput("X", xsize);
		addOutput("R", rsize);

		// Build in implementation a tree constant multiplier 
		implementation->result = buildMultBoothTree(n);

		if(verbose>=DETAILED) showShiftAddDag();
		
		int cost=compute_total_cost(implementation->result);
		REPORT(INFO, "Estimated bare cost (not counting pipeline overhead) : " << cost << " FA/LUT" );
		REPORT(INFO, "Depth of the DAG : " << compute_tree_depth(implementation->result) );
		
		double delay=0.0;
		// recursively build the pipeline in the vhdl stream
		build_pipeline(implementation->result, delay);
		
		// copy the top of the DAG into variable R
		vhdl << endl << tab << "R <= " << implementation->result->name << "("<< rsize-1 <<" downto 0);"<<endl;
		outDelayMap["R"] = delay;
	}









	// The constructor for rational constants

	IntConstMult::IntConstMult(Target* _target, int _xsize, mpz_class n, mpz_class period, int periodMSBZeroes, int periodSize, mpz_class header, int headerSize, int i, int j) :
		Operator(_target), xsize(_xsize){
		ostringstream name; 

		srcFileName="IntConstMult (periodic)";
		setCopyrightString("Florent de Dinechin (2007-2011)");
		name <<"IntConstMultPeriodic_"<<xsize<<"_"<<mpz2string(header)<<"_"<<headerSize
		     <<"_"<<mpz2string(period<<periodMSBZeroes)<<"_"<<periodSize<<"_"<<i<<"_";
		if (j<0) 
			name << "M" << -j;
		else
			name << j;
		setName(name.str());

		implementation = new ShiftAddDag(this);

		rsize = intlog2(n * ((mpz_class(1)<<xsize)-1));
		REPORT(INFO, "Building a periodic DAG for  " << n );

		addInput("X", xsize);
		addOutput("R", rsize);

		// Build in implementation a tree constant multiplier 
		
		ShiftAddOp* powerOfTwo[1000]; // Should be enough for anybody

		// Build the multiplier by the period
		powerOfTwo[0] = buildMultBoothTree(period);

		// Example: actual period 11000, periodSize=5, is represented by period=11, periodMSBZeroes=3
		// powerOfTwo[0] will build mult by 11
		// powerOfTwo[1] will build mult by 1100011: shift=periodSize
		// powerOfTwo[2] will build mult by 11000110001100011: shift=2*periodSize

		for (int k=1; k<=i; k++){
			powerOfTwo[k]=  new ShiftAddOp(implementation, Add, powerOfTwo[k-1], (periodSize<<(k-1)), powerOfTwo[k-1] );
		}


		if(header==0)  {
			if(j==-1) //just repeat the period 2^i times
				implementation->result = 	powerOfTwo[i];
			else
				implementation->result = new ShiftAddOp(implementation, Add, powerOfTwo[j], (periodSize<<i), powerOfTwo[i] );
		}
		else {
			// REPORT(DEBUG, "DAG before adding header and zero=" << zeroLSBs);
			// if(verbose>=DETAILED) showShiftAddDag();
			REPORT(DEBUG, "Header not null: header="<<header);
			ShiftAddOp* headerSAO;
			headerSAO=buildMultBoothTree(header);
			if(j==-1)//just repeat the period 2^i times
				implementation->result = 	new ShiftAddOp(implementation, Add, headerSAO, (periodSize<<i) - periodMSBZeroes, powerOfTwo[i] );
			else { // Here we should generate both trees and use the smaller. The following static decision is probably always the good one
				if (i==j) {// I know this case should be equivalent to j=-1 and i+1, but it seems to happen
					powerOfTwo[i+1] = implementation->provideShiftAddOp(Add, powerOfTwo[i], (periodSize<<i), powerOfTwo[i] );
					implementation->result = new ShiftAddOp(implementation, Add, headerSAO, (periodSize<<(i+1)) - periodMSBZeroes,powerOfTwo[i+1]);
				}
				else{
					ShiftAddOp* tmp = implementation->provideShiftAddOp(Add, headerSAO, (periodSize<<j) - periodMSBZeroes, powerOfTwo[j] );
					implementation->result = new ShiftAddOp(implementation, Add, tmp, (periodSize<<i), powerOfTwo[i] );
				}
			}
		}

		if(verbose>=DETAILED) showShiftAddDag();
		
		int cost=compute_total_cost(implementation->result);
		REPORT(INFO, "Estimated bare cost (not counting pipeline overhead) : " << cost << " FA/LUT" );
		REPORT(INFO, "Depth of the DAG : " << compute_tree_depth(implementation->result) );

		double delay=0.0;
		// recursively build the pipeline in the vhdl stream
		build_pipeline(implementation->result, delay);
		
		// copy the top of the DAG into variable R
		vhdl << endl << tab << "R <= " << implementation->result->name << "("<< rsize-1 <<" downto 0);"<<endl;
		outDelayMap["R"] = delay;
		
	}




	// One hand-coded Lefevre multiplier, for comparison purposes -- to be inserted somewhere in the constructor

#if 0
	if(false && n==mpz_class("254876276031724631276054471292942"))
		{
			const int PX=0;
			cerr<<"Optimization by rigo.c"<< endl;//                    x    s    y
			/*
			 */
			implementation->computeVarSizes(); 
			implementation->result = implementation->sao.size()-1;
		}
	else 
		if(n==mpz_class("1768559438007110"))
			{
				const int PX=0;
				cerr<<"Optimization by rigo.c"<< endl;//                   
				implementation->addOp( new ShiftAddOp(implementation, Neg,   PX) );       // 1  mx = -u0
				implementation->addOp( new ShiftAddOp(implementation, Add,   0, 19,  0) );        // 2  u3 = u0 << 19 + u0
				implementation->addOp( new ShiftAddOp(implementation, Shift,   2, 20) );          // 3  u103 = u3 << 20
				implementation->addOp( new ShiftAddOp(implementation, Add,   3, 4,   3) );        // 4  u203 = u103 << 4  + u103
				implementation->addOp( new ShiftAddOp(implementation, Add,   0, 14,  1) );        // 5  u7 = u0 << 14 + mx
				implementation->addOp( new ShiftAddOp(implementation, Add,   5, 6,  0) );         // 6  u6 = u7 << 6 + u0
				implementation->addOp( new ShiftAddOp(implementation, Add,   6, 10,  0) );        // 7  u5 = u6 << 10 + u0
				implementation->addOp( new ShiftAddOp(implementation, Shift, 7,  16    ));         // 8  u1 = u5 << 16
				implementation->addOp( new ShiftAddOp(implementation, Add,   8, 0,   4) ) ;       // 9  u101 = u1 + u203
				implementation->addOp( new ShiftAddOp(implementation, Add,   0, 21,  1) );        // 10 u107 = u0 << 21 + mx
				implementation->addOp( new ShiftAddOp(implementation, Add,   10, 18,  0) );       // 11 u106 = u107 << 18 + u0
				implementation->addOp( new ShiftAddOp(implementation, Add,   11, 4,   1) );       // 12 u105 = u106 << 4 + mx
				implementation->addOp( new ShiftAddOp(implementation, Add,   12, 5,   0) );       // 13 u2 = u105 << 5 + u0
				implementation->addOp( new ShiftAddOp(implementation, Shift, 13, 1) );            // 14 u102 = u2 << 1
				implementation->addOp( new ShiftAddOp(implementation, Neg,   14) );       // 15 mu102 = - u102
				implementation->addOp( new ShiftAddOp(implementation, Add,   14, 2,   15) );       // 16 u202 = u102 << 2  + mu102
				implementation->addOp( new ShiftAddOp(implementation, Add,   9, 0,   16) );        // R = u101 + u202

				/*
				  0  u0 = x
				  1  mx = -u0
				  2  u3 = u0 << 19 + u0
				  3  u103 = u3 << 20
				  4  u203 = u103 << 4  + u103
				  5  u7 = u0 << 14 + mx
				  6  u6 = u7 << 6 + u0
				  7  u5 = u6 << 10 + u0
				  8  u1 = u5 << 16
				  9  u101 = u1 + u203
				  10 u107 = u0 << 21 + mx
				  11 u106 = u107 << 18 + u0
				  12 u105 = u106 << 4 + mx
				  13 u2 = u105 << 5 + u0
				  14 u102 = u2 << 1
				  15 mu102 = - u102
				  16 u202 = u102 << 2  + mu102
				  R = u101 + u202
				*/
				implementation->computeVarSizes(); 
				implementation->result = implementation->saolist.size()-1;
			}
	//	else
#endif


	IntConstMult::~IntConstMult() {
		delete implementation;
	}





	string IntConstMult::printBoothCode(int* BoothCode, int size) {
		ostringstream s;
		for (int i=size-1; i>=0; i--) {
			if(BoothCode[i]==0)       s << "0"; 
			else if(BoothCode[i]==1)  s << "+" ;
			else if(BoothCode[i]==-1) s << "-" ;   
		}
		return s.str();
	}



	bool needsMinusX(int * boothCode, int n) {
		for(int i=0; i<n; i++)
			if(boothCode[i]==-1)
				return true;
		return false;
	}



	/** Recodes input n, returns the number of non-zero bits */
	int IntConstMult::recodeBooth(mpz_class n, int* BoothCode) {
		int i;
		int *b, *c;
		int nonZeroInBoothCode = 0;
		int nsize = intlog2(n);

		// Build the binary representation -- I'm sure there is a mpz method for it
		mpz_class nn = n;
		int l = 0;
		int nonZero=0;
		b = new int[nsize+1];
		while(nn!=0) {
			b[l] = (nn.get_ui())%2;
			nonZero+=b[l]; // count the ones
			l++;
			nn = nn>>1;
		}
		b[nsize]=0;

		ostringstream o;
		for (int i=nsize-1; i>=0; i--)    o << ((int) b[i]);   
		REPORT(DETAILED, "Constant binary is  " << o.str() << " with " << nonZero << " ones"); 

		int needMinusX=0;
		c = new int[nsize+1];
				
		c[0] = 0;
		for (i=0; i<nsize; i++) {
			if (b[i] + b[i+1] + c[i] >= 2)
				c[i+1] = 1;
			else 
				c[i+1] = 0;
			BoothCode[i] = b[i] + c[i] -2*c[i+1];
			if(BoothCode[i]==-1)
				needMinusX=1;
		}
		BoothCode[nsize] = c[nsize];


		for (i=0; i<=nsize; i++) {
			if (BoothCode[i] != 0) 
				nonZeroInBoothCode ++;
		}

		REPORT(DETAILED, "Booth recoding is  " << printBoothCode(BoothCode, nsize+1)  << " with " << nonZeroInBoothCode << " non-zero digits"); 
		
		// If there is no savings in terms of additions, discard the Booth code 
		if (nonZeroInBoothCode+needMinusX >= nonZero) {
			REPORT(DETAILED, "Reverting to non-Booth"); 
			for (i=0; i<nsize; i++)
				BoothCode[i] = b[i];
			nonZeroInBoothCode=nonZero;
			REPORT(DETAILED, "Booth recoding is   " << printBoothCode(BoothCode, nsize)  << " with " << nonZeroInBoothCode << " non-zero digits"); 
		}
  
		delete [] c; delete [] b;
		return nonZeroInBoothCode;
	}







#if 0
	// The simple, sequential version: the DAG is a rake
	void  IntConstMult::buildMultBooth(){
		int k,i;
		ShiftAddOp *z;
		i=0;

		// build the opposite of the input
		ShiftAddOp* MX = new ShiftAddOp(implementation, Neg, implementation->PX);

		while(0==BoothCode[i]) i++;
		// first op is a shift, possibly of 0
		if (1==BoothCode[i]) {
			if(i==0) // no need to add a variable
				z = implementation->PX;
			else {
				z = new ShiftAddOp(implementation, Shift,   implementation->PX, i);
			}
		}
		else {
			if(i==0) 
				z = MX;
			else {
				z = new ShiftAddOp(implementation, Shift,   MX, i);
			}
		}
		i++;
	
		for (k=1; k<nonZeroInBoothCode; k++) {
			while(0==BoothCode[i]) i++;
			if (1==BoothCode[i]) 
				z = new ShiftAddOp(implementation, Add,   implementation->PX, i,  z);
			else
				z = new ShiftAddOp(implementation, Add,   MX, i,  z);
			i++;
		}
		// Which variable number holds the result?
		implementation->result = z;
	}
#endif





	// Builds a balanced tree.
	// Assumes: implementation is initialized
	ShiftAddOp*  IntConstMult::buildMultBoothTree(mpz_class n){
		int k,i,j,nk;
		ShiftAddOp *result;
		ShiftAddOp**  level;
		int*     shifts;
	
		int nsize = intlog2(n);

		REPORT(DEBUG, "Entering buildMultBoothTree for "<< n);		
		if((mpz_class(1) << (nsize-1)) == n) { // power of two
			REPORT(INFO, "Power of two");
			result= implementation->provideShiftAddOp(Shift, implementation->PX, intlog2(n)-1);
		}
		else {
			int* BoothCode;
			int nonZeroInBoothCode;
			BoothCode = new int[nsize+1];
			nonZeroInBoothCode = recodeBooth(n, BoothCode);
			
			i=0;
			while(0==BoothCode[i]) i++;
			int globalshift=i;
			if (nonZeroInBoothCode==1) { // a power of two
				if(i==0) // no need to add a variable
					result = implementation->PX;
				else {
					result = new ShiftAddOp(implementation, Shift,   implementation->PX, i);
				}
			}
			else { // at least two non-zero bits
				
				// build the opposite of the input
	
				ShiftAddOp* MX=0;

				if(needsMinusX(BoothCode, nsize))
					MX = new ShiftAddOp(implementation, Neg, implementation->PX);
				
				// fill an initial array with Xs and MXs
				level = new ShiftAddOp*[nonZeroInBoothCode];
				shifts = new int[nonZeroInBoothCode];
				for (j=0; j<nonZeroInBoothCode-1; j++) {
					if (1==BoothCode[i]) 
						level[j] = implementation->PX;
					else
						level[j] = MX; 
					shifts[j] = i - globalshift;
					i++;
					while(0==BoothCode[i]) i++;
				}
				level[j] = implementation->PX;
				shifts[j] = i-globalshift;
				
				k=nonZeroInBoothCode;
				while(k!=1) {
					nk=k>>1;
					for (j=0; j<nk; j++) {
						
						level[j] = implementation->provideShiftAddOp(Add, level[2*j+1], (shifts[2*j+1]-shifts[2*j]),  level[2*j]);
						shifts[j] = shifts[2*j];
					}
					if(nk<<1 != k) {
						level[j] = level[2*j];
						shifts[j] = shifts[2*j];
						k=nk+1;
					}
					else 
						k=nk;
				}
				if(globalshift==0)
					result = level[0];
				else
					result= implementation->provideShiftAddOp(Shift, level[0], globalshift);
				
				delete level;
				delete shifts;
				delete [ ] BoothCode;
				
			}
		}

		REPORT(DETAILED,  "Number of adders: "<<implementation->saolist.size() );
		return result;
	}
	


	void IntConstMult::showShiftAddDag(){
		REPORT(DETAILED, " ShiftAddDag:");
		for (uint32_t i=0; i<implementation->saolist.size(); i++) {
			REPORT(DETAILED, "  "<<*(implementation->saolist[i]));
		}
	};








	void optimizeLefevre(const vector<mpz_class>& constants) {
	

	};


	void IntConstMult::emulate(TestCase *tc){
		mpz_class svX = tc->getInputValue("X");
		mpz_class svR = svX * n;
		tc->addExpectedOutput("R", svR);
	}



	void IntConstMult::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		tc = new TestCase(this); 
		tc->addInput("X", mpz_class(0));
		emulate(tc);
		tc->addComment("Multiplication by 0");
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("X", mpz_class(1));
		emulate(tc);
		tc->addComment("Multiplication by 1");
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("X", mpz_class(2));
		emulate(tc);
		tc->addComment("Multiplication by 2");
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("X", (mpz_class(1) << xsize) -1);
		emulate(tc);
		tc->addComment("Multiplication by the max positive value");
		tcl->add(tc);

		//	tc = new TestCase(this); 
		//	tc->addInput("X", (mpz_class(1) << (xsize) -1) + mpz_class(1));
		//	emulate(tc);
		//	tc->addComment("Multiplication by 10...01");
		//	tcl->add(tc);


	}

}
