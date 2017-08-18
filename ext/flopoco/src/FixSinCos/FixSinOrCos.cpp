#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SOLLYA
#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"

#include "FixSinOrCos.hpp"

using namespace std;

namespace flopoco{


	FixSinOrCos::FixSinOrCos(Target* target, int w_, int degree_, map<string, double> inputDelays) 
		: Operator(target), w(w_), degree(degree_)
	{

		srcFileName="FixSinOrCos";
		ostringstream name;

		setCopyrightString ( "Matei Istoan, Florent de Dinechin (2008-2012)" );
		if(target->isPipelined())
			name << "FixSinOrCos_" << w <<"_f"<< target->frequencyMHz() << "_uid" << getNewUId();
		else
			name << "FixSinOrCos_" << w << "_uid" << getNewUId();
		setName( name.str() );

		// declaring inputs
		addInput("X",1+w,true);
		addInput("SinCosbar");		// 0 for cosine 1 for sine

		// declaring output
		addOutput("SorC",1+w,2);

		// TODO free scale in the destructor
		mpfr_init2(scale, 10*w);
		mpfr_set_d(scale, -1.0, GMP_RNDN);
		mpfr_mul_2si(scale, scale, -w, GMP_RNDN);
		mpfr_add_d(scale, scale, 1.0, GMP_RNDN);

		
		//reduce the argument X to [0, 1/2)
		vhdl << tab << declare("absX", 1+w ) << "<= (X xor (" << w << " downto 0 => X(" << w << ")))"
											<< " + " 
											<< "(" << zg(w, 0) << " & X(" << w << "));"<< endl;
		vhdl << tab << declare("reducedX", 1+w) 
						<< "<= (absX(" << w << " downto " << w-1 << ") - (\'0\' & (absX(" << w << ") xor absX(" << w-1 << "))))" 
						<< " & absX(" << w-2 << " downto 0);" << endl;
		vhdl << tab << declare("s") << " <= X(" << w << ");  -- sign" << endl;
		vhdl << tab << declare("q") << " <= X(" << w-1 << ");  -- quadrant" << endl;
		vhdl << tab << declare("sq", 2) << " <= X(" << w << " downto " << w-1 << ");  -- sign and quadrant" << endl;
		
		vhdl << tab << declare("XinSin", 1+w) << " <= reducedX;" << endl; 
		vhdl << tab << declare("XinCos", 1+w) << " <= (\"01\" & " << zg(w-1) << ") - reducedX;" << endl; 
		
		//remove the sign bit (msb)
		vhdl << tab << declare("shortXinSin", w) << " <= XinSin(" << w-1 << " downto 0);" << endl; 
		vhdl << tab << declare("shortXinCos", w) << " <= XinCos(" << w-1 << " downto 0);" << endl; 
		
		vhdl << tab << declare("effectiveSin") << " <= s xor q xor SinCosbar;" << endl; 

		vhdl << tab << declare("shortXin", w) << " <= shortXinSin when effectiveSin = '1' else shortXinCos;" << endl; 

		nextCycle();
		//compute the sine and the cosine
		ostringstream fun;
		fun << "(1-1b-"<<w<<")*sin(x*Pi),0,1,1";
		FunctionEvaluator* sinCosEvaluator = new FunctionEvaluator(target, fun.str(), w, w, degree);
		oplist.push_back(sinCosEvaluator);
		
		inPortMap(sinCosEvaluator, "X", "shortXin");
		outPortMap(sinCosEvaluator, "R", "intSinCos");
		vhdl << instance(sinCosEvaluator, "sinEvaluator") << endl;

		syncCycleFromSignal("intSinCos");
		nextCycle();

		
		//extract the needed bits from the function output
		vhdl << tab << declare("shortIntSinCos", 1+w) << " <= intSinCos(" << w << " downto 0);" << endl; 
		
		//assign the correct value to the output
		vhdl << tab << declare("reducedSC", 1+w) << "<= shortIntSinCos;" << endl;
		
		vhdl << tab << declare("negReducedSC", 1+w) << "<= " << zg(w, 0) << " - reducedSC;"<< endl;
		
		vhdl << tab << declare("changeSign") << " <= s xor (q and not SinCosbar);" << endl; 
		vhdl << tab << declare("finalSC", 1+w) << "<=  reducedSC when changeSign = '0' else negReducedSC;" << endl;
		
		vhdl << tab << "SorC <= finalSC;" << endl;
		
	};



	 FixSinOrCos::~FixSinOrCos(){
		mpfr_clear(scale);
	 };

