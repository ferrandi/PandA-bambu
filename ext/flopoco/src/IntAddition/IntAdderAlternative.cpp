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
#include "IntAdderAlternative.hpp"

using namespace std;

namespace flopoco {
	
	IntAdderAlternative::IntAdderAlternative ( Target* target, int wIn, map<string, double> inputDelays, int optimizeType, bool srl) :
	IntAdder(target, wIn, inputDelays, true), wIn_(wIn), alternativeSlackVersion(0) {
		srcFileName="IntAdderAlternative";
		ostringstream name;
		
		setCopyrightString ( "Bogdan Pasca, Florent de Dinechin (2008-2010)" );
		name << "IntAdderAlternative_" << wIn_<<"_f"<<target->frequencyMHz()<<"_uid"<<getNewUId();
		setName ( name.str() );
		
		// Set up the IO signals
		addInput ( "X"  , wIn_, true );
		addInput ( "Y"  , wIn_, true );
		addInput( "Cin");
		addOutput ( "R"  , wIn_, 1 , true );
		
		inputsGotRegistered = false;
		objectivePeriod	 = 1 / target->frequency();
		maxInputDelay    = getMaxInputDelays ( inputDelays );
		
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
		
		vhdl << tab << "--Alternative"<<endl;
		alternativeSlackVersion = -1;
		 	
		switch (optimizeType) {
			case 0:  cost = getLutCostAlternative  (target,wIn, inputDelays, srl); break;
			case 1:  cost = getRegCostAlternative  (target,wIn, inputDelays, srl); break;
			case 2:  cost = getSliceCostAlternative(target,wIn, inputDelays, srl); break;

			default: cost = getSliceCostAlternative(target,wIn, inputDelays, srl); break;
		}
		
		if ( isSequential() ) {
			objectivePeriod	 = 1 / target->frequency();
			maxInputDelay = min(getMaxInputDelays(inputDelays), objectivePeriod);
			
			if ( alternativeSlackVersion == -1 ) {
				/* the non-slack version */
				updateParameters ( target, alpha, beta, k );
				REPORT ( DETAILED, "1) alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			} else {
				if (alternativeSlackVersion==0){
					updateParameters ( target, inputDelays, alpha, beta, k );
					REPORT ( DETAILED, "2) alpha="<<alpha<<" beta="<<beta<<" k="<<k );
				} else {
					nextCycle(); ///////////////////////////////////////////////
					updateParameters ( target, alpha, beta, k );
					REPORT ( DETAILED, "3) alpha="<<alpha<<" beta="<<beta<<" k="<<k );
				}
			}
			
			if (k>1){ 
				cSize = new int[k+1];
				for ( int i=0; i<k-1; i++ )
					cSize[i] = alpha;
				cSize[k-1] = beta;
				
				//the indexes in the inputs of the chunks
				cIndex = new int[k];
				cIndex[0]= cSize[0];
				for ( int i=1; i < k; i++ )
					cIndex[i] = cIndex[i-1] + cSize[i];
				

				for ( int i=0; i < k; i++ ) {
					vhdl << tab << declare ( join ( "s_sum_l",0,"_idx",i ), cSize[i]+1, true ) << " <= ( \"0\" & X" << range ( cIndex[i]-1, ( i>0?cIndex[i-1]:0 ) ) << ") + "
					<< "( \"0\" & Y" << range ( cIndex[i]-1, ( i>0?cIndex[i-1]:0 ) ) << ")" ;
					if ( i==0 ) vhdl << " + Cin";
					vhdl << ";" << endl;
				}

				for ( int i=0; i < k; i++ ) {
					vhdl << tab << declare ( join ( "sum_l",0,"_idx",i ), cSize[i], true ) << " <= " << join ( "s_sum_l",0,"_idx",i ) <<range ( cSize[i]-1,0 ) << ";" << endl;
					vhdl << tab << declare ( join ( "c_l",0,"_idx",i ), 1, true )          << " <= " << join ( "s_sum_l",0,"_idx",i ) <<range ( cSize[i],cSize[i] ) << ";" << endl;
				}
				
				for ( int i=1; i <= k-1 ; i++ ) {
					nextCycle(); ///////////////////////////////////////////////////////
					vhdl << tab << declare ( join ( "s_sum_l",i,"_idx",i ), cSize[i]+1, true ) << " <=  " <<  join ( "s_sum_l",0,"_idx",i ) << " + "// "//( \"0\" & " << join ( "sum_l",0,"_idx",i ) << ") + "
					<< join ( "c_l", i-1 ,"_idx",i-1 ) <<range (0,0) << ";" << endl ;
//					if ( i>1 )
//						vhdl << " + " << join ( "sum_l",i-1,"_idx",i-1 ) <<of ( cSize[i-1] );
//					vhdl<<";"<<endl;

					vhdl << tab << declare ( join ( "sum_l",i,"_idx",i ), cSize[i], true ) << " <= " << join ( "s_sum_l",i,"_idx",i ) <<range ( cSize[i]-1,0 ) << ";" << endl;
					vhdl << tab << declare ( join ( "c_l",i,"_idx",i ), 1, true )          << " <= " << join ( "s_sum_l",i,"_idx",i ) <<range ( cSize[i],cSize[i] ) << ";" << endl;
				}
				
				vhdl << tab << "R <= ";
				for ( int i=k-1; i >= 1; i-- ) {
					vhdl << join ( "sum_l",i,"_idx",i ) <<range ( cSize[i]-1,0 ) << " & ";
				}
				vhdl << "sum_l0_idx0" << range ( cSize[0]-1,0 ) <<";"<<endl;
				
				outDelayMap["R"] = target->adderDelay ( cSize[k-1] );
			} else {
				vhdl << tab << " R <= X + Y + Cin;" << endl;
				if ( alternativeSlackVersion == 1 )  {
					outDelayMap["R"] = target->adderDelay ( wIn );
				} else
					outDelayMap["R"] = getMaxInputDelays ( inputDelays ) + target->adderDelay ( wIn );
			}
		} else {
			vhdl << tab << " R <= X + Y + Cin;" << endl;
			outDelayMap["R"] = target->adderDelay ( wIn_ ) + getMaxInputDelays ( inputDelays );
		}
	}
	
	/**************************************************************************/
	IntAdderAlternative::~IntAdderAlternative() {
	}
	
	/**************************************************************************/
	int IntAdderAlternative::getLutCostAlternative ( Target* target, int wIn, map<string, double> inputDelays, bool srl ) {
		REPORT ( DEBUG, DEBUG_SEPARATOR );
		if ( getMaxInputDelays(inputDelays) == target->ffDelay() + target->localWireDelay() || getMaxInputDelays(inputDelays) == 0.0 ) {
			/* no input slack problem */
			int alpha, beta, k, cost;
			updateParameters ( target, alpha, beta, k );
			REPORT ( DEBUG, "LUT, Alternative, NO-SLACK: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			
			if (target->getVendor() == "Xilinx" && srl ) {
				if ( k == 1 ) {
					cost = wIn;
				} else if ( k <= 3 ) {
					cost = (k-1)*wIn -  alpha + k*(k-1)/2;
				} else { //k>=4
					/* more than 3 chunk splitting */
					cost = (4*k-10)*alpha + 3*beta + 3*k-7;
				}
			} else {
				cost =  2*wIn - alpha + 2*k-3;
			}
			REPORT ( DETAILED, "Selected: Alternative, NO-SLACK with LUT cost " << cost );
			return cost;
		} else {
			int version0, version1;
			int alpha, beta, k, kVersion0, kVersion1;
			
			/* Version 0: we try to adapt the architecture to the new slack */
			updateParameters ( target, inputDelays, alpha, beta, k );
			kVersion0 = k;
			REPORT ( DEBUG, "LUT, Alternative, SLACK, Version 0: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			
			if ( k>0 ){
				if (target->getVendor() == "Xilinx" && srl) {
					if ( k==1 ) {
						version0 = wIn;
					} else if ( k <= 3 ) {
						version0 = (k-1)*wIn -  alpha + k*(k-1)/2;
					} else { //k>=4
						/* more than 3 chunk splitting */
						version0 = (4*k-10)*alpha + 3*beta + 3*k-7;
					}
				} else {
					version0 = 2*wIn - alpha + 2*k - 3;
				}
			}else
				version0 = PINF; //infinity
				
			/* Version 1: we buffer the inputs and proceed */
			updateParameters ( target, alpha, beta, k );
			kVersion1 = k;
			REPORT ( DEBUG, "LUT, Alternative, SLACK, Version 1 (buffered inputs): alpha="<<alpha<<" beta="<<beta<<" k="<<k << " p="<<k+1 );
			
			if (target->getVendor() == "Xilinx" && srl) {
				if ( k==1 ) {
					version1 = wIn;
				} else if ( k <= 3 ) {
					version1 = (k-1)*wIn -  alpha + k*(k-1)/2;
				} else { //k>=4
					/* more than 3 chunk splitting */
					version1 = (4*k-10)*alpha + 3*beta + 3*k-7;
				}
			} else {
				version1 = 2*wIn - alpha + 2*k - 3;
			}
			
			REPORT ( DEBUG, "LUT, Alternative, SLACK, Version 0: " << version0 );
			REPORT ( DEBUG, "LUT, Alternative, SLACK, Version 1 (buffered inputs): " << version1 );
			
			if ( version0 == version1 ) {
				if ( kVersion0 <= kVersion1 ) {
					alternativeSlackVersion = 0;
					REPORT ( DETAILED, "Alternative SLACK, version is 0 with LUT cost " << version0 << " and k="<<kVersion0 );
					return version0;
				} else {
					alternativeSlackVersion = 1;
					REPORT ( DETAILED, "Alternative SLACK, version is 1 with LUT cost " << version1 << " and k="<<kVersion1 );
					return version1;
				}
			} else if ( version0 < version1 ) {
				alternativeSlackVersion = 0;
				REPORT ( DETAILED, "Alternative SLACK version is 0 with LUT cost " << version0 );
				return version0;
			} else {
				alternativeSlackVersion = 1;
				REPORT ( DETAILED, "Alternative SLACK version is 1 with LUT cost " << version1 );
				return version1;
			}
		}
		
		cerr << "Error in " <<  __FILE__ << "@" << __LINE__;
		exit ( -1 );
		return -1;
	}

	
	/**************************************************************************/
	int IntAdderAlternative::getRegCostAlternative ( Target* target, int wIn, map<string, double> inputDelays, bool srl ) {
		REPORT ( DEBUG, DEBUG_SEPARATOR );
		if ( getMaxInputDelays(inputDelays) == target->ffDelay() + target->localWireDelay() || getMaxInputDelays(inputDelays) == 0.0 ) {
			/* no input slack problem */
			int alpha, beta, k, cost;
			updateParameters ( target, alpha, beta, k );
			REPORT ( DEBUG, "REG, ALTERNATIVE, NO-SLACK: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			
			if ( k == 1 ) {
				cost = 0;
			} else {
				if (target->getVendor() == "Xilinx" && srl )
					if (k <= 3)
						cost = (k-1)*wIn + k*(k-1)/2;
					else
						cost = (4*k-8)*alpha + 2*beta + (k-1)*k/2;
				else
					cost = (k-1)*wIn + k*k-2*k+1;
			}
			
			REPORT ( DETAILED, "Selected: Alternative, NO-SLACK, with REG cost " << cost );
			return cost;
		} else {
			int version0, version1;
			int alpha, beta, k;
			/* Version 0 */
			updateParameters ( target, inputDelays, alpha, beta, k );
			REPORT ( DEBUG, "REG, Alternative, SLACK, Version 0: alpha="<<alpha<<" beta="<<beta<<" k="<<k << " p="<<k+1 );
			
			if ( k>0 ){
				if ( k==1 ) {
					version0 = 0;
				} else {
					if (target->getVendor() == "Xilinx" &&  srl )
						if (k <= 3)
							version0 = (k-1)*wIn + k*(k-1)/2;
						else
							version0 = (4*k-8)*alpha + 2*beta + 3*k-9;// (2*k-2)*alpha + beta + (k-1)*k/2;
					else
						version0 = (k-1)*wIn + k*k-2*k+1;
				}
			}else
				version0 = PINF; //infinity
					
			/* Version 1 */
			updateParameters ( target, alpha, beta, k );
			REPORT ( DEBUG, "REG, Alternative, SLACK, Version 1: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			
			if ( k==1 ) {
				version1 = 0;
			} else {
				if (target->getVendor() == "Xilinx" &&  srl )
					if (k <= 3)
						version1 = 2*wIn + 1 + (k-1)*wIn + k*(k-1)/2;
					else
						version1 = 2*wIn + 1 + (4*k-8)*alpha + 2*beta + 3*k-9;//(2*k-2)*alpha + beta + (k-1)*k/2;
				else
					version1 = 2*wIn + 1 + (k-1)*wIn + k*k-2*k+1;;
			}
			
			REPORT ( DETAILED, "REG, Alternative, Version 0: " << version0 );
			REPORT ( DETAILED, "REG, Alternative, Version 1 (buffered inputs): " << version1 );
			
			if ( version0 <= version1 ) {
				alternativeSlackVersion = 0;
				REPORT ( DETAILED, "Selected: Alternative slack version is 0 with cost " << version0 );
				return version0;
			} else {
				alternativeSlackVersion = 1;
				REPORT ( DETAILED, "Selected: Alternative slack version is 1 with cost " << version1 );
				return version1;
			}
		}
		cerr << "Error in " <<  __FILE__ << "@" << __LINE__;
		exit ( -1 );
		return -1;
	}
	
	/**************************************************************************/
	int IntAdderAlternative::getSliceCostAlternative ( Target* target, int wIn, map<string, double> inputDelays, bool srl ) {
		REPORT ( DEBUG, DEBUG_SEPARATOR );
		if ( getMaxInputDelays(inputDelays) == target->ffDelay() + target->localWireDelay() || getMaxInputDelays(inputDelays) == 0.0 ) {
			/* no input slack problem */
			int alpha, beta, k, cost;
			updateParameters ( target, alpha, beta, k );
			REPORT ( DEBUG, "SLICE, ALTERNATIVE, NO-SLACK: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			
			if ( k == 1 ){
				cost = wIn ;
			} else if (target->getVendor() == "Xilinx" &&  srl ) {
				if (k<=3)
					cost = (k-1)*wIn + beta + (k-1)*k/2;
				else
					cost = (4*k-8)*alpha + 3*beta + (k-1)*k/2;
			} else {
				cost = (k-1)*wIn + beta + k*k-2*k+1;
			}
			
			REPORT ( DETAILED, "Selected: Alternative, NO-SLACK with SLICE cost " << cost );
			return cost;
		} else {
			int version0, version1;
			int alpha, beta, k;
			updateParameters ( target, inputDelays, alpha, beta, k );
			REPORT ( DEBUG, "SLICE, Alternative, SLACK, Version 0: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			
			if ( k>0 ) {
				if ( k==1 ) {
					version0 = wIn;
				} else if (target->getVendor() == "Xilinx" && srl ) {
					if (k<=3)
						version0 = (k-1)*wIn + beta + (k-1)*k/2;
					else
						version0 = (4*k-8)*alpha + 3*beta + (k-1)*k/2;
				} else {
					version0 = (k-1)*wIn + beta + k*k-2*k+1;
				}
			} else
				version0 = PINF; //infinity
				
			updateParameters (target, alpha, beta, k );
			REPORT ( DEBUG, "SLICE, Alternative, SLACK, Version 1: alpha="<<alpha<<" beta="<<beta<<" k="<<k );
			
			if ( k==1 ) {
				version1 = 2*wIn + 1 + wIn;
			} else if (target->getVendor() == "Xilinx" &&  srl ) {
				if (k<=3){
					version1 = 2*wIn + 1 + (k-1)*wIn + beta + (k-1)*k/2;
				} else {
					version1 = 2*wIn + 1 + (4*k-8)*alpha + 3*beta + (k-1)*k/2;
				}
			} else {
				version1 = 2*wIn + 1 + (k-1)*wIn + beta + k*k-2*k+1;
			}
			REPORT ( DETAILED, "SLICE, Alternative, SLACK, Version 0: " << version0 );
			REPORT ( DETAILED, "SLICE, Alternative, SLACK, Version 1: " << version1 );
			
			if ( version0 <= version1 ) {
				alternativeSlackVersion = 0;
				REPORT ( DETAILED, "Alternative SLACK version is 0 with SLICE cost " << version0 );
				return version0;
			} else {
				alternativeSlackVersion = 1;
				REPORT ( DETAILED, "Alternative SLACK version is 1 with SLICE cost " << version1 );
				return version1;
			}
		}
		cerr << "Error in " <<  __FILE__ << "@" << __LINE__;
		exit ( -1 );
		return -1;
	}
}


