/*
  A class to manage heaps of weighted bits in FloPoCo

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr, Kinga Illyes, Bogdan Popa

  Initial software.
  Copyright Š ENS-Lyon, INRIA, CNRS, UCBL,
  2012.
  All rights reserved.

*/

// To enable SVG plotting, #define BITHEAP_GENERATE_SVG in BitHeap.hpp


#include "BitHeap.hpp"
#include "Plotter.hpp"
#include <iostream>
#include <fstream>

#include <math.h>
#include <string>
#include "utils.hpp"
#include <vector>
#include <list>

using namespace std;


namespace flopoco
{

	BitHeap::BitHeap(Operator* op, int maxWeight, bool enableSuperTiles, string name, int compressionType) :
		op(op), maxWeight(maxWeight), compressionType(compressionType), enableSuperTiles(enableSuperTiles)
	{
		// Set up the vector of lists of weighted bits, and the vector of uids
		srcFileName=op->getSrcFileName() + ":BitHeap"; // for REPORT to work
		guid = Operator::getNewUId();
		stringstream s;
		s << op->getName() << "_BitHeap"<< name << "_" <<guid; // for REPORT to work

		uniqueName_=s.str();
		REPORT(DEBUG, "Creating BitHeap of size " << maxWeight);
		chunkDoneIndex=0;
		inConcatIndex=0;
		outConcatIndex=0;
		compressorIndex=0;
		adderIndex=0;
		for(int i=0; i<100;++i)
			usedCompressors[i]=false;
		for (int i=0; i< maxWeight; i++) {
			uid.push_back(0);
			list<WeightedBit*> t;
			bits.push_back(t);
		}

		adder3Index = 0;
		minAdd3Length = 4;
		maxAdd3Length = 32;
		minWeight=0;
		plottingStage=0;
		plottingCycle=op->getCurrentCycle();
		plottingCP=op->getCriticalPath();
		elementaryTime = op->getTarget()->lutDelay()    + op->getTarget()->localWireDelay();
		stagesPerCycle = (1/op->getTarget()->frequency()) / elementaryTime;
		for(int i=0; i<100000;++i)
			cnt[i]=0;

		// create a Plotter object for the SVG output
		plotter = new Plotter(this);
		halfAdder = NULL;
		fullAdder = NULL;
	}


	BitHeap::~BitHeap()
	{
		for(unsigned i=0; i<bits.size(); i++){
			list<WeightedBit*>& l = bits[i];
			list<WeightedBit*>::iterator it;
			for(it=l.begin(); it!=l.end(); it++){
				delete (*it);
			}
		}
	}


	Plotter* BitHeap::getPlotter()
	{
		return plotter;
	}


	int BitHeap::newUid(unsigned w){
		return uid[w]++;
	}

	int BitHeap::getGUid(){
		return guid;
	}


	void BitHeap::addConstantOneBit(int weight) {
		if (weight<0)
			THROWERROR("Negative weight (" << weight << ") in addConstantOneBit");

		constantBits += (mpz_class(1) << weight);
	}

	/** "remove" a constant 1 from the bit heap. 
	    @param weight   the weight of the 1 to be added */
	void BitHeap::subConstantOneBit(int weight) {
		if (weight<0)
			THROWERROR("Negative weight (" << weight << ") in subConstantOneBit");

		constantBits -= (mpz_class(1) << weight);
	}

	/** add a constant to the bit heap. It will be added to the constantBits mpz, so we don't generate hardware to compress constants....
	    @param weight   the weight of the LSB of c (or, where c should be added)
	    @param c        the value to be added */
	void BitHeap::addConstant(int weight, mpz_class c) {
		if (weight<0)
			THROWERROR("Negative weight (" << weight << ") in addConstant");

		constantBits += (c << weight);
	};

	void BitHeap::addUnsignedBitVector(int weight, string x, unsigned size)	{
		if (weight<0)
			THROWERROR("Negative weight (" << weight << ") in addUnsignedBitVector");
		if(weight+size>maxWeight) {
			REPORT(INFO, "in subtractUnsignedBitVector: Size of signal " << x << " is " << size <<
			       ", adding it at weight " << weight << " overflows the bit heap (maxWeight=" << maxWeight << ")");
		}
		op->setCycleFromSignal(x);
		for (unsigned i=0; i<size; i++) {
			ostringstream s;
			s << x << of(i);
			addBit(weight+i, s.str());
		}
	}

	void BitHeap::subtractUnsignedBitVector(int weight, string x, unsigned size)
	{
		if (weight<0)
			THROWERROR("Negative weight (" << weight << ") in subtractUnsignedBitVector");
		if(weight+size>maxWeight) {
			REPORT(INFO, "in subtractUnsignedBitVector: Size of signal " << x << " is " << size <<
			       ", adding it at weight " << weight << " overflows the bit heap (maxWeight=" << maxWeight << ")");
		}

		for (unsigned i=0; i<size; i++) {
			ostringstream s;
			s << "not " << x << of(i);

			addBit(weight+i, s.str(),"", 1);
		}
		addConstantOneBit(weight);
		if (size+weight < maxWeight) {
			// complement all the way to maxWeight
			for (unsigned i=size+weight; i<maxWeight; i++) {
				addConstantOneBit(i);
			}
		}

	}


	void BitHeap::addSignedBitVector(int weight, string x, unsigned size)
	{
		if (weight<0)
			THROWERROR("Negative weight (" << weight << ") in addSignedBitVector");

 		if(weight+size>maxWeight)
 		{
			REPORT(INFO, "in subtractUnsignedBitVector: Size of signal " << x << " is " << size <<
			       ", adding it at weight " << weight << " overflows the bit heap (maxWeight=" << maxWeight << ")");
		}

		for(unsigned i=0; i<size; i++)
		{
			ostringstream rhs;
			if(i==size-1) // complement for sign extension
				rhs << "not ";
			rhs << x << of(i);
			addBit(weight + i, rhs.str());
		}
		// sign extension of the addend
		for (unsigned i=size+weight-1; i<maxWeight; i++) {
			addConstantOneBit(i);
		}
	}


	void BitHeap::subtractSignedBitVector(int weight, string x, unsigned size)
	{
		if (weight<0)
			THROWERROR("Negative weight (" << weight << ") in subtractSignedBitVector");
		// TODO
		THROWERROR("BitHeap::addSignedBitVector not yet implemented");
	}




	void  BitHeap::addMultiplierBlock(MultiplierBlock* m)
	{
		//now, I can insert in any position, because the supertile chaining will be done in an ascending way(hopefully)
		mulBlocks.push_back(m);

	}






	// TODO This code is a bit too rigid, it assumes supertiles are chains, whereas some Altera supertiles are not.

	void BitHeap::buildSupertiles()
	{
		bool isXilinx;
		
		if(op->getTarget()->getVendor() == "Xilinx")
			isXilinx = true;
		else
			isXilinx = false;

		//REPORT(DEBUG, "in buildSupertiles: mulBlocks' size=" << mulBlocks.size());
		for(unsigned i=0; i<mulBlocks.size()-1; i++)
			for(unsigned j=i+1; j<mulBlocks.size(); j++)
			{
				//if 2 blocks can be chained, then the chaining is done ascending by weight.
				//TODO improve the chaining
				// - pass the full target
				// - use the "double multiplier mode" on Altera
				// ...
				bool chain = mulBlocks[i]->canBeChained(mulBlocks[j], isXilinx);
				//REPORT(INFO, chain);
				//REPORT(INFO, mulBlocks[i]->getWeight())
				
				if(chain)
				{
					if(mulBlocks[j]->getWeight() <= mulBlocks[i]->getWeight())
					{
						if((mulBlocks[j]->getNext() == NULL) && (mulBlocks[i]->getPrevious() == NULL))
						{
							//REPORT(INFO,"block : " << mulBlocks[j]->getbotX() << "  " << mulBlocks[j]->getbotY());
							//REPORT(INFO,"with block : " << mulBlocks[i]->getbotX() << "  " << mulBlocks[i]->getbotY());
							mulBlocks[j]->setNext(mulBlocks[i]);
							mulBlocks[i]->setPrevious(mulBlocks[j]);
						}
					}
					else
					{
						if((mulBlocks[i]->getNext() == NULL) && (mulBlocks[j]->getPrevious() == NULL))
						{
							//REPORT(INFO,"block : " << mulBlocks[i]->getbotX() << "  " << mulBlocks[i]->getbotY());
							//REPORT(INFO,"with block : " << mulBlocks[j]->getbotX() << "  " << mulBlocks[j]->getbotY());
							mulBlocks[i]->setNext(mulBlocks[j]);
							mulBlocks[j]->setPrevious(mulBlocks[i]);
						}
					}
				}
			}
	}
	
	
	void BitHeap::generateAlteraSupertileVHDL(MultiplierBlock* x, MultiplierBlock* y, string resultName)
	{
		bool x1Signed, y1Signed, x2Signed, y2Signed, rSigned;
		int length;
		
		REPORT(DEBUG, "generating Altera supertile");
		
		//test to see if the sizes match those of a DSP
		//TODO: should be replaced with a test on the array of DSP configurations
		x1Signed = (signedIO) && ((x->getwX()==9) || (x->getwX()==12) || (x->getwX()==16) || (x->getwX()==18) || (x->getwX()==27) || (x->getwX()==36));
		y1Signed = (signedIO) && ((x->getwY()==9) || (x->getwY()==12) || (x->getwY()==16) || (x->getwY()==18) || (x->getwY()==27) || (x->getwY()==36));
		x2Signed = (signedIO) && ((y->getwX()==9) || (y->getwX()==12) || (y->getwX()==16) || (y->getwX()==18) || (y->getwX()==27) || (y->getwX()==36));
		y2Signed = (signedIO) && ((y->getwY()==9) || (y->getwY()==12) || (y->getwY()==16) || (y->getwY()==18) || (y->getwY()==27) || (y->getwY()==36));
		rSigned = x1Signed || y1Signed || x2Signed || y2Signed;
		
		//set the length
		//	all blocks must have the same size
		length = x->getwX() + (x1Signed ? 0 : 1);
		
		op->vhdl << endl << tab << "-- code generated for supertile" << endl << endl;
		
		op->vhdl << tab << "process(clk, rst)" << endl
				<< tab << tab << "variable x1_d: " << (rSigned ? "" : "un") << "signed(" << length-1 << " downto 0) := (others => '0');" << endl
				<< tab << tab << "variable y1_d: " << (rSigned ? "" : "un") << "signed(" << length-1 << " downto 0) := (others => '0');" << endl
				<< tab << tab << "variable x2_d: " << (rSigned ? "" : "un") << "signed(" << length-1 << " downto 0) := (others => '0');" << endl
				<< tab << tab << "variable y2_d: " << (rSigned ? "" : "un") << "signed(" << length-1 << " downto 0) := (others => '0');" << endl
				<< tab << tab << "variable p1: " << (rSigned ? "" : "un") << "signed(" << 2*length-1 << " downto 0) := (others => '0');" << endl
				<< tab << tab << "variable p2: " << (rSigned ? "" : "un") << "signed(" << 2*length-1 << " downto 0) := (others => '0');" << endl
				<< tab << tab << "variable result: " << (rSigned ? "" : "un") << "signed(" << 2*length << " downto 0) := (others => '0');" << endl
				<< tab << "begin" << endl
				<< tab << tab << "if (clk'event) and (clk='1') then" << endl
				<< tab << tab << tab << "x1_d := " << (rSigned ? "" : "un") << "signed(" << ((rSigned && x1Signed) ? "" : "\"0\" & ") << x->getInputName1() << range(x->gettopX()+x->getwX()-1,(x->gettopX()>0?x->gettopX():0)) << " & " << zg((x->gettopX()<0?-x->gettopX():0)) << ");" << endl
				<< tab << tab << tab << "y1_d := " << (rSigned ? "" : "un") << "signed(" << ((rSigned && y1Signed) ? "" : "\"0\" & ") << x->getInputName2() << range(x->gettopY()+x->getwY()-1,(x->gettopY()>0?x->gettopY():0)) << " & " << zg((x->gettopY()<0?-x->gettopY():0)) << ");" << endl
				<< tab << tab << tab << "x2_d := " << (rSigned ? "" : "un") << "signed(" << ((rSigned && x2Signed) ? "" : "\"0\" & ") << y->getInputName1() << range(y->gettopX()+y->getwX()-1,(y->gettopX()>0?y->gettopX():0)) << " & " << zg((y->gettopX()<0?-y->gettopX():0)) << ");" << endl
				<< tab << tab << tab << "y2_d := " << (rSigned ? "" : "un") << "signed(" << ((rSigned && y2Signed) ? "" : "\"0\" & ") << y->getInputName2() << range(y->gettopY()+y->getwY()-1,(y->gettopY()>0?y->gettopY():0)) << " & " << zg((y->gettopY()<0?-y->gettopY():0)) << ");" << endl
				<< endl
				<< tab << tab << tab << "p1 := x1_d * y1_d;" << endl
				<< tab << tab << tab << "p2 := x2_d * y2_d;" << endl
				<< endl
				<< tab << tab << tab << "result := resize(p1, " << 2*length+1 << ") + resize(p2, " << 2*length+1 << ");" << endl
				<< endl;
				
		//manage critical path
		op->manageCriticalPath(op->getTarget()->DSPMultiplierDelay() + op->getTarget()->DSPAdderDelay() );
				
		op->vhdl << tab << tab << tab << resultName << " <= std_logic_vector(result);" << endl
				<< tab << tab << "end if;" << endl
				<< endl
				<< tab << "end process;" << endl
				<< endl;
		
		op->vhdl << tab << "-- end of code generated for supertile" << endl << endl;
		
		x->setSignalLength(x->getwX() + x->getwY() + ((rSigned && x1Signed) ? 0 : 1) + ((rSigned && y1Signed) ? 0 : 1));
		y->setSignalLength(y->getwX() + y->getwY() + ((rSigned && x2Signed) ? 0 : 1) + ((rSigned && y2Signed) ? 0 : 1));
		
		REPORT(DEBUG, "generating Altera supertile completed");
	}


