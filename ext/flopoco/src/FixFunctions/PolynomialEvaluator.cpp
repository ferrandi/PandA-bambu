/*
  A polynomial evaluator for FloPoCo
 
  Author : Bogdan Pasca
 
  This file is part of the FloPoCo project 
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Authors:   Bogdan Pasca, Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.
*/

//TODO choice of ratio/threshold for the multiplier

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <gmp.h>
#include <mpfr.h>
#include <stdio.h>
#include <gmpxx.h>
#include "FixMultAdd.hpp"
#include "PolynomialEvaluator.hpp"



using namespace std;

namespace flopoco{
	
	PolynomialEvaluator::PolynomialEvaluator(Target* target, vector<FixedPointCoefficient*> coef, YVar* y, int targetPrec, mpfr_t* approxError,map<string, double> inputDelays):
		Operator(target, inputDelays), y_(y), targetPrec_(targetPrec), sol(false) {

		thresholdForDSP = 0.9; // Should be a param of the constructor

		setPolynomialDegree(coef.size()-1);
		
		setCopyrightString("Bogdan Pasca, Florent de Dinechin (2010-2012)");
		srcFileName = "PolynomialEvaluator";
		setName(join("PolynomialEvaluator_degree",degree_,"_uid",getNewUId()));

		setApproximationError(approxError); /* set the approximation error budget we are allowed */

		mpfr_init2 ( targetError, 150);
		mpfr_set_si( targetError, 2, GMP_RNDN);
		mpfr_pow_si( targetError, targetError, -targetPrec-1, GMP_RNDN); /* 1/2 ulp target error */

		// TODO fix comments: MSB weight below, LSB weight in most of the code

		/* both y and a[i], i in 0 ... d are described by two values: size and MSB weight 

		   for y, which is always positive, the two parameters are
		
		   |<-- y_->getWeight() --->|
		   .---------------------[0][y_{size-1} downto 0    ]
		   |<--- y_getSize() ----->|
		
		   for the coefficients
		   |<-- coef[i]->getWeight() --->|
		   .--------------------------[s][coef[i]->getSize()-1 downto 0 ]
		   |<--- coef[i]_->getSize() ---->|        */
		
		updateCoefficients(coef);
		REPORT(DETAILED, "Polynomial to evaluate: " << printPolynomial(coef, y, 0));
		REPORT(DETAILED, "y size=" << y_->getSize() << " weight=" << y_->getWeight());
		
		/* I/O Signal Declarations; y and the coefficients*/
		addInput("Y", y_->getSize()); /* y is positive so we don't store the sign */

		for (uint32_t i=0; i <= unsigned(degree_); i++){
			addInput(join("a",i), coef_[i]->getSize()+1); /* the size does not contain the sign bit */
			REPORT(DETAILED, "a"<<i<<" size=" << coef_[i]->getSize() << " weight=" << coef_[i]->getWeight());
		}
		
		allocateErrorVectors();
		allocateAndInitializeExplorationVectors();
		
		/* needed in the approximation error computation. We do it once as 
		   this doesn't change during the iterations */
		setMaxYValue(y); 

		hornerGuardBitProfiler(); // sets the min values for pi truncations
				
		determineObjectiveStatesForTruncationsOnY();
		setNumberOfPossibleValuesForEachY();
		initializeExplorationVectors();

		mpfr_t u, *e;
		mpfr_init2(u, 100);

		/* design space exploration */				

		/* try to truncate as much as possible from y using the maximum
		   useful number of guard bits */			
		while ((!sol) && (nextStateY())){
			reinitCoefficientGuardBits();
			/* run once the error estimation algo with these parameters */
			e = errorEstimator(yGuard_, aGuard_);
			/* test if by chance we met the error budget */
			/* epsilon_approx + epsilon_eval <= 1/2 ulp. the other half ulp
			   comes from the final rounding (which we incorporate in a0*/
			mpfr_add( u, *approximationError, *e, GMP_RNDN);

			if (  mpfr_cmp( u, targetError) <= 0 ){
				REPORT(DETAILED, " Solution found. Starting refinement");
				/* if we do, then we set the solution true and that's it */
				sol = true;
				mpfr_clear(*e);
				free(e);
			}
		}
		/* at this point we have found a rough solution. We now try to find 
		   smaller values for coefficient guard bits */

		if (degree_ > 1 ){
			/* the second part of the exploration is performed only when the 
			   polynomial degree is larger than 1 */			
			sol = false; //reinit sol;
			reinitCoefficientGuardBits(); /* sets all guard bits to maximum -1 */
			nextStateA(); /* this state brings them back to maximum so that the
			                 next nextStateA() gets them to 0,0,0,0 ...0 */
			while (((!sol) && nextStateA())){
				e = errorEstimator(yGuard_, aGuard_);
				mpfr_add( u, *approximationError, *e, GMP_RNDN);
				if ( mpfr_cmp ( u, targetError) <= 0){ //proper verification
					sol = true;
					mpfr_clear(u);
					mpfr_clear(*e);
					free(e);
				}else{
					mpfr_clear(*e);
					free(e);
				}
			}
		}

		ostringstream s1, s2;		
		s1 << "Guard bits for the sums: ";
		s2 << "Gard bits on the truncation of y : ";
		for (unsigned j=0; j<=degree_; j++)
			s1 << "aG["<<j<<"]="<<aGuard_[j]<<" "; 

		for (unsigned j=1; j<=degree_; j++)
			s2 << "yG["<<j<<"]="<<yGuard_[j]<<" "; 
		
		REPORT(INFO, s1.str());
		REPORT(INFO, s2.str());
		setCriticalPath(getMaxInputDelays(inputDelays));

		for (unsigned i=0; i<=degree_; i++){
			if (i==0){
				vhdl << tab << "-- LSB weight of sigmaP"<<i<<" is="<<coef_[degree_-i]->getWeight()<<" size="<<1+coef_[degree_-i]->getSize()<<endl;
				vhdl << tab << declare( join("sigmaP",i), 1+coef_[degree_-i]->getSize()) << " <= a"<<degree_<<";"<<endl; 
			}else{
				if (i<degree_){
					vhdl << tab << "-- LSB weight of yT"<<i<<" is="<<y_->getWeight()<<" size="<<1+y_->getSize()+yGuard_[i]<<endl;
					vhdl << tab << declare( join("yT",i) , 1+y_->getSize()+yGuard_[i]) << " <= \"0\" & Y"<<range(y_->getSize()-1, -yGuard_[i]) << ";" << endl;
					vhdl << tab << "-- LSB weight of piP"<<i<<" is="<<pikPWeight[i]<<" size="<<pikPSize[i]+2<<endl;

					//TODO => input Delay
					int wIn1 = 1+y_->getSize()+yGuard_[i];
					int wIn2 = sigmakPSize[i-1]+1;
					// orig					int k = 1 + y_->getSize() + yGuard_[i] + sigmakPSize[i-1] + 1 - (pikPTSize[i]+2);
					int k =  y_->getSize() + yGuard_[i] + sigmakPSize[i-1] - pikPTSize[i]; // looks like the number of truncated bits

#if 0
#else					
#define USE_BITHEAP 0
#if !USE_BITHEAP //

					//					nextCycle(); //TODO fix it by feeding the input delay to IntTruncMultiplier


					IntMultiplier *sm = new IntMultiplier ( target, wIn1, wIn2, wIn1+wIn2-k, true /*signedIO*/, thresholdForDSP,   inDelayMap("X",getCriticalPath()));
					oplist.push_back(sm);
					
					inPortMap ( sm, "X", join("yT",i));
					inPortMap ( sm, "Y", join("sigmaP",i-1));
					outPortMap (sm, "R", join("piPT",i));
					vhdl << instance ( sm, join("Product_",i) );
					syncCycleFromSignal(join("piPT",i)); 
				
					setCriticalPath( sm->getOutputDelay("R") );

					// coeff: shifted and sign extended 
					vhdl << tab << declare( join("op1_",i), sigmakPSize[i]+1 ) 
					     << " <= (" << rangeAssign(sigmakPWeight[i] - coef_[degree_-i]->getWeight()-1,0, join("a",degree_-i)+of(coef_[degree_-i]->getSize()))
					     << " & " << join("a",degree_-i) << " & "<< zg(aGuard_[degree_-i],0) << ");"<<endl;
					// product 
					vhdl << tab << declare( join("op2_",i), sigmakPSize[i]+1 ) 
					     << " <= (" << rangeAssign(sigmakPWeight[i]-pikPTWeight[i]-1,0, join("piPT",i)+of(pikPTSize[i])) 
					     << " & " << join("piPT",i) << range(pikPTSize[i], pikPTSize[i] - pikPTWeight[i] - (coef_[degree_-i]->getSize()-coef_[degree_-i]->getWeight() + aGuard_[degree_ -i]))
					     << " & "<< zg( - (pikPTSize[i] - pikPTWeight[i] - (coef_[degree_-i]->getSize()-coef_[degree_-i]->getWeight() + aGuard_[degree_ -i])) ,0)
					     << ");" << endl;

					nextCycle();
					IntAdder* sa = new IntAdder (target, sigmakPSize[i]+1,  inDelayMap("X", getCriticalPath()));
					oplist.push_back(sa);

					inPortMap ( sa, "X", join("op1_",i) );
					inPortMap ( sa, "Y", join("op2_",i) );
					inPortMapCst ( sa, "Cin", "'1'");
					outPortMap( sa, "R", join("sigmaP",i));
				
					vhdl << instance ( sa, join("Sum",i));
					syncCycleFromSignal( join("sigmaP",i) );
					setCriticalPath( sa->getOutputDelay("R") );
					nextCycle();

#else //bitheap-based 347sl 15ns
					int wA=coef_[degree_-i]->getSize()+1;
					int wR=sigmakPSize[i]+1;
					int wOutP=wIn1+wIn2-k;
					int lsbPTruncated= - (pikPTSize[i] - pikPTWeight[i] - (coef_[degree_-i]->getSize()-coef_[degree_-i]->getWeight() + aGuard_[degree_ -i])) ;
					int msbP=lsbPTruncated + wOutP;
					int lsbA=aGuard_[degree_-i];
					// cout << "k=" << k << " lsbPTruncated=" << lsbPTruncated << " msbP=" <<msbP << " lsbA=" << lsbA << " wOutO=" <<wOutP<< endl; 
					//cout << "wIn1=" << wIn1<< " wIn2=" << wIn2 << " wA=" << wA << " wR=" << wR << " msbP=" <<msbP << " lsbA=" << lsbA << endl; 
					FixMultAdd * ma = new  FixMultAdd(target, wIn1, wIn2, wA,
					                                  wR , 
					                                  msbP, /* msbP */
					                                  lsbA,
					                                  true /*signedIO*/, 
					                                  thresholdForDSP);
					//cout << " Fin du constr" << endl;
					oplist.push_back(ma);

					inPortMap ( ma, "X", join("yT",i));
					inPortMap ( ma, "Y", join("sigmaP",i-1));
					inPortMap ( ma, "A", join("a",degree_-i));
					outPortMap( ma, "R", join("sigmaP",i));
					vhdl << instance (ma, join("MultAdd_",i) );
					syncCycleFromSignal( join("sigmaP",i) );
					setCriticalPath( ma->getOutputDelay("R") );

#endif
#endif
				                                                                   
				}else{ // i=degree
					vhdl << tab << "-- weight of yT"<<i<<" is="<<y_->getWeight()<<" size="<<1+y_->getSize()+yGuard_[i]<<endl;
					vhdl << tab << declare( join("yT",i) , 1+y_->getSize()+yGuard_[i]) << " <= \"0\" & Y"<<range(y_->getSize()-1, -yGuard_[i]) << ";" << endl;
					vhdl << tab << "-- weight of piP"<<i<<" is="<<pikPWeight[i]<<" size="<<pikPSize[i]+2<<endl;

					// IntTruncMultiplier* sm = new IntTruncMultiplier ( target, 
					// 																									1+y_->getSize()+yGuard_[i], 
					// 																									sigmakPSize[i-1]+1, 	
					//                                                   (1+y_->getSize()+yGuard_[i]) +  (sigmakPSize[i-1]+1) - (sigmakPSize[i] - (coef_[0]->getSize()+2)) , 
					//                                                   1.1, 
					// 																									1, -1, false, true, false); //inDelayMap("X",getCriticalPath()));
					
					int wOut=(1+y_->getSize()+yGuard_[i]) +  (sigmakPSize[i-1]+1) - (sigmakPSize[i] - (coef_[0]->getSize()+2));
#if !USE_BITHEAP //
					IntMultiplier* sm = new IntMultiplier ( target, 
					                                        1+y_->getSize()+yGuard_[i], 
					                                        sigmakPSize[i-1]+1, 	
					                                        wOut , 
					                                        thresholdForDSP, 
					                                        true); //inDelayMap("X",getCriticalPath()));
					oplist.push_back(sm);
				
					inPortMap ( sm, "X", join("yT",i));
					inPortMap ( sm, "Y", join("sigmaP",i-1));
					outPortMap (sm, "R", join("piP",i));
					vhdl << instance ( sm, join("Product_",i) );
					syncCycleFromSignal(join("piP",i)); 
					
					nextCycle(); // Argh
					setCriticalPath( sm->getOutputDelay("R") );
					vhdl << tab << "-- the delay at the output of the multiplier is : " << sm->getOutputDelay("R") << endl;

					IntAdder* sa = new IntAdder (target, (coef_[0]->getSize()+2), inDelayMap("X",target->localWireDelay() +  getCriticalPath()));
					oplist.push_back(sa);

					// the product
					vhdl << tab << declare( join("op1_",i), (coef_[0]->getSize()+2) ) << " <= " 
					     << rangeAssign( (coef_[0]->getSize()+2)-(wOut-1) -1,0, join("piP",i)+of(wOut-1)) 
					     << " & " << join("piP",i)<<range(wOut-2,0) << ";" << endl;
					
					// a0
					vhdl << tab << declare( join("op2_",i), (coef_[0]->getSize()+2) ) << " <= " 
					     << rangeAssign(0,0, join("a",degree_-i)+of(coef_[degree_-i]->getSize()))
					     << " & " << join("a",degree_-i) << ";"<<endl;

					inPortMapCst ( sa, "X", join("op1_",i));
					inPortMapCst ( sa, "Y", join("op2_",i));
					inPortMapCst ( sa, "Cin", "'1'");
					outPortMap( sa, "R", join("sigmaP",i));
		
					vhdl << instance ( sa, join("Sum",i));
					syncCycleFromSignal( join("sigmaP",i) );
					setCriticalPath(sa->getOutputDelay("R"));
#else
					int wIn1 = 1+y_->getSize()+yGuard_[i]; 
					int wIn2 = sigmakPSize[i-1]+1; 	
					int wOutP = wOut; 
					int msbP=wOutP;
					int wA = coef_[0]->getSize()+2;
					int lsbA=0;

					FixMultAdd * ma = new  FixMultAdd(target, wIn1, wIn2, wA,
					                                  wA , 
					                                  msbP, /* msbP */
					                                  lsbA,
					                                  true /*signedIO*/, 
					                                  thresholdForDSP);
					//cout << " Fin du constr" << endl;
					oplist.push_back(ma);


					// a0
					vhdl << tab << declare( join("op2_",i), (coef_[0]->getSize()+2) ) << " <= " 
					     << rangeAssign(0,0, join("a",degree_-i)+of(coef_[degree_-i]->getSize()))
					     << " & " << join("a",degree_-i) << ";"<<endl;

					inPortMap ( ma, "X", join("yT",i));
					inPortMap ( ma, "Y", join("sigmaP",i-1));
					inPortMap ( ma, "A", join("op2_",i));
					outPortMap( ma, "R", join("sigmaP",i));
					vhdl << instance (ma, join("MultAdd_",i) );
					syncCycleFromSignal( join("sigmaP",i) );
					setCriticalPath( ma->getOutputDelay("R") );

#endif

					wR = coef_[0]->getSize()+2; //sigmakPSize[i]+1;
					weightR = sigmakPWeight[i];
					addOutput("R", wR );//sigmakPSize[i]+1);
					vhdl << tab << "R <= " << join("sigmaP",i)<< range(wR-1,0)<<";"<<endl; //<< << ";" << endl;
				}

				outDelayMap["R"]=getCriticalPath();			
			}		
		}
	}
	
	
	void PolynomialEvaluator::hornerGuardBitProfiler(){
		Sigma* sigma[degree_];
		Pi*    pi   [degree_];
		
		sigma[0] = new Sigma( coef_[degree_]->getSize(), coef_[degree_]->getWeight() , unsigned(0) , 0);
		
		for (unsigned i=1; i<=degree_; i++){
			pi[i]     = new Pi    ( y_->getSize(), y_->getWeight(), sigma[i-1]->getSize(), sigma[i-1]->getWeight());
			sigma[i]  = new Sigma ( pi[i]->getSize(), pi[i]->getWeight(), coef_[degree_-i]->getSize(), coef_[degree_-i]->getWeight());
		}	
		for (unsigned i=0; i<=degree_; i++)
			maxBoundA[i] = sigma[i]->getGuardBits();			
	}


