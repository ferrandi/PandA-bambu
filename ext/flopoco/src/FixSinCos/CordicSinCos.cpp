#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"

#include "CordicSinCos.hpp"

using namespace std;

namespace flopoco{


	//The wIn+1 below is for consistency with FixSinCos and FixSinOrCos interfaces.
	// TODO possibly fix all the code instead... This would enable sharing emulate() etc.
 
	CordicSinCos::CordicSinCos(Target* target, int wIn_, int wOut_, int reducedIterations_, map<string, double> inputDelays) 
		: Operator(target), wIn(wIn_+1), wOut(wOut_+1), reducedIterations(reducedIterations_)
	{

		int stage;
		srcFileName="CordicSinCos";
		setCopyrightString ( "Matei Istoan, Florent de Dinechin (2012-...)" );

		ostringstream name;
		name << "CordicSinCos_" << (reducedIterations==1?"reducedIterations":"") << wIn_ << "_" << wOut_;
		if(target->isPipelined())
			name  <<"_f" << target->frequencyMHz();
		else 
			name << "_comb";
		name << "_uid" << getNewUId();
		setName( name.str() );

		if(wIn<12){
			REPORT(INFO, "wIn is small, are you sure you don't want to tabulate this operator in a ROM?");
		}

		if (reducedIterations == 1)
			maxIterations=(wOut>>1)+1;
		else
			maxIterations = wOut+1;
		
#define ROUNDED_ROTATION 1 // 0:trunc 

#if ROUNDED_ROTATION
		REPORT(DEBUG, "Using rounded rotation trick");
#endif

		//error analysis
		double eps;  //error in ulp
		eps=0.5; //initial rounding of kfactor
		double shift=0.5;
		for(stage=1; stage<=maxIterations; stage++){
#if ROUNDED_ROTATION
			eps = eps + eps*shift + 0.5; // 0.5 assume rounding in the rotation.
#else
			eps = eps + eps*shift + 1.0; // 1.0 assume truncation in the rotation.
#endif
			shift *=0.5;
		}

		if (reducedIterations == 1) {
			eps+=2; // two multiplications in sequence, each truncated
		}

		eps+=1; // the final neg-by-not
		REPORT(DEBUG, "Error analysis computes eps=" << eps << " ulps (before final rounding)");

		// guard bits depend only on the number of iterations
		g = 1+(int) ceil(log2(eps)); // +1 for the final rounding 

		
		// *********    internal precision and fixed-point alignment **************

 		// The input is as follows:
		// s has weight 2^0
		// q has weight 2^-1
		// o has weight 2^-2
		// Purpose: have the LSB of z, cosine, sine at weight -w
		// This means that the Cos and Sin datapath will have w+1 bits
		//   (sign at weight zero)
		// while the Z datapath starts on w-1 bits (sign bit at weight  -2,
		//  and decreasing, so the invariant is: sign bit at weight -stage-1)

		w = wOut-1 + g; // -1 because of sign

		REPORT(DEBUG, "wIn=" << wIn << " wOut=" << wOut 
		       << "   MaxIterations: " << maxIterations 
		       << "  Guard bits g=" << g << "  Neg. weight of LSBs w=" << w );


		// everybody needs many digits of Pi
		mpfr_init2(constPi, 10*w);
		mpfr_const_pi( constPi, GMP_RNDN);

		//compute the scale factor		
		mpfr_init2(scale, wOut+2);
		mpfr_set_d(scale, -1.0, GMP_RNDN);           // exact
		mpfr_mul_2si(scale, scale, -wOut+1, GMP_RNDN); // exact
		mpfr_add_d(scale, scale, 1.0, GMP_RNDN);     // exact
		REPORT(DEBUG, "scale=" << printMPFR(scale, 15));
		

		// declaring inputs
		addInput  ( "X"  , wIn, true );

		// declaring output
		addOutput  ( "C"  , wOut, 2 );
		addOutput  ( "S"  , wOut, 2 );
		
		setCriticalPath(getMaxInputDelays(inputDelays));
		manageCriticalPath( target->lutDelay());
		
		//reduce the argument X to [0, 1/2)
		vhdl << tab << declare("sgn") << " <= X(" << wIn-1 << ");  -- sign" << endl;
		vhdl << tab << declare("q") << " <= X(" << wIn-2 << ");  -- quadrant" << endl;
		vhdl << tab << declare("o") << " <= X(" << wIn-3 << ");  -- octant" << endl;
		vhdl << tab << declare("sqo", 3) << " <= sgn & q & o;  -- sign, quadrant, octant" << endl;

		vhdl << tab << declare("qrot0", 3) << " <= sqo +  \"001\"; -- rotate by an octant" << endl; 
		vhdl << tab << declare("qrot", 2) << " <= qrot0(2 downto 1); -- new quadrant: 00 is the two octants around the origin" << endl; 
		// y is built out of the remaining wIn-2 bits



		int zMSB=-2;   // -2 'cause we live in a quadrant, initial angle is in -0.25pi, 0.25pi
		int zLSB=-w-1; // better have a bit more accuracy here, it seems 
		// extract the bits below the octant bit, the new sign will be at the weight of the octant bit
		int sizeZ =  zMSB - zLSB +1;
 		int sizeY=wIn-2;
		vhdl << tab << declare("Yp", sizeZ ) << "<= " ;
		if(sizeZ >= sizeY) {
			vhdl << "X" << range(sizeY-1,0); // sizeY-1 = wIn-4,  i.e. MSB = -3
			if(sizeZ > sizeY)
				vhdl << " & " << zg(sizeZ-sizeY) << ";" << endl;
		}
		else // sizeZ < sizeY
			vhdl << "X" << range(sizeY-1, sizeY-sizeZ) <<";" << endl;

		vhdl << tab << "--  This Yp is in -pi/4, pi/4. Now start CORDIC with angle atan(1/2)" << endl;

		//create the C1, S1, X1 and D1 signals for the first stage
		mpfr_t temp, zatan;

 		mpfr_init2(kfactor, 10*w);
		mpfr_init2(temp, 10*w);
		mpfr_set_d(kfactor, 1.0, GMP_RNDN);
		for(int i=1; i<=maxIterations; i++){
			mpfr_set_d(temp, 1.0, GMP_RNDN);
			mpfr_div_2si(temp, temp, 2*i, GMP_RNDN);
			mpfr_add_d(temp, temp, 1.0, GMP_RNDN);
			
			mpfr_mul(kfactor, kfactor, temp, GMP_RNDN);
		}
		mpfr_sqrt(kfactor, kfactor, GMP_RNDN);
		mpfr_d_div(kfactor, 1.0, kfactor, GMP_RNDN);

		mpfr_mul(kfactor, kfactor, scale, GMP_RNDN);
		
		REPORT(DEBUG, "kfactor=" << printMPFR(kfactor, 15));
		mpfr_clear(temp);
		
		// initialize the zatan mpfr. It will be cleared outside the loop
		mpfr_init2(zatan, 10*w);
			


		vhdl << tab << declare("Cos1", w+1) << " <= " <<  unsignedFixPointNumber(kfactor, 0, -w) << ";" 
		     << "-- scale factor, about " << printMPFR(kfactor, 15) << endl;
		vhdl << tab << declare("Sin1", w+1) << " <= " << zg(w+1) << ";" << endl;
		vhdl << tab << declare("Z1", sizeZ) << "<= Yp;" << endl;
		vhdl << tab << declare("D1") << "<= Yp" << of(sizeZ-1) << ";" << endl;
		
				
		//create the stages of micro-rotations

		//build the cordic stages
		for(stage=1; stage<=maxIterations; stage++){
			//shift Xin and Yin with 2^n positions to the right
			// Cosine is always positive, but sine may be negative and thus need sign extend
			vhdl << tab << declare(join("CosShift", stage), w+1) << " <= " << zg(stage) << " & Cos" << stage << range(w, stage) << ";" <<endl;
			
			vhdl << tab << declare(join("sgnSin", stage))  << " <= " <<  join("Sin", stage)  <<  of(w) << ";" << endl; 
			vhdl << tab << declare(join("SinShift", stage), w+1) 
			     << " <= " << rangeAssign(w, w+1-stage, join("sgnSin", stage))   
			     << " & Sin" << stage << range(w, stage) << ";" << endl;
			
			// Critical path delay for one stage:
			// The data dependency is from one Z to the next 
			// We may assume that the rotations themselves overlap once the DI are known
			//manageCriticalPath(target->localWireDelay(w) + target->adderDelay(w) + target->lutDelay()));
 			manageCriticalPath(target->localWireDelay(sizeZ) + target->adderDelay(sizeZ));
			
#if ROUNDED_ROTATION  // rounding of the shifted operand, should save 1 bit in each addition
			
#if 1 // this if just to check that it is useful
			vhdl << tab << declare(join("CosShiftRoundBit", stage)) << " <= " << join("Cos", stage)  << of(stage-1) << ";" << endl;
			vhdl << tab << declare(join("SinShiftRoundBit", stage)) << " <= " << join("Sin", stage) << of(stage-1) << ";" <<endl;
#else
			vhdl << tab << declare(join("CosShiftRoundBit", stage)) << " <= '0';" << endl;
			vhdl << tab << declare(join("SinShiftRoundBit", stage)) << " <= '0';" <<endl;
#endif
			vhdl << tab << declare(join("CosShiftNeg", stage), w+1) << " <= " << rangeAssign(w, 0, join("D", stage)) << " xor " << join("CosShift", stage)   << " ;" << endl;
			vhdl << tab << declare(join("SinShiftNeg", stage), w+1) << " <= (not " << rangeAssign(w, 0, join("D", stage)) << ") xor " << join("SinShift", stage)   << " ;" << endl;

			vhdl << tab << declare(join("Cos", stage+1), w+1) << " <= " 
			     << join("Cos", stage) << " + " << join("SinShiftNeg", stage) << " +  not (" << join("D", stage) << " xor " << join("SinShiftRoundBit", stage) << ") ;" << endl;

			vhdl << tab << declare(join("Sin", stage+1), w+1) << " <= " 
			     << join("Sin", stage) << " + " << join("CosShiftNeg", stage) << " + (" << join("D", stage) << " xor " << join("CosShiftRoundBit", stage) << ") ;" << endl;

#else
// truncation of the shifted operand
			vhdl << tab << declare(join("Cos", stage+1), w+1) << " <= " 
			     << join("Cos", stage) << " - " << join("SinShift", stage) << " when " << join("D", stage) << "=\'0\' else "
			     << join("Cos", stage) << " + " << join("SinShift", stage) << " ;" << endl;

			vhdl << tab << declare(join("Sin", stage+1), w+1) << " <= " 
			     << join("Sin", stage) << " + " << join("CosShift", stage) << " when " << join("D", stage) << "=\'0\' else "
			     << join("Sin", stage) << " - " << join("CosShift", stage) << " ;" << endl;

#endif			
			
			//create the constant signal for the arctan	
			mpfr_set_d(zatan, 1.0, GMP_RNDN);
			mpfr_div_2si(zatan, zatan, stage, GMP_RNDN);
			mpfr_atan(zatan, zatan, GMP_RNDN);
			mpfr_div(zatan, zatan, constPi, GMP_RNDN);
			REPORT(DEBUG, "stage=" << stage << "  atancst=" << printMPFR(zatan, 15));		
			//create the arctangent factor to be added to Zin
									
			REPORT(DEBUG, "  sizeZ=" << sizeZ << "   zMSB="<<zMSB );

			if(stage<maxIterations || reducedIterations == 1) {
				// LSB is always -w 
				vhdl << tab << declare(join("atan2PowStage", stage), sizeZ) << " <= " << unsignedFixPointNumber(zatan, zMSB, zLSB) << ";" <<endl;
				
				vhdl << tab << declare(join("fullZ", stage+1), sizeZ) << " <= " 
				     << join("Z", stage) << " + " << join("atan2PowStage", stage) << " when " << join("D", stage) << "=\'1\' else "
				     << join("Z", stage) << " - " << join("atan2PowStage", stage) << " ;" << endl;
				vhdl << tab << declare(join("Z", stage+1), sizeZ-1) << " <= "<< join("fullZ", stage+1) << range(sizeZ-2, 0) << ";" << endl; 
				vhdl << tab << declare(join("D", (stage+1))) << " <= fullZ" << stage+1 << "(" << sizeZ-1 <<");" <<endl;
			}
			//decrement the size of Z
			sizeZ--;
			zMSB--;
		}
		
		// Give the time to finish the last rotation
		manageCriticalPath( target->localWireDelay(w+1) + target->adderDelay(w+1) // actual CP delay
		                    - (target->localWireDelay(sizeZ+1) + target->adderDelay(sizeZ+1))); // CP delay that was already added

			
		if(reducedIterations == 0){ //regular struture; all that remains is to assign the outputs correctly
			
			//assign output
			
			vhdl << tab << declare("redCos", w+1) << "<= " << join("Cos", stage) << ";" << endl;
			vhdl << tab << declare("redSin", w+1) << "<= " << join("Sin", stage) << ";" << endl;
			

		}
		else{	//reduced iterations structure; rotate by the remaining angle and then assign the angles
			
			vhdl << tab << "-- Reduced iteration: finish the computation by a rotation by Pi Z" << stage << endl; 

		
			nextCycle();
			//multiply X by Pi
			FixRealKCM* piMultiplier = new FixRealKCM(target, zLSB, zMSB, 1, zLSB, "pi", 1.0, inDelayMap("X",getCriticalPath()) ); 
			oplist.push_back(piMultiplier);
			
			vhdl << tab << declare("FinalZ", sizeZ+1) << " <= " << join("D", stage)<< " & " << join("Z", stage) << ";" << endl;
			inPortMap(piMultiplier, "X", "FinalZ");
			outPortMap(piMultiplier, "R", "PiZ");
			vhdl << instance(piMultiplier, "piMultiplier") << endl;
		
			syncCycleFromSignal("PiZ");
			nextCycle();

			manageCriticalPath(target->localWireDelay(sizeZ) + target->DSPMultiplierDelay());



			sizeZ+=3; // +2 cause mult by pi, +1 because we add back the sign
			
			vhdl << tab << declare("CosTrunc", sizeZ) << " <= " << join("Cos", stage) << range(w, w-sizeZ+1) << ";" << endl;
			vhdl << tab << declare("SinTrunc", sizeZ) << " <= " << join("Sin", stage) << range(w, w-sizeZ+1) << ";" << endl;
			
			//multiply with the angle X to obtain the actual values for sine and cosine
			IntMultiplier* zmultiplier = new IntMultiplier(target, sizeZ, sizeZ, true); // signed
			oplist.push_back(zmultiplier);
			
			inPortMap(zmultiplier, "X", "CosTrunc");
			inPortMap(zmultiplier, "Y", "PiZ");
			outPortMap(zmultiplier, "R", "CosTimesZ");
			vhdl << instance(zmultiplier, "MultCosZ") << endl;
			
			inPortMap(zmultiplier, "X", "SinTrunc");
			inPortMap(zmultiplier, "Y", "PiZ");
			outPortMap(zmultiplier, "R", "SinTimesZ");
			vhdl << instance(zmultiplier, "MultSinZ") << endl;
			
			syncCycleFromSignal("CosTimesZ");
			
			manageCriticalPath(target->localWireDelay(w) + target->adderDelay(w));
			
			vhdl << tab << declare("CosTimesZTrunc", sizeZ) << "<= CosTimesZ" << range(2*sizeZ-1, sizeZ) << ";" << endl;
			vhdl << tab << declare("SinTimesZTrunc", sizeZ) << "<= SinTimesZ" << range(2*sizeZ-1, sizeZ) << ";" << endl;
			
			vhdl << tab << declare("ShiftedCosTimesZ", w+1) << " <= (" << w << " downto " << sizeZ << " => CosTimesZ(" << 2*sizeZ-1 << ")) & CosTimesZTrunc;"  << endl;
			vhdl << tab << declare("ShiftedSinTimesZ", w+1) << " <= (" << w << " downto " << sizeZ << " => SinTimesZ(" << 2*sizeZ-1 << ")) & SinTimesZTrunc;"  << endl;
			
			vhdl << tab << declare("redCos", w+1) << " <= " << join("Cos", stage) << " - ShiftedSinTimesZ;" << endl;
			vhdl << tab << declare("redSin", w+1) << " <= " << join("Sin", stage) << " + ShiftedCosTimesZ;" << endl;
						
		}


		
		vhdl << tab << "---- final reconstruction " << endl;
		

		// All this should fit in one level of LUTs
		manageCriticalPath(target->localWireDelay(wOut) + target->lutDelay());

		vhdl << tab << declare("redCosNeg", w+1) << " <= (not redCos); -- negate by NOT, 1 ulp error"<< endl;
		vhdl << tab << declare("redSinNeg", w+1) << " <= (not redSin); -- negate by NOT, 1 ulp error"<< endl;
												   
		vhdl << tab << "with qrot select" << endl
		     << tab << tab << declare("CosX0", w+1) << " <= " << endl;
		vhdl << tab << tab << tab << " redCos    when \"00\"," << endl;
		vhdl << tab << tab << tab << " redSinNeg when \"01\"," << endl;
		vhdl << tab << tab << tab << " redCosNeg when \"10\"," << endl;
		vhdl << tab << tab << tab << " redSin    when others;" << endl;

		vhdl << tab << "with qrot select" << endl
		      << tab << tab << declare("SinX0", w+1) << " <= " << endl;
		vhdl << tab << tab << tab << " redSin    when \"00\"," << endl;
		vhdl << tab << tab << tab << " redCos    when \"01\"," << endl;
		vhdl << tab << tab << tab << " redSinNeg when \"10\"," << endl;
		vhdl << tab << tab << tab << " redCosNeg when others;" << endl;
		
		
		manageCriticalPath( target->adderDelay(1+wOut+1));

		vhdl << tab << declare("roundedCosX", wOut+1) << " <= CosX0" << range(w, w-wOut) << " + " << " (" << zg(wOut) << " & \'1\');" << endl;
		vhdl << tab << declare("roundedSinX", wOut+1) << " <= SinX0" << range(w, w-wOut) << " + " << " (" << zg(wOut) << " & \'1\');" << endl;
														   
		vhdl << tab << "C <= roundedCosX" << range(wOut, 1) << ";" << endl;
		vhdl << tab << "S <= roundedSinX" << range(wOut, 1) << ";" << endl;


		mpfr_clears (zatan, NULL);		
	};


