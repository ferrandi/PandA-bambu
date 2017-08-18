/*
  Floating-point pipeline generator for FloPoCo

  Author : Bogdan Pasca, Florent de Dinechin

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
#include "utils.hpp"
#include "Operator.hpp"

#ifdef HAVE_SOLLYA
#include "FPPipeline.hpp"

#include "ExpressionParser.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int FlopocoExpressionlex(void);
int FlopocoExpressionparse(void);
int FlopocoExpressionerror(char *);
program* p;


//#define sumeofsquaresNaive
//#define sumeofsquares
//#define polynomial
//#define sqrtx2y2
//#define sqrtx2y2z2		

using namespace std;

namespace flopoco{
	
	FPPipeline::FPPipeline(Target* target, string filename, int wE_, int wF_): 
		Operator(target), wE(wE_), wF(wF_) {
		// Name HAS to be unique!
		// will cause weird bugs otherwise
		ostringstream complete_name;
		complete_name << "Pipeline" << getNewUId(); 
		setName(complete_name.str());
		// r = x^2 + y^2 + z^2 example
		srcFileName = "FPPipeline";

#ifdef sumeofsquares		
		vector<FPNode*> fpNodeList;
		
		vector<string> inx, iny, inz;
		inx.push_back("x");
		iny.push_back("y");
		inz.push_back("z");

		FPNode *nx = new FPNode(FPNode::input, inx);
		FPNode *ny = new FPNode(FPNode::input, iny);
		FPNode *nz = new FPNode(FPNode::input, inz);
		
		vector<FPNode*> vsqrx, vsqry,vsqrz;
		vsqrx.push_back(nx);
		vsqry.push_back(ny);
		vsqrz.push_back(nz);

		FPNode *nxsqr = new FPNode(FPNode::sqr, vsqrx);
		FPNode *nysqr = new FPNode(FPNode::sqr, vsqry);
		FPNode *nzsqr = new FPNode(FPNode::sqr, vsqrz);
		
		vector<FPNode*> vadd1;
		vadd1.push_back(nxsqr);
		vadd1.push_back(nysqr);
		
		FPNode *nadd1 = new FPNode(FPNode::adder, vadd1);
		
		vector<FPNode*> vadd2;
		vadd2.push_back(nadd1);
		vadd2.push_back(nzsqr);
		
		FPNode *nadd2 = new FPNode(FPNode::adder, vadd2, "r", true);
		fpNodeList.push_back(nadd2);
#endif

#ifdef sumeofsquaresNaive		
		vector<FPNode*> fpNodeList;
		
		vector<string> inx, iny, inz;
		inx.push_back("x");
		iny.push_back("y");
		inz.push_back("z");

		FPNode *nx = new FPNode(FPNode::input, inx);
		FPNode *ny = new FPNode(FPNode::input, iny);
		FPNode *nz = new FPNode(FPNode::input, inz);
		
		vector<FPNode*> vsqrx, vsqry,vsqrz;
		vsqrx.push_back(nx);
		vsqrx.push_back(nx);
		
		vsqry.push_back(ny);
		vsqry.push_back(ny);
		
		vsqrz.push_back(nz);
		vsqrz.push_back(nz);

		FPNode *nxsqr = new FPNode(FPNode::multiplier, vsqrx);
		FPNode *nysqr = new FPNode(FPNode::multiplier, vsqry);
		FPNode *nzsqr = new FPNode(FPNode::multiplier, vsqrz);
		
		vector<FPNode*> vadd1;
		vadd1.push_back(nxsqr);
		vadd1.push_back(nysqr);
		
		FPNode *nadd1 = new FPNode(FPNode::adder, vadd1);
		
		vector<FPNode*> vadd2;
		vadd2.push_back(nadd1);
		vadd2.push_back(nzsqr);
		
		FPNode *nadd2 = new FPNode(FPNode::adder, vadd2, "r", true);
		fpNodeList.push_back(nadd2);
#endif


#ifdef polynomial		
		vector<FPNode*> fpNodeList;
		
		vector<string> inx;
		inx.push_back("x");
		FPNode *nx = new FPNode(FPNode::input, inx);

		vector<FPNode*> vsqrx;
		vsqrx.push_back(nx);
		FPNode *sqrx = new FPNode(FPNode::sqr, vsqrx);
		
		vector<string> ina2;
		ina2.push_back("a2");
		FPNode *na2 = new FPNode(FPNode::input, ina2);

		vector<FPNode*> vprod1;
		vprod1.push_back(sqrx);
		vprod1.push_back(na2);
		
		FPNode *nprod1 = new FPNode(FPNode::multiplier, vprod1);




		vector<string> ina1;
		ina1.push_back("a1");
		FPNode *na1 = new FPNode(FPNode::input, ina1);

		vector<FPNode*> vprod2;
		vprod2.push_back(nx);
		vprod2.push_back(na1);
		
		FPNode *nprod2 = new FPNode(FPNode::multiplier, vprod2);

		vector<string> ina0;
		ina0.push_back("a0");
		FPNode *na0 = new FPNode(FPNode::input, ina0);

		vector<FPNode*> vadd1;
		vadd1.push_back(na0);
		vadd1.push_back(nprod2);
		
		FPNode *nadd1 = new FPNode(FPNode::adder, vadd1);


		vector<FPNode*> vadd2;
		vadd2.push_back(nadd1);
		vadd2.push_back(nprod1);
		
		FPNode *nadd2 = new FPNode(FPNode::adder, vadd2, "r", true);
		fpNodeList.push_back(nadd2);

#endif


#ifdef sqrtx2y2		
		vector<FPNode*> fpNodeList;
		
		vector<string> inx;
		inx.push_back("x");
		FPNode *nx = new FPNode(FPNode::input, inx);

		vector<FPNode*> vsqrx;
		vsqrx.push_back(nx);
		FPNode *sqrx = new FPNode(FPNode::sqr, vsqrx);

		vector<string> iny;
		iny.push_back("y");
		FPNode *ny = new FPNode(FPNode::input, iny);

		vector<FPNode*> vsqry;
		vsqry.push_back(ny);
		FPNode *sqry = new FPNode(FPNode::sqr, vsqry);

		vector<FPNode*> vadd1;
		vadd1.push_back(sqrx);
		vadd1.push_back(sqry);
		
		FPNode *nadd1 = new FPNode(FPNode::adder, vadd1);

		vector<FPNode*> vsqrt1;
		vsqrt1.push_back(nadd1);

		FPNode *nsqrt1 = new FPNode(FPNode::sqrt, vsqrt1, "r", true);
		fpNodeList.push_back(nsqrt1);

#endif

#ifdef sqrtx2y2z2		
		vector<FPNode*> fpNodeList;
		
		vector<string> inx;
		inx.push_back("x");
		FPNode *nx = new FPNode(FPNode::input, inx);

		vector<FPNode*> vsqrx;
		vsqrx.push_back(nx);
		FPNode *nsqrx = new FPNode(FPNode::sqr, vsqrx);

		vector<string> iny;
		iny.push_back("y");
		FPNode *ny = new FPNode(FPNode::input, iny);

		vector<FPNode*> vsqry;
		vsqry.push_back(ny);
		FPNode *nsqry = new FPNode(FPNode::sqr, vsqry);

		vector<string> inz;
		inz.push_back("z");
		FPNode *nz = new FPNode(FPNode::input, inz);

		vector<FPNode*> vsqrz;
		vsqrz.push_back(nz);
		FPNode *nsqrz = new FPNode(FPNode::sqr, vsqrz);

		vector<FPNode*> vadd1;
		vadd1.push_back(nsqrx);
		vadd1.push_back(nsqry);
		FPNode *nadd1 = new FPNode(FPNode::adder, vadd1);

		vector<FPNode*> vadd2;
		vadd2.push_back(nadd1);
		vadd2.push_back(nsqrz);
		FPNode *nadd2 = new FPNode(FPNode::adder, vadd2);

		vector<FPNode*> vsqrt1;
		vsqrt1.push_back(nadd2);

		FPNode *nsqrt1 = new FPNode(FPNode::sqrt, vsqrt1, "r", true);
		fpNodeList.push_back(nsqrt1);

#endif
		// redirect stdin to the file pointer
		int my_stdin = dup(0);
		close(0);
		int fp = open(filename.c_str(), O_RDONLY, "r");

		dup2(fp, 0);
		FlopocoExpressionparse();
		close(fp);
		dup2(0, my_stdin);
	
		REPORT(DEBUG, "-----------------------------------");
		nodeList* head = p->assignList;
		while (head!=NULL){
			printExpression(head->n); 	
			REPORT(DEBUG,endl);			
			head = head->next;
		}
		REPORT(DEBUG, "-----------------------------------");
		varList* headv = p->outVariableList;
		while (headv != NULL){
			REPORT(DEBUG, "out: variable " << headv->name	<< ";");
			headv = headv->next;
		}
		REPORT(DEBUG, "-----------------------------------");	
		head = p->assignList;
		/* creates the computational tree our of the assignment list, by linking 
		   all variables already declared to their use */
		makeComputationalTree(NULL, head, head); 

		REPORT(DEBUG, "NEW NODES: ------------------------");
		head = p->assignList;
		while (head!=NULL){
			printExpression(head->n); 	
			REPORT(DEBUG,endl);			
			head = head->next;
		}
		REPORT(DEBUG, "-----------------------------------");	
	
		/* create a node output list, where each node is a computational datapath.
		   if in the user-provided outputList, some of the variables are part of 
		   intermediary computations for one, say larger, node these will not be added
		   to the new list */

		nodeList* outList = createOuputList(p->assignList, p->outVariableList);

		REPORT(DEBUG, "PROPER OUT LIST: ------------------");
		nodeList* outListHead = outList;
		while (outListHead != NULL){
			printExpression( outListHead->n);
			outListHead = outListHead->next;
		} 
		REPORT(DEBUG,endl);			
	
		nodeList* oh = outList;
	
		while (oh!=NULL){
			generateVHDL_c( oh->n, true);
			oh = oh->next;	
		}
	}
	
	FPPipeline::~FPPipeline() {
	}

	void FPPipeline::generateVHDL_c(node* n, bool top){
#if 0
		REPORT(DETAILED, "Generating VHDL for node " << n->name );
#else
		REPORT(DETAILED, "Generating VHDL" );
#endif				
		if (n->type == 0){
			//we start at cycle 0, for now
			setCycle(0);
			//check if inputs are already declared. if not declare the inputs
			if (n->name!=NULL){
				if (!isSignalDeclared(n->name)){
					REPORT(DETAILED, "signal " << n->name << "   declared");
					addFPInput(n->name, wE, wF);
				}
			}else{
				//this is a constant, so it has no name, and is not declared
			}
		}else{
			//iterate on all inputs
			nodeList* lh = n->nodeArray;
			while (lh!=NULL){
				generateVHDL_c(lh->n, false);
				lh=lh->next;
			}
			lh = n->nodeArray;
			while (lh!=NULL){
				if (lh->n->name!=NULL)
					syncCycleFromSignal(lh->n->name);
				lh=lh->next;
			}
			REPORT(DETAILED, "finished with node");
		}
				
		bool hadNoName = (n->name==NULL);
				
		if (n->name==NULL){
			//assign a unique name;
			ostringstream t;
			t << "tmp_var_"<<getNewUId();
			string w = t.str();
			char *c  = new char[t.str().length()+1];
			c = strncpy(c, t.str().c_str(), t.str().length() );
			c[t.str().length()]=0;
			REPORT(DETAILED, " new temporary variable created "<< c <<" size="<<t.str().length());
			n->name = c;
			REPORT(DETAILED, " the value was created for the constant " << n->value);
		}
				
		if ((hadNoName)&&(n->type == 0)){
			//it is a constant_expr
			mpfr_t mpx;
			mpfr_init2 (mpx, wF+1);
			mpfr_set_str (mpx, n->s_value, 10, GMP_RNDN);
			vhdl << tab << declare(n->name, wE+wF+3) << " <= \""<<fp2bin(mpx, wE, wF)<< "\";"<<endl;
		}

		ostringstream t;				
		if (n->isOutput){
			t << "out_" << n->name;
			addFPOutput(t.str(), wE, wF);
		}

				
		Operator* op1;
		//let's instantiate the proper operator

		switch (n->type) {

		case 0:{  //input
			break;
		}
					
		case 1:{ //adder 
			REPORT(DETAILED, " instance adder");
						
			op1 = new FPAdderSinglePath(target_, wE, wF, wE, wF, wE, wF);
			oplist.push_back(op1);

			inPortMap( op1, "X", n->nodeArray->n->name);
			inPortMap( op1, "Y", n->nodeArray->next->n->name);
			outPortMap( op1, "R", n->name);
						
			ostringstream tmp;
			tmp << "adder" << getNewUId();
			vhdl << instance(op1, tmp.str())<<endl;
			break;
		}

		case 2:{ //subtracter 
			REPORT(DETAILED, " instance subtracter");
						
			op1 = new FPAdderSinglePath(target_, wE, wF, wE, wF, wE, wF);
			oplist.push_back(op1);

			ostringstream temp;
			temp << "minus_"<<n->nodeArray->next->n->name;
			vhdl<<tab<<declare(temp.str(),wE+wF+3) << " <= " << n->nodeArray->next->n->name <<range(wE+wF+2,wE+wF+1)
			    << " & not(" << n->nodeArray->next->n->name <<of(wE+wF)<<")"
			    << " & " << n->nodeArray->next->n->name << range(wE+wF-1,0)<<";"<<endl;  

			inPortMap( op1, "X", n->nodeArray->n->name);
			inPortMap( op1, "Y", temp.str());
			outPortMap( op1, "R", n->name);
						
			ostringstream tmp;
			tmp << "adder" << getNewUId();
			vhdl << instance(op1, tmp.str())<<endl;
			break;
		}

		case 3:{ //multiplier
			REPORT(DETAILED, " instance multiplier");
			if (((n->nodeArray->n->type==0)&&(n->nodeArray->n->s_value!=NULL)) || 
			    ((n->nodeArray->next->n->type==0)&&(n->nodeArray->next->n->s_value!=NULL))){
				REPORT(INFO, "constant node detected");
				ostringstream constant_expr, operand_name;
				if ((n->nodeArray->n->type==0)&&(n->nodeArray->n->s_value!=NULL)){
					//the first one is the constant
					constant_expr << n->nodeArray->n->s_value;
					operand_name << n->nodeArray->next->n->name;		
				}else{
					constant_expr << n->nodeArray->next->n->s_value;
					operand_name << n->nodeArray->n->name;		
				}
							
				REPORT(INFO, "Constant is "<< constant_expr.str());

#if 0 // Not that it is bad, but it poorly handles special cases such as mult by 2
				op1 = new FPRealKCM(target_, wE, wF, constant_expr.str());
#else
				//				cout << "JLTBB" << endl;
				op1 = new FPConstMult(target_, 
				                      wE, wF, // in 
				                      wE, wF, // out
				                      wF+2 ,  // constant precision 
				                      constant_expr.str()  );
				//				cout << "JLTBB" << endl;
#endif
				oplist.push_back(op1);
							
				inPortMap( op1, "X", operand_name.str());
				outPortMap( op1, "R", n->name);

				ostringstream tmp;
				tmp << "constant_multiplier" << getNewUId();
				vhdl << instance(op1, tmp.str())<<endl;
			}else{
				//we just plug-in a regular multiplier
				op1 = new FPMultiplier(target_, wE, wF, wE, wF, wE, wF);
				oplist.push_back(op1);

				inPortMap( op1, "X", n->nodeArray->n->name);
				inPortMap( op1, "Y", n->nodeArray->next->n->name);
				outPortMap( op1, "R", n->name);
						
				ostringstream tmp;
				tmp << "multiplier" << getNewUId();
				vhdl << instance(op1, tmp.str())<<endl;
			}
			break;
		}

		case 4:{ //divider 
			REPORT(DETAILED, " instance divider");
						
			op1 = new FPDiv(target_, wE, wF);
			oplist.push_back(op1);

			inPortMap( op1, "X", n->nodeArray->n->name);
			inPortMap( op1, "Y", n->nodeArray->next->n->name);
			outPortMap( op1, "R", n->name);
						
			ostringstream tmp;
			tmp << "divider" << getNewUId();
			vhdl << instance(op1, tmp.str())<<endl;
			break;
		}

		case 5:{ //squarer
			REPORT(DETAILED, " instance squarer");
						
			op1 = new FPSquarer(target_, wE, wF, wF);
			oplist.push_back(op1);

			inPortMap( op1, "X", n->nodeArray->n->name);
			outPortMap( op1, "R", n->name);
						
			ostringstream tmp;
			tmp << "squarer" << getNewUId();
			vhdl << instance(op1, tmp.str())<<endl;
			break;
		}
		case 6:{ //sqrt
			REPORT(DETAILED, " instance sqrt");
#ifdef ha
			int degree = int ( floor ( double(wF) / 10.0) );
			op1 = new FPSqrtPoly(target_, wE, wF, 0, degree);
#else
			op1 = new FPSqrt(target_, wE, wF);//, 1, degree);
#endif
			oplist.push_back(op1);
						
			inPortMap( op1, "X", n->nodeArray->n->name);
			outPortMap( op1, "R", n->name);
						
			ostringstream tmp;
			tmp << "squarer" << getNewUId();
			vhdl << instance(op1, tmp.str())<<endl;
			break;
		}
		case 7:{ //exponential
			REPORT(DETAILED, " instance exp");
						
			op1 = new FPExp(target_, wE, wF, 0, 0);
			oplist.push_back(op1);

			inPortMap( op1, "X", n->nodeArray->n->name);
			outPortMap( op1, "R", n->name);
						
			ostringstream tmp;
			tmp << "exponential" << getNewUId();
			vhdl << instance(op1, tmp.str())<<endl;
			break;
		}
		case 8:{ //logarithm
			REPORT(DETAILED, " instance log");
						
			op1 = new FPLog(target_, wE, wF, 9);
			oplist.push_back(op1);

			inPortMap( op1, "X", n->nodeArray->n->name);
			outPortMap( op1, "R", n->name);
						
			ostringstream tmp;
			tmp << "logarithm" << getNewUId();
			vhdl << instance(op1, tmp.str())<<endl;
			break;
		}
		case 17:{ //assignement
			vhdl << tab << declare( n->name, wE+wF+3) << "<= " << n->nodeArray->n->name <<";"<<endl;
			break;
		}

		default:{
						
			cerr << "nothing else implemented yet for operation code: "<<n->type << endl;
			exit(-1);
		}
		}
				
		if (n->isOutput){
			syncCycleFromSignal(n->name);
			nextCycle();
			vhdl << tab << "out_"<<n->name << " <= " << n->name << ";" << endl;
		}
				
	};

	
}
#endif //HAVE_SOLLYA


