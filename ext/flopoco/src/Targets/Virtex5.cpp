/*
  A model of Virtex-5 FPGA  (LX, speedgrade -3, e.g. xc5vlx30-3) 

  Author : Florent de Dinechin, Sebastian Banescu, Bogdan Pasca
  
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.

*/
#include "Virtex5.hpp"
#include <iostream>
#include <sstream>
#include "../utils.hpp"


namespace flopoco{
	
	double Virtex5::adderDelay(int size) {
		return lut2_ + muxcyStoO_ + double(size-1)*muxcyCINtoO_ + xorcyCintoO_ ; 
	};
	
	void Virtex5::getAdderParameters(double &k1, double &k2, int size){
		k1 = lut2_ + muxcyStoO_ + xorcyCintoO_;
		k2 = muxcyCINtoO_;
	}
	
	double Virtex5::eqComparatorDelay(int size){
		return lut2_ + muxcyStoO_ + double(ceil(size/2.0)-1)*muxcyCINtoO_; 
	}
	
	double Virtex5::eqConstComparatorDelay(int size){
		return lut2_ + muxcyStoO_ + double((size-1)/lutInputs_+1)*muxcyCINtoO_; 
	}
	
	double Virtex5::ffDelay() {
		return fdCtoQ_ + ffd_; 
	};
	
	double Virtex5::carryPropagateDelay() {
		return  fastcarryDelay_; 
	};
	
	double Virtex5::localWireDelay(int fanout){
		return  elemWireDelay_*(1+double(fanout)/50.0) ;
	};
	
	double Virtex5::distantWireDelay(int n){
		return n*elemWireDelay_;
	};
	
	double Virtex5::lutDelay(){
		return lutDelay_;
	};
	
	long Virtex5::sizeOfMemoryBlock()
	{
		return sizeOfBlock_;	
	};
	
	
	DSP* Virtex5::createDSP() 
	{
		int x, y;
		getDSPWidths(x, y);
		
		/* create DSP block with constant shift of 17
		* and a maxium unsigned multiplier width (17x17) for this target
		*/
		DSP* dsp_ = new DSP(dspFixedShift_, x, y);
		
		return dsp_;
	};
	
	bool Virtex5::suggestSubmultSize(int &x, int &y, int wInX, int wInY){
		
		getDSPWidths(x, y);
		
		//	//try the two possible chunk splittings
		//	int score1 = int(ceil((double(wInX)/double(x)))+ceil((double(wInY)/double(y))));
		//	int score2 = int(ceil((double(wInY)/double(x)))+ceil((double(wInX)/double(y))));
		//	
		//	if (score2 < score1)
		//		getDSPWidths(y,x);		
		
		if (wInX <= x)
			x = wInX;
		
		if (wInY <= y)
			y = wInY;
		return true;
	}	 

	bool Virtex5::suggestSubaddSize(int &x, int wIn){
		int chunkSize = 2 + (int)floor( (1./frequency() - (fdCtoQ_ + slice2sliceDelay_ + lut2_ + muxcyStoO_ + xorcyCintoO_ + ffd_)) / muxcyCINtoO_ );
		x = min(chunkSize, wIn);		
		if (x > 0) 
			return true;
		else {
			x = min(2,wIn);		
			return false;
		} 
	};
	
	bool Virtex5::suggestSlackSubaddSize(int &x, int wIn, double slack){
		int chunkSize =  1 + (int)floor( (1./frequency() - slack - (lut2_ + muxcyStoO_ + xorcyCintoO_)) / muxcyCINtoO_ );
		x = chunkSize;	
		x = min(chunkSize, wIn);	
		if (x > 0) 
			return true;
		else {
			x = min(2,wIn);		
			return false;
		} 
	};

	
//	bool Virtex5::suggestSubaddSize(int &x, int wIn){
//		
////		int chunkSize = 2 + (int)floor( (1./frequency() - (fdCtoQ_ + slice2sliceDelay_ + lut2_ + muxcyStoO_ + xorcyCintoO_ + ffd_)) / muxcyCINtoO_ );
//		int chunkSize = 1 + (int)floor( (1./frequency() - localWireDelay() - (lut2_ + muxcyStoO_ + xorcyCintoO_)) / muxcyCINtoO_ );
//		
//		x = chunkSize;		
//		if (x > 0) 
//			return true;
//		else {
//			x = 2;		
//			return false;
//		} 
//	};
//	
//	bool Virtex5::suggestSlackSubaddSize(int &x, int wIn, double slack){
//		
//// 		int chunkSize = 2 + (int)floor( (1./frequency() - slack - (fdCtoQ_ + slice2sliceDelay_ + lut2_ + muxcyStoO_ + xorcyCintoO_ + ffd_)) / muxcyCINtoO_ );
//		int chunkSize = 1 + (int)floor( (1./frequency() - slack - (lut2_ + muxcyStoO_ + xorcyCintoO_)) / muxcyCINtoO_ );
//		
//		x = chunkSize;		
//		if (x > 1) 
//			return true;
//		else {
//			x = 2;		
//			return false;
//		} 
//	};
	
