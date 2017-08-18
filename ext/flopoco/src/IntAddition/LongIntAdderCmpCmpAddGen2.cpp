/*
  A FIXME for FloPoCo. 
 
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
#include "LongIntAdderCmpCmpAddGen2.hpp"
#include "../IntAdder.hpp"
#include "../IntComparator.hpp"
	
using namespace std;
namespace flopoco{

	LongIntAdderCmpCmpAddGen2::LongIntAdderCmpCmpAddGen2(Target* target, int wIn, map<string, double> inputDelays):
		Operator(target), wIn_(wIn), inputDelays_(inputDelays) 
	{
		srcFileName="LongIntAdderCmpCmpAddGen2";
		setName(join("LongIntAdderCmpCmpAddGen2_", wIn_));
		
		// Set up the IO signals
		addInput ( "X", wIn_);
		addInput ( "Y", wIn_);
		addInput( "Cin");
		addOutput( "R"  , wIn_);

		if (isSequential()){
			
		//compute the maximum input delay
//		maxInputDelay = getMaxInputDelays(inputDelays);
			
		double xordelay;
		double dcarry;
		double muxcystoo;
		double muxcytolacal;
		if (target->getID()=="Virtex5"){
			xordelay = 0.300e-9;
			dcarry = 0.023e-9;
			muxcystoo = 0.305e-9;
			muxcytolacal = 0.222e-9;
		}else if (target->getID()=="Virtex6"){
			xordelay = 0.180e-9;
			dcarry = 0.015e-9;
			muxcystoo =	0.219e-9;
			muxcytolacal = 0.169e-9;
		}else if(target->getID()=="Virtex4"){
			xordelay = 0.273e-9;
			dcarry = 0.034e-9;
			muxcystoo = 0.278e-9;
			muxcytolacal = 0.273e-9;
		}
		
		int ll;
		int l1;
		int l0;
		int maxAdderSize;
						
#ifdef MAXSIZE
for (int aa=25; aa<=400; aa+=25){
	target->setFrequency(double(aa)*1000000.0);
#endif
			double z = getMaxInputDelays(inputDelays);
			double t = 1.0 / target->frequency();
			
			if (target->getVendor()=="Xilinx"){
				ll = (2.0/3.0)* ((t - 
									z - /* delay of input reg c to q + net delay */
									(target->lutDelay() + muxcytolacal) - //fist comparators
									(target->lutDelay() + muxcystoo) - //cgc
									(target->lutDelay() + muxcystoo + xordelay)-
									2*target->localWireDelay())/dcarry + 2);
//									3*target->lutDelay()-3*xordelay-3*muxcystoo-2*target->localWireDelay())/dcarry + 2);
			}else if (target->getVendor()=="Altera"){
				ll = 1;
				bool sol1 = false, sol2 = false;
				while (!sol1 || !sol2){
					double ed = target->localWireDelay() + target->eqComparatorDelay(ll) + target->localWireDelay() + target->lutDelay() + target->adderDelay(ll);
					if ((ed<t) && (!sol1))
						sol1 = true;
					if ((sol1) && (ed>=t))	
						sol2 = true;
					ll++;	
				}
			}else{
				cerr << "ERROR: Check your target FPGA" << endl;
				exit(-1);	
			}
			/* chunk 1 has the same size as ll */
			l1 = ll;
			double c = (target->eqComparatorDelay(l1) + target->lutDelay()); 
			if (t-c<=0) l0 = 2;
			else target->suggestSlackSubaddSize(l0, wIn, t-c);
			
			REPORT(INFO, "l0 ="<<l0<<"	l1="<<l1<<"	ll="<<ll);
			maxAdderSize =  l0+l1+ll*(ll+1)/2;
			REPORT(INFO, "Max adder size is="<< maxAdderSize);
#ifdef MAXSIZE
		int area = 2*maxAdderSize - l0 - 1 + (ll-1);
		cout << " f="<<aa<<" s="<<maxAdderSize<<" area="<<area<<endl;
}
exit(-1);
#endif			
			
			cSize = new int[1000];
			cSize[0]=l0;
			cSize[1]=l1;
			cSize[2]=ll;
			
			int td = wIn;
			td -= (l0+l1+ll);
			
			if ((td < 0) || (wIn>maxAdderSize)){
				cout << "OOOups ..." <<endl;
				exit(-1);
			}
			
			nbOfChunks = 3;
			while (td>0){
				int nc = cSize[nbOfChunks-1] -1;
				if (nc >= td){
					//finish
					cSize[nbOfChunks] = td;
					td = 0;
					nbOfChunks++;
				}else{
					cSize[nbOfChunks]= cSize[nbOfChunks-1]-1;
					td-=cSize[nbOfChunks];
					nbOfChunks++;
				}
			}
			
			for (int i=0; i<nbOfChunks; i++)
				REPORT(INFO, "cSize["<<i<<"]="<<cSize[i]);
			
