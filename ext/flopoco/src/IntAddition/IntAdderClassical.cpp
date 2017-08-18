/*
An integer adder for FloPoCo

It may be pipelined to arbitrary frequency.
Also useful to derive the carry-propagate delays for the subclasses of Target

Authors:  Bogdan Pasca, Florent de Dinechin

This file is part of the FloPoCo project
developed by the Arenaire team at Ecole Normale Superieure de Lyon

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
2008-2010.
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
#include "../Operator.hpp"
#include "../IntAdder.hpp"
#include "IntAdderClassical.hpp"

using namespace std;

namespace flopoco {
	
	IntAdderClassical::IntAdderClassical ( Target* target, int wIn, map<string, double> inputDelays, int optimizeType, bool srl) :
	IntAdder(target, wIn, inputDelays, true), wIn_ ( wIn ) {
		srcFileName="IntAdderClassical";
		ostringstream name;
		
		setCopyrightString ( "Bogdan Pasca, Florent de Dinechin (2008-2010)" );
		name << "IntAdderClassical_" << wIn_<<"_f"<<target->frequencyMHz()<<"_uid"<<getNewUId();
		setName( name.str() );
		
		// Set up the IO signals
		addInput  ( "X"  , wIn_, true );
		addInput  ( "Y"  , wIn_, true );
		addInput( "Cin");
		addOutput ( "R"  , wIn_, 1 , true );
		
		inputsGotRegistered = false;
		objectivePeriod	 = 1 / target->frequency();
		maxInputDelay      = getMaxInputDelays ( inputDelays );

		if (maxInputDelay < target->ffDelay() + target->localWireDelay()){
			inputDelays["X"] = target->ffDelay() + target->localWireDelay();
		}
		
		if ( maxInputDelay > objectivePeriod ) {
			/* this component should register its inputs */
			nextCycle();
			maxInputDelay = target->ffDelay() + target->localWireDelay();	
			inputDelays.clear();
			inputsGotRegistered = true;
		}	
		
		classicalSlackVersion = -1;
		
		switch (optimizeType) {
			case 0:  cost = getLutCostClassical  (target,wIn, inputDelays, srl); break;
			case 1:  cost = getRegCostClassical  (target,wIn, inputDelays, srl); break;
			case 2:  cost = getSliceCostClassical(target,wIn, inputDelays, srl); break;

			default: cost = getSliceCostClassical(target,wIn, inputDelays, srl); break;
		}
		
		
		vhdl << tab << "--Classical"<<endl;
		if ( isSequential() ) {
			if ( classicalSlackVersion == -1 ) {
				/* the non-slack version */
				updateParameters ( target, alpha, beta, k );
				REPORT ( DEBUG, "1) alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			} else {
				if ( classicalSlackVersion == 0 ) {
					/* the slack version that does not buffer the inputs*/
					updateParameters ( target, inputDelays, alpha, beta, gamma, k );
					REPORT ( DEBUG, "alpha="<<alpha<<" beta="<<beta<<" gamma="<<gamma<<" k="<<k );
				} else {
					nextCycle(); /* bufferning the inputs */
					REPORT ( DETAILED, "Building architecture for classical version with slack: buffering" );
					updateParameters ( target, alpha, beta, k );
					REPORT ( DEBUG, "alpha="<<alpha<<" beta="<<beta<<" k="<<k );
				}
			}
			
			if ( k>1 ) {
				/* init the array with chunk sizes */
				cSize = new int[k+1];
				if ( (maxInputDelay != 0 ) && (classicalSlackVersion == 0) )
					cSize[0] = gamma;
				else
					cSize[0] = alpha;

				for ( int i=1; i<k-1; i++ )
					cSize[i] = alpha;
				
				cSize[k-1] = beta;
			
				/* the indexes of the chunks */
				cIndex = new int[k];
				cIndex[0] = cSize[0];
				for ( int i=1; i < k; i++ )
					cIndex[i] = cIndex[i-1] + cSize[i];
				
				/* The implementation */
				for ( int i=0; i < k; i++ ) {
					vhdl << tab << declare ( join ( "x",i ), cSize[i],true ) << " <= X" << range ( cIndex[i]-1, ( i>0?cIndex[i-1]:0 ) ) << ";" << endl;
					vhdl << tab << declare ( join ( "y",i ), cSize[i],true ) << " <= Y" << range ( cIndex[i]-1, ( i>0?cIndex[i-1]:0 ) ) << ";" << endl;
				}
				
				for ( int i=0; i < k; i++ ) {
					vhdl << tab << declare ( join ( "sum",i ),cSize[i]+1,true ) << " <= ( \"0\" & "<< join ( "x",i ) << ") + ( \"0\" & "<< join ( "y",i ) << ")  + ";
					if ( i==0 ) vhdl << "Cin";
					else      vhdl << join ( "sum",i-1 ) <<of ( cSize[i-1] );
					vhdl << ";"	<< endl;
					if ( i < k-1 )
						nextCycle(); //////////////////////////////////////////
				}
				
				vhdl << tab << "R <= ";
				for ( int i=k-1; i >= 1; i-- ) {
					vhdl << join ( "sum",i ) <<range ( cSize[i]-1,0 ) << " & ";
				}
				vhdl << "sum0" << range ( cSize[0]-1,0 ) <<";"<<endl;
				/* the output is asociated with the combinatorial delay caused
				by the most-significant bits addition */
				outDelayMap["R"] = target->adderDelay ( cSize[k-1] ) + ( getCurrentCycle() >0?0:getMaxInputDelays ( inputDelays ) );
			} else {
				vhdl << tab << " R <= X + Y + Cin;" << endl;
				if ( (maxInputDelay == 0 ) || (classicalSlackVersion==0) )
					outDelayMap["R"] = target->adderDelay ( wIn ) + getMaxInputDelays ( inputDelays );
				else 
					outDelayMap["R"] = target->adderDelay ( wIn );
			}
	
		} else {
			vhdl << tab << " R <= X + Y + Cin;" << endl;
			outDelayMap["R"] = target->adderDelay ( wIn_ ) + getMaxInputDelays ( inputDelays );
		}
	}

	
	/**************************************************************************/
	IntAdderClassical::~IntAdderClassical() {
	}

	
	/**************************************************************************/
	/*******************             LUTS             *************************/
	/**************************************************************************/
	int IntAdderClassical::getLutCostClassical (Target* target, int wIn, map<string, double> inputDelays, bool srl ) {
		REPORT ( DEBUG, DEBUG_SEPARATOR );
		if ( getMaxInputDelays(inputDelays) == target->ffDelay() + target->localWireDelay() || getMaxInputDelays(inputDelays) == 0.0 ) {
			/* either the previous component has registered its outputs or
			   this component has registered its inputs or 
			   the user did not properly use the framework, so we consider that the 
			   input comes from a register level
			*/
			int alpha, beta, k, cost;
			updateParameters (target, alpha, beta, k);
			REPORT ( DEBUG, "LUT, Classical, NO-SLACK: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			if (target->getVendor() == "Xilinx" && srl ){
				if (k==1 || k==2)  
					cost = wIn + k-1;
				else
					cost = (4*k-10)*alpha + 3*beta + k-1; 
			}else{// for Altera or NoSRL 
				cost = wIn + k-1;
				REPORT(DETAILED, "Selected: Classical, NO-SLACK with LUT cost " << cost);
			}
			return cost;
		}else {
			int version0, version1;
			int alpha, beta, gamma, k;
			/* Version 0: we try to adapt the architecture to the new slack */
			updateParameters (target, inputDelays, alpha, beta, gamma, k);
			REPORT ( DEBUG, "LUT, Classical, SLACK, Version 0: alpha="<<alpha<<" beta`="<<beta<<" gamma="<<gamma<<" k="<<k );
			if ( k > 0 ) /* there is an alternative possible splitting */
				if (target->getVendor()=="Xilinx" && srl) {
					if ( k==1 ) 
						version0 = wIn;
					else if (k==2)
						version0 = wIn + 1;
					else if (k==3)
						version0 = wIn + 2*beta + 2;
					else
						version0 = wIn + gamma + (3*k-10)*alpha + 2*beta + k-1;
				}else /* NO SRLs */
					version0 = wIn + k-1;
			else /* no solution was found, cost is +INF */
				version0 = PINF;
				
			/* Version 1: we buffer the inputs and proceed */
			updateParameters (target, alpha, beta, k);
			REPORT ( DEBUG, "LUT, Classical, SLACK, Version 1 (buffered inputs): alpha="<<alpha<<" beta="<<beta<<" k="<<k << " p="<<k+1 );
			if (target->getVendor() == "Xilinx" && srl ){
				if (k==1)
					version1 = wIn;   
				else if (k==2)
					version1 = 2*beta + wIn + 1;
				else
					version1 = (4*k-8)*alpha + 3*beta + k-1; 
			}else{// for Altera or NoSRL 
				version1 = wIn + k-1;
				REPORT(DETAILED, "Selected: Classical, NO-SLACK with LUT cost " << cost);
			}
				
			REPORT ( DEBUG, "LUT, Classical, SLACK, Version 0: " << version0 );
			REPORT ( DEBUG, "LUT, Classical, SLACK, Version 1 (buffered inputs) " << version1 );
				
			if ( version0 <= version1 ) {
				/* for equality version 0 has less pipeline levels */
				classicalSlackVersion = 0;
				REPORT ( DETAILED, "Selected: Classical SLACK version is 0 with LUT cost " << version0 );
				return version0;
			} else {
				classicalSlackVersion = 1;
				REPORT ( DETAILED, "Selected: Classical SLACK version is 1 (buffered inputs) with LUT cost " << version1 );
				return version1;
			}
		}
		cerr << "Error in " <<  __FILE__ << "@" << __LINE__;
		exit ( -1 );
		return -1;
	}
	
	
	/**************************************************************************/
	/*******************          REGISTERS           *************************/
	/**************************************************************************/
	int IntAdderClassical::getRegCostClassical ( Target* target, int wIn, map<string, double> inputDelays, bool srl ) {
		REPORT ( DEBUG, DEBUG_SEPARATOR );
		
		if ( getMaxInputDelays(inputDelays) == target->ffDelay() + target->localWireDelay() || getMaxInputDelays(inputDelays) == 0.0 ) {

//			if ( inputsGotRegistered ){
//				int alpha, beta, k, cost;
//				updateParameters (target, alpha, beta, k);
//				REPORT ( DEBUG, "REG, CLASSICAL, NO-SLACK: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
//				if ( k == 1 ) {
//					cost = 2*wIn;
//				}else if (target->getVendor() == "Xilinx" && srl)
//					cost = wIn - beta  + 2* wIn;
//				else
//					cost = ( ( 3*k*k -7*k+4 ) *alpha/2 + 2* ( k-1 ) *beta + k-1 ) + 2*wIn;
//			
//				REPORT ( DETAILED, "Selected: Classical, NO-SLACK, with REG cost " << cost );
//				return cost;
//			}else{ //the adder inputs come from one register which is outside
				int alpha, beta, k, cost;
				updateParameters (target, alpha, beta, k);
				REPORT ( DEBUG, "REG, CLASSICAL, NO-SLACK: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
				if ( k == 1 ) {
					cost = 0;
				}else if (target->getVendor() == "Xilinx" && srl)
					cost = wIn - beta;
				else
					cost = ( ( 3*k*k -7*k+4 ) *alpha/2 + 2* ( k-1 ) *beta + k-1 );
			
				REPORT ( DETAILED, "Selected: Classical, NO-SLACK, with REG cost " << cost );
				return cost;
//			}
		} else { /* there's no register before the computations, eiter internal or external to the adder */
			int version0, version1;
			int alpha, beta, gamma, k;
			
			/* Version 0: we try to adapt the architecture to the new slack */
			updateParameters ( target, inputDelays, alpha, beta, gamma, k );
			REPORT ( DEBUG, "REG, Classical, SLACK, Version 0: alpha="<<alpha<<" beta="<<beta<<" gamma="<<gamma<<" k="<<k );
			if ( k>0 ) { /* we found room for improvement */
				if ( k==1 )
					version0 = 0;
				if ( target->getVendor()=="Xilinx" && srl){
					if ( k == 2 )
						version0 = gamma + 2*alpha + 1;
					else if (k==3)
						version0 = 2*gamma + 3*alpha + 2*beta + 2;
					else 	
						version0 = 2*gamma + (4*k-9)*alpha + 2*beta + k-1;
				}else{ //altera  
					if (k == 2 )
						version0 = wIn + alpha + 1;
					else 
						version0 = ( k-1 ) *gamma + alpha*3*(k*k-3*k+2)/2 + 2*(k-1)*beta + k-1;
				}
			} else /* there's no room for any computations */
				version0 = PINF; //infinity
				
			/* Version 1: we buffer the inputs and proceed */
			updateParameters ( target, alpha, beta, k );
			REPORT ( DEBUG, "REG, Classical, SLACK, Version 1: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			
			if (target->getVendor()=="Xilinx" && srl){
				if ( k==1 ) 
					version1 = 2*wIn + 1;
				else if ( k == 2 )
					version1 = 3*alpha + 2*beta + 2;
				else 
					version1 = (4*k-5)*alpha + 2*beta + k;
			}else{
				if ( k==1 )
					version1 = 2*wIn + 1;
				else
					version1 = 2*wIn + 1 + ( ( 3*k*k -7*k+4 ) *alpha/2 + 2* ( k-1 ) *beta + k-1 );
			}
			
			REPORT ( DEBUG, "REG, Classical, SLACK, Version 0: " << version0 );
			REPORT ( DEBUG, "REG, Classical, SALCK, Version 1 (buffered inputs): " << version1 );
			
			if ( version0 <= version1 ) {
				classicalSlackVersion = 0;
				REPORT ( DETAILED, "Selected: Classical SLACK version is 0 with REG cost " << version0 );
				return version0;
			} else {
				classicalSlackVersion = 1;
				REPORT ( DETAILED, "Selected: Classical SLACK version is 1 with REG cost " << version1 );
				return version1;
			}
		}
		
		cerr << "Error in " <<  __FILE__ << "@" << __LINE__;
		exit ( -1 );
		return -1;
	}

	
	/**************************************************************************/
	/*******************           SLICES             *************************/
	/**************************************************************************/
	int IntAdderClassical::getSliceCostClassical ( Target* target, int wIn, map<string, double> inputDelays, bool srl ) {
		REPORT ( DEBUG, DEBUG_SEPARATOR );

		if ( getMaxInputDelays(inputDelays) == target->ffDelay() + target->localWireDelay() || getMaxInputDelays(inputDelays) == 0.0 ) {

//			if (inputsGotRegistered){
//				int alpha, beta, k, cost;
//				updateParameters ( target, alpha, beta, k );
//				REPORT ( DEBUG, "SLICE, CLASSICAL, NO-SLACK: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
//				if ( k == 1 )
//					cost =  wIn + 2*wIn;
//				else if ( target->getVendor()=="Xilinx" && srl )
//					cost = wIn + ( k-2 )*alpha  + k - 1 + 2*alpha;
//				else
//					cost = wIn + ( ( 3* ( k*k-3*k+2 ) ) /2 ) *alpha + 2* ( k-1 ) *beta + 2*wIn;
//			
//				REPORT ( DETAILED, "Selected: Classical NO-SLACK, with elementary SLICE/ALM cost " << cost );
//				return cost;
//			}else{	/* inputs were already registerd before entering intAdder */
	
				int alpha, beta, k, cost;
				updateParameters ( target, alpha, beta, k );
				REPORT ( DEBUG, "SLICE, CLASSICAL, NO-SLACK: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
				if ( k == 1 )
					cost =  wIn ;
				else if ( target->getVendor()=="Xilinx" && srl )
					cost = (4*k-7)*alpha + 3*beta + k-1 ;
				else
					cost = wIn + ( ( 3* ( k*k-3*k+2 ) ) /2 ) *alpha + 2* ( k-1 ) *beta;

				REPORT ( DETAILED, "Selected: Classical NO-SLACK, with elementary SLICE/ALM cost " << cost );
				return cost;

//			}	
		} else {
			int version0, version1;
			int alpha, beta, gamma, k;
			
			/* Version 0 */
			updateParameters ( target, inputDelays, alpha, beta, gamma, k );
			REPORT ( DEBUG, "SLICE, Classical, SLACK, Version 0: alpha="<<alpha<<" beta="<<beta<<"gamma="<<gamma<< " k="<<k );
			
			if ( k>0 ){ /* solution found */
				if ( k==1 )
					version0= wIn;
				if ( target->getVendor() == "Xilinx" && srl ) {
					if ( k == 2 ) 
						version0 = gamma + 3*alpha +1;
					else if (k==3)
						version0 = wIn + 3*beta + gamma + 2;
					else
						version0 = wIn + gamma + (3*k-7)*alpha + 2*beta + k-1; 						
				} else { //no SRL, Altera
					if ( k == 2 ) {
						version0 = gamma + 3*alpha + 1;
					} else {
						version0 = wIn + (k-2 )*gamma + 2* ( k-1 ) *beta + alpha* ( 2*k*k-11*k+10 ) /2 ;
					}
				}
			}else
				version0 = PINF; //infinity
				
			/* Version 1 */
			updateParameters ( target, alpha, beta, k );
			REPORT ( DEBUG, "elementary SLICE/ALM, Classical, SLACK Version 1: alpha="<<alpha<<" beta="<<beta<<" k="<<k<<" p="<<k+1 );
			
			if ( k==1 )
				version1= wIn;
			if (target->getVendor() == "Xilinx" && srl){
				if ( k == 2 ) {
					version1 = 3*alpha + 3*beta + 2;
				} else {
					version1= 3*wIn + (k-2)*alpha + k-1;
				}
			} else { //noSRL and Altera
				if ( k == 2 ) {
					version1 = 2*wIn + 1 + 3*((k*k-3*k+2)/2)*alpha + 2*( k-1 )*beta;
				} else {
					version1 = 3*wIn + 1 + 3*((k*k-3*k+2)/2)*alpha + 2*( k-1 )*beta;
				}
			}
			
			REPORT ( DETAILED, "elementary SLICE/ALM, Classical, SLACK, Version 0: " << version0 );
			REPORT ( DETAILED, "elementary SLICE/ALM, Classical, SLACK, Version 1: " << version1 );
			
			if ( version0 <= version1 ) {
				classicalSlackVersion = 0;
				REPORT ( DETAILED, "Selected: Classical SLACK version is 0 with elementary SLICE/ALM cost " << version0 );
				return version0;
			} else {
				classicalSlackVersion = 1;
				REPORT ( DETAILED, "Selected: Classical SLACK version is 1 with elementary SLICE/ALM cost " << version1 );
				return version1;
			}
		}
		cerr << "Error in " <<  __FILE__ << "@" << __LINE__;
		exit ( -1 );
		return -1;
	}
	
}


