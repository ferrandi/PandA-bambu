/*
 A model of SpartanIII FPGA  (FIXME exact model?) 

 Author: Bogdan Pasca

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.
*/
#include "Spartan3.hpp"
#include <iostream>
#include <sstream>
#include "../utils.hpp"

namespace flopoco{
	
	
	double Spartan3::adderDelay(int size) {
		return lut2_ + muxcyStoO_ + double(size-1)*muxcyCINtoO_ + xorcyCintoO_ ; 
	};
	
	double Spartan3::adder3Delay(int size) {
		return 0 ; // currently irrelevant for Xilinx
	};
	
	void Spartan3::getAdderParameters(double &k1, double &k2, int size){
		k1 = lut2_ + muxcyStoO_ + xorcyCintoO_;
		k2 = muxcyCINtoO_;
	}
	
	double Spartan3::eqComparatorDelay(int size){
		return lut2_ + muxcyStoO_ + double((size-1)/(lutInputs_/2)+1)*muxcyCINtoO_; 
	}
	
	double Spartan3::eqConstComparatorDelay(int size){
		return lut2_ + muxcyStoO_ + double((size-1)/lutInputs_+1)*muxcyCINtoO_; 
	}
	
	double Spartan3::ffDelay() {
		return fdCtoQ_ + ffd_; 
	};
	
	double Spartan3::carryPropagateDelay() {
		return  fastcarryDelay_; 
	};
	
	double Spartan3::localWireDelay(int fanout){
		return  elemWireDelay_ ;
	};
	
	double Spartan3::distantWireDelay(int n){
		return n*elemWireDelay_;
	};
	
	double Spartan3::lutDelay(){
		return lutDelay_;
	};
	
	long Spartan3::sizeOfMemoryBlock()
	{
		return sizeOfBlock_;	
	};
	
	
	DSP* Spartan3::createDSP() 
	{
		int x, y;
		getDSPWidths(x, y);
		
		/* create DSP block with constant shift of 17
		* and a maxium unsigned multiplier width (17x17) for this target
		*/
		DSP* dsp_ = new DSP(17, x, y);
		
		return dsp_;
	};
	
	bool Spartan3::suggestSubmultSize(int &x, int &y, int wInX, int wInY){
		x = y = 2;
		/**	
		if (getUseHardMultipliers()){
			if ((wInX<=17) && (wInY<=17))	{
				x = max(wInX, wInY);
				y = x;
				if (frequency()>600000000)
					return false;
				else
					return true;
	}else{
		int f1=(wInX % 17 ==0)?0:1;
		int f2=(wInY % 17 ==0)?0:1;
		int k=wInX/17+wInY/17 + f1+ f2;
		x = 17;	y = 17;
		
		if (k<=4)
			if (frequency()<=400000000)
				return true;
			else 
				return false; 
			else{
				double freq;
				freq = 11.2 + 1560/k;
				if (frequency()<=freq*1000000)			
					return true;
				else 
					return false;
	} 
	}
	}else{
		// Following lines suppressed to remove a warning
		//int f1=(wInX % 17 ==0)?0:1;
		//int f2=(wInY % 17 ==0)?0:1;
		// int k=wInX/17+wInY/17 + f1+ f2;
		double freq;
		
		if ((max(wInX,wInY)<=4)&&(max(wInX,wInY)>=2))
		{
			freq = 669-13* (max(wInX,wInY));
			
			x=wInX;
			y=wInY;
			if (frequency()<=freq*1000000)			
				return true;
			else 
				return false;
	} else if ((max(wInX,wInY)<=15)&&(max(wInX,wInY)>=5)){
		freq = 411-9*(max(wInX,wInY));
		if (frequency()<=freq*1000000){			
			x=wInX;
			y=wInY;			
			return true;
	}
	else{
		freq = 121.3+549/2+988/4;
		if (frequency()>freq*1000000){
			x=2;
			y=2;
			return false;
	}else{
		int i=2;
		while ( ((121.3+549/i+988/(i*i))*1000000) > frequency())
			i++;
		
		x=i-1;
		y=i-1;
		return true;
	}
	}
	} else if (max(wInX,wInY)>15){
		freq = 80.2+34037/(max(wInX,wInY)*max(wInX,wInY));
		if (frequency()<=freq*1000000){
			x=wInX;
			y=wInY;			
			return true;
	}
	else{
		freq = 121.3+549/2+988/4;
		if (frequency()>freq*1000000){
			x=2;
			y=2;
			return false;
	}else{
		int i=2;
		while ( ((121.3+549/i+988/(i*i))*1000000) > frequency())
			i++;
		
		x=i-1;
		y=i-1;
		return true;
	}
	}	
	}
	}
	**/
		// To suppress a warning, but we should never get here
		return false;
	};	 
	
