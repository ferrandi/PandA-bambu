%code requires{
	#include "ExpressionParserData.h"
	extern program* p;
}

%{
#include <stdio.h>
#include <iostream>
#include <map>


int FlopocoExpressionlex(void);
int FlopocoExpressionparse(void);
int FlopocoExpressionerror(char *);
%}

%union{
	char c_type;
	int i_type;
	double d_type;
	char* s_type;
	node* thisNode;         // each node is either a leaf or an assignement
	nodeList* thisNodeList; //list containing the assignments
	varList*  thisVarList; //list containing the assignments
	program* theProgram; //assignment list + output variables
}


%token FPEXPRESSION_UNKNOWN FPEXPRESSION_PV FPEXPRESSION_OUTPUT FPEXPRESSION_LPAR FPEXPRESSION_RPAR FPEXPRESSION_SQR FPEXPRESSION_SQRT FPEXPRESSION_EXP FPEXPRESSION_LOG
%token <c_type> FPEXPRESSION_PLUS FPEXPRESSION_MINUS FPEXPRESSION_TIMES FPEXPRESSION_DIV FPEXPRESSION_EQUALS 
%token <s_type> FPEXPRESSION_FPNUMBER
%token <s_type> FPEXPRESSION_VARIABLE

%type <thisNode> factor expression expressionp term termp assignment
%type <thisNodeList> assignmentList 
%type <thisVarList> varlist outputList
%type <theProgram> code

%start code

%%

code: assignmentList outputList { 	program* ptemp;
									ptemp= (program*) malloc( sizeof(program));
									ptemp->assignList = $1;
									ptemp->outVariableList = $2;
									$$ = ptemp;
									p = $$;
									//std::cout << "parse went fine" << std::endl; 
								}
	;

assignmentList: assignmentList assignment { 
											if ($1!=NULL){
												$$=appendNodeToListBack($2, $1);
											}else{
												$$=createNodeList($2);
											}
										}
	| { $$=NULL;}
	;

assignment: FPEXPRESSION_VARIABLE FPEXPRESSION_EQUALS expression FPEXPRESSION_PV { $3->name=$1;
											$$ = $3; 
											//std::cout << " Valid assignment;" << std::endl;
										}
	;

expression: term expressionp { 
									if ($2==NULL){
										// this is just an assignement
										$$=createNodeHavingRightOperandOnly($1,17); //assignment code is 17
									}else{
										//fill the rest of the node;
										$2->nodeArray = appendNodeToListFront($1,$2->nodeArray);
										$$=$2;
									} 
							 }
	;

expressionp: FPEXPRESSION_PLUS term expressionp {
										//std::cout << "here" << std::endl;
										//1 stands for +. To replace by the flopoco structure with operations
										if ($3!=NULL){
											//complete right node
											$3->nodeArray = appendNodeToListFront($2, $3->nodeArray); 										
											//create new one
											$$=createNodeHavingRightOperandOnly($3,1);	
										}else{
											$$=createNodeHavingRightOperandOnly($2,1);									
										}
									}
	| FPEXPRESSION_MINUS term expressionp        {
										//2 stands for -. To replace by the flopoco structure with operations
										if ($3!=NULL){
											//complete right node
//											$3->nodeArray[0] = $2; 
											$3->nodeArray = appendNodeToListFront($2, $3->nodeArray); 										
											$$=createNodeHavingRightOperandOnly($3,2);	
										}else{
											$$=createNodeHavingRightOperandOnly($2,2);									
										}
									}
	|								{ $$ = NULL;  }
	;

term: factor termp {
									if ($2==NULL){
										$$ = $1;
									}else{
										//fill the rest of the node;
										$2->nodeArray = appendNodeToListFront($1, $2->nodeArray); 
//										$2->nodeArray[0]=$1;
										$$=$2;
									} 
					}
	;
	
termp: FPEXPRESSION_TIMES factor termp
									{
										//3 stands for *. To replace by the flopoco structure with operations
										if ($3!=NULL){
											//complete right node
//											$3->nodeArray[0] = $2;
											$3->nodeArray = appendNodeToListFront($2, $3->nodeArray); 										
											$$=createNodeHavingRightOperandOnly($3,3);	
										}else{
											$$=createNodeHavingRightOperandOnly($2,3);									
										}
									}
	|FPEXPRESSION_DIV factor termp
									{
										//4 stands for *. To replace by the flopoco structure with operations
										if ($3!=NULL){
											//complete right node
//											$3->nodeArray[0] = $2; 
											$3->nodeArray = appendNodeToListFront($2, $3->nodeArray);										
											$$=createNodeHavingRightOperandOnly($3,4);	
										}else{
											$$=createNodeHavingRightOperandOnly($2,4);									
										}
									}	
	|								{ $$ = NULL; 
									}
	;

factor: FPEXPRESSION_SQRT FPEXPRESSION_LPAR expression FPEXPRESSION_RPAR{
								//std::cout << "creating sqrt node " << std::endl;  
	                         $$=createNodeHavingRightOperandOnly($3,6);
	                         }
	| FPEXPRESSION_SQR FPEXPRESSION_LPAR expression FPEXPRESSION_RPAR{   
							//std::cout << "creating sqr node " << std::endl;
	                         $$=createNodeHavingRightOperandOnly($3,5);
	                         } 
	| FPEXPRESSION_EXP FPEXPRESSION_LPAR expression FPEXPRESSION_RPAR{   
							//std::cout << "creating exp node " << std::endl;
	                         $$=createNodeHavingRightOperandOnly($3,7);
	                         }
	| FPEXPRESSION_LOG FPEXPRESSION_LPAR expression FPEXPRESSION_RPAR{   
							//std::cout << "creating log node " << std::endl;
	                         $$=createNodeHavingRightOperandOnly($3,8);
	                         }	                          	                         
	|FPEXPRESSION_VARIABLE {  
					$$=createVariableNode($1);
				 }
	| FPEXPRESSION_FPNUMBER     { $$=createConstantNode($1); }
	| FPEXPRESSION_LPAR expression FPEXPRESSION_RPAR { $$=$2; }
	;

outputList: FPEXPRESSION_OUTPUT varlist FPEXPRESSION_PV  { $$ = $2; }
	;

varlist: varlist FPEXPRESSION_VARIABLE {
								if ($1==NULL){
									$$=createVarList($2);
								}else{
									$$=appendVarToList($2,$1);
								}
						}
	| {$$=NULL;}
	;

%%
/*--------------------------------------------------------*/
/* Additional C code */
/* Error processor for yyparse */
int FlopocoExpressionerror(char *s)        /* called by yyparse on error */
{
    printf("---->%s\n",s);
    return(0);
}

/*--------------------------------------------------------*/
/* The controlling function */
/*
int main(void)
{
    yyparse();
	return 1;
}
*/