	bool Virtex5::suggestSlackSubcomparatorSize(int& x, int wIn, double slack, bool constant)
	{
		bool succes = true;
		
		if (!constant){
			x = (lutInputs_/2)*(((1./frequency() - slack) - (lut2_ + muxcyStoO_))/muxcyCINtoO_ - 1)+1;
		}else{
			x = (lutInputs_/2)*(((1./frequency() - slack) - (lut2_ + muxcyStoO_))/muxcyCINtoO_ - 1)+1;
		}
		if (x<lutInputs_){ //capture possible negative values
			x = lutInputs_;
			succes = false;
		}
		
		if (x> wIn)//saturation 
			x = wIn;
		
		return succes;
	}
	
	
	int Virtex5::getIntMultiplierCost(int wInX, int wInY){
		
		int cost = 0;
		int halfLut = lutInputs_/2;
		int cx = int(ceil((double) wInX/halfLut));	// number of chunks on X
		int cy = int(ceil((double) wInY/halfLut));  // number of chunks on Y
		int padX = cx*halfLut - wInX; 				// zero padding of X input
		int padY = cy*halfLut - wInY; 				// zero padding of Y input
		
		if (cx > cy) // set cx as the min and cy as the max
		{
			int tmp = cx;
			cx = cy;
			cy = tmp;
			tmp = padX;
			padX = padY;
			padY = tmp;
		}
		
		float p = (double)cy/(double)halfLut; // number of chunks concatenated per operand
		float r = p - floor(p); // relative error; used for detecting how many operands have ceil(p) chunks concatenated
		int chunkSize, aux;
		suggestSubaddSize(chunkSize, wInX+wInY);
		// int lastChunkSize = (wInX+wInY)%chunkSize;
		// int nr = ceil((double) (wInX+wInY)/chunkSize);
		
		
		if (r == 0.0) // all IntNAdder operands have p concatenated partial products
		{
			aux = halfLut*cx; // number of operands having p concatenated chunks
			
			if (aux <= 4)	
				cost = p*lutInputs_*(aux-2)*(aux-1)/2-(padX*cx*(cx+1)+padY*aux*(aux+1))/2; // registered partial products without zero paddings
				else
					cost = p*lutInputs_*3 + p*lutInputs_*(aux-4)-(padX*(cx+1)+padY*(aux+1)); // registered partial products without zero paddings including SRLs
		}
		else if (r > 0.5) // 2/3 of the IntNAdder operands have p concatenated partial products
		{
			aux = (halfLut-1)*cx; // number of operands having p concatenated chunks
			
			if (halfLut*cx <= 4)
				cost = ceil(p)*lutInputs_*(aux-2)*(aux-1)/2 + floor(p)*lutInputs_*((aux*cx)+(cx-2)*(cx-1)/2);// registered partial products without zero paddings
				else
				{
					if (aux - 4 > 0)
					{
						cost = ceil(p)*lutInputs_*3 - padY - 2*padX + 	// registered partial products without zero paddings
						(ceil(p)*lutInputs_-padY) * (aux-4) + 	// SRLs for long concatenations
						(floor(p)*lutInputs_*cx - cx*padY); 		// SRLs for shorter concatenations
					}	
					else
					{
						cost = ceil(p)*lutInputs_*(aux-2)*(aux-1)/2 + floor(p)*lutInputs_*((aux*cx)+(cx+aux-6)*(cx+aux-5)/2); // registered partial products without zero paddings
						cost += (floor(p)*lutInputs_-padY) * (aux+cx-4); // SRLs for shorter concatenations
					}
				}
		}	
		else if (r > 0) // 1/3 of the IntNAdder operands have p concatenated partial products
		{
			aux = (halfLut-1)*cx; // number of operands having p concatenated chunks
			
			if (halfLut*cx <= 4)
				cost = ceil(p)*lutInputs_*(cx-2)*(cx-1)/2 + floor(p)*lutInputs_*((aux*cx)+(aux-2)*(aux-1)/2);// registered partial products without zero paddings
				else
				{
					cost = ceil(p)*lutInputs_*(cx-2)*(cx-1)/2 + floor(p)*lutInputs_*((aux*cx)+(aux-2)*(aux-1)/2);// registered partial products without zero paddings
					if (cx - 4 > 0)
					{
						cost = ceil(p)*lutInputs_*3 - padY - 2*padX; // registered partial products without zero paddings
						cost += ceil(p)*lutInputs_*(cx-4) - (cx-4)*padY; // SRLs for long concatenations
						cost += floor(p)*lutInputs_*aux - aux*padY; // SRLs for shorter concatenations
					}	
					else
					{
						cost = ceil(p)*lutInputs_*(cx-2)*(cx-1)/2 + floor(p)*lutInputs_*((aux*cx)+(cx+aux-6)*(cx+aux-5)/2); // registered partial products without zero paddings
						cost += (floor(p)*lutInputs_-padY) * (aux+cx-4); // SRLs for shorter concatenations
					}
				}
		}
		aux = halfLut*cx;
		cost += p*lutInputs_*aux + halfLut*(aux-1)*aux/2; // registered addition results on each pipeline stage of the IntNAdder
		
		if (padX+padY > 0)
			cost += (cx-1)*(cy-1)*lutInputs_ + cx*(lutInputs_-padY) + cy*(lutInputs_-padX);
		else
			cost += cx*cy*lutInputs_; // LUT cost for small multiplications 
			
			return cost*5/8;
	};
	