	//TODO: make sure it works for the combinatorial version
	void BitHeap::generateSupertileVHDL()
	{
		//making all the possible supertiles
		if((enableSuperTiles) && (mulBlocks.size()>1))
		{	
			buildSupertiles();
			REPORT(DEBUG, "supertiles built");
		}
		//generate the VHDL code for each supertile
		op->vhdl << tab << "-- code generated by BitHeap::generateSupertileVHDL()"<< endl;

		// This loop iterates on all the blocks, looking for the roots of supertiles
		for(unsigned i=0;i<mulBlocks.size();i++)
		{
			//take just the blocks which are roots
			if(mulBlocks[i]->getPrevious()==NULL)
			{
				int DSPuid=0;
				MultiplierBlock* next;
				MultiplierBlock* current = mulBlocks[i];
				int newLength=0;
				int addedCycles;
				double addedCriticalPath;

				//TODO reset cycle/CP to the beginning of mult
				op->setCycle(0);
				op->setCriticalPath(0);

				//the first DSP from the supertile(it has the smallest weight in the supertile)
				if(op->getTarget()->getVendor() == "Xilinx")
				{
					generateVHDLforDSP(current, DSPuid, i);
					
					//manage critical path
					op->getTarget()->delayForDSP(current, op->getCriticalPath(), addedCycles, addedCriticalPath);
					for(int j=0; j<addedCycles; j++)
						op->nextCycle();
					op->setCriticalPath(addedCriticalPath);
				}
				
				if(op->getTarget()->getVendor() == "Altera")
				{
					stringstream sumName;
					int length, weightBase;
					
					next = current->getNext();
					
					if(next == NULL)
					{
						sumName << join("DSP_bh", guid, "_ch", i, "_", DSPuid);
						
						generateVHDLforDSP(current, DSPuid, i);
					
						//manage critical path
						op->getTarget()->delayForDSP(current, op->getCriticalPath(), addedCycles, addedCriticalPath);
						for(int j=0; j<addedCycles; j++)
							op->nextCycle();
						op->setCriticalPath(addedCriticalPath);
					}
					else
					{
						DSPuid++;
						sumName << join("DSP_bh", guid, "_ch", i, "_", DSPuid);
						
						//manage critical path
						op->getTarget()->delayForDSP(current, op->getCriticalPath(), addedCycles, addedCriticalPath);
						for(int j=0; j<addedCycles; j++)
							op->nextCycle();
						op->setCriticalPath(addedCriticalPath);
						if(!op->getTarget()->isPipelined())
							op->manageCriticalPath( op->getTarget()->adderDelay(next->getSigLength()+1) );
						
						//the combinatorial case
						if(!op->getTarget()->isPipelined())
						{
							stringstream zeros;
														
							DSPuid--;
							generateVHDLforDSP(current, DSPuid, i);
							DSPuid++;
							generateVHDLforDSP(next, DSPuid, i);
							
							op->declare(sumName.str(), 2*(current->getwX()+((signedIO) && ((current->getwX()==9) || (current->getwX()==12) || (current->getwX()==16) || (current->getwX()==18) || (current->getwX()==27) || (current->getwX()==36)) ? 0 :1)) + 1);
							
							for(int j=0; j<current->getSigLength()-next->getSigLength()+1; j++)
								zeros << next->getSigName() << "(" << next->getSigLength()-1 << ") & ";
							
							op->vhdl << tab << sumName.str()
						         << "<= (" << current->getSigName() << "(" << current->getSigLength()-1 << ") & " << current->getSigName()
						         << ") +  ( " << zeros.str() << next->getSigName() << " );" <<endl;
						}
						else
						{
							//reset timing
							op->setCycleFromSignal(current->getInputName1());
							op->syncCycleFromSignal(current->getInputName2());
							op->syncCycleFromSignal(next->getInputName1());
							op->syncCycleFromSignal(next->getInputName2());
							
							//manage critical path properly for the super-tile process
							int currentCycle = op->getCurrentCycle(), addedCycles;
							double currentCriticalPath = op->getCriticalPath(), addedCP;
							
							op->manageCriticalPath(op->getTarget()->DSPMultiplierDelay() + op->getTarget()->DSPAdderDelay() );
							
							op->declare(sumName.str(), 2*(current->getwX()+((signedIO) && ((current->getwX()==9) || (current->getwX()==12) || (current->getwX()==16) || (current->getwX()==18) || (current->getwX()==27) || (current->getwX()==36)) ? 0 :1)) + 1);
							
							//reset the timing to the original state
							op->setCycle(currentCycle);
							op->setCriticalPath(currentCriticalPath);
														
							generateAlteraSupertileVHDL(current, next, sumName.str());
							
							//manage critical path once again
							op->syncCycleFromSignal(sumName.str());
						}
					}
					
					length = current->getSigLength();
					if(next != NULL)
					{
						length = ((next->getSigLength() > current->getSigLength()) ? next->getSigLength() : current->getSigLength()) + 1;
						weightBase = next->getWeight();
					}else
					{
						weightBase = current->getWeight();
					}
						
					for(int k=length-1;k>=0;k--)
					{
						int weight = weightBase +k;
						
						if(weight>=0)
						{
							stringstream s;
		
							if((k==length-1)&&(signedIO))
							{
								s << "not( " << sumName.str() << "(" << k << ") )";
								addBit(weight, s.str(), "", 5);
								for(int j=maxWeight; j>=weight; j--)
									addConstantOneBit(j);
							}
							else
							{
								s << sumName.str() << "(" << k << ")";
								addBit(weight, s.str(), "", 1);
							}
						}
					}
					
				}
				else
				{
					//iterate on the other blocks of the supertile
					while(current->getNext()!=NULL)
					{
						DSPuid++;
						next=current->getNext();

						generateVHDLforDSP(next, DSPuid, i);
						//TODO ! replace 17 with multiplierblock->getshift~ something like that

						//manage critical path
						op->getTarget()->delayForDSP(next, op->getCriticalPath(), addedCycles, addedCriticalPath);
						for(int j=0; j<addedCycles; j++)
							op->nextCycle();
						op->setCriticalPath(addedCriticalPath);

						if(current->getSigLength() > next->getSigLength())
							newLength=current->getSigLength();
						else
							newLength=next->getSigLength();

						//addition, the 17lsb-s from the first block will go directly to bitheap
						if(signedIO)
						{
							stringstream s;
							for(int j=0;j<17;j++)
								s<<current->getSigName()<<"("<<current->getSigLength()-1<<") & ";
							s<<current->getSigName()<<"("<<current->getSigLength()-1<<")";

							newLength++;

							op->vhdl << tab <<  op->declare(join("DSP_bh",guid,"_ch",i,"_",DSPuid),newLength)
									 << "<= " <<next->getSigName()
									 << " +  ( "<<  s.str() <<" & "<<"  "
									 <<current->getSigName()
									 <<range(current->getSigLength()-1,17)<<" );"<<endl ;
						}else
						{
							op->vhdl << tab <<  op->declare(join("DSP_bh",guid,"_ch",i,"_",DSPuid),newLength)
									 << "<= " <<next->getSigName()
									 << " +  ( "<<  zg(17) /* s.str()*/ <<" & "<<"  "
									 <<current->getSigName()<<range(newLength-1,17)<<" );"<<endl ;
						}

						//sending the 17 lsb to the bitheap
						for(int k=16;k>=0;k--)
						{
							int weight=current->getWeight()+k;
							if(weight>=0)
							{
								stringstream s;
								s<<current->getSigName()<<"("<<k<<")";

								addBit(weight,s.str(),"",1);
							}
						}
						
						//setting the name and length of the current block, to be used properly in the next iteration
						stringstream q;
						q << join("DSP_bh",guid,"_ch",i,"_",DSPuid);
						next->setSignalName(q.str());
						next->setSignalLength(newLength);
						//next
						current=next;
					}

					// adding the result to the bitheap (in the last block from the supertile)
					string name=current->getSigName();
					int length=current->getSigLength();
					REPORT(DEBUG,"sending bits to bitheap after chain adding");
					for(int k=length-1;k>=0;k--)
					{
						int weight=current->getWeight()+k;
						
						if(weight>=0)
						{
							stringstream s;
		
							if((k==length-1)&&(signedIO))
							{
								s<<"not( "<<name<<"("<<k<<") )";
								addBit(weight,s.str(),"",5);
								for(int i=maxWeight;i>=weight;i--)
									addConstantOneBit(i);
							}
							else
							{
								s<<name<<"("<<k<<")";
								addBit(weight,s.str(),"",1);
							}
						}
					}
				}
			}
		}
	}


	int BitHeap::computeStage()
	{
		return (op->getCurrentCycle()*stagesPerCycle + op->getCriticalPath()/elementaryTime);

	}


	void  BitHeap::addBit(int w, string rhs, string comment, int type)
	{
		if (w<0)
			THROWERROR("Negative weight (" << w << ") in addBit");
			
		REPORT(DEBUG, "addBit at weigth " <<w <<"   for rhs=" << rhs );
		
		// ignore bits beyond the declared maxWeight
		if((unsigned)w >= maxWeight)
		{
			REPORT(INFO, "WARNING in addBit, w=" << w << " greater than mawWeight=" <<maxWeight << "... ignoring it"  );
			return;
		}

		WeightedBit* bit= new WeightedBit(getGUid(), newUid(w), w, type, op->getCurrentCycle(), op->getCriticalPath()) ;
		// created at (op->getCycle(), opt-getCriticalPath())

		int bitStage = bit->computeStage(stagesPerCycle, elementaryTime);
		if (bitStage > plottingStage)
			plottingStage = bitStage;

		list<WeightedBit*>& l=bits[w];

		//insert so that the list is sorted by bit cycle/delay
		list<WeightedBit*>::iterator it=l.begin();
		bool proceed=true;
		unsigned count=0;
		while(proceed)
		{
			if (it==l.end() || (*bit <= **it))
			{ // test in this order to avoid segfault!
				l.insert(it, bit);
				proceed=false;
			}
			else
			{
				count++;
				it++;
			}
		}

		// now generate VHDL
		op->vhdl << tab << op->declare(bit->getName()) << " <= " << rhs << ";";
		if(comment.size())
			op->vhdl <<  " -- " << comment;
		op->vhdl <<  " -- " << "cycle= " << bit->getCycle() << " cp= " << bit->getCriticalPath(bit->getCycle()) << endl;

		REPORT(DEBUG, "added bit named "  << bit->getName() << " on column " << w <<" at cycle= "<< bit->getCycle() <<" cp= "<<bit->getCriticalPath(bit->getCycle()));

		printColumnInfo(w);
	};



	void BitHeap::removeBit(unsigned weight, int dir)
	{
		list<WeightedBit*>& l=bits[weight];

		//WeightedBit* bit;

		//if dir=0 the bit will be removed from the begining of the list, else from the end of the list of weighted bits
		if(dir==0)
			l.pop_front();
		else if(dir==1)
			l.pop_back();



		REPORT(DEBUG,"remove bit from column " << weight);
	}




	// The following code assumes that a column is sorted by increasing time

