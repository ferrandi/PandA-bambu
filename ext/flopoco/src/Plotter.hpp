/*
   A class used for plotting various drawings in SVG format

   This file is part of the FloPoCo project
   developed by the Arenaire team at Ecole Normale Superieure de Lyon

Author : Florent de Dinechin, Kinga Illyes, Bogdan Popa

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
2012.
All rights reserved.

*/

#ifndef __PLOTTER_HPP
#define __PLOTTER_HPP

#include <vector>
#include <list>
#include <iostream>
#include <fstream>

#include "BitHeap.hpp"
#include "WeightedBit.hpp"
#include "IntMultipliers/MultiplierBlock.hpp"


namespace flopoco
{

	class Plotter
	{

	public:
	
		class Snapshot
		{
			public:

				Snapshot(vector<list<WeightedBit*> > bitheap, int minWeight_, int maxWeight_, unsigned maxHeight,  
						bool didCompress_,  int cycle, double cp);

				~Snapshot(){};

				/** ordering by availability time */
				friend bool operator< (Snapshot& b1, Snapshot& b2);
				/** ordering by availability time */
				friend bool operator<= (Snapshot& b1, Snapshot& b2);
				/** ordering by availability time */
				friend bool operator== (Snapshot& b1, Snapshot& b2);
				/** ordering by availability time */
				friend bool operator!= (Snapshot& b1, Snapshot& b2);
				/** ordering by availability time */
				friend bool operator> (Snapshot& b1, Snapshot& b2);
				/** ordering by availability time */
				friend bool operator>= (Snapshot& b1, Snapshot& b2);

				//unsigned getMaxHeight();

				vector<list<WeightedBit*> > bits;
				int maxWeight;
				int minWeight;
				unsigned maxHeight;
				bool didCompress; 
				int cycle;
				double cp;
				string srcFileName;
		};
		
		/** 
		 * constructor 
		 */
		Plotter(BitHeap* bh_);

		/** 
		 * destructor 
		 */
		~Plotter();

		/** 
		 * takes a snapshot of the bitheap's current state 
		 */
		void heapSnapshot(bool compress, int cycle, double cp);

		/** 
		 * plots all the bitheap's stages 
		 */
		void plotBitHeap();

		/** 
		 * plots multiplier area and sheared views 
		 */
		void plotMultiplierConfiguration(string name, vector<MultiplierBlock*> mulBlocks, int wX, int wY, int wOut, int g);

		void setBitHeap(BitHeap* bh_);

		void addSmallMult(int topX, int topY, int dx, int dy);


		stringstream ss;


	private:
	
		/** 
		 * draws the initial bitheap 
		 */
		void drawInitialHeap();

		/** 
		 * draws the stages of the heap compression 
		 */
		void drawCompressionHeap();

		/** 
		 * draws the area view of the DSP configuration 
		 */
		void drawAreaView(string name, vector<MultiplierBlock*> mulBlocks, int wX, int wY, int wOut, int g);

		/** 
		 * draws the sheared view of the DSP configuration 
		 */
		void drawLozengeView(string name, vector<MultiplierBlock*> mulBlocks, int wX, int wY, int wOut, int g);

		/** 
		 * draws a line between the specified coordinates 
		 */
		void drawLine(int wX, int wY, int wRez, int offsetX, int offsetY, int scalingFactor, bool isRectangle);

		/** 
		 * draws a DSP block 
		 */
		void drawDSP(int wXY,  int wY, int i, int xT, int yT, int xB, int yB, int offsetX, int offsetY, int scalingFactor,  bool isRectangle);

		/** 
		 * draws the target rectangle or lozenge 
		 */
		void drawTargetFigure(int wX, int wY, int offsetX, int offsetY, int scalingFactor, bool isRectangle);

		/** 
		 * draws a small multiplier table 
		 */
		void drawSmallMult(int wX, int wY, int xT, int yT, int xB, int yB, int offsetX, int offsetY, int scalingFactor,  bool isRectangle);

		void initializeHeapPlotting(bool isInitial);

		void drawInitialConfiguration( vector<list<WeightedBit*> > bits, int maxWeight, int offsetY, int turnaroundX);

		void drawConfiguration(vector<list<WeightedBit*> > bits, unsigned nr, int cycle, double cp, 
				int maxWeight, int offsetY, int turnaroundX, bool timeCondition);

		/** 
		 * draws a single bit 
		 */
		void drawBit(int cnt, int w, int turnaroundX, int offsetY, int color, int cycle, int cp, string name);

		void addECMAFunction();

		ofstream fig;
		ofstream fig2;
		//			vector<vector<list<WeightedBit*> > > snapshots;
		vector<Snapshot*> snapshots;

		int topX[10000];
		int topY[10000];
		int dx, dy;
		int smallMultIndex;

		string srcFileName;

		int stagesPerCycle;
		int lastStage;

		double elementaryTime;

		BitHeap* bh;
	};
}

#endif
