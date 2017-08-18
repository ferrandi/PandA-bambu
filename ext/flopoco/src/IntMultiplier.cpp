/*
  An integer multiplier mess for FloPoCo

  TODO in the virtual multiplier case, manage properly the case when the initial instant is not  0

  Authors:  Bogdan Pasca, and then F de Dinechin, Matei Istoan, Kinga Illyes and Bogdan Popa spent 12 cumulated months getting rid of the last bits of his code

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,
  2008-2012.
  All rights reserved.
*/

// To enable SVG plotting, #define BITHEAP_GENERATE_SVG in BitHeap.hpp


// TODO 
// Again needs a complete overhaul I'm afraid
// - Improve the case when a multiplier fits one DSP (very common)
//     In this case even for truncated mults we should have simple truncation, g=0 (also in neededGuardBits),
//     and a special management of negate and signedIO


/* 
   Who calls whom
   the constructor calls buildLogicOnly or buildTiling
   (maybe these should be unified some day)
   They call buildXilinxTiling or buildAlteraTiling or buildHeapLogicOnly

*/



/* VHDL variable names:
   X, Y: inputs
   XX,YY: after swap

*/






/* For two's complement arithmetic on n bits, the representable interval is [ -2^{n-1}, 2^{n-1}-1 ]
   so the product lives in the interval [-2^{n-1}*2^{n-1}-1,  2^n]
   The value 2^n can only be obtained as the product of the two minimal negative input values
   (the weird ones, which have no opposite)
   Example on 3 bits: input interval [-4, 3], output interval [-12, 16] and 16 can only be obtained by -4*-4.
   So the output would be representable on 2n-1 bits in two's complement, if it werent for this weird*weird case.

   So even for signed multipliers, we just keep the 2n bits, including one bit used for only for this weird*weird case.
   Current situation is: if you don't like this you manage it from outside:
   An application that knows that it will not use the full range (e.g. signal processing, poly evaluation) can ignore the MSB bit, 
   but we produce it.

*/




#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "utils.hpp"
#include "Operator.hpp"
#include "IntMultiplier.hpp"
#include "IntAdder.hpp"
#include "Targets/StratixII.hpp"
#include "Targets/StratixIII.hpp"
#include "Targets/StratixIV.hpp"
#include "Plotter.hpp"

using namespace std;

namespace flopoco {

#define vhdl parentOp->vhdl
#define declare parentOp->declare
#define inPortMap parentOp->inPortMap
#define outPortMap parentOp->outPortMap
#define instance parentOp->instance
#define useSoftRAM parentOp->useSoftRAM
#define manageCriticalPath parentOp->manageCriticalPath
#define getCriticalPath parentOp->getCriticalPath
#define setCycle parentOp->setCycle
#define oplist parentOp->getOpListR()


	int IntMultiplier::neededGuardBits(int wX, int wY, int wOut)
	{
		int g;
		if(wX+wY==wOut)
			g=0;
		else {
			unsigned i=0;
			mpz_class ulperror=1;
			while(wX+wY - wOut  > intlog2(ulperror)) {
				// REPORT(DEBUG,"i = "<<i<<"  ulperror "<<ulperror<<"  log:"<< intlog2(ulperror) << "  wOut= "<<wOut<< "  wFull= "<< wX+wY);
				i++;
				ulperror += (i+1)*(mpz_class(1)<<i);
			}
			g=wX+wY-i-wOut;
			// REPORT(DEBUG, "ulp truncation error=" << ulperror << "    g=" << g);
		}
		return g;
	}


	void IntMultiplier::initialize() 
	{
		if(wOut<0 || wXdecl<0 || wYdecl<0) {
			THROWERROR("negative input/output size");
		}

		wFull = wXdecl+wYdecl;

		if(wOut > wFull){
			THROWERROR("wOut=" << wOut << " too large for " << (signedIO?"signed":"unsigned") << " " << wXdecl << "x" << wYdecl <<  " multiplier." );
		}

		if(wOut==0){ 
			wOut=wFull;
		}

		if(wOut<min(wXdecl, wYdecl))
			REPORT(0, "wOut<min(wX, wY): it would probably be more economical to truncate the inputs first, instead of building this multiplier.");

		wTruncated = wFull - wOut;

		g = neededGuardBits(wXdecl, wYdecl, wOut);
		REPORT(DEBUG, "    g=" << g);

		weightShift = wFull - (wOut+g); 
		REPORT(DEBUG,   "weightShift=" << weightShift);


		// Halve number of cases by making sure wY<=wX:
		// interchange x and y in case wY>wX
		// After which we negate y (the smaller) by 1/ complementing it and 2/  adding it back to the bit heap

		string newxname, newyname;
		if(wYdecl> wXdecl) {
			wX=wYdecl;	 
			wY=wXdecl;	 
			newxname=yname;
			newyname=xname;
		}
		else {
			wX=wXdecl;	 
			wY=wYdecl;
			newxname=xname;
			newyname=yname;
		}

		// The larger of the two 
		vhdl << tab << declare(addUID("XX"), wX, true) << " <= " << newxname << " ;" << endl;	 

		// possibly negate the smaller
		if(!negate)
			vhdl << tab << declare(addUID("YY"), wY, true) << " <= " << newyname << " ;" << endl;	 
		else
		{
			vhdl << tab << "-- we compute -xy as x(not(y)+1)" << endl;
			vhdl << tab << declare(addUID("YY"), wY, true) << " <= not " << newyname << " ;" << endl;	 
		}
	}





	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// The virtual constructor 
	IntMultiplier::IntMultiplier (Operator* parentOp_, BitHeap* bitHeap_, Signal* x_, Signal* y_, int wX_, 
	                              int wY_, int wOut_, int lsbWeight_, bool negate_, bool signedIO_, float DSPThreshold_):
		Operator (  parentOp_->getTarget()), 
		wxDSP(0), wyDSP(0), wXdecl(wX_), wYdecl(wY_), wX(0), wY(0), wOut(wOut_), DSPThreshold(DSPThreshold_),  maxError(0.0), 
		parentOp(parentOp_), bitHeap(bitHeap_), lsbWeight(lsbWeight_),
		x(x_), y(y_), negate(negate_), signedIO(signedIO_) 
	{

		multiplierUid=parentOp->getNewUId();
		srcFileName="IntMultiplier";
		useDSP = (DSPThreshold>=0) &&  parentOp->getTarget()->useHardMultipliers() && parentOp->getTarget()->useHardMultipliers();

		ostringstream name;
		name <<"VirtualIntMultiplier";
		if(useDSP) 
			name << "UsingDSP_";
		else
			name << "LogicOnly_";
		name << wXdecl << "_" << wYdecl <<"_" << wOut << "_" << (signedIO?"signed":"unsigned") << "_uid"<<Operator::getNewUId();
		setName ( name.str() );
		
		REPORT(DEBUG, "Building " << name.str() );

		xname = x->getName();
		yname = y->getName();

		// // we create a separate Plotter for this mult. Is it useful?
		// plotter = new Plotter(bitHeap);
		//plotter->setBitHeap(bitHeap);

		bitHeap->setSignedIO(signedIO);
		

		initialize();

		fillBitHeap();

		// leave the compression to the parent op
	}







	// The constructor for a stand-alone operator
	IntMultiplier::IntMultiplier (Target* target_, int wX_, int wY_, int wOut_, bool signedIO_, float DSPThreshold_, map<string, double> inputDelays_, bool enableSuperTiles_):
		Operator ( target_, inputDelays_ ), wxDSP(0), wyDSP(0), wXdecl(wX_), wYdecl(wY_), wX(0), wY(0), wOut(wOut_), wFull(0), DSPThreshold(DSPThreshold_),  maxError(0.0), negate(false), signedIO(signedIO_),enableSuperTiles(enableSuperTiles_), target(target_)
	{
		srcFileName="IntMultiplier";
		setCopyrightString ( "Florent de Dinechin, Kinga Illyes, Bogdan Popa, Bogdan Pasca, 2012" );

		// useDSP or not? 
		//useDSP = (DSPThreshold>0) && target->useHardMultipliers();
		useDSP = (DSPThreshold>=0) && target->useHardMultipliers();

		{
			ostringstream name;
			name <<"IntMultiplier";
			if(useDSP) 
				name << "_UsingDSP_";
			else
				name << "_LogicOnly_";
			name << wXdecl << "_" << wYdecl <<"_" << wOut << "_" << (signedIO?"signed":"unsigned") << "_uid"<<Operator::getNewUId();
			setName ( name.str() );
			REPORT(DEBUG, "Building " << name.str() )
		}


		parentOp=this;
		multiplierUid=parentOp->getNewUId();
		xname="X";
		yname="Y";
		
		initialize();
		lsbWeight=g; // g was computed in initialize()

		// Set up the IO signals
		addInput ( xname  , wXdecl, true );
		addInput ( yname  , wYdecl, true );

		// TODO FIXME This 1 should be 2. It breaks TestBench but not TestBenchFile. Fix TestBench first! (check in addExpectedOutput or something)
		addOutput ( "R"  , wOut, 1 , true );

		// Set up the VHDL library style
		if(signedIO)
			useNumericStd_Signed();
		else
			useNumericStd_Unsigned();

		// The bit heap
		bitHeap = new BitHeap(this, wOut+g, enableSuperTiles);

		// TODO CHECK ??? A bit heap is sign-agnostic.
		bitHeap->setSignedIO(signedIO);



		// initialize the critical path
		setCriticalPath(getMaxInputDelays ( inputDelays_ ));

		fillBitHeap();
		
		bitHeap -> generateCompressorVHDL();			
		vhdl << tab << "R" << " <= " << bitHeap-> getSumName() << range(wOut+g-1, g) << ";" << endl;
	}