	CordicSinCos::~CordicSinCos(){
		mpfr_clears (scale, kfactor, constPi, NULL);		
	 };


	void CordicSinCos::emulate(TestCase * tc) 
	{
		mpfr_t z, rsin, rcos;
		mpz_class sin_z, cos_z;
		mpfr_init2(z, 10*w);
		mpfr_init2(rsin, 10*w); 
		mpfr_init2(rcos, 10*w); 
		
														  

		/* Get I/O values */
		mpz_class svZ = tc->getInputValue("X");
		
		/* Compute correct value */
		
		mpfr_set_z (z, svZ.get_mpz_t(), GMP_RNDN); //  exact
		mpfr_div_2si (z, z, wIn-1, GMP_RNDN); // exact
	
		// No need to manage sign bit etc: modulo 2pi is the same as modulo 2 in the initial format
		mpfr_mul(z, z, constPi, GMP_RNDN);

		mpfr_sin(rsin, z, GMP_RNDN); 
		mpfr_cos(rcos, z, GMP_RNDN);
		mpfr_mul(rsin, rsin, scale, GMP_RNDN);
		mpfr_mul(rcos, rcos, scale, GMP_RNDN);

		mpfr_add_d(rsin, rsin, 6.0, GMP_RNDN); // exact rnd here
		mpfr_add_d(rcos, rcos, 6.0, GMP_RNDN); // exact rnd here
		mpfr_mul_2si (rsin, rsin, wOut-1, GMP_RNDN); // exact rnd here
		mpfr_mul_2si (rcos, rcos, wOut-1, GMP_RNDN); // exact rnd here

		// Rounding down
		mpfr_get_z (sin_z.get_mpz_t(), rsin, GMP_RNDD); // there can be a real rounding here
		mpfr_get_z (cos_z.get_mpz_t(), rcos, GMP_RNDD); // there can be a real rounding here
		sin_z -= mpz_class(6)<<(wOut-1);
		cos_z -= mpz_class(6)<<(wOut-1);

		tc->addExpectedOutput ("S", sin_z);
		tc->addExpectedOutput ("C", cos_z);

		// Rounding up
		mpfr_get_z (sin_z.get_mpz_t(), rsin, GMP_RNDU); // there can be a real rounding here
		mpfr_get_z (cos_z.get_mpz_t(), rcos, GMP_RNDU); // there can be a real rounding here
		sin_z -= mpz_class(6)<<(wOut-1);
		cos_z -= mpz_class(6)<<(wOut-1);

		tc->addExpectedOutput ("S", sin_z);
		tc->addExpectedOutput ("C", cos_z);
		
		// clean up
		mpfr_clears (z, rsin, rcos, NULL);		
	}