	mpfr_t* PolynomialEvaluator::errorEstimator(vector<int> &yGuard, vector<int> &aGuard){
		ostringstream s1, s2, s3;		
		
		for (unsigned j=0; j<=degree_; j++)
			s1 << "aG["<<j<<"]="<<aGuard[j]<<" "; 
		for (unsigned j=1; j<=degree_; j++){
			s2 << "yG["<<j<<"]=" << yGuard[j] << " "; 
		}

		REPORT(DETAILED, "------------------------------------------------------------");
		REPORT(DETAILED, s1.str());
		REPORT(DETAILED, s2.str());

		/* pre-set the magnitude of y */
		mpfr_t *yy;
		yy =(mpfr_t*) malloc( sizeof( mpfr_t));
		mpfr_init2 ( *yy, 100);
		mpfr_set_ui( *yy, 2, GMP_RNDN);
		mpfr_pow_si( *yy, *yy, y_->getSize(), GMP_RNDN);
		mpfr_add_si( *yy, *yy , -1, GMP_RNDN);
		mpfr_set_exp( *yy, ( mpfr_get_d(*yy, GMP_RNDZ)!=0?mpfr_get_exp(*yy):0) + y_->getWeight() - y_->getSize());

		/* pre-setting the error values for the truncations on y */		
		vector<mpfr_t*> ykT_y; //the yk tild. The max absolute value of ykT
		ykT_y.push_back( NULL );
		for (uint32_t i=1; i<= unsigned(degree_); i++){
			mpfr_t *cykT;
			cykT = (mpfr_t*) malloc(sizeof(mpfr_t));
			mpfr_init2(*cykT, 100);
			if ( yGuard[i] < 0){
				mpfr_set_ui(*cykT, 2, GMP_RNDN);
				mpfr_pow_si(*cykT, *cykT, y_->getWeight()-(signed(y_->getSize())+yGuard[i]), GMP_RNDN); 
			}else
				mpfr_set_si(*cykT, 0, GMP_RNDN);
			ykT_y.push_back(cykT);
		}

		/* pre-setting the error values for the truncations on pi' */		
		vector<mpfr_t*> pikPT_pikP; //the pi k prime tild - p k prime ( trunc error)
		pikPT_pikP.push_back(NULL);
		for (uint32_t i=1; i< unsigned(degree_); i++){ //not needed of n
			mpfr_t *cpikPT_pikP;
			cpikPT_pikP = (mpfr_t*) malloc(sizeof(mpfr_t));
			mpfr_init2(*cpikPT_pikP, 100);
			mpfr_set_ui(*cpikPT_pikP, 2, GMP_RNDN);
			mpfr_pow_si(*cpikPT_pikP, *cpikPT_pikP, coef_[degree_-i]->getWeight()-(signed(coef_[degree_-i]->getSize())+aGuard[degree_-i]), GMP_RNDN); 
			pikPT_pikP.push_back(cpikPT_pikP);
		}
		pikPT_pikP.push_back(NULL); /* don't do truncation on last addition operand */

		/* pre-set the first value from sigmakP-sigmak, being 0 */
		vector<mpfr_t*> sigmakP_sigmak;
		mpfr_t *sigmanP_sigman = (mpfr_t*) malloc( sizeof( mpfr_t));
		mpfr_init2 ( *sigmanP_sigman, 100);
		mpfr_set_ui( *sigmanP_sigman, 0, GMP_RNDN);
		sigmakP_sigmak.push_back(sigmanP_sigman);	//sigmakP_sigmak[0]

		vector<mpfr_t*> pikP_pik;
		vector<mpfr_t*> sigmakP;
		vector<mpfr_t*> a;

		mpfr_t *ak;
		/* pre-set the magnitudes of the coefficients */
		for (uint32_t i=0; i<=unsigned(degree_);i++){
			ak =(mpfr_t*) malloc( sizeof( mpfr_t));
			mpfr_init2 ( *ak, 100);
			mpfr_set_ui( *ak, 2, GMP_RNDN);
			mpfr_pow_si( *ak, *ak, coef_[i]->getSize(), GMP_RNDN);
			mpfr_add_si( *ak, *ak , -1, GMP_RNDN);
			mpfr_set_exp( *ak, (mpfr_get_d(*ak, GMP_RNDZ)!=0?mpfr_get_exp(*ak):0) + coef_[i]->getWeight() - coef_[i]->getSize());
			a.push_back(ak);
		}

		/* the magnitude of the sigmaP[0] is the magnitude of a[d] */		
		sigmakP.push_back(a[degree_]);

		vector<mpfr_t*> pikPT;

		sigmakPSize[0]   = coef_[degree_]->getSize();
		sigmakPWeight[0] = coef_[degree_]->getWeight();
		
		pikP_pik.push_back(NULL);
		pikPT.push_back(NULL);
		
		for (uint32_t i=1; i<=unsigned(degree_); i++){
			
			/* (yT-y)*sigmak-1P */
			mpfr_t *t,*t2; 
			t = (mpfr_t *) malloc( sizeof( mpfr_t ));			
			mpfr_init2( *t, 100);
			mpfr_mul ( *t, *ykT_y[i], *sigmakP[i-1], GMP_RNDN);

			/* y(sigmak-1P-sigmak-1) */
			t2 = (mpfr_t *) malloc (sizeof (mpfr_t));
			mpfr_init2( *t2, 100);
			mpfr_mul ( *t2 , *yy , *sigmakP_sigmak[i-1], GMP_RNDN);
			mpfr_add ( *t, *t , *t2, GMP_RNDN);

			/* pikP-piK = (yT-y)*sigmak-1P + y(sigmak-1P-sigmak-1) */
			pikP_pik.push_back(t);
			mpfr_clear(*t2);
			free(t2);

			/* sigma computation: sigmakP - sigmaK = (pikPT - pikP) + (pikP - pik) */
			pikPWeight[i]  = y_->getWeight() + sigmakPWeight[i-1]; 
			pikPSize[i]    = (y_->getSize()+yGuard[i]) + sigmakPSize[i-1];
		
			pikPTWeight[i] =  y_->getWeight() + sigmakPWeight[i-1]; 
			pikPTSize[i]   =  y_->getWeight() + sigmakPWeight[i-1]  + (coef_[degree_-i]->getSize()+aGuard[i]-coef_[degree_-i]->getWeight());

			mpfr_t *t3; //the sigma
			if (i < unsigned(degree_)){
				t3 = (mpfr_t *) malloc( sizeof( mpfr_t ));
				mpfr_init2( *t3, 100);
				mpfr_add( *t3, *pikPT_pikP[i], *pikP_pik[i],GMP_RNDN); 		   
			}else{
				t3 = (mpfr_t *) malloc( sizeof( mpfr_t ));
				mpfr_init2( *t3, 100);
				mpfr_set( *t3, *pikP_pik[i], GMP_RNDN); 		   
			}
			sigmakP_sigmak.push_back(t3);

			if (i!=unsigned(degree_)){
				int maxMSB = max ( coef_[degree_-i]->getWeight(), pikPTWeight[i]);
				sigmakPSize[i]   = maxMSB + 1 - (coef_[degree_-i]->getWeight() - coef_[degree_-i]->getSize() - aGuard[degree_-i]);
				sigmakPWeight[i] = maxMSB + 1;
			}else{
				int maxMSB = max ( coef_[degree_-i]->getWeight(), pikPWeight[i]);
				int minLSB = min ( coef_[degree_-i]->getWeight()-coef_[degree_-i]->getSize(),pikPWeight[i]-pikPSize[i]); 
				sigmakPSize[i]   = 1 + maxMSB - minLSB + 1;//maxMSB + 1 - (coef_[degree_-i]->getWeight() - coef_[degree_-i]->getSize() - aGuard[degree_-i]);
				sigmakPWeight[i] = 1 + maxMSB;
			}

			/* set values for next iteration */
			/* pikpt */
			mpfr_t *h;
			h = (mpfr_t *) malloc( sizeof(mpfr_t));
			mpfr_init2(*h, 100);
			mpfr_set_ui( *h, 2, GMP_RNDN);
			mpfr_pow_si( *h, *h, pikPTSize[i], GMP_RNDN);
			mpfr_add_si( *h, *h , -1, GMP_RNDN);
			mpfr_set_exp( *h, (mpfr_get_d(*h, GMP_RNDZ)!=0? mpfr_get_exp(*h):0) + pikPTWeight[i] - pikPTSize[i]);
			pikPT.push_back(h);

			/* sigmakP = a[d-k] + pikpt */			
			mpfr_t *r;
			r = (mpfr_t *) malloc( sizeof(mpfr_t));
			mpfr_init2( *r, 100);
			mpfr_add ( *r, *pikPT[i], *a[degree_-i], GMP_RNDN);			
			sigmakP.push_back(r);
			
		}
		
		REPORT(DETAILED, "Error (order) for P(y)=" << mpfr_get_exp(*sigmakP_sigmak[degree_]));
		
		/***** Clean up *********************/
		for (uint32_t i=1; i<= unsigned(degree_); i++){
			if (ykT_y[i]!= NULL){
				if (*ykT_y[i] != ((mpfr_ptr)0))
					mpfr_clear(*ykT_y[i]);
				free( ykT_y[i] );
			}
			if (pikPT[i]!=NULL){
				mpfr_clear(*pikPT[i]);
				free(pikPT[i]);
			}

			if ( pikPT_pikP[i] != NULL){
				if ( *pikPT_pikP[i]  != ((mpfr_ptr)0))
					mpfr_clear(*pikPT_pikP[i]);
				free(pikPT_pikP[i]);
			}
			if (pikP_pik[i] != NULL){
				mpfr_clear(*pikP_pik[i]);
				free(pikP_pik[i]);
			}
		}
		for (uint32_t i=0; i<= unsigned(degree_); i++){
			if (i < unsigned(degree_)){
				if (sigmakP_sigmak[i] != NULL){
					if (*sigmakP_sigmak[i] != ((mpfr_ptr)0))
						mpfr_clear(*sigmakP_sigmak[i]);
					free(sigmakP_sigmak[i]);
				}
			}
			if (sigmakP[i] != NULL){
				if ( *sigmakP[i] != ((mpfr_ptr)0))
					mpfr_clear(*sigmakP[i]);
				free(sigmakP[i]);
			}
			if ( i < unsigned(degree_)){
				if (a[i] != NULL){
					if ( (*a[i]) != NULL)
						mpfr_clear(*a[i]);
					free(a[i]);
				}
			}
		}
		mpfr_clear(*yy);
		free(yy);
			 
		return sigmakP_sigmak[degree_];
	}