	WeightedBit* BitHeap::latestInputBitToCompressor(unsigned w, int c0, int c1)
	{
		list<WeightedBit*>::iterator it;

		if(w>=maxWeight)	{
			REPORT(DEBUG, "latestInputBitToCompressor returns null because w>=maxWeight");
			return NULL;
		}

		if(c1==0) { // compressor of one column only
			int k=1;
			for(it = bits[w].begin(); it!=bits[w].end(); ++it)	{
				if (k==c0)
					return *it;
				k++;
			}
			THROWERROR("latestInputBitToCompressor: shouldn't get there");
			//REPORT(DEBUG, "in latestInputBitToCompressor");
		}
		else { // compressor for two columns
			int i=1, j=1;
			WeightedBit *b0, *b1;
				for(it = bits[w].begin(); it!=bits[w].end(); ++it){
					if (i==c0)
						b0 = *it;
					i++;
				}
				for(it = bits[w+1].begin(); it!=bits[w+1].end(); ++it)	{
					if (j==c1)
						b1 = *it;
					j++;
				}

				if((*b0) <= (*b1))
					return b1;
				else
					return b0;
			}
	}




	void BitHeap::elemReduce(unsigned i, BasicCompressor* bc, int type)
	{
		REPORT(DEBUG, "Entering elemReduce for column "<< i << " using compressor " << bc->getName() );
		list<WeightedBit*>::iterator it = bits[i].begin();
		stringstream signal[2];

		op->vhdl << endl;

		//		REPORT(DEBUG, cnt[i] << "  " << bits[i].size());
		//    REPORT(DEBUG, cnt[i+1] << "  " << bc->getColumnSize(1));


		WeightedBit* b =  latestInputBitToCompressor(i, bc->getColumnSize(0), bc->getColumnSize(1)) ;


		if(b)	{
			REPORT(DEBUG, "latest bit for this reduction" << b->getName());
				op->setCycle(  b ->getCycle()  );
				op->setCriticalPath(  b ->getCriticalPath(op->getCurrentCycle()));
				op->manageCriticalPath( op->getTarget()->lutDelay() + op->getTarget()->localWireDelay() );

				if((op->getCurrentCycle()>plottingCycle) || ((op->getCurrentCycle()==plottingCycle) &&
				                                             (op->getCriticalPath()>plottingCP)))	{
						plottingCycle = op->getCurrentCycle();
						plottingCP = op->getCriticalPath();
					}
			}
		else THROWERROR("elemReduce: No latest bit?");

		// build the first input of the compressor
		for(unsigned j=0; j<bc->getColumnSize(0); j++)	{
				signal[0] << (*it)->getName();
				if(j!=bc->getColumnSize(0)-1)
					signal[0] << " & ";
				it++;
			}

 		// build the second input of the compressor, if any
		if(bc->getColumnSize(1)!=0)
			{
				it = bits[i+1].begin();
				for(unsigned j=0; j<bc->getColumnSize(1); j++)
					{
						signal[1] << (*it)->getName();
						if(j!=bc->getColumnSize(1)-1)
							signal[1] << " & ";
						it++;
					}
			}

		string in_concat=join("CompressorIn_bh", getGUid(), "_");
		string out_concat=join("CompressorOut_bh", getGUid(), "_");
		string compressor=join("Compressor_bh", getGUid(), "_");

		for(unsigned j=0; j<bc->height.size(); j++)
			{
				if (bc->getColumnSize(j)==1)
					op->vhdl << tab << op->declare(join(in_concat, compressorIndex,"_", inConcatIndex), bc->getColumnSize(j))
					         << "(0) <= " << signal[j].str() << ";" << endl;
				else
					op->vhdl << tab << op->declare(join(in_concat, compressorIndex,"_", inConcatIndex), bc->getColumnSize(j))
					         << " <= " << signal[j].str() << ";" << endl;
				op->inPortMap(bc, join("X",j), join(in_concat, compressorIndex,"_", inConcatIndex));
				++inConcatIndex;
			}

		op->outPortMap(bc, "R", join(out_concat, compressorIndex,"_", outConcatIndex));

		op->vhdl << tab << op->instance(bc, join(compressor, compressorIndex));

		removeCompressedBits(i,bc->getColumnSize(0));
		if(bc->getColumnSize(1)!=0)
			removeCompressedBits(i+1,bc->getColumnSize(1));


		// add the bits, at the current (global) instant.
		for (int j=0; j<bc->getOutputSize(); j++) {
			addBit(i+j, join(out_concat, compressorIndex,"_", outConcatIndex, of(j)),"",type);
		}
#if 0
		addBit(i, join(out_concat, compressorIndex,"_", outConcatIndex, "(0)"),"",type);
		addBit(i+1,    join(out_concat, compressorIndex,"_", outConcatIndex, "(1)"),"",type);
		if(!((bc->getColumnSize(0)==3) && (bc->getColumnSize(1)==0)))
			addBit(i+2, join(out_concat, compressorIndex,"_", outConcatIndex, "(2)"),"",type);
#endif
		REPORT(DEBUG, "Exiting elemReduce() ");

		++compressorIndex;
		++outConcatIndex;

	}



	unsigned BitHeap::currentHeight(unsigned w) {
		int h=0;
		list<WeightedBit*>& l = bits[w];
		h=l.size();
		return h;
	}


	//c - the current column, red- number of bits to be reduced
	void BitHeap::removeCompressedBits(int c, int red)
	{
		while(red>0)
		{
			removeBit(c,0);
			red--;
		}
	}


	unsigned BitHeap::getMaxHeight()
	{
		unsigned max=0;
		
		for(unsigned i=0; i<maxWeight; i++)
		{
			if(bits[i].size()>max)
				max=bits[i].size();
		}
		return max;
	}






	void BitHeap::generatePossibleCompressors()
	{
		//Build a vector of basic compressors that are fit for this target
		int maxCompressibleBits, col0, col1;

		maxCompressibleBits = op->getTarget()->lutInputs();



#if 0 // The other alternative works better but has to be adapted to arbitrary lutInputs
		// TODO FIXME What fitness function? 
		//Generate all "workable" compressors for 2 columns, on this target, descending on fitness function
		for(col0=maxCompressibleBits; col0>=3; col0--)
			for(col1=col0; col1>=0; col1--)
				if((col0 + col1<=maxCompressibleBits) && (intlog2(col0 + 2*col1)<=3))
					{
						REPORT(DEBUG, "Generating compressor for col1=" << col1 <<", col0=" << col0);
						vector<int> newVect;
						newVect.push_back(col0);
						newVect.push_back(col1);
						BasicCompressor* bc = new BasicCompressor(op->getTarget(), newVect);
						if(col0==3 && col1==0)
							fullAdder = bc;
						possibleCompressors.push_back(bc);
					}

#else // just for test

		/*
		{//test
			col0=7; col1=0;
			vector<int> newVect;
			newVect.push_back(col0);
			newVect.push_back(col1);
			possibleCompressors.push_back(new BasicCompressor(op->getTarget(), newVect));
		}
		{// test
			col0=5; col1=1;
			vector<int> newVect;
			newVect.push_back(col0);
			newVect.push_back(col1);
			fullAdder = new BasicCompressor(op->getTarget(), newVect);
			possibleCompressors.push_back(fullAdder);
		}
		*/
		{
			col0=6; col1=0;
			vector<int> newVect;
			newVect.push_back(col0);
			newVect.push_back(col1);
			possibleCompressors.push_back(new BasicCompressor(op->getTarget(), newVect));
		}
		{//test
			col0=4; col1=1;	
			vector<int> newVect;
			newVect.push_back(col0);
			newVect.push_back(col1);
			fullAdder = new BasicCompressor(op->getTarget(), newVect);
			possibleCompressors.push_back(fullAdder);
		}
		{// test
			col0=5; col1=0;	
			vector<int> newVect;
			newVect.push_back(col0);
			newVect.push_back(col1);
			fullAdder = new BasicCompressor(op->getTarget(), newVect);
			possibleCompressors.push_back(fullAdder);
		}
		{//test
			col0=4; col1=0;
			vector<int> newVect;
			newVect.push_back(col0);
			newVect.push_back(col1);
			fullAdder = new BasicCompressor(op->getTarget(), newVect);
			possibleCompressors.push_back(fullAdder);
		}
		{
			col0=3; col1=2;	
			vector<int> newVect;
			newVect.push_back(col0);
			newVect.push_back(col1);
			possibleCompressors.push_back(new BasicCompressor(op->getTarget(), newVect));
		}
		{//test
			col0=3; col1=1;
			vector<int> newVect;
			newVect.push_back(col0);
			newVect.push_back(col1);
			fullAdder = new BasicCompressor(op->getTarget(), newVect);
			possibleCompressors.push_back(fullAdder);
		}
		{
			col0=3; col1=0;	
			vector<int> newVect;
			newVect.push_back(col0);
			newVect.push_back(col1);
			fullAdder = new BasicCompressor(op->getTarget(), newVect);
			possibleCompressors.push_back(fullAdder);
		}
		
#endif


		/*
		if(!fullAdder)
			THROWERROR("What, generatePossibleCompressors() didn't build a full adder !?!");
		
		// Building the half-adder
		{
			vector<int> newVect;
			newVect.push_back(2);
			newVect.push_back(0);
			halfAdder = new BasicCompressor(op->getTarget(), newVect);
		}
		*/
	}


	void BitHeap::printColumnInfo(int w)
	{
		int i=0;

		for(list<WeightedBit*>::iterator it = bits[w].begin(); it!=bits[w].end(); ++it)
			{
				REPORT(FULL, "i="<<i<<"  name=" << (*it)->getName() <<" cycle="<<(*it)->getCycle()
				       << " cp="<<(*it)->getCriticalPath((*it)->getCycle()));
				// check the ordering -- this should be useless, was inserted for debug purpose
				if(i>0) {
					list<WeightedBit*>::iterator itm1 = it;
					itm1--;
					if(**it < **itm1)
						THROWERROR("Wrong ordering of weighted bits: *it=" << *it << " ("<< 
						           (*it)->getCycle()<< ", " <<  (*it)->getCriticalPath((*it)->getCycle()) 
						           << ") <    *itm1=" << *itm1 << "(" << 
						           (*itm1)->getCycle()<< ", " <<  (*itm1)->getCriticalPath((*itm1)->getCycle()) << ")" );
				}
				i++;
			}
	}



