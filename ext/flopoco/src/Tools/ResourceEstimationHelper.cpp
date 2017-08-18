/*
Tools for performing resource estimation

Author : Matei Istoan, Florent de Dinechin

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
2012.
  All rights reserved.

*/


#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include "Operator.hpp"
#include "utils.hpp"
#include "ResourceEstimationHelper.hpp"


namespace flopoco{
	

	ResourceEstimationHelper::ResourceEstimationHelper(Target* target_, Operator* op_){
		
		target = target_;
		parentOp = op_;
	}
	
	
	//--Logging functions-----------------------------------------------
	//---General resources----------------------------------------------
	void ResourceEstimationHelper::initResourceEstimation(){
		
		estimatedCountFF = 0;
		estimatedCountLUT = 0;
		estimatedCountMultiplier = 0;
		estimatedCountMemory = 0;
		
		estimatedCountDSP = 0;
		estimatedCountRAM = 0;
		estimatedCountROM = 0;
		estimatedCountSRL = 0;
		estimatedCountWire = 0;
		estimatedCountIOB = 0;
		
		estimatedCountMux = 0;
		estimatedCountCounter = 0;
		estimatedCountAccumulator = 0;
		estimatedCountDecoder = 0;
		estimatedCountArithOp = 0;
		estimatedCountAdderSubtracter = 0;
		estimatedCountReg = 0;
		estimatedCountFSM = 0;
	}
	
	std::string ResourceEstimationHelper::addFF(int count){
		std::ostringstream output;
		
		if(parentOp->isSequential()){
			estimatedCountFF += count;
			
			(count>0)? output << tab << "FF count increased by " << count : output << "FF count decreased by " << count;
			output << " by adding " << count << " flip-flops";
			output << endl;
		}else{
			output << "FF count not increased: combinatorial circuit. Warning: adding register resources for combinatorial circuit" << endl;
		}
		
		return output.str();
	}
	
	std::string ResourceEstimationHelper::addLUT(int nrInputs, int count){
		std::ostringstream output;								 
		int targetLUTType, increment;
		map<int, int>::iterator it;
		
		(nrInputs == 0) ? targetLUTType =  target->lutInputs() : targetLUTType = nrInputs;
		if(target->lutSplitInputs(targetLUTType)){
			increment = count/2;
		}else
			increment = count;
		
		it = estimatedLUTTypes.find(nrInputs);
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[nrInputs] = count;
		}else{
			estimatedLUTTypes[nrInputs] += count;
		}
		
		estimatedCountLUT += increment;
		
