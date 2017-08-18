/*
  A class to manage heaps of weighted bits in FloPoCo
  
  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2012.
  All rights reserved.

*/

#include "WeightedBit.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>	

#include "utils.hpp"
#include <vector>
#include <list>

using namespace std;


namespace flopoco
{

	WeightedBit::WeightedBit(int guid, int _uid, int weight_, int type_, int cycle_, double criticalPath_) : 
		cycle(cycle_),  criticalPath(criticalPath_), weight(weight_), type(type_), uid(_uid)
	{
		srcFileName="WeightedBit";
		std::ostringstream p;
		p  << "heap_bh" << guid << "_w" << weight << "_" << _uid;
		name=p.str();		
	}



	WeightedBit::WeightedBit(WeightedBit* bit) {
		srcFileName="WeightedBit";
		cycle = bit->cycle;
		criticalPath = bit->criticalPath;
		weight = bit-> weight;
		type = bit->type;
		uid = bit->uid;
		name = bit->name;
	}

	WeightedBit::WeightedBit(WeightedBit* bit, int deathCycle,  double deathCP, string killerCompressor) {
		srcFileName="WeightedBit";
		cycle = bit->cycle;
		criticalPath = bit->criticalPath;
		weight = bit-> weight;
		type = bit->type;
		uid = bit->uid;
		name = bit->name;
		deathCycle =deathCycle;
		deathCP = deathCP; 
		killerCompressor = killerCompressor;
	}


	double WeightedBit::getCriticalPath(int atCycle)
	{
#if 0
		if (cycle>atCycle){
			THROWERROR("For bit " << name << ", getCriticalPath() called for cycle "<<atCycle<< " but this bit is defined only at cycle "<< cycle);
		}
#endif
		if (cycle==atCycle)
			return criticalPath;
		if (cycle<atCycle)
			return 0.0;
		
		return 0.0;  //because it returned no value on this branch
	}

	int WeightedBit::computeStage(int stagesPerCycle, double elementaryTime)
	{
		return (getCycle()*stagesPerCycle + getCriticalPath(getCycle())/elementaryTime);        
	}


	
	/* which bit was defined earlier */
	bool operator< (WeightedBit& b1, WeightedBit& b2){
		//cout <<"**** <" << endl;
		return ((b1.cycle<b2.cycle) || (b1.cycle==b2.cycle && b1.criticalPath<b2.criticalPath));
	} 

	bool operator<= (WeightedBit& b1, WeightedBit& b2){
		//cout <<"**** <=" << endl;
		return ((b1.cycle<b2.cycle) || (b1.cycle==b2.cycle && b1.criticalPath<=b2.criticalPath));
	} 
	
	bool operator> (WeightedBit& b1, WeightedBit& b2){
		//cout <<"**** >" << endl;
		return ((b1.cycle>b2.cycle) || (b1.cycle==b2.cycle && b1.criticalPath>b2.criticalPath));
	} 

	bool operator>= (WeightedBit& b1, WeightedBit& b2){
		//cout <<"**** >=" << endl;
		return ((b1.cycle>b2.cycle) || (b1.cycle==b2.cycle && b1.criticalPath>=b2.criticalPath));
	} 
	
	bool operator== (WeightedBit& b1, WeightedBit& b2){
		//cout <<"**** ==" << endl;
		return ((b1.cycle==b2.cycle) && (b1.criticalPath==b2.criticalPath));
	} 
	
	bool operator!= (WeightedBit& b1, WeightedBit& b2){
		return  ((b1.cycle!=b2.cycle) || (b1.criticalPath!=b2.criticalPath));
	} 
	
	

	
}
