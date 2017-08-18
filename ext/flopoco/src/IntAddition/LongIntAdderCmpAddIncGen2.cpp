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
#include "LongIntAdderCmpAddIncGen2.hpp"
#include "../IntAdder.hpp"
#include "../IntComparator.hpp"
	

using namespace std;
namespace flopoco{

	LongIntAdderCmpAddIncGen2::LongIntAdderCmpAddIncGen2(Target* target, int wIn, map<string, double> inputDelays):
		Operator(target), wIn_(wIn), inputDelays_(inputDelays) 
	{
		srcFileName="LongIntAdderCmpAddIncGen2";
		setName(join("LongIntAdderCmpAddIncGen2_", wIn_));
		
		// Set up the IO signals
		addInput ( "X" , wIn_);
		addInput ( "Y" , wIn_);
		addInput( "Cin");
		addOutput( "R"  , wIn_);


		//compute the maximum input delay
		maxInputDelay = getMaxInputDelays(inputDelays);
			
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
		}else if (target->getID()=="Virtex4"){
			xordelay = 0.273e-9;
			dcarry = 0.034e-9;
			muxcystoo = 0.278e-9;
			muxcytolacal = 0.273e-9;
		}
			
		int ll;
		int l1;
		int l0;
		int maxAdderSize;
		double z = getMaxInputDelays(inputDelays);
			
#ifdef MAXSIZE
for (int aa=25; aa<=400; aa+=25){
	target->setFrequency(double(aa)*1000000.0);

#endif
		double t = 1.0 / target->frequency();				

		if (target->getVendor()=="Xilinx"){
			ll = (1.0/2.0)* ((t - 
							  z - //the register c->q delay + net delay from register
							  2*(target->lutDelay() + muxcystoo + muxcytolacal) - 
							  (target->lutDelay() + muxcystoo + xordelay) - 
							  2*target->localWireDelay())/dcarry + 2);
			if (ll<=1){
				cerr << "WARNING: The adder does not seem to meet the required frequency constraints"<<endl;
				ll = 2;
			}	
		}else if (target->getVendor()=="Altera"){
			ll = 1;
			bool sol1 = false, sol2 = false;
			while (!sol1 || !sol2){
				double ed = target->localWireDelay() + target->adderDelay(ll) + target->localWireDelay() + target->lutDelay() + target->adderDelay(ll);
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
		l1 = ll-1;
		target->suggestSlackSubaddSize(l0, wIn, t - (target->lutDelay()+ target->adderDelay(l1)) );

		maxAdderSize = l0+l1+ll*(ll+1)/2;
		REPORT(INFO, "l0 ="<<l0<<"	l1="<<l1<<"	ll="<<ll);
		REPORT(INFO, "The maximum adder size is="<< maxAdderSize);

#ifdef MAXSIZE
		cout << " Frequency="<< aa <<" Width="<<maxAdderSize<<endl;
}
exit(-1);
#endif			

		cSize = new int[1000];
		cSize[0]=l0; cSize[1]=l1; cSize[2]=ll;
		nbOfChunks = 3;
		bool solution = false;
		
		if (l0 + l1 + ll > wIn){
			/* these chunks are too big for this small addition*/
			int turn =0;
			while (!solution){
				if (cSize[turn]>1) 
					cSize[turn]--;
				turn = (turn==2?0:turn+1);
				if ((cSize[0]==1)&&(cSize[1]==1)&&(cSize[2]==1)){
					/* no solution can be found, the adder is really small */
					vhdl << tab << "R <= X + Y + Cin;"<<endl;
					return; //finish the execution of the constructor
				}
				if (cSize[0]+cSize[1]+cSize[2]==wIn)
					solution = true;
			}
		}
		
		if (wIn>maxAdderSize){
			cerr << "WARNING: the "<<wIn<<"-bit adder is too large for frequency " << target->frequencyMHz() << endl;
		}

		if(!solution){ //it did not get split into 3 chunks or one simple addition
			int td = wIn - (l0 + l1 + ll);
			while (td > 0){
				int nc = cSize[nbOfChunks-1] -1;
				if (nc >= td){ //finish
					cSize[nbOfChunks] = td; td = 0; nbOfChunks++;
				}else{
					cSize[nbOfChunks]= cSize[nbOfChunks-1]-1; td-=cSize[nbOfChunks]; nbOfChunks++;
				}
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
		/////////////////////////////////////////////////////////////		
		//split the inputs ( this should be reusable )
		vhdl << tab << "--split the inputs into chunks of bits depending on the frequency" << endl;
		for (int i=0;i<2;i++){
			for (int j=0; j<nbOfChunks; j++){
				ostringstream name; //the naming standard: sX j_i_l: j=the chunk index, i=input index, l=current level
				name << "sX"<<j<<"_"<<i<<"_l"<<0;
				int low=0, high=0;
				for (int k=0;k<=j;k++)   high+=cSize[k];
				for (int k=0;k<=j-1;k++) low+=cSize[k];
				vhdl << tab << declare (name.str(),cSize[j],true) << " <= "<<(i?"Y":"X")<<range(high-1,low)<<";"<<endl;
			}
		}	
		
		int l=1;
		for (int j=0; j<nbOfChunks; j++){
			if (j>0){ //for all chunks greater than zero we perform this comparissons
				IntAdderSpecific *acsz = new IntAdderSpecific(target, cSize[j]);
				oplist.push_back(acsz);
				
				inPortMap(acsz, "X", join("sX",j,"_0_l",l-1));
				inPortMap(acsz, "Y", join("sX",j,"_1_l",l-1));
				inPortMapCst(acsz, "Cin", "'0'");
				outPortMap(acsz, "R", join("sX",j,"_0_l",l,"_Zero"));
				outPortMap(acsz, "Cout", join("coutX",j,"_0_l",l,"_Zero"));
				vhdl << tab << instance(acsz, join("addercz",j));
				
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
		vhdl << tab << declare("carryStringZero",nbOfChunks-2,true) << " <= "; 
		for (int i=nbOfChunks-3; i>=0; i--) {
			vhdl << "coutX"<<i+1<<"_0_l"<<l<<"_Zero" << (i>0?" & ":" & \"\";") ;
		} vhdl << endl;
		
		vhdl << tab << declare("carryStringOne",  nbOfChunks-2,true) << "  <= "; 
		for (int i=nbOfChunks-3; i>=0; i--) {
			vhdl << "coutX"<<i+1<<"_0_l"<<l<<"_One" << (i>0?" & ":"& \"\";");
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

		//////////////////////////////////////////////////////
		//perform the additions to recover the sum bits
		if (target->getVendor()== "Xilinx"){
			declare("rawCarrySum2",nbOfChunks-2,true);
			declare("p",nbOfChunks-2,true);
			declare("g",nbOfChunks-2,true);
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

			inPortMap(adder, "X", join("sX",j,"_0_l",l,"_Zero") );
			inPortMapCst(adder, "Y", zg(cSize[j]) );
			if (j==1)
				inPortMapCst(adder, "Cin", join("coutX",0,"_0_l",1,"_Cin"));
			else
				if (target->getVendor()== "Xilinx")
					inPortMapCst(adder, "Cin", "rawCarrySum2"+of(j-2));
				else if (target->getVendor()== "Altera")
					inPortMapCst(adder, "Cin", "rawCarrySum"+of(j-2));

			outPortMap(adder, "R",    join("sX",j,"_0_l",l+1));
			outPortMap(adder, "Cout", join("coutX",j,"_0_l",l+1) ); //this one will get discarded
			vhdl << instance(adder, join("adder",j) );
		}
			
		vhdl << tab <<"--get the final pipe results"<<endl;
		for ( int i=0; i<nbOfChunks; i++){
			if (i==0) vhdl << tab << declare(join("res",i),cSize[i],true) << " <= sX0_0_l1_Cin;" << endl;
			else      vhdl << tab << declare(join("res",i),cSize[i],true) << " <= " << join("sX",i,"_0_l",l+1) << ";" << endl;
		}

		/////////////////////////////////////////////////////////		
		vhdl << tab << "R <= ";
		for (int i=nbOfChunks-1, k=0; i>=0; i--){
			vhdl << use(join("res",i));
			if (i > 0) vhdl << " & ";
			k++;
		} vhdl << ";" <<endl;
	}

	LongIntAdderCmpAddIncGen2::~LongIntAdderCmpAddIncGen2() {
	}

	void LongIntAdderCmpAddIncGen2::outputVHDL(std::ostream& o, std::string name) {
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
		}else{
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


	void LongIntAdderCmpAddIncGen2::emulate(TestCase* tc)
	{
		mpz_class svX[2];
		svX[0] = tc->getInputValue("X");
		svX[1] = tc->getInputValue("Y");

		mpz_class svC =  tc->getInputValue("Cin");

		mpz_class svR = svX[0] + svX[1] + svC;
		mpz_clrbit(svR.get_mpz_t(),wIn_); 
		tc->addExpectedOutput("R", svR);
	}
}