		(increment>0)? output << tab << "LUT count increased by " << increment : output << "LUT count decreased by " << increment;
		output << " by adding " << count << " function generators";
		output << endl;
		return output.str();
	}
	
	std::string ResourceEstimationHelper::addReg(int width, int count){
		std::ostringstream output;
		int increment = count*width;
		map<int, int>::iterator it;
		
		if(parentOp->isSequential()){
			estimatedCountFF += increment;
		
			(count>0)? output << "Register count increased by " << count : output << "Register count decreased by " << count;
			output << endl;
			(count>0)? output << tab << "FF count increased by " << increment : output << "FF count decreased by " << increment;
			output << " after adding " << count << " registers of width " << width;
			output << endl;
			
			it = estimatedRegisterTypes.find(width);
			if(it == estimatedRegisterTypes.end()){
				estimatedRegisterTypes[width] = count;
			}else{
				estimatedRegisterTypes[width] += count;
			}
		}else{
			output << "Register and FF counts not increased: combinatorial circuit" << endl;
		}
		
		return output.str();
	}
	
	//TODO: verify increase in the DSP count
	std::string ResourceEstimationHelper::addMultiplier(int count){
		std::ostringstream output;
		int increment, increment2, increment3, widthX, widthY, maxWidth;
		map<int, int>::iterator it;
		
		estimatedCountMultiplier += count;
		target->getDSPWidths(widthX, widthY);
		(widthX>widthY) ? maxWidth=widthX : maxWidth=widthY;
		increment = ceil(count*maxWidth*target->getLUTPerMultiplier(widthX, widthY));
		estimatedCountLUT += increment;
		if(parentOp->isSequential()){
			increment2 = ceil(count*maxWidth*target->getFFPerMultiplier(widthX, widthY));
			estimatedCountFF += increment2;
		}
		if(target->dspSplitMult()){
			increment3 = count*target->getMultPerDSP(widthX, widthY);
		}else{
			increment3 = count;
		}
		estimatedCountDSP += increment3;
		
		(count>0)? output << "Multiplier count increased by " << count : output << "Multiplier count decreased by " << count;
		output << " after ading " << count << " multipliers";
		output << endl;
		(increment>0)? output << tab << "LUT count increased by " << increment : output << "LUT count decreased by " << increment;
		output << " after adding " << count << " " << widthX << "by" << widthY << " multipliers";
		output << endl;
		if(parentOp->isSequential()){
			(increment2>0)? output << tab << "FF count increased by " << increment2 : output << "FF count decreased by " << increment2;
			output << " after adding " << count << " " << widthX << "by" << widthY << " multipliers";
			output << endl;
		}else{
			output << "FF count not increased: combinatorial circuit" << endl;
		}
		(increment3>0)? output << tab << "DSP count increased by " << increment3 : output << "DSP count decreased by " << increment3;
		output << " after adding " << count << " " << widthX << "by" << widthY << " multipliers";
		output << endl;
		
		it = estimatedLUTTypes.find(target->lutInputs());
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[target->lutInputs()] = increment2;
		}else{
			estimatedLUTTypes[target->lutInputs()] += increment2;
		}
		it = estimatedMultiplierTypes.find(maxWidth);
		if(it == estimatedMultiplierTypes.end()){
			estimatedMultiplierTypes[maxWidth] = increment;
		}else{
			estimatedMultiplierTypes[maxWidth] += increment;
		}
		
		return output.str();
	}
	
	//TODO: verify increase in the DSP count 
	std::string ResourceEstimationHelper::addMultiplier(int widthX, int widthY, double ratio, int count){
		std::ostringstream output;
		int sizeDSPx, sizeDSPy, maxWidth;
		int increment, increment2, increment3, increment4;
		double ratioX, ratioY;
		map<int, int>::iterator it;
		
		target->getDSPWidths(sizeDSPx, sizeDSPy, true);
		(widthX>widthY) ? maxWidth=sizeDSPx : maxWidth=sizeDSPy;
		ratioX = (double)widthX/sizeDSPx;
		ratioY = (double)widthY/sizeDSPy;
		increment = ceil(ratio * count * ceil(ratioX) * ceil(ratioY));
		estimatedCountMultiplier += increment;
		
		
		increment2 = ceil(count*maxWidth*target->getLUTPerMultiplier(widthX, widthY));
		estimatedCountLUT += increment2;
		
		if(parentOp->isSequential()){
			increment3 = ceil(count*maxWidth*target->getFFPerMultiplier(widthX, widthY));
			estimatedCountFF += increment3;
		}
		
		if(target->dspSplitMult()){
			increment4 = count*target->getMultPerDSP(widthX, widthY);
		}else{
			increment4 = count*ceil(ratioX)*ceil(ratioY);
		}
		estimatedCountDSP += increment4;
		
		(increment>0)? output << "Multiplier count increased by " << increment : output << "Multiplier count decreased by " << increment;
		output << endl;
		(increment2>0)? output << tab << "LUT count increased by " << increment2 : output << "LUT count decreased by " << increment2;
		output << " after adding " << increment << " " << widthX << "by" << widthY << " multipliers";
		output << endl;
		if(parentOp->isSequential()){
			(increment3>0)? output << tab << "FF count increased by " << increment3 : output << "FF count decreased by " << increment3;
			output << " after adding " << increment << " " << widthX << "by" << widthY << " multipliers";
			output << endl;
		}else{
			output << "FF count not increased: combinatorial circuit" << endl;
		}
		(increment4>0)? output << tab << "DSP count increased by " << increment4 : output << "DSP count decreased by " << increment4;
		output << " after ading " << increment << " " << widthX << "by" << widthY << " multipliers";
		output << endl;
		
		it = estimatedLUTTypes.find(target->lutInputs());
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[target->lutInputs()] = increment2;
		}else{
			estimatedLUTTypes[target->lutInputs()] += increment2;
		}
		it = estimatedMultiplierTypes.find(maxWidth);
		if(it == estimatedMultiplierTypes.end()){
			estimatedMultiplierTypes[maxWidth] = increment;
		}else{
			estimatedMultiplierTypes[maxWidth] += increment;
		}
		
		return output.str();
	}
	
	//TODO: verify increase in the element count
	std::string ResourceEstimationHelper::addAdderSubtracter(int widthX, int widthY, double ratio, int count){
		std::ostringstream output;
		int increment, increment2;
		double period;
		int alpha, beta, kappa;
		int maxWidth;
		map<int, int>::iterator it;
		
		(widthX>widthY) ? maxWidth=widthX : maxWidth=widthY;
		
		estimatedCountAdderSubtracter += count;
		
		//method based on FloPoCo formulas
		/*
		period = 1.0/frequency_;
		//alpha = 1 + floor((period-2.0*lutDelay())/carryPropagateDelay());
		suggestSubaddSize(alpha, maxWidth);
		kappa = maxWidth/alpha + 1;
		beta = maxWidth - (kappa - 1)*alpha;
		*/
		
		period = 1.0/target->frequency();
		target->suggestSubaddSize(alpha, maxWidth);
		beta = (maxWidth % alpha == 0 ? alpha : maxWidth % alpha);
		kappa = (maxWidth % alpha == 0 ? maxWidth/alpha : maxWidth/alpha + 1);
		
		//TODO: decide formula to use based on the type of the adder
		if(kappa == 1){
			increment = count * maxWidth;
			if(parentOp->isSequential()){
				increment2 = count * maxWidth;
			}
		}else if(kappa == 2){
			increment = count * alpha+2*beta;
			if(parentOp->isSequential()){
				increment2 = count * (2*kappa-3)*alpha+beta+2*kappa-3;
			}
		}else{
			increment = count * (4*kappa-8)*alpha+3*beta+kappa-3;
			if(parentOp->isSequential()){
				increment2 = count * (2*kappa-3)*alpha+beta+2*kappa-3;
			}
		}
		
		//method based on approximations and experimental data
		//increment = count * ceil(count*maxWidth*target_->getLUTPerAdderSubtracter(widthX, widthY));
		//increment = count * ceil(count*maxWidth*target_->getFFPerAdderSubtracter(widthX, widthY));
		estimatedCountLUT += increment;
		if(parentOp->isSequential()){
			estimatedCountFF += increment2;
		}
		
		(increment>0)? output << "Adder/Subtracter count increased by " << count : output << "Adder/Subtracter count decreased by " << count;
		output << endl;
		(increment>0)? output << tab << "LUT count increased by " << increment : output << "LUT count decreased by " << increment;
		output << " after adding " << count << " " << widthX << "by" << widthY << " adders";
		output << endl;
		if(parentOp->isSequential()){
			(increment2>0)? output << tab << "FF count increased by " << increment2 : output << "FF count decreased by " << increment2;
			output << " after adding " << count << " " << widthX << "by" << widthY << " adders";
			output << endl;
		}else{
			output << "FF count not increased: combinatorial circuit" << endl;
		}
		
		it = estimatedLUTTypes.find(target->lutInputs());
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[target->lutInputs()] = increment;
		}else{
			estimatedLUTTypes[target->lutInputs()] += increment;
		}
		it = estimatedAdderTypes.find(maxWidth);
		if(it == estimatedMultiplierTypes.end()){
			estimatedAdderTypes[maxWidth] = count;
		}else{
			estimatedAdderTypes[maxWidth] += count;
		}
		
		return output.str();
	}
	
	//TODO: take into account the memory type (RAM or ROM); depending on 
	//		the type, might be implemented through distributed memory or
	//		dedicated memory blocks
	std::string ResourceEstimationHelper::addMemory(int size, int width, int type, int count){
		std::ostringstream output;
		int increment, WpB;
		
		WpB = target->wordsPerBlock(width);
		increment = ceil(count*(double)size/WpB);
		estimatedCountMemory += increment;
		(type) ? estimatedCountROM += increment : estimatedCountRAM += increment;
		
		(increment>0)? output << "Memory elements count increased by " << increment : output << "Memory elements count decreased by " << increment;
		(type>0) ? output << tab << " ROM memory was targeted" : output << " RAM memory was targeted";
		(type>0) ? (increment>0)? output << tab << "ROM count increased by " << increment : output << "ROM count decreased by " << increment
			   : (increment>0)? output << tab << "RAM count increased by " << increment : output << "RAM count decreased by " << increment;
		output << endl;
		return output.str();
	}
	
	//---More particular resource logging
	std::string ResourceEstimationHelper::addDSP(int count){
		std::ostringstream output;
		map<int, int>::iterator it;
		int widthX, widthY, maxWidth;
		
		estimatedCountDSP += count;
		
		(count>0)? output << tab << "DSP count increased by " << count : output << "DSP count decreased by " << count;
		output << endl;
		
		target->getDSPWidths(widthX, widthY, true);
		(widthX>widthY) ? maxWidth=widthX : maxWidth=widthY;
		it = estimatedMultiplierTypes.find(maxWidth);
		if(it == estimatedLUTTypes.end()){
			estimatedMultiplierTypes[maxWidth] = count;
		}else{
			estimatedMultiplierTypes[maxWidth] += count;
		}
		
		return output.str();
	}
	
	//TODO: get a better default size
	std::string ResourceEstimationHelper::addRAM(int count){
		std::ostringstream output;
		int increment, WpB;
		
		WpB = target->wordsPerBlock(1);
		increment = ceil(count*WpB);
		estimatedCountMemory += increment;
		estimatedCountRAM += count;
		
		(increment>0)? output << "Memory elements count increased by " << increment : output << "Memory elements count decreased by " << increment;
		(count>0)? output << tab << "RAM count increased by " << count : output << "RAM count decreased by " << count;
		output << endl;
		return output.str();
	}
	
	//TODO: get a better default size
	std::string ResourceEstimationHelper::addROM(int count){
		std::ostringstream output;
		int increment, WpB;
		
		WpB = target->wordsPerBlock(1);
		increment = ceil(count*WpB);
		estimatedCountMemory += increment;
		estimatedCountROM += count;
		
		(increment>0)? output << "Memory elements count increased by " << increment : output << "Memory elements count decreased by " << increment;
		(count>0)? output << tab << "ROM count increased by " << count : output << "ROM count decreased by " << count;
		output << endl;
		return output.str();
	}
	
	//TODO: should count the shift registers according to their bitwidths
	std::string ResourceEstimationHelper::addSRL(int width, int depth, int count){
		std::ostringstream output;
		int increment, increment2, increment3, increment4;
		int defaultShifterDepth, lutInputs;
		map<int, int>::iterator it;
		
		defaultShifterDepth = target->getSRLDepth(depth);
		increment = count*width*ceil((double)depth/defaultShifterDepth);
		estimatedCountSRL += increment;
		
		increment2 = ceil(count*width*target->getLUTPerSRL(depth));
		estimatedCountLUT += increment2;
		
		if(parentOp->isSequential()){
			increment3 = ceil(count*width*target->getFFPerSRL(depth));
			estimatedCountFF += increment3;
		}
		
		increment4 = ceil(count*target->getRAMPerSRL(depth));
		estimatedCountRAM += increment4;
		
		(increment>0)? output << "SRL count increased by " << increment : output << "SRL count decreased by " << increment;
		output << endl;
		(increment2>0)? output << tab << "LUT count increased by " << increment2 : output << "LUT count decreased by " << increment2;
		output << " after adding " << increment << " adders of width " << width << " and depth " << depth;
		output << endl;
		if(parentOp->isSequential()){
			(increment3>0)? output << tab << "FF count increased by " << increment3 : output << "FF count decreased by " << increment3;
			output << " after adding " << increment << " adders of width " << width << " and depth " << depth;
			output << endl;
		}else{
			output << "FF count not increased: combinatorial circuit" << endl;
		}
		(increment4>0)? output << tab << "RAM count increased by " << increment4 : output << "RAM count decreased by " << increment4;
		output << " after adding " << increment << " adders of width " << width << " and depth " << depth;
		output << endl;
		
		lutInputs = target->lutInputs();
		it = estimatedLUTTypes.find(lutInputs);
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[lutInputs] = increment2;
		}else{
			estimatedLUTTypes[lutInputs] += increment2;
		}
		it = estimatedShifterTypes.find(width);
		if(it == estimatedShifterTypes.end()){
			estimatedShifterTypes[width] = increment;
		}else{
			estimatedShifterTypes[width] += increment;
		}
		
		return output.str();
	}
	
	std::string ResourceEstimationHelper::addWire(int count, std::string signalName){
		std::ostringstream output;
		int increment;
		
		if(signalName.empty())
			increment = count;
		else{
			Signal* s = (parentOp->getSignalMap())[signalName];
			
			increment = s->width();
			estimatedSignalNames.push_back(signalName);
		}
		estimatedCountWire += increment;
		
		(increment>0)? output << tab << "Wire count increased by " << increment : output << "Wire count decreased by " << increment;
		output  << endl;
		return output.str();
	}
	
	std::string ResourceEstimationHelper::addIOB(int count, std::string portName){
		std::ostringstream output;
		int increment;
		
		if(portName.empty())
			increment = count;
		else{
			Signal* s = (parentOp->getSignalMap())[portName];
			
			increment = s->width();
			estimatedPortNames.push_back(portName);
		}
		estimatedCountIOB += increment;
		
		(increment>0)? output << tab << "IOB count increased by " << increment : output << "IOB count decreased by " << increment;
		output << endl;
		return output.str();
	}
	
	//---Even more particular resource logging-------------------------
	
	//TODO: get a more accurate count of the number of multiplexers 
	//		needed; currently specific resources are not taken into account
	std::string ResourceEstimationHelper::addMux(int width, int nrInputs, int count){
		std::ostringstream output;
		int increment, increment2;
		int stdInputs, lutInputs;
		map<int, int>::iterator it;
		
		increment = count;
		estimatedCountMux += increment;
		
		stdInputs = 1;
		while(stdInputs<nrInputs)
			stdInputs *= 2;
		if(stdInputs == 1){
			cout << "Warning: addMux(): trying to add multiplexer with a single input" << endl;
			return 0;
		}
		increment2 = ceil(count*width*target->getLUTFromMux(stdInputs));
		estimatedCountLUT += increment2;
		
		(increment>0)? output << "MUX count increased by " << increment : output << "MUX count decreased by " << increment;
		output << " after adding " << count << " multiplexers of width " << width << " and with " << nrInputs << " inputs";
		output << endl;
		(increment>0)? output << tab << "LUT count increased by " << increment2 : output << "LUT count decreased by " << increment2;
		output << " after adding " << count << " multiplexers of width " << width << " and with " << nrInputs << " inputs";
		output << endl;
		
		lutInputs = target->lutInputs();
		it = estimatedLUTTypes.find(lutInputs);
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[lutInputs] = increment2;
		}else{
			estimatedLUTTypes[lutInputs] += increment2;
		}
		
		return output.str();
	}
	
	//TODO: count the counters according to their bitwidth
	//TODO: get estimations when using specific resources (like DSPs)
	//		involves also changes to getLUTPerCounter() getFFPerCounter()
	std::string ResourceEstimationHelper::addCounter(int width, int count){
		std::ostringstream output;
		int increment, increment2;
		int lutInputs;
		map<int, int>::iterator it;
		
		estimatedCountCounter += count;
		increment = ceil(count*width*target->getLUTPerCounter(width));
		estimatedCountLUT += increment;
		if(parentOp->isSequential()){
			increment2 = ceil(count*width*target->getFFPerCounter(width));
			estimatedCountFF += increment2;
		}
				
		(count>0)? output << "Counter count increased by " << count : output << "Counter count decreased by " << count;
		output << " after adding " << count << " counters of width " << width;
		output << endl;
		(increment>0)? output << tab << "LUT count increased by " << increment : output << "LUT count decreased by " << increment;
		output << " after adding " << count << " counters of width " << width;
		output << endl;
		if(parentOp->isSequential()){
			(increment2>0)? output << tab << "FF count increased by " << increment2 : output << "FF count decreased by " << increment2;
			output << " after adding " << count << " counters of width " << width;
			output << endl;
		}else{
			output << "FF count not increased: combinatorial circuit" << endl;
		}
		
		lutInputs = target->lutInputs();
		it = estimatedLUTTypes.find(lutInputs);
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[lutInputs] = increment;
		}else{
			estimatedLUTTypes[lutInputs] += increment;
		}
		it = estimatedCounterTypes.find(width);
		if(it == estimatedCounterTypes.end()){
			estimatedCounterTypes[width] = count;
		}else{
			estimatedCounterTypes[width] += count;
		}
		
		return output.str();
	}
	
	//TODO: count the accumulators according to their bitwidth
	std::string ResourceEstimationHelper::addAccumulator(int width, bool useDSP, int count){
		std::ostringstream output;
		int increment, increment2, increment3;
		map<int, int>::iterator it;
		
		estimatedCountAccumulator += count;
		increment = ceil(count*width*target->getLUTPerAccumulator(width, useDSP));
		estimatedCountLUT += increment;
		if(parentOp->isSequential()){
			increment2 = ceil(count*width*target->getFFPerAccumulator(width, useDSP));
			estimatedCountFF += increment2;
		}
		increment3 = ceil(count*width*target->getDSPPerAccumulator(width));
		estimatedCountDSP += increment3;
		
		(count>0)? output << "Accumulator count increased by " << count : output << "Accumulator count decreased by " << count;
		output << endl;
		(increment>0)? output << tab << "LUT count increased by " << increment : output << "LUT count decreased by " << increment;
		output << " after adding " << count << " accumulators of width " << width;
		output << endl;
		if(parentOp->isSequential()){
			(increment2>0)? output << tab << "FF count increased by " << increment2 : output << "FF count decreased by " << increment2;
			output << " after adding " << count << " accumulators of width " << width;
			output << endl;
		}else{
			output << "FF count not increased: combinatorial circuit" << endl;
		}
		(increment3>0)? output << tab << "DSP count increased by " << increment3 : output << "DSP count decreased by " << increment3;
		output << " after adding " << count << " accumulators of width " << width;
		output << endl;
		
		it = estimatedLUTTypes.find(target->lutInputs());
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[target->lutInputs()] = increment;
		}else{
			estimatedLUTTypes[target->lutInputs()] += increment;
		}
		
		return output.str();
	}
	
	//TODO: count the decoders according to their input and output 
	//		bitwidths
	std::string ResourceEstimationHelper::addDecoder(int wIn, int wOut, int count){
		std::ostringstream output;
		int increment, increment2;
		int lutInputs;
		map<int, int>::iterator it;
		
		estimatedCountDecoder += count;
		increment = ceil(count*wIn*target->getLUTPerDecoder(wIn));
		estimatedCountLUT += increment;
		if(parentOp->isSequential()){
			increment2 = ceil(count*wIn*target->getFFPerDecoder(wIn));
			estimatedCountFF += increment2;
		}
		
		(count>0)? output << "Decoder count increased by " << count : output << "Decoder count decreased by " << count;
		output << endl;
		(increment>0)? output << tab << "LUT count increased by " << increment : output << "LUT count decreased by " << increment;
		output << " after adding " << count << " decoders of width " << wIn;
		output << endl;
		if(parentOp->isSequential()){
			(increment2>0)? output << tab << "FF count increased by " << increment2 : output << "FF count decreased by " << increment2;
			output << " after adding " << count << " decoders of width " << wIn;
			output << endl;
		}else{
			output << "FF count not increased: combinatorial circuit" << endl;
		}
		
		lutInputs = target->lutInputs();
		it = estimatedLUTTypes.find(lutInputs);
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[lutInputs] = increment;
		}else{
			estimatedLUTTypes[lutInputs] += increment;
		}
		
		return output.str();
	}
	
	std::string ResourceEstimationHelper::addArithOp(int width, int nrInputs, int count){
		std::ostringstream output;
		int increment, lutInputs;
		map<int, int>::iterator it;
		
		estimatedCountArithOp += count;
		
		increment = ceil(count*width*((double)nrInputs/target->lutInputs()));
		estimatedCountLUT += increment;
		
		(count>0)? output << "Arithmetic Operator count increased by " << count : output << "Arithmetic Operator count decreased by " << count;
		output << endl;
		(increment>0)? output << tab << "LUT count increased by " << increment : output << "LUT count decreased by " << increment;
		output << " after adding " << count << " arithmetic operators of width " << width << " and with " << nrInputs << " inputs";
		output << endl;
		
		lutInputs = target->lutInputs();
		it = estimatedLUTTypes.find(lutInputs);
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[lutInputs] = increment;
		}else{
			estimatedLUTTypes[lutInputs] += increment;
		}
		it = estimatedArithOpTypes.find(width);
		if(it == estimatedArithOpTypes.end()){
			estimatedArithOpTypes[width] = count;
		}else{
			estimatedArithOpTypes[width] += count;
		}
		
		return output.str();
	}
	
	//TODO: find a better approximation for the resources
	//		currently just logic corresponding to the multiplexers
	//TODO: find a better approximation for the resources
	//		currently just logic corresponding to the state register
	//TODO: find a better approximation for the resources
	//		for now, RAM blocks are not used
	std::string ResourceEstimationHelper::addFSM(int nrStates, int nrTransitions, int count){
		std::ostringstream output;
		int increment, increment2, increment3;
		int lutInputs;
		map<int, int>::iterator it;
		
		estimatedCountFSM += count;
		
		increment = ceil(count*nrStates*nrTransitions*target->getLUTFromMux(2));
		estimatedCountLUT += increment;
		increment2 = ceil(count*log2(nrStates));
		estimatedCountFF += increment2;
		increment3 = 0;
		estimatedCountRAM += increment3;
		
		(count>0)? output << "FSM count increased by " << count : output << "FSM count decreased by " << count;
		output << endl;
		(increment>0)? output << tab << "LUT count increased by " << increment : output << "LUT count decreased by " << increment;
		output << " after adding " << count << " multiplexers of " << nrStates << " states and with " << nrTransitions << " transitions";
		output << endl;
		(increment2>0)? output << tab << "FF count increased by " << increment2 : output << "FF count decreased by " << increment2;
		output << " after adding " << count << " multiplexers of " << nrStates << " states and with " << nrTransitions << " transitions";
		output << endl;
		(increment3>0)? output << tab << "RAM count increased by " << increment3 : output << "ROM count decreased by " << increment3;
		output << " after adding " << count << " multiplexers of " << nrStates << " states and with " << nrTransitions << " transitions";
		output << endl;
		
		lutInputs = target->lutInputs();
		it = estimatedLUTTypes.find(lutInputs);
		if(it == estimatedLUTTypes.end()){
			estimatedLUTTypes[lutInputs] = increment;
		}else{
			estimatedLUTTypes[lutInputs] += increment;
		}
		
		return output.str();
	}
	
	//--Resource usage statistics---------------------------------------
	std::string ResourceEstimationHelper::generateStatistics(int detailLevel){
		std::ostringstream result;
		
		
		result << "=========================================================================" << endl;
		result << "*                     Resource Estimation Report                        *" << endl;
		result << "=========================================================================" << endl;
		
		result << endl;
		result << "Top level entity name                : " << parentOp->getName() << endl;
		result << "Top level entity is pipelined        : " << ((target->isPipelined()) ? "True" : "False") << endl;
		result << "Top level entity target frequency    : " << ((target->isPipelined()) ? join("", target->frequencyMHz()) : "N/A") << endl;
		result << "Top level entity uses DSP blocks     : " << ((target->useHardMultipliers()) ? "True" : "False") << endl;
		result << endl;
		result << "Number of Flip-Flops                 : " << ((estimatedCountFF) ? join("", estimatedCountFF) : "None") << endl;
		result << "Number of Function Generators        : " << ((estimatedCountLUT) ? join("", estimatedCountLUT) : "None") << endl;
		result << "Number of Multipliers                : " << ((estimatedCountMultiplier) ? join("", estimatedCountMultiplier) : "None") << endl;
		result << "Number of Memory blocks              : " << ((estimatedCountMemory) ? join("", estimatedCountMemory) : "None") << endl;
		result << endl;
		if(detailLevel>0){
		(estimatedCountDSP)  ?	result << "Number of DSP blocks                 : " << estimatedCountDSP << endl : result << "";
		(estimatedCountRAM)  ?	result << "Number of RAM blocks                 : " << estimatedCountRAM << endl : result << "";
		(estimatedCountROM)  ?	result << "Number of ROM blocks                 : " << estimatedCountROM << endl : result << "";
		(estimatedCountSRL)  ?	result << "Number of SRLs                       : " << estimatedCountSRL << endl : result << "";
		(estimatedCountWire) ?	result << "Number of Wires                      : " << estimatedCountWire << endl: result << "";
		(estimatedCountIOB)  ?	result << "Number of IOs                        : " << estimatedCountIOB << endl : result << "";
		result << endl;
		}
		if(detailLevel>1){
		(estimatedCountMux) 		?	result << "Number of Multiplexers               : " << estimatedCountMux << endl : result << "";
		(estimatedCountCounter) 	?	result << "Number of Counters                   : " << estimatedCountCounter << endl : result << "";
		(estimatedCountAccumulator) ?	result << "Number of Accumulators               : " << estimatedCountAccumulator << endl : result << "";
		(estimatedCountDecoder)		?	result << "Number of Decoders/Encoders          : " << estimatedCountDecoder << endl : result << "";
		(estimatedCountArithOp)		?	result << "Number of Arithmetic Operators       : " << estimatedCountArithOp << endl : result << "";
		(estimatedCountAdderSubtracter) ? result << "Number of Adders/Subtracters         : " <<  estimatedCountAdderSubtracter << endl : result << "";
		(estimatedCountReg)			?	result << "Number of registers                  : " << estimatedCountReg << endl : result << "";
		(estimatedCountFSM)			?	result << "Number of FSMs                       : " << estimatedCountFSM << endl : result << "";
		result << endl;
		}
		
		if(detailLevel>2){
		result << "=========================================================================" << endl;
		result << "*                      Detailed Resource Usage                          *" << endl;
		result << "=========================================================================" << endl;
		
		result << endl;
		if(estimatedCountLUT>0){
			result << "Number of Function Generators                     : " << estimatedCountLUT << endl;
			for(map<int, int>::iterator it = estimatedLUTTypes.begin(); it !=estimatedLUTTypes.end(); it++) {
				result << tab << "*Number of " << it->first << " input function generators         : " << it->second << endl;		
			}
		}
		if(estimatedCountMultiplier>0){
			result << "Number of Multipliers                             : " << estimatedCountMultiplier << endl;
			for(map<int, int>::iterator it = estimatedMultiplierTypes.begin(); it !=estimatedMultiplierTypes.end(); it++) {
				result << tab << "*Number of multipliers of width " << it->first << "         " << ((it->first>=1000) ? " : " : (it->first>=100) ? "  : " : (it->first>=10) ? "    : " : "     : ") << it->second << endl;
			}
		}
		if(estimatedCountSRL>0){
			result << "Number of SRLs                                    : " << estimatedCountSRL << endl;
			for(map<int, int>::iterator it = estimatedShifterTypes.begin(); it !=estimatedShifterTypes.end(); it++) {
				result << tab << "*Number of shift registers of width " << it->first << "     " << ((it->first>=1000) ? " : " : (it->first>=100) ? "  : " : (it->first>=10) ? "    : " : "     : ") << it->second << endl;		
			}
		}
		if(estimatedCountCounter>0){
			result << "Number of Counters                                : " << estimatedCountCounter << endl;
			for(map<int, int>::iterator it = estimatedCounterTypes.begin(); it !=estimatedCounterTypes.end(); it++) {
				result << tab << "*Number of counters of width " << it->first << "            " << ((it->first>=1000) ? " : " : (it->first>=100) ? "  : " : (it->first>=10) ? "    : " : "     : ") << it->second << endl;		
			}
		}	
		if(estimatedCountArithOp>0){
			result << "Number of Arithmetic Operators                    : " << estimatedCountArithOp << endl;
			for(map<int, int>::iterator it = estimatedArithOpTypes.begin(); it !=estimatedArithOpTypes.end(); it++) {
				result << tab << "*Number of arithmetic operators of width " << it->first << "" << ((it->first>=1000) ? " : " : (it->first>=100) ? "  : " : (it->first>=10) ? "    : " : "     : ") << it->second << endl;
			}
		}
		if(estimatedCountAdderSubtracter>0){
			result << "Number of Adders/Subtracters                      : " <<  estimatedCountAdderSubtracter << endl;
			for(map<int, int>::iterator it = estimatedAdderTypes.begin(); it !=estimatedAdderTypes.end(); it++) {
				result << tab << "*Number of adders/subtracters of width " << it->first << "  " << ((it->first>=1000) ? " : " : (it->first>=100) ? "  : " : (it->first>=10) ? "    : " : "     : ") << it->second << endl;
			}
		}
		if(estimatedCountReg>0){
			result << "Number of registers                               : " << estimatedCountReg << endl;
			for(map<int, int>::iterator it = estimatedRegisterTypes.begin(); it !=estimatedRegisterTypes.end(); it++) {
				result << tab << "*Number of registers of width " << it->first << "          " << ((it->first>=1000) ? " : " : (it->first>=100) ? "  : " : (it->first>=10) ? "    : " : "     : ") << it->second << endl;
			}
		}			
		result << endl;
		}
		
		result << "=========================================================================" << endl;
		
		return result.str();
	}
	
	//--Utility functions related to the generation of resource usage statistics-----------------------
	
	//TODO: find a more precise way to determine the required number of
	//		registers due to pipeline
	std::string ResourceEstimationHelper::addPipelineFF(){
		std::ostringstream output;
		int increment = 0;
		
		output << "FF count being compensated for " << endl;
		
		for(unsigned int i=0; i<(parentOp->getSignalList()).size(); i++) {
			Signal *s = (parentOp->getSignalList())[i];
			if ((s->type() == Signal::registeredWithoutReset) || (s->type() == Signal::wire) 
					|| (s->type() == Signal::registeredWithAsyncReset) || (s->type() == Signal::registeredWithSyncReset)) 
				if(s->getLifeSpan() >0) {
					if(target->isPipelined() && parentOp->isSequential()){
						increment += s->getLifeSpan() * s->width();
						output << tab << "Added " << s->getLifeSpan()*s->width() << " FFs for signal " << s->getName() << " having width " << s->width() << " and life-span of " << s->getLifeSpan() << " (pipelined target)" << endl;
					}else{
						increment += s->width();
						output << tab << "Added " << s->width() << " FFs for signal " << s->getName() << " having width " << s->width() << " and life-span of " << s->getLifeSpan() << " (non-pipelined target)" << endl;
					}
				}
		}
		estimatedCountFF += increment;
		
		output << "FF count compensated for by " << increment << " with pipeline related registers." << endl;
		return output.str();
	}
	
	std::string ResourceEstimationHelper::addWireCount(){
		std::ostringstream output;
		int increment = 0;
		
		for(unsigned int i=0; i<(parentOp->getSignalList()).size(); i++) {
			Signal *s = (parentOp->getSignalList())[i];
			bool processed = false;
			
			for(unsigned int j=0; j<estimatedSignalNames.size(); i++) {
				std::string stemp = estimatedSignalNames[j];
				if((s->getName()).compare(stemp)){
					processed = true;
					break;
				}
			}
			if(processed)
				continue;
			increment += s->width();
		}
		estimatedCountWire += increment;
		
		output << "Wire count compensated for by " << increment << " with wires from signal declarations." << endl;
		return output.str();
	}
	
	std::string ResourceEstimationHelper::addPortCount(){
		std::ostringstream output;
		int increment = 0;
		
		for(unsigned int i=0; i<(parentOp->getIOListV()).size(); i++) {
			Signal *s = (parentOp->getIOListV())[i];
			bool processed = false;
			
			for(unsigned int j=0; j<estimatedPortNames.size(); i++) {
				std::string stemp = estimatedPortNames[j];
				if((s->getName()).compare(stemp)){
					processed = true;
					break;
				}
			}
			if(processed)
				continue;
			increment += s->width();
		}
		estimatedCountIOB += increment;
		
		output << "Port count compensated for by " << increment << " with ports from port declarations." << endl;
		return output.str();
	}
	
	
	//TODO: add function to add resource count from specified component
	std::string ResourceEstimationHelper::addComponentResourceCount(){
		std::ostringstream output;
		map<string, Operator*> subComponents = parentOp->getSubComponents();
		
		output << "Resource count is being compensated for with resource estimations from subcomponents." << endl;
		for(map<string, Operator*>::iterator it = subComponents.begin(); it != subComponents.end(); it++) {
			Operator *op = it->second;
			
			estimatedCountFF 				+= op->reHelper->estimatedCountFF;
			estimatedCountLUT 				+= op->reHelper->estimatedCountLUT;
			estimatedCountMultiplier 		+= op->reHelper->estimatedCountMultiplier;
			
			estimatedCountDSP 				+= op->reHelper->estimatedCountDSP;
			estimatedCountRAM 				+= op->reHelper->estimatedCountRAM;
			estimatedCountROM 				+= op->reHelper->estimatedCountROM;
			estimatedCountSRL 				+= op->reHelper->estimatedCountSRL;
			estimatedCountWire 				+= op->reHelper->estimatedCountWire;
			estimatedCountIOB 				+= op->reHelper->estimatedCountIOB;
			
			estimatedCountMux 				+= op->reHelper->estimatedCountMux;
			estimatedCountCounter	 		+= op->reHelper->estimatedCountCounter;
			estimatedCountAccumulator 		+= op->reHelper->estimatedCountAccumulator;
			estimatedCountDecoder 			+= op->reHelper->estimatedCountDecoder;
			estimatedCountArithOp 			+= op->reHelper->estimatedCountArithOp;
			estimatedCountAdderSubtracter 	+= op->reHelper->estimatedCountAdderSubtracter;
			estimatedCountFSM 				+= op->reHelper->estimatedCountFSM;
			
			if(op->reHelper->estimatedCountLUT>0){
				for(map<int, int>::iterator itTypesImport = op->reHelper->estimatedLUTTypes.begin(); itTypesImport != op->reHelper->estimatedLUTTypes.end(); itTypesImport++) {
					map<int, int>::iterator itTypes;
					
					itTypes = estimatedLUTTypes.find(itTypesImport->first);
					if(itTypes == estimatedLUTTypes.end()){
						estimatedLUTTypes[itTypesImport->first] = itTypesImport->second;
					}else{
						estimatedLUTTypes[itTypesImport->first] += itTypesImport->second;
					}
				}
			}
			if(op->reHelper->estimatedCountAdderSubtracter>0){ 
				for(map<int, int>::iterator itTypesImport = op->reHelper->estimatedAdderTypes.begin(); itTypesImport != op->reHelper->estimatedAdderTypes.end(); itTypesImport++) {
					map<int, int>::iterator itTypes;
					
					itTypes = estimatedAdderTypes.find(itTypesImport->first);
					if(itTypes == estimatedAdderTypes.end()){
						estimatedAdderTypes[itTypesImport->first] = itTypesImport->second;
					}else{
						estimatedAdderTypes[itTypesImport->first] += itTypesImport->second;
					}
				}
			}
			if(op->reHelper->estimatedCountMultiplier>0){
				for(map<int, int>::iterator itTypesImport = op->reHelper->estimatedMultiplierTypes.begin(); itTypesImport != op->reHelper->estimatedMultiplierTypes.end(); itTypesImport++) {
					map<int, int>::iterator itTypes;
					
					itTypes = estimatedMultiplierTypes.find(itTypesImport->first);
					if(itTypes == estimatedMultiplierTypes.end()){
						estimatedMultiplierTypes[itTypesImport->first] = itTypesImport->second;
					}else{
						estimatedMultiplierTypes[itTypesImport->first] += itTypesImport->second;
					}
				}
			}
			if(op->reHelper->estimatedCountReg>0){
				for(map<int, int>::iterator itTypesImport = op->reHelper->estimatedRegisterTypes.begin(); itTypesImport != op->reHelper->estimatedRegisterTypes.end(); itTypesImport++) {
					map<int, int>::iterator itTypes;
					
					itTypes = estimatedRegisterTypes.find(itTypesImport->first);
					if(itTypes == estimatedLUTTypes.end()){
						estimatedRegisterTypes[itTypesImport->first] = itTypesImport->second;
					}else{
						estimatedRegisterTypes[itTypesImport->first] += itTypesImport->second;
					}
				}
			}
			if(op->reHelper->estimatedCountSRL>0){
				for(map<int, int>::iterator itTypesImport = op->reHelper->estimatedShifterTypes.begin(); itTypesImport != op->reHelper->estimatedShifterTypes.end(); itTypesImport++) {
					map<int, int>::iterator itTypes;
					
					itTypes = estimatedShifterTypes.find(itTypesImport->first);
					if(itTypes == estimatedShifterTypes.end()){
						estimatedShifterTypes[itTypesImport->first] = itTypesImport->second;
					}else{
						estimatedShifterTypes[itTypesImport->first] += itTypesImport->second;
					}
				}
			}
			if(op->reHelper->estimatedCountCounter>0){
				for(map<int, int>::iterator itTypesImport = op->reHelper->estimatedCounterTypes.begin(); itTypesImport != op->reHelper->estimatedCounterTypes.end(); itTypesImport++) {
					map<int, int>::iterator itTypes;
					
					itTypes = estimatedCounterTypes.find(itTypesImport->first);
					if(itTypes == estimatedCounterTypes.end()){
						estimatedCounterTypes[itTypesImport->first] = itTypesImport->second;
					}else{
						estimatedCounterTypes[itTypesImport->first] += itTypesImport->second;
					}
				}
			}
			if(op->reHelper->estimatedCountArithOp>0){
				for(map<int, int>::iterator itTypesImport = op->reHelper->estimatedArithOpTypes.begin(); itTypesImport != op->reHelper->estimatedArithOpTypes.end(); itTypesImport++) {
					map<int, int>::iterator itTypes;
					
					itTypes = estimatedArithOpTypes.find(itTypesImport->first);
					if(itTypes == estimatedArithOpTypes.end()){
						estimatedArithOpTypes[itTypesImport->first] = itTypesImport->second;
					}else{
						estimatedArithOpTypes[itTypesImport->first] += itTypesImport->second;
					}
				}
			}
			
			output << endl << op->resourceEstimate.str();
		}
		
		output << "Resource count compensated for with resource estimations from subcomponents." << endl;
		return output.str();
	}
	
	std::string ResourceEstimationHelper::addAutomaticResourceEstimations(){
		std::ostringstream result;
		
		result << addPipelineFF();
		result << addWireCount();
		result << addPortCount();
		result << addComponentResourceCount();
		
		return result.str();
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////
}