	void BitHeap::generateCompressorVHDL()
	{
		op->vhdl << tab << endl << tab << "-- Beginning of code generated by BitHeap::generateCompressorVHDL" << endl;
		REPORT(DEBUG, "begin generateCompressorVHDL");

		generateSupertileVHDL();
		// add the constant bits to the actual bit heap

		//empty bitheap
		if (getMaxHeight()==0)
			return ;

		op->setCycle(0); // TODO FIXME for the virtual multiplier case where inputs can arrive later

		REPORT(DEBUG, "Adding the constant bits");
		op->vhdl << endl << tab << "-- Adding the constant bits" << endl;

		for (unsigned w=0; w<maxWeight; w++){
			if (1 == ((constantBits>>w) & 1) )
				addBit(w, "'1'","",2);
		}
		
		op->vhdl << endl;

		printBitHeapStatus();

		WeightedBit* firstBit = getFirstSoonestBit();
		REPORT(DEBUG, "getFirstSoonestBit " << firstBit);
		int minCycle = firstBit->getCycle();
		double minCP = firstBit->getCriticalPath(minCycle);
		op->setCycle(minCycle);
		op->setCriticalPath(minCP);

		if (getMaxHeight()<=1)        
		{
			// If the bit heap is of max size 1, compression is done, just copy these bits to the result vector.
			op->vhdl << tab << op->declare(join("CompressionResult", guid), maxWeight) << " <= ";
			for(int w= (signed)getMaxWeight()-1; w>=0; w--)
			{
				if(bits[w].size()==0)
					op->vhdl << "'0'" ;
				else // size should be 1
					op->vhdl << bits[w].front()->getName() ;
				if(w!=0)
					op->vhdl << " & ";
				else
					op->vhdl << ";" << endl;
			}
		}
		else 
		{
			// There is some compression to do
			generatePossibleCompressors();

			elementaryTime = op->getTarget()->lutDelay() + op->getTarget()->localWireDelay();
			stagesPerCycle = (1/op->getTarget()->frequency()) / elementaryTime;
			int stage = minCycle*stagesPerCycle + minCP/elementaryTime ;

			minWeight=0;
			didCompress = true;

			//first snapshot, containing all bits available at minCycle, not regarding their critical path
			plotter->heapSnapshot(didCompress,  minCycle, 1);

			//compressing until the maximum height of the columns is 3
			//EXPERIMENTAL------------------------------------------------------
			
			if(compressionType == 0)
			{
				//lutCompressionLevel=0 - compression using only compressors
				
				//first compress using the largest compressor that can fit in a LUT,
				//	then compress with (what should be the best, but is actually the) second best compressor
				//	and then compress with all the rest of the compressors, in order to reduce the heap to just two lines
				
				REPORT(DEBUG, "checkpoint 0: compression type 0, starting compressions");
				
				while(getMaxHeight() >= 3)
				{
					compress(stage);

					plotter->heapSnapshot(didCompress, plottingCycle, plottingCP);
					plottingCycle=0;
					plottingCP=0;
					
					stage++;
				}
			}
			else
			{
				//lutCompressionLevel=2 - compression using a mix of compressors and adder tree for the last lines
				//lutCompressionLevel>0 - adder tree compression involved
				
				//continue here -> check if the version that skips stages actually works, or not
				
				int startingIndex = minWeight;
				
				while(getMaxHeight() >= op->getTarget()->lutInputs())
				{
					int currentColumnIndex = startingIndex;
					
					compress(stage);

					plotter->heapSnapshot(didCompress, plottingCycle, plottingCP);
					plottingCycle=0;
					plottingCP=0;
					
					stage++;
				}
				
				if(compressionType > 0)
				{
					applyAdderTreeCompression();
				}
			}
			
			//EXPERIMENTAL -----------------------------------------------------

			//only three levels left
			if(getMaxHeight() > 2)
			{
				//plotter->heapSnapshot(true, op->getCurrentCycle(), op->getCriticalPath() );
				REPORT(DEBUG, "only three levels left");

				WeightedBit *bb = getLatestBit(minWeight, maxWeight-1);
				plotter->heapSnapshot(didCompress, bb->getCycle(), bb->getCriticalPath(bb->getCycle()));

				//Altera
				if(op->getTarget()->hasFastLogicTernaryAdders())
				{
					generateFinalAddVHDL(false);
				}
				else
				{
					//Xilinx
					//                 8 8 16 1         12 12 24 1
					// #if 0 (new):   101 4.865ns   202 5.371ns
					// #if 1:         97 5.176ns		192  6.840		
#if 1
					//do additional compressions or additions
					if (getMaxHeight()>2)
					{
						unsigned i = minWeight;
						// remember the initial heights
						for(unsigned i=minWeight; i<maxWeight; i++)   
							cnt[i]=bits[i].size();

						//printBitHeapStatus();

						//find the first column with 3 bits (starting from LSB); the rest go to the final adder
						while(cnt[i]<3)
							i++;

						REPORT(DEBUG, "minWeight=" << minWeight << "    first weight with 3 bits:" << i);
						if(i>minWeight)
						{
							WeightedBit *b = getLatestBit(minWeight, i-1);
							op->setCycle( b->getCycle());
							op->setCriticalPath(b->getCriticalPath(op->getCurrentCycle()));
							op->manageCriticalPath(op->getTarget()->localWireDelay() +
							                       op->getTarget()->adderDelay(i-minWeight));
							
							applyAdder(minWeight, i-1, false);
							
							concatenateLSBColumns();

							//REPORT(DEBUG, "here" << maxWeight << " " << i);
						}
						
						WeightedBit* latestBit;

						while(i<maxWeight)
						{
							REPORT(DEBUG, "i= "<< i << " cnt= " << cnt[i]);
							// Now we are sure cnt[i] is 3
							if (i==maxWeight-1)
							{
								if (bits[i].size()>=3)
									applyCompressor3_2(i);
								i++;
							}
							else
							{
								if((cnt[i+1]==3) || (cnt[i+1]==1) || (cnt[i+1]==0))
								{
									if (bits[i].size()>=3)
										applyCompressor3_2(i);
									do
									{
										i++;
									}
									while(cnt[i]!=3);
								}
								else
								{
									if(cnt[i+1]==2)
									{
										int j=i;
										do
										{
											i++;
										}
										while(cnt[i]==2);
										
										//REPORT(INFO, "j= "<< j << " i-1= " << i-1);
										
										latestBit = getLatestBit(j, i-1);
										if(latestBit)
										{
											op->setCycle( latestBit ->getCycle()  );
											op->setCriticalPath(   latestBit ->getCriticalPath(op->getCurrentCycle()));
											op->manageCriticalPath( op->getTarget()->localWireDelay() +
											                        op->getTarget()->adderDelay(i-j) );

											//REPORT(INFO, endl << op->getTarget()->adderDelay(i-j) << endl );

											stage = computeStage();
										}

										applyAdder(j, i-1);

										while((i<=maxWeight) && (cnt[i]!=3))
										{
											i++;
										}
									}
								}
							}
						}
					}
					
#else
					if (getMaxHeight()>2)	
					{
						// Just use a row of 3:2 compressors
						for(unsigned i=minWeight; i<maxWeight; i++)   
							cnt[i]=bits[i].size();

						// TODO ne pas generer de HA si pas besoin
						unsigned i=minWeight;
						while(i<maxWeight){
							unsigned size = cnt[i];
							if(size==3) {
								elemReduce(i, fullAdder);
							}
							else if(size==2) {
								elemReduce(i, halfAdder);
							}
							else { // size of this column is 1 or 0, nothing to do
							}
							i++;
						}
					} // else getMaxHeight==2, nothing to do			
#endif

					concatenateLSBColumns();

					REPORT(DEBUG, "Column height after all compressions");
					printBitHeapStatus();

					stage = computeStage();

					plotter->heapSnapshot(true,  plottingCycle, plottingCP);

					//final addition
					generateFinalAddVHDL(true);
				}
			}
			else
			{
				plotter->heapSnapshot(true,  plottingCycle, plottingCP);
				if(compressionType == 0)
					generateFinalAddVHDL(true);
			}
				
#if BITHEAP_GENERATE_SVG
			plotter->plotBitHeap();
#endif

		}

		for(int i=0; i<(sizeof(usedCompressors)/sizeof(bool)); i++)
		{
			if (usedCompressors[i]==true)
			{
					if(i < possibleCompressors.size())
					{
						possibleCompressors[i]->addToGlobalOpList();
					}
					else
					        usedCompressors[i] = false;
			}
		}

		// Let's assume the half-adder and full adder are always used
		if(halfAdder) // but protect, because if the multipliers fits a DSP it is not true
			halfAdder->addToGlobalOpList();
		if(fullAdder)
			fullAdder->addToGlobalOpList();

		op->vhdl << tab << "-- End of code generated by BitHeap::generateCompressorVHDL" << endl;
	}


	//returns a pointer to the first bit which has the smallest cycle & CP combination
	WeightedBit* BitHeap::getFirstSoonestBit()
	{
		int minCycle= 9999;
		double minCP = 100;
		WeightedBit *b=0;
		for(unsigned w=minWeight; w<maxWeight; w++)
			{
				//REPORT(DEBUG, "w=" << bits[w].size());
				if (bits[w].size() > (unsigned)0)
					{

						for(list<WeightedBit*>::iterator it = bits[w].begin(); it!=bits[w].end(); ++it)
							{
								if (minCycle > (*it)->getCycle())
									{
										minCycle = (*it)->getCycle();
										minCP = (*it)->getCriticalPath(minCycle);
										b = *it;
									}
								else
									if ((minCycle == (*it)->getCycle()) &&  (minCP > (*it)->getCriticalPath(minCycle)))
										{
											minCP = (*it)->getCriticalPath(minCycle);
											b = *it;
										}
							}
					}
			}

		return b;

	}




	void BitHeap::applyCompressor3_2(int col)
	{
		REPORT(DEBUG, "Entering applyCompressor3_2(" << col << ") ");
		unsigned i;
		for(i=0; i<possibleCompressors.size(); i++)
			{
				if((possibleCompressors[i]->getColumnSize(0)==3) && (possibleCompressors[i]->getColumnSize(1)==0))
					break; // exit the loop
			}
		REPORT(DEBUG, "Using Compressor3_2, reduce column " << col);
		elemReduce(col, possibleCompressors[i], 4);
		usedCompressors[i]=true;
		REPORT(DEBUG, "Exiting applyCompressor3_2(" << col << ") ");
	}




