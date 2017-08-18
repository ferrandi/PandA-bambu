/*
  A class to manage heaps of weighted bits in FloPoCo

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr, Kinga Illyes, Bogdan Popa

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2012.
  All rights reserved.

*/
#ifndef __BITHEAP_HPP
#define __BITHEAP_HPP
#include <vector>
#include <sstream>
#include "WeightedBit.hpp"
#include "Operator.hpp"
#include "Table.hpp"
#include "DualTable.hpp"
#include "IntAdder.hpp"
#include "IntAddition/BasicCompressor.hpp"
#include "IntMultipliers/MultiplierBlock.hpp"

// To enable SVG plotting, uncomment the following line
 #define BITHEAP_GENERATE_SVG 1
 
 #define COMPRESSION_TYPE 0


/* 
   Each bit in the bit heap is flagged with the cycle at which it is produced.
   Compression works as follows:

   First check if there are DSP blocks, chain them into supertiles, generate the corresponding VHDL, and add the result to the bit heap
   Then, compress the bit heap




   setCycle(0)
   while needed
   manageCriticalPath() to advance the cycle  
   consider the subset of bits with cycles 0 to getCycle()
   build the longest possible heap of size 2 or less from the right,
   and feed them to an adder.
   compress the others bits greedily, 
   adding more bits, flagged with the current cycle, 
   setting the cycle of the consumed bits to -1 so they won't be considered in subsequent iterations

*/

namespace flopoco{



	class Plotter;

	class BitHeap 
	{


	public:

		/** The constructor
		    @param op                the operator in which this bit heap is beeing built
		    @param maxWeight         the maximum weight of the heap (it should be known statically, shouldn't it?)
		    @param enableSuperTiles  if true, the bit heap compression will try and supertile DSP blocks   
		    @param name              a description of the heap that will be integrated into its unique name
		    @param compressionType	 the type of compression applied to the bit heap: 
										0 = using only compressors (default),
										1 = using only an adder tree,
										2 = using a mix of the two, with an addition tree at the end of the compression
		*/
		BitHeap(Operator* op, int maxWeight, bool enableSuperTiles = true, string name = "", int compressionType = COMPRESSION_TYPE);
		~BitHeap();

		/** add a bit to the bit heap. The bit will be added at the cycle op->currentCycle() with critical path op->getCriticalPath().
		    @param weight   the weight of the bit to be added. It should be positive.
		    @param rhs      the right-hand VHDL side defining this bit.
		    @param comment  a VHDL comment for this bit
		    @param type     shows the origin(type) of the bit:
		    0 - compression
		    1 - external
		    2 - constant */		
		void addBit(int weight, string rhs, string comment="", int type=1);

		/** add a constant 1 to the bit heap. All the constant bits are added to the constantBits mpz, so we don't generate hardware to compress constants....
		    @param weight   the weight of the 1 to be added */
		void addConstantOneBit(int weight); 

		/** "remove" a constant 1 from the bit heap. 
		    @param weight   the weight of the 1 to be added */
		void subConstantOneBit(int weight); 
		
		/** add a constant to the bit heap. It will be added to the constantBits mpz, so we don't generate hardware to compress constants....
		    @param weight   the weight of the LSB of c (or, where c should be added)
		    @param c        the value to be added */
		void addConstant(int weight, mpz_class c); 

		/** add to the bit heap the value held by a signal, considered as an unsigned integer */
		void addUnsignedBitVector(int weight, string x, unsigned size);
		
		/** add to the bit heap the opposite of the value held by a signal, considered as an unsigned integer */
		void subtractUnsignedBitVector(int weight, string x, unsigned size);

		/** add to the bit heap the value held by a signal, considered as a signed integer. size includes the sign bit */
		void addSignedBitVector(int weight, string x, unsigned size);

		/** add to the bit heap the opposite of the value held by a signal, considered as a signed integer. size includes the sign bit */
		void subtractSignedBitVector(int weight, string x, unsigned size);


		/** generate the VHDL for the bit heap. To be called last by operators using BitHeap.*/
		void generateCompressorVHDL();

		/** returns the name of the compressed sum */
		string getSumName();

		/** returns the current stage of the bitheap, given the global cycle and CP */
		int computeStage();


		/** adds a new MultiplierBlock */
		void  addMultiplierBlock(MultiplierBlock* m);



		/** search for the possible chainings and build supertiles*/
		void buildSupertiles();

		/**  generates the VHDL code for the supertiles*/
		void generateSupertileVHDL();
		
		/**
		 * Generate the code VHDL for the process which implements the supertile,
		 * in order to have the addition inferred inside the DSP block for Altera 
		 * architectures.
		 */
		void generateAlteraSupertileVHDL(MultiplierBlock* x, MultiplierBlock* y, string resultName);


		/** returns the maximum weight of the bit heap */
		unsigned getMaxWeight() {return maxWeight; };

		unsigned getMinWeight() {return minWeight; };

		/** returns the maximum height of the bit heap*/
		unsigned getMaxHeight();

		int getStagesPerCycle() {return stagesPerCycle;};

		double getElementaryTime() {return elementaryTime;};

		Plotter* getPlotter();

		Operator* getOp() {return op;};

