/*
 * Table Generator unit for FloPoCo
 *
 * Author : Mioara Joldes
 *
 * This file is part of the FloPoCo project developed by the Arenaire
 * team at Ecole Normale Superieure de Lyon

 Initial software.
 Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
 2008-2010.
  All rights reserved.

 All rights reserved
*/


#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <string.h>
#include <gmp.h>
#include <mpfr.h>
#include <cstdlib>
#include <gmpxx.h>
#include "../utils.hpp"
#include "../Operator.hpp"

#ifdef HAVE_SOLLYA
#include "PolyCoeffTable.hpp"

using namespace std;

namespace flopoco{

	PolyCoeffTable::PolyCoeffTable(Target* target, PiecewiseFunction* pf, int wOutX, int n): 
		Table(target), wOutX_(wOutX), pwf(pf){
		
		setCopyrightString("Mioara Joldes (2010)");
		srcFileName = "PolyCoeffTable";

		ostringstream cacheFileName;
		cacheFileName << "Poly_"<<vhdlize(pf->getName()) << "_" << n << "_" << wOutX << ".cache";

		// Test existence of cache file
		fstream file;
		file.open(cacheFileName.str().c_str(), ios::in);
		
		// check for bogus .cache file
		if(file.is_open() && file.peek() == std::ifstream::traits_type::eof())
		{
		  file.close();
		  std::remove(cacheFileName.str().c_str());
		  file.open(cacheFileName.str().c_str(), ios::in);
		}

		if(!file.is_open()){
			//********************** Do the work, then write the cache ********************* 
			REPORT(INFO, "No polynomial data cache found, creating " << cacheFileName.str());
			file.open(cacheFileName.str().c_str(), ios::out);

			/* Start initialization */
			setToolPrecision(165);
			int nrMaxIntervals=1024*1024;

			int	nrFunctions=(pwf->getPiecewiseFunctionArray()).size();
			int	iter;
			int guardBits =1;
		
			/*this is the size of the multipliers to be prefered*/
			/*in the current version this is not used yet */
			int msize =1;
			
			mpfr_t a;
			mpfr_t b;
			mpfr_t eps;
			mpfr_init2(a,getToolPrecision());
			mpfr_init2(b,getToolPrecision());
			mpfr_init2(eps,getToolPrecision());
	
			/* Convert parameters to their required type */
			mpfr_set_d(a,0.,GMP_RNDN);
			mpfr_set_d(b,1.,GMP_RNDN);
			mpfr_set_ui(eps, 1, GMP_RNDN);
			mpfr_mul_2si(eps, eps, -wOutX-1-guardBits, GMP_RNDN); /* eps< 2^{-woutX-1} */
	
	
			vector<sollya_node_t> polys;
			vector<mpfr_t*> errPolys;
	
			sollya_node_t tempNode=0,tempNode2, tempNode3, nDiff=0, sX=0,sY,aiNode;
			sollya_chain_t tempChain, tempChain2=0;
	
			mpfr_t ai;
			mpfr_t bi;
			mpfr_t zero;
			mpfr_t* mpErr;
			int nrIntCompleted=-1;
	
			mpfr_init2(ai,getToolPrecision());
			mpfr_init2(bi,getToolPrecision());
			mpfr_init2(zero,getToolPrecision());
 
			int k, errBoundBool=0;
			tempNode2=parseString("0"); /*ct part*/
			tempChain = makeIntPtrChainFromTo(0,n); /*monomials*/
 
			int sizeList[n+1];
			for (k=0;k<=n;k++)	
				sizeList[k]=0;

			for (iter=0; iter<nrFunctions;iter++){
				Function *fi;
				fi=(pwf->getPiecewiseFunctionArray(iter));
			
				REPORT(INFO, "Now approximating "<<fi->getName());
				/*start with one interval; subdivide until the error is satisfied for all functions involved*/

				int nrIntervals = 256;
				int precShift=7;

//				int nrIntervals = 1;
//				int precShift=0;

				errBoundBool =0;
		
				/*Convert the input string into a sollya evaluation tree*/ 
				tempNode = copyTree(fi->getSollyaNode());
		
				while((errBoundBool==0)&& (nrIntervals <=nrMaxIntervals)){
					errBoundBool=1; 
					REPORT(DETAILED, "Now trying with "<< nrIntervals <<" intervals");
					for (k=0; k<nrIntervals; k++){
						mpfr_set_ui(ai,k,GMP_RNDN);
						mpfr_set_ui(bi,1,GMP_RNDN);
						mpfr_div_ui(ai, ai, nrIntervals, GMP_RNDN);
						mpfr_div_ui(bi, bi, nrIntervals, GMP_RNDN);		
						mpfr_set_ui(zero,0,GMP_RNDN);
			
						aiNode = makeConstant(ai);
						sX = makeAdd(makeVariable(),aiNode);
						sY = substitute(tempNode, sX);
						if (sY == 0)
							REPORT(DEBUG, "Sollya error when performing range mapping.");
						if(verbose>=DEBUG){
							cout<<"\n-------------"<<endl;
							printTree(sY);
							cout<<"\nover: "<<sPrintBinary(zero)<<" "<< sPrintBinary(bi)<<"withprecshift:"<<precShift<<endl;
						}
						tempChain2 = makeIntPtrChainCustomized(wOutX+1+guardBits,n+1, precShift,msize ); //precision
				
						tempNode3 = FPminimax(sY, tempChain ,tempChain2, NULL, zero, bi, FIXED, ABSOLUTESYM, tempNode2,NULL);
						polys.push_back(tempNode3);
				
						if (verbose>=DEBUG){
							printTree(tempNode3);
							printf("\n");
						}
						/*Compute the error*/
						nDiff = makeSub(sY, tempNode3);
						mpErr= (mpfr_t *) safeMalloc(sizeof(mpfr_t));
						mpfr_init2(*mpErr,getToolPrecision());	
						uncertifiedInfnorm(*mpErr, nDiff, zero, bi, 501, getToolPrecision()); 
						if (verbose>=DEBUG){
							cout<< "infinite norm:"<<sPrintBinary(*mpErr)<<endl;
							cout<< "eps:"<<sPrintBinary(eps)<<endl;
						}
						errPolys.push_back(mpErr);
						if (mpfr_cmp(*mpErr, eps)>0) {
							errBoundBool=0; 
							REPORT(DETAILED, tab << "we have found an interval where the error is not small enough");
							REPORT(DETAILED, tab << "failed at interval "<< k+1 << "/" << nrIntervals);
							REPORT(DETAILED, tab << "proceed to splitting"); 
							/*erase the polys and the errors put so far for this function; keep the previous good ones intact*/
							polys.resize(nrIntCompleted+1);
							errPolys.resize(nrIntCompleted+1);
							nrIntervals=2 * nrIntervals;
							precShift=precShift+1;
							break;
						}
					}
				}
				if (errBoundBool==1){ 
					/*we have the good polynomials for one function*/
					REPORT(DETAILED, "the number of intervals is:"<< nrIntervals); 
					REPORT(DETAILED, "We proceed to the next function");
					/*we have to update the total size of the coefficients*/
					int ii=0;
					while (tempChain2!=NULL){
						int sizeNew=*((int *)first(tempChain2));
						tempChain2=tail(tempChain2);
						if (sizeNew>sizeList[ii] )sizeList[ii]= sizeNew;
						ii++;
					}
					nrIntCompleted=nrIntCompleted+nrIntervals;	
					nrIntervalsArray.push_back(intlog2(nrIntervals-1));
				} 
			}
			/*here we have the good polynomials for all the piecewise functions*/
			/*clear some vars used*/

			mpfr_clear(ai);
			mpfr_clear(bi);
			mpfr_clear(zero);
	
			if (errBoundBool==1){
				if(verbose>=DEBUG){
					cout<< "the total number of intervals is:"<< nrIntCompleted<<endl; 
					cout<< "We proceed to the extraction of the coefficients:"<<endl; 
				}

				/*Get the maximum error*/
				mpfr_t *mpErrMax;
				mpErrMax=(mpfr_t*) safeMalloc(sizeof(mpfr_t));
				mpfr_init2(*mpErrMax, getToolPrecision());
				mpfr_set(*mpErrMax,*errPolys[0], GMP_RNDN);
		
				for (k=1;(unsigned)k<errPolys.size();k++){
					if (mpfr_cmp(*mpErrMax, *(errPolys[k]))<0)
						mpfr_set(*mpErrMax,*(errPolys[k]), GMP_RNDN);
				}
		
				maxError=(mpfr_t*) safeMalloc(sizeof(mpfr_t));
				mpfr_init2(*maxError,getToolPrecision());
				mpfr_set(*maxError,*mpErrMax,GMP_RNDN);
		
				mpfr_clear(*mpErrMax);
				free(mpErrMax);
				if (verbose>=DEBUG){
					cout<< "maximum error="<<sPrintBinary(*maxError)<<endl;
				}
				/*Extract coefficients*/
				vector<FixedPointCoefficient*> fpCoeffVector;
		
				k=0;
				for (k=0;(unsigned)k<polys.size();k++){
					if (verbose>=DEBUG){
						cout<<"\n----"<< k<<"th polynomial:----"<<endl;
						printTree(polys[k]);
					}
			
					fpCoeffVector = getPolynomialCoefficients(polys[k], sizeList);
					polyCoeffVector.push_back(fpCoeffVector);
				}
				/*setting of Table parameters*/
				wIn=intlog2(polys.size()-1);
				minIn=0;
				maxIn=polys.size()-1;
				wOut=0;
				for(k=0; (unsigned)k<coeffParamVector.size();k++){
					wOut=wOut+(*coeffParamVector[k]).getSize()+(*coeffParamVector[k]).getWeight()+1; /*a +1 is necessary for the sign*/
				}

				/* Setting up the actual array of values */ 
				buildActualTable();

				REPORT(INFO, "Writing the cache file");
				file << "Polynomial data cache for " << cacheFileName.str() << endl;
				file << "Erasing this file is harmless, but do not try to edit it." <<endl; 
				file << wIn  << endl;
				file << wOut << endl;
				// The approx error
				file << mpfr_get_d(*getMaxApproxError(), GMP_RNDU) << endl;
				// The size of the nrIntervalsArray
				file << nrIntervalsArray.size() << endl;  
				// The values of nrIntervalsArray
				for(k=0; (unsigned)k<nrIntervalsArray.size(); k++) {
					file << nrIntervalsArray[k] << endl;
				}
				// The size of the actual table
				file << actualTable.size() << endl;  
				// The compact array
				for(k=0; (unsigned)k<actualTable.size(); k++) {
					mpz_class r = actualTable[k];
					file << r << endl;
				}
				// The coefficient details
				file << coeffParamVector.size()  << endl;
				for(k=0; (unsigned)k<coeffParamVector.size(); k++) {
					file << coeffParamVector[k]->getSize() << endl;
					file << coeffParamVector[k]->getWeight() << endl;
				}


			}else{
				/*if we didn't manage to have the good polynomials for all the piecewise functions*/
				cout<< "PolyCoeffTable error: Could not approximate the given function(s)"<<endl; 
			}
	
			mpfr_clear(a);
			mpfr_clear(b);
			mpfr_clear(eps);
			free_memory(tempNode);
			free_memory(tempNode2);
			free_memory(nDiff);
			free_memory(sX);
			freeChain(tempChain,freeIntPtr);
			freeChain(tempChain2,freeIntPtr);
		}
		else{
			REPORT(INFO, "Polynomial data cache found: " << cacheFileName.str());
			//********************** Just read the cache ********************* 
			string line;
			int nrIntervals, nrCoeffs;
			getline(file, line); // ignore the first line which is a comment
			getline(file, line); // ignore the second line which is a comment
			file >> wIn; // third line
			file >> wOut; // fourth line
			// The approx error
			double maxApproxError;
			file >> maxApproxError;
			maxError=(mpfr_t*) safeMalloc(sizeof(mpfr_t));
			mpfr_init2(*maxError, 53);
			mpfr_set_d(*maxError, maxApproxError, GMP_RNDU);
			// The size of the nrIntervalsArray
			int nrIntervalsArraySize;
			file >> nrIntervalsArraySize;
			// The values of nrIntervalsArray
			for(int k=0; k<nrIntervalsArraySize; k++) {
				int nrInter;
				file >> nrInter;
				nrIntervalsArray.push_back(nrInter);
			}
			// The size of the actual table
			file >> nrIntervals;
			// The compact array
			for (int i=0; i<nrIntervals; i++){
				mpz_class t;
				file >> t;
				actualTable.push_back(t);
			}
			// these are two attributes of Table.
			minIn=0;
			maxIn=actualTable.size()-1;
			// Now in the file: the coefficient details
			file >> nrCoeffs;
			for (int i=0; i<nrCoeffs; i++){
				FixedPointCoefficient *c;
				int size, weight;
				file >> size;
				file >> weight;
				c = new FixedPointCoefficient(size, weight);
				coeffParamVector.push_back(c);
			}

			//generateDebugPwf();
			if (verbose>=INFO){	
				printPolynomialCoefficientsVector();
				REPORT(DETAILED, "Parameters for polynomial evaluator:");
				printCoeffParamVector();
			}

		}
			
		ostringstream name;
		/*Set up the name of the entity */
		name <<"PolyCoeffTable_"<<wIn<<"_"<<wOut;
		setName(name.str());
		
		/*Set up the IO signals*/
		addInput ("X"	, wIn, true);
		addOutput ("Y"	, wOut, true,1);
		
		/* And that's it: this operator inherits from Table all the VHDL generation */
		nextCycle();//this operator has one cycle delay, in order to be inferred by synthesis tools

		file.close();

	}