	// addition stretching from weights col0(LSB) to msb included
	// assumes cnt has been set up
	void BitHeap::applyAdder(int lsb, int msb, bool hasCin)
	{
		REPORT(DEBUG, "Applying an adder from columns " << lsb << " to " << msb);
		stringstream inAdder0, inAdder1, cin;
		
		WeightedBit *lastBit = bits[lsb].front();
		
		//compute the critical path
		REPORT(DEBUG, "Computing critical path between columns " << lsb << " and " << msb);
		
		for(unsigned int i=lsb; i<=msb; i++)
		{
			if(cnt[i]>0)
			{
				int count = 0;
				
				for(list<WeightedBit*>::iterator it = bits[i].begin(); (it!=bits[i].end() && count<(i==lsb ? 3 : 2)); ++it)
				{
#if 0
					if(
						(lastBit->getCycle() < (*it)->getCycle()) || 
						(lastBit->getCycle()==(*it)->getCycle() && lastBit->getCriticalPath(lastBit->getCycle())<(*it)->getCriticalPath((*it)->getCycle()))
					   )
#else
					if(*lastBit<**it)
#endif
					{
						lastBit = *it;
					}
					
					count++;
				}
			}
		}
		
		for(int i = msb; i>=lsb+1; i--)
		{
			list<WeightedBit*>::iterator it = bits[i].begin();

			if(cnt[i]>=2)
			{

				inAdder0 << (*it)->getName();
				it++;
				inAdder1 << (*it)->getName();
			}
			else
			{
				//if(bits[i].size()==1)
				if(cnt[i]==1)
				{
					inAdder0 << (*it)->getName();
					inAdder1 << "\'0\'";
				}
				else
				{
					inAdder0 << "\'0\'";
					inAdder1 << "\'0\'";
				}
			}

			inAdder0 << " & ";
			inAdder1 << " & ";

			removeCompressedBits(i, cnt[i]);
		}

		// We know the LSB col is of size 3
		list<WeightedBit*>::iterator it = bits[lsb].begin();
		if(cnt[lsb]>0)
			inAdder0 << (*it)->getName();
		if(cnt[lsb]>1)
		{
			it++;
			inAdder1 << (*it)->getName();
		}
		else
		{
			inAdder1 << "\'0\'";
		}

		if(hasCin)
		{
			it++;
			cin << (*it)->getName() << ";";
		}
		else
		{
			cin << "\'0\';";
		}
		removeCompressedBits(lsb, cnt[lsb]);
		
		REPORT(DEBUG, "removed bits that are being compressed through an addition");
		
		//for timing purposes
		op->setCycle(lastBit->getCycle());
		//op->syncCycleFromSignal(lastBit->getName());		//working
		op->setCriticalPath(lastBit->getCriticalPath(op->getCurrentCycle()));

		inAdder0 << ";";
		inAdder1 << ";";
		string inAdder0Name = join("inAdder0_bh", getGUid(), "_");
		string inAdder1Name = join("inAdder1_bh", getGUid(), "_");
		string cinName = join("cin_bh", getGUid(), "_");
		string outAdder = join("outAdder_bh", getGUid(), "_");

		op->vhdl << tab << op->declare(join(inAdder0Name, adderIndex), msb-lsb+2) << " <= \'0\' & " << inAdder0.str() << endl;
		op->vhdl << tab << op->declare(join(inAdder1Name, adderIndex), msb-lsb+2) << " <= \'0\' & " << inAdder1.str() << endl;
		op->vhdl << tab << op->declare(join(cinName, adderIndex)) << " <= " << cin.str() << endl;
		
		op->syncCycleFromSignal(join(inAdder0Name,adderIndex));
		op->syncCycleFromSignal(join(inAdder1Name,adderIndex));
		op->syncCycleFromSignal(join(cinName,adderIndex));
		
		op->manageCriticalPath( op->getTarget()->localWireDelay() + op->getTarget()->adderDelay(msb-lsb+2) );

#if 0
		//experimental
		
		IntAdder* adder = new IntAdder(op->getTarget(), msb-lsb+2);
		//op->getOpListR().push_back(adder);
		op->addSubComponent(adder);

		op->inPortMap(adder, "X", join(inAdder0Name,adderIndex));
		op->inPortMap(adder, "Y", join(inAdder1Name,adderIndex));
		op->inPortMap(adder, "Cin", join(cinName,adderIndex));

		op->outPortMap(adder, "R", join(outAdder,adderIndex));
		
		op->vhdl << tab << op->instance(adder, join("Adder_bh", getGUid(), "_", adderIndex));
		op->syncCycleFromSignal(join(outAdder,adderIndex));
		//op->setCriticalPath( adder->getOutputDelay("R") );

#else
		op->vhdl << tab << op->declare(join(outAdder, adderIndex), msb-lsb+2) << " <= "
			<<  join(inAdder0Name, adderIndex) << " + " << join(inAdder1Name,adderIndex) << " + " << join(cinName, adderIndex) << ";" <<endl ;

#endif


		for(int i=lsb; i<msb + 2 ; i++)
		{
			addBit(i, join(outAdder, adderIndex,"(",i-lsb,")"),"",3); //adder working as a compressor = type 3 for added bit
		}


		adderIndex++;
	}
	
	
	//FIXME: for now, the adder tree-based compression works on all the bitheap
	//	the bits are assumed all inside the bitheap, all available before the 
	//	begining of the compression
	//	Possible solution: replace the use of all bits, with the use of those 
	//		given by cnt[] vector
	void BitHeap::applyAdderTreeCompression()
	{
		int adderCount = 0;
		int nbRows = getMaxHeight();
		std::list<std::string> generatedAdderNames;
		std::list<int> generatedAddersCount, generatedAddersIndexLeft, generatedAddersIndexRight;
		ostringstream compressionResult;
		
		while(nbRows > 1)
		{
			REPORT(DEBUG, "Column height before adder tree round");
			for (int i=0; i<bits.size(); i++) {
				REPORT(DEBUG, "   w=" << i << ":\t height=" << bits[i].size());
				printColumnInfo(i);
			}
			
			for(int i=0; i<nbRows/2; i++)
			{
				int term1IndexLeft = -1, term2IndexLeft = -1, count;
				int term1IndexLeftNew;
				int term1IndexRight, term1IndexRightNew, term2IndexRight;
				int lastUsableColumn;
				int fullAdderIndexLeft;
				ostringstream term1String, term2String, term3String;
				WeightedBit *lastBit;
				
				//look for the index of the columns most to left, which are not empty
				count = bits.size()-1;
				while(((term1IndexLeft == -1) || (term2IndexLeft == -1)) && (count >= 0))
				{
					if(term1IndexLeft == -1)
					{
						while((count >= 0) && (bits[count].empty() || (bits[count].size() < 1)))
							count--;
						term1IndexLeft = count;
					}
					else if(term2IndexLeft == -1)
					{
						if((count == term1IndexLeft) && (!bits[count].empty()) && (bits[count].size() > 1))
							term2IndexLeft = count;
						
						while((count >= 0) && (bits[count].empty() || (bits[count].size() < 2)))
							count--;
						term2IndexLeft = count;
					}else
						break;
				}
				
				//term1IndexLeft should be > -1, from the
				//	assumption that getMaxHeight() returns the number of rows
				
				//term2IndexLeft might be -1, because of the fact that the carry-in 
				//	bit might have already consumed the remaining bit
				//if term2IndexLeft==-1 
				if(term2IndexLeft == -1)
				{
					break;
				}
				
				//now look for the corresponding index to the right
				//	look for the last column that has at least a bit in it
				count = term1IndexLeft;
				lastUsableColumn = term1IndexLeft;
				while(count >= 0)
				{
					if(bits[count].size() >= 1)
						lastUsableColumn = count;
					count--;
				}
				term1IndexRight = lastUsableColumn;
				
				count = term2IndexLeft;
				lastUsableColumn = term2IndexLeft;
				while(count >= 0)
				{
					if(bits[count].size() >= 2)
						lastUsableColumn = count;
					count--;
				}
				term2IndexRight = lastUsableColumn;
				
				//now create the two terms to the addition
				//	check to see which is the true right limit
				//	the left limit is necessarly that of term 1
				if(term2IndexRight >= term1IndexRight)
					term1IndexRightNew = term2IndexRight;
				else
					term1IndexRightNew = term1IndexRight;
					
				//if the number of remaining rows is at most 2, just perform the addition
				//	if not, perform the addition on as few bits as possible
				if(nbRows > 2)
				{
					if(term2IndexLeft <= term1IndexLeft)
						term1IndexLeftNew = term2IndexLeft;
					else
						term1IndexLeftNew = term1IndexLeft;
				}
				else
				{
					term1IndexLeftNew = term1IndexLeft;
				}
				
				/*
				//OPTIONAL
				//now look for the smallest top index for which the resulting 
				//	adder is full on both addends
				if(nbRows > 3)
				{
					fullAdderIndexLeft = term1IndexRightNew;
					while(fullAdderIndexLeft < term1IndexLeft)
					{
						if(bits[fullAdderIndexLeft].size() >= 2)
						{
							fullAdderIndexLeft++;
							continue;
						}
						fullAdderIndexLeft--;
						
						break;
					}
					
					term1IndexLeftNew = fullAdderIndexLeft;
					term2IndexLeft = fullAdderIndexLeft;
				}
				*/
				
				//synchronize the bits
				lastBit = bits[term1IndexLeft].front();
				for(int j=term1IndexLeftNew; j>=term1IndexRightNew; j--)
				{
					if(bits[j].size() > 0)
					{
						int syncLoopCount = 0;
						
						for(list<WeightedBit*>::iterator it=bits[j].begin(); ((it!=bits[j].end()) && (syncLoopCount<2)); it++)
						{
							if(*lastBit < **it)
								lastBit = *it;
								
							syncLoopCount++;
						}
					}
				}
				//for timing purposes
				op->setCycle(lastBit->getCycle());
				//op->syncCycleFromSignal(lastBit->getName());		//working
				op->setCriticalPath(lastBit->getCriticalPath(op->getCurrentCycle()));
				
				//create the first term
				for(int j=term1IndexLeftNew; j>=term1IndexRightNew; j--)
				{
					list<WeightedBit*>::iterator it = bits[j].begin();

					if(bits[j].size() >= 1)
					{
						term1String << (*it)->getName();
						removeBit(j, 0);
					}
					else
						term1String << "\'0\'";

					if(j != term1IndexRightNew)
						term1String << " & ";
				}
				
				//create the second term
				for(int j=term2IndexLeft; j>=term2IndexRight; j--)
				{
					list<WeightedBit*>::iterator it;
					
					if(bits[j].empty())
					{
						term2String << "\'0\'";
					}
					else
					{
						it = bits[j].begin();
						term2String << (*it)->getName();
						removeBit(j, 0);
					}
						
					if(j != term2IndexRight)
						term2String << " & ";
				}
				
				//create the carry-in
				if(bits[term2IndexRight].size() != 0)
				{
					term3String << bits[term2IndexRight].front()->getName();
					removeBit(term2IndexRight, 0);
				}
				else
				{
					term3String << "\'0\'";
				}
				
				//create the two terms for the addition
				string term1Name = join("inAdderTree_1_bh", getGUid(), "_adder", adderCount);
				string term2Name = join("inAdderTree_2_bh", getGUid(), "_adder", adderCount);
				string outputName = join("outAdderTree_bh", getGUid(), "_adder", adderCount);
				
				REPORT(DEBUG, "term1IndexLeft=" << term1IndexLeft << " term1IndexRight=" << term1IndexRight << " term1IndexLeftNew=" << term1IndexLeftNew << " term1IndexRightNew=" << term1IndexRightNew);
				REPORT(DEBUG, "term2IndexLeft=" << term2IndexLeft << " term2IndexRight=" << term2IndexRight);

				op->vhdl << tab << op->declare(term1Name, term1IndexLeftNew-term1IndexRightNew+1+1) << " <= \'0\' & " << term1String.str() << ";" << endl;
				op->vhdl << tab << op->declare(term2Name, term1IndexLeftNew-term1IndexRightNew+1+1) << " <= \'0\' & " << zg(term1IndexLeftNew-term2IndexLeft) << " & " << term2String.str() << ";" << endl;
				op->vhdl << tab << op->declare(outputName, term1IndexLeftNew-term1IndexRightNew+1+1) << " <= " << term1Name << " + " << term2Name << " + " << term3String.str() << ";" << endl;
				
				generatedAddersCount.push_back(adderCount++);
				generatedAdderNames.push_back(outputName);
				generatedAddersIndexLeft.push_back(term1IndexLeftNew+1);		//+1 because of the overflow bit
				generatedAddersIndexRight.push_back(term1IndexRightNew);
				
				plotter->heapSnapshot(true, lastBit->getCycle(), lastBit->getCriticalPath(lastBit->getCycle()));
			}
			
			//add the results of this round of additions back to the bit heap
			//for(int i=0; i<nbRows/2; i++)
			//TEST
			int finalLoopCount = (generatedAddersCount.empty() ? 0 : generatedAddersCount.size());
			
			for(int i=0; i<finalLoopCount; i++)
			{
				int top = generatedAddersIndexLeft.front();
				int bottom = generatedAddersIndexRight.front();
				string currentAdderName = generatedAdderNames.front();
				
				generatedAddersCount.pop_front();
				generatedAddersIndexLeft.pop_front();
				generatedAddersIndexRight.pop_front();
				generatedAdderNames.pop_front();
				
				for(int j=top; j>=bottom; j--)
				{
					addBit(j, join(currentAdderName, "(" , j-bottom, ")"), "", 3); //adder working as a compressor = type 3 for added bit
				}
			}
			
			nbRows = getMaxHeight();
		}
		
		//create the result of the compression
		for(int i=maxWeight-1; i>=0; i--)
		{
			list<WeightedBit*>::iterator it = bits[i].begin();

			if(bits[i].size() >= 1)
			{
				compressionResult << (*it)->getName();
				removeBit(i, 0);
			}
			else
				compressionResult << "\'0\'";

			if(i > 0)
				compressionResult << " & ";
		}
		
		//assign the result of the adder tree reduction to the result of the compression
		op->vhdl << tab << op->declare(join("CompressionResult", guid), maxWeight) << " <= " << compressionResult.str() << ";" << endl;
	}


	//returns a pointer to the bit which has the biggest criticalPath+cycle
	//combination, considering the given column limits (both included)
	WeightedBit* BitHeap::getLatestBit(int lsbColumn, int msbColumn)
	{

		double maxCycle=0;
		double maxCP = 0;
		WeightedBit *b=0;
		for(int w=lsbColumn; w<=msbColumn; w++)
			{
				if(bits[w].size() > 0)
					{

						for(list<WeightedBit*>::iterator it = bits[w].begin(); it!=bits[w].end(); ++it)
							{
								if (maxCycle < (*it)->getCycle())
									{
										maxCycle = (*it)->getCycle();
										maxCP = (*it)->getCriticalPath(maxCycle);
										b = *it;
									}
								else
									if ((maxCycle == (*it)->getCycle()) &&  (maxCP <= (*it)->getCriticalPath(maxCycle)))
										{
											maxCP = (*it)->getCriticalPath(maxCycle);
											b = *it;
										}
							}

					}
			}
		if(b==0)
			THROWERROR("All columns are void!");

		return b;
	}



