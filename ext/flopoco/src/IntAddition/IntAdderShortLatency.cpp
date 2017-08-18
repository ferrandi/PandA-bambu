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
#include "IntAdderShortLatency.hpp"

using namespace std;

namespace flopoco {
	
	IntAdderShortLatency::IntAdderShortLatency ( Target* target, int wIn, map<string, double> inputDelays, int optimizeType, bool srl) :
	IntAdder ( target, wIn, inputDelays, true ), wIn_ ( wIn ), shortLatencyInputRegister ( 0 ) {
		srcFileName="IntAdderShortLatency";
		ostringstream name;
		
		setCopyrightString ( "Bogdan Pasca, Florent de Dinechin (2008-2010)" );
		name << "IntAdderShortLatency_" << wIn_<<"_f"<<target->frequencyMHz()<<"_uid"<<getNewUId();
		setName ( name.str() );
		
		// Set up the IO signals
		addInput ( "X"  , wIn_, true );
		addInput ( "Y"  , wIn_, true );
		addInput( "Cin");
		addOutput ( "R"  , wIn_, 1 , true );
		
		vhdl << tab << "--ShortLatency"<<endl;
		
		switch (optimizeType) {
			case 0:  cost = getLutCostShortLatency(target,wIn, inputDelays, srl); break;
			case 1:  cost = getRegCostShortLatency(target,wIn, inputDelays, srl); break;
			case 2:  cost = getSliceCostShortLatency(target,wIn, inputDelays, srl); break;
			default: cost = getSliceCostShortLatency(target,wIn, inputDelays, srl); break;
		}
		
		if ( isSequential() ) {
			
			objectivePeriod	 = 1 / target->frequency();
			k = shortLatencyKValue;
			
			ostringstream verb;
			verb << "Implementing ShortLatency with chunks: ";
			for ( int i=k-1; i>=0; i-- )
				verb << " " << cSize[i];
			
			REPORT ( DETAILED, verb.str() );
			
			//split	 the inputs
			for ( int i=0;i<2;i++ )
				for ( int j=0; j<k; j++ ) {
					ostringstream name;
					//the naming standard: sX j _ i _ l
					//j = the chunk index i is the input index and l is the current level
					name << "sX"<<j<<"_"<<i<<"_l"<<0;
					int low=0, high=0;
					for ( int k=0;k<=j;k++ )
						high+=cSize[k];
					for ( int k=0;k<=j-1;k++ )
						low+=cSize[k];
					vhdl << tab << declare ( name.str(),cSize[j]+1 ) << " <=  \"0\" & "<< ( i==0?"X":"Y" ) <<range ( high-1,low ) <<";"<<endl;
				}
				vhdl << tab << declare( "scIn") << " <= Cin;"<<endl;
				
				//				if (shortLatencyInputRegister ==1)
				//					nextCycle();///////////////////
				
				int l=1;
				for ( int j=0; j<k; j++ ) {
					ostringstream dnameZero, dnameOne, uname1, uname2, dnameCin;
					dnameZero << "sX"<<j<<"_0_l"<<l<<"_Zero";
					dnameOne  << "sX"<<j<<"_0_l"<<l<<"_One";
					dnameCin  << "sX"<<j<<"_0_l"<<l<<"_Cin";
					
					uname1 << "sX"<<j<<"_0_l"<<l-1;
					uname2 << "sX"<<j<<"_1_l"<<l-1;
					
					#ifdef XILINX_OPTIMIZATION
					// the xst synthetsies x+y and x+y+1 slower if this optimization is not used
					bool pipe = target->isPipelined();
					target->setNotPipelined();
					

					
					vhdl << tab << declare ( uname1.str() +"ext", cSize[j]+1 ) << " <= \"0\" & "  <<  uname1.str() << range ( cSize[j]-1,0 ) << ";" << endl;
					vhdl << tab << declare ( uname2.str() +"ext", cSize[j]+1 ) << " <= \"0\" & "  <<  uname2.str() << range ( cSize[j]-1,0 ) << ";" << endl;
					
					if ( j>0 ) { //for all chunks greater than zero we perform this additions
						IntAdder *adder = new IntAdder( target, cSize[j]+1, emptyDelayMap, optimizeType, srl );
						ostringstream a;
						a.str ( "" );
						
/*						bool found = false;
						for ( unsigned kk=0; kk<oplist.size(); kk++ ) {
							if ( ( oplist[kk]->getName() ).compare ( adder->getName() ) ==0 ) {
								REPORT ( 3, "found in opList ... not adding" );
								found = true;
							}
						}
						if ( found == false )						{
							REPORT ( 3, "Not found in list, adding " << adder->getName() );
							oplist.push_back ( adder );
						}*/
						
						inPortMapCst ( adder, "X", uname1.str() +"ext" );
						inPortMapCst ( adder, "Y", uname2.str() +"ext" );
						inPortMapCst ( adder, "Cin", "'0'" );
						outPortMap ( adder, "R", dnameZero.str() );
						a<< "Adder" << cSize[j]+1 << "Zero" << j;
						vhdl << instance ( adder, a.str() );
						
						//						if (j<k-1){
							
							inPortMapCst ( adder, "X", uname1.str() +"ext" );
							inPortMapCst ( adder, "Y", uname2.str() +"ext" );
							inPortMapCst ( adder, "Cin", "'1'" );
							outPortMap ( adder, "R", dnameOne.str() );
							a.str ( "" );
							a<< "Adder" << cSize[j]+1 << "One" << j;
							vhdl << instance ( adder, a.str() );
							//						}
					} else {
						vhdl << tab << "-- the carry resulting from the addition of the chunk + Cin is obtained directly" << endl;
						vhdl << tab << declare ( dnameCin.str(),cSize[j]+1 ) << "  <= (\"0\" & "<< uname1.str() <<range ( cSize[j]-1,0 ) <<") +  (\"0\" & "<< uname2.str() <<range ( cSize[j]-1,0 ) <<") + scIn;"<<endl;
					}
					
					if ( pipe )
						target->setPipelined();
					else
						target->setNotPipelined();
					
					#else
					if ( j>0 ) { //for all chunks greater than zero we perform this additions
						vhdl << tab << declare ( dnameZero.str(),cSize[j]+1 ) << " <= (\"0\" & "<< uname1.str() <<range ( cSize[j]-1,0 ) <<") +  (\"0\" & "<< uname2.str() <<range ( cSize[j]-1,0 ) <<");"<<endl;
						if ( j<k-1 )
							vhdl << tab << declare ( dnameOne.str(),cSize[j]+1 ) << "  <= (\"0\" & "<< uname1.str() <<range ( cSize[j]-1,0 ) <<") +  (\"0\" & "<< uname2.str() <<range ( cSize[j]-1,0 ) <<") + '1';"<<endl;
					} else {
						vhdl << tab << "-- the carry resulting from the addition of the chunk + Cin is obtained directly" << endl;
						vhdl << tab << declare ( dnameCin.str(),cSize[j]+1 ) << "  <= (\"0\" & "<< uname1.str() <<range ( cSize[j]-1,0 ) <<") +  (\"0\" & "<< uname2.str() <<range ( cSize[j]-1,0 ) <<") + scIn;"<<endl;
					}
					#endif
				}
				
				if ( k >2 ) {
					vhdl << tab <<"--form the two carry string"<<endl;
					vhdl << tab << declare ( "carryStringZero",2* ( k-2 ) ) << " <= ";
					for ( int i=2* ( k-2 )-1; i>=0; i-- ) {
						ostringstream dnameZero;
						if ( i%2==0 ) {
							dnameZero << "sX"<< ( i/2 ) +1<<"_0_l"<<l<<"_Zero";
							vhdl << " " << dnameZero.str() <<"(" << cSize[ ( i/2 ) +1] << ")";
						} else
							vhdl << " \"0\" ";
						if ( i>0 ) vhdl << " & ";
						else     vhdl << " ; ";
					}
					vhdl << endl;
					vhdl << tab << declare ( "carryStringOne",2* ( k-2 ) ) << "  <= ";
					for ( int i=2* ( k-2 )-1; i>=0; i-- ) {
						ostringstream dnameOne;
						if ( i%2==0 ) {
							dnameOne << "sX"<< ( i/2 ) +1<<"_0_l"<<l<<"_One";
							vhdl << " " << dnameOne.str() <<"(" << cSize[ ( i/2 ) +1] << ") ";
						} else
							vhdl << " \"1\" ";
						if ( i>0 ) vhdl << " & ";
						else     vhdl << " ; ";
					}
					vhdl << endl;
					
					if ( shortLatencyVersion > 1 )
						nextCycle();/////////////////////
						
						vhdl << tab << "--perform the short carry additions" << endl; //TODO: PIPELINE ADDITION
						ostringstream unameCin;
					unameCin  << "sX"<<0<<"_0_l"<<l<<"_Cin";
					vhdl << tab << declare ( "rawCarrySum",2* ( k-2 ) ) << " <= carryStringOne + carryStringZero + " << unameCin.str() << "(" << cSize[0] << ") ;" << endl;
					
					if ( shortLatencyVersion > 2 )
						nextCycle();/////////////////////
				}
				
				vhdl << tab <<"--get the final pipe results"<<endl;
				for ( int i=0; i<k; i++ ) {
					ostringstream unameZero, unameOne, unameCin;
					unameZero << "sX"<<i<<"_0_l"<<l<<"_Zero";
					unameOne  << "sX"<<i<<"_0_l"<<l<<"_One";
					unameCin  << "sX"<<0<<"_0_l"<<l<<"_Cin";
					if ( i==0 ) vhdl << tab << declare ( join ( "res",i ),cSize[i],true ) << " <= " << unameCin.str() << range ( cSize[0]-1,0 ) <<  ";" << endl;
					else {
						//						if (i==1) vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << unameZero.str() << range(cSize[i]-1,0) << " + " << unameCin.str() << "(" << cSize[0] << ");"<<endl;
						//						else      vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << unameZero.str() << range(cSize[i]-1,0) << " + not(rawCarrySum("<<2*(i-2)+1<<"));"<<endl;
						if ( i==1 ) vhdl << tab << declare ( join ( "res",i ),cSize[i],true ) << " <= " << unameZero.str() << range ( cSize[i]-1,0 ) << " when " << unameCin.str() << "(" << cSize[0] << ")='0'"
							<< " else " << unameOne.str() << range ( cSize[i]-1,0 ) << ";"<<endl;
						else      vhdl << tab << declare ( join ( "res",i ),cSize[i],true ) << " <= " << unameZero.str() << range ( cSize[i]-1,0 ) << " when rawCarrySum("<<2* ( i-2 ) +1<<")='1'"
							<< " else " << unameOne.str() << range ( cSize[i]-1,0 ) << ";"<<endl;
					}
				}
				
				vhdl << tab << "R <= ";
				for ( int i=k-1; i>=0; i-- ) {
					vhdl << join ( "res",i );
					if ( i > 0 ) vhdl << " & ";
				}
				vhdl << ";" <<endl;
				outDelayMap["R"] = target->adderDelay ( cSize[k-1] );
				
				REPORT ( DEBUG, " FINISHED SHORT-LATENCY IMPLEMENTATION" );
		} else {
			vhdl << tab << " R <= X + Y + Cin;" << endl;
			outDelayMap["R"] = target->adderDelay ( wIn_ ) + getMaxInputDelays ( inputDelays );
		}
		
		REPORT ( DEBUG, "OutDelay for R is " << outDelayMap["R"] );
		
	}
	