	/*This constructor receives the function to be approximated as a string
		Look above to find one that receives the function as a Piecewise function already parsed*/
	PolyCoeffTable::PolyCoeffTable(Target* target, string func,  int wOutX, int n):
		Table(target) {
		
		setCopyrightString("Mioara Joldes (2010)");		

		/*parse the string, create a list of functions, create an array of f's, compute an approximation on each interval*/
		PiecewiseFunction *pf=new PiecewiseFunction(func);
		PolyCoeffTable(target, pf,  wOutX, n);
	}





	PolyCoeffTable::~PolyCoeffTable() {
	}





	MPPolynomial* PolyCoeffTable::getMPPolynomial(sollya_node_t t){
		int degree=1,i;
		sollya_node_t *nCoef;
		mpfr_t *coef;
		
		//printTree(t);
		getCoefficients(&degree, &nCoef, t);
		//cout<<degree<<endl;
		coef = (mpfr_t *) safeCalloc(degree+1,sizeof(mpfr_t));
		
			
		for (i = 0; i <= degree; i++){
			mpfr_init2(coef[i], getToolPrecision());
			//cout<< i<<"th coeff:"<<endl;
			//printTree(getIthCoefficient(t, i));
			evaluateConstantExpression(coef[i], getIthCoefficient(t, i), getToolPrecision());
			if (verbose>=DEBUG){
				cout<< i<<"th coeff:"<<sPrintBinary(coef[i])<<endl;
			}
		}
		MPPolynomial* mpPx = new MPPolynomial(degree, coef);
		//Cleanup 
		for (i = 0; i <= degree; i++)
			mpfr_clear(coef[i]);
		free(coef);
		
		return mpPx;
	}