	void PolynomialEvaluator::updateCoefficients(vector<FixedPointCoefficient*> coef){
		REPORT(DETAILED, "Coefficient manipulation ... ");
		for (uint32_t i=0; i< coef.size(); i++){
			REPORT(DEBUG, "Coefficient before; size="<<coef[i]->getSize()<<" weight="<<coef[i]->getWeight());
			FixedPointCoefficient *fp = new FixedPointCoefficient(coef[i]);
			/* update the coefficient size; see Doxygen in hpp for more details*/
			fp->setSize(coef[i]->getSize()+coef[i]->getWeight());
			coef_.push_back(fp);
			REPORT(DEBUG, "Coefficient after; size="<<coef_[i]->getSize()<<" weight="<<coef_[i]->getWeight()); 
		}
	}


	PolynomialEvaluator::~PolynomialEvaluator() {
	}


	/* ----------------- ERROR RELATED ----------------------------------*/
	/* ------------------------------------------------------------------*/
	void PolynomialEvaluator::allocateErrorVectors(){
		sigmakPSize.reserve(degree_+2);
		pikPTSize.reserve(degree_+2);
		pikPSize.reserve(degree_+2);
		sigmakPWeight.reserve(degree_+2);
		pikPTWeight.reserve(degree_+2);
		pikPWeight.reserve(degree_+2);
	}