	void  IntMultiplier::fillBitHeap()
	{

		Plotter* plotter= bitHeap->getPlotter();
		///////////////////////////////////////
		//  architectures for corner cases   //
		///////////////////////////////////////

		// TODO Support negate in all the corner cases
		// To manage stand-alone cases, we just build a bit-heap of max height one, so the compression will do nothing

		// The really small ones fit in two LUTs and that's as small as it gets  
		if(wX+wY <=  parentOp->getTarget()->lutInputs()+2)
		{
			vhdl << tab << "-- Ne pouvant me fier a mon raisonnement, j'ai appris par coeur le résultat de toutes les multiplications possibles" << endl;

			SmallMultTable *t = new SmallMultTable(  parentOp->getTarget(), wX, wY, wOut, negate, signedIO, signedIO);
			t->addToGlobalOpList();

			vhdl << tab << declare(addUID("XY"), wX+wY) << " <= "<<addUID("YY")<<" & "<<addUID("XX")<<";"<<endl;

			inPortMap(t, "X", addUID("XY"));
			outPortMap(t, "Y", addUID("RR"));
			vhdl << instance(t, "multTable");
			useSoftRAM(t);

			plotter->addSmallMult(0,0,wX,wY);
			plotter->plotMultiplierConfiguration(getName(), localSplitVector, wX, wY, wOut, g);

			for(int w=wOut-1; w>=0; w--)	{ // this is a weight in the multiplier output
				stringstream s;
				s<<addUID("RR")<<of(w);
				bitHeap->addBit(lsbWeight + w, s.str()); 
			}		
			return;
		}

		// Multiplication by 1-bit integer is simple
		if ((wY == 1))
		{
			vhdl << tab << "-- How to obfuscate multiplication by 1 bit: first generate a trivial bit vector" << endl;
			if (signedIO){
				manageCriticalPath(  parentOp->getTarget()->localWireDelay(wX) +  parentOp->getTarget()->adderDelay(wX+1) );
				
				vhdl << tab << declare(addUID("RR"), wX+wY)  << " <= (" << zg(wX+1)  << " - ("<<addUID("XX")<< of(wX-1) 
				     << " & "<<addUID("XX")<<")) when "<<addUID("YY")<<"(0)='1' else "<< zg(wX+1,0)<<";"<<endl;	
			}
			else {
				manageCriticalPath(  parentOp->getTarget()->localWireDelay(wX) +  parentOp->getTarget()->lutDelay() );
				vhdl << tab  << declare(addUID("RR"), wX+wY) << " <= (\"0\" & "<<addUID("XX")<<") when "<< addUID("YY")<<"(0)='1' else "<<zg(wX+1,0)<<";"<<endl;	
			}
			vhdl << tab << "-- then send its relevant bits to a useless bit heap " << endl;
			for(int w=wOut-1; w>=0; w--)	{ // this is a weight in the multiplier output
				stringstream s;
				s<<addUID("RR")<<of(w);
				bitHeap->addBit(lsbWeight-g + w, s.str()); 
			}

			vhdl << tab << "-- then compress this height-1 bit heap by doing nothing" << endl;
			outDelayMap[addUID("R")] = getCriticalPath();
			return;
		}

		// Multiplication by 2-bit integer is one addition, which is delegated to BitHeap compression anyway
		if ((wY == 2))
		{
			vhdl << tab << declare(addUID("R0"),wX+2) << " <= (";
			if (signedIO) 
				vhdl << addUID("XX") << of(wX-1) << " & "<< addUID("XX") << of(wX-1);  
			else  
				vhdl << "\"00\"";
			vhdl <<  " & "<<addUID("XX")<<") when "<<addUID("YY")<<"(0)='1' else "<<zg(wX+2,0)<<";"<<endl;	

			vhdl << tab << declare(addUID("R1i"),wX+2) << " <= ";
			if (signedIO) 
				vhdl << "("<<addUID("XX")<< of(wX-1) << "  &  " <<addUID("XX")<<" & \"0\")";
			else  
				vhdl << "(\"0\" & "<< addUID("XX") <<" & \"0\")";
			vhdl << " when "<<addUID("YY")<<"(1)='1' else " << zg(wX+2,0) << ";"<<endl;	

			vhdl << tab << declare(addUID("R1"),wX+2) << " <= ";
			if (signedIO) 
				vhdl << "not "<<addUID("R1i")<<";" <<endl;
			else  
				vhdl << addUID("R1i")<<";"<<endl;
			
			for (int w=0; w<wOut+g; w++){
				stringstream s0,s1;
				s0<<addUID("R0")<<of(w+(wX+2-wOut-g));
				bitHeap->addBit(lsbWeight-g + w, s0.str());
				s1<<addUID("R1")<<of(w+(wX+2-wOut-g));
				bitHeap->addBit(lsbWeight-g + w, s1.str());
			}
			// Rounding bit (or carry in bit for signed inputs)
			if(g || signedIO)
				bitHeap->addConstantOneBit(lsbWeight-g);
			// and that's it
			return;
		}
		
		// Multiplications that fit directly into a DSP
		int dspXSize, dspYSize;
		
		//		if(parentOp != NULL) Test commented out by Florent because the following parentOp->getTarget was not protected the same way
		parentOp->getTarget()->getDSPWidths(dspXSize, dspYSize, signedIO);
		//		else
		//getTarget()->getDSPWidths(dspXSize, dspYSize, signedIO);
			
		//correct the DSP sizes for Altera targets
		if(parentOp->getTarget()->getVendor() == "Altera")
		{
			if(dspXSize >= 18)
			{
				if(signedIO)
					dspXSize = 18;
				else
					dspXSize = 17;
			}
			if(dspYSize >= 18)
			{
				if(signedIO)
					dspYSize = 18;
				else
					dspYSize = 17;
			}
		}
		
		// if we are using at least SMALL_MULT_RATIO of the DSP, then just implement 
		//	the multiplication in a DSP, without passing through a bitheap
		//	the last three conditions ensure that the multiplier can actually fit in a DSP
		//if((1.0*wX*wY >= 1.0*SMALL_MULT_RATIO*dspXSize*dspYSize) && (1.0*wX*wY < 1.0*dspXSize*dspYSize) && (wX <= dspXSize) && (wY <= dspYSize))
		if(useDSP && worthUsingOneDSP(wX, wY, 0, 0, dspXSize, dspYSize) && (wX <= dspXSize) && (wY <= dspYSize))
		{
			ostringstream s, zerosXString, zerosYString, zerosYNegString;
			int zerosX = dspXSize - wX + (signedIO ? 0 : 1);
			int zerosY = dspYSize - wY + (signedIO ? 0 : 1);
			int startingIndex, endingIndex;

			if(zerosX<0)
				zerosX=0;
			if(zerosY<0)
				zerosY=0;
				
			//sign extension (or zero-extension, if working with unsigned numbers)
			if(signedIO)
			{
				//sign extension
				zerosXString << "(";
				for(int i=0; i<zerosX; i++)
						zerosXString << addUID("XX") << of(wX-1) << (i!=(zerosX-1) ? " & " : ")");
				zerosYString << "(";
				zerosYNegString << "(";
				for(int i=0; i<zerosY; i++)
				{
					zerosYString << addUID("YY") << of(wY-1) << (i!=(zerosY-1) ? " & " : ")");
					//zerosYNegString << addUID("YY") << "_neg" << of(wY-1) << (i!=(zerosY-1) ? " & " : ")");
					zerosYNegString << addUID("YY") << of(wY-1) << (i!=(zerosY-1) ? " & " : ")");
				}
			}
			else // ! signedIO
			{
				//zero extension
				zerosXString << "(" << zg(zerosX) << ")";
				zerosYString << "(" << zg(zerosY) << ")";
				zerosYNegString << "(" << zg(zerosY) << ")";
			}
				
			//if negated, the product becomes -xy = not(y)*x + x
			//	TODO: this should be more efficient than negating the product at
			//	the end, as it should be implemented in a single DSP, both the multiplication and the addition
			if(negate)
				vhdl << tab << declare(join(addUID("YY"), "_neg"), wY+zerosY) << " <= " << (zerosY>0 ? join(zerosYNegString.str(), " & ") : "") << addUID("YY") << ";" << endl;

			//manage the pipeline
			manageCriticalPath(parentOp->getTarget()->DSPMultiplierDelay());

			vhdl << tab << declare(join("DSP_mult_", getuid()), dspXSize+dspYSize+(signedIO ? 0 : 2))
						 << " <= (" << (zerosX>0 ? join(zerosXString.str(), " & ") : "") << addUID("XX") << ")"
						 << " *";
			if(negate)
				vhdl	 <<	" (" << addUID("YY") << "_neg);" << endl;
			else
				vhdl	 << " (" << (zerosY>0 ? join(zerosYString.str(), " & ") : "") << addUID("YY") << ");" << endl;
			
			s << "DSP_mult_" << getuid();
			
			//manage the pipeline
			//syncCycleFromSignal(s.str());
			
			//add the bits of x*(not y) (respectively x*y, when not negated)
			endingIndex	  = wX+wY;
			startingIndex = 0+(wX+wY-wOut-g);
			for(int w=startingIndex; w<endingIndex; w++)
				if(w-startingIndex >= 0)
				{
					//no point of sign-extending for just one bit
					if((wOut < (int)(bitHeap->getMaxWeight()-bitHeap->getMinWeight())) && (w == (endingIndex-1)) && (bitHeap->getMaxWeight()-(endingIndex-1-startingIndex) > 1))
					{
						s.str("");
						s << "not DSP_mult_" << getuid();
						
						bitHeap->addBit(w-startingIndex, join(s.str(), of(w)));
					}
					else
						bitHeap->addBit(w-startingIndex, join(s.str(), of(w)));
				}
						
			//keep sign-extending, if necessary
			if(bitHeap->getMaxWeight()-(endingIndex-1-startingIndex) > 1)
				for(int w=endingIndex-1-startingIndex; w<(int)bitHeap->getMaxWeight(); w++)
					bitHeap->addConstantOneBit(w);
			
			//add the bits of x, (not x)<<2^wY, 2^wY
			if(negate)
			{
				//add x
				endingIndex	  = wX;
				startingIndex = 0+(wX+wY-wOut-g);
				for(int w=startingIndex; w<endingIndex; w++)
					if(w-startingIndex >= 0)
						bitHeap->addBit(w-startingIndex, join(addUID("XX"), of(w)));
					
				//x, when added, should be sign-extended
				if(signedIO)
					for(int w=endingIndex-startingIndex; w<(int)bitHeap->getMaxWeight(); w++)
						if(w-startingIndex >= 0)
							bitHeap->addBit(w, join(addUID("XX"), of(wX-1)));
				
					
				if(!signedIO)
				{
					//add (not x)<<2^(wY+1)
					endingIndex	  = wX+wY-(wX+wY-wOut-g);
					startingIndex = wY-(wX+wY-wOut-g);
					for(int w=startingIndex; w<endingIndex; w++)
					{
						ostringstream s;
						
						s << "not(" << addUID("XX") << of(w-startingIndex) << ")";
						
						if((w >= 0) && (w < wOut+g))
							bitHeap->addBit(w, s.str());
					}
						
					//add 2^(wY+1)
					if(wY+1-(wX+wY-wOut-g) >= 0)
						bitHeap->addConstantOneBit(wY+1-(wX+wY-wOut-g));
				}
			}
			
			// this should be it
			return;
		}

		// Now getting more and more generic

#if 1
		// Finish the negation of the smaller input by adding X (since -yx=not(y)x +x)

		//		setCycle(0); // TODO F2D FIXME for the virtual multiplier case where inputs can arrive later
		// setCriticalPath(initialCP);

		if(negate)
		{
			vhdl << tab << "-- Finish the negation of the product (-xy as x((111..111-y)+111..111)) by adding XX + 2^wY.not XX +  2^wY" << endl;

			// Adding XX
			for(int i=0; i<wX; i++)
			{
				int w = lsbWeight + i-weightShift;
				if(w>=0)
				{
					ostringstream rhs;
					if(signedIO && i==wX-1){
						rhs << addUID("not XX") << of(i);
						// sign extension
						for(unsigned int j=i; j<bitHeap->getMaxWeight(); j++)
							bitHeap->addConstantOneBit(j); 
					}
					else
						rhs << addUID("XX") << of(i);
					bitHeap->addBit(w, rhs.str());
				}
			}

			// Adding 2^(wY+1) not XX
			for(int i=0; i<wX; i++)
			{
				int w = wY + lsbWeight + i-weightShift;
				if(w>=0 && w<(int)bitHeap->getMaxWeight())
				{
					ostringstream rhs;
					rhs << "not " << addUID("XX") << of(i);
					bitHeap->addBit(w, rhs.str());
				}
			}
			int w = wY + lsbWeight -weightShift;
			if(w>=0 && w<(int)bitHeap->getMaxWeight())
				bitHeap->addConstantOneBit(wY + lsbWeight -weightShift); 

		}
#endif

		if(useDSP) 
		{
			REPORT(DETAILED,"useDSP");
			parentOp->getTarget()->getDSPWidths(wxDSP, wyDSP, signedIO);
			buildTiling();
		}
		else 
		{
			// This target has no DSP, going for a logic-only implementation	
			buildLogicOnly();
		}



		// The following turns truncation into rounding, except that the overhead is large for small multipliers.
		if(g>0)
		{
			int weight = lsbWeight-1;
			if(negate)
				bitHeap->subConstantOneBit(weight);
			else
				bitHeap->addConstantOneBit(weight);
		}



#if GENERATE_SVG
		plotter->plotMultiplierConfiguration(getName(), localSplitVector, wX, wY, wOut, g);
#endif
	}
	