	vector<FixedPointCoefficient*> PolyCoeffTable::getPolynomialCoefficients(sollya_node_t t, sollya_chain_t c){
		int degree=1,i,size, weight;
		sollya_node_t *nCoef;
		mpfr_t *coef;
		sollya_chain_t cc;
		vector<FixedPointCoefficient*> coeffVector;
		FixedPointCoefficient* zz;
		
		//printTree(t);
		getCoefficients(&degree, &nCoef, t);
		//cout<<degree<<endl;
		coef = (mpfr_t *) safeCalloc(degree+1,sizeof(mpfr_t));
		cc=c;
			
		for (i = 0; i <= degree; i++)
			{
				mpfr_init2(coef[i], getToolPrecision());
				//cout<< i<<"th coeff:"<<endl;
				//printTree(getIthCoefficient(t, i));
				evaluateConstantExpression(coef[i], getIthCoefficient(t, i), getToolPrecision());
				if (verbose>=DEBUG){
					cout<< i<<"th coeff:"<<sPrintBinary(coef[i])<<endl;
				}
				size=*((int *)first(cc));
				cc=tail(cc);
				if (mpfr_sgn(coef[i])==0){
					weight=0;
					size=1;
				} 
				else 
					weight=mpfr_get_exp(coef[i]);
			
				zz= new FixedPointCoefficient(size, weight, coef[i]);
				coeffVector.push_back(zz);
				updateMinWeightParam(i,zz);
			}
			
		//Cleanup 
		for (i = 0; i <= degree; i++)
			mpfr_clear(coef[i]);
		free(coef);
			
		return coeffVector;
	}