	void PolynomialEvaluator::setApproximationError( mpfr_t *p){
		approximationError = (mpfr_t*) malloc( sizeof(mpfr_t));
		mpfr_init2(*approximationError, 1000);
		mpfr_set( *approximationError, *p, GMP_RNDN);
		REPORT(DETAILED, "The approximation error budget is (represented as double):" << mpfr_get_d(*approximationError,GMP_RNDN)); 
	}


	void PolynomialEvaluator::setMaxYValue(YVar* y){
		/* the abs of the maximal value of y */
		mpfr_init2 ( maxABSy, 100);
		mpfr_set_ui( maxABSy, 2, GMP_RNDN);
		mpfr_pow_si( maxABSy, maxABSy, y_->getSize(), GMP_RNDN);
		mpfr_add_si( maxABSy, maxABSy, -1, GMP_RNDN);
		mpfr_set_exp( maxABSy, mpfr_get_exp(maxABSy)+y_->getWeight()-y_->getSize());
		REPORT(DETAILED, "Abs max value of y is " << mpfr_get_d( maxABSy, GMP_RNDN)); 
	}


	/* ---------------- EXPLORATION RELATED -----------------------------*/
	/* ------------------------------------------------------------------*/
	void PolynomialEvaluator::allocateAndInitializeExplorationVectors(){
		/* reserve memory */
		yGuard_.reserve(degree_+2);
		aGuard_.reserve(degree_+2);
		yState_.reserve(degree_+2);
		maxBoundY.reserve(degree_+2);
		maxBoundA.reserve(degree_+2);
		/*init vectors */
		for (uint32_t i=0; i<=unsigned(degree_)+1; i++){
			yGuard_[i]  = 0;
			yState_[i] = 0;
			aGuard_[i] = 0;
			maxBoundY[i] = 0; //proper values set by setNumberOfPossibleValuesForEachY
			maxBoundA[i] = 0; //the proper values will be set by the profiler
		}
	}