	//the final addition
	void BitHeap::generateFinalAddVHDL(bool isXilinx)
	{
		REPORT(DEBUG, "in generateFinalAddVHDL");
		
		if(getMaxHeight() < 2)
		{
			if(getMaxHeight() == 1)
				concatenateLSBColumns();

			op->vhdl << tab << op->declare(join("CompressionResult", guid), (maxWeight+1)) << " <= '0'&" << 
				join("tempR_bh", guid, "_", chunkDoneIndex-1);

			//adding the rightmost bits
			for(int i=chunkDoneIndex-2; i>=0; i--)
				op->vhdl <<  " & " << join("tempR_bh", guid, "_", i);

			op->vhdl << ";" << endl;

			return;
		}
		
		if(isXilinx)
		{
			stringstream inAdder0, inAdder1, outAdder;
			int i;
			int minIndex, maxIndex;
			
			//determine the index of the column where the addition should start
			//	not necessarly the minimum index line
			minIndex = minWeight;
			while((minIndex<maxWeight) && (bits[minIndex].size()<2))
				minIndex++;
			
			maxIndex = maxWeight;
			
			//forming the input signals for the first and second line
			i = maxIndex-1;
			while(i >= minIndex)
			{
				REPORT(DEBUG,"i=   " << i);
				if(i >= 0)
				{
					list<WeightedBit*>::iterator it = bits[i].begin();
					if(bits[i].size()==2)
					{
						inAdder0 << (*it)->getName();
						it++;
						inAdder1 << (*it)->getName();
					}
					else if (bits[i].size()==1)
					{
						inAdder0 << (*it)->getName();
						inAdder1 << "\'0\'";
					}
					else
					{
						inAdder0 << "\'0\'";
						inAdder1 << "\'0\'";
					}

					if (i != minIndex)
					{
						inAdder0 << " & ";
						inAdder1 << " & ";
					}
				}

				i--;
			}

			inAdder0 << ";";
			inAdder1 << ";";

			//managing the pipeline
			WeightedBit* b = getLatestBit(minWeight, maxWeight-1);			

			op->setCycle(b ->getCycle());
			op->setCriticalPath(b->getCriticalPath(op->getCurrentCycle()));
			op->manageCriticalPath(op->getTarget()->localWireDelay() + op->getTarget()->adderDelay(maxWeight-minWeight+1));
			
			string inAdder0Name;
			string inAdder1Name;
			string cinName;
			string outAdderName;
			
			if(minIndex == maxIndex)
			{
				outAdderName = "";
			}
			else
			{
				inAdder0Name = join("finalAdderIn0_bh", getGUid());
				inAdder1Name = join("finalAdderIn1_bh", getGUid());
				cinName = join("finalAdderCin_bh", getGUid());
				outAdderName = join("finalAdderOut_bh", getGUid());

				op->vhdl << tab << op->declare(inAdder0Name, maxIndex-minIndex+1) << " <= \"0\" & " << inAdder0.str() << endl;
				op->vhdl << tab << op->declare(inAdder1Name, maxIndex-minIndex+1) << " <= \"0\" & " << inAdder1.str() << endl;
				op->vhdl << tab << op->declare(cinName) << " <= \'0\';" << endl;
				
				IntAdder* adder = new IntAdder(op->getTarget(), maxIndex-minIndex+1);
				op->addSubComponent(adder);
		
				op->inPortMap(adder, "X", inAdder0Name);
				op->inPortMap(adder, "Y", inAdder1Name);
				op->inPortMap(adder, "Cin", cinName);
				op->outPortMap(adder, "R", outAdderName);
		
				op->vhdl << tab << op->instance(adder, join("Adder_final", getGUid(), "_", adderIndex));
				op->syncCycleFromSignal(outAdderName);
				op->setCriticalPath( adder->getOutputDelay("R") );
			}
			
			//create the rest of the pieces that make up the result
			concatenateLSBColumns();
			
			op->vhdl << tab << "-- concatenate all the compressed chunks" << endl;
			//result
			op->syncCycleFromSignal(outAdderName);
			op->vhdl << tab << op->declare(join("CompressionResult", guid), (maxWeight+1)) << " <= " << outAdderName;

			//adding the rightmost bits
			for(int i=chunkDoneIndex-1; i>=0; i--)
				op->vhdl <<  " & " << join("tempR_bh", guid, "_", i);

			op->vhdl << ";" << endl;
		}
		else
		{
			stringstream inAdder0, inAdder1, inAdder2, outAdder;

			unsigned i=maxWeight-1;

			//forming the input signals for the first, second and third line
			while((i>=minWeight)&&(i<maxWeight))
			{
				REPORT(DEBUG,"i=   "<<i);
				if(i>=0)
				{
					list<WeightedBit*>::iterator it = bits[i].begin();
					if(bits[i].size()==3)
					{
						inAdder0 << (*it)->getName();
						it++;
						inAdder1 << (*it)->getName();
						it++;
						inAdder2 << (*it)->getName();
					}else
					{
						if(bits[i].size()==2)
						{
							inAdder0 << (*it)->getName();
							it++;
							inAdder1 << (*it)->getName();
							inAdder2 << "\'0\'";

						}else
						{
							if (bits[i].size()==1)
							{
								inAdder0 << (*it)->getName();
								inAdder1 << "\'0\'";
								inAdder2 << "\'0\'";

							}else
							{
								inAdder0 << "\'0\'";
								inAdder1 << "\'0\'";
								inAdder2 << "\'0\'";

							}
						}
					}

					if (i!=minWeight)
					{
						inAdder0 <<" & ";
						inAdder1 <<" & ";
						inAdder2 <<" & ";
					}
				}

				--i;
			}

			inAdder0 << ";";
			inAdder1 << ";";
			inAdder2 << ";";

			WeightedBit* b = getLatestBit(minWeight, maxWeight-1);
			//managing the pipeline
			if(b)
			{
				op->setCycle(  b ->getCycle()  );
				op->setCriticalPath( b->getCriticalPath(op->getCurrentCycle()));
				op->manageCriticalPath(op->getTarget()->localWireDelay() +
				                       op->getTarget()->adderDelay(maxWeight-minWeight));
			}

			string inAdder0Name = join("finalAdderIn0_bh", getGUid());
			string inAdder1Name = join("finalAdderIn1_bh", getGUid());
			string inAdder2Name = join("finalAdderIn2_bh", getGUid());
			string outAdderName = join("finalAdderOut_bh", getGUid());

			op->vhdl << tab << op->declare(inAdder0Name, maxWeight-minWeight) << " <= " << inAdder0.str() << endl;
			op->vhdl << tab << op->declare(inAdder1Name, maxWeight-minWeight) << " <= " << inAdder1.str() << endl;
			op->vhdl << tab << op->declare(inAdder2Name, maxWeight-minWeight) << " <= " << inAdder2.str() << endl;

			//Old
			/*
			op->vhdl << tab << op->declare(outAdderName, maxWeight-minWeight+2)
			         << " <= (\"00\" & "<< inAdder0Name << ") + (\"0\" & " << inAdder1Name
			         << ") + (\"00\" & " << inAdder2Name << ");" << endl;
			*/			
			
			// EXPERIMENTAL ----------------------------------------------------
			int subAddSize, subAdd3Size;
			
			op->getTarget()->suggestSubaddSize(subAddSize, maxWeight-minWeight);
			op->getTarget()->suggestSubadd3Size(subAdd3Size, maxWeight-minWeight);
			
			REPORT(DEBUG, "preparing for addition on " << maxWeight-minWeight << " bits; will be split into chunks of size " << subAddSize);
			REPORT(DEBUG,"preparing for addition3 on " << maxWeight-minWeight << " bits; will be split into chunks of size " << subAdd3Size);
			REPORT(DEBUG, tab << "maxWeight=" << maxWeight << " and minWeight=" << minWeight);
			
			//perform addition on the pieces
			for(unsigned int i=0; i<(maxWeight-minWeight)/subAddSize; i++)
			{
				if(i == 0)
				{
					//handle timing		-- TODO: redo timing
					op->setCycleFromSignal(inAdder0Name);
					op->syncCycleFromSignal(inAdder1Name);
					op->syncCycleFromSignal(inAdder2Name);
					op->manageCriticalPath(op->getTarget()->localWireDelay() + op->getTarget()->adder3Delay(subAddSize+2));
					
					op->vhdl << tab << op->declare(join(outAdderName, "_int_", i), subAddSize+2) << " <= "
						<< "(\"00\" & " << inAdder0Name << range (subAddSize-1, 0) << ")"
						<< " + "
						<< "(\"00\" & " << inAdder1Name << range (subAddSize-1, 0) << ")"
						<< " + "
						<< "(\"00\" & " << inAdder2Name << range (subAddSize-1, 0) << ");" << endl;
				}else
				{
					//handle timing		-- TODO: redo timing
					op->setCycleFromSignal(join(outAdderName, "_int_", i-1));
					op->manageCriticalPath(op->getTarget()->localWireDelay() + op->getTarget()->lutDelay());
					
					//concatenate the carrys from the previous addition
					if(i == 1)
					{
						op->vhdl << tab << op->declare(join(outAdderName, "_int_", i, "_lsb_select"), 2) 
							<< " <= " << join(outAdderName, "_int_", i-1) << range(subAddSize+1, subAddSize) << ";" << endl;
					}else
					{
						op->vhdl << tab << op->declare(join(outAdderName, "_int_", i, "_lsb_select"), 2) 
							<< " <= " << join(outAdderName, "_int_", i-1) << range(subAddSize+3, subAddSize+2) << ";" << endl;
					}
					
					op->vhdl << tab << "with " << join(outAdderName, "_int_", i, "_lsb_select") << " select " << endl
							<< tab << tab << op->declare(join(outAdderName, "_int_", i, "_a0b0prime"), 2) << " <= " << endl
							<< tab << tab << tab << "\"00\" when \"00\"," << endl
							<< tab << tab << tab << "\"11\" when \"10\"," << endl
							<< tab << tab << tab << "\"01\" when \"01\"," << endl
							<< tab << tab << tab << "\"00\" when \"11\"," << tab << tab << "-- should never occur" << endl
							<< tab << tab << tab << "\"--\" when others;" << endl;
					
					//handle timing		-- TODO: redo timing
					op->syncCycleFromSignal(join(outAdderName, "_int_", i, "_a0b0prime"));
					op->manageCriticalPath(op->getTarget()->localWireDelay() + op->getTarget()->adder3Delay(subAddSize+4));
					
					//perform the actual ternary addition
					op->vhdl << tab << op->declare(join(outAdderName, "_int_", i), subAddSize+4) << " <= "
						<< "(\"00\" & " << inAdder0Name << range(subAddSize*(i+1)-1, subAddSize*i+1) << " & " << join(outAdderName, "_int_", i, "_a0b0prime") << of(1) << " & " << inAdder0Name << of(subAddSize*i) << " & \"0\")"
						<< " + "
						<< "(\"00\" & " << inAdder1Name << range(subAddSize*(i+1)-1, subAddSize*i+1) << " & " << join(outAdderName, "_int_", i, "_a0b0prime") << of(0) << " & " << inAdder0Name << of(subAddSize*i) << " & " << inAdder1Name << of(subAddSize*i) << ")"
						<< " + "
						<< "(\"00\" & " << inAdder2Name << range(subAddSize*(i+1)-1, subAddSize*i) << " & " << inAdder1Name << of(subAddSize*i) << " & " << inAdder1Name << of(subAddSize*i) << ");" << endl;
				}
			}
			
			//the leftover bits
			unsigned int currentLevel = (maxWeight-minWeight) / subAddSize;
			
			if(((maxWeight-minWeight)%subAddSize > 0) && ((maxWeight-minWeight)>subAddSize))
			{
				//handle timing
				op->setCycleFromSignal(join(outAdderName, "_int_", currentLevel-1));
				op->manageCriticalPath(op->getTarget()->localWireDelay() + op->getTarget()->lutDelay());
				
				if(currentLevel == 1)
				{
					op->vhdl << tab << op->declare(join(outAdderName, "_int_", currentLevel, "_lsb_select"), 2) 
						<< " <= " << join(outAdderName, "_int_", currentLevel-1) << range(subAddSize+1, subAddSize) << ";" << endl;
				}else
				{
					op->vhdl << tab << op->declare(join(outAdderName, "_int_", currentLevel, "_lsb_select"), 2) 
						<< " <= " << join(outAdderName, "_int_", currentLevel-1) << range(subAddSize+3, subAddSize+2) << ";" << endl;
				}
				
				op->vhdl << tab << "with " << join(outAdderName, "_int_", currentLevel, "_lsb_select") << " select " << endl
						<< tab << tab << op->declare(join(outAdderName, "_int_", currentLevel, "_a0b0prime"), 2) << " <= " << endl
						<< tab << tab << tab << "\"00\" when \"00\"," << endl
						<< tab << tab << tab << "\"11\" when \"10\"," << endl
						<< tab << tab << tab << "\"01\" when \"01\"," << endl
						<< tab << tab << tab << "\"00\" when \"11\"," << tab << tab << "-- should never occur" << endl
						<< tab << tab << tab << "\"--\" when others;" << endl;
				
				//handle timing
				op->syncCycleFromSignal(join(outAdderName, "_int_", currentLevel, "_lsb_select"));
				op->manageCriticalPath(op->getTarget()->localWireDelay() + op->getTarget()->adder3Delay(maxWeight-minWeight-subAddSize*currentLevel+4));
					
				//perform the actual ternary addition
				if(maxWeight-minWeight-subAddSize*currentLevel == 1)
				{
					op->vhdl << tab << op->declare(join(outAdderName, "_int_", currentLevel), maxWeight-minWeight-subAddSize*currentLevel+4) << " <= "
						<< "(\"00\" & " << join(outAdderName, "_int_", currentLevel, "_a0b0prime") << "(1 downto 1) " << " & " << inAdder0Name << "(" << maxWeight-minWeight-1 << " downto " << maxWeight-minWeight-1 << ")" << " & \"0\")"
						<< " + "
						<< "(\"00\" & " << join(outAdderName, "_int_", currentLevel, "_a0b0prime") << "(0 downto 0) " << " & " << inAdder0Name << "(" << maxWeight-minWeight-1 << " downto " << maxWeight-minWeight-1 << ")" << " & " << inAdder1Name << "(" << maxWeight-minWeight-1 << " downto " << maxWeight-minWeight-1 << ")" << ")"
						<< " + "
						<< "(\"00\" & " << inAdder2Name << "(" << maxWeight-minWeight-1 << " downto " << maxWeight-minWeight-1 << ")" << " & " << inAdder1Name << "(" << maxWeight-minWeight-1 << " downto " << maxWeight-minWeight-1 << ")" << " & " << inAdder1Name << "(" << maxWeight-minWeight-1 << " downto " << maxWeight-minWeight-1 << ")" << ");" << endl;
				}else
				{
					op->vhdl << tab << op->declare(join(outAdderName, "_int_", currentLevel), maxWeight-minWeight-subAddSize*currentLevel+4) << " <= "
						<< "(\"00\" & " << inAdder0Name << range(maxWeight-minWeight-1, subAddSize*currentLevel+1) << " & " << join(outAdderName, "_int_", currentLevel, "_a0b0prime") << of(1) << " & " << inAdder0Name << of(subAddSize*currentLevel) << " & \"0\")"
						<< " + "
						<< "(\"00\" & " << inAdder1Name << range(maxWeight-minWeight-1, subAddSize*currentLevel+1) << " & " << join(outAdderName, "_int_", currentLevel, "_a0b0prime") << of(0) << " & " << inAdder0Name << of(subAddSize*currentLevel) << " & " << inAdder1Name << of(subAddSize*currentLevel) << ")"
						<< " + "
						<< "(\"00\" & " << inAdder2Name << range(maxWeight-minWeight-1, subAddSize*currentLevel) << " & " << inAdder1Name << of(subAddSize*currentLevel) << " & " << inAdder1Name << of(subAddSize*currentLevel) << ");" << endl;
				}					
			}
			
			//splitting wasn't/was necessary
			if(subAddSize >= (int)( maxWeight-minWeight))
			{
				//handle timing
				op->setCycleFromSignal(inAdder0Name);
				op->syncCycleFromSignal(inAdder1Name);
				op->syncCycleFromSignal(inAdder2Name);
				op->manageCriticalPath(op->getTarget()->localWireDelay() +
						op->getTarget()->adder3Delay(maxWeight-minWeight+2));
				
				op->vhdl << tab << op->declare(outAdderName, maxWeight-minWeight+2)
					<< " <= (\"00\" & "<< inAdder0Name << ") + (\"00\" & " << inAdder1Name
					<< ") + (\"00\" & " << inAdder2Name << ");" << endl;
			}else
			{
				//join all the intermediary results together
				if(((maxWeight-minWeight)%subAddSize > 0) && ((maxWeight-minWeight)>subAddSize))
				{
					//handle timing
					op->syncCycleFromSignal(join(outAdderName, "_int_", currentLevel));
					op->manageCriticalPath(op->getTarget()->localWireDelay());
				}else
				{
					//handle timing
					op->syncCycleFromSignal(join(outAdderName, "_int_", currentLevel-1));
					op->manageCriticalPath(op->getTarget()->localWireDelay());
				}
				
				op->vhdl << tab << op->declare(outAdderName, maxWeight-minWeight+2) << " <= ";
				
				if(((maxWeight-minWeight)%subAddSize > 0) && ((maxWeight-minWeight)>subAddSize))
				{
					op->vhdl << join(outAdderName, "_int_", (maxWeight-minWeight)/subAddSize) << range(maxWeight-minWeight-subAddSize*currentLevel+3, 2) << " & ";
				}
				
				for(unsigned int i=0; i<(maxWeight-minWeight)/subAddSize; i++)
				{
					unsigned int index = (maxWeight-minWeight)/subAddSize - i - 1;
					
					if(index == 0)
					{
						op->vhdl << join(outAdderName, "_int_", index) << range(subAddSize-1, 0) << ";" << endl;
					}else if((index == (maxWeight-minWeight+2)/subAddSize - 1) && !(((maxWeight-minWeight+2)%subAddSize > 0) && ((maxWeight-minWeight+2)>subAddSize)))
					{
						op->vhdl << join(outAdderName, "_int_", index) << range(subAddSize+3, 2) << " & ";
					}else
					{
						op->vhdl << join(outAdderName, "_int_", index) << range(subAddSize+1, 2) << " & ";
					}
				}
			}
			//end of Altera -----------------------------------------------------------------
			
			
			op->vhdl << tab << "-- concatenate all the compressed chunks" << endl;
			//result
			op->syncCycleFromSignal(outAdderName);
			op->vhdl << tab << op->declare(join("CompressionResult", guid), maxWeight+2) << " <= " << outAdderName;

			//adding the rightmost bits
			for(int i=chunkDoneIndex-1; i>=0; i--)
				op->vhdl <<  " & " << join("tempR_bh", guid, "_", i);

			op->vhdl << ";" << endl;
		}

	}


