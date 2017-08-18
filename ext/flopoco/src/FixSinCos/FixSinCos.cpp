
// TODOs 
// Move FixSinPoly here!
// Compare not-ing Y and negating it properly
// FixSinCosTable has constant sign bit as soon as there is argument reduction. As we just hit 36 output bits, this is important
// The upper test for order-one seems wrong, I get a 38-Kbit table for ./flopoco -pipeline=no  -verbose=2 FixSinCos 15 TestBenchFile 10

// One optim for 24 bits would be to compute z² for free by a table using the second unused port of the blockram
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// works only with sollya
#ifdef HAVE_SOLLYA
#include "../ConstMult/FixRealKCM.hpp"
#include "../IntMultiplier.hpp"
#include "../FixFunctions/FunctionTable.hpp"
#include "../IntConstDiv.hpp"
#include "../BitHeap.hpp"
#include "../IntMultipliers/FixSinPoly.hpp"

#include <iostream>
#include <sstream>

/* header of libraries to manipulate multiprecision numbers */
#include "mpfr.h"
#include "FixSinCos.hpp"
#include "Table.hpp"


using namespace std;
using namespace flopoco;


#define SUBCYCLEPIPELINING 0
#define USEBITHEAP 1
#define LARGE_PREC 1000 // 1000 bits should be enough for everybody
#define LOW_PREC 0


////////////////////////////////////// SinCosTable ///////////////////
// argRedCase=1 for full table, 4 for quadrant, 8 for octant
// 
FixSinCos::SinCosTable::SinCosTable(Target* target_, int wIn_, int lsbOut_, int g_, int argRedCase_, FixSinCos* parent_):
	Table(target_, wIn_, 2*(lsbOut_+g_+(argRedCase_==1?1:0))), 
	lsbOut(lsbOut_),
	g(g_),
	argRedCase(argRedCase_), 
	parent(parent_)
{
	ostringstream name;
	srcFileName = "FixSinCos::SinCosTable";
	name << "SinCosTable_" << wIn << "_2x" << lsbOut;
	if (g>0) 
		name << "p" << g;
	setName(name.str());
	//	outDelayMap["Y"]=target->RMADelay();
}

FixSinCos::SinCosTable::~SinCosTable(){
};



mpz_class FixSinCos::SinCosTable::function (int x){
	mpz_class sin,cos;
	mpfr_t a, c, s;

	mpfr_init2(c, LARGE_PREC); // cosine
	mpfr_init2(s, LARGE_PREC); // sine

	// 2's complement convertion of x into xs
	int xs=x;
	if ( (xs>>(wIn-1)) && (argRedCase==1) )
		xs-=(1<<wIn);

	// a will be used for the value in xs
	mpfr_init2(a,10*wIn-1); 
	mpfr_set_si(a, xs, GMP_RNDN);

	//REPORT(0,"Evaluating function on point x="<<x<<" positive value xs="<<xs<<" converted value a="<<printMPFR(a, 10));

	//divide by 2^w then we get a true fixpoint number between -1 and 1.
	if (argRedCase==1){
		if (xs>>(wIn-1))
			xs-=(1<<wIn);
		mpfr_div_2si(a,a,wIn-1, GMP_RNDN);
		//REPORT(0,"a divided by 2^"<<wIn<<" a="<<printMPFR(a,10));
	}

	else if (argRedCase==4)	{ //quadrant
		mpfr_div_2si(a,a,wIn+1, GMP_RNDN);
		//REPORT(0,"a divided by 2^"<<wIn<<" a="<<printMPFR(a,10));
	}

	else if (argRedCase==8)	{ // octant 
		mpfr_div_2si(a,a,wIn+2, GMP_RNDN);
	}
	else 
		THROWERROR("Bad value for argRedCase in FixSinCos::SinCosTable: " << argRedCase);

	mpfr_mul(a, a, parent->constPi, GMP_RNDN);

	mpfr_sin_cos(s, c, a, GMP_RNDN); //function evaluation on point x

	mpfr_mul(s, s, parent->scale, GMP_RNDN); //rescale the sine
	mpfr_mul(c, c, parent->scale, GMP_RNDN); //rescale the cosine

	//REPORT(0," s="<<printMPFR(s,10)<<"; c="<<printMPFR(c,10));

	mpfr_mul_2si(s, s, lsbOut+g, GMP_RNDN); //scale to int
	mpfr_mul_2si(c, c, lsbOut+g, GMP_RNDN); 

	mpfr_get_z(sin.get_mpz_t(), s, GMP_RNDN); //rounding into mpz
	mpfr_get_z(cos.get_mpz_t(), c, GMP_RNDN);

  if(g){ // add the round bit
		sin += (mpz_class(1)<<(g-1));
		cos += (mpz_class(1)<<(g-1));
	}

	//REPORT(0,"Calculated values before 2's complement test: sin="<<sin.get_mpz_t()<<"; cos="<<cos.get_mpz_t());

	// no more need intermediates a, c, and s
	mpfr_clears(a, c, s, NULL);

	if(argRedCase==1) { // Full table: need to manage two's complement
		
		// check if negative, then 2's complement
		if(sin<0){
			sin+=mpz_class(1)<<(wOut/2); 
		}
		
		if (cos<0){
			cos+=mpz_class(1)<<(wOut/2); 
		}
	}

	// REPORT(0," function() returns. Value: "<<(sin+(cos<<wIn))<<" ( sin=" << sin<<" , cos="<<cos<<  " )");
	return ( cos  + ( sin << (wOut/2) ) ); 

}







////////////////////////////////////// FixSinCos ///////////////////