	/**************************************************************************/
	IntAdderShortLatency::~IntAdderShortLatency() {
	}
	
	/**************************************************************************/
	int IntAdderShortLatency::getLutCostShortLatency ( Target* target, int wIn, map<string, double> inputDelays, bool srl ) {
		REPORT ( DEBUG, DEBUG_SEPARATOR );
		tryOptimizedChunkSplittingShortLatency ( target, wIn , k );
		REPORT ( DEBUG, "LUT, Short-Latency: k="<<k );
		
		int cost;
		if ( getMaxInputDelays ( inputDelays ) == 0 ) {
			/* no input slack problem */
			if ( shortLatencyVersion == 0 ) {
				cost = cSize[0] + 3*cSize[1];
			} else if ( shortLatencyVersion == 1 ) {
				cost = 3*wIn - 2*cSize[0] + 2* ( k-2 );
			} else if ( shortLatencyVersion == 2 ) {
				/* the algorithm found a good way to split inputs and save 1 pipeline level */
				cost = ( 3*wIn - 2*cSize[0] + 2* ( k-2 ) );
			} else if ( shortLatencyVersion == 3 ) {
				if ( srl )
					cost = 5*wIn - 3*cSize[0] + 2* ( k-2 ) + 1;
				else
					cost = 3*wIn - 2*cSize[0] + 2* ( k-2 );
			} else
				cost = PINF; //+inf
				
				REPORT ( DETAILED, "Selected: Short-Latency, with LUT cost " << cost );
			return cost;
			
		} else {
			//TODO No implementation for now, so cost is +inf
			return PINF;
		}
		cerr << "Error in " <<  __FILE__ << "@" << __LINE__;
		exit ( -1 );
		return -1;
	}
	

