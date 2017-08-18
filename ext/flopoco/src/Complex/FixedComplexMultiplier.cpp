#include <fstream>
#include <sstream>
#include "FixedComplexMultiplier.hpp"

using namespace std;

namespace flopoco{

	extern vector<Operator *> oplist;

	//TODO: explore implementation using multiply-accumulate operators
	//FIXME: correct timing of the circuit
	FixedComplexMultiplier::FixedComplexMultiplier(Target* target, int wI_, int wO_, float threshold_, bool signedOperator_, bool threeMultiplications)
		: Operator(target), wI(wI_), wO(wO_), signedOperator(signedOperator_), threshold(threshold_)
	{
		
		ostringstream name;

		setCopyrightString ( "Matei Istoan, Florent de Dinechin (2008-2012)" );

			if(signedOperator)
				useStdLogicSigned();
			else
				useStdLogicUnsigned();

		if(target->isPipelined())
			name << "FixedComplexMultiplier_" << wI << "_" << wO << "_f"<< target->frequencyMHz() << "_uid" << getNewUId();
		else
			name << "FixedComplexMultiplier_" << wI << "_" << wO << "_uid" << getNewUId();
		setName( name.str() );

		addInput("Xr", 		wI, true);
		addInput("Xi", 		wI, true);
		addInput("Yr", 		wI, true);
		addInput("Yi", 		wI, true);

	
#if 1
		addOutput("Zi",   wO, 2);
		addOutput("Zr",   wO, 2);

		// we compute the two products faithfully on wO bits
		// we add them, we obtain wO+1 bits
		// so after truncating the sum to wO bits the result is faithful
		int g = IntMultiplier::neededGuardBits(wI, wI, wO); 

		BitHeap* bitHeapRe = new BitHeap(this, 1+wO+g, "Re");  // will add XrYr - XiYi
		BitHeap* bitHeapIm = new BitHeap(this, 1+wO+g, "Im");  // will add XrYi + XiYr
		// Use virtual multipliers that will add their result to the bitHeap
		//IntMultiplier* multXrYr = 

		setCycle(0);
		new IntMultiplier(this, bitHeapRe,
		                  getSignalByName("Xr"),
		                  getSignalByName("Yr"),
		                  wI, wI, wO, 
		                  g, // lsbWeight
		                  false, // negate
		                  signedOperator, 
		                  threshold);
		//IntMultiplier* multXiYi = 
		setCycle(0);
		new IntMultiplier(this, bitHeapRe,
		                  getSignalByName("Xi"),
		                  getSignalByName("Yi"),
		                  wI, wI, wO, 
		                  g, // lsbWeight
		                  true, // negate
		                  signedOperator, 
		                  threshold);
		// The round bit
		if(g)
			bitHeapRe -> addConstantOneBit(g);

	
		bitHeapRe -> generateCompressorVHDL();	
		

		//IntMultiplier* multXrYi = 
		setCycle(0);
		new IntMultiplier(this, bitHeapIm,
		                  getSignalByName("Xr"),
		                  getSignalByName("Yi"),
		                  wI, wI, wO, 
		                  g, // lsbWeight
		                  false, // negate
		                  signedOperator,
		                  threshold);
		//IntMultiplier* multXiYr = 
		setCycle(0);
		new IntMultiplier(this, bitHeapIm,
		                  getSignalByName("Xi"),
		                  getSignalByName("Yr"),
		                  wI, wI, wO, 
		                  g, // lsbWeight
		                  false, // negate
		                  signedOperator, 
		                  threshold);
		// The round bit
		if(g)
			bitHeapIm -> addConstantOneBit(g);


		bitHeapIm -> generateCompressorVHDL();			

		vhdl << tab << "Zr <= " << bitHeapRe -> getSumName() << range(wO+g, g+1) << ";" << endl;
		vhdl << tab << "Zi <= " << bitHeapIm -> getSumName() << range(wO+g, g+1) << ";" << endl;
		
#else // pre-BitHeap version, use this to compare
		addOutput("Zi",   2*w, 2);
		addOutput("Zr",   2*w, 2);

		if(!threeMultiplications){
			IntMultiplier* multiplyOperator = new IntMultiplier(target, w, w, w, signedOperator, 1.0, inDelayMap("X",getCriticalPath()));
			oplist.push_back(multiplyOperator);
			IntAdder* addOperator =  new IntAdder(target, 2*w, inDelayMap("X",getCriticalPath()));
			oplist.push_back(addOperator);
			
			inPortMap (multiplyOperator, "X", "Xi");
			inPortMap (multiplyOperator, "Y", "Yi");
			outPortMap(multiplyOperator, "R", "XiYi");
			vhdl << instance(multiplyOperator, "MUL_XiYi");
			
			inPortMap (multiplyOperator, "X", "Xr");
			inPortMap (multiplyOperator, "Y", "Yr");
			outPortMap(multiplyOperator, "R", "XrYr");
			vhdl << instance(multiplyOperator, "MUL_XrYr");
			
			inPortMap (multiplyOperator, "X", "Xr");
			inPortMap (multiplyOperator, "Y", "Yi");
			outPortMap(multiplyOperator, "R", "XrYi");
			vhdl << instance(multiplyOperator, "MUL_XrYi");
			
			inPortMap (multiplyOperator, "X", "Xi");
			inPortMap (multiplyOperator, "Y", "Yr");
			outPortMap(multiplyOperator, "R", "XiYr");
			vhdl << instance(multiplyOperator, "MUL_XiYr");
			
			syncCycleFromSignal("XiYr", false);
			
			// invert the sign of XiYi to obtain a subtraction
			vhdl << tab << declare("neg_XiYi", 2*w) << " <= XiYi xor (" << 2*w-1 << " downto 0 => \'1\');" << endl;
			
			syncCycleFromSignal("neg_XiYi", false);
			nextCycle();
			
			inPortMap 	(addOperator, "X", 	 "XrYr");
			inPortMap 	(addOperator, "Y", 	 "neg_XiYi");
			inPortMapCst(addOperator, "Cin", "\'1\'");
			outPortMap	(addOperator, "R", 	 "Zr", false);
			vhdl << instance(addOperator, "ADD_XrYrMinXiYi");
			
			inPortMap 	(addOperator, "X", 	 "XrYi");
			inPortMap 	(addOperator, "Y", 	 "XiYr");
			inPortMapCst(addOperator, "Cin", "\'0\'");
			outPortMap	(addOperator, "R", 	 "Zi", false);
			vhdl << instance(addOperator, "ADD_XrYiAddXiYr");
		}
		else{
			try{
				IntMultiplier* multiplyOperator = new IntMultiplier(target, w, w, w, signedOperator, 1.0, inDelayMap("X",getCriticalPath()));
				oplist.push_back(multiplyOperator);
				IntAdder* addOperator =  new IntAdder(target, w, inDelayMap("X",getCriticalPath()));
				oplist.push_back(addOperator);	
				
				vhdl << tab << declare("neg_Yr", w) << " <= Yr xor (" << w-1 << " downto 0 => \'1\');" << endl;	
				
				inPortMap 	(addOperator, "X", 	 "Xr");
				inPortMap 	(addOperator, "Y",   "Xi");
				inPortMapCst(addOperator, "Cin", "\'0\'");
				outPortMap	(addOperator, "R", 	 "XrAddXi");
				vhdl << instance(addOperator, "ADD_XrXi");
			
				inPortMap 	(addOperator, "X", 	 "Yi");
				inPortMap 	(addOperator, "Y",   "neg_Yr");
				inPortMapCst(addOperator, "Cin", "\'1\'");
				outPortMap	(addOperator, "R",   "YiMinYr");
				vhdl << instance(addOperator, "ADD_YiMinYr");
			
				inPortMap 	(addOperator, "X",   "Yi");
				inPortMap 	(addOperator, "Y",   "Yr");
				inPortMapCst(addOperator, "Cin", "\'0\'");
				outPortMap	(addOperator, "R",   "YrAddYi");
				vhdl << instance(addOperator, "ADD_YrAddYi");
			
				syncCycleFromSignal("YrAddYi", false);
				//nextCycle(); 
			
				inPortMap (multiplyOperator, "X", "Yr");
				inPortMap (multiplyOperator, "Y", "XrAddXi");
				outPortMap(multiplyOperator, "R", "K1");
				vhdl << instance(multiplyOperator, "MUL_K1");
			
				inPortMap (multiplyOperator, "X", "Xr");
				inPortMap (multiplyOperator, "Y", "YiMinYr");
				outPortMap(multiplyOperator, "R", "K2");
				vhdl << instance(multiplyOperator, "MUL_K2");
			
				inPortMap (multiplyOperator, "X", "Xi");
				inPortMap (multiplyOperator, "Y", "YrAddYi");
				outPortMap(multiplyOperator, "R", "K3");
				vhdl << instance(multiplyOperator, "MUL_K3");
			
				syncCycleFromSignal("K3", false);
				//nextCycle(); 
			
				vhdl << tab << declare("neg_K3", w) << " <= K3 xor (" << w-1 << " downto 0 => \'1\');" << endl;
			
				syncCycleFromSignal("neg_K3", false);
				//nextCycle();
			
				IntAdder *addOperator2 =  new IntAdder(target, 2*w, inDelayMap("X",getCriticalPath()));
				oplist.push_back(addOperator2);
			
				inPortMap 	(addOperator2, "X",   "K1");
				inPortMap 	(addOperator2, "Y",   "neg_K3");
				inPortMapCst(addOperator2, "Cin", "\'1\'");
				outPortMap	(addOperator2, "R",   "Zr", false);
				vhdl << instance(addOperator2, "ADD_K1MinK3");
			
				inPortMap 	(addOperator2, "X",   "K1");
				inPortMap 	(addOperator2, "Y",   "K2");
				inPortMapCst(addOperator2, "Cin", "\'0\'");
				outPortMap	(addOperator2, "R",   "Zi", false);
				vhdl << instance(addOperator2, "ADD_K1AddK2");
			}catch(std::string str){
				cout << "execution interrupted: " << str << endl;
				exit(1);
			}
		}
#endif
	
	}	