	bool Spartan3::suggestSubaddSize(int &x, int wIn){
		
		//	int chunkSize = (int)floor( (1./frequency() - lutDelay()) / carryPropagateDelay()); // 1 if no need for pipeline
		
		
		int chunkSize = 2 + (int)floor( (1./frequency() - (fdCtoQ_ + slice2sliceDelay_ + lut2_ + muxcyStoO_ + xorcyCintoO_ + ffd_)) / muxcyCINtoO_ );
		x = chunkSize;		
		if (x > 0) 
			return true;
		else {
			x = 2;		
			return false;
		} 
	};
	
	bool Spartan3::suggestSubadd3Size(int &x, int wIn)
	{
		return 0; // currently irrelevant for Xilinx
	};
	
	bool Spartan3::suggestSlackSubaddSize(int &x, int wIn, double slack){
		
		//	int chunkSize = (int)floor( (1./frequency() - lutDelay()) / carryPropagateDelay()); // 1 if no need for pipeline
		
		
		int chunkSize = 2 + (int)floor( (1./frequency() - slack - (fdCtoQ_ + slice2sliceDelay_ + lut2_ + muxcyStoO_ + xorcyCintoO_ + ffd_)) / muxcyCINtoO_ );
		x = chunkSize;		
		if (x > 0) 
			return true;
		else {
			x = 2;		
			return false;
		} 
	};
	
	bool Spartan3::suggestSlackSubadd3Size(int &x, int wIn, double slack)
	{
		return 0; // currently irrelevant for Xilinx
	};
	
	bool Spartan3::suggestSlackSubcomparatorSize(int& x, int wIn, double slack, bool constant)
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
	
	int Spartan3::getIntMultiplierCost(int wInX, int wInY){
		
		int lutCost = 0;
		int chunkSize_ = this->lutInputs()/2;
		
		int chunksX =  int(ceil( ( double(wInX) / (double) chunkSize_) ));
		int chunksY =  int(ceil( ( double(wInY) / (double) chunkSize_) ));
		
		if (chunksX > 0)
		{
			// FIRST SET OF PARTIAL PRODUCTS
			for (int i=0; i<chunksY; i++)
			{
				if (i%chunkSize_ == 0) 
					lutCost += 6; // one LUT for each: CY<1>, CY<2>_mand, lut<1>, xor<2>, product of 2 pairs of underlying bits
					else
						lutCost += 4; // one LUT for each: CY<2>, lut<1>, xor<2>, product of 1 pair of underlying LSBs
			}
			
			if (chunksX > 1)
			{
				// SECOND SET OF PARTIAL PRODUCTS
				for (int i=0; i<chunksY; i++)
				{	
					if (i%chunkSize_ == 0) 
						lutCost += 3; // one LUT for each: CY<2>, lut<1>, product of 1 pair of underlying LSBs
						else
							lutCost += 4; // one LUT for each: CY<2>, lut<1>, xor<2>, product of 1 pair of underlying LSBs
				}
				
				for (int i=2; i<chunksX; i++)
					for (int j=0; j<chunksY; j++)
						lutCost += 4; // one LUT for each: CY<2>, lut<1>, xor<2>, product of 1 pair of underlying LSBs
						
			}
		}
		
		return lutCost + this->getIntNAdderCost(chunksX*chunkSize_, chunksY*chunkSize_);
	};
	
	void Spartan3::getDSPWidths(int &x, int &y, bool sign){
		if (! sign){
			x = 17;
			y = 17;
		}else{
			x = 18;
			y = 18;
		}
	}
	
	int Spartan3::getEquivalenceSliceDSP(){
		int lutCost = 0;
		int x, y;
		getDSPWidths(x,y);
		// add multiplier cost
		lutCost += getIntMultiplierCost(x, y);
		// add shifter and accumulator cost
		//lutCost += accumulatorLUTCost(x, y);
		return lutCost;
	}
	
	int Spartan3::getNumberOfDSPs() 
	{
		return 12; // XC3S200 12 DSPs		
	};
	
	int Spartan3::getIntNAdderCost(int wIn, int n)
	{
		int chunkSize, lastChunkSize, nr, a, b, cost;
		
		suggestSubaddSize(chunkSize, wIn);
		lastChunkSize = wIn%chunkSize;
		nr = ceil((double) wIn/chunkSize);
		a = (nr-2)*lastChunkSize/2 + nr*wIn/2 + 2*(nr-1 + wIn) + chunkSize + (2*wIn+(nr-1))*(n-3);
		b = nr*lastChunkSize/2 + nr*wIn/2 + (nr-2)*chunkSize + wIn + 2*wIn*(n-3);
		cost = a+b*0.25;
		return cost;
	}
	
	void Spartan3::delayForDSP(MultiplierBlock* multBlock, double currentCp, int& cycleDelay, double& cpDelay)
	{
		double targetPeriod, totalPeriod;
		
		targetPeriod = 1.0/frequency();
		totalPeriod = currentCp + DSPMultiplierDelay_;
		
		cycleDelay = floor(totalPeriod/targetPeriod);
		cpDelay = totalPeriod-targetPeriod*cycleDelay;
	}
	
	
}
