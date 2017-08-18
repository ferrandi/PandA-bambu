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
#ifndef __WEIGHTEDBIT_HPP
#define __WEIGHTEDBIT_HPP
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <math.h>	


using namespace std;

namespace flopoco{



	class WeightedBit
		{
		public:
		
			/** ordering by availability time */
			friend bool operator< (WeightedBit& b1, WeightedBit& b2); 
			/** ordering by availability time */
			friend bool operator<= (WeightedBit& b1, WeightedBit& b2);
			friend bool operator> (WeightedBit& b1, WeightedBit& b2); 
			/** ordering by availability time */
			friend bool operator>= (WeightedBit& b1, WeightedBit& b2);
			/** equality for availability time */
			friend bool operator== (WeightedBit& b1, WeightedBit& b2);
			/** difference for availability time */
			friend bool operator!= (WeightedBit& b1, WeightedBit& b2);




			/** standard constructor   */ 
			WeightedBit(int bitHeapId, int uid, int weight, int type, int cycle,  double criticalPath);


			/** This clone constructor is used by the copies in snapshots made by Plotter*/
			WeightedBit(WeightedBit* bit);


			/** Constructor for bits that go to history */
			WeightedBit(WeightedBit* bit, int deathCycle,  double DeathCP, string killerCompressor);


			/** destructor */ 
			~WeightedBit(){};
		

			/** return the cycle at which this bit is defined */
			int getCycle(){
				return cycle;
			};

			/** return the critical path of this bit */
			double getCriticalPath(int atCycle);

			/** returns the stage when this bit should be compressed */ 
			int computeStage(int stagesPerCycle, double elementaryTime);




			

			/** return the VHDL signal name of this bit */
			string getName(){
				return name;
			};

			int getWeight(){return weight;};

			int getType(){return type;};

			int getUid(){return uid;};



		private:
			int cycle;  /**< The cycle at which the bit was created */
			double criticalPath; /**< The cycle at which the bit was created */
			int weight;
			int type;
			int uid;
			std::string name;
			std::string srcFileName;
			std::string uniqueName_;
			
			// Stuff for post-mortem plotting
			int deathCycle; /**< The cycle at which the bit is removed from the bit heap */ 
			int deathCP; /**< The critical path at which the bit is removed from the bit heap */ 
			string killerCompressor; /**< the instance name of the compressor that input this bit*/
		};

}
#endif