	vector<FixedPointCoefficient*> PolyCoeffTable::getPolynomialCoefficients(sollya_node_t t, int* sizeList){
		int degree=1,i,size, weight;
		sollya_node_t *nCoef;
		mpfr_t *coef;
		
		vector<FixedPointCoefficient*> coeffVector;
		FixedPointCoefficient* zz;
		
		//printTree(t);
		getCoefficients(&degree, &nCoef, t);
		//cout<<degree<<endl;
		coef = (mpfr_t *) safeCalloc(degree+1,sizeof(mpfr_t));
		
		for (i = 0; i <= degree; i++)
			{
				mpfr_init2(coef[i], getToolPrecision());
				//cout<< i<<"th coeff:"<<endl;
				//printTree(getIthCoefficient(t, i));
				evaluateConstantExpression(coef[i], getIthCoefficient(t, i), getToolPrecision());
				if (verbose>=DEBUG){
					cout<< i<<"th coeff:"<<sPrintBinary(coef[i])<<endl;
				}
				size=sizeList[i];
			
				if (mpfr_sgn(coef[i])==0){
					weight=0;
					size=1;
				} 
				else 
					weight=mpfr_get_exp(coef[i]);
			
				zz= new FixedPointCoefficient(size, weight, coef[i]);
				coeffVector.push_back(zz);
				updateMinWeightParam(i,zz);
			}
			
		//Cleanup 
		for (i = 0; i <= degree; i++)
			mpfr_clear(coef[i]);
		free(coef);
		
		return coeffVector;
	}


