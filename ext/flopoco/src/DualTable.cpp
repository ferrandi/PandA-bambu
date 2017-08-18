/*
  Primitive Dual Port Memory block for FloPoCo

  Author: Radu Tudoran

  This file is part of the FloPoCo project 
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.

*/


/* 
   The class contains a primitive Dual Port Memory block.

   The memories are split in fundamental units (blocks) that can be
   recognized by the synthesis tools as dual port memories. The limit of
   a memory block until it is recognized as a dual port one is 2^11*9
   bits on Virtex 4.

   This class is an abstract one, and in order to be used it must be
   extended by a concrete class which will implement the "function"
   method. It is very similar to the Table class.  */

#include "DualTable.hpp"


using namespace std;



namespace flopoco{


	int DualTable::double2input(double x){
		cerr << "Error, double2input is being used and has not been overriden";
		return 1;
	}

	double DualTable::input2double(int x) {
		cerr << "Error, input2double is being used and has not been overriden";
		return 1;
	}

	mpz_class DualTable::double2output(double x){
		cerr << "Error, double2output is being used and has not been overriden";
		return 0;
	}

	double DualTable::output2double(mpz_class x) {
		cerr << "Error, output2double is being used and has not been overriden";
		return 1;
	}



	DualTable::DualTable(Target* target, int _wIn, int _wOut, int _minIn, int _maxIn) : 
		Operator(target),
		wIn(_wIn), wOut(_wOut), minIn(_minIn), maxIn(_maxIn),target(target)
	{
	
		setCopyrightString("Radu Tudoran, Florent de Dinechin (2009)");
		setCombinatorial();

		//limitSingleMemory = intpow2(11)*9;
		limitSingleMemory = target->sizeOfMemoryBlock();
		
		// Set up the IO signals
		addInput ("X1"  , wIn);
		addOutput ("Y1"  , wOut);
		addInput ("X2"  , wIn);
		addOutput ("Y2"  , wOut);			
		
		// if((1<<wIn)*wOut<=limitSingleMemory)
		// 	{
		if(maxIn==-1) maxIn=(1<<wIn)-1;
		if(minIn<0) {
			cerr<<"ERROR in DualTable::DualTable, minIn<0\n";
			exit(EXIT_FAILURE);
		}
		if(maxIn>=(1<<wIn)) {
			cerr<<"ERROR in DualTable::DualTable, maxIn too large\n";
			exit(EXIT_FAILURE);
		}
		if((minIn==0) && (maxIn==(1<<wIn)-1)) 
			full=true;
		else
			full=false;
		nrOfMemBlocks=1;
		// }
		// else
		// 	{
		// 		maxIn= intlog2(limitSingleMemory / wOut);
		// 		if((1<<maxIn)*wOut>limitSingleMemory)
		// 			maxIn--;
		
		// 		nrOfMemBlocks = 1<<(wIn-maxIn);
	
		// 		if(nrOfMemBlocks * maxIn<wIn)
		// 			nrOfMemBlocks++;
		
			
		// 		if(minIn<0) {
		// 			cerr<<"ERROR in DualTable::DualTable, minIn<0\n";
		// 			exit(EXIT_FAILURE);
		// 		}
	
		// 		full=true;
	
		// 		cerr << "WARNING : FloPoCo is building a DualTable with " << wIn <<" X "<< wOut<< " , it will be large." << endl;
		// 	}
	}
	


	DualTable::DualTable(Target* target) : 
		Operator(target)
	{
		setCopyrightString("Radu Tudoran (2010)");
	}


	void DualTable::outputVHDL(std::ostream& o, std::string name)
	{
		fillTable();
		Operator::outputVHDL(o,  name);
	}