	/**************************************************************************/
	void IntMultiplier::buildLogicOnly() 
	{
		buildHeapLogicOnly(0,0,wX,wY);
	}



	/**************************************************************************/
	void IntMultiplier::buildTiling() 
	{
		int* multiplierWidth;
		int size;	
		
		if( parentOp->getTarget()->getVendor() == "Altera")
		{
			if ( parentOp->getTarget()->getID()=="StratixII")
			{
				StratixII* t = (StratixII*) parentOp->getTarget();
				multiplierWidth = t->getDSPMultiplierWidths();
				size = t->getNrDSPMultiplier();	

			}else if( parentOp->getTarget()->getID()=="StratixIII")
			{
				StratixIII* t = (StratixIII*) parentOp->getTarget();
				multiplierWidth = t->getDSPMultiplierWidths();
				size = t->getNrDSPMultiplier();	

			}else if( parentOp->getTarget()->getID()=="StratixIV")
			{
				StratixIV* t = (StratixIV*) parentOp->getTarget();
				multiplierWidth = t->getDSPMultiplierWidths();
				size = t->getNrDSPMultiplier();	

			}else //add Altera boards here
			{
				StratixII* t = (StratixII*) parentOp->getTarget();
				multiplierWidth = t->getDSPMultiplierWidths();
				size = t->getNrDSPMultiplier();
			}

			for(int i=0; i<size; i++)
				multWidths.push_back(multiplierWidth[i]);

			buildAlteraTiling(0, 0, wX, wY, 0, signedIO, signedIO);
		}else
		{
			// Xilinx here
			if((!signedIO) && ((wX==41) && (wY==41) && (wFull-wOut-g==0)))
				buildFancy41x41Tiling();
			else
				buildXilinxTiling();
		}
	}


	//the fancy tiling is used only for a hardwired case 41 41 82 
	/***********************************************************************/
	void IntMultiplier::buildFancy41x41Tiling()
	{
		//THROWERROR("fancy tiling not implemented yet");

		stringstream inx,iny;
		inx<<addUID("XX");
		iny<<addUID("YY");


		int widthX, widthY,topx,topy;

		//topright dsp;
		widthX=wxDSP;
		widthY=wyDSP;
		topx=0;
		topy=0;
		MultiplierBlock* m = new MultiplierBlock(widthX,widthY,topx,topy,inx.str(),iny.str(), lsbWeight);
		m->setNext(NULL);		
		m->setPrevious(NULL);			
		localSplitVector.push_back(m);
		bitHeap->addMultiplierBlock(m);


		//topleft dsp
		widthX=wyDSP;
		widthY=wxDSP;
		topx=wxDSP;
		topy=0;
		m = new MultiplierBlock(widthX,widthY,topx,topy,inx.str(),iny.str(), lsbWeight);
		m->setNext(NULL);		
		m->setPrevious(NULL);			
		localSplitVector.push_back(m);
		bitHeap->addMultiplierBlock(m);

		//bottomleft dsp
		widthX=wxDSP;
		widthY=wyDSP;
		topx=wyDSP;
		topy=wxDSP;
		m = new MultiplierBlock(widthX,widthY,topx,topy,inx.str(),iny.str(), lsbWeight);
		m->setNext(NULL);		
		m->setPrevious(NULL);			
		localSplitVector.push_back(m);
		bitHeap->addMultiplierBlock(m);

		//bottomright dsp
		widthX=wyDSP;
		widthY=wxDSP;
		topx=0;
		topy=wyDSP;
		m = new MultiplierBlock(widthX,widthY,topx,topy,inx.str(),iny.str(), lsbWeight);
		m->setNext(NULL);		
		m->setPrevious(NULL);			
		localSplitVector.push_back(m);
		bitHeap->addMultiplierBlock(m);

		//logic

		buildHeapLogicOnly(wyDSP,wyDSP,wxDSP,wxDSP,parentOp->getNewUId());

	}