	FixedComplexMultiplier::~FixedComplexMultiplier()
	{
	}
	


	void FixedComplexMultiplier::buildStandardTestCases(TestCaseList* tcl){
		TestCase *tc;

		// first few cases to check emulate()
		tc = new TestCase(this); 
		tc->addInput("Xr", mpz_class(0) );
		tc->addInput("Xi", mpz_class(0) );
		tc->addInput("Yr", mpz_class(0) );
		tc->addInput("Yi", mpz_class(0) );
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("Xr", mpz_class(1) );
		tc->addInput("Xi", mpz_class(0) );
		tc->addInput("Yr", mpz_class(1) );
		tc->addInput("Yi", mpz_class(0) );
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("Xr", mpz_class(0) );
		tc->addInput("Xi", mpz_class(1) );
		tc->addInput("Yr", mpz_class(0) );
		tc->addInput("Yi", mpz_class(1) );
		emulate(tc);
		tcl->add(tc);

		tc = new TestCase(this); 
		tc->addInput("Xr", mpz_class(3) );
		tc->addInput("Xi", mpz_class(3));
		tc->addInput("Yr", mpz_class(3) );
		tc->addInput("Yi", mpz_class(3) );
		emulate(tc);
		tcl->add(tc);

		mpz_class neg = (mpz_class(1)<<wI);
		tc = new TestCase(this); 
		tc->addInput("Xr", mpz_class(3) );
		tc->addInput("Xi", neg -3);
		tc->addInput("Yr", mpz_class(3) );
		tc->addInput("Yi", mpz_class(3) );
		emulate(tc);
		tcl->add(tc);

	}

	
	void FixedComplexMultiplier::emulate ( TestCase* tc ) {
		mpz_class svXr = tc->getInputValue("Xr");
		mpz_class svXi = tc->getInputValue("Xi");
		mpz_class svYr = tc->getInputValue("Yr");
		mpz_class svYi = tc->getInputValue("Yi");
		
		
		if (! signedOperator){

			// mpz_class svZi = svXr*svYi + svXi*svYr;
			// mpz_class svZr = svXr*svYr - svXi*svYi;
			
			// // Don't allow overflow
			// mpz_clrbit ( svZi.get_mpz_t(), 2*wI );
			// mpz_clrbit ( svZr.get_mpz_t(), 2*w );

			// tc->addExpectedOutput("Zi", svZi);
			// tc->addExpectedOutput("Zr", svZr);
		}
		else{

			svXr = bitVectorToSigned(svXr, wI);
			svXi = bitVectorToSigned(svXi, wI);
			svYr = bitVectorToSigned(svYr, wI);
			svYi = bitVectorToSigned(svYi, wI);

			mpz_class svZr = svXr*svYr - svXi*svYi;
			mpz_class svZi = svXr*svYi + svXi*svYr;
			
			svZr = signedToBitVector(svZr, 2*wI+1);
			svZi = signedToBitVector(svZi, 2*wI+1);

			// now truncate to wO bits
			if (wO<2*wI+1){
				svZr = svZr >> (2*wI+1-wO);
				svZi = svZi >> (2*wI+1-wO);
			}

			if (wO>2*wI+1){
				svZr = svZr << (-2*wI+1+wO);
				svZi = svZi << (-2*wI+1+wO);
			}
			tc->addExpectedOutput("Zi", svZi);
			tc->addExpectedOutput("Zr", svZr);
			
			svZr++;
			svZi++;
			mpz_clrbit ( svZr.get_mpz_t(), wO );			// no overflow
			mpz_clrbit ( svZi.get_mpz_t(), wO );			// no overflow
			tc->addExpectedOutput("Zi", svZi);
			tc->addExpectedOutput("Zr", svZr);
			
			
		}
		

	}

}
