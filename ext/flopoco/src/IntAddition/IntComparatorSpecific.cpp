/*
  An integer comparator for FloPoCo using vendor primitives
 
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
#include "IntComparatorSpecific.hpp"

using namespace std;
namespace flopoco{

	IntComparatorSpecific::IntComparatorSpecific(Target* target, int wIn, int type, map<string, double> inputDelays):
		Operator(target,inputDelays), wIn_(wIn), type_(type), inputDelays_(inputDelays)
	{	
	
//		type 1 // X ge not(y)
		srcFileName="IntComparatorSpecific";
		setName(join("IntComparatorSpecific_", wIn_,"_",getNewUId()));
		
		// Set up the IO signals
		addInput ( "X", wIn_,true);
		addInput ( "Y", wIn_,true);
		addOutput( "R", 1, false);
		
		int evenwIn = (wIn%2==0?wIn:wIn+1);
		bool extwIn = (evenwIn==wIn?false:true);
		vhdl << tab << declare("sX", evenwIn) << " <= "<<(extwIn?"\"0\" &":"") << " X;"<<endl;
		vhdl << tab << declare("sY", evenwIn) << " <= "<<(extwIn?"\"1\" &":"") << " Y;"<<endl;

		
		/*unpipelined IntAdder architecture FPGA specific primitives*/
		if (target->getVendor() == "Xilinx"){
			declare("p",evenwIn/2,true);
			declare("g",evenwIn/2,true);
			declare("c",evenwIn/2,true);
			for (int i=0; i<evenwIn;i+=2){
				vhdl << tab << "l"<<getNewUId()<<": LUT6_2 generic map ( INIT => X\"12481248EC80EC80\")"<<endl;
				vhdl << tab << "port map( O6 => p("<<i/2<<"),"<<endl;
				vhdl << tab << "          O5 => g("<<i/2<<"),"<<endl;
				vhdl << tab << "          I0 => sX("<<i<<"),"<<endl;
				vhdl << tab << "          I1 => sX("<<i+1<<"),"<<endl;
				vhdl << tab << "          I2 => sY("<<i<<"),"<<endl;
				vhdl << tab << "          I3 => sY("<<i+1<<"),"<<endl;
				vhdl << tab << "          I4 => '0',"<<endl;
				vhdl << tab << "          I5 => '1');"<<endl; //fixed value
				vhdl << tab << "l"<<getNewUId()<<": MUXCY port map ("<<endl;
				if (i+2>=evenwIn)
					vhdl << tab << "          O  => R, -- Carry local output signal"<<endl;
				else
					vhdl << tab << "          O  => c("<<i/2<<"), -- Carry local output signal"<<endl;
				if (i==0)		
					if (type == 1)
						vhdl << tab << "          CI => '1',  -- Carry input signal"<<endl;
					else
						vhdl << tab << "          CI => '0',  -- Carry input signal"<<endl; //TODO for other values of Cin
				else
					vhdl << tab << "      CI => c("<<i/2-1<<"),  -- Carry input signal"<<endl;
				vhdl << tab << "          DI => g("<<i/2<<"), -- Data input signal"<<endl;
				vhdl << tab << "          S  => p("<<i/2<<")   -- MUX select, tie to '1' or LUT4 out"<<endl;
				vhdl << tab << ");"<<endl;
			}
		}else{ 
		//ALTERA 
			vhdl << tab << "LPM_ADD_SUB_component : LPM_ADD_SUB"<<endl;
			vhdl << tab << "GENERIC MAP ("<<endl;
			vhdl << tab << "	lpm_direction => \"ADD\","<<endl;
			vhdl << tab << "	lpm_hint => \"ONE_INPUT_IS_CONSTANT=NO,CIN_USED=YES\","<<endl;
			vhdl << tab << "	lpm_representation => \"UNSIGNED\","<<endl;
			vhdl << tab << "	lpm_type => \"LPM_ADD_SUB\","<<endl;
			vhdl << tab << "	lpm_width => "<<wIn<<""<<endl;
			vhdl << tab << ")"<<endl;
			vhdl << tab << "PORT MAP ("<<endl;
			if (type_==0)
			vhdl << tab << "	cin => '0',"<<endl;
			else
			vhdl << tab << "	cin => '1',"<<endl;
			
			vhdl << tab << "	datab => Y,"<<endl;
			vhdl << tab << "	dataa => X,"<<endl;
			vhdl << tab << "    cout => R"<<endl;
			vhdl << tab << ");"<<endl;
		}	
	}

	IntComparatorSpecific::~IntComparatorSpecific() {
	}

	
	void IntComparatorSpecific::outputVHDL(std::ostream& o, std::string name) {
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
			o << "			dataa	: IN STD_LOGIC_VECTOR ("<<wIn_-1<<" DOWNTO 0)"<<endl;
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

	
	void IntComparatorSpecific::emulate(TestCase* tc)
	{
		mpz_class svX= tc->getInputValue("X");
		mpz_class svY= tc->getInputValue("Y");
		mpz_class svR;
		svR = svX + svY;
		if (type_==1){
			svR = svR + 1;
		}
		svR = (svR>>wIn_);
		tc->addExpectedOutput("R", svR);
	}
}