FixSinCos::FixSinCos(Target * target, int w_, float ratio):Operator(target), w(w_){
	int g=-42; // silly value from the start, because so many different paths may assign it (or forget to do so) 
	srcFileName="FixSinCos";

	// definition of the name of the operator
	ostringstream name;
	name << "FixSinCos_" << w;
	setNameWithFreq(name.str());

	setCopyrightString("Florent de Dinechin, Antoine Martinet, Guillaume Sergent, (2013)");

	// everybody needs many digits of Pi
	mpfr_init2(constPi, 10*w);
	mpfr_const_pi( constPi, GMP_RNDN);

	//compute the scale factor		
	mpfr_init2(scale, w+1);
	mpfr_set_d(scale, -1.0, GMP_RNDN);           // exact
	mpfr_mul_2si(scale, scale, -w, GMP_RNDN); // exact
	mpfr_add_d(scale, scale, 1.0, GMP_RNDN);     // exact
	//REPORT(DEBUG, "scale=" << printMPFR(scale, 15));

	// declaring inputs
	addInput("X", w+1);

	// declaring outputs
	addOutput("S", w+1);
	addOutput("C", w+1);

	// These are borders between small-precision cases for which we generate simpler architectures 

	// plain tabulation fits LUTs
	bool wSmallerThanBorder1 = ( w <= target->lutInputs() );    

	//  table with quadrant reduction fits LUTs
	bool wSmallerThanBorder2 = ( w-2 <= target->lutInputs() );   

	// plain tabulation fits BlockRAM
	bool wSmallerThanBorder3 = ( (w+1)*(mpz_class(2)<<(w+1)) <= target->sizeOfMemoryBlock() );

	//  table with quadrant reduction fits BlockRAM
	bool wSmallerThanBorder4 = ( (w+1)*(mpz_class(2)<<(w+1-3)) <= target->sizeOfMemoryBlock() );

	bool usePlainTable = wSmallerThanBorder1 || (!wSmallerThanBorder2 && wSmallerThanBorder3);
	bool usePlainTableWithQuadrantReduction = wSmallerThanBorder2 || (!wSmallerThanBorder3 && wSmallerThanBorder4);


	// order-1 architecture: sinZ \approx Z, cosZ \approx 1
	int gOrder1Arch=3; 
	// order-2 architecture: sinZ \approx Z, cosZ \approx 1-Z^2/2
	int gOrder2Arch=3; // TODO check
	// generic case:
	int gGeneric=4;

	// For all the other methods we need to address a table of sincos with wA bits of the input
	// Let us compute wA such that these bits fit in a blockRAM
	// but it depends on g, so we compute the various cases
	int wAtemp=3;
	while((mpz_class(2)<<wAtemp)*(w+1+gOrder1Arch) < target->sizeOfMemoryBlock()) wAtemp++; 
	int wAOrder1Arch = wAtemp--;
	wAtemp=3;
	while((mpz_class(2)<<wAtemp)*(w+1+gOrder2Arch) < target->sizeOfMemoryBlock()) wAtemp++; 
	int wAOrder2Arch = wAtemp--;
	wAtemp=3;
	while((mpz_class(2)<<wAtemp)*(w+1+gGeneric) < target->sizeOfMemoryBlock()) wAtemp++; 
	int wAGeneric = wAtemp--;


	// Now we may compute the borders on the simplified cases
	// Y will be smaller than 1/4 => Z will be smaller than pi*2^(-wA-2), or Z<2^-wA 
	// for sinZ we neglect something in the order of Z^2/2 : we want 2^(-2wA-1) < 2^(-w-g)    2wA+1>w+g
	bool wSmallerThanBorderFirstOrderTaylor = (w + gOrder1Arch < 2*wAOrder1Arch + 1);

	// now we neglect something in the order of Z^3/6 (smaller than Z^3/4): we want 2^(-3wA-2) < 2^(-w-g)
	bool wSmallerThanBorderSecondOrderTaylor = (w + gOrder2Arch < 3*wAOrder2Arch + 2);



	REPORT(DEBUG, "Boundaries on the various cases for w="<<w << ": " << wSmallerThanBorder1 << wSmallerThanBorder2 << wSmallerThanBorder3 << wSmallerThanBorder4 << wSmallerThanBorderFirstOrderTaylor<< wSmallerThanBorderSecondOrderTaylor);


	// We must set g here (early) in order to be able to factor out code that uses g
	g=0; 
	int wA=0;
	if (!wSmallerThanBorder4 && wSmallerThanBorderFirstOrderTaylor) {
		REPORT(DEBUG, "Using order-1 arch");
		g=gOrder1Arch;
		wA = wAOrder1Arch;
	}
	else if(wSmallerThanBorderSecondOrderTaylor) {
		REPORT(DEBUG, "Using order-2 arch");
		g = gOrder2Arch;
		wA = wAOrder2Arch;
	}
	else{ // generic case
		g=gGeneric;
		wA=wAGeneric;
	// In the generic case we neglect order-4 term, Z^4/24 < Z^4/16
	// Let us check that our current wA allows that, otherwise increase it. 
		while (w > 4*wA-4-g)
			wA++;
	}

	if(wA)
		REPORT(DETAILED, "Bits addressing the table for this size : wA=" << wA); 


	if (usePlainTable)	{
		REPORT(DETAILED, "Simpler architecture: Using plain table" );
		scT= new SinCosTable(target, w+1, w, 0, 1, this);

		addSubComponent(scT); // adding the table

		//int sinCosSize = 2*(w_+1); // size of output (sine plus cosine in a same number, sine on high weight bits)
		vhdl << tab << declare("sinCosTabIn", w+1) << " <= X;" << endl;// signal declaration

		inPortMap(scT, "X", "sinCosTabIn");
		outPortMap(scT, "Y", "SC"); // ports mapping

		vhdl << instance(scT, "sinCosTable" ); //instanciation
		syncCycleFromSignal("SC");

		
		vhdl << tab << declare("Sine", w+1) << " <= SC" << range(2*(w+1)-1, w+1) << ";" << endl;// signal declaration
		vhdl << tab << declare("Cosine", w+1) << " <= SC" << range(w, 0) << ";" << endl;// signal declaration
		//delete (scT);
		vhdl << tab<< "S <= Sine;"<<endl;
		vhdl << tab<< "C <= Cosine;"<<endl;

	}

	else if (usePlainTableWithQuadrantReduction) 	{
		REPORT(DETAILED, "Simpler architecture: Using plain table with quadrant reduction");
		/*********************************** RANGE REDUCTION **************************************/
		// the argument is reduced into (0,1/2) because one sin/cos
		// computation in this range can always compute the right sin/cos
		// 2's complement: 0 is always positive
		vhdl << tab << declare ("X_sgn") << " <= X" << of (w) << ";" << endl;
		vhdl << tab << declare ("Q") << " <= X" << of (w-1) << ";" << endl;
		vhdl << tab << declare ("X_in",w-1) << " <= X " << range (w-2,0) << ";" << endl;

		vhdl << tab << declare("C_sgn")
			<< " <= Q xor X_sgn;" << endl; //sign of cosin

		/*********************************** REDUCED TABLE **************************************/
		scT= new SinCosTable(target, w-1, w, 0, 4, this);

		addSubComponent(scT); // adding the table to vhdl component list

		//int sinCosSize = 2*(w-2); // size of output (sine plus cosine in a same number, sine on high weight bits)

		inPortMap(scT, "X", "X_in");
		outPortMap(scT, "Y", "SC_red"); // ports mapping

		vhdl << instance(scT, "sinCosRedTable" ); //instanciation
		syncCycleFromSignal("SC_red");

		//vhdl << instance(scT, "cosTable" );

		vhdl << tab << declare("S_out", w) << " <= SC_red " << range( 2*w-1 , w ) << ";" << endl;// signal declaration
		vhdl << tab << declare("C_out", w) << " <= SC_red " << range( w-1, 0 ) << ";" << endl;// signal declaration
		//delete (scT);

		/*********************************** Reconstruction of both sine and cosine **************************************/

		vhdl << tab << declare ("S_wo_sgn", w)
			<< " <= C_out when Q = '1' else S_out;" << endl; //swap sine and cosine if q.
		vhdl << tab << declare ("C_wo_sgn", w)
			<< " <= S_out when Q = '1' else C_out;" << endl;


		vhdl << tab << declare ("S_wo_sgn_ext", w+1)
			<< " <= '0' & S_wo_sgn;" << endl
			<< tab << declare ("C_wo_sgn_ext", w+1)
			<< " <= '0' & C_wo_sgn;" << endl; //S_wo_sgn_ext and C_wo_sgn are the positive versions of the sine and cosine

		vhdl << tab << declare ("S_wo_sgn_neg", w+1)
			<< " <= (not S_wo_sgn_ext) + 1;" << endl;
		vhdl << tab << declare ("C_wo_sgn_neg", w+1)
			<< " <= (not C_wo_sgn_ext) + 1;" << endl; //2's complement. We have now the negative version of the sine and cosine results


		vhdl << tab << "S <= S_wo_sgn_ext when X_sgn = '0'"
			<< " else S_wo_sgn_neg;" << endl
			<< tab << "C <= C_wo_sgn_ext when C_sgn = '0'"
			<< " else C_wo_sgn_neg;" << endl; //set the correspondant value to C and S according to input sign
	}






	else { // From now on we will have a table-based argument reduction
 		/*********************************** RANGE REDUCTION **************************************/ 
		addComment("The argument is reduced into (0,1/4)");
		vhdl << tab << declare ("X_sgn") << " <= X" << of (w) << ";  -- sign" << endl;
		vhdl << tab << declare ("Q") << " <= X" << of (w-1) << ";  -- quadrant" << endl;
		vhdl << tab << declare ("O") << " <= X" << of (w-2) << ";  -- octant" << endl;
		vhdl << tab << declare ("Y",w-2) << " <= X " << range (w-3,0) << ";" << endl;

		// now X -> X_sgn + Q*.5 + O*.25 + Y where Q,O \in {0,1} and Y \in {0,.25}

		int wYIn=w-2+g;
		
		addComment("Computing .25-Y :  we do a logic NOT, at a cost of 1 ulp");
		manageCriticalPath(target->localWireDelay(w-2) + target->lutDelay());
		vhdl << tab << declare ("Yneg", wYIn) << " <= ((not Y) & " << '"' << std::string (g, '1') << '"' << ") when O='1' "
				 << "else (Y & " << '"' << std::string (g, '0') << '"' << ");" << endl;

		int wY = wYIn-wA; // size of Y_red
		
		vhdl << tab << declare ( "A", wA) << " <= Yneg " << range(wYIn-1, wYIn-wA) << ";" << endl;
		vhdl << tab << declare ("Y_red", wY) << " <= Yneg" << range (wYIn-wA-1,0) << ";" << endl; // wYin-wA=wY: OK

		//------------------------------------SinCosTable building for A -------------------------------------
		scT= new SinCosTable(target, wA, w, g, 8, this);
		addSubComponent(scT); // adding the table to vhdl component list
		inPortMap(scT, "X", "A");
		outPortMap(scT, "Y", "SCA");
		vhdl << instance(scT, "sinCosPiATable" ); 
		syncCycleFromSignal("SCA");

		vhdl << tab << declare("SinPiA", w+g) << " <= SCA " << range( 2*(w+g)-1 , w+g ) << ";" << endl;
		vhdl << tab << declare("CosPiA", w+g) << " <= SCA " << range( w+g-1, 0 ) << ";" << endl;
		
		//-------------------------------- MULTIPLIER BY PI ---------------------------------------
		
		map<string, double> pi_mult_inputDelays;
		pi_mult_inputDelays["X"] = getCriticalPath();
		int wZ=w-wA+g; // see alignment below. Actually w-2-wA+2  (-2 because Q&O bits, +2 because mult by Pi)

		pi_mult = new FixRealKCM (target,
															-w-g,     // lsbIn
															-2-wA-1,  // msbIn
															false,    // signedInput
															-w-g ,    // lsbOut
															"pi",     // constant 
															1.0,      // targetUlpError
															pi_mult_inputDelays); 
		oplist.push_back (pi_mult);
		inPortMap (pi_mult, "X", "Y_red");
		outPortMap (pi_mult, "R", "Z");
		int wZz=getSignalByName("Z")->width();
		REPORT(DEBUG, "wZ=" <<wZ<<";"<<" wZz="<<wZz<<";");
		vhdl << instance (pi_mult, "pi_mult");
		syncCycleFromSignal("Z", pi_mult->getOutputDelay("R"));




		if (wSmallerThanBorderFirstOrderTaylor) {
				REPORT(DETAILED,"Simpler architecture: Using only first order Taylor");

				int m=3; // another number of guard bits, this time on the truncation of CosPiA and SinPiA

				//---------------------------- Sine computation ------------------------
				vhdl << tab <<  declare("SinPiACosZ",w+g) << " <= SinPiA; -- For these sizes  CosZ approx 1"<<endl; // msb is -1; 
				vhdl << tab << declare("CosPiAtrunc", wZ+m ) << " <= CosPiA" << range( w+g-1, w+g-wZ-m ) <<";" <<endl; // 
				vhdl << tab << declare("CosPiASinZ", 2*wZ+m ) << " <= CosPiAtrunc*Z;  -- For these sizes  SinZ approx Z" <<endl; //
				// msb of CosPiASinZ is that of Z, plus 2 (due to multiplication by Pi)
				//   for g=2 and wA=4:          :  .QOAAAAYYYYgg
				//                                      ZZZZZZZZ
				// to align with sinACosZ:         .XXXXXXXXXXgg we need to add wA+2-2 zeroes. 
				// and truncate cosAsinZ to the size of Z, too
				vhdl << tab << declare("PreSinX", w+g) << " <= SinPiACosZ + ( " << zg(wA) << " & (CosPiASinZ" << range( 2*wZ+m-1, 2*wZ+m - (w+g - wA) ) << ") );"<<endl;

				//---------------------------- Cosine computation -------------------------------
				vhdl << tab << declare("CosPiACosZ", w+g ) << " <= CosPiA; -- For these sizes  CosZ approx 1" << endl;
				vhdl << tab << declare("SinPiAtrunc", wZ+m ) << " <= SinPiA" << range( w+g-1, w+g-wZ-m ) <<";" <<endl; // 
				vhdl << tab << declare("SinPiASinZ", 2*wZ+m ) << " <= SinPiAtrunc*Z;  -- For these sizes  SinZ approx Z" <<endl; //
				vhdl << tab << declare("PreCosX", w+g) << " <= CosPiACosZ - ( " << zg(wA) << " & (SinPiASinZ" << range( 2*wZ+m-1, 2*wZ+m - (w+g - wA) )<< ") );" << endl;

				// Reconstruction expects a positive C_out and S_out, without their sign bits
				vhdl << tab << declare ("C_out", w) << " <= PreCosX" << range (w+g-1, g) << ';' << endl;
				vhdl << tab << declare ("S_out", w) << " <= PreSinX" << range (w+g-1, g) << ';' << endl;
			}



		else if (wSmallerThanBorderSecondOrderTaylor) {

		REPORT(DETAILED,"Using first-order Taylor for sine and second-order for cosine");

		//--------------------------- SQUARER --------------------------------
		// For these sizes it always fits a DSP mult, so no need to use a flopoco component
		vhdl << tab << declare("Z2", 2*wZ) << "<= Z*Z;" << endl;
		
		// Z < 2^-wA  :
		//   for g=2 and wA=4:          :  .QOAAAAYYYYgg
		//                                 .0000ZZZZZZZZ
		// Z^2/2 <  2^(-2wA-1):            .000000000SSS
		int m=3; // Another number of guard bits because the multiplication will fit in DSP anyway
		int wZ2o2 = w+g-(2*wA+1); // according to figure above
		int wZ2o2Guarded = wZ2o2 + m; // TODO check that it always still fit DSPs
		REPORT(DEBUG, "Useful size of Z2o2 is wZ2o2=" << wZ2o2 << ", to which we add " << m << " guard bits");
		vhdl << tab << declare("Z2o2_trunc", wZ2o2Guarded) << "<= Z2" << range(2*wZ-1, 2*wZ - wZ2o2Guarded) << ";" << endl;

		vhdl << tab << declare("CosPiA_trunc", wZ2o2Guarded) << " <= CosPiA" << range(w+g-1, w+g-wZ2o2Guarded) << ";" << endl;
		vhdl << tab << declare("Z2o2CosPiA", 2*wZ2o2Guarded)<< " <=  CosPiA_trunc * Z2o2_trunc;" << endl;
		vhdl << tab << declare("Z2o2CosPiA_aligned", w+g)<< " <= " << zg(2*wA+1) << " & Z2o2CosPiA" << range(2*wZ2o2Guarded-1, 2*wZ2o2Guarded- wZ2o2) << ";" << endl;
		vhdl << tab << declare("CosPiACosZ", w+g) << "<= CosPiA - Z2o2CosPiA_aligned;" << endl;

		vhdl << tab << declare("SinPiA_trunc", wZ2o2Guarded) << " <= SinPiA" << range(w+g-1, w+g-wZ2o2Guarded) << ";" << endl;
		vhdl << tab << declare("Z2o2SinPiA", 2*wZ2o2Guarded)<< " <=  SinPiA_trunc * Z2o2_trunc;" << endl;
		vhdl << tab << declare("Z2o2SinPiA_aligned", w+g)<< " <= " << zg(2*wA+1) << " & Z2o2SinPiA" << range(2*wZ2o2Guarded-1, 2*wZ2o2Guarded- wZ2o2) << ";" << endl;
		vhdl << tab << declare("SinPiACosZ", w+g) << "<= SinPiA - Z2o2SinPiA_aligned;" << endl;



		vhdl << tab << declare("CosPiAZ", w+g +  wZ) << " <= CosPiA*Z;  -- TODO check it fits DSP" <<endl;
		vhdl << tab << declare("CosPiASinZ", w+g) << " <= " << zg(wA) << " & CosPiAZ"  << range(w+g+wZ-1, w+g+wZ- (w+g-wA)) << ";" <<endl; // alignment according to figure above
		vhdl << tab << declare("SinPiAZ", w+g +  wZ) << " <= SinPiA*Z;  -- TODO check it fits DSP" <<endl;
		vhdl << tab << declare("SinPiASinZ", w+g) << " <= " << zg(wA) << " & SinPiAZ"  << range(w+g+wZ-1, w+g+wZ- (w+g-wA)) << ";" <<endl; // alignment according to figure above

		vhdl << tab << declare("PreSinX", w+g) << " <= SinPiACosZ + CosPiASinZ;"<<endl;
		vhdl << tab << declare("PreCosX", w+g) << " <= CosPiACosZ - SinPiASinZ;"<<endl;

		// Reconstruction expects a positive C_out and S_out, without their sign bits
		vhdl << tab << declare ("C_out", w) << " <= PreCosX" << range (w+g-1, g) << ';' << endl;
		vhdl << tab << declare ("S_out", w) << " <= PreSinX" << range (w+g-1, g) << ';' << endl;
		}

		else	{

		REPORT(DETAILED, "Using generic architecture with 3rd-order Taylor");

		/*********************************** THE SQUARER **************************************/
			
			map<string, double> sqr_z_inputDelays;
#if SUBCYCLEPIPELINING
			sqr_z_inputDelays["X"] = pi_mult->getOutputDelay("R");
			sqr_z_inputDelays["Y"] = pi_mult->getOutputDelay("R");
#else
			nextCycle();
#endif

			// vhdl:sqr (Z -> Z2o2)
			// we have no truncated squarer as of now
			/*IntSquarer *sqr_z;
				sqr_z = new IntSquarer (target, wZ);
				oplist.push_back (sqr_z);
				inPortMap (sqr_z, "X", "Z");
				outPortMap (sqr_z, "R", "Z2o2_ext");
				vhdl << instance (sqr_z, "sqr_z");
				// so now we truncate unnecessarily calculated bits of Z2o2_ext
				int wZ2o2 = 2*wZ - (w+g);
				vhdl << declare ("Z2o2",wZ2o2) << " <= Z2o2_ext"
				<< range (wZ-1,wZ-wZ2o2) << ";" << endl;*/
			// so we use a truncated multiplier instead
			IntMultiplier *sqr_z;
			int wZ2o2 = 2*wZ - (w+g)-1;
			if (wZ2o2 < 2)
				wZ2o2 = 2; //for sanity
			vhdl << tab << "-- First truncate the inputs of the multiplier to the precision of the output" << endl;
			vhdl << tab << declare("Z_truncToZ2", wZ2o2) << " <= Z" << range(wZ-1, wZ-wZ2o2) << ";" << endl;
			sqr_z = new IntMultiplier (target, wZ2o2, wZ2o2, wZ2o2, false, ratio, sqr_z_inputDelays);
			oplist.push_back (sqr_z);
			inPortMap (sqr_z, "Y", "Z_truncToZ2");
			inPortMap (sqr_z, "X", "Z_truncToZ2");
			outPortMap (sqr_z, "R", "Z2o2");
			vhdl << instance (sqr_z, "sqr_z");
			syncCycleFromSignal("Z2o2");


			/*********************************** Z-Z^3/6 **************************************/

			//	int wZ3 = 3*wZ - 2*(w+g) -1; // -1 for the div by 2
			int wZ3o6 = 3*wZ - 2*(w+g) -2;
			if (wZ3o6 < 2)
				wZ3o6 = 2; //using 1 will generate bad vhdl
			
			if(wZ3o6<=12) {
				vhdl << tab << "-- First truncate Z" << endl;
				vhdl << tab << declare("Z_truncToZ3o6", wZ3o6) << " <= Z" << range(wZ-1, wZ-wZ3o6) << ";" << endl;
				FunctionTable *z3o6Table;
				z3o6Table = new FunctionTable (target, "x^3/6", wZ3o6, -wZ3o6-2, -3);
				z3o6Table -> changeName(getName() + "_Z3o6Table");
				oplist.push_back (z3o6Table);
				inPortMap (z3o6Table, "X", "Z_truncToZ3o6");
				outPortMap (z3o6Table, "Y", "Z3o6");
				vhdl << instance (z3o6Table, "z3o6Table");
				syncCycleFromSignal("Z3o6");

				manageCriticalPath(target->adderDelay(wZ));
				vhdl << tab << declare ("SinZ", wZ) << " <= Z - Z3o6;" << endl;
				setSignalDelay("SinZ", getCriticalPath());
				
			}
			else {
				// TODO: replace all this with an ad-hoc unit
				FixSinPoly *fsp =new FixSinPoly(target, 
																				-wA-1, //msbin
																				-w-g, // lsbin
																				true, // truncated
																				-wA-1, // msbOut_ = 0,
																				-w-g, // lsbout
																				false);
				oplist.push_back (fsp);
				inPortMap (fsp, "X", "Z");
				outPortMap(fsp, "R", "SinZ");
				vhdl << instance (fsp, "ZminusZ3o6");
				syncCycleFromSignal("SinZ");

			}


			// vhdl:sub (Z, Z3o6 -> SinZ)
			


			// and now, evaluate Sin Yneg and Cos Yneg
			// Cos Yneg:
			// vhdl:slr (Z2o2 -> Z2o2)
			
			

			/*********************************** Reconstruction of cosine **************************************/

			// First get back to the cycle of Z2
#if SUBCYCLEPIPELINING
			setCycleFromSignal("Z2o2", getSignalDelay("Z2o2"));
#else
			setCycleFromSignal("Z2o2");
			nextCycle();
#endif






		
#if 1 		// No bit heap
			// // vhdl:id (CosPiA -> C_out_1)
			// vhdl:mul (Z2o2, CosPiA -> Z2o2CosPiA)
			
			vhdl << tab << "--  truncate the larger input of each multiplier to the precision of its output" << endl;
			vhdl << tab << declare("CosPiA_truncToZ2o2", wZ2o2) << " <= CosPiA" << range(w+g-1, w+g-wZ2o2) << ";" << endl;
			IntMultiplier *c_out_2;
			c_out_2 = new IntMultiplier (target, wZ2o2, wZ2o2, wZ2o2, false, ratio);
			oplist.push_back (c_out_2);
			inPortMap (c_out_2, "X", "Z2o2");
			inPortMap (c_out_2, "Y", "CosPiA_truncToZ2o2");
			outPortMap (c_out_2, "R", "Z2o2CosPiA");
			vhdl << instance (c_out_2, "c_out_2_compute");


#if SUBCYCLEPIPELINING
			syncCycleFromSignal("Z2o2CosPiA", c_out_2->getOutputDelay("R"));
#else
			syncCycleFromSignal("Z2o2CosPiA");
			nextCycle();
#endif

			// get back to the cycle of SinZ, certainly later than the SinPiA
#if SUBCYCLEPIPELINING
			setCycleFromSignal("SinZ", getSignalDelay("SinZ")); 
#else
			setCycleFromSignal("SinZ"); 
			nextCycle();
#endif

			vhdl << tab << "--  truncate the larger input of each multiplier to the precision of its output" << endl;
			vhdl << tab << declare("SinPiA_truncToZ", wZ) << " <= SinPiA" << range(w+g-1, w+g-wZ) << ";" << endl;


			// vhdl:mul (SinZ, SinPiA -> SinZSinPiA)
			IntMultiplier *c_out_3;
			
			c_out_3 = new IntMultiplier (target, wZ, wZ, wZ, false, ratio);
			oplist.push_back (c_out_3);
			inPortMap (c_out_3, "Y", "SinPiA_truncToZ");
			inPortMap (c_out_3, "X", "SinZ");
			outPortMap (c_out_3, "R", "SinZSinPiA");
			vhdl << instance (c_out_3, "c_out_3_compute");
			
			setCycleFromSignal ("Z2o2CosPiA");
			nextCycle();

			// TODO: critical path supposed suboptimal (but don't know how to fix)
			manageCriticalPath(target->localWireDelay() + target->adderDelay(w+g));
			vhdl << tab << declare ("CosZCosPiA_plus_rnd", w+g)
					 << " <= CosPiA - Z2o2CosPiA;" << endl;

			syncCycleFromSignal ("SinZSinPiA");
			nextCycle();

			manageCriticalPath(target->localWireDelay() + target->adderDelay(w+g));
			vhdl << tab << declare ("C_out_rnd_aux", w+g)
			<< " <= CosZCosPiA_plus_rnd - SinZSinPiA;" << endl;

			vhdl << tab << declare ("C_out", w)
					 << " <= C_out_rnd_aux" << range (w+g-1, g) << ';' << endl;




			// ---------------------------------------------
#else //Bit heap computing Cos Z  ~   CosPiA - Z2o2*cosPiA - sinZ*SinPiA
			//
			// cosPiA   xxxxxxxxxxxxxxxxxxggggg
			//
			
#define TINKERCOS 1
#if TINKERCOS
			int gMult=0;
#else
			int g1 = IntMultiplier::neededGuardBits(wZ, wZ, wZ);
			int g2 = IntMultiplier::neededGuardBits(wZ2o2, wZ2o2, wZ2o2);
			int gMult=max(g1,g2);
#endif

			REPORT(0, "wZ2o2=" << wZ2o2 << "    wZ=" << wZ << "    g=" << g << "    gMult=" << gMult);
			BitHeap* bitHeapCos = new BitHeap(this, w+g+gMult, "Sin"); 
			
			// Add CosPiA to the bit heap
			bitHeapCos -> addUnsignedBitVector(gMult, "CosPiA", w+g);
			
			vhdl << tab << "--  truncate the larger input of each multiplier to the precision of its output" << endl;
			vhdl << tab << declare("CosPiA_truncToZ2o2", wZ2o2) << " <= CosPiA" << range(w+g-1, w+g-wZ2o2) << ";" << endl;
			vhdl << tab << "--  truncate the larger input of each multiplier to the precision of its output" << endl;
			vhdl << tab << declare("SinPiA_truncToZ", wZ) << " <= SinPiA" << range(w+g-1, w+g-wZ) << ";" << endl;
			

#if TINKERCOS
			setCycleFromSignal ("Z2o2");
			nextCycle();
			vhdl <<  tab << declare("Z2o2CosPiA", 2*wZ2o2) << " <= Z2o2 * CosPiA_truncToZ2o2" << ";" << endl;
			nextCycle();
		// add it to the bit heap

			for (int i=0; i<wZ2o2-1; i++)
				bitHeapCos->addBit(i, "not Z2o2CosPiA"+of(i+wZ2o2));
			bitHeapCos->addBit(wZ2o2-1, "Z2o2CosPiA"+of(wZ2o2+wZ2o2-1));
			for (int i=wZ2o2-1; i<w+g+gMult; i++)
			bitHeapCos->addConstantOneBit(i);
			bitHeapCos->addConstantOneBit(0);
			

			setCycleFromSignal ("SinZ");
			nextCycle();
			vhdl <<  tab << declare("SinZSinPiA", 2*wZ) << " <=  SinZ *SinPiA_truncToZ" << ";" << endl;
			nextCycle();
			
			// add it to the bit heap	
			for (int i=0; i<wZ-1; i++)
				bitHeapCos->addBit(i, "not SinZSinPiA"+of(i+wZ));
			bitHeapCos->addBit(wZ-1, "SinZSinPiA"+of(wZ+wZ-1));
			for (int i=wZ-1; i<w+g+gMult; i++)
				bitHeapCos->addConstantOneBit(i);
			bitHeapCos->addConstantOneBit(0);
			
			//	vhdl <<  tab << declare("Z2o2CosPiA", 2*wZ) << " <= " << range(w+g-1, w+g-wZ) << ";" << endl;
#else
			// First virtual multiplier
			new IntMultiplier (this,
												 bitHeapCos,
												 getSignalByName("Z2o2"),
												 getSignalByName("CosPiA_truncToZ2o2"),
												 wZ2o2, wZ2o2, wZ2o2,
												 gMult,
												 true, // negate
												 false, // signed inputs
												 ratio);



			// Second virtual multiplier
			new IntMultiplier (this,
												 bitHeapCos,
												 getSignalByName("SinZ"),
												 getSignalByName("SinPiA_truncToZ"),
												 wZ, wZ, wZ,
												 gMult,
												 true, // negate
												 false, // signed inputs
												 ratio
												 );
#endif
			
			// The round bit is in the table already
			bitHeapCos -> generateCompressorVHDL();	
			vhdl << tab << declare ("C_out", w) << " <= " << bitHeapCos -> getSumName() << range (w+g+gMult-1, g+gMult) << ';' << endl;
			
#endif

			/*********************************** Reconstruction of sine **************************************/
			//Bit heap computing   SinPiA - Z2o2*sinPiA + sinZ*CosPiA
			// Sin Z:
			
			// First get back to the cycle of Z2 (same as Cos_y_red):
			// it is certainly later than SinPiA
			
#if SUBCYCLEPIPELINING
			// TODO
#else
			setCycleFromSignal("Z2o2");
			nextCycle();
			
#endif

			// // vhdl:id (SinPiA -> S_out_1)
			// vhdl:mul (Z2o2, SinPiA -> Z2o2SinPiA)
			vhdl << tab << "-- First truncate the larger input of the multiplier to the precision of the output" << endl;
			vhdl << tab << declare("SinPiA_truncToZ2o2", wZ2o2) << " <= SinPiA" << range(w+g-1, w+g-wZ2o2) << ";" << endl;
			IntMultiplier *s_out_2;
			s_out_2 = new IntMultiplier (target, wZ2o2, wZ2o2, wZ2o2, false, ratio);
			oplist.push_back (s_out_2);
			inPortMap (s_out_2, "X", "Z2o2");
			inPortMap (s_out_2, "Y", "SinPiA_truncToZ2o2");
			outPortMap (s_out_2, "R", "Z2o2SinPiA");
			vhdl << instance (s_out_2, "s_out_2_compute");
			syncCycleFromSignal("Z2o2SinPiA");
			

			// get back to the cycle of SinZ, certainly later than the CosPiA
			setCycleFromSignal("SinZ", getSignalDelay("SinZ"));

			// vhdl:mul (SinZ, CosPiA -> SinZCosPiA)
			nextCycle();

			vhdl << tab << "-- First truncate the larger input of the multiplier to the precision of the output" << endl;
			vhdl << tab << declare("CosPiA_truncToSinZ", wZ) << " <= CosPiA" << range(w+g-1, w+g-wZ) << ";" << endl;
			
			IntMultiplier *s_out_3;
			s_out_3 = new IntMultiplier (target, wZ, wZ, wZ, false, ratio);
			oplist.push_back (s_out_3);
			inPortMap (s_out_3, "X", "SinZ");
			inPortMap (s_out_3, "Y", "CosPiA_truncToSinZ");
			outPortMap (s_out_3, "R", "SinZCosPiA");
			vhdl << instance (s_out_3, "s_out_3_compute");
			syncCycleFromSignal("SinZCosPiA");
			
			setCycleFromSignal ("Z2o2SinPiA");
			nextCycle();
			
			manageCriticalPath(target->localWireDelay() + target->adderDelay(w+g));
			vhdl << tab << declare ("CosZSinPiA_plus_rnd", w+g)
					 << " <= SinPiA - Z2o2SinPiA;" << endl;
			
			syncCycleFromSignal ("SinZCosPiA");
			nextCycle();
			
			manageCriticalPath(target->localWireDelay() + target->adderDelay(w+g));
			vhdl << tab << declare ("S_out_rnd_aux", w+g)
					 << " <= CosZSinPiA_plus_rnd + SinZCosPiA;" << endl;


			//Final synchronization
			syncCycleFromSignal("C_out");
			
			vhdl << tab << declare ("S_out", w)
					 << " <= S_out_rnd_aux" << range (w+g-1, g) << ';' << endl;
			
		REPORT(INFO, " wA=" << wA <<" wZ=" << wZ <<" wZ2=" << wZ2o2 <<" wZ3o6=" << wZ3o6 );

		// For LateX in the paper
		//	cout << "     " << w <<  "   &   "  << wA << "   &   " << wZ << "   &   " << wZ2o2 << "   &   " << wZ3o6 << "   \\\\ \n \\hline" <<  endl;

		} // closes if for generic case



		// When we arrive here we should have two signals C_out and S_out, each of size w 
		addComment("--- Final reconstruction of both sine and cosine ---");

		vhdl << tab << declare("C_sgn") << " <= X_sgn xor Q;" << endl;
		vhdl << tab << declare ("Exch") << " <= Q xor O;" << endl;

		vhdl << tab << declare ("S_wo_sgn", w)
				 << " <= C_out when Exch = '1' else S_out;" << endl; //swap sine and cosine if q xor o
		vhdl << tab << declare ("C_wo_sgn", w)
				 << " <= S_out when Exch = '1' else C_out;" << endl;
		
		
		vhdl << tab << declare ("S_wo_sgn_ext", w+1)
				 << " <= '0' & S_wo_sgn;" << endl
				 << tab << declare ("C_wo_sgn_ext", w+1)
				 << " <= '0' & C_wo_sgn;" << endl; //S_wo_sgn_ext and C_wo_sgn are the positive versions of the sine and cosine
		
		vhdl << tab << declare ("S_wo_sgn_neg", w+1)
				 << " <= (not S_wo_sgn_ext) + 1;" << endl;
		vhdl << tab << declare ("C_wo_sgn_neg", w+1)
				 << " <= (not C_wo_sgn_ext) + 1;" << endl; //2's complement. We have now the negative version of the sine and cosine results
		
		vhdl << tab << "S <= S_wo_sgn_ext when X_sgn = '0'"
				 << " else S_wo_sgn_neg;" << endl;

		vhdl << tab << "C <= C_wo_sgn_ext when C_sgn = '0'"
				 << " else C_wo_sgn_neg;" << endl; //set the correspondant value to C and S according to input sign
	}

};