	void CordicSinCos::buildStandardTestCases(TestCaseList * tcl) 
	{
		TestCase* tc;
		mpfr_t z;
		mpz_class zz;
		
		
		mpfr_init2(z, 10*w);
		
		//z=0
		tc = new TestCase (this);
		tc -> addInput ("X", mpz_class(0));
		emulate(tc);
		tcl->add(tc);
					
		tc = new TestCase (this);
		tc->addComment("Pi/4-eps");
		mpfr_set_d (z, 0.24, GMP_RNDD); 
		mpfr_mul_2si (z, z, wIn-1, GMP_RNDD); 
		mpfr_get_z (zz.get_mpz_t(), z, GMP_RNDD);  
		tc -> addInput ("X", zz);
		emulate(tc);
		tcl->add(tc);
				
		tc = new TestCase (this);
		tc->addComment("Pi/6");
		mpfr_set_d (z, 0.166666666666666666666666666666666, GMP_RNDD); 
		mpfr_mul_2si (z, z, wIn-1, GMP_RNDD); 
		mpfr_get_z (zz.get_mpz_t(), z, GMP_RNDD);  
		tc -> addInput ("X", zz);
		emulate(tc);
		tcl->add(tc);
				
		tc = new TestCase (this);
		tc->addComment("Pi/3");
		mpfr_set_d (z, 0.333333333333333333333333333333, GMP_RNDD); 
		mpfr_mul_2si (z, z, wIn-1, GMP_RNDD); 
		mpfr_get_z (zz.get_mpz_t(), z, GMP_RNDD);  
		tc -> addInput ("X", zz);
		emulate(tc);
		tcl->add(tc);
				
		
		mpfr_clears (z, NULL);
	}


	
	mpz_class CordicSinCos::fp2fix(mpfr_t x, int wI, int wF){
		mpz_class h;
		
		mpfr_mul_2si(x, x, wF, GMP_RNDN);
		mpfr_get_z(h.get_mpz_t(), x,  GMP_RNDN);  
		
		return h;
	}

}