	/**************************************************************************/
	void IntMultiplier::buildHeapLogicOnly(int topX, int topY, int botX, int botY,int blockUid)
	{
		REPORT(DETAILED,"buildheaplogiconly called for "<<topX<<" "<<topY<<" "<<botX<<" "<<botY);
		Target *target= parentOp->getTarget();
		
		if(blockUid == -1)
			blockUid++;    /// ???????????????

		vhdl << tab << "-- code generated by IntMultiplier::buildHeapLogicOnly()"<< endl;
		vhdl << tab << "-- buildheaplogiconly called for topX=" << topX << " topY=" << topY << " botX="<< botX << " botY="<< botY << endl;

		int dx, dy;				// Here we need to split in small sub-multiplications
		int li = target->lutInputs();

		dx = li >> 1;
		dy = li - dx; 
		REPORT(DEBUG, "dx="<< dx << "  dy=" << dy );

		int wXX=wX;
		int wYY=wY;

		int wX = botX-topX;
		int wY = botY-topY;
		int chunksX =  int(ceil( ( double(wX) / (double) dx) ));
		int chunksY =  int(ceil( ( 1+double(wY-dy) / (double) dy) ));
		int sizeXPadded=dx*chunksX; 
		int sizeYPadded=dy*chunksY;
		REPORT(DEBUG, "sizeXpadded"<<sizeXPadded);	

		int padX=sizeXPadded-wX;
		int padY=sizeYPadded-wY;

		REPORT(DEBUG, "X split in "<< chunksX << " chunks and Y in " << chunksY << " chunks; ");
		REPORT(DEBUG, " sizeXPadded="<< sizeXPadded << "  sizeYPadded="<< sizeYPadded);
		
		//we do more than 1 subproduct 
		// FIXME where is the else?
		if (chunksX + chunksY >= 2) 
		{
			// Padding X to the right
			vhdl << tab << declare(addUID("Xp", blockUid), sizeXPadded) << " <= "
					<< addUID("XX") << range(botX-1,topX) << " & " <<zg(padX) << ";" << endl;
			REPORT(DETAILED,addUID("XX") << range(botX-1,topX) << " & "<<zg(padX)<<";");

			// Padding Y to the right
			vhdl << tab << declare(addUID("Yp",blockUid), sizeYPadded)<<" <= "
					<< addUID("YY") << range(botY-1,topY) << " & "<< zg(padY) << ";" << endl;
			REPORT(DETAILED,addUID("YY") << range(botY-1,topY) << " & "<<zg(padY)<<";");
			
			//SPLITTING
			for (int k=0; k<chunksX ; k++)
			{
				vhdl << tab << declare(join(addUID("x",blockUid),"_",k),dx) << " <= "<< addUID("Xp",blockUid) << range((k+1)*dx-1,k*dx)<<";"<<endl;
				REPORT(DETAILED,join(addUID("x",blockUid),"_",k)<<" <= "<< addUID("Xp",blockUid) << range((k+1)*dx-1,k*dx)<<";");
			}	
			for (int k=0; k<chunksY ; k++)
			{
				vhdl << tab << declare(join(addUID("y",blockUid),"_",k),dy) << " <= " << addUID("Yp",blockUid) << range((k+1)*dy-1, k*dy)<<";"<<endl;
				REPORT(DETAILED,join(addUID("y",blockUid),"_",k)<<" <= "<< addUID("Yp",blockUid) << range((k+1)*dy-1,k*dy)<<";");
			}	

			SmallMultTable *tUU, *tSU, *tUS, *tSS;

			// In the negate case we will negate the bits coming out of this table
			tUU = new SmallMultTable( target, dx, dy, dx+dy, false /*negate*/, false /*signx*/, false/*signy*/);
			tUU->addToGlobalOpList();

			if(signedIO) 
			{ // need for 4 different tables

				tSU = new SmallMultTable( target, dx, dy, dx+dy, false, true, false );
				tSU->addToGlobalOpList();

				tUS = new SmallMultTable( target, dx, dy, dx+dy, false, false, true );
				tUS->addToGlobalOpList();


				tSS = new SmallMultTable( target, dx, dy, dx+dy, false, true, true );
				tSS->addToGlobalOpList();

			}

			setCycle(0); // TODO FIXME for the virtual multiplier case where inputs can arrive later
			setCriticalPath(initialCP);
			// SmallMultTable is built to cost this:
			manageCriticalPath(  parentOp->getTarget()->localWireDelay(chunksX) + parentOp->getTarget()->lutDelay() ) ;  
			for (int iy=0; iy<chunksY; iy++)
			{
				vhdl << tab << "-- Partial product row number " << iy << endl;

				for(int ix=0; ix<chunksX; ix++)
				{
					SmallMultTable *t;
					
					if(!signedIO) 
						t=tUU;
					else
					{
						// 4  cases 
						if( ((ix==chunksX-1)&&(botX==wXX)) && ((iy==chunksY-1)&&(botY==wYY) ))
							t=tSS;
						else if ((ix==chunksX-1)&&(botX==wXX)) 
							t=tSU;
						else if ((iy==chunksY-1)&&(botY==wYY))
							t=tUS;
						else
							t=tUU; 
					}

					//smallMultTable needed only if it is on the left of the truncation line
					if(dx*(ix+1)+dy*(iy+1)+topX+topY-padX-padY > wFull-wOut-g)
					{
						bitHeap->getPlotter()->addSmallMult(dx*(ix)+topX-padX, dy*(iy)+topY-padY,dx,dy);

						REPORT(DETAILED,XY(ix,iy,blockUid)<<" <= " << addUID("y",blockUid) <<"_"<< iy << " & " << addUID("x",blockUid) <<"_"<< ix << ";");
						
						vhdl << tab << declare(XY(ix,iy,blockUid), dx+dy) 
						     << " <= " << addUID("y",blockUid) <<"_"<< iy << " & " << addUID("x",blockUid) <<"_"<< ix << ";"<<endl;

						inPortMap(t, "X", XY(ix,iy,blockUid));
						outPortMap(t, "Y", PP(ix,iy,blockUid));
						vhdl << instance(t, PPTbl(ix,iy,blockUid));
						useSoftRAM(t);

						vhdl << tab << "-- Adding the relevant bits to the heap of bits" << endl;

						// Two's complement management
						// There are really 2 cases:
						// If the result is known positive, ie if tUU and !negate, nothing to do
						// If the result is in two's complement  
						//    sign extend by adding ones on weights >= the MSB of the table, so its sign is propagated.
						//    Also need to complement the sign bit


						// The following comments are obsolete since we negate X at the beginning of the operator:
						// Note that even when negate and tUU, the result is known negative, but it may be zero, so its actual sign is not known statically
						// Note also that in this case (negate and tUU), the result overflows the dx+dy two's complement format.
						// This is why tUU is never negated above, and we negate it below. A bit messy, but probably the most resource-efficient


						bool resultSigned = false;
						
						if((t==tSS) || (t==tUS) || (t==tSU)) 
							resultSigned = true ;

						int maxK=t->wOut; // or, dx+dy + (t==tUU && negate?1:0); 
						int minK=0;
						//if(ix == chunksX-1)
						if ((ix == 0))
							minK+=padX;
						if((iy == 0))
							//maxK-=padY;
							minK+=padY;
							
						REPORT(DEBUG,"The bits will be added from mink="<<minK<<"	to maxk="<<maxK);
						REPORT(DEBUG,  "ix=" << ix << " iy=" << iy << "  maxK=" << maxK  << "  negate=" << negate <<  "  resultSigned="  << resultSigned );
						
						for (int k=minK; k<maxK; k++) {
							ostringstream s, nots;
							s << PP(ix,iy,blockUid) << of(k); // right hand side
							nots << "not " << s.str(); 

							int weight =  ix*dx+iy*dy+k  +topX+topY-padX-padY  - (wFull-wOut) + lsbWeight;

							if(weight>=0) 
							{
								// otherwise these bits are just truncated
								if(resultSigned && (k==maxK-1)) 
								{ 
									// This is a sign bit: sign-extend it by 1/ complementing and 2/ adding constant 1s all the way up to maxweight
									REPORT(DEBUG, "adding neg bit " << nots.str() << " at weight " << weight); 
									bitHeap->addBit(weight, nots.str());
									REPORT(DEBUG,  "  adding constant ones from weight "<< weight << " to "<< bitHeap->getMaxWeight());
									for (unsigned w=weight; w<bitHeap->getMaxWeight(); w++)
										bitHeap->addConstantOneBit(w);
								}
								else 
								{ 
									// just add the bit
									bitHeap->addBit(weight, s.str());
								}
							}
						}

						vhdl << endl;
					}
				}
			}				



		}

	}