	/**************************************************************************/
	int IntAdderShortLatency::getRegCostShortLatency ( Target* target, int wIn, map<string, double> inputDelays, bool srl ) {
		REPORT ( DEBUG, DEBUG_SEPARATOR );
		tryOptimizedChunkSplittingShortLatency ( target, wIn , k );
		REPORT ( DEBUG, "REG, Short-Latency:  k="<<k );
		int cost;
		if ( getMaxInputDelays ( inputDelays ) == 0 ) {
			/* no input slack problem */
			if ( ( shortLatencyVersion == 0 ) || ( shortLatencyVersion == 1 ) ) {
				cost = 0;
			} else if ( shortLatencyVersion == 2 ) {
				cost = 2*wIn - cSize[0] + 2* ( k-2 ) + 1;
			} else if ( shortLatencyVersion == 3 ) {
				if ( srl )
					cost = 2*wIn - cSize[0] + 3* ( k-2 ) + 1;
				else
					cost = 4*wIn - 2*cSize[0] + 3*k - 4;
			} else
				cost = PINF;
			
			REPORT ( DETAILED, "Selected: Short-Latency with REG cost " << cost );
			return cost;
			
		} else {
			/* TODO for slack */
			return PINF;
		}
		cerr << "Error in " <<  __FILE__ << "@" << __LINE__;
		exit ( -1 );
		return -1;
	}
	