FixSinCos::~FixSinCos(){
	if(scT) free(scT);
	if(pi_mult) free(pi_mult);
	mpfr_clears (scale, constPi, NULL);		
};






void FixSinCos::emulate(TestCase * tc)
{
	// TODO eventually have only one shared FixSinCos emulate code

	mpfr_t z, rsin, rcos;
	mpz_class sin_z, cos_z;
	mpfr_init2(z, 10*w);
	mpfr_init2(rsin, 10*w); 
	mpfr_init2(rcos, 10*w); 

	/* Get I/O values */
	mpz_class svZ = tc->getInputValue("X");

	/* Compute correct value */

	mpfr_set_z (z, svZ.get_mpz_t(), GMP_RNDN); //  exact
	mpfr_div_2si (z, z, w, GMP_RNDN); // exact

	// No need to manage sign bit etc: modulo 2pi is the same as modulo 2 in the initial format
	mpfr_mul(z, z, constPi, GMP_RNDN);

	//REPORT(DEBUG, "Emulate scale=" << printMPFR(scale, 15));
	
	mpfr_sin(rsin, z, GMP_RNDN); 
	mpfr_cos(rcos, z, GMP_RNDN);
	mpfr_mul(rsin, rsin, scale, GMP_RNDN);
	mpfr_mul(rcos, rcos, scale, GMP_RNDN);

	mpfr_add_d(rsin, rsin, 6.0, GMP_RNDN); // exact rnd here
	mpfr_add_d(rcos, rcos, 6.0, GMP_RNDN); // exact rnd here
	mpfr_mul_2si (rsin, rsin, w, GMP_RNDN); // exact rnd here
	mpfr_mul_2si (rcos, rcos, w, GMP_RNDN); // exact rnd here

	// Rounding down
	mpfr_get_z (sin_z.get_mpz_t(), rsin, GMP_RNDD); // there can be a real rounding here
	mpfr_get_z (cos_z.get_mpz_t(), rcos, GMP_RNDD); // there can be a real rounding here
	sin_z -= mpz_class(6)<<(w);
	cos_z -= mpz_class(6)<<(w);

	tc->addExpectedOutput ("S", sin_z);
	tc->addExpectedOutput ("C", cos_z);

	// Rounding up
	mpfr_get_z (sin_z.get_mpz_t(), rsin, GMP_RNDU); // there can be a real rounding here
	mpfr_get_z (cos_z.get_mpz_t(), rcos, GMP_RNDU); // there can be a real rounding here
	sin_z -= mpz_class(6)<<(w);
	cos_z -= mpz_class(6)<<(w);

	tc->addExpectedOutput ("S", sin_z);
	tc->addExpectedOutput ("C", cos_z);

	// clean up
	mpfr_clears (z, rsin, rcos, NULL);		
}