	void PolynomialEvaluator::resetCoefficientGuardBits(){
		for (uint32_t i=0; i<unsigned(degree_)+1; i++)
			aGuard_[i] = 0;
	}


	void PolynomialEvaluator::reinitCoefficientGuardBits(){
		bool done = false;
		for (uint32_t i=1; i<unsigned(degree_)+1; i++)
			if ((!done) && (maxBoundA[i]>0)){
				aGuard_[i] = maxBoundA[i] - 1;
				done = true;
			}else{
				aGuard_[i] = maxBoundA[i];
			}
	}

	void PolynomialEvaluator::reinitCoefficientGuardBitsLastIteration(){
		/* a solution was found with max values for guard bits. now we try to
		   reduce */
		for (uint32_t i=1; i<unsigned(degree_)+1; i++)
			aGuard_[i] = maxBoundA[i];
	}

	
	void PolynomialEvaluator::initializeExplorationVectors(){
		/*init vectors */
		for (uint32_t i=1; i<=unsigned(degree_)+1; i++){
			yGuard_[i] = 0; //maxBoundY;
			yState_[i] = 0;
		}
		for (uint32_t i=0; i<unsigned(degree_)+1; i++)
			aGuard_[i] = maxBoundA[i];
		yState_[1]=-1;	
	}