	/** checks how many DSPs will be used in case of a tiling **/
	int IntMultiplier::checkTiling(int wxDSP, int wyDSP, int& horDSP, int& verDSP)
	{
		int widthOnX=wX;
		int widthOnY=wY;
		int horDS=0;
		int verDS=0;

		//**** how many dsps will be vertical*******************************/
		int hor=0;

		//if the multiplication is signed, the first DSP will have different size, will be bigger
		if( widthOnX>=wxDSP)
			{
				hor++;
				widthOnX-=wxDSP;
			}

		if(signedIO)	
			wxDSP--;

		//how many DSPs fits on the remaining part, without the first one
		horDS=int(ceil ( (double(widthOnX) / (double) wxDSP)))+hor;
		/***********************************************************************/


		//*** how many dsps will be horizontal**********************************/
		int ver=0;

		if( widthOnY>=wyDSP)
			{
				ver++;
				widthOnY-=wyDSP;
			}


		if(signedIO)	
			wyDSP--;

		verDS=int(ceil ( (double(widthOnY) / (double) wyDSP)))+ver;
		//***********************************************************************/

		horDSP=horDS;
		verDSP=verDS;

		return verDS*horDS;
	}


	/** 
	 * checks against the DSPThreshold the given block and adds a DSP or logic
	*/
	void IntMultiplier::addExtraDSPs(int topX, int topY, int botx, int boty,int wxDSP,int wyDSP)
	{
		REPORT(DEBUG, "in addExtraDSPs at sizeX=" << wxDSP << " and sizeY=" << wyDSP << ": topX=" << topX << " topY=" << topY << " botX=" << botx << " botY=" << boty);
		int topx=topX,topy=topY;
		
		//if the block is on the margins of the multipliers, then the coordinates have to be reduced.
		if(topX<0)
			topx=0;
		else
			topx=topX;	

		if(topY<0)
			topy=0;	
		else
			topy=topY;

		//if the truncation line splits the block, then the used block is smaller, the coordinates need to be updated
		if((botx+boty > wFull-wOut-g) && (topx+topy < wFull-wOut-g))
		{
			int x=topx;
			int y=boty;
			while((x+y<wFull-wOut-g)&&(x<botx))
			{
				x++;
				topx=x;
			}

			x=botx;
			y=topy;
			while((x+y<wFull-wOut-g)&&(y<boty))
			{
				y++;
				topy=y;	
			}	
		}

		//now check against the DSPThreshold
		if(worthUsingOneDSP(topx,topy,botx,boty,wxDSP,wyDSP))
		{
			//worth using DSP
			stringstream inx,iny;
			inx<<addUID("XX");
			iny<<addUID("YY");

			topx=botx-wxDSP;
			topy=boty-wyDSP;

			MultiplierBlock* m;
			m = new MultiplierBlock(wxDSP,wyDSP,topx,topy,inx.str(),iny.str(),weightShift);
			m->setNext(NULL);		
			m->setPrevious(NULL);			
			localSplitVector.push_back(m);
			bitHeap->addMultiplierBlock(m);
			REPORT(DEBUG, "in addExtraDSPs, adding a multiplier block of size wxDSP=" << wxDSP << " and wyDSP=" << wyDSP 
			       << ": topX=" << topx << " topY=" << topy << " weightShift=" << weightShift << " weight=" << m->getWeight());
			
		}
		else
		{
			//build logic	
			if((topx<botx)&&(topy<boty))
				buildHeapLogicOnly(topx,topy,botx,boty,parentOp->getNewUId());	
		}
	}



	/** 
	 * checks the area usage of 1 dsp according to a given block and ratio(threshold)
	 * 	ratio(threshold) = says what percentage of 1 DSP area is allowed to be lost
	 **/
	//FIXME: the unused area of the DSP is not necessarly an isosceles triangle
	//			in fact, it's not necessarly a triangle
	/*
	bool IntMultiplier::worthUsingOneDSP(int topX, int topY, int botX, int botY,int wxDSP,int wyDSP)
	{
	
		REPORT(DEBUG, "in checktreshhold at coordinates: topX="<<topX<<" topY="<<topY<<" botX="<<botX<<" botY"<<botY);
		int widthX=(botX-topX);
		int widthY=(botY-topY);
		double blockArea;
		double triangle=0.0;
		double dspArea=wxDSP*wyDSP;

		if((wFull == wOut) || (topX+topY > wFull-wOut-g))
		{
			int x = widthX>wxDSP ? wxDSP : widthX;
			int y = widthY>wyDSP ? wyDSP : widthY;
			
			blockArea = x*y;
			//checking according to ratio/area
			if(blockArea>=(1.0-ratio)*dspArea)
					return true;
			else
					return false;
		}else
		{
			// if the truncation line splits the block, we need to subtract the area of the lost corner
			// the triangle is the area which will be lost from the block area
			
			//computing the triangle's edge (45degree => area=(edge^2)/2)		
			int x1, x2;
			int y1, y2;
			
			x1 = (topX < botX-wxDSP) ? topX : botX-wxDSP;
			y1 = (topY < botY-wyDSP) ? topY : botY-wyDSP;
			while(x1+y1<wFull-wOut-g)
				x1++;
			
			x2 = (topX < botX-wxDSP) ? topX : botX-wxDSP;
			y2 = (topY < botY-wyDSP) ? topY : botY-wyDSP;
			while(x2+y2<wFull-wOut-g)
				y2++;	
			
			blockArea = (widthX>wxDSP ? wxDSP : widthX) * (widthY>wyDSP ? wyDSP : widthY);
			REPORT(DEBUG, "in worthUsingOneDSP: full blocArea=" << blockArea << " x="<<x1<<" topCornerX="<<((topX < botX-wxDSP) ? topX : botX-wxDSP) << " y="<<y2<<" topCornerY="<<((topY < botY-wyDSP) ? topY : botY-wyDSP));
			
			//computing the triangle's area
			if(topX+topY<=wFull-wOut-g)
				triangle=((x1-((topX < botX-wxDSP) ? topX : botX-wxDSP))*(y2-((topY < botY-wyDSP) ? topY : botY-wyDSP)))/2;
			else
				triangle=0.0;
			
			//the final area which is used
			blockArea = blockArea-triangle;
			REPORT(DEBUG, "in worthUsingOneDSP: usable blockArea=" << blockArea);
			REPORT(DEBUG, "in worthUsingOneDSP: dspArea=" << dspArea);
			
			//checking according to ratio/area
			if(blockArea>=(1.0-ratio)*dspArea)
					return true;
			else
					return false;
		}
	}
	*/
	
	/** 
	 * Checks the area usage of a DSP, according to a given block and DSPThreshold(threshold)
	 * 		- DSPThreshold(threshold) = says what percentage of 1 DSP area is allowed to be lost
	 * Algorithm: compute the area which is used out of a DSP, and compute 
	 * the unused ratio from this. The used area can be a triangle, a trapeze or 
	 * a pentagon, in the truncated case, or a rectangle in the case of a full 
	 * multiplier.
	 **/
	/*
	  Definition of the DSP use threshold t:
	  Consider a submultiplier block, by definition smaller than (or equal to) a DSP in both dimensions
	  let r=(submultiplier area)/(DSP area); r is between 0 and 1
	  if r >= 1-t   then use a DSP for this block 
	  So: t=0 means: any submultiplier that does not fill a DSP goes to logic
        t=1 means: any submultiplier, even very small ones, go to DSP
	*/