	/**************************************************************************/
	int IntAdderShortLatency::getSliceCostShortLatency ( Target* target, int wIn, map<string, double> inputDelays, bool srl ) {
		shortLatencyInputRegister = -1;
		REPORT ( DEBUG, DEBUG_SEPARATOR );
		int cost;
		if ( getMaxInputDelays ( inputDelays ) == 0 ) {  /* no input slack problem */
			tryOptimizedChunkSplittingShortLatency ( target, wIn , k );
			REPORT ( DEBUG, "SLICE, Short-Latency:  k="<<k );
			
			switch ( shortLatencyVersion ) {
				case  0: cost = int ( ceil ( double ( cSize[0] + 3*cSize[1] ) /double ( 2 ) ) );
				break;
				case  1: cost = int ( ceil ( double ( 3*wIn - 2*cSize[0] + 2* ( k-2 ) ) /double ( 2 ) ) );
				break;
				case  2: cost = int ( ceil ( double ( 3*wIn - 2*cSize[0] + 4* ( k-2 ) + 1 ) /double ( 2 ) ) );
				break;
				case  3: cost = int ( ceil ( double ( 5*wIn - 3*cSize[0] + 5* ( k-2 ) + 1 ) / double ( 2 ) ) );
				break;
				default: cost = PINF;
			}
			REPORT ( DETAILED, "Selected: Short-Latency with SLICE cost " << cost );
			return cost;
		} else {
			tryOptimizedChunkSplittingShortLatency ( target, inputDelays, wIn , k );
			REPORT ( DEBUG, "SLICE, Short-Latency:  k="<<k );
			
			switch ( shortLatencyVersion ) {
				case  0: cost = int ( ceil ( double ( cSize[0] + 3*cSize[1] ) /double ( 2 ) ) );
				break;
				case  1: cost = int ( ceil ( double ( 3*wIn - 2*cSize[0] + 2* ( k-2 ) ) /double ( 2 ) ) );
				break;
				case  2: cost = int ( ceil ( double ( 3*wIn - 2*cSize[0] + 4* ( k-2 ) + 1 ) /double ( 2 ) ) );
				break;
				case  3: cost = int ( ceil ( double ( 5*wIn - 3*cSize[0] + 5* ( k-2 ) + 1 ) / double ( 2 ) ) );
				break;
				default: cost = PINF;
			}
			//			shortLatencyInputRegister = 1;
			/* TODO */
			return PINF;
			//			return cost;
		}
		cerr << "Error in " <<  __FILE__ << "@" << __LINE__;
		exit ( -1 );
		return -1;
	}
	
//	/**************************************************************************/
//	void IntAdderShortLatency::updateParameters ( Target* target, int &alpha, int &beta, int &k ) {
//		
//		target->suggestSubaddSize ( alpha , wIn_ ); /* chunk size */
//		if ( wIn_ == alpha ) {
//			/* addition requires one chunk */
//			beta = 0;
//			k    = 1;
//		} else {
//			beta = ( wIn_ % alpha == 0 ? alpha : wIn_ % alpha );
//			k    = ( wIn_ % alpha == 0 ? wIn_ / alpha : ceil ( double ( wIn_ ) / double ( alpha ) ) );
//		}
//	}
//	
//	/**************************************************************************/
//	void IntAdderShortLatency::updateParameters ( Target* target, map<string, double> inputDelays, int &alpha, int &beta, int &gamma, int &k ) {
//		
//		int typeOfChunks = 1;
//		bool status = target->suggestSlackSubaddSize ( gamma , wIn_, getMaxInputDelays ( inputDelays ) ); // the first chunk size
//		REPORT ( DEBUG, "suggestSlackSubaddSize returns gamma="<<gamma<<" with status:"<< ( status?"true":"false" ) );
//		
//		if ( ! status ) {
//			k=-1;
//			alpha=0;
//			beta=0;
//			gamma=0;
//		} else
//			if ( wIn_ - gamma > 0 ) { //more than 1 chunk
//				target->suggestSubaddSize ( alpha, wIn_-gamma );
//				if ( wIn_-gamma == alpha )
//					typeOfChunks++; //only two types of chunks
//					else
//						typeOfChunks+=2; //three types of chunks
//						
//						REPORT ( DETAILED, "Types of chunks = " << typeOfChunks );
//					
//					if ( typeOfChunks==3 )
//						beta = ( ( wIn_-gamma ) % alpha == 0 ? alpha : ( wIn_-gamma ) % alpha );
//					else
//						beta = alpha;
//					
//					
//					if ( typeOfChunks==2 )
//						k = 2;
//					else
//						k = 2 +   int ( ceil ( double ( wIn_ - beta - gamma ) / double ( alpha ) ) );
//					
//					
//			} else {
//				alpha = 0;
//				beta = 0;
//				k=1;
//			}
//	}
//	
//	/**************************************************************************/
//	void IntAdderShortLatency::updateParameters ( Target* target, map<string, double> inputDelays, int &alpha, int &beta, int &k ) {
//		bool status = target->suggestSlackSubaddSize ( alpha , wIn_,  getMaxInputDelays ( inputDelays ) ); /* chunk size */
//		if ( !status ) {
//			k=-1;
//			alpha=0;
//			beta=0;
//		} else
//			if ( wIn_ == alpha ) {
//				/* addition requires one chunk */
//				beta = 0;
//				k    = 1;
//			} else {
//				beta = ( wIn_ % alpha == 0 ? alpha : wIn_ % alpha );
//				k    = ( wIn_ % alpha == 0 ? wIn_ / alpha : ceil ( double ( wIn_ ) / double ( alpha ) ) );
//			}
//			
//	}
//	
	/**************************************************************************/
	void IntAdderShortLatency::tryOptimizedChunkSplittingShortLatency ( Target* target, int wIn, int &k ) {
		cSize = new int[2000];
		for ( int u=0; u<2000; u++ )
			cSize[u] = -1;
		
		int alpha0;
		double tSelect = target->lutDelay() + target->localWireDelay();
		
		double k1,k2;
		target->getAdderParameters ( k1,k2,wIn );  /* adder delay is modeled as d = k1 + (w-1)k2 */
		target->suggestSlackSubaddSize ( alpha0, wIn, tSelect );
		int alpha;
		target->suggestSubaddSize ( alpha,wIn );
		
		double C = ( ( 1.0 / target->frequency() ) - tSelect - 2*k1 + 2*k2 ) /k2;
		int U = int ( floor ( C ) );
		
		REPORT ( DEBUG, "U="<<U<<" C="<<C );
		int maxW;
		if ( U < 0 )
			U = 0;
		
		if ( U >= 0 )
			if ( U % 2 ==0 )
				maxW = 2*alpha0 + U* ( U+2 ) /4;
			else
				maxW = 2*alpha0 + ( U+1 ) * ( U+1 ) /4;
			else
				maxW = -1;
			
			REPORT ( DEBUG, "alpha is " << alpha );
		REPORT ( DEBUG, "Max addition size for latency 0, two chunk architecture:" << 2*alpha0 );
		REPORT ( DEBUG, "Max addition size for latency 0 is:" << maxW );
		
		double C2 = ( ( 1.0 / target->frequency() ) - tSelect - k1 + k2 ) / ( 2*k2 );
		int U2 = int ( floor ( C2 ) );
		
		double C3 = ( ( 1.0 / target->frequency() ) - k1 + k2 ) / ( 2*k2 );
		int U3 = int ( floor ( C3 ) );
		
		REPORT ( DEBUG, "Max addition size for latency 1 is:" << ( U2+2 ) *alpha0 );
		REPORT ( DEBUG, "Max addition size for latency 2 is:" << ( U3+2 ) *alpha );
		
		if ( wIn <= 2*alpha0 ) {
			// TWO CHUNK ARCHITECTURE
			cSize[0]= int ( floor ( wIn/2 ) );
			cSize[1]= wIn - cSize[0];
			REPORT ( DEBUG, " Chunk sizes are: " << cSize[0] << " " << cSize[1] );
			shortLatencyVersion = 0;
			k = 2;
		} else if ( wIn <= maxW ) {
			// LATENCY 0 Architecture
			cSize[0]=alpha0;
			cSize[1]=alpha0;
			int tmpWIn = wIn - 2*alpha0;
			int i=2;
			while ( tmpWIn > 0 ) {
				if ( tmpWIn - ( U-2* ( i-2 ) ) >= 0 ) {
					cSize[i]=U-2* ( i-2 );
					tmpWIn -= cSize[i];
				} else {
					cSize[i] = tmpWIn;
					tmpWIn -= cSize[i];
				}
				i++;
			}
			k=i;
			ostringstream tmp;
			for ( int kk=i-1; kk>=0;kk-- ) {
				tmp <<  cSize[kk] << " ";
			}
			REPORT ( DEBUG, " Chunks " << k <<"  Sizes: " << tmp.str() );
			shortLatencyVersion = 1;
		} else if ( ( wIn > maxW ) && ( wIn <= ( U2+2 ) *alpha0 ) ) {
			//LATENCY 1 architecture
			if ( wIn % alpha0 == 0 )
				k = wIn / alpha0;
			else
				k = int ( ceil ( double ( wIn ) / double ( alpha0 ) ) );
			
			for ( int p=0; p<k-1; p++ )
				cSize[p] = alpha0;
			
			cSize[k-1] = ( wIn % alpha0 == 0 ? alpha0 : wIn % alpha0 );
			
			ostringstream tmp;
			for ( int kk=k-1; kk>=0;kk-- ) {
				tmp <<  cSize[kk] << " ";
			}
			REPORT ( DEBUG, " Chunk " << k<< "  Sizes: " << tmp.str() );
			shortLatencyVersion = 2;
		}  else if ( ( wIn > ( U2+2 ) *alpha0 ) && ( wIn <= ( U3+2 ) *alpha ) ) {
			if ( wIn % alpha == 0 )
				k = wIn / alpha;
			else
				k = int ( ceil ( double ( wIn ) / double ( alpha ) ) );
			
			for ( int p=0; p<k-1; p++ )
				cSize[p] = alpha;
			
			cSize[k-1] = ( wIn % alpha == 0 ? alpha : wIn % alpha );
			
			ostringstream tmp;
			for ( int kk=k-1; kk>=0;kk-- ) {
				tmp <<  cSize[kk] << " ";
			}
			REPORT ( DEBUG, " Chunk " << k<< "  Sizes: " << tmp.str() );
			shortLatencyVersion = 3;
		} else if ( wIn > ( U3+2 ) *alpha ) {
			//LATENCY 2+ architecture
			if ( wIn % alpha == 0 )
				k = wIn / alpha;
			else
				k = int ( ceil ( double ( wIn ) / double ( alpha ) ) );
			
			for ( int p=0; p<k-1; p++ )
				cSize[p] = alpha;
			
			cSize[k-1] = ( wIn % alpha == 0 ? alpha : wIn % alpha );
			
			ostringstream tmp;
			for ( int kk=k-1; kk>=0;kk-- ) {
				tmp <<  cSize[kk] << " ";
			}
			REPORT ( DEBUG, " Chunk " << k<< "  Sizes: " << tmp.str() );
			shortLatencyVersion = 4;
		}
		
		REPORT ( DEBUG, "Selected Short-Latency Version is " << shortLatencyVersion );
		shortLatencyKValue = k;
	}
	