		/** return the UID of the bit heap*/
		int getGUid();

		/** return the UID of the bit heap*/
		string getName() {return uniqueName_;};


		void setSignedIO(bool s){this->signedIO=s;};

		bool getSignedIO() {return signedIO;};
	protected:


		void elemReduce(unsigned i, BasicCompressor* bc, int type=0);

		//applies a 3_2 compressor to the column sent as parameter
		void applyCompressor3_2(int col);

		//applies an adder with wIn = col1-col0+1; col0 always has size=3 and the other columns (includind col1) have size=2
		void applyAdder(int col0, int col1, bool hasCin=true);
		
		/**
		 * compress the remaining columns using adders
		 */
		void applyAdderTreeCompression();

		/** returns a pointer to the  latest bit from the inputs to a compressor applied to the bottom of a bit heap
		  w is the weight, c0 and c1 are the input heights of the compressor, e.g. 3,0 for a full adder */
		WeightedBit* latestInputBitToCompressor(unsigned w, int c0, int c1);

		/** 
		 * computes the latest bit from the bitheap, in order to manage the cycle before the final adding
		 */
		WeightedBit* getLatestBit(int lsbColumn, int msbColumn);

		WeightedBit* getFirstSoonestBit();

		/** remove a bit from the bitheap.
		    @param weight  the weight of the bit to be removed
		    @param dir if dir==0 the bit will be removed from the begining of the list 
		    if dir==1 the bit will be removed from the end of the list
		*/
		void removeBit(unsigned weight, int dir);

		/** get the parent operator */



		/** generate the final adder for the bit heap (when the columns height is maximum 2*/
		void generateFinalAddVHDL(bool isXilinx);



		/**
		 * Compress the bitheap using compressors
		 * @param stage: the compression stage
		 **/
		void compress(int stage);

		/** return the current height a column (bits not yet compressed) */
		unsigned currentHeight(unsigned w);

		/** return a fresh uid for a bit of weight w*/
		int newUid(unsigned w);

			


		/** counts the bits not processed yet in wb */
		int count(list<WeightedBit*> wb, int cycle);

		void printColumnInfo(int w);

		void generatePossibleCompressors();

		/** remove the compressed bits */
		void removeCompressedBits(int c, int red);


		/** generate the VHDL code for 1 dsp */ 
		void generateVHDLforDSP(MultiplierBlock* m, int uid,int i);

		void initializeDrawing();

		void closeDrawing(int offsetY);

		void drawConfiguration(int offsetY);

		void drawBit(int cnt, int w, int turnaroundX, int offsetY, int c);

		void concatenateLSBColumns();

		void printBitHeapStatus();

	public: // TODO privatize
		vector<list<WeightedBit*> > bits; 			/**<  Each list is ordered by arrival time of the bits, i.e. lexicographic order on (cycle, cp). 
															During the generation of the compressor, bits are added and removed to these lists */
		vector<list<WeightedBit*> > history; 		/**<  remembers all the changes to bits */
	private:
		Operator* op;
		int compressionType;						/**< The type of compression performed (explained in the header of the constructor)*/
		unsigned maxWeight;							/**< The compressor tree will produce a result for weights < maxWeight (work modulo 2^maxWeight)*/
		unsigned minWeight;							/**< bits smaller than this one are already compressed */    
		mpz_class constantBits;						/**< This int gather all the constant bits that need to be added to the bit heap (for rounding, two's complement etc) */
		vector<BasicCompressor *> possibleCompressors;
		bool usedCompressors[100];					/** the list of compressors which were used at least once*/ // 100 should be more than enough for everybody
		BasicCompressor * halfAdder;
		BasicCompressor * fullAdder;
		unsigned chunkDoneIndex; 
		unsigned inConcatIndex;						/** input index - to form the inputsignals of the compressor*/
		unsigned outConcatIndex;					/** output index - to form the outputsignals of the compressor*/
		unsigned compressorIndex;					/** the index of the instance of compressors*/
		unsigned adderIndex;						/** the index of the instance of IntAdder*/
		unsigned cnt[100000];						/** number of bits which will be compressed in the current iteration*/
		vector<int> uid;							/**< unique id, per weight */
		int guid;									/**< global uid  for this bit heap, useful in operators managing several bit heaps */
		ofstream fileFig;
		ostringstream fig;
		bool drawCycleLine;
		int drawCycleNumber;    
		int stagesPerCycle;  
		double elementaryTime; 
		bool didCompress; 
		vector<MultiplierBlock*> mulBlocks; //the vector of multiplier blocks
		Plotter* plotter;	
		// For error reporting to work
		string srcFileName;
		string uniqueName_;
		// TODO? signedIO should be managed multiplier block per multiplier block.
		bool signedIO;								/**< true if the uncompressed multiplier blocks have signed IO*/  
		int plottingStage;
		int plottingCycle;
		double plottingCP;
		unsigned int adder3Index;					/**< The index of the ternary adders */
		unsigned int minAdd3Length;					/**< The minimum length of a 3-input adder */
		unsigned int maxAdd3Length;					/**< The maximum length of a 3-input adder */
		bool enableSuperTiles;
	};


}
#endif