	void PolynomialEvaluator::determineObjectiveStatesForTruncationsOnY(){
		int Xd, Yd;
		target_->getDSPWidths(Xd,Yd);
		for (int i=0; i < getPolynomialDegree(); i++){
			if (( y_->getSize()> unsigned(Xd) ) && (y_->getSize() % unsigned(Xd) != 0)){
				int k=1;
				while (k*Xd < int(y_->getSize())){
					objectiveStatesY.insert(pair<int,int>(i+1, y_->getSize() - k*Xd));
					k++;					
				}
			}
			if (Yd!=Xd)
				if (( y_->getSize()> unsigned(Yd) ) && (y_->getSize() % unsigned(Yd) != 0)){
					int k=1;
					while (k*Yd < int(y_->getSize())){
						objectiveStatesY.insert(pair<int,int>(i+1, y_->getSize() - k*Yd));
						k++;					
					}
				}
			/* insert the "do no truncation" pair */
			objectiveStatesY.insert(pair<int,int>(i+1, 0)); 
		}
		for (multimap<int, int>::iterator it = objectiveStatesY.begin(); it != objectiveStatesY.end(); ++it){
			REPORT(DEBUG, "yGuardObjective[" << (*it).first << ", " << (*it).second << "]");
		}
	}
	
	
	void PolynomialEvaluator::setNumberOfPossibleValuesForEachY(){
		for (int i=1; i <= getPolynomialDegree(); i++){
			maxBoundY[i] = objectiveStatesY.count(i);
			REPORT(DEBUG, "MaxBoundY["<<i<<"]="<<maxBoundY[i]);
		}
	}