	void FixSinOrCos::emulate(TestCase * tc) 
	{
		/* Get I/O values */
		mpz_class svZ = tc->getInputValue("X");
		mpz_class svSelect = tc->getInputValue("SinCosbar");
		mpfr_t z, constPi, sinorcos;
		mpz_t sinorcos_z;
		
		/* Compute correct value */
		mpfr_init2(z, 10*w);
		
		mpfr_init2(constPi, 10*w);
				
		mpfr_set_z (z, svZ.get_mpz_t(), GMP_RNDN); // this rounding is exact
		mpfr_div_2si (z, z, w, GMP_RNDN); // this rounding is acually exact
		
		mpfr_const_pi( constPi, GMP_RNDN);
		mpfr_mul(z, z, constPi, GMP_RNDN);
		
		mpfr_init2(sinorcos, 10*w); 

		mpz_init2 (sinorcos_z, 2*w);


		if(svSelect==0)
			mpfr_cos(sinorcos, z, GMP_RNDN);
		else
			mpfr_sin(sinorcos, z, GMP_RNDN);


		mpfr_mul (sinorcos, sinorcos, scale, GMP_RNDN);
		mpfr_add_d(sinorcos, sinorcos, 6.0, GMP_RNDN);
		mpfr_mul_2si (sinorcos, sinorcos, w, GMP_RNDN);

		// Rounding down
		mpfr_get_z (sinorcos_z, sinorcos, GMP_RNDD); // Here is where the only rounding takes place
		mpz_class sinorcos_zc (sinorcos_z);
		sinorcos_zc -= mpz_class(6)<<w;
		tc->addExpectedOutput ("SorC", sinorcos_zc);
		
		// Rounding up
		mpfr_get_z (sinorcos_z, sinorcos, GMP_RNDU); // Here is where the only rounding takes place
		mpz_class sinorcos_zcu (sinorcos_z);
		sinorcos_zcu -= mpz_class(6)<<w;
		tc->addExpectedOutput ("SorC", sinorcos_zcu);
		
		// clean up
		mpfr_clears (z, sinorcos, NULL);		
		mpfr_free_cache();
	}


	void FixSinOrCos::buildStandardTestCases(TestCaseList * tcl) 
	{
		TestCase* tc;
		mpf_t zinit;
		mpfr_t z;
		mpz_t z_z;
		
		//mpf_set_default_prec (1+wI+wF+guard);
		
		mpfr_init2(z, 1+w+ceil(log2(1 + w)));
		mpz_init2 (z_z, 1+w+ceil(log2(1 + w)));
		
		//z=0
		tc = new TestCase (this);
		tc -> addInput ("X",mpz_class(0));
		tc -> addInput ("SinCosbar",mpz_class(1));
		emulate(tc);
		tcl->add(tc);
		
		//z=pi/2
		tc = new TestCase (this);
		
		mpf_init2   (zinit, 1+w+ceil(log2(1 + w)));
		//mpf_set_str (zinit, "1.5707963267949e0", 10);
		mpf_set_str (zinit, "0.5e0", 10);
		mpfr_set_f (z, zinit, GMP_RNDD); 
		
		mpfr_mul_2si (z, z, w, GMP_RNDD); 
		mpfr_get_z (z_z, z, GMP_RNDD);  
		tc -> addInput ("X",mpz_class(z_z));
		tc -> addInput ("SinCosbar",mpz_class(1));
		emulate(tc);
		tcl->add(tc);
		
		//z=pi/6
		tc = new TestCase (this); 
		
		mpf_init2   (zinit, 1+w+ceil(log2(1 + w)));
		//mpf_set_str (zinit, "0.5235987755983e0", 10);
		mpf_set_str (zinit, "0.16666666666666e0", 10);
		mpfr_set_f (z, zinit, GMP_RNDD); 
		
		mpfr_mul_2si (z, z, w, GMP_RNDD); 
		mpfr_get_z (z_z, z, GMP_RNDD);  
		tc -> addInput ("X",mpz_class(z_z));
		tc -> addInput ("SinCosbar",mpz_class(1));
		emulate(tc);
		tcl->add(tc);
		
		//z=pi/4
		tc = new TestCase (this);
		
		mpf_init2   (zinit, 1+w+ceil(log2(1 + w)));
		//mpf_set_str (zinit, "0.78539816339745e0", 10);
		mpf_set_str (zinit, "0.25e0", 10);
		mpfr_set_f (z, zinit, GMP_RNDD); 
		
		mpfr_mul_2si (z, z, w, GMP_RNDD); 
		mpfr_get_z (z_z, z, GMP_RNDD);  
		tc -> addInput ("X",mpz_class(z_z));
		tc -> addInput ("SinCosbar",mpz_class(1));
		emulate(tc);
		tcl->add(tc);
		
		//z=pi/3
		tc = new TestCase (this);
		
		mpf_init2   (zinit, 1+w+ceil(log2(1 + w)));
		//mpf_set_str (zinit, "1.0471975511966e0", 10);
		mpf_set_str (zinit, "0.33333333333333e0", 10);
		mpfr_set_f (z, zinit, GMP_RNDD);
		
		mpfr_mul_2si (z, z, w, GMP_RNDD); 
		mpfr_get_z (z_z, z, GMP_RNDD);  
		
		tc -> addInput ("X",mpz_class(z_z));
		tc -> addInput ("SinCosbar",mpz_class(1));
		emulate(tc);
		tcl->add(tc);
		
		mpfr_clears (z, NULL);
	}

}
#endif //HAVE_SOLLYA	