	void PolyCoeffTable::updateMinWeightParam(int i, FixedPointCoefficient* zz)
	{
		if (coeffParamVector.size()<=(unsigned)i) {
			coeffParamVector.push_back(zz);
		}
		else if (mpfr_sgn((*(*coeffParamVector[i]).getValueMpfr()))==0) coeffParamVector[i]=zz;
		else if ( ((*coeffParamVector[i]).getWeight() <(*zz).getWeight()) && (mpfr_sgn(*((*zz).getValueMpfr()))!=0) )
			coeffParamVector[i]=zz;
	}


	vector<vector<FixedPointCoefficient*> > PolyCoeffTable::getPolynomialCoefficientsVector(){
		return polyCoeffVector;
	}


	void PolyCoeffTable::printPolynomialCoefficientsVector(){
		int i,j,nrIntervals, degree;
		vector<FixedPointCoefficient*> pcoeffs;
		nrIntervals=polyCoeffVector.size();

		for (i=0; i<nrIntervals; i++){	
			pcoeffs=polyCoeffVector[i];
			degree= pcoeffs.size();
			REPORT(DEBUG, "polynomial "<<i<<": ");
			for (j=0; j<degree; j++){
				REPORT(DEBUG, " "<<(*pcoeffs[j]).getSize()<< " "<<(*pcoeffs[j]).getWeight());
			}
		}
	}