	/**************************************************************************/
	void IntAdderShortLatency::tryOptimizedChunkSplittingShortLatency ( Target* target, map<string, double> inputDelays, int wIn, int &k ) {
		cSize = new int[2000];
		for ( int u=0; u<2000; u++ )
			cSize[u] = -1;
		
		int alpha0;
		double tSelect = target->lutDelay() + target->localWireDelay();
		
		double T0 = ( 1.0 / target->frequency() ) - getMaxInputDelays ( inputDelays );
		//		double Tf = (1.0 / target->frequency());
		
		double k1,k2;
		target->getAdderParameters ( k1,k2,wIn ); /* adder delay is modeled as d = k1 + (w-1)k2 */
		target->suggestSlackSubaddSize ( alpha0, wIn, tSelect + getMaxInputDelays ( inputDelays ) );
		
		int alpha;
		target->suggestSubaddSize ( alpha,wIn );
		
		double C = ( T0 - tSelect - 2*k1 + 2*k2 ) /k2;
		int U = int ( floor ( C ) );
		
		REPORT ( DEBUG, "U="<<U<<" C="<<C );
		int maxW;
		if ( U < 0 )
			U = 0;
		
		if ( U >= 0 )
			if ( U % 2 ==0 )
				maxW = 2*alpha0 + U* ( U+2 ) /4;
			else
				maxW = 2*alpha0 + ( U+1 ) * ( U+1 ) /4;
			else
				maxW = -1;
			
			REPORT ( DEBUG, "alpha is " << alpha );
		REPORT ( DEBUG, "Max addition size for latency 0, two chunk architecture:" << 2*alpha0 );
		REPORT ( DEBUG, "Max addition size for latency 0 is:" << maxW );
		
		double C2 = ( ( 1.0 / target->frequency() ) - tSelect - k1 + k2 ) / ( 2*k2 );
		int U2 = int ( floor ( C2 ) );
		
		double C3 = ( ( 1.0 / target->frequency() ) - k1 + k2 ) / ( 2*k2 );
		int U3 = int ( floor ( C3 ) );
		
		REPORT ( DEBUG, "Max addition size for latency 1 is:" << ( U2+2 ) *alpha0 );
		REPORT ( DEBUG, "Max addition size for latency 2 is:" << ( U3+2 ) *alpha );
		
		if ( wIn <= 2*alpha0 ) {
			// TWO CHUNK ARCHITECTURE
			cSize[0]= int ( floor ( wIn/2 ) );
			cSize[1]= wIn - cSize[0];
			REPORT ( DEBUG, " Chunk sizes are: " << cSize[0] << " " << cSize[1] );
			shortLatencyVersion = 0;
			k = 2;
		} else if ( wIn <= maxW ) {
			// LATENCY 0 Architecture
			cSize[0]=alpha0;
			cSize[1]=alpha0;
			int tmpWIn = wIn - 2*alpha0;
			int i=2;
			while ( tmpWIn > 0 ) {
				if ( tmpWIn - ( U-2* ( i-2 ) ) >= 0 ) {
					cSize[i]=U-2* ( i-2 );
					tmpWIn -= cSize[i];
				} else {
					cSize[i] = tmpWIn;
					tmpWIn -= cSize[i];
				}
				i++;
			}
			k=i;
			ostringstream tmp;
			for ( int kk=i-1; kk>=0;kk-- ) {
				tmp <<  cSize[kk] << " ";
			}
			REPORT ( DEBUG, " Chunks " << k <<"  Sizes: " << tmp.str() );
			shortLatencyVersion = 1;
		} else if ( ( wIn > maxW ) && ( wIn <= ( U2+2 ) *alpha0 ) ) {
			//LATENCY 1 architecture
			if ( wIn % alpha0 == 0 )
				k = wIn / alpha0;
			else
				k = int ( ceil ( double ( wIn ) / double ( alpha0 ) ) );
			
			for ( int p=0; p<k-1; p++ )
				cSize[p] = alpha0;
			
			cSize[k-1] = ( wIn % alpha0 == 0 ? alpha0 : wIn % alpha0 );
			
			ostringstream tmp;
			for ( int kk=k-1; kk>=0;kk-- ) {
				tmp <<  cSize[kk] << " ";
			}
			REPORT ( DEBUG, " Chunk " << k<< "  Sizes: " << tmp.str() );
			shortLatencyVersion = 2;
		}  else if ( ( wIn > ( U2+2 ) *alpha0 ) && ( wIn <= ( U3+2 ) *alpha ) ) {
			if ( wIn % alpha == 0 )
				k = wIn / alpha;
			else
				k = int ( ceil ( double ( wIn ) / double ( alpha ) ) );
			
			for ( int p=0; p<k-1; p++ )
				cSize[p] = alpha;
			
			cSize[k-1] = ( wIn % alpha == 0 ? alpha : wIn % alpha );
			
			ostringstream tmp;
			for ( int kk=k-1; kk>=0;kk-- ) {
				tmp <<  cSize[kk] << " ";
			}
			REPORT ( DEBUG, " Chunk " << k<< "  Sizes: " << tmp.str() );
			shortLatencyVersion = 3;
		} else if ( wIn > ( U3+2 ) *alpha ) {
			//LATENCY 2+ architecture
			if ( wIn % alpha == 0 )
				k = wIn / alpha;
			else
				k = int ( ceil ( double ( wIn ) / double ( alpha ) ) );
			
			for ( int p=0; p<k-1; p++ )
				cSize[p] = alpha;
			
			cSize[k-1] = ( wIn % alpha == 0 ? alpha : wIn % alpha );
			
			ostringstream tmp;
			for ( int kk=k-1; kk>=0;kk-- ) {
				tmp <<  cSize[kk] << " ";
			}
			REPORT ( DEBUG, " Chunk " << k<< "  Sizes: " << tmp.str() );
			shortLatencyVersion = 4;
		}
		
		REPORT ( DEBUG, "Selected Short-Latency Version is " << shortLatencyVersion );
		shortLatencyKValue = k;
	}
	
	
	
	/******************************************************************************/
	void IntAdderShortLatency::emulate ( TestCase* tc ) {
		mpz_class svX = tc->getInputValue ( "X" );
		mpz_class svY = tc->getInputValue ( "Y" );
		mpz_class svC = tc->getInputValue ( "Cin" );
		
		mpz_class svR = svX + svY + svC;
		// Don't allow overflow
		mpz_clrbit ( svR.get_mpz_t(),wIn_ );
		
		tc->addExpectedOutput ( "R", svR );
	}
	
	
}