	bool IntMultiplier::worthUsingOneDSP(int topX, int topY, int botX, int botY,int wxDSP,int wyDSP)
	{
		REPORT(DEBUG, "in checktreshhold at coordinates: topX="<<topX<<" topY="<<topY<<" botX="<<botX<<" botY"<<botY);
		
		double intersectRightX, intersectRightY, intersectTopX, intersectTopY, intersectBottomX, intersectBottomY, intersectLeftX, intersectLeftY;
		double intersectX1, intersectY1, intersectX2, intersectY2;
		int aTopEdge, bTopEdge, cTopEdge, aBottomEdge, bBottomEdge, cBottomEdge, aRightEdge, bRightEdge, cRightEdge, aLeftEdge, bLeftEdge, cLeftEdge;
		int aTruncLine, bTruncLine, cTruncLine;
		double usefulDSPArea, totalDSPArea;
		
		//if the truncation line passes above the top on the DSP
		if((wFull == wOut) || (topX+topY > wFull-wOut-g))
		{
			REPORT(DEBUG, "in worthUsingOneDSP: full multiplication case, truncation line does not pass through this block");
			int x = max(topX,botX-wxDSP);
			int y = max(topY,botY-wyDSP);
			// REPORT(DEBUG, "*********** x=" << x << "  y=" << y);
			
			usefulDSPArea = (botX-x)*(botY-y);
			
			REPORT(DEBUG, "in worthUsingOneDSP: usable blockArea=" << usefulDSPArea << "   dspArea=" << totalDSPArea);
			
			//checking according to ratio/area
			if(usefulDSPArea >= (1.0-DSPThreshold)*totalDSPArea)
					return true;
			else
					return false;
		}
		
		// equations of the lines which bound the area to be paved
		// line equation: Ax + By + C = 0
		// A=y1-y2, B=x2-x1, C=x1y2-y1x2
		aTopEdge = 0;		//top edge
		bTopEdge = 1;
		cTopEdge = -(topY>botY-wyDSP ? topY : botY-wyDSP);
		
		aBottomEdge = 0;		//bottom edge
		bBottomEdge = 1;
		cBottomEdge = -botY;
		
		aLeftEdge = 1;		//left edge
		bLeftEdge = 0;
		cLeftEdge = -botX;
		
		aRightEdge = 1;		//right edge
		bRightEdge = 0;
		cRightEdge = -(topX>botX-wxDSP ? topX : botX-wxDSP);
		
		//equation of truncation line - given by the 2 points (wX-g, 0) and (0, wY-g)
		aTruncLine = g-wY;
		bTruncLine = g-wX;
		cTruncLine = (wX-g)*(wY-g);
		
		//first, assume that the truncated part is a triangle
		//	then, the two intersections are with the left and bottom edge
		
		//the left edge intersected with the truncation line
		intersectLeftX = 1.0 * (bLeftEdge*cTruncLine - bTruncLine*cLeftEdge) / (aLeftEdge*bTruncLine - aTruncLine*bLeftEdge);
		intersectLeftY = 1.0 * (aTruncLine*cLeftEdge - aLeftEdge*cTruncLine) / (aLeftEdge*bTruncLine - aTruncLine*bLeftEdge);
		
		//the bottom edge intersected with the truncation line
		intersectBottomX = 1.0 * (bBottomEdge*cTruncLine - bTruncLine*cBottomEdge) / (aBottomEdge*bTruncLine - aTruncLine*bBottomEdge);
		intersectBottomY = 1.0 * (aTruncLine*cBottomEdge - aBottomEdge*cTruncLine) / (aBottomEdge*bTruncLine - aTruncLine*bBottomEdge);
		
		//check to see if the intersection points are inside the target square
		//	intersectLeft should be above the top edge
		//	intersectBottom should be to the right of right edge
		
		//check intersectLeft
		if(intersectLeftX*aTopEdge + intersectLeftY*bTopEdge + cTopEdge < 0)
		{
			//then the intersection is the one between the truncation line and 
			//	the top edge
			intersectTopX = 1.0 * (bTopEdge*cTruncLine - bTruncLine*cTopEdge) / (aTopEdge*bTruncLine - aTruncLine*bTopEdge);
			intersectTopY = 1.0 * (aTruncLine*cTopEdge - aTopEdge*cTruncLine) / (aTopEdge*bTruncLine - aTruncLine*bTopEdge);
			
			intersectX1 = intersectTopX;
			intersectY1 = intersectTopY;
		}else
		{
			intersectX1 = intersectLeftX;
			intersectY1 = intersectLeftY;
		}
		
		//check intersectBottom
		if(intersectBottomX*aRightEdge + intersectBottomY*bRightEdge + cRightEdge < 0)
		{
			//then the intersection is the one between the truncation line and 
			//	the right edge
			intersectRightX = 1.0 * (bRightEdge*cTruncLine - bTruncLine*cRightEdge) / (aRightEdge*bTruncLine - aTruncLine*bRightEdge);
			intersectRightY = 1.0 * (aTruncLine*cRightEdge - aRightEdge*cTruncLine) / (aRightEdge*bTruncLine - aTruncLine*bRightEdge);
			
			intersectX2 = intersectRightX;
			intersectY2 = intersectRightY;
		}else
		{
			intersectX2 = intersectBottomX;
			intersectY2 = intersectBottomY;
		}
		
		//renormalize the intersection points' coordinates
		intersectX1 = intersectX1<topX ? topX : intersectX1;
		intersectY2 = intersectY2<topY ? topY : intersectY2;
		
		//compute the useful DSP area
		usefulDSPArea = ((1.0*botX-intersectX1 + 1.0*botX-intersectX2)*(intersectY2-intersectY1)/2.0) + (1.0*botY-intersectY2)*(botX-intersectX2);
		totalDSPArea = wxDSP*wyDSP;
		
		REPORT(DEBUG, "in worthUsingOneDSP: truncated multiplication case");
		REPORT(DEBUG, "in worthUsingOneDSP: usable blockArea=" << usefulDSPArea);
		REPORT(DEBUG, "in worthUsingOneDSP: dspArea=" << totalDSPArea);
		REPORT(DEBUG, "in worthUsingOneDSP: intersectX1=" << intersectX1 << " intersectY1=" << intersectY1 << " intersectX2=" << intersectX2 << " intersectY2=" << intersectY2);
		
		//test if the DSP DSPThreshold is satisfied
		if(usefulDSPArea >= (1.0-DSPThreshold)*totalDSPArea)
				return true;
		else
				return false;
	}

	
	void IntMultiplier::buildAlteraTiling(int blockTopX, int blockTopY, int blockBottomX, int blockBottomY, int multIndex, bool signedX, bool signedY)
	{
		int dspSizeX,dspSizeY;
		int widthX, widthY;
		int topX, topY, botX, botY;
		int multWidthsSize = multWidths.size();
		int newMultIndex = multIndex;
		bool dspSizeNotFound = true;
		bool originalSignedX = signedX;
		bool originalSignedY = signedY;
		
		//set the size of the DSP widths, preliminary
		dspSizeX = multWidths[multWidthsSize-newMultIndex-1];
		if(signedX == false)
			dspSizeX--;
		dspSizeY = multWidths[multWidthsSize-newMultIndex-1];
		if(signedY == false)
			dspSizeY--;
			
		//normalize block coordinates
		if(blockTopX<0)
			blockTopX = 0;
		if(blockTopY<0)
			blockTopY = 0;
		if(blockBottomX<0)
			blockBottomX = 0;
		if(blockBottomY<0)
			blockBottomY = 0;
		
		REPORT(DEBUG, "-----------Call to buildAlteraTiling, at dspSizeX=" << dspSizeX << " and dspSizeY=" << dspSizeY << " with parameters  - blockTopX=" << blockTopX << " blockTopY=" << blockTopY << " blockBottomX=" << blockBottomX << " blockBottomY=" << blockBottomY << (signedX ? " signed" : " unsigned") << " by " << (signedY ? "signed" : "unsigned"));
		
		//if the whole DSP is outside the range of interest, skip over it altogether
		if((blockTopX+blockTopY<wFull-wOut-g) && (blockBottomX+blockBottomY<wFull-wOut-g))
		{
			REPORT(DEBUG, "" << tab << tab << "adding DSP skipped (out of range of interest) at coordinates topX=" << blockTopX << " topY=" << blockTopY << " botX=" << blockBottomX << " botY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
			return;
		}
		
		//if the block is of width/height zero, then end this function call, as there is nothing to do
		widthX = (blockBottomX-blockTopX+1)/dspSizeX;
		widthY = (blockBottomY-blockTopY+1)/dspSizeY;
		if(((widthX==0) && (blockTopX==blockBottomX)) || ((widthY==0) && (blockTopY==blockBottomY)))
			return;
		
		REPORT(DEBUG, "" << tab << "Testing the best DSP size");
		//search for the largest DSP size that corresponds to the ratio
		while(dspSizeNotFound)
		{
			widthX = (blockBottomX-blockTopX+1)/dspSizeX;
			widthY = (blockBottomY-blockTopY+1)/dspSizeY;
			
			REPORT(DEBUG, "" << tab << tab << "at dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY << " and widthX=" << widthX << " widthY=" << widthY);
			
			if((widthX==0) && (widthY==0))
			{
				//if both DSP dimensions are larger, the DSP block might still fit the DSPThreshold
				if(worthUsingOneDSP(blockTopX, blockTopY, blockBottomX, blockBottomY, dspSizeX, dspSizeY))
				{
					//DSPThreshold fulfilled; search is over
					dspSizeNotFound = false;
				}else
				{
					//DSPThreshold not fulfilled; pass on to the next smaller DSP size
					if(newMultIndex == multWidthsSize-1)
					{
						dspSizeNotFound = false;
					}else
					{
						newMultIndex++;
					}
					dspSizeX = multWidths[multWidthsSize-newMultIndex-1];
					if(signedX == false)
						dspSizeX--;
					dspSizeY = multWidths[multWidthsSize-newMultIndex-1];
					if(signedY == false)
						dspSizeY--;
				}
			}
			else
			{
				//there is one dimension on which the DSP can fit
				if((widthX<=1) || (widthY<=1))
				{
					int tx = ((widthX==0 || widthY==0) ? blockBottomX-dspSizeX : blockTopX), ty = ((widthX==0 || widthY==0) ? blockBottomY-dspSizeY : blockTopY);
					int bx = blockBottomX, by = blockBottomY;
					
					//if(worthUsingOneDSP(tx, ty, bx, by, dspSizeX, dspSizeY))
					if(worthUsingOneDSP((tx<blockTopX ? blockTopX : tx), (ty<blockTopY ? blockTopY : ty), bx, by, dspSizeX, dspSizeY))
					{
						//DSPThreshold fulfilled; search is over
						dspSizeNotFound = false;
					}else
					{
						//DSPThreshold not fulfilled; pass on to the next smaller DSP size
						if(newMultIndex == multWidthsSize-1)
						{
							dspSizeNotFound = false;
						}else
						{
							newMultIndex++;
						}
						dspSizeX = multWidths[multWidthsSize-newMultIndex-1];
						if(signedX == false)
							dspSizeX--;
						dspSizeY = multWidths[multWidthsSize-newMultIndex-1];
						if(signedY == false)
							dspSizeY--;
					}
				}else
				{
					dspSizeNotFound = false;
				}
			}
		}
		
		REPORT(DEBUG, "" << tab << "DSP sizes set to dspSizeX=" << dspSizeX << " and dspSizeY=" << dspSizeY);
		
		if(signedX && signedY)
		{
			REPORT(DEBUG, "" << tab << "Initial call to buildAlteraTiling, with both parameters signed");
			
			//SxS multiplication
			//	just for the top-index (for both x and y)
			//the starting point
			//	what remains to be done: one SxS multiplication in the top corner
			//							 recursive call on the top line (UxS)
			//							 recursive call on the right line (SxU)
			//							 recursive call for the rest of the multiplication (UxU)
			
			
			/*
			 * First version: sign is handled in the DSPs
			 */
			
			//top corner, SxS multiplication
			REPORT(DEBUG, "" << tab << "adding DSP (signed by signed) at coordinates topX=" << blockBottomX-dspSizeX << " topY=" << blockBottomY-dspSizeY << " botX=" << blockBottomX << " botY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
			addExtraDSPs(blockBottomX-dspSizeX, blockBottomY-dspSizeY, blockBottomX, blockBottomY, dspSizeX, dspSizeY);
			
			//top line, UxS multiplications where needed, UxU for the rest
			buildAlteraTiling(blockTopX, blockTopY, blockBottomX-dspSizeX, blockBottomY, newMultIndex, false, true);
			
			//right column, SxU multiplication where needed, UxU for the rest
			//FIXME: conditions for sign are not true -> needs to be signed on the bottom part
			buildAlteraTiling(blockBottomX-dspSizeX, blockTopY, blockBottomX, blockBottomY-dspSizeY, newMultIndex, true, false);
			
			
			
			/*
			 * Second version: sign is handled separately
			 */
			/*
			//handle top line
			//		for now just call the buildHeapLogicOnly function which should handle the rest of the work
			buildHeapLogicOnly(blockTopX, blockBottomY-1, blockBottomX, blockBottomY, parentOp->getNewUId());
			 
			//handle right column
			//		for now just call the buildHeapLogicOnly function which should handle the rest of the work
			buildHeapLogicOnly(blockBottomX-1, blockTopY, blockBottomX, blockBottomY-1, parentOp->getNewUId());
			 
			//handle the rest of the multiplication
			//	the remaining part is UxU
			buildAlteraTiling(blockTopX, blockTopY, blockBottomX-1, blockBottomY-1, newMultIndex, false, false);
			*/
		}else
		{
			//SxU, UxS, UxU multiplications - they all share the same structure,
			//	the only thing that changes being the size of the operands
			
			//start tiling
			topX = (blockBottomX-blockTopX < dspSizeX) ? blockTopX : blockBottomX-dspSizeX;
			topY = (blockBottomY-blockTopY < dspSizeY) ? blockTopY : blockBottomY-dspSizeY;
			botX = blockBottomX;
			botY = blockBottomY;
			
			REPORT(DEBUG, "" << tab << "Original block separated in widthX=" << widthX << " by widthY=" << widthY << " blocks");
			
			//handle the bits that can be processed at the current DSP size
			for(int i=0; i<(widthY>0 ? widthY : 1); i++)
			{
				for(int j=0; j<(widthX>0 ? widthX : 1); j++)
				{
					//readjust the sign-ness and the DSP sizes
					//	only the blocks on the border need to have signed components
					if((j!=0) && (signedX))
					{
						signedX = false;
						dspSizeX--;
						
						topX++;
					}
					if((widthX>1) && (j==0) && (originalSignedX) && (i!=0))
					{
						signedX = originalSignedX;
						dspSizeX++;
						
						topX--;
					}
					
					if((i!=0) && (signedY))
					{
						signedY = false;
						dspSizeY--;
						
						topY++;
					}
					
					//if the whole DSP is outside the range of interest
					if((topX+topY<wFull-wOut-g) && (botX+botY<wFull-wOut-g))
					{
						if((widthX!=0) && (j!=widthX-1))
						{
							botX = topX;
							topX = topX-dspSizeX;
						}
						REPORT(DEBUG, "" << tab << tab << "adding DSP skipped (out of range of interest) at coordinates topX=" << blockTopX << " topY=" << blockTopY << " botX=" << blockBottomX << " botY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
						continue;						
					}
					
					if(((widthX<=1) && (widthY<=1)) && ((blockBottomX-blockTopX <= dspSizeX) && (blockBottomY-blockTopY <= dspSizeY)))
					{
						//special case; the remaining block is less than or equal to a DSP
						topX = blockTopX;
						topY = blockTopY;
						botX = blockBottomX;
						botY = blockBottomY;
						
						REPORT(DEBUG, "" << tab << tab << "adding DSP (to cover the whole block) at coordinates topX=" << blockTopX << " topY=" << blockTopY << " botX=" << blockBottomX << " botY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
						addExtraDSPs(blockTopX, blockTopY, blockBottomX, blockBottomY, dspSizeX, dspSizeY);
					}else
					{
						//the regular case; just add a new DSP
						if(worthUsingOneDSP((topX<0 ? blockTopX : topX), (topY<0 ? blockTopY : topY), (botX<0 ? blockTopX : botX), (botY<0 ? blockTopY : botY), dspSizeX, dspSizeY))
						{
							REPORT(DEBUG, "" << tab << tab << "DSPThreshold satisfied - adding DSP at coordinates topX=" << topX << " topY=" << topY << " botX=" << botX << " botY=" << botY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
							addExtraDSPs(topX, topY, botX, botY, dspSizeX, dspSizeY);
						}
						else
						{
							REPORT(DEBUG, "" << tab << tab << "DSPThreshold not satisfied - recursive call at coordinates topX=" << topX << " topY=" << topY << " botX=" << botX << " botY=" << botY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY << (signedX ? " signed" : " unsigned") << " by " << (signedY ? "signed" : "unsigned"));
							if(newMultIndex == multWidthsSize-1)
							{
								REPORT(DEBUG, "" << tab << tab << tab << "size cannot be decreased anymore; will add DSP at this size");
								if((topX+topY<wFull-wOut-g) && (botX+botY<wFull-wOut-g))
								{
									if((widthX!=0) && (j!=widthX-1))
									{
										botX = topX;
										topX = topX-dspSizeX;
									}
									REPORT(DEBUG, "" << tab << tab << "adding DSP skipped (out of range of interest) at coordinates topX=" << blockTopX << " topY=" << blockTopY << " botX=" << blockBottomX << " botY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
									continue;						
								}else
								{
									addExtraDSPs(topX, topY, botX, botY, dspSizeX, dspSizeY);
								}
							}else
							{
								REPORT(DEBUG, "" << tab << tab << tab << "size can be decreased still");
								buildAlteraTiling(topX, topY, botX, botY, newMultIndex+1, signedX, signedY);
							}
						}
						
						if((widthX!=0) && (j!=widthX-1))
						{
							botX = topX;
							topX = topX-dspSizeX;
						}
					}
				}
				
				//pass on to the next column
				if((widthY!=0) && (i!=widthY-1))
				{
					botX = blockBottomX;
					botY = topY;
					topX = (blockBottomX-dspSizeX < blockTopX) ? blockTopX : blockBottomX-dspSizeX;
					topY = (topY-dspSizeY < blockTopY) ? blockTopY : topY-dspSizeY;
				}
			}
			
			REPORT(DEBUG, "" << tab << tab << tab << "last DSP added at topX=" << topX << " topY=" << topY << " botX=" << botX << " botY=" << botY);
			
			//handle the bottom leftover bits, if necessary
			if((topY>0) && (topY != blockTopY))
			{
				REPORT(DEBUG, "" << tab << tab << "handling the bottom leftover bits at coordinates topX=" << topX << " topY=" << blockTopY << " botX=" << blockBottomX << " botY=" << topY << (signedX ? " signed" : " unsigned") << " by " << (signedY ? "signed" : "unsigned"));
				if((topX+blockTopY<wFull-wOut-g) && (blockBottomX+topY<wFull-wOut-g))
				{
					REPORT(DEBUG, "" << tab << tab << tab << "adding DSP skipped (out of range of interest) at coordinates topX=" << topX << " topY=" << blockTopY << " botX=" << blockBottomX << " botY=" << topY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
				}else
				{
					buildAlteraTiling(topX, blockTopY, blockBottomX, topY, newMultIndex, originalSignedX, false);
				}
			}
				
			//handle the left-side leftover bits, if necessary
			if((topX>0) && (topX != blockTopX))
			{
				REPORT(DEBUG, "" << tab << tab << "handling the left-side leftover bits at coordinates topX=" << blockTopX << " topY=" << blockTopY << " botX=" << topX << " botY=" << blockBottomY << (signedX ? " signed" : " unsigned") << " by " << (signedY ? "signed" : "unsigned"));
				if((blockTopX+blockTopY<wFull-wOut-g) && (topX+blockBottomY<wFull-wOut-g))
				{
					REPORT(DEBUG, "" << tab << tab << tab << "adding DSP skipped (out of range of interest) at coordinates topX=" << blockTopX << " topY=" << blockTopY << " botX=" << topX << " botY=" << blockBottomY << " dspSizeX=" << dspSizeX << " dspSizeY=" << dspSizeY);
				}else
				{
					buildAlteraTiling(blockTopX, blockTopY, topX, blockBottomY, newMultIndex, false, originalSignedY);
				}
			}
			
			REPORT(DEBUG, "-----------End of call to buildAlteraTiling, at dspSizeX=" << dspSizeX << " and dspSizeY=" << dspSizeY << " with parameters  - blockTopX=" << blockTopX << " blockTopY=" << blockTopY << " blockBottomX=" << blockBottomX << " blockBottomY=" << blockBottomY << (originalSignedX ? " signed" : " unsigned") << " by " << (originalSignedY ? "signed" : "unsigned"));
		}
	}


	void IntMultiplier::buildXilinxTiling()
	{
		int widthXX,widthX;//local wxDSP
		int widthYY,widthY;//local wyDSP
		int hor1,hor2,ver1,ver2;	
		int horizontalDSP,verticalDSP;
		int nrDSPvertical=checkTiling(wyDSP,wxDSP,hor1,ver1); //number of DSPs used in the case of vertical tiling
		int nrDSPhorizontal=checkTiling(wxDSP,wyDSP,hor2,ver2);//number of DSPs used in case of horizontal tiling
		int botx=wX;
		int boty=wY;
		int topx,topy;

		//decides if a horizontal tiling will be used or a vertical one
		if(nrDSPvertical<nrDSPhorizontal)
		{
			widthXX=wyDSP;
			widthYY=wxDSP;
			horizontalDSP=hor1;
			verticalDSP=ver1;
		}
		else
		{
			widthXX=wxDSP;
			widthYY=wyDSP;
			horizontalDSP=hor2;
			verticalDSP=ver2;
		}


		//applying the tiles
		for(int i=0;i<verticalDSP;i++)
		{
			//managing the size of a DSP according to its position if signed
			if((signedIO)&&(i!=0))
				widthY=widthYY-1;
			else
				widthY=widthYY;	

			topy=boty-widthY;
			botx=wX;

			for(int j=0;j<horizontalDSP;j++)
			{
				//managing the size of a DSP according to its position if signed
				if((signedIO)&&(j!=0))
					widthX=widthXX-1;
				else
					widthX=widthXX;

				topx=botx-widthX;

				if(botx+boty>wFull-wOut-g)
					addExtraDSPs(topx,topy,botx,boty,widthX,widthY);
				botx=botx-widthX;			
			}

			boty=boty-widthY;
		}

	}


	IntMultiplier::~IntMultiplier() {
	}


	//signal name construction

	string IntMultiplier::addUID(string name, int blockUID)
	{
		ostringstream s;
		s << name << "_m" << multiplierUid;
		if (blockUID!=-1) 
			s << "b"<< blockUID;
		return s.str() ;
	};

	string IntMultiplier::PP(int i, int j, int uid ) {
		std::ostringstream p;		
		if(uid==-1) 
			p << "PP" <<  "_X" << i << "Y" << j;
		else
			p << "PP" <<uid<<"X" << i << "Y" << j;
		return  addUID(p.str());
	};

	string IntMultiplier::PPTbl(int i, int j, int uid ) {
		std::ostringstream p;		
		if(uid==-1) 
			p << addUID("PP") <<  "_X" << i << "Y" << j << "_Tbl";
		else
			p << addUID("PP") <<"_"<<uid<<"X" << i << "Y" << j << "_Tbl";
		return p.str();
	};


	string IntMultiplier::XY( int i, int j,int uid) {
		std::ostringstream p;		
		if(uid==-1) 
			p  << "Y" << j<< "X" << i;
		else
			p  << "Y" << j<< "X" << i<<"_"<<uid;
		return  addUID(p.str());	
	};






	IntMultiplier::SmallMultTable::SmallMultTable(Target* target, int dx, int dy, int wO, bool negate, bool  signedX, bool  signedY ) : 
		Table(target, dx+dy, wO, 0, -1, true), // logic table
		dx(dx), dy(dy), wO(wO), negate(negate), signedX(signedX), signedY(signedY) {
		ostringstream name; 
		srcFileName="LogicIntMultiplier::SmallMultTable";
		// No getUid() in the name: this kind of table should be added to the globalOpList 
		name <<"SmallMultTable"<< (negate?"M":"P") << dy << "x" << dx << "r" << wO << (signedX?"Xs":"Xu") << (signedY?"Ys":"Yu");
		setName(name.str());				
	};


	mpz_class IntMultiplier::SmallMultTable::function(int yx){
		mpz_class r;
		int y = yx>>dx;
		int x = yx -(y<<dx);
		int wF=dx+dy;

		if(signedX){
			if ( x >= (1 << (dx-1)))
				x -= (1 << dx);
		}
		if(signedY){
			if ( y >= (1 << (dy-1)))
				y -= (1 << dy);
		}
		//if(!negate && signedX && signedY) cerr << "  y=" << y << "  x=" << x;
		r = x * y;
		//if(!negate && signedX && signedY) cerr << "  r=" << r;
		if(negate)
			r=-r;
		//if(negate && signedX && signedY) cerr << "  -r=" << r;
		if ( r < 0)
			r += mpz_class(1) << wF; 
		//if(!negate && signedX && signedY) cerr << "  r2C=" << r;

		if(wOut<wF){ // wOut is that of Table
			// round to nearest, but not to nearest even
			int tr=wF-wOut; // number of truncated bits 
			// adding the round bit at half-ulp position
			r += (mpz_class(1) << (tr-1));
			r = r >> tr;
		}

		//if(!negate && signedX && signedY) cerr << "  rfinal=" << r << endl;

		return r;

	};



	void IntMultiplier::emulate (TestCase* tc)
	{
		mpz_class svX = tc->getInputValue("X");
		mpz_class svY = tc->getInputValue("Y");
		mpz_class svR;

		if(!signedIO)
		{
			svR = svX * svY;
		}
		else
		{
			// Manage signed digits
			mpz_class big1 = (mpz_class(1) << (wXdecl));
			mpz_class big1P = (mpz_class(1) << (wXdecl-1));
			mpz_class big2 = (mpz_class(1) << (wYdecl));
			mpz_class big2P = (mpz_class(1) << (wYdecl-1));

			if(svX >= big1P)
				svX -= big1;

			if(svY >= big2P)
				svY -= big2;

			svR = svX * svY;
		}
		
		if(negate)
			svR = -svR;

		// manage two's complement at output
		if(svR < 0)
		{
			svR += (mpz_class(1) << wFull); 
		}
		if(wTruncated==0) 
			tc->addExpectedOutput("R", svR);
		else
		{
			// there is truncation, so this mult should be faithful
			svR = svR >> wTruncated;
			tc->addExpectedOutput("R", svR);
			svR++;
			svR &= (mpz_class(1) << (wOut)) -1;
			tc->addExpectedOutput("R", svR);
		}
	}



	void IntMultiplier::buildStandardTestCases(TestCaseList* tcl)
	{
		TestCase *tc;

		mpz_class x, y;

		// 1*1
		x = mpz_class(1); 
		y = mpz_class(1); 
		tc = new TestCase(this); 
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);

		// -1 * -1
		x = (mpz_class(1) << wXdecl) -1; 
		y = (mpz_class(1) << wYdecl) -1; 
		tc = new TestCase(this); 
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);

		// The product of the two max negative values overflows the signed multiplier
		x = mpz_class(1) << (wXdecl -1); 
		y = mpz_class(1) << (wYdecl -1); 
		tc = new TestCase(this); 
		tc->addInput("X", x);
		tc->addInput("Y", y);
		emulate(tc);
		tcl->add(tc);
	}




}