	vector<FixedPointCoefficient*> PolyCoeffTable::getCoeffParamVector(){
		return coeffParamVector;
	}


	void PolyCoeffTable::printCoeffParamVector(){
		int j, degree;
		degree= coeffParamVector.size();
		for (j=0; j<degree; j++){		
			REPORT(DETAILED, " "<<(*coeffParamVector[j]).getSize()<< " "<<(*coeffParamVector[j]).getWeight()); 
		}
	}


	mpfr_t * PolyCoeffTable::getMaxApproxError(){
		return maxError;
	}


	void PolyCoeffTable::generateDebug(){
		int j;
		cout<<"f=";
		printTree(simplifyTreeErrorfree(f->getSollyaNode()));
		cout<<" wOut="<<(-1)*wOutX_<<endl;
		cout<<"k="<<polyCoeffVector.size()<<" d="<<coeffParamVector.size()<<endl;
		cout<<"The size of the coefficients is:"<<endl;
		for (j=0; (unsigned)j<coeffParamVector.size(); j++){
			cout<<"c"<<j<<":"<<(*coeffParamVector[j]).getSize()+(*coeffParamVector[j]).getWeight()+1<<endl; 
		}
	}


	vector<int> PolyCoeffTable::getNrIntArray(){
		return nrIntervalsArray;
	}


	void PolyCoeffTable::generateDebugPwf(){
		int j;
		cout<<"pwf=";
		cout<<pwf->getName()<<endl;
		cout<<" wOut="<<(-1)*wOutX_<<endl;
		cout<<"k="<<polyCoeffVector.size()<<" d="<<coeffParamVector.size()<<endl;
		cout<<"The size of the branch is:"<<endl;
		for (j=0; (unsigned)j<getNrIntArray().size(); j++){
			cout<<j<<":"<<(getNrIntArray())[j]<<endl;
		}

		cout<<"The size of the coefficients is:"<<endl;
		for (j=0; (unsigned)j<coeffParamVector.size(); j++){
			cout<<"c"<<j<<":"<<(*coeffParamVector[j]).getSize()+(*coeffParamVector[j]).getWeight()+1<<endl; 
		}
	}


	sollya_chain_t PolyCoeffTable::makeIntPtrChainCustomized(int m, int n, int precshift, int msize) {
		int i,j, temp;
		int *elem;
		sollya_chain_t c;
		int tempTable[n+1];
		tempTable[0]=m;
		for (i=1; i<n; i++){
			temp=(tempTable[i-1]-precshift)/msize;
			if (temp!=0)
				tempTable[i]=temp*msize ;
			else	
				tempTable[i]=(tempTable[i-1]-precshift);
		}

		//		tempTable[0]+=2;		
		//		tempTable[1]+=2;
		//		tempTable[2]+=2;
		//		tempTable[4]+=2;
		tempTable[3]+=1;
		
	
		c = NULL;
	 
		for(j=n-1;j>=0;j--) {
			elem = (int *) malloc(sizeof(int));
			*elem = tempTable[j];
			c = addElement(c,elem);
		}
		return c;
	}