	string PolynomialEvaluator::printPolynomial( vector<FixedPointCoefficient*> coef, YVar* y, int level){
		ostringstream horner;
		if (level == getPolynomialDegree()){
			horner << "a["<<level<<"]2^("<<coef[level]->getWeight()<<")";
			return horner.str();
		}else{
			horner << "y*2^("<<y_->getWeight()<<"){"<< printPolynomial(coef, y, level+1) << "} + " << "a["<<level<<"]2^("<<coef[level]->getWeight()<<")";
			return horner.str();
		}
	}

	int PolynomialEvaluator::getPossibleYValue(int i, int state){
		REPORT(DEBUG, "Possible value i="<<i<<" state="<<state);
		pair<multimap<int, int>::iterator, multimap<int, int>::iterator> ppp;
		ppp = objectiveStatesY.equal_range(i);
		int index=0;
		for (multimap<int, int>::iterator it2 = ppp.first; it2 != ppp.second; ++it2){
			if (state==index)
				return (*it2).second;
			index++;
		}
		throw("Oooops ...");
		return 0; //should never get here			
	}

	/** advances to the next step in the design space exploration on the
	 * y dimmension.
	 * @return true if there is a next state, false if a solution has been
	 *found.
	 */
	bool PolynomialEvaluator::nextStateY(){
		if (! sol){
			aGuard_[degree_] = 0 ;
			int carry = 1;
			bool allMaxBoundsZero = true;
			for (unsigned i=1; i<=degree_;i++){
				if (maxBoundY[i]-1 != 0)
					allMaxBoundsZero = false; 
				if ((yState_[i] == maxBoundY[i]-1) && ( carry==1)){
					yState_[i] = 0;
					carry = 1;
				}else{
					yState_[i]+=carry;
					carry = 0;
				}
			}
				
			for (unsigned i=1; i<=degree_; i++){
				yGuard_[i] = -getPossibleYValue(i,yState_[i]);
			}
					
			if ((carry==1) && (!allMaxBoundsZero)){	
				return false;
			}else
				return true;
		}else
			return false;
	}


	/** advances to the next step in the design space exploration on the
	 * coefficient guard bit direction.
	 * @return true if there is a next state, false if a solution has been
	 *found.
	 */
	bool PolynomialEvaluator::nextStateA(){
		if (! sol){
			int carry = 1;
			for (unsigned i=1; i<=degree_;i++){
				if ((aGuard_[i] == maxBoundA[i]) && ( carry==1)){
					aGuard_[i] = 0;
					carry = 1;
				}else{
					aGuard_[i]+=carry;
					carry=0;
				}
			}
			if ( aGuard_[degree_] == 0)
				return true;
			else
				return false;
		}
		else
			return false;
	}

}



