/*
  FIXME
 
  Author: ???
 

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.

 */

#include <iostream>
#include <sstream>
#include <cstdlib>
#include "utils.hpp"
#include "DualTable2.hpp"

using namespace std;


namespace flopoco{


	int DualTable2::double2input(double x){
		throw string("Error, double2input is being used and has not been overriden");
	}

	double DualTable2::input2double(int x) {
		throw string("Error, input2double is being used and has not been overriden");
	}

	mpz_class DualTable2::double2output(double x){
		throw string("Error, double2output is being used and has not been overriden");
	}

	double DualTable2::output2double(mpz_class x) {
		throw string("Error, output2double is being used and has not been overriden");
	}

#if 0 // TODO some day
	mpz_class DualTable2::mpfr2output(mpfr_t x){
		throw string("Error, mpfr2output is being used and has not been overriden");
	}

	void DualTable2::output2mpfr(mpz_class x, mpfr_t y) {
		throw string("Error, output2mpfr is being used and has not been overriden");
	}
#endif



	DualTable2::DualTable2(Target* target, int _wIn, int _wOut, int _minIn, int _maxIn) : 
		Operator(target),
		wIn(_wIn), wOut(_wOut), minIn(_minIn), maxIn(_maxIn)
	{
		setCopyrightString("Florent de Dinechin (2007)");

		// Set up the IO signals
		addInput ("X1"  , wIn, true);
		addInput ("X2"  , wIn, true);
		addOutput ("Y1"  , wOut);
		addOutput ("Y2"  , wOut);
		
		nextCycle(); //delay is 1 cycle 
		
		if(maxIn==-1) maxIn=(1<<wIn)-1;
		if(minIn<0) {
			cerr<<"ERROR in DualTable2::DualTable2, minIn<0\n";
			exit(EXIT_FAILURE);
		}
		if(maxIn>=(1<<wIn)) {
			cerr<<"ERROR in DualTable2::DualTable2, maxIn too large\n";
			exit(EXIT_FAILURE);
		}
		if((minIn==0) && (maxIn==(1<<wIn)-1)) 
			full=true;
		else
			full=false;
		if (wIn > 10)
		  REPORT(0, "WARNING : FloPoCo is building a DualTable2 with " << wIn << " input bits, it will be large.");
	}

DualTable2::DualTable2(Target* target) : 
		Operator(target)
	{
		setCopyrightString("Florent de Dinechin, Bogdan Pasca (2007, 2010)");
		if ((target->getVendor()=="Xilinx")){
			setCombinatorial();
		}else
			nextCycle();
	}

	// We have to define this method because the constructor of DualTable2 cannot use the (pure virtual) function()
	void DualTable2::outputVHDL(std::ostream& o, std::string name) {

			int x;
			mpz_class y;
			licence(o);

			o << "library ieee; " << endl;
			o << "use ieee.std_logic_1164.all;" << endl;
			o << "use ieee.numeric_std.all;" << endl;
			o << "library work;" << endl;
			outputVHDLEntity(o);
			newArchitecture(o,name);
			
			o << tab << "-- Build a 2-D array type for the RoM" << endl;
			o << tab << "subtype word_t is std_logic_vector("<<wOut-1<<" downto 0);" << endl;
			o << tab << "type memory_t is array(0 to 2**"<<wIn<<"-1) of word_t;" << endl;
			o << tab <<"function init_rom" << endl;
			o << tab << tab << "return memory_t is " << endl;
			o << tab << tab << "variable tmp : memory_t := (" << endl;
			for (x = minIn; x <= maxIn; x++) {
				y=function(x);
				//if( y>=(1<<wOut) || y<0)
					//REPORT(0, "Output out of range" << "x=" << x << "  y= " << y );
				o << tab << "\"" << unsignedBinary(y, wOut) << "\"," << endl;
			}
			o << tab << tab << "others => (others => '0'));" << endl;
			o << tab << tab << "	begin " << endl;
			o << tab << tab << "return tmp;" << endl;
			o << tab << tab << "end init_rom;" << endl;
			
			o << "	signal rom : memory_t := init_rom;" << endl;
			beginArchitecture(o);		
			o << "	process(clk)" << endl;
			o << tab << "begin" << endl;
			o << tab << "if(rising_edge(clk)) then" << endl;
			o << tab << "	Y1 <= rom(  TO_INTEGER(unsigned(X1))  );" << endl;
			o << tab << "   Y2 <= rom(  TO_INTEGER(unsigned(X2))  );" << endl;
			o << tab << "end if;" << endl;
			o << tab << "end process;" << endl;
			endArchitecture(o);
	
	}


	int DualTable2::size_in_LUTs() {
		return wOut*int(intpow2(wIn-target_->lutInputs()));
	}

}
