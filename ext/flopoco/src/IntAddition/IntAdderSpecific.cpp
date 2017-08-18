/*
  An integer adder for FloPoCo using vendor primitives
 
  It may be pipelined to arbitrary frequency.
  Also useful to derive the carry-propagate delays for the subclasses of Target
 
  Author: Bogdan Pasca

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include <gmp.h>
#include <mpfr.h>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"
#include "IntAdderSpecific.hpp"

using namespace std;
namespace flopoco{

	IntAdderSpecific::IntAdderSpecific(Target* target, int wIn, map<string, double> inputDelays):
		Operator(target,inputDelays), wIn_(wIn), inputDelays_(inputDelays)
	{	
		srcFileName="IntAdderSpecific";
		setName(join("IntAdderSpecific_", wIn_,"_",getNewUId()));
		
		// Set up the IO signals
		addInput ( "X", wIn_,true);
		addInput ( "Y", wIn_,true);
		addInput("Cin");
		addOutput("R"  , wIn_,2,true);
		addOutput("Cout", 1, false);

		/*unpipelined IntAdder architecture FPGA specific primitives*/
		if (target->getVendor() == "Xilinx"){
			declare("p",wIn,true);
			declare("c",wIn-1,true);
			for (int i=0; i<wIn;i++){
				vhdl << tab << "l"<<getNewUId()<<": LUT2_L generic map ( INIT => X\"6\")"<<endl;
				vhdl << tab << "port map( LO => p("<<i<<"),"<<endl;
				vhdl << tab << "          I0 => X("<<i<<"),"<<endl;
				vhdl << tab << "          I1 => Y("<<i<<"));"<<endl;
				vhdl << tab << "l"<<getNewUId()<<": MUXCY port map ("<<endl;
				if (i==wIn-1)
					vhdl << tab << "          O  => Cout, -- Carry local output signal"<<endl;
				else
					vhdl << tab << "          O  => c("<<i<<"), -- Carry local output signal"<<endl;
				if (i==0)
					vhdl << tab << "          CI => Cin,  -- Carry input signal"<<endl;
				else
					vhdl << tab << "          CI => c("<<i-1<<"),  -- Carry input signal"<<endl;
				vhdl << tab << "          DI => X("<<i<<"), -- Data input signal"<<endl;
				vhdl << tab << "          S  => p("<<i<<")   -- MUX select, tie to '1' or LUT4 out"<<endl;
				vhdl << tab << ");"<<endl;
				vhdl << tab << "l"<<getNewUId()<<": XORCY port map ("<<endl;
				vhdl << tab << "      O => R("<<i<<"),   -- XOR output signal"<<endl;
				if (i==0)
					vhdl << tab << "      CI => Cin, -- Carry input signal"<<endl;
				else 
					vhdl << tab << "      CI => c("<<i-1<<"), -- Carry input signal"<<endl;
				vhdl << tab << "      LI => p("<<i<<")  -- LUT4 input signal"<<endl;
				vhdl << tab << ");"<<endl;
			}
		}else{ //ALTERA 
			vhdl << tab << "LPM_ADD_SUB_component : LPM_ADD_SUB"<<endl;
			vhdl << tab << "GENERIC MAP ("<<endl;
			vhdl << tab << "	lpm_direction => \"ADD\","<<endl;
			vhdl << tab << "	lpm_hint => \"ONE_INPUT_IS_CONSTANT=NO,CIN_USED=YES\","<<endl;
//			vhdl << tab << "	lpm_pipeline => 0,"<<endl;
			vhdl << tab << "	lpm_representation => \"UNSIGNED\","<<endl;
			vhdl << tab << "	lpm_type => \"LPM_ADD_SUB\","<<endl;
			vhdl << tab << "	lpm_width => "<<wIn<<""<<endl;
			vhdl << tab << ")"<<endl;
			vhdl << tab << "PORT MAP ("<<endl;
			vhdl << tab << "	cin => Cin,"<<endl;
//			vhdl << tab << "	clock => clk,"<<endl;
			vhdl << tab << "	datab => Y,"<<endl;
			vhdl << tab << "	dataa => X,"<<endl;
			vhdl << tab << "    cout => Cout,"<<endl;
			vhdl << tab << "	result => R"<<endl;
			vhdl << tab << ");"<<endl;
		}	
	}

	IntAdderSpecific::~IntAdderSpecific() {
	}

	
	void IntAdderSpecific::outputVHDL(std::ostream& o, std::string name) {
		ostringstream signame;
		licence(o);
		pipelineInfo(o);
		o << "library ieee; " << endl;
		o << "use ieee.std_logic_1164.all;" << endl;
		o << "use ieee.std_logic_arith.all;" << endl;
		o << "library work;" << endl;
		if (target_->getVendor() == "Xilinx"){
			o << "library UNISIM;"<<endl;
			o << "use UNISIM.VComponents.all;"<<endl;
		}else if(target_->getVendor() == "Altera"){
			o << "LIBRARY lpm;"<<endl;
			o << "USE lpm.all;"<<endl;
		}
		outputVHDLEntity(o);
		newArchitecture(o,name);
		if (target_->getVendor() == "Altera"){
			o << "	COMPONENT lpm_add_sub "<<endl;
			o << "	GENERIC ("<<endl;
			o << "		lpm_direction		: STRING;"<<endl;
			o << "		lpm_hint		: STRING;"<<endl;
			o << "		lpm_representation		: STRING;"<<endl;
			o << "		lpm_type		: STRING;"<<endl;
			o << "		lpm_width		: NATURAL"<<endl;
			o << "	);"<<endl;
			o << "	PORT ("<<endl;
			o << "			cin	: IN STD_LOGIC ;"<<endl;
			o << "			datab	: IN STD_LOGIC_VECTOR ("<<wIn_-1<<" DOWNTO 0);"<<endl;
			o << "			cout	: OUT STD_LOGIC ;"<<endl;
			o << "			dataa	: IN STD_LOGIC_VECTOR ("<<wIn_-1<<" DOWNTO 0);"<<endl;
			o << "			result	: OUT STD_LOGIC_VECTOR ("<<wIn_-1<<" DOWNTO 0)"<<endl;
			o << "	);"<<endl;
			o << "	END COMPONENT;"<<endl;			
		}

		o << buildVHDLComponentDeclarations();	
		o << buildVHDLSignalDeclarations();
		beginArchitecture(o);		
		o<<buildVHDLRegisters();
		o << vhdl.str();
		endArchitecture(o);
	}

	
	void IntAdderSpecific::emulate(TestCase* tc)
	{
		mpz_class svX= tc->getInputValue("X");
		mpz_class svY= tc->getInputValue("Y");
		mpz_class svC =  tc->getInputValue("Cin");

		mpz_class sR = svX + svY + svC;
		
		mpz_class svR = sR % (1<<wIn_);
		mpz_class cout = sR >> wIn_;
		
		tc->addExpectedOutput("R", svR);
		tc->addExpectedOutput("Cout",cout);
	}
}