#if 0 
		// Rough simulation, for debug purpose
		double c,cc,s,ss,z, z0, dd,p,scale;
		int d;
		const double pi=3.14159265358979323846264338327950288419716939937508;
		c=mpfr_get_d(kfactor, GMP_RNDN);
		s=0.0;
		z=0.15625; // 1/6
		z0=z;
		p=0.5;
		scale=(double) (1<<19);
		for(stage=1; stage<=maxIterations; stage++){
			if(z>=0) d=0; else d=1;
			if(d==0) dd=1.0; else dd=-1.0;
			cc = c - dd*p*s;
			ss = s + dd*p*c;
			cout << stage << "\t atan=" << atan(p)/pi<< "  \t d=" << d << "\t z=" << z << "\t c=" << c << "\t s=" << s;
			cout  << "      \t z=" << (int)(z*scale) << "  \t c=" << (int)(c*scale*4) << "\t s=" << (int)(s*scale*4) << endl;
			z = z - dd*atan(p)/pi;
			c=cc;
			s=ss;
			p=p*0.5;
		}		
		cout  << "Should be  \t\t\t\t\t\t c=" << cos(pi*z0) << "  \t s=" << sin(pi*z0) << endl;
		//		manageCriticalPath(target->localWireDelay(wcs + g) + target->lutDelay());
#endif		