//#define test512
#ifdef test512				
				nbOfChunks = 16;
				for (int i=1;i<=16;i++)
					cSize[i-1]=32;
		
#endif		
			
			//=================================================
			//split the inputs ( this should be reusable )
			vhdl << tab << "--split the inputs into chunks of bits depending on the frequency" << endl;
			for (int i=0;i<2;i++){
				for (int j=0; j<nbOfChunks; j++){
					ostringstream name;
					//the naming standard: sX j _ i _ l
					//j=the chunk index i is the input index and l is the current level
					name << "sX"<<j<<"_"<<i<<"_l"<<0;
					int low=0, high=0;
					for (int k=0;k<=j;k++)
						high+=cSize[k];
					for (int k=0;k<=j-1;k++)
						low+=cSize[k];
					if (j==0)
						vhdl << tab << declare (name.str(),cSize[j],true) << " <="<<(i==0?"X":"Y")<<range(high-1,low)<<";"<<endl;
					else
						vhdl << tab << declare (name.str(),cSize[j],true) << " <= "<<(i==0?"X":"Y")<<range(high-1,low)<<";"<<endl;
				}
			}	
			vhdl << tab << declare("scIn") << " <= Cin;"<<endl;
			
			int l=1;
			for (int j=0; j<nbOfChunks; j++){
				if (j>0){ //for all chunks greater than zero we perform this comparissons
					IntComparatorSpecific *icsz = new IntComparatorSpecific(target, cSize[j],0);
					oplist.push_back(icsz);
					
					inPortMap(icsz, "X", join("sX",j,"_0_l",l-1));
					inPortMap(icsz, "Y", join("sX",j,"_1_l",l-1));
					outPortMap(icsz, "R", join("coutX",j,"_0_l",l,"_Zero"));
					vhdl << tab << instance(icsz, join("cz",j));
					
					IntComparatorSpecific *icso = new IntComparatorSpecific(target, cSize[j],1);
					oplist.push_back(icso);
					
					inPortMap(icso, "X", join("sX",j,"_0_l",l-1));
					inPortMap(icso, "Y", join("sX",j,"_1_l",l-1));
					outPortMap(icso, "R", join("coutX",j,"_0_l",l,"_One"));
					vhdl << tab << instance(icso, join("co",j));
				}else{
					//for the zero chunk we directly perform the addition
					vhdl<<tab<< "-- the carry resulting from the addition of the chunk + Cin is obtained directly" << endl;
					IntAdderSpecific *fca = new IntAdderSpecific(target,cSize[0]);
					oplist.push_back(fca);

					inPortMap(fca, "X", join("sX",j,"_0_l",l-1) );
					inPortMap(fca, "Y", join("sX",j,"_1_l",l-1) );
					inPortMapCst(fca, "Cin", "Cin");
					outPortMap(fca, "R",    join("sX",j,"_0_l",l,"_Cin") );
					outPortMap(fca, "Cout", join("coutX",j,"_0_l",l,"_Cin") );
					vhdl << instance(fca, "firstAdder");
				}
			}
			//////////////////////////////////////////////////////
			vhdl << tab <<"--form the two carry string"<<endl;
			vhdl << tab << declare("carryStringZero",nbOfChunks-2) << " <= "; 
			for (int i=nbOfChunks-3; i>=0; i--) {
				vhdl << "coutX"<<i+1<<"_0_l"<<l<<"_Zero" << (i>0?" & ":";") ;
			} vhdl << endl;
			
			vhdl << tab << declare("carryStringOne",  nbOfChunks-2) << "  <= "; 
			for (int i=nbOfChunks-3; i>=0; i--) {
				vhdl << "coutX"<<i+1<<"_0_l"<<l<<"_One" << (i>0?" & ":";");
			} vhdl << endl;
			
			vhdl << tab << "--perform the short carry additions" << endl;
			//////////////////////////////////////////////////////
			vhdl << tab << "--perform the short CGC" << endl;
			CarryGenerationCircuit *cgc = new CarryGenerationCircuit(target,nbOfChunks-2);
			oplist.push_back(cgc);
	
			inPortMap(cgc, "X", "carryStringZero" );
			inPortMap(cgc, "Y", "carryStringOne" );
			inPortMapCst(cgc, "Cin", join("coutX",0,"_0_l",1,"_Cin"));
			outPortMap(cgc, "R",    "rawCarrySum" );
			vhdl << instance(cgc, "cgc");

			if (target->getVendor()== "Xilinx"){
				//////////////////////////////////////////////////////
				//perform the additions to recover the sum bits
				declare("rawCarrySum2",nbOfChunks-2);
				declare("p",nbOfChunks-2);
				declare("g",nbOfChunks-2);
			}

			for (int j=1;j<nbOfChunks;j++){
				if (target->getVendor()== "Xilinx"){
					if (j>1){
						vhdl << tab << "l"<<getNewUId()<<": LUT6_2 generic map ( INIT => X\"0000000000000002\")"<<endl;
						vhdl << tab << "port map( O6 => p("<<j-2<<"),"<<endl;
						vhdl << tab << "          O5 => g("<<j-2<<"),"<<endl;
						vhdl << tab << "          I0 => rawCarrySum("<<j-2<<"),"<<endl;
						vhdl << tab << "          I1 => '0',"<<endl;
						vhdl << tab << "          I2 => '0',"<<endl;
						vhdl << tab << "          I3 => '0',"<<endl;
						vhdl << tab << "          I4 => '0',"<<endl;
						vhdl << tab << "          I5 => '1');"<<endl; //fixed value
					
						vhdl << tab << "l"<<getNewUId()<<": MUXCY port map ("<<endl;
						vhdl << tab << "          O  => rawCarrySum2("<<j-2<<"), -- Carry local output signal"<<endl;
						vhdl << tab << "          CI => '1',  -- Carry input signal"<<endl;
						vhdl << tab << "          DI => g("<<j-2<<"), -- Data input signal"<<endl;
						vhdl << tab << "          S  => p("<<j-2<<")   -- MUX select, tie to '1' or LUT4 out"<<endl;
						vhdl << tab << ");"<<endl;
					}
				}		
				
				IntAdderSpecific *adder = new IntAdderSpecific(target, cSize[j]);
				oplist.push_back(adder);

				inPortMap(adder, "X", join("sX",j,"_0_l",l-1) );
				inPortMap(adder, "Y", join("sX",j,"_1_l",l-1) );
				if (j==1)
					inPortMapCst(adder, "Cin", join("coutX",0,"_0_l",1,"_Cin"));
				else{
					if (target->getVendor()== "Xilinx")
						inPortMapCst(adder, "Cin", "rawCarrySum2"+of(j-2));
					else
						inPortMapCst(adder, "Cin", "rawCarrySum"+of(j-2));
				}
				outPortMap(adder, "R",    join("sX",j,"_0_l",l+1));
				outPortMap(adder, "Cout", join("coutX",j,"_0_l",l+1) ); //this one will get discarded
				vhdl << instance(adder, join("adder",j) );
			}
				
			vhdl << tab <<"--get the final pipe results"<<endl;
			for ( int i=0; i<nbOfChunks; i++){
				if (i==0) 
					vhdl << tab << declare(join("res",i),cSize[i],true) << " <= sX0_0_l1_Cin;" << endl;
				else 
					vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << join("sX",i,"_0_l",l+1) << ";" << endl;
			}
			/////////////////////////////////////////////////////////		
			vhdl << tab << "R <= ";
			int k=0;
			for (int i=nbOfChunks-1; i>=0; i--){
				vhdl << use(join("res",i));
				if (i > 0) vhdl << " & ";
				k++;
			}
			vhdl << ";" <<endl;
		}else
			vhdl << tab << " R <= X + Y + Cin;"<<endl;
	}

	LongIntAdderCmpCmpAddGen2::~LongIntAdderCmpCmpAddGen2() {
	}

	void LongIntAdderCmpCmpAddGen2::outputVHDL(std::ostream& o, std::string name) {
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


	void LongIntAdderCmpCmpAddGen2::emulate(TestCase* tc)
	{
		mpz_class svX,svY,svC;
		svX = tc->getInputValue("X");
		svY = tc->getInputValue("Y");
		svC = tc->getInputValue("Cin");

		mpz_class svR = svX + svY + svC;
		mpz_clrbit(svR.get_mpz_t(),wIn_); 

		tc->addExpectedOutput("R", svR);
	}
}