	void Virtex5::getDSPWidths(int &x, int &y, bool sign){
		// unsigned multiplication
		if (! sign ){
			x = multXInputs_-1;
			y = multYInputs_-1;
		}else{
			x = multXInputs_;
			y = multYInputs_;
		}
	}
	
	int Virtex5::getEquivalenceSliceDSP(){
		int lutCost = 0;
		int x, y;
		getDSPWidths(x,y);
		// add multiplier cost
		lutCost += getIntMultiplierCost(x, y);
		// add shifter and accumulator cost
		//lutCost += accumulatorLUTCost(x, y);
		return lutCost;
	}
	
	int Virtex5::getNumberOfDSPs() 
	{
		return nrDSPs_; 		
	};
	
	int Virtex5::getIntNAdderCost(int wIn, int n)
	{
		/* compressor tree cost */
		int p = n, q = 0;
		int levels = 0;
		
		
		do{
			while (p - 6 > 0){
				p-=6;
				q+=3;
			}
			while (p - 5 > 0){
				p-=5;
				q+=3;
			}
			while (p - 4 > 0){
				p-=4;
				q+=3;
			}
			while (p - 3 > 0){
				p-=3;
				q+=2;
			}
			levels++;
			p=q;
			q=0;
		}while (p>2);
		
		/* at this point levels holds the number of lut levels*/
		int alpha, beta, k, intAdderCost;
		suggestSubaddSize(alpha, wIn);
		beta = (wIn % alpha == 0? alpha: wIn % alpha);
		k = (wIn % alpha ==0? wIn/alpha: wIn/alpha + 1);
				
		intAdderCost = (4*k-7)*alpha + 3*beta;
		
		return (intAdderCost + levels*wIn);
		
		/*
		int chunkSize, lastChunkSize, nr, a, b, cost;
		
		suggestSubaddSize(chunkSize, wIn);
		lastChunkSize = wIn%chunkSize;
		nr = ceil((double) wIn/chunkSize);
		// IntAdder
		a = (nr-1)*nr*chunkSize/2 + (nr-1)*lastChunkSize;
		b = nr*lastChunkSize + (nr-1)*nr*chunkSize/2;
		
		if (nr > 2) // carry
		{
			a += (nr-1)*(nr-2)/2;
		}
		
		if (nr == 3) // SRL16E
		{
			a += chunkSize;	
		} 
		else if (nr > 3) // SRL16E, FDE
		{
			a += (nr-3)*chunkSize + 1;
			b += (nr-3)*chunkSize + 1;
		}
		// IntNAdder
		if (n >= 3)
		{
			a += (2*n-4)*wIn + 1;
			b += (2*n-5)*wIn;
		}
		
		cost = a+b*0.25;
		return cost;
		*/
	}
	
	void Virtex5::delayForDSP(MultiplierBlock* multBlock, double currentCp, int& cycleDelay, double& cpDelay)
	{
		double targetPeriod, totalPeriod;
		
		targetPeriod = 1.0/frequency();
		totalPeriod = currentCp + DSPMultiplierDelay_;
		
		cycleDelay = floor(totalPeriod/targetPeriod);
		cpDelay = totalPeriod-targetPeriod*cycleDelay;
	}
}