	string BitHeap::getSumName() {
		return join("CompressionResult", guid);
	}




	//the compression
	void BitHeap::compress(int stage)
	{
		plottingStage = stage;

		unsigned w;
		didCompress = false;

		concatenateLSBColumns();

		//update cnt
		for(unsigned i=minWeight; i<maxWeight; i++)
		{
			cnt[i]=0;
			for(list<WeightedBit*>::iterator it = bits[i].begin(); it!=bits[i].end(); it++)
			{
				if((*it)->computeStage(stagesPerCycle, elementaryTime)<=stage)
				{
					cnt[i]++;
				}
			}
		}


		REPORT(DEBUG, "maxWeight=" << maxWeight);
		REPORT(DEBUG, "stage=" << stage);
		REPORT(DEBUG, "Column height before compression");
		for (w=0; w<bits.size(); w++)
		{
			REPORT(DEBUG, "   w=" << w << ":\t height=" << bits[w].size() << "\t cnt[w]=" << cnt[w]);
			printColumnInfo(w);
		}
		

		//extra additions for lsb columns
		unsigned index = minWeight;
		unsigned columnIndex;
		double timeLatestBitAdded=0.0e-12, timeFirstBitNotAdded=1;
		double adderDelay;
		unsigned adderMaxWeight = minWeight;

		WeightedBit *latestBitAdded, *possiblyLatestBitAdded;

		//search for lsb columns that won't be compressed at the current stage
		//REPORT(INFO, endl);

		while((index<maxWeight) && ((cnt[index]<=2)&&(cnt[index]>0)))
		{
			list<WeightedBit*>::iterator it = bits[index].begin();
			columnIndex = 0;
			
			while(columnIndex<cnt[index]-1)
			{
				it++;
				columnIndex++;
			}
			
			if (timeLatestBitAdded < (*it)->getCycle()*(1/op->getTarget()->frequency()) +
			    (*it)->getCriticalPath((*it)->getCycle()))
			{
				timeLatestBitAdded = (*it)->getCycle()*(1/op->getTarget()->frequency()) +
					(*it)->getCriticalPath((*it)->getCycle());
				possiblyLatestBitAdded = *it;
			}

			if((cnt[index] > 1) && (columnIndex < bits[index].size()-1))
			{
				it++;
				columnIndex++;
			}
			
			if (timeFirstBitNotAdded > (*it)->getCycle()*(1/op->getTarget()->frequency()) +
			    (*it)->getCriticalPath((*it)->getCycle()))
			{
				timeFirstBitNotAdded = (*it)->getCycle()*(1/op->getTarget()->frequency()) +
					(*it)->getCriticalPath((*it)->getCycle());
			}

			adderDelay = op->getTarget()->adderDelay(index-minWeight+1);// + op->getTarget()->localWireDelay();

			if ((timeLatestBitAdded + adderDelay) < timeFirstBitNotAdded)
			{
				adderMaxWeight = index;
				latestBitAdded = possiblyLatestBitAdded;
			}

			index++;
		}
		
		REPORT(DEBUG,"checked for adder in last columns; found adder from " << minWeight << " to " << adderMaxWeight);

		if(adderMaxWeight > minWeight)
		{
			//op->setCycle(  latestBitAdded ->getCycle()  );
			//op->setCriticalPath(   latestBitAdded ->getCriticalPath(op->getCurrentCycle()) );
			//op->manageCriticalPath( op->getTarget()->localWireDelay() +
			//                        op->getTarget()->adderDelay(adderMaxWeight-minWeight+1) );
			applyAdder(minWeight, adderMaxWeight, false);

			if(plottingCycle < op->getCurrentCycle())
			{
				plottingCycle = op->getCurrentCycle();
				plottingCP = op->getCriticalPath();
			}
			else
				if((plottingCycle ==  op->getCurrentCycle()) &&
				   (plottingCP < op->getCriticalPath()))
				{
					plottingCycle = op->getCurrentCycle();
					plottingCP = op->getCriticalPath();
				}


			//plotter->heapSnapshot(true, op->getCurrentCycle(), op->getCriticalPath());

			for(unsigned i=minWeight; i<=adderMaxWeight; i++)
				cnt[i]=0;

			//didCompress = true;
		}
		
		//update cnt
		for(unsigned i=minWeight; i<maxWeight; i++)
		{
			cnt[i]=0;
			for(list<WeightedBit*>::iterator it = bits[i].begin(); it!=bits[i].end(); it++)
			{
				if((*it)->computeStage(stagesPerCycle, elementaryTime)<=stage)
				{
					cnt[i]++;
				}
			}
		}
		

		//--------------- Compress with ADD3s ----------------------------------
		REPORT(DEBUG,"starting compression with ternary adders");

		//search for the starting column
		
		//bool bitheapCompressible = op->getTarget()->hasFastLogicTernaryAdders();
		bool bitheapCompressible = false;
		
		unsigned int lastCompressed = (adderMaxWeight > minWeight) ? adderMaxWeight+1 : minWeight;
		bool performedCompression;

		//go through the bitheap from left to right and form adders
		while(bitheapCompressible)
		{
			index = lastCompressed;
			performedCompression = false;

			while(index<maxWeight){
				//see if an addition can be started from this index
				//    rule: at least 2 bits in a column; no gap (column where
				//        there is only 1 bit in the column) bigger than 2 columns
				unsigned int endAddChain = index;
				bool continueAddChain = true;

				while((continueAddChain) && (endAddChain <= maxWeight))
				{
					/*if(cnt[endAddChain] >= 2)*/ if(cnt[endAddChain] >= 3)
					{
						//at least 3 bit in the column -> add no matter what
						if(endAddChain == maxWeight)
						{
							continueAddChain = false;
						}else
						{
							endAddChain++;
						}

					}else if(cnt[endAddChain] == 2)
					{
						//two bits in the column -> no more than two consecutive 
						//    columns like this
						if(((endAddChain>=1) && (cnt[endAddChain-1]>=3)) || ((endAddChain>=2) && (cnt[endAddChain-1]>=2) && (cnt[endAddChain-2]>=3)))
						{
							if((endAddChain<maxWeight) && (cnt[endAddChain-1]==2) && (cnt[endAddChain+1]>=2))
							{
								endAddChain++;
							}else
							{
								if(endAddChain>0)
									endAddChain--;
								continueAddChain = false;
							}								
						}else
						{
							if(endAddChain>0)
								endAddChain--;
							continueAddChain = false;
						}

					}else if(cnt[endAddChain] == 1)
					{
						//one bit in the column -> add only if the last column
						//    had at least 2 bits or the one before had 3
						//    and the next column has at least 2 bits
						if(((endAddChain>=1) && (cnt[endAddChain-1]>=2)) || ((endAddChain>=2) && (cnt[endAddChain-2]>=3)))
						{
							if((endAddChain<maxWeight) && (cnt[endAddChain+1]>=2))
							{
								endAddChain++;
							}else
							{
								if(endAddChain>0)
									endAddChain--;
								continueAddChain = false;
							}
						}else
						{
							if(endAddChain>0)
								endAddChain--;
							continueAddChain = false;
						} 

					}else
					{
						//empty column -> add only if the last (or the one before)
						//    column had at least 3 bits
						//    and the next column has at least 2 bits
						if(endAddChain == index)
						{
							continueAddChain = false;
							continue;
						}
						
						if(((endAddChain>=1) && (cnt[endAddChain-1]>=3)) || ((endAddChain>=2) && (cnt[endAddChain-1]>=2) && (cnt[endAddChain-2]>=3)))
						{
							if((endAddChain<maxWeight-1) && (cnt[endAddChain+1]>=2) && (cnt[endAddChain+2]>=2))
							{
								endAddChain++;
							}else if((endAddChain<maxWeight) && (cnt[endAddChain+1]>=2))
							{
								endAddChain++;
							}else
							{
								if(endAddChain>0)
									endAddChain--;
								continueAddChain = false;
							}
						}else
						{
							if(endAddChain>0)
								endAddChain--;
							continueAddChain = false;
						}
					}
					
					if(endAddChain-index == maxAdd3Length)
					{
						continueAddChain = false;
					}
				}
				
				if((endAddChain > index) && (endAddChain-index >= minAdd3Length) && (endAddChain-index <= maxAdd3Length))
				{
					//found possible addition
					stringstream addInput0, addInput1, addInput2;
					string addInput0Name, addInput1Name, addInput2Name, addOutputName;

					performedCompression = true;
					didCompress = true;

					//for timing purposes
					WeightedBit *lastBit = bits[index].front();
					WeightedBit *currentBit = bits[index].front();
					
					REPORT(DEBUG,"checking columns for critical path for found adder");
					for(unsigned int i=index; i<=endAddChain; i++)
					{
						REPORT(DEBUG,"columns for the for found adder: index=" << i << " should have bits=" << cnt[i] << " actually has bits=" << bits[i].size());
						
						if(i<maxWeight)
						{
							REPORT(DEBUG,"       next column has bits=" << cnt[i+1]);
						}
						else
						{
							REPORT(DEBUG,"       next column has bits= already at maxWeight");
						}
					}

					REPORT(DEBUG,"computing critical path for found adder");

					//compute the critical path
					for(unsigned int i=index; i<=endAddChain; i++)
					{
						if(cnt[i]>0)
						{
							currentBit = latestInputBitToCompressor(i, ((cnt[i]>3) ? 3 : cnt[i]), 0);
							if(lastBit < currentBit)
								lastBit = currentBit;		
						}
					}

					//create the right side of the terms for the 3-input addition
					for(unsigned int i=endAddChain; i>=index; i--){
						list<WeightedBit*>::iterator it = bits[i].begin();

						if(cnt[i]>=3)
						{
							addInput0 << (*it)->getName() << ((i != index) ? " & " : ";");
							it++;
							addInput1 << (*it)->getName() << ((i != index) ? " & " : ";");
							it++;
							addInput2 << (*it)->getName() << ((i != index) ? " & " : ";");
							removeCompressedBits(i, 3);
							cnt[i] -= 3;
						}else if(cnt[i]==2)
						{
							addInput0 << (*it)->getName() << ((i != index) ? " & " : ";");
							it++;
							addInput1 << (*it)->getName() << ((i != index) ? " & " : ";");
							addInput2 << "\'0\'" << ((i != index) ? " & " : ";");
							removeCompressedBits(i, 2);
							cnt[i] -= 2;
						}else if(cnt[i]==1)
						{
							addInput0 << (*it)->getName() << ((i != index) ? " & " : ";");
							addInput1 << "\'0\'" << ((i != index) ? " & " : ";");
							addInput2 << "\'0\'" << ((i != index) ? " & " : ";");
							removeCompressedBits(i, 1);
							cnt[i] -= 1;
						}else
						{
							addInput0 << "\'0\'" << ((i != index) ? " & " : ";");
							addInput1 << "\'0\'" << ((i != index) ? " & " : ";");
							addInput2 << "\'0\'" << ((i != index) ? " & " : ";");
						}
						
						if(i == 0)
							break;
					}

					//handle timing and the critical path
					op->setCycle(lastBit->getCycle());
					op->setCriticalPath(lastBit->getCriticalPath(op->getCurrentCycle()));
					op->manageCriticalPath( op->getTarget()->localWireDelay() + op->getTarget()->adderDelay(endAddChain-index+1+2) );

					//handle plotting timing
					if(plottingCycle < op->getCurrentCycle())
					{
						plottingCycle = op->getCurrentCycle();
						plottingCP = op->getCriticalPath();
					}
					else if((plottingCycle == op->getCurrentCycle()) && (plottingCP < op->getCriticalPath()))
					{
						plottingCycle = op->getCurrentCycle();
						plottingCP = op->getCriticalPath();
					}

					//create the terms of the addition
					addInput0Name = join("addInput0_bh", getGUid(), "_");
					addInput1Name = join("addInput1_bh", getGUid(), "_");
					addInput2Name = join("addInput2_bh", getGUid(), "_");
					addOutputName = join("addOutput_bh", getGUid(), "_");

					op->vhdl << tab << op->declare(join(addInput0Name, adder3Index), endAddChain-index+1+2) << " <= \"00\" & " << addInput0.str() << endl;
					op->vhdl << tab << op->declare(join(addInput1Name, adder3Index), endAddChain-index+1+2) << " <= \"00\" & " << addInput1.str() << endl;
					op->vhdl << tab << op->declare(join(addInput2Name, adder3Index), endAddChain-index+1+2) << " <= \"00\" & " << addInput2.str() << endl;

					op->vhdl << tab << op->declare(join(addOutputName, adder3Index), endAddChain-index+1+2) << " <= "
						<<  join(addInput0Name, adder3Index) << " + " << join(addInput1Name,adder3Index) << " + " << join(addInput2Name, adder3Index) << ";" << endl;
						
					//experimental
					//handle timing again
					//op->setCycleFromSignal(join(addOutputName, adder3Index), true);
					op->syncCycleFromSignal(join(addOutputName, adder3Index), true);

					//add the bits of the addition to the bitheap
					for(unsigned int i=index; i<=endAddChain + 2 ; i++)
					{
						addBit(i, join(addOutputName, adder3Index,"(",i-index,")"),"",3); //adder working as a compressor = type 3 for added bit
					}


					adder3Index++;

					index = endAddChain + 1;
				}else
				{
					//a new addition cannot start from this index, so go to the next
					index++;
				}

				//update exit condition on bitheapCompressible
				if((index == maxWeight) && (performedCompression == false))
				{
					bitheapCompressible = false;
				}
			}
		}
				
		//----------------------------------------------------------------------


		REPORT(DEBUG,"start compressing maxHeight=" << maxWeight);
		
		/*
		 * Try to use only optimal compressors. When going through the columns, 
		 * if the compressible bits in a column are not enough to fill an OPTIMAL 
		 * compressor, then leave the bits unprocessed and move on to the next column
		 */
		int i, j;
		
		i = minWeight;		//column index
		j = 0;				//compressor index
		while(j < possibleCompressors.size())
		{
			while(i < maxWeight)
			{
				while(
						((possibleCompressors[j]->getColumnSize(1) == 0) && (cnt[i] >= possibleCompressors[j]->getColumnSize(0))) ||
						((possibleCompressors[j]->getColumnSize(1)!=0) && (cnt[i] >= possibleCompressors[j]->getColumnSize(0)) && (cnt[i+1] >= possibleCompressors[j]->getColumnSize(1)))
					)
				{
					if(possibleCompressors[j]->getColumnSize(1)!=0)
					{
						if((i < bits.size()-1) && (cnt[i] >= possibleCompressors[j]->getColumnSize(0)) && (cnt[i+1]>=possibleCompressors[j]->getColumnSize(1)))
						{
							REPORT(DEBUG,endl);
							REPORT(DEBUG,"Using Compressor " << j <<" to reduce columns " << i << " and " << i+1);
							elemReduce(i, possibleCompressors[j]);
							cnt[i]-=possibleCompressors[j]->getColumnSize(0);
							cnt[i+1]-=possibleCompressors[j]->getColumnSize(1);
							didCompress = true;
							usedCompressors[j]=true;
						}
					}
					else
					{
						if(cnt[i] >= possibleCompressors[j]->getColumnSize(0))
						{
							REPORT(DEBUG,endl);
							REPORT(DEBUG,"Using Compressor " << j <<" to reduce column " << i);
							elemReduce(i, possibleCompressors[j]);
							cnt[i]-=possibleCompressors[j]->getColumnSize(0);
							didCompress = true;
							usedCompressors[j]=true;
						}
					}
				}
				
				i++;
			}
			
			i = minWeight;
			j++;
		}
		
		//update the next three (actually, the log of the size of the largest compressor) columns
		for(int i=minWeight; i<maxWeight; i++)
		{
			cnt[i]=0;
			for(list<WeightedBit*>::iterator it = bits[i].begin(); it!=bits[i].end(); it++)
			{
				if((*it)->computeStage(stagesPerCycle, elementaryTime) <= stage)
				{
					cnt[i]++;
				}
			}
		}
				
	}