	/****************************************************************************************/
	/************Implementation of virtual methods of Class Table***************************/
	int PolyCoeffTable::double2input(double x){
		int result;
		cerr << "???	PolyCoeffTable::double2input not yet implemented ";
		exit(1);
		return result;
	}


	double	PolyCoeffTable::input2double(int x) {
		double y;
		cerr << "??? PolyCoeffTable::double2input not yet implemented ";
		exit(1);
		return(y);
	}


	mpz_class	PolyCoeffTable::double2output(double x){
		cerr << "???	PolyCoeffTable::double2input not yet implemented ";
		exit(1);
		return 0;
	}


	double	PolyCoeffTable::output2double(mpz_class x) {
		double y;
		cerr << "???	PolyCoeffTable::double2input not yet implemented ";
		exit(1);
	
		return(y);
	}


	void PolyCoeffTable::buildActualTable() {
		unsigned int x;
		mpz_class r=0; mpfr_t *cf; mpz_t c; char * z;
		int amount,j,nrIntervals, degree,trailingZeros,numberSize;
		vector<FixedPointCoefficient*> pcoeffs;
		nrIntervals=polyCoeffVector.size();
		for(x=0; x<(unsigned)nrIntervals; x++){
			r=0;
			pcoeffs=polyCoeffVector[(unsigned)x];
			degree= pcoeffs.size();
			amount=0;
			//cout<<"polynomial "<<x<<": "<<endl;
			//r=mpz_class( 133955332 )+(mpz_class( 65664 )<< 27 )+(mpz_class( 64 )<< 44 )
			for (j=0; j<degree; j++){		
				//cout<<" "<<(*pcoeffs[j]).getSize()<< " "<<(*pcoeffs[j]).getWeight()<<endl; 
				r=r+(mpz_class(0)<<amount);
				
				
				cf=(*pcoeffs[j]).getValueMpfr();
				
				//cout<< j<<"th coeff:"<<sPrintBinary(*cf)<<endl;
				z=sPrintBinaryZ(*cf);
				//cout<< j<<"th coeff:"<<z<<" "<<strlen(z)<<endl;
				mpz_init(c);
				if (mpfr_sgn(*cf)!=0) {
					
					trailingZeros=(*coeffParamVector[j]).getSize()+(*pcoeffs[j]).getWeight()-strlen(z);
					//cout<<"Trailing zeros="<< trailingZeros<<endl;
					numberSize= (*coeffParamVector[j]).getSize()+(*coeffParamVector[j]).getWeight()+1 ;
					//mpz_set_str (mpz_t rop, char *str, int base) 
					mpz_set_str (c,z,2);
					if (mpfr_sgn(*cf)<0) {
						if (j==0)
							r=r+(((mpz_class(1)<<numberSize) -	((mpz_class(c)<<trailingZeros) - (mpz_class(1)<<2) ) )<<amount);
						else
							r=r+(((mpz_class(1)<<numberSize) -	(mpz_class(c)<<trailingZeros)	)<<amount);
					}
					else{
						if (j==0)
							r=r+(((mpz_class(c)<<trailingZeros) + (mpz_class(1)<<2) )<<amount);
						else
							r=r+((mpz_class(c)<<trailingZeros)<<amount);							
					}
					
				}
				else{
					if (j==0)
 						r=r+(((mpz_class(0)) + (mpz_class(1)<<2))<<amount);
					else
						r=r+(mpz_class(0)<<amount);
				} 
				
				amount=amount+(*coeffParamVector[j]).getSize()+(*coeffParamVector[j]).getWeight()+1;
			}
			//cout << x << "  " << r << endl;
			actualTable.push_back(r);
		}
	}

	/***************************************************************************************/
	/********************This is the implementation of the actual mapping*******************/
	mpz_class	PolyCoeffTable::function(int x)
	{  
		// if ((x<0) ||(x>=nrIntervals)) {
		// 	//TODO an error here
		// }
		return actualTable[x];
	}
	/***************************************************************************************/

}
#endif //HAVE_SOLLYA