	void DualTable::fillTable() {
		// if( (1<<wIn) * wOut<=limitSingleMemory)
		// 	{
		ostringstream array;
		int x;
		mpz_class y;
		ostringstream type;

		type<<"array (0 to "<< intpow2(wIn)-1<< ") of std_logic_vector("<<wOut-1 <<" downto 0)";
		addType("ROMContent", type.str());
				

		array <<tab << "( "<<endl;
	
		int count=0;
		for (x = minIn; x <= maxIn; x++) {
			//cout << x << "  " << y << endl;
			y=function(x);
			array 	<< tab<<tab << "\"" << unsignedBinary(y, wOut)  ;
			if(x!=maxIn)
				array<< "\", ";
			else
				array<<"\" ";
			count++;
			if(count==4)
				{array<<endl;
					count=0;
				}
		}
		array<<")"<<endl;

				
		addConstant("memVar", "ROMContent", array.str());

		vhdl<<tab<<tab<<tab<<" Y1 <= memVar(conv_integer(X1)); "<<endl;
		vhdl<<tab<<tab<<tab<<" Y2 <= memVar(conv_integer(X2)); "<<endl;
	} 

#if 0
	// This is remnants of code by Radu that did the splitting of a large table in many smaller tables. 
	// I can't believe the tools are unable to do it.
	// To check
	else
		{
			throw string("Large DualTables broken for now, sorry");
			ostringstream name1,name2,name3, data;
			int rangeHighAddress = wIn - maxIn;
		
			vhdl<<tab<<declare("address1L",maxIn)<<" <= X1"<<range(maxIn-1,0)<<";"<<endl;
			vhdl<<tab<<declare("address1H",rangeHighAddress,true)<<" <= X1"<<range(wIn-1,maxIn)<<";"<<endl;
			vhdl<<tab<<declare("address2L",maxIn)<<" <= X2"<<range(maxIn-1,0)<<";"<<endl;
			vhdl<<tab<<declare("address2H",rangeHighAddress,true)<<" <= X2"<<range(wIn-1,maxIn)<<";"<<endl;
	
			primitiveBlocks = (primitiveDualMemory**) calloc (nrOfMemBlocks,sizeof(primitiveDualMemory*));

			for(int c1=0;c1<nrOfMemBlocks;c1++)
				{
					data.str("");
					name1.str("");
					name1<<"primitiveBlock_"<<c1;
					name2.str("");
					name2<<"data1FromM_"<<c1;
					name3.str("");
					name3<<"data2FromM_"<<c1;
		
					int maxInInner= (1<<maxIn);
		
					for(int v=minIn;v<=maxInInner-1-minIn;v++)
						{
							data<<function(v +maxInInner*c1)<<" ";
						}
		
					primitiveBlocks[c1]  = new primitiveDualMemory(target,maxIn,wOut,minIn,maxInInner-1-minIn);
					primitiveBlocks[c1]->setInputData(data);
					primitiveBlocks[c1]  ->changeName(getName()+name1.str());	
					oplist.push_back(primitiveBlocks[c1] );
					inPortMapCst  (primitiveBlocks[c1]  , "X1","address1L");
					inPortMapCst  (primitiveBlocks[c1]  , "X2","address2L");
					outPortMap (primitiveBlocks[c1]  , "Y1",name2.str());
					outPortMap (primitiveBlocks[c1]  , "Y2",name3.str());
					vhdl << instance(primitiveBlocks[c1]  ,name1.str());
				}
	
			vhdl<<endl;
			vhdl <<tab<< "  with address1H select  Y1 <= " << endl;
			for (int c1 = 0; c1 < nrOfMemBlocks; c1++) {
				name2.str("");
				name2<<"data1FromM_"<<c1;
				vhdl  << tab << name2.str()<<" when \"" << unsignedBinary(c1,rangeHighAddress) << "\"," << endl;
			}
			vhdl << tab << "\"";
			for (i = 0; i < wOut; i++) 
				vhdl << "-";
			vhdl <<  "\" when others;" << endl;
	
			vhdl<<endl;
			vhdl <<tab<< "  with address2H select  Y2 <= " << endl;
			for (int c1 = 0; c1 < nrOfMemBlocks; c1++) {
				name3.str("");
				name3<<"data2FromM_"<<c1;
				vhdl 	<<tab <<tab << name3.str()<<" when \"" << unsignedBinary(c1,rangeHighAddress) << "\"," << endl;
			}
			vhdl << tab<<tab << "\"";
			for (i = 0; i < wOut; i++) 
				vhdl << "-";
			vhdl <<  "\" when others;" << endl;		
		}
#endif
	





	int DualTable::size_in_LUTs() {
		return wOut*(1<<(wIn-target_->lutInputs()));
	}


	void DualTable::primitiveDualMemory::setInputData(std::ostringstream&  data)
	{
		inputData.str("");
		inputData<<data.str();
	}
		
				
	DualTable::primitiveDualMemory::primitiveDualMemory(Target* target, int _wIn, int _wOut, int _minIn, int _maxIn) : 
		Operator(target),
		wIn(_wIn), wOut(_wOut), minIn(_minIn), maxIn(_maxIn) 
	{
		 setCopyrightString("Radu Tudoran, Florent de Dinechin (2009)");
		 setCombinatorial();
		
		// Set up the IO signals

		addInput ("X1"  , wIn);
		addOutput ("Y1"  , wOut);
		addInput ("X2"  , wIn);
		addOutput ("Y2"  , wOut);			
		
		ostringstream array;
		int x;
		mpz_class y;
		ostringstream type;

		type<<"array (0 to "<< intpow2(wIn)-1<< ") of std_logic_vector("<<wOut-1 <<" downto 0)";
		addType("ROMContent", type.str());
				

		array <<tab << "( "<<endl;
	
		int count=0;
		for (x = minIn; x <= maxIn; x++) {
			inputData>>y;
			array 	<< tab<<tab << "\"" << unsignedBinary(y, wOut)  ;
			if(x!=maxIn)
				array<< "\", ";
			else
				array<<"\" ";
			count++;
			if(count==4)
				{array<<endl;
					count=0;
				}
		}
		array<<")"<<endl;
				
		addConstant("memVar", "ROMContent", array.str());

		vhdl<<tab<<tab<<tab<<" Y1 <= memVar(conv_integer(X1)); "<<endl;
		vhdl<<tab<<tab<<tab<<" Y2 <= memVar(conv_integer(X2)); "<<endl;
	}

}

	
	
