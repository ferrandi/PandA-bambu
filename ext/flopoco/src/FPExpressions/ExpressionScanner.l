%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <iostream>
#include "ExpressionParser.h"

int FlopocoExpressionlex(void);
int FlopocoExpressionparse(void);
int FlopocoExpressionerror(char *);

using namespace std;

%}

A [/]
B [*]
C [^/*]

%option noyywrap
%%

{A}({A}.*|{B}({A}|{C}|{B}+{C})*{B}+{A}) { /* skip comment */ }

" "               {
                  }
"\t"              {
                  }
                  
                  
"+"               {
                    	FlopocoExpressionlval.c_type = FlopocoExpressiontext[0];
	                    return FPEXPRESSION_PLUS;
                  }
"-"               {
					FlopocoExpressionlval.c_type = FlopocoExpressiontext[0];
                   return FPEXPRESSION_MINUS;
                  }
"*"               { 
					return FPEXPRESSION_TIMES;}
"/"               { 
					return FPEXPRESSION_DIV;}                  
                  
"="               {
                    FlopocoExpressionlval.c_type = FlopocoExpressiontext[0];
                    return FPEXPRESSION_EQUALS;
                  }

";"               { 
					return FPEXPRESSION_PV;
				  }

"output"          { 
					return FPEXPRESSION_OUTPUT;
				  }

"sqrt"            {
					return FPEXPRESSION_SQRT;
					}             
"sqr"            {
					return FPEXPRESSION_SQR;
				}
"exp"            {
					return FPEXPRESSION_EXP;
					}             
"log"            {
					return FPEXPRESSION_LOG;
				}
								  
"("				{ 	
					return FPEXPRESSION_LPAR;
				}
")"				  { 
					return FPEXPRESSION_RPAR;
				}


[a-zA-Z_][a-zA-Z0-9_]*  {
						
						FlopocoExpressionlval.s_type = (char*) malloc( sizeof(char)*strlen(FlopocoExpressiontext) );
						for (unsigned i=0;i<=strlen(FlopocoExpressiontext);i++) { /* <= to copy the ending NULL, too */
							FlopocoExpressionlval.s_type[i] = FlopocoExpressiontext[i];
            }
						return FPEXPRESSION_VARIABLE;
				  }

                 
[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)? {
						FlopocoExpressionlval.s_type = (char*) malloc( sizeof(char)*strlen(FlopocoExpressiontext) );
						for (unsigned i=0;i<strlen(FlopocoExpressiontext);i++)
							FlopocoExpressionlval.s_type[i] = FlopocoExpressiontext[i];
						return FPEXPRESSION_FPNUMBER;
					}                  
                  
.                 {
                    FlopocoExpressionlval.c_type = FlopocoExpressiontext[0];
                    return FPEXPRESSION_UNKNOWN;
                  }
<<EOF>>           {
                    yyterminate();
                  }

%%

/*
int main(void) {
    yylex();
    return 0;
} */

