/*
  Euclidean division by a small constant

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Author : Florent de Dinechin, Florent.de.Dinechin@ens-lyon.fr

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2011.
  All rights reserved.

*/

// TODOs: remove from d its powers of two .

#include <iostream>
#include <fstream>
#include <sstream>

#include "IntConstDiv.hpp"


using namespace std;


namespace flopoco{

	IntConstDiv::EuclideanDivTable::EuclideanDivTable(Target* target, int d_, int alpha_, int gamma_) :
		Table(target, alpha_+gamma_, alpha_+gamma_, 0, -1, 1), d(d_), alpha(alpha_), gamma(gamma_) {
				ostringstream name; 
				srcFileName="IntConstDiv::EuclideanDivTable";
				name <<"EuclideanDivTable_" << d << "_" << alpha ;
				setName(name.str());
			};


	mpz_class IntConstDiv::EuclideanDivTable::function(int x){
		// machine integer arithmetic should be safe here
		if (x<0 || x>=(1<<(alpha+gamma))){
			ostringstream e;
			e << "ERROR in IntConstDiv::EuclideanDivTable::function, argument out of range" <<endl;
			throw e.str();
		}
		
		int q = x/d;
		int r = x-q*d;
		return mpz_class((q<<gamma) + r);
	};

	int IntConstDiv::quotientSize() {return qSize; };

	int IntConstDiv::remainderSize() {return gamma; };




	IntConstDiv::IntConstDiv(Target* target, int wIn_, int d_, int alpha_,  bool remainderOnly_, map<string, double> inputDelays)
		: Operator(target), d(d_), wIn(wIn_), alpha(alpha_), remainderOnly(remainderOnly_)
	{
		setCopyrightString("F. de Dinechin (2011)");
		srcFileName="IntConstDiv";

		gamma = intlog2(d-1);
		if(alpha==-1){
			alpha = target->lutInputs()-gamma;
			if (alpha<1) {
				REPORT(LIST, "WARNING: This value of d is too large for the LUTs of this FPGA (alpha="<<alpha<<").");
				REPORT(LIST, " Building an architecture nevertheless, but it may be very large and inefficient.");
				alpha=1;
			}
		}
		REPORT(INFO, "alpha="<<alpha);

		if((d&1)==0)
			REPORT(LIST, "WARNING, d=" << d << " is even, this is suspiscious. Might work nevertheless, but surely suboptimal.")

		/* Generate unique name */
		
		std::ostringstream o;
		if(remainderOnly)
			o << "IntConstRem_";
		else
			o << "IntConstDiv_";
		o << d << "_" << wIn << "_"  << alpha << "_" ;
		if(target->isPipelined()) 
				o << target->frequencyMHz() ;
		else
			o << "comb";
		uniqueName_ = o.str();

		qSize = wIn - intlog2(d) +1;  


		addInput("X", wIn);

		if(!remainderOnly)
			addOutput("Q", qSize);
		addOutput("R", gamma);

		int k = wIn/alpha;
		int rem = wIn-k*alpha;
		if (rem!=0) k++;

		REPORT(INFO, "Architecture consists of k=" << k  <<  " levels."   );
		REPORT(DEBUG, "  d=" << d << "  wIn=" << wIn << "  alpha=" << alpha << "  gamma=" << gamma <<  "  k=" << k  <<  "  qSize=" << qSize );
		
		EuclideanDivTable* table;
		table = new EuclideanDivTable(target, d, alpha, gamma);
		useSoftRAM(table);
		oplist.push_back(table);
		double tableDelay=table->getOutputDelay("Y"); 

		string ri, xi, ini, outi, qi;
		ri = join("r", k);
		vhdl << tab << declare(ri, gamma) << " <= " << zg(gamma, 0) << ";" << endl;

		setCriticalPath( getMaxInputDelays(inputDelays) );

		for (int i=k-1; i>=0; i--) {

			manageCriticalPath(tableDelay);

			//			cerr << i << endl;
			xi = join("x", i);
			if(i==k-1 && rem!=0) // at the MSB, pad with 0es
				vhdl << tab << declare(xi, alpha, true) << " <= " << zg(alpha-rem, 0) <<  " & X" << range(wIn-1, i*alpha) << ";" << endl; 
			else // normal case
				vhdl << tab << declare(xi, alpha, true) << " <= " << "X" << range((i+1)*alpha-1, i*alpha) << ";" << endl; 
			ini = join("in", i);
			vhdl << tab << declare(ini, alpha+gamma) << " <= " << ri << " & " << xi << ";" << endl; // This ri is r_{i+1}
			outi = join("out", i);
			outPortMap(table, "Y", outi);
			inPortMap(table, "X", ini);


			vhdl << instance(table, join("table",i));
			ri = join("r", i);
			qi = join("q", i);
			vhdl << tab << declare(qi, alpha, true) << " <= " << outi << range(alpha+gamma-1, gamma) << ";" << endl; 
			vhdl << tab << declare(ri, gamma) << " <= " << outi << range(gamma-1, 0) << ";" << endl; 
		}


		if(!remainderOnly) { // build the quotient output
		vhdl << tab << declare("tempQ", k*alpha) << " <= " ;
		for (unsigned int i=k-1; i>=1; i--) 
			vhdl << "q" << i << " & ";
		vhdl << "q0 ;" << endl; 
		vhdl << tab << "Q <= tempQ" << range(qSize-1, 0)  << ";" << endl;
		}		

		vhdl << tab << "R <= " << ri << ";" << endl; // This ri is r_0
		
	}	

	IntConstDiv::~IntConstDiv()
	{
	}



	void IntConstDiv::emulate(TestCase * tc)
	{
		/* Get I/O values */
		mpz_class X = tc->getInputValue("X");
		/* Compute correct value */
		mpz_class Q = X/d;
		mpz_class R = X-Q*d;
		if(!remainderOnly)
			tc->addExpectedOutput("Q", Q);
		tc->addExpectedOutput("R", R);
	}
 

}