void FixSinCos::buildStandardTestCases(TestCaseList * tcl)
{
	mpfr_t z;
	mpz_class zz;
	TestCase* tc;

	mpfr_init2(z, 10*w);

	//z=0
	tc = new TestCase (this);
	tc -> addInput ("X", mpz_class(0));
	emulate(tc);
	tcl->add(tc);

	tc = new TestCase (this);
	tc->addComment("Pi/4-eps");
	mpfr_set_d (z, 0.24, GMP_RNDD); 
	mpfr_mul_2si (z, z, w-1, GMP_RNDD); 
	mpfr_get_z (zz.get_mpz_t(), z, GMP_RNDD);  
	tc -> addInput ("X", zz);
	emulate(tc);
	tcl->add(tc);

	tc = new TestCase (this);
	tc->addComment("Pi/6");
	mpfr_set_d (z, 0.166666666666666666666666666666666, GMP_RNDD); 
	mpfr_mul_2si (z, z, w-1, GMP_RNDD); 
	mpfr_get_z (zz.get_mpz_t(), z, GMP_RNDD);  
	tc -> addInput ("X", zz);
	emulate(tc);
	tcl->add(tc);

	tc = new TestCase (this);
	tc->addComment("Pi/3");
	mpfr_set_d (z, 0.333333333333333333333333333333, GMP_RNDD); 
	mpfr_mul_2si (z, z, w-1, GMP_RNDD); 
	mpfr_get_z (zz.get_mpz_t(), z, GMP_RNDD);  
	tc -> addInput ("X", zz);
	emulate(tc);
	tcl->add(tc);


	mpfr_clears (z, NULL);

}

#endif // SOLLYA