	void BitHeap::generateVHDLforDSP(MultiplierBlock* m, int uid, int i)
	{
		stringstream s;
		string input1=m->getInputName1();
		string input2=m->getInputName2();
		int topX=m->gettopX();
		int topY=m->gettopY();
		int botX=topX+m->getwX()-1;
		int botY=topY+m->getwY()-1;
		int zerosX=0;
		int zerosY=0;

		//REPORT(INFO,"LENGHTS"<<m->getwX()<<" "<<m->getwY());

		if((signedIO) && (op->getTarget()->getVendor()=="Xilinx"))
		{
			if ((op->getTarget()->getID()=="Virtex4") || (op->getTarget()->getID()=="Spartan3"))
			{
				zerosX = 18 - m->getwX();
				zerosY = 18 - m->getwY();
			}
			else
			{
				if(m->getwX()>m->getwY())
				{
					zerosX=25-m->getwX();	
					zerosY=18-m->getwY();
				}
				else
				{
					zerosX=18-m->getwX();	
					zerosY=25-m->getwY();
				}
			}
		}

		if((signedIO) && (op->getTarget()->getVendor()=="Altera"))
		{
			if((m->getwX()!=9)&&(m->getwX()!=12)&&(m->getwX()!=18)&&(m->getwX()!=36))
				zerosX=1;
			if((m->getwY()!=9)&&(m->getwY()!=12)&&(m->getwY()!=18)&&(m->getwY()!=36))
				zerosY=1;
		}

		if(zerosX<0)
			zerosX=0;
		if(zerosY<0)
			zerosY=0;

		//if the coordinates are negative, then the signals must be completed with 0-s at the end, for the good result.
		//addx, addy represents the number of 0-s to be added

		int addx=0;
		int addy=0;

		if(topX<0)
			addx=0-topX;

		if(topY<0)
			addy=0-topY;
			
		op->vhdl << tab << op->declare(join("DSP_bh", guid, (uid==0 ? "_ch" : "_root"), i, "_", uid), m->getwX()+m->getwY()+zerosX+zerosY)
			     << " <= (" << zg(zerosX) << " & " << input1 << range(botX,topX+addx) << " & " << zg(addx) <<")" 
			     << " * " 
			     << "(" << zg(zerosY) << " & " << input2 << range(botY,topY+addy) << " & " <<zg(addy) << ");" << endl;

		s << join("DSP_bh" , guid, (uid==0 ? "_ch" : "_root"), i, "_", uid);

		REPORT(DETAILED,"comuted in this moment= "<< join("DSP_bh", guid, (uid==0 ? "_ch" : "_root"), i, "_", uid)
		       << "length= " << m->getwX()+m->getwY() << " <= ("
		       << input1 << range(botX,topX+addx) << " & " << zg(addx) << ") * ("
		       << input2 << range(botY,topY+addy) << " & " << zg(addy)<<");");

		m->setSignalName(s.str());
		m->setSignalLength(m->getwX()+m->getwY()+zerosX+zerosY);
	}





	void BitHeap::concatenateLSBColumns()
	{
		unsigned w;
		REPORT(DEBUG, "Checking whether rightmost columns need compressing");
		bool alreadyCompressed=true;
		w=minWeight;
		while(w<bits.size() && alreadyCompressed) {
			if(currentHeight(w)>1)
				alreadyCompressed=false;
			else	{// currentHeight(w) <= 1
				if(currentHeight(w) == 0)	{
					THROWERROR("LSB columns cannot be void!");
					w++;
				}
				else { // currentHeight(w) == 1
					REPORT(DEBUG, "Level " << w << " is already compressed; will go directly to the final result");
					w++;
				}
			}
		}



		if(w!=minWeight)
			{
				
				WeightedBit *b = getLatestBit(minWeight, w-1);
				op->setCycle(  b ->getCycle()  );
				op->setCriticalPath(   b ->getCriticalPath(op->getCurrentCycle()));



				if (w-minWeight>1)
					op->vhdl << tab << op->declare(join("tempR_bh", guid, "_", chunkDoneIndex), w-minWeight, true) << " <= " ;
				else
					op->vhdl << tab << op->declare(join("tempR_bh", guid, "_", chunkDoneIndex), 1, false) << " <= " ;
				unsigned i=w-1;
				while((i>=minWeight)&&(i<w))
					{

						if(currentHeight(i)==1)
							{
								op->vhdl << (bits[i].front())->getName();
								bits[i].pop_front();
							}
						else
							{
								op->vhdl << "'0'";
							}
						if (i!=minWeight)
							op->vhdl << " & ";
						i--;
					}
				op->vhdl << "; -- already compressed" << endl;
				chunkDoneIndex++;
			}



		minWeight=w;

		REPORT(DEBUG, "minWeight="<< minWeight);

	}

	void BitHeap::printBitHeapStatus()
	{
		REPORT(DEBUG, "Columns:");
		for (unsigned w=0; w<bits.size(); w++)
			{
				REPORT(DEBUG, "   w=" << w << ":\t height=" << bits[w].size());
				printColumnInfo(w);
			}

	}


}

