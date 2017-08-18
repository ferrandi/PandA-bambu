/*

Copyright 2006-2011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

and

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France.

Contributors Ch. Lauter, S. Chevillard

christoph.lauter@ens-lyon.org
sylvain.chevillard@ens-lyon.org

This software is a computer program whose purpose is to provide an
environment for safe floating-point code development. It is
particularily targeted to the automatized implementation of
mathematical floating-point libraries (libm). Amongst other features,
it offers a certified infinity norm, an automatic polynomial
implementer and a fast Remez algorithm.

This software is governed by the CeCILL-C license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL-C
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-C license and that you accept its terms.

This program is distributed WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

%{
#define YYERROR_VERBOSE 1
#define YYPARSE_PARAM scanner
#define YYLEX_PARAM   scanner

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "expression.h"
#include "assignment.h"
#include "chain.h"
#include "general.h"
#include "execute.h"
#include "parser.h"
#include "library.h"
#include "help.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern int yylex(YYSTYPE *lvalp, void *scanner);
extern FILE *yyget_in(void *scanner);
extern char *getCurrentLexSymbol();

void yyerror(void * YYSTYPE, char *message) {
  char *str;
  if (!feof(yyget_in(scanner))) {
    str = getCurrentLexSymbol();
    printMessage(1,"Warning: %s.\nThe last symbol read has been \"%s\".\nWill skip input until next semicolon after the unexpected token. May leak memory.\n",message,str);
    free(str);
    promptToBePrinted = 1;
    lastWasSyntaxError = 1;
    considerDyingOnError();
  } 
}

int parserCheckEof() {
  FILE *myFd;

  myFd = yyget_in(scanner);
  if (myFd == NULL) return 0;
  
  return feof(myFd);
}

// #define WARN_IF_NO_HELP_TEXT 1

%}

%defines

%expect 2

%pure_parser
%lex-param {void * YYLEX_PARAM}
%parse-param {void * YYLEX_PARAM}

%union {
  doubleNode *dblnode;
  struct entryStruct *association;
  char *value;
  node *tree;
  chain *list;
  int *integerval;
  int count;
  void *other;
};



%token  <value> CONSTANTTOKEN;
%token  <value> MIDPOINTCONSTANTTOKEN;
%token  <value> DYADICCONSTANTTOKEN;
%token  <value> HEXCONSTANTTOKEN;
%token  <value> HEXADECIMALCONSTANTTOKEN;
%token  <value> BINARYCONSTANTTOKEN;

%token  PITOKEN;

%token  <value> IDENTIFIERTOKEN;

%token  <value> STRINGTOKEN;

%token  LPARTOKEN;
%token  RPARTOKEN;
%token  LBRACKETTOKEN;
%token  RBRACKETTOKEN;
%token  EQUALTOKEN;
%token  ASSIGNEQUALTOKEN;
%token  COMPAREEQUALTOKEN;
%token  COMMATOKEN;
%token  EXCLAMATIONTOKEN;
%token  SEMICOLONTOKEN;
%token  STARLEFTANGLETOKEN;
%token  LEFTANGLETOKEN;
%token  RIGHTANGLEUNDERSCORETOKEN;
%token  RIGHTANGLEDOTTOKEN;
%token  RIGHTANGLESTARTOKEN;
%token  RIGHTANGLETOKEN;
%token  DOTSTOKEN;
%token  DOTTOKEN;
%token  QUESTIONMARKTOKEN;
%token  VERTBARTOKEN;
%token  ATTOKEN;
%token  DOUBLECOLONTOKEN;
%token  COLONTOKEN;
%token  DOTCOLONTOKEN;
%token  COLONDOTTOKEN;
%token  EXCLAMATIONEQUALTOKEN;
%token  APPROXTOKEN;
%token  ANDTOKEN;
%token  ORTOKEN;

%token  PLUSTOKEN;
%token  MINUSTOKEN;
%token  MULTOKEN;
%token  DIVTOKEN;
%token  POWTOKEN;

%token  SQRTTOKEN;
%token  EXPTOKEN;
%token  LOGTOKEN;
%token  LOG2TOKEN;
%token  LOG10TOKEN;
%token  SINTOKEN;
%token  COSTOKEN;
%token  TANTOKEN;
%token  ASINTOKEN;
%token  ACOSTOKEN;
%token  ATANTOKEN;
%token  SINHTOKEN;
%token  COSHTOKEN;
%token  TANHTOKEN;
%token  ASINHTOKEN;
%token  ACOSHTOKEN;
%token  ATANHTOKEN;
%token  ABSTOKEN;
%token  ERFTOKEN;
%token  ERFCTOKEN;
%token  LOG1PTOKEN;
%token  EXPM1TOKEN;
%token  DOUBLETOKEN;
%token  SINGLETOKEN;
%token  HALFPRECISIONTOKEN;
%token  QUADTOKEN;
%token  DOUBLEDOUBLETOKEN;
%token  TRIPLEDOUBLETOKEN;
%token  DOUBLEEXTENDEDTOKEN;
%token  CEILTOKEN;
%token  FLOORTOKEN;
%token  NEARESTINTTOKEN;

%token  HEADTOKEN;
%token  REVERTTOKEN;
%token  SORTTOKEN;
%token  TAILTOKEN;
%token  MANTISSATOKEN;
%token  EXPONENTTOKEN;
%token  PRECISIONTOKEN;
%token  ROUNDCORRECTLYTOKEN;

%token  PRECTOKEN;
%token  POINTSTOKEN;
%token  DIAMTOKEN;
%token  DISPLAYTOKEN;
%token  VERBOSITYTOKEN;
%token  CANONICALTOKEN;
%token  AUTOSIMPLIFYTOKEN;
%token  TAYLORRECURSIONSTOKEN;
%token  TIMINGTOKEN;
%token  TIMETOKEN;
%token  FULLPARENTHESESTOKEN;
%token  MIDPOINTMODETOKEN;
%token  DIEONERRORMODETOKEN;
%token  SUPPRESSWARNINGSTOKEN;
%token  RATIONALMODETOKEN;
%token  HOPITALRECURSIONSTOKEN;

%token  ONTOKEN;
%token  OFFTOKEN;
%token  DYADICTOKEN;
%token  POWERSTOKEN;
%token  BINARYTOKEN;
%token  HEXADECIMALTOKEN;
%token  FILETOKEN;
%token  POSTSCRIPTTOKEN;
%token  POSTSCRIPTFILETOKEN;
%token  PERTURBTOKEN;
%token  MINUSWORDTOKEN;
%token  PLUSWORDTOKEN;
%token  ZEROWORDTOKEN;
%token  NEARESTTOKEN;
%token  HONORCOEFFPRECTOKEN;
%token  TRUETOKEN;
%token  FALSETOKEN;
%token  DEFAULTTOKEN;
%token  MATCHTOKEN;
%token  WITHTOKEN;
%token  ABSOLUTETOKEN;
%token  DECIMALTOKEN;
%token  RELATIVETOKEN;
%token  FIXEDTOKEN;
%token  FLOATINGTOKEN;

%token  ERRORTOKEN;

%token  QUITTOKEN;
%token  FALSEQUITTOKEN;
%token  RESTARTTOKEN;

%token  LIBRARYTOKEN;
%token  LIBRARYCONSTANTTOKEN;

%token  DIFFTOKEN;
%token  SIMPLIFYTOKEN;
%token  REMEZTOKEN;
%token  BASHEVALUATETOKEN;
%token  FPMINIMAXTOKEN;
%token  HORNERTOKEN;
%token  EXPANDTOKEN;
%token  SIMPLIFYSAFETOKEN;

%token  TAYLORTOKEN;
%token  TAYLORFORMTOKEN;
%token  AUTODIFFTOKEN;
%token  DEGREETOKEN;
%token  NUMERATORTOKEN;
%token  DENOMINATORTOKEN;
%token  SUBSTITUTETOKEN;
%token  COEFFTOKEN;
%token  SUBPOLYTOKEN;
%token  ROUNDCOEFFICIENTSTOKEN;
%token  RATIONALAPPROXTOKEN;
%token  ACCURATEINFNORMTOKEN;
%token  ROUNDTOFORMATTOKEN;
%token  EVALUATETOKEN;
%token  LENGTHTOKEN;
%token  INFTOKEN;
%token  MIDTOKEN;
%token  SUPTOKEN;
%token  MINTOKEN;
%token  MAXTOKEN;

%token  READXMLTOKEN;
%token  PARSETOKEN;

%token  PRINTTOKEN;
%token  PRINTXMLTOKEN;
%token  PLOTTOKEN;
%token  PRINTHEXATOKEN;
%token  PRINTFLOATTOKEN;
%token  PRINTBINARYTOKEN;
%token  PRINTEXPANSIONTOKEN;
%token  BASHEXECUTETOKEN;
%token  EXTERNALPLOTTOKEN;
%token  WRITETOKEN;
%token  ASCIIPLOTTOKEN;
%token  RENAMETOKEN;


%token  INFNORMTOKEN;
%token  SUPNORMTOKEN;
%token  FINDZEROSTOKEN;
%token  FPFINDZEROSTOKEN;
%token  DIRTYINFNORMTOKEN;
%token  NUMBERROOTSTOKEN;
%token  INTEGRALTOKEN;
%token  DIRTYINTEGRALTOKEN;
%token  WORSTCASETOKEN;
%token  IMPLEMENTPOLYTOKEN;
%token  IMPLEMENTCONSTTOKEN;
%token  CHECKINFNORMTOKEN;
%token  ZERODENOMINATORSTOKEN;
%token  ISEVALUABLETOKEN;
%token  SEARCHGALTOKEN;
%token  GUESSDEGREETOKEN;
%token  DIRTYFINDZEROSTOKEN;

%token  IFTOKEN;
%token  THENTOKEN;
%token  ELSETOKEN;
%token  FORTOKEN;
%token  INTOKEN;
%token  FROMTOKEN;
%token  TOTOKEN;
%token  BYTOKEN;
%token  DOTOKEN;
%token  BEGINTOKEN;
%token  ENDTOKEN;
%token  LEFTCURLYBRACETOKEN;
%token  RIGHTCURLYBRACETOKEN;
%token  WHILETOKEN;

%token  READFILETOKEN;

%token  ISBOUNDTOKEN;

%token  EXECUTETOKEN;

%token  EXTERNALPROCTOKEN;
%token  VOIDTOKEN;
%token  CONSTANTTYPETOKEN;
%token  FUNCTIONTOKEN;
%token  RANGETOKEN;
%token  INTEGERTOKEN;
%token  STRINGTYPETOKEN;
%token  BOOLEANTOKEN;
%token  LISTTOKEN;
%token  OFTOKEN;

%token  VARTOKEN;
%token  PROCTOKEN;
%token  PROCEDURETOKEN;
%token  RETURNTOKEN;
%token  NOPTOKEN;

%token  HELPTOKEN;
%token  VERSIONTOKEN;


%type <other> startsymbol;
%type <other> help;
%type <other> helpmeta;
%type <other> egalquestionmark;
%type <count> unaryplusminus;
%type <tree>  command;
%type <tree>  procbody;
%type <tree>  variabledeclaration;
%type <tree>  simplecommand;
%type <list>  commandlist;
%type <list>  variabledeclarationlist;
%type <list>  identifierlist;
%type <tree>  thing;
%type <tree>  supermegaterm;
%type <list>  thinglist;
%type <list>  matchlist;
%type <tree>  matchelement; 
%type <list>  structelementlist;
%type <association>  structelement;
%type <other>  structelementseparator;
%type <tree>  structuring;
%type <tree>  ifcommand;
%type <tree>  forcommand;
%type <tree>  assignment;
%type <tree>  simpleassignment;
%type <tree>  stateassignment;
%type <tree>  stillstateassignment;
%type <tree>  basicthing;
%type <tree>  list;
%type <tree>  constant;
%type <list>  simplelist;
%type <tree>  range;
%type <tree>  debound;
%type <tree>  headfunction;
%type <tree>  term;
%type <tree>  hyperterm;
%type <tree>  subterm;
%type <tree>  megaterm;
%type <tree>  statedereference;
%type <dblnode>  indexing;
%type <integerval> externalproctype;
%type <integerval> extendedexternalproctype;
%type <list>  externalproctypesimplelist;
%type <list>  externalproctypelist;
%type <other> beginsymbol;
%type <other> endsymbol;

%%

startsymbol:            command SEMICOLONTOKEN
                          {
			    parsedThing = $1;
			    $$ = NULL;
			    YYACCEPT;
			  }
                      | helpmeta SEMICOLONTOKEN
                          {
			    outputMode();
                            sollyaPrintf("This is %s.\nType 'help help;' for the list of available commands. Type 'help <command>;' for help on the specific command <command>.\nType 'quit;' for quitting the %s interpreter.\n\nYou can get moral support and help with bugs by writing to %s.\n\n",PACKAGE_NAME,PACKAGE_NAME,PACKAGE_BUGREPORT);
			    parsedThing = NULL;
			    $$ = NULL;
			    YYACCEPT;
			  }
                      | QUESTIONMARKTOKEN
                          {
			    outputMode();
                            sollyaPrintf("This is %s.\nType 'help help;' for the list of available commands. Type 'help <command>;' for help on the specific command <command>.\nType 'quit;' for quitting the %s interpreter.\n\nYou can get moral support and help with bugs by writing to %s.\n\n",PACKAGE_NAME,PACKAGE_NAME,PACKAGE_BUGREPORT);
			    parsedThing = NULL;
			    $$ = NULL;
			    YYACCEPT;
			  }
                      | helpmeta help SEMICOLONTOKEN
                          {
			    parsedThing = NULL;
			    $$ = NULL;
			    YYACCEPT;
			  }
                      | VERSIONTOKEN SEMICOLONTOKEN
                          {
			    outputMode();
			    sollyaPrintf("This is\n\n\t%s.\n\nCopyright 2006-2011 by\n\n    Laboratoire de l'Informatique du Parallelisme,\n    UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668, Lyon, France,\n\n    LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2), Nancy, France,\n\n    Laboratoire d'Informatique de Paris 6, equipe PEQUAN,\n    UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France,\n\nand by\n\n    INRIA Sophia-Antipolis Mediterranee, APICS Team,\n    Sophia-Antipolis, France.\n\nAll rights reserved.\n\nContributors are S. Chevillard, N. Jourdan, M. Joldes and Ch. Lauter.\n\nThis software is governed by the CeCILL-C license under French law and\nabiding by the rules of distribution of free software.  You can  use,\nmodify and/ or redistribute the software under the terms of the CeCILL-C\nlicense as circulated by CEA, CNRS and INRIA at the following URL\n\"http://www.cecill.info\".\n\nPlease send bug reports to %s.\n\nThis program is distributed WITHOUT ANY WARRANTY; without even the\nimplied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\nThis build of %s is based on GMP %s, MPFR %s and MPFI %s.\n",PACKAGE_STRING,PACKAGE_BUGREPORT,PACKAGE_STRING,gmp_version,mpfr_get_version(),sollya_mpfi_get_version());
#if defined(HAVE_FPLLL_VERSION_STRING)
			    sollyaPrintf("%s uses FPLLL as: \"%s\"\n",PACKAGE_STRING,HAVE_FPLLL_VERSION_STRING);
#endif
			    sollyaPrintf("\n");
			    parsedThing = NULL;
			    $$ = NULL;
			    YYACCEPT;
			  }
                      | error SEMICOLONTOKEN
                          {
			    parsedThing = NULL;
			    $$ = NULL;
			    YYACCEPT;
			  }
;

helpmeta:               HELPTOKEN
                          {
			    helpNotFinished = 1;
			    $$ = NULL;
			  }
;

beginsymbol:            BEGINTOKEN
                          {
			    $$ = NULL;
			  }
                      | LEFTCURLYBRACETOKEN
		          {
			    $$ = NULL;
			  }
;

endsymbol:              ENDTOKEN
                          {
			    $$ = NULL;
			  }
                      | RIGHTCURLYBRACETOKEN
		          {
			    $$ = NULL;
			  }
;

command:                simplecommand
                          {
			    $$ = $1;
			  }
                      | beginsymbol commandlist endsymbol
                          {
			    $$ = makeCommandList($2);
                          }
                      | beginsymbol variabledeclarationlist commandlist endsymbol
                          {
			    $$ = makeCommandList(concatChains($2, $3));
                          }
                      | beginsymbol variabledeclarationlist endsymbol
                          {
			    $$ = makeCommandList($2);
                          }
                      | beginsymbol endsymbol
                          {
			    $$ = makeNop();
                          }
                      | IFTOKEN ifcommand
                          {
			    $$ = $2;
			  }
                      | WHILETOKEN thing DOTOKEN command
                          {
			    $$ = makeWhile($2, $4);
			  }
                      | FORTOKEN forcommand
                          {
			    $$ = $2;
			  }
;

ifcommand:              thing THENTOKEN command
                          {
			    $$ = makeIf($1, $3);
                          }
                      | thing THENTOKEN command ELSETOKEN command
                          {
			    $$ = makeIfElse($1,$3,$5);
                          }
;



forcommand:             IDENTIFIERTOKEN FROMTOKEN thing TOTOKEN thing DOTOKEN command
                          {
			    $$ = makeFor($1, $3, $5, makeConstantDouble(1.0), $7);
			    free($1);
                          }
                      | IDENTIFIERTOKEN FROMTOKEN thing TOTOKEN thing BYTOKEN thing DOTOKEN command
                          {
			    $$ = makeFor($1, $3, $5, $7, $9);
			    free($1);
                          }
                      | IDENTIFIERTOKEN INTOKEN thing DOTOKEN command
                          {
			    $$ = makeForIn($1, $3, $5);
			    free($1);
                          }
;


commandlist:            command SEMICOLONTOKEN
                          {
			    $$ = addElement(NULL, $1);
			  }
                      | command SEMICOLONTOKEN commandlist
                          {
			    $$ = addElement($3, $1);
			  }
;

variabledeclarationlist: variabledeclaration SEMICOLONTOKEN
                          {
			    $$ = addElement(NULL, $1);
			  }
                      | variabledeclaration SEMICOLONTOKEN variabledeclarationlist
                          {
			    $$ = addElement($3, $1);
			  }
;

variabledeclaration:    VARTOKEN identifierlist
                          {
			    $$ = makeVariableDeclaration($2);
			  }
;


identifierlist:         IDENTIFIERTOKEN
                          {
			    $$ = addElement(NULL, $1);
			  }
                      | IDENTIFIERTOKEN COMMATOKEN identifierlist
                          {
			    $$ = addElement($3, $1);
			  }
;

procbody:               LPARTOKEN RPARTOKEN beginsymbol commandlist endsymbol
                          {
			    $$ = makeProc(NULL, makeCommandList($4), makeUnit());
                          }
                      | LPARTOKEN RPARTOKEN beginsymbol variabledeclarationlist commandlist endsymbol
                          {
			    $$ = makeProc(NULL, makeCommandList(concatChains($4, $5)), makeUnit());
                          }
                      | LPARTOKEN RPARTOKEN beginsymbol variabledeclarationlist endsymbol
                          {
			    $$ = makeProc(NULL, makeCommandList($4), makeUnit());
                          }
                      | LPARTOKEN RPARTOKEN beginsymbol endsymbol
                          {
			    $$ = makeProc(NULL, makeCommandList(addElement(NULL,makeNop())), makeUnit());
                          }
                      | LPARTOKEN RPARTOKEN beginsymbol commandlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProc(NULL, makeCommandList($4), $6);
                          }
                      | LPARTOKEN RPARTOKEN beginsymbol variabledeclarationlist commandlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProc(NULL, makeCommandList(concatChains($4, $5)), $7);
                          }
                      | LPARTOKEN RPARTOKEN beginsymbol variabledeclarationlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProc(NULL, makeCommandList($4), $6);
                          }
                      | LPARTOKEN RPARTOKEN beginsymbol RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProc(NULL, makeCommandList(addElement(NULL,makeNop())), $5);
                          }
                      | LPARTOKEN identifierlist RPARTOKEN beginsymbol commandlist endsymbol
                          {
			    $$ = makeProc($2, makeCommandList($5), makeUnit());
                          }
                      | LPARTOKEN identifierlist RPARTOKEN beginsymbol variabledeclarationlist commandlist endsymbol
                          {
			    $$ = makeProc($2, makeCommandList(concatChains($5, $6)), makeUnit());
                          }
                      | LPARTOKEN identifierlist RPARTOKEN beginsymbol variabledeclarationlist endsymbol
                          {
			    $$ = makeProc($2, makeCommandList($5), makeUnit());
                          }
                      | LPARTOKEN identifierlist RPARTOKEN beginsymbol endsymbol
                          {
			    $$ = makeProc($2, makeCommandList(addElement(NULL,makeNop())), makeUnit());
                          }
                      | LPARTOKEN identifierlist RPARTOKEN beginsymbol commandlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProc($2, makeCommandList($5), $7);
                          }
                      | LPARTOKEN identifierlist RPARTOKEN beginsymbol variabledeclarationlist commandlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProc($2, makeCommandList(concatChains($5, $6)), $8);
                          }
                      | LPARTOKEN identifierlist RPARTOKEN beginsymbol variabledeclarationlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProc($2, makeCommandList($5), $7);
                          }
                      | LPARTOKEN identifierlist RPARTOKEN beginsymbol RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProc($2, makeCommandList(addElement(NULL, makeNop())), $6);
                          }
                      | LPARTOKEN IDENTIFIERTOKEN EQUALTOKEN DOTSTOKEN RPARTOKEN beginsymbol commandlist endsymbol
                          {
			    $$ = makeProcIllim($2, makeCommandList($7), makeUnit());
                          }
                      | LPARTOKEN IDENTIFIERTOKEN EQUALTOKEN DOTSTOKEN RPARTOKEN beginsymbol variabledeclarationlist commandlist endsymbol
                          {
			    $$ = makeProcIllim($2, makeCommandList(concatChains($7, $8)), makeUnit());
                          }
                      | LPARTOKEN IDENTIFIERTOKEN EQUALTOKEN DOTSTOKEN RPARTOKEN beginsymbol variabledeclarationlist endsymbol
                          {
			    $$ = makeProcIllim($2, makeCommandList($7), makeUnit());
                          }
                      | LPARTOKEN IDENTIFIERTOKEN EQUALTOKEN DOTSTOKEN RPARTOKEN beginsymbol endsymbol
                          {
			    $$ = makeProcIllim($2, makeCommandList(addElement(NULL,makeNop())), makeUnit());
                          }
                      | LPARTOKEN IDENTIFIERTOKEN EQUALTOKEN DOTSTOKEN RPARTOKEN beginsymbol commandlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProcIllim($2, makeCommandList($7), $9);
                          }
                      | LPARTOKEN IDENTIFIERTOKEN EQUALTOKEN DOTSTOKEN RPARTOKEN beginsymbol variabledeclarationlist commandlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProcIllim($2, makeCommandList(concatChains($7, $8)), $10);
                          }
                      | LPARTOKEN IDENTIFIERTOKEN EQUALTOKEN DOTSTOKEN RPARTOKEN beginsymbol variabledeclarationlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProcIllim($2, makeCommandList($7), $9);
                          }
                      | LPARTOKEN IDENTIFIERTOKEN EQUALTOKEN DOTSTOKEN RPARTOKEN beginsymbol RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeProcIllim($2, makeCommandList(addElement(NULL, makeNop())), $8);
                          }
;


simplecommand:          QUITTOKEN
                          {
			    $$ = makeQuit();
			  }
                      | FALSEQUITTOKEN
                          {
			    $$ = makeFalseQuit();
			  }
                      | NOPTOKEN
                          {
			    $$ = makeNop();
			  }
                      | NOPTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeNopArg($3);
			  }
                      | NOPTOKEN LPARTOKEN RPARTOKEN
                          {
			    $$ = makeNopArg(makeDefault());
			  }
                      | RESTARTTOKEN
                          {
			    $$ = makeRestart();
			  }
                      | PRINTTOKEN LPARTOKEN thinglist RPARTOKEN
                          {
			    $$ = makePrint($3);
			  }
                      | PRINTTOKEN LPARTOKEN thinglist RPARTOKEN RIGHTANGLETOKEN thing
                          {
			    $$ = makeNewFilePrint($6, $3);
			  }
                      | PRINTTOKEN LPARTOKEN thinglist RPARTOKEN RIGHTANGLETOKEN RIGHTANGLETOKEN thing
                          {
			    $$ = makeAppendFilePrint($7, $3);
			  }
                      | PLOTTOKEN LPARTOKEN thing COMMATOKEN thinglist RPARTOKEN
                          {
			    $$ = makePlot(addElement($5, $3));
			  }
                      | PRINTHEXATOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makePrintHexa($3);
			  }
                      | PRINTFLOATTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makePrintFloat($3);
			  }
                      | PRINTBINARYTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makePrintBinary($3);
			  }
                      | PRINTEXPANSIONTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makePrintExpansion($3);
			  }
                      | IMPLEMENTCONSTTOKEN LPARTOKEN thinglist RPARTOKEN
                          {
			    $$ = makeImplementConst($3);
			  }
                      | BASHEXECUTETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeBashExecute($3);
			  }
                      | EXTERNALPLOTTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thing COMMATOKEN thing COMMATOKEN thinglist RPARTOKEN
                          {
			    $$ = makeExternalPlot(addElement(addElement(addElement(addElement($11,$9),$7),$5),$3));
			  }
                      | WRITETOKEN LPARTOKEN thinglist RPARTOKEN
                          {
			    $$ = makeWrite($3);
			  }
                      | WRITETOKEN LPARTOKEN thinglist RPARTOKEN RIGHTANGLETOKEN thing
                          {
			    $$ = makeNewFileWrite($6, $3);
			  }
                      | WRITETOKEN LPARTOKEN thinglist RPARTOKEN RIGHTANGLETOKEN RIGHTANGLETOKEN thing
                          {
			    $$ = makeAppendFileWrite($7, $3);
			  }
                      | ASCIIPLOTTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeAsciiPlot($3, $5);
			  }
                      | PRINTXMLTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makePrintXml($3);
			  }
                      | EXECUTETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeExecute($3);
			  }
                      | PRINTXMLTOKEN LPARTOKEN thing RPARTOKEN RIGHTANGLETOKEN thing
                          {
			    $$ = makePrintXmlNewFile($3,$6);
			  }
                      | PRINTXMLTOKEN LPARTOKEN thing RPARTOKEN RIGHTANGLETOKEN RIGHTANGLETOKEN thing
                          {
			    $$ = makePrintXmlAppendFile($3,$7);
			  }
                      | WORSTCASETOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thing COMMATOKEN thing COMMATOKEN thinglist RPARTOKEN
                          {
			    $$ = makeWorstCase(addElement(addElement(addElement(addElement($11, $9), $7), $5), $3));
			  }
                      | RENAMETOKEN LPARTOKEN IDENTIFIERTOKEN COMMATOKEN IDENTIFIERTOKEN RPARTOKEN
                          {
			    $$ = makeRename($3, $5);
			    free($3);
			    free($5);
			  }
                      | EXTERNALPROCTOKEN LPARTOKEN IDENTIFIERTOKEN COMMATOKEN thing COMMATOKEN externalproctypelist MINUSTOKEN RIGHTANGLETOKEN extendedexternalproctype RPARTOKEN
                          {
			    $$ = makeExternalProc($3, $5, addElement($7, $10));
			    free($3);
			  }
                      | assignment
                          {
			    $$ = $1;
			  }
                      | thinglist
                          {
			    $$ = makeAutoprint($1);
			  }
                      | PROCEDURETOKEN IDENTIFIERTOKEN procbody
                          {
			    $$ = makeAssignment($2, $3);
			    free($2);
			  }
;

assignment:             stateassignment
                          {
			    $$ = $1;
			  }
                      | stillstateassignment EXCLAMATIONTOKEN
                          {
			    $$ = $1;
			  }
                      | simpleassignment
                          {
			    $$ = $1;
			  }
                      | simpleassignment EXCLAMATIONTOKEN
                          {
			    $$ = $1;
			  }
;

simpleassignment:       IDENTIFIERTOKEN EQUALTOKEN thing
                          {
			    $$ = makeAssignment($1, $3);
			    free($1);
			  }
                      | IDENTIFIERTOKEN ASSIGNEQUALTOKEN thing
                          {
			    $$ = makeFloatAssignment($1, $3);
			    free($1);
			  }
                      | IDENTIFIERTOKEN EQUALTOKEN LIBRARYTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeLibraryBinding($1, $5);
			    free($1);
			  }
                      | IDENTIFIERTOKEN EQUALTOKEN LIBRARYCONSTANTTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeLibraryConstantBinding($1, $5);
			    free($1);
			  }
                      | indexing EQUALTOKEN thing
                          {
			    $$ = makeAssignmentInIndexing($1->a,$1->b,$3);
			    free($1);
			  }
                      | indexing ASSIGNEQUALTOKEN thing
                          {
			    $$ = makeFloatAssignmentInIndexing($1->a,$1->b,$3);
			    free($1);
			  }
                      | structuring EQUALTOKEN thing
                          {
			    $$ = makeProtoAssignmentInStructure($1,$3);
			  }
                      | structuring ASSIGNEQUALTOKEN thing
                          {
			    $$ = makeProtoFloatAssignmentInStructure($1,$3);
			  }
;

structuring:            basicthing DOTTOKEN IDENTIFIERTOKEN 
		          {
			    $$ = makeStructAccess($1,$3);
			    free($3);
			  }
;

stateassignment:        PRECTOKEN EQUALTOKEN thing
                          {
			    $$ = makePrecAssign($3);
			  }
                      | POINTSTOKEN EQUALTOKEN thing
                          {
			    $$ = makePointsAssign($3);
			  }
                      | DIAMTOKEN EQUALTOKEN thing
                          {
			    $$ = makeDiamAssign($3);
			  }
                      | DISPLAYTOKEN EQUALTOKEN thing
                          {
			    $$ = makeDisplayAssign($3);
			  }
                      | VERBOSITYTOKEN EQUALTOKEN thing
                          {
			    $$ = makeVerbosityAssign($3);
			  }
                      | CANONICALTOKEN EQUALTOKEN thing
                          {
			    $$ = makeCanonicalAssign($3);
			  }
                      | AUTOSIMPLIFYTOKEN EQUALTOKEN thing
                          {
			    $$ = makeAutoSimplifyAssign($3);
			  }
                      | TAYLORRECURSIONSTOKEN EQUALTOKEN thing
                          {
			    $$ = makeTaylorRecursAssign($3);
			  }
                      | TIMINGTOKEN EQUALTOKEN thing
                          {
			    $$ = makeTimingAssign($3);
			  }
                      | FULLPARENTHESESTOKEN EQUALTOKEN thing
                          {
			    $$ = makeFullParenAssign($3);
			  }
                      | MIDPOINTMODETOKEN EQUALTOKEN thing
                          {
			    $$ = makeMidpointAssign($3);
			  }
                      | DIEONERRORMODETOKEN EQUALTOKEN thing
                          {
			    $$ = makeDieOnErrorAssign($3);
			  }
                      | RATIONALMODETOKEN EQUALTOKEN thing
                          {
			    $$ = makeRationalModeAssign($3);
			  }
                      | SUPPRESSWARNINGSTOKEN EQUALTOKEN thing
                          {
			    $$ = makeSuppressWarningsAssign($3);
			  }
                      | HOPITALRECURSIONSTOKEN EQUALTOKEN thing
                          {
			    $$ = makeHopitalRecursAssign($3);
			  }
;

stillstateassignment:   PRECTOKEN EQUALTOKEN thing
                          {
			    $$ = makePrecStillAssign($3);
			  }
                      | POINTSTOKEN EQUALTOKEN thing
                          {
			    $$ = makePointsStillAssign($3);
			  }
                      | DIAMTOKEN EQUALTOKEN thing
                          {
			    $$ = makeDiamStillAssign($3);
			  }
                      | DISPLAYTOKEN EQUALTOKEN thing
                          {
			    $$ = makeDisplayStillAssign($3);
			  }
                      | VERBOSITYTOKEN EQUALTOKEN thing
                          {
			    $$ = makeVerbosityStillAssign($3);
			  }
                      | CANONICALTOKEN EQUALTOKEN thing
                          {
			    $$ = makeCanonicalStillAssign($3);
			  }
                      | AUTOSIMPLIFYTOKEN EQUALTOKEN thing
                          {
			    $$ = makeAutoSimplifyStillAssign($3);
			  }
                      | TAYLORRECURSIONSTOKEN EQUALTOKEN thing
                          {
			    $$ = makeTaylorRecursStillAssign($3);
			  }
                      | TIMINGTOKEN EQUALTOKEN thing
                          {
			    $$ = makeTimingStillAssign($3);
			  }
                      | FULLPARENTHESESTOKEN EQUALTOKEN thing
                          {
			    $$ = makeFullParenStillAssign($3);
			  }
                      | MIDPOINTMODETOKEN EQUALTOKEN thing
                          {
			    $$ = makeMidpointStillAssign($3);
			  }
                      | DIEONERRORMODETOKEN EQUALTOKEN thing
                          {
			    $$ = makeDieOnErrorStillAssign($3);
			  }
                      | RATIONALMODETOKEN EQUALTOKEN thing
                          {
			    $$ = makeRationalModeStillAssign($3);
			  }
                      | SUPPRESSWARNINGSTOKEN EQUALTOKEN thing
                          {
			    $$ = makeSuppressWarningsStillAssign($3);
			  }
                      | HOPITALRECURSIONSTOKEN EQUALTOKEN thing
                          {
			    $$ = makeHopitalRecursStillAssign($3);
			  }
;

thinglist:              thing
                          {
			    $$ = addElement(NULL, $1);
			  }
                      | thing COMMATOKEN thinglist
                          {
			    $$ = addElement($3, $1);
			  }
;

structelementlist:      structelement
                          {
			    $$ = addElement(NULL, $1);
			  }
                      | structelement structelementseparator structelementlist
                          {
			    $$ = addElement($3, $1);
			  }
;

structelementseparator: COMMATOKEN
                          {
			    $$ = NULL;
			  }
                      | SEMICOLONTOKEN
		          {
			    $$ = NULL;
			  }
;

structelement:          DOTTOKEN IDENTIFIERTOKEN EQUALTOKEN thing
                          {
			    $$ = (entry *) safeMalloc(sizeof(entry));
			    $$->name = (char *) safeCalloc(strlen($2) + 1, sizeof(char));
			    strcpy($$->name,$2);
			    free($2);
			    $$->value = (void *) ($4);
			  }
;

thing:                  supermegaterm
                         {
			   $$ = $1;
			 }
                      | MATCHTOKEN supermegaterm WITHTOKEN matchlist
		          {
			    $$ = makeMatch($2,$4);
			  }
;

supermegaterm:          megaterm
                          {
			    $$ = $1;
			  }
                      | thing ANDTOKEN megaterm
                          {
			    $$ = makeAnd($1, $3);
			  }
                      | thing ORTOKEN megaterm
                          {
			    $$ = makeOr($1, $3);
			  }
                      | EXCLAMATIONTOKEN megaterm
                          {
			    $$ = makeNegation($2);
			  }
;

indexing:               basicthing LBRACKETTOKEN thing RBRACKETTOKEN
                          {
			    $$ = (doubleNode *) safeMalloc(sizeof(doubleNode));
			    $$->a = $1;
			    $$->b = $3;
			  }
;


megaterm:               hyperterm
                          {
			    $$ = $1;
			  }
                      | megaterm COMPAREEQUALTOKEN hyperterm
                          {
			    $$ = makeCompareEqual($1, $3);
			  }
                      | megaterm INTOKEN hyperterm
                          {
			    $$ = makeCompareIn($1, $3);
			  }
                      | megaterm LEFTANGLETOKEN hyperterm
                          {
			    $$ = makeCompareLess($1, $3);
			  }
                      | megaterm RIGHTANGLETOKEN hyperterm
                          {
			    $$ = makeCompareGreater($1, $3);
			  }
                      | megaterm LEFTANGLETOKEN EQUALTOKEN hyperterm
                          {
			    $$ = makeCompareLessEqual($1, $4);
			  }
                      | megaterm RIGHTANGLETOKEN EQUALTOKEN hyperterm
                          {
			    $$ = makeCompareGreaterEqual($1, $4);
			  }
                      | megaterm EXCLAMATIONEQUALTOKEN hyperterm
                          {
			    $$ = makeCompareNotEqual($1, $3);
			  }
;

hyperterm:                term
                          {
			    $$ = $1;
			  }
                      | hyperterm PLUSTOKEN term
                          {
			    $$ = makeAdd($1, $3);
			  }
                      | hyperterm MINUSTOKEN term
                          {
			    $$ = makeSub($1, $3);
			  }
                      | hyperterm ATTOKEN term
                          {
			    $$ = makeConcat($1, $3);
			  }
                      | hyperterm DOUBLECOLONTOKEN term
                          {
			    $$ = makeAddToList($1, $3);
			  }
                      | hyperterm DOTCOLONTOKEN term
                          {
			    $$ = makePrepend($1, $3);
			  }
                      | hyperterm COLONDOTTOKEN term
                          {
			    $$ = makeAppend($1, $3);
			  }
;

unaryplusminus:         PLUSTOKEN
			  {
			    $$ = 0;
                          }
		      |	MINUSTOKEN
                          {
			    $$ = 1;
                          }
                      | PLUSTOKEN unaryplusminus
			  {
			    $$ = $2;
                          }
		      |	MINUSTOKEN unaryplusminus
                          {
			    $$ = $2+1;
                          }
;


term:                   subterm
			  {
			    $$ = $1;
                          }
		      |	unaryplusminus subterm
                          {
			    tempNode = $2;
			    for (tempInteger=0;tempInteger<$1;tempInteger++)
			      tempNode = makeNeg(tempNode);
			    $$ = tempNode;
                          }
		      |	APPROXTOKEN subterm
                          {
			    $$ = makeEvalConst($2);
                          }
		      |	term MULTOKEN subterm
			  {
			    $$ = makeMul($1, $3);
                          }
		      |	term DIVTOKEN subterm
                          {
			    $$ = makeDiv($1, $3);
                          }
		      |	term MULTOKEN unaryplusminus subterm
			  {
			    tempNode = $4;
			    for (tempInteger=0;tempInteger<$3;tempInteger++)
			      tempNode = makeNeg(tempNode);
			    $$ = makeMul($1, tempNode);
                          }
		      |	term DIVTOKEN unaryplusminus subterm
                          {
			    tempNode = $4;
			    for (tempInteger=0;tempInteger<$3;tempInteger++)
			      tempNode = makeNeg(tempNode);
			    $$ = makeDiv($1, tempNode);
                          }
		      |	term MULTOKEN APPROXTOKEN subterm
			  {
			    $$ = makeMul($1, makeEvalConst($4));
                          }
		      |	term DIVTOKEN APPROXTOKEN subterm
                          {
			    $$ = makeDiv($1, makeEvalConst($4));
                          }
;

subterm:                basicthing
                          {
			    $$ = $1;
                          }
                      | basicthing POWTOKEN subterm
                          {
			    $$ = makePow($1, $3);
                          }
                      | basicthing POWTOKEN unaryplusminus subterm
                          {
			    tempNode = $4;
			    for (tempInteger=0;tempInteger<$3;tempInteger++)
			      tempNode = makeNeg(tempNode);
			    $$ = makePow($1, tempNode);
                          }
                      | basicthing POWTOKEN APPROXTOKEN subterm
                          {
			    $$ = makePow($1, makeEvalConst($4));
                          }
;


basicthing:             ONTOKEN
                          {
			    $$ = makeOn();
			  }
                      | OFFTOKEN
                          {
			    $$ = makeOff();
			  }
                      | DYADICTOKEN
                          {
			    $$ = makeDyadic();
			  }
                      | POWERSTOKEN
                          {
			    $$ = makePowers();
			  }
                      | BINARYTOKEN
                          {
			    $$ = makeBinaryThing();
			  }
                      | HEXADECIMALTOKEN
                          {
			    $$ = makeHexadecimalThing();
			  }
                      | FILETOKEN
                          {
			    $$ = makeFile();
			  }
                      | POSTSCRIPTTOKEN
                          {
			    $$ = makePostscript();
			  }
                      | POSTSCRIPTFILETOKEN
                          {
			    $$ = makePostscriptFile();
			  }
                      | PERTURBTOKEN
                          {
			    $$ = makePerturb();
			  }
                      | MINUSWORDTOKEN
                          {
			    $$ = makeRoundDown();
			  }
                      | PLUSWORDTOKEN
                          {
			    $$ = makeRoundUp();
			  }
                      | ZEROWORDTOKEN
                          {
			    $$ = makeRoundToZero();
			  }
                      | NEARESTTOKEN
                          {
			    $$ = makeRoundToNearest();
			  }
                      | HONORCOEFFPRECTOKEN
                          {
			    $$ = makeHonorCoeff();
			  }
                      | TRUETOKEN
                          {
			    $$ = makeTrue();
			  }
                      | VOIDTOKEN
                          {
			    $$ = makeUnit();
			  }
                      | FALSETOKEN
                          {
			    $$ = makeFalse();
			  }
                      | DEFAULTTOKEN
                          {
			    $$ = makeDefault();
			  }
                      | DECIMALTOKEN
                          {
			    $$ = makeDecimal();
			  }
                      | ABSOLUTETOKEN
                          {
			    $$ = makeAbsolute();
			  }
                      | RELATIVETOKEN
                          {
			    $$ = makeRelative();
			  }
                      | FIXEDTOKEN
                          {
			    $$ = makeFixed();
			  }
                      | FLOATINGTOKEN
                          {
			    $$ = makeFloating();
			  }
                      | ERRORTOKEN
                          {
			    $$ = makeError();
			  }
                      | DOUBLETOKEN
                          {
			    $$ = makeDoubleSymbol();
			  }
                      | SINGLETOKEN
                          {
			    $$ = makeSingleSymbol();
			  }
                      | QUADTOKEN
                          {
			    $$ = makeQuadSymbol();
			  }
                      | HALFPRECISIONTOKEN
                          {
			    $$ = makeHalfPrecisionSymbol();
			  }
                      | DOUBLEEXTENDEDTOKEN
                          {
			    $$ = makeDoubleextendedSymbol();
			  }
                      | DOUBLEDOUBLETOKEN
                          {
			    $$ = makeDoubleDoubleSymbol();
			  }
                      | TRIPLEDOUBLETOKEN
                          {
			    $$ = makeTripleDoubleSymbol();
			  }
                      | STRINGTOKEN
                          {
			    tempString = safeCalloc(strlen($1) + 1, sizeof(char));
			    strcpy(tempString, $1);
			    free($1);
			    tempString2 = safeCalloc(strlen(tempString) + 1, sizeof(char));
			    strcpy(tempString2, tempString);
			    free(tempString);
			    $$ = makeString(tempString2);
			    free(tempString2);
			  }
                      | constant
                          {
			    $$ = $1;
			  }
                      | IDENTIFIERTOKEN
                          {
			    $$ = makeTableAccess($1);
			    free($1);
			  }
                      | ISBOUNDTOKEN LPARTOKEN IDENTIFIERTOKEN RPARTOKEN
                          {
			    $$ = makeIsBound($3);
			    free($3);
			  }
                      | IDENTIFIERTOKEN LPARTOKEN thinglist RPARTOKEN
                          {
			    $$ = makeTableAccessWithSubstitute($1, $3);
			    free($1);
			  }
                      | IDENTIFIERTOKEN LPARTOKEN RPARTOKEN
                          {
			    $$ = makeTableAccessWithSubstitute($1, NULL);
			    free($1);
			  }
                      | list
                          {
			    $$ = $1;
			  }
                      | range
                          {
			    $$ = $1;
			  }
                      | debound
                          {
			    $$ = $1;
			  }
                      | headfunction
                          {
			    $$ = $1;
			  }
                      | LPARTOKEN thing RPARTOKEN
                          {
			    $$ = $2;
			  }
                      | LEFTCURLYBRACETOKEN structelementlist RIGHTCURLYBRACETOKEN
		          {
			    $$ = makeStructure($2);
			  }
                      | statedereference
                          {
			    $$ = $1;
			  }
                      | indexing
                          {
			    $$ = makeIndex($1->a, $1->b);
			    free($1);
			  }
                      | basicthing DOTTOKEN IDENTIFIERTOKEN 
		          {
			    $$ = makeStructAccess($1,$3);
			    free($3);
			  }
                      | basicthing DOTTOKEN IDENTIFIERTOKEN LPARTOKEN thinglist RPARTOKEN
		          {
			    $$ = makeApply(makeStructAccess($1,$3),$5);
			    free($3);
			  }
                      | LPARTOKEN thing RPARTOKEN LPARTOKEN thinglist RPARTOKEN
                          {
			    $$ = makeApply($2,$5);
			  }
                      | PROCTOKEN procbody
                          {
			    $$ = $2;
			  }
                      | TIMETOKEN LPARTOKEN command RPARTOKEN
                          {
			    $$ = makeTime($3);
                          }
;

matchlist:              matchelement
                          {
			    $$ = addElement(NULL,$1);
			  }
                      | matchelement matchlist
		          {
			    $$ = addElement($2,$1);
			  }
;

matchelement:          thing COLONTOKEN beginsymbol variabledeclarationlist commandlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeMatchElement($1,makeCommandList(concatChains($4, $5)),$7);
			  }
                      | thing COLONTOKEN beginsymbol variabledeclarationlist commandlist endsymbol
                          {
			    $$ = makeMatchElement($1,makeCommandList(concatChains($4, $5)),makeUnit());
			  }
                      | thing COLONTOKEN beginsymbol variabledeclarationlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeMatchElement($1,makeCommandList($4),$6);
			  }
                      | thing COLONTOKEN beginsymbol variabledeclarationlist endsymbol
                          {
			    $$ = makeMatchElement($1,makeCommandList($4),makeUnit());
			  }
                      | thing COLONTOKEN beginsymbol commandlist RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeMatchElement($1,makeCommandList($4),$6);
			  }
                      | thing COLONTOKEN beginsymbol commandlist endsymbol
                          {
			    $$ = makeMatchElement($1,makeCommandList($4),makeUnit());
			  }
                      | thing COLONTOKEN beginsymbol RETURNTOKEN thing SEMICOLONTOKEN endsymbol
                          {
			    $$ = makeMatchElement($1, makeCommandList(addElement(NULL,makeNop())), $5);
			  }
                      | thing COLONTOKEN beginsymbol endsymbol
                          {
			    $$ = makeMatchElement($1, makeCommandList(addElement(NULL,makeNop())), makeUnit());
			  }
                      | thing COLONTOKEN LPARTOKEN thing RPARTOKEN
		          {
			    $$ = makeMatchElement($1, makeCommandList(addElement(NULL,makeNop())), $4);
			  } 
;

constant:               CONSTANTTOKEN
                          {
			    $$ = makeDecimalConstant($1);
			    free($1);
			  }
                      | MIDPOINTCONSTANTTOKEN
                          {
			    $$ = makeMidpointConstant($1);
			    free($1);
			  }
                      | DYADICCONSTANTTOKEN
                          {
			    $$ = makeDyadicConstant($1);
			    free($1);
			  }
                      | HEXCONSTANTTOKEN
                          {
			    $$ = makeHexConstant($1);
			    free($1);
			  }
                      | HEXADECIMALCONSTANTTOKEN
                          {
			    $$ = makeHexadecimalConstant($1);
			    free($1);
			  }
                      | BINARYCONSTANTTOKEN
                          {
			    $$ = makeBinaryConstant($1);
			    free($1);
			  }
                      | PITOKEN
                          {
			    $$ = makePi();
			  }
;



list:                   LBRACKETTOKEN VERTBARTOKEN VERTBARTOKEN RBRACKETTOKEN
                          {
			    $$ = makeEmptyList();
			  }
                      | LBRACKETTOKEN ORTOKEN RBRACKETTOKEN
                          {
			    $$ = makeEmptyList();
			  }
		      | LBRACKETTOKEN VERTBARTOKEN simplelist VERTBARTOKEN RBRACKETTOKEN
                          {
			    $$ = makeRevertedList($3);
			  }
                      | LBRACKETTOKEN VERTBARTOKEN simplelist DOTSTOKEN VERTBARTOKEN RBRACKETTOKEN
                          {
			    $$ = makeRevertedFinalEllipticList($3);
			  }
;

simplelist:             thing
                          {
			    $$ = addElement(NULL, $1);
			  }
                      | simplelist COMMATOKEN thing
                          {
			    $$ = addElement($1, $3);
			  }
                      | simplelist COMMATOKEN DOTSTOKEN COMMATOKEN thing
                          {
			    $$ = addElement(addElement($1, makeElliptic()), $5);
			  }
;


range:                  LBRACKETTOKEN thing COMMATOKEN thing RBRACKETTOKEN
                          {
			    $$ = makeRange($2, $4);
			  }
                      | LBRACKETTOKEN thing SEMICOLONTOKEN thing RBRACKETTOKEN
                          {
			    $$ = makeRange($2, $4);
			  }
                      | LBRACKETTOKEN thing RBRACKETTOKEN
                          {
			    $$ = makeRange($2, copyThing($2));
			  }
;

debound:                STARLEFTANGLETOKEN thing RIGHTANGLESTARTOKEN
                          {
			    $$ = makeDeboundMax($2);
			  }
                      | STARLEFTANGLETOKEN thing RIGHTANGLEDOTTOKEN
                          {
			    $$ = makeDeboundMid($2);
			  }
                      | STARLEFTANGLETOKEN thing RIGHTANGLEUNDERSCORETOKEN
                          {
			    $$ = makeDeboundMin($2);
			  }
                      | SUPTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeDeboundMax($3);
			  }
                      | MIDTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeDeboundMid($3);
			  }
                      | INFTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeDeboundMin($3);
			  }
;

headfunction:           DIFFTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeDiff($3);
			  }
                      | SIMPLIFYTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeSimplify($3);
			  }
                      | BASHEVALUATETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeBashevaluate(addElement(NULL,$3));
			  }
                      | BASHEVALUATETOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeBashevaluate(addElement(addElement(NULL,$5),$3));
			  }
                      | REMEZTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thinglist RPARTOKEN
                          {
			    $$ = makeRemez(addElement(addElement($7, $5), $3));
			  }
                      | MINTOKEN LPARTOKEN thinglist RPARTOKEN
                          {
			    $$ = makeMin($3);
			  }
                      | MAXTOKEN LPARTOKEN thinglist RPARTOKEN
                          {
			    $$ = makeMax($3);
			  }
                      | FPMINIMAXTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thing COMMATOKEN thinglist RPARTOKEN
                          {
			    $$ = makeFPminimax(addElement(addElement(addElement($9, $7), $5), $3));
			  }
                      | HORNERTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeHorner($3);
			  }
                      | CANONICALTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeCanonicalThing($3);
			  }
                      | EXPANDTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeExpand($3);
			  }
                      | SIMPLIFYSAFETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeSimplifySafe($3);
			  }
                      | TAYLORTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeTaylor($3, $5, $7);
			  }
                      | TAYLORFORMTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thinglist RPARTOKEN
                          {
                            $$ = makeTaylorform(addElement(addElement($7, $5), $3));
			  }
                      | AUTODIFFTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thing RPARTOKEN
                          {
                            $$ = makeAutodiff(addElement(addElement(addElement(NULL, $7), $5), $3));
			  }
                      | DEGREETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeDegree($3);
			  }
                      | NUMERATORTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeNumerator($3);
			  }
                      | DENOMINATORTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeDenominator($3);
			  }
                      | SUBSTITUTETOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeSubstitute($3, $5);
			  }
                      | COEFFTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeCoeff($3, $5);
			  }
                      | SUBPOLYTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeSubpoly($3, $5);
			  }
                      | ROUNDCOEFFICIENTSTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeRoundcoefficients($3, $5);
			  }
                      | RATIONALAPPROXTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeRationalapprox($3, $5);
			  }
                      | ACCURATEINFNORMTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thinglist RPARTOKEN
                          {
			    $$ = makeAccurateInfnorm(addElement(addElement($7, $5), $3));
			  }
                      | ROUNDTOFORMATTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeRoundToFormat($3, $5, $7);
			  }
                      | EVALUATETOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeEvaluate($3, $5);
			  }
                      | PARSETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeParse($3);
			  }
                      | READXMLTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeReadXml($3);
			  }
                      | INFNORMTOKEN LPARTOKEN thing COMMATOKEN thinglist RPARTOKEN
                          {
			    $$ = makeInfnorm(addElement($5, $3));
			  }
                      | SUPNORMTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thing COMMATOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeSupnorm(addElement(addElement(addElement(addElement(addElement(NULL,$11),$9),$7),$5),$3));
			  }
                      | FINDZEROSTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeFindZeros($3, $5);
			  }
                      | FPFINDZEROSTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeFPFindZeros($3, $5);
			  }
                      | DIRTYINFNORMTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeDirtyInfnorm($3, $5);
			  }
                      | NUMBERROOTSTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeNumberRoots($3, $5);
			  }
                      | INTEGRALTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeIntegral($3, $5);
			  }
                      | DIRTYINTEGRALTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeDirtyIntegral($3, $5);
			  }
                      | IMPLEMENTPOLYTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thing COMMATOKEN thing COMMATOKEN thing COMMATOKEN thinglist RPARTOKEN
                          {
			    $$ = makeImplementPoly(addElement(addElement(addElement(addElement(addElement($13, $11), $9), $7), $5), $3));
			  }
                      | CHECKINFNORMTOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeCheckInfnorm($3, $5, $7);
			  }
                      | ZERODENOMINATORSTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeZeroDenominators($3, $5);
			  }
                      | ISEVALUABLETOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeIsEvaluable($3, $5);
			  }
                      | SEARCHGALTOKEN LPARTOKEN thinglist RPARTOKEN
                          {
			    $$ = makeSearchGal($3);
			  }
                      | GUESSDEGREETOKEN LPARTOKEN thing COMMATOKEN thing COMMATOKEN thinglist RPARTOKEN
                          {
			    $$ = makeGuessDegree(addElement(addElement($7, $5), $3));
			  }
                      | DIRTYFINDZEROSTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeDirtyFindZeros($3, $5);
			  }
                      | HEADTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeHead($3);
			  }
                      | ROUNDCORRECTLYTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeRoundCorrectly($3);
			  }
                      | READFILETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeReadFile($3);
			  }
                      | REVERTTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeRevert($3);
			  }
                      | SORTTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeSort($3);
			  }
                      | MANTISSATOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeMantissa($3);
			  }
                      | EXPONENTTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeExponent($3);
			  }
                      | PRECISIONTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makePrecision($3);
			  }
                      | TAILTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeTail($3);
			  }
                      | SQRTTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeSqrt($3);
			  }
                      | EXPTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeExp($3);
			  }
                      | FUNCTIONTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeProcedureFunction($3);
			  }
                      | FUNCTIONTOKEN LPARTOKEN thing COMMATOKEN thing RPARTOKEN
                          {
			    $$ = makeSubstitute(makeProcedureFunction($3),$5);
			  }
                      | LOGTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeLog($3);
			  }
                      | LOG2TOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeLog2($3);
			  }
                      | LOG10TOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeLog10($3);
			  }
                      | SINTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeSin($3);
			  }
                      | COSTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeCos($3);
			  }
                      | TANTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeTan($3);
			  }
                      | ASINTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeAsin($3);
			  }
                      | ACOSTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeAcos($3);
			  }
                      | ATANTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeAtan($3);
			  }
                      | SINHTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeSinh($3);
			  }
                      | COSHTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeCosh($3);
			  }
                      | TANHTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeTanh($3);
			  }
                      | ASINHTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeAsinh($3);
			  }
                      | ACOSHTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeAcosh($3);
			  }
                      | ATANHTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeAtanh($3);
			  }
                      | ABSTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeAbs($3);
			  }
                      | ERFTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeErf($3);
			  }
                      | ERFCTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeErfc($3);
			  }
                      | LOG1PTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeLog1p($3);
			  }
                      | EXPM1TOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeExpm1($3);
			  }
                      | DOUBLETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeDouble($3);
			  }
                      | SINGLETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeSingle($3);
			  }
                      | QUADTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeQuad($3);
			  }
                      | HALFPRECISIONTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeHalfPrecision($3);
			  }
                      | DOUBLEDOUBLETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeDoubledouble($3);
			  }
                      | TRIPLEDOUBLETOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeTripledouble($3);
			  }
                      | DOUBLEEXTENDEDTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeDoubleextended($3);
			  }
                      | CEILTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeCeil($3);
			  }
                      | FLOORTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeFloor($3);
			  }
                      | NEARESTINTTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeNearestInt($3);
			  }
                      | LENGTHTOKEN LPARTOKEN thing RPARTOKEN
                          {
			    $$ = makeLength($3);
			  }
;

egalquestionmark:       EQUALTOKEN QUESTIONMARKTOKEN
                          {
			    $$ = NULL;
			  }
                      |
                          {
			    $$ = NULL;
			  }
;

statedereference:       PRECTOKEN egalquestionmark
                          {
			    $$ = makePrecDeref();
			  }
                      | POINTSTOKEN egalquestionmark
                          {
			    $$ = makePointsDeref();
			  }
                      | DIAMTOKEN egalquestionmark
                          {
			    $$ = makeDiamDeref();
			  }
                      | DISPLAYTOKEN egalquestionmark
                          {
			    $$ = makeDisplayDeref();
			  }
                      | VERBOSITYTOKEN egalquestionmark
                          {
			    $$ = makeVerbosityDeref();
			  }
                      | CANONICALTOKEN egalquestionmark
                          {
			    $$ = makeCanonicalDeref();
			  }
                      | AUTOSIMPLIFYTOKEN egalquestionmark
                          {
			    $$ = makeAutoSimplifyDeref();
			  }
                      | TAYLORRECURSIONSTOKEN egalquestionmark
                          {
			    $$ = makeTaylorRecursDeref();
			  }
                      | TIMINGTOKEN egalquestionmark
                          {
			    $$ = makeTimingDeref();
			  }
                      | FULLPARENTHESESTOKEN egalquestionmark
                          {
			    $$ = makeFullParenDeref();
			  }
                      | MIDPOINTMODETOKEN egalquestionmark
                          {
			    $$ = makeMidpointDeref();
			  }
                      | DIEONERRORMODETOKEN egalquestionmark
                          {
			    $$ = makeDieOnErrorDeref();
			  }
                      | RATIONALMODETOKEN egalquestionmark
                          {
			    $$ = makeRationalModeDeref();
			  }
                      | SUPPRESSWARNINGSTOKEN egalquestionmark
                          {
			    $$ = makeSuppressWarningsDeref();
			  }
                      | HOPITALRECURSIONSTOKEN egalquestionmark
                          {
			    $$ = makeHopitalRecursDeref();
			  }
;

externalproctype:       CONSTANTTYPETOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = CONSTANT_TYPE;
			    $$ = tempIntPtr;
			  }
                      | FUNCTIONTOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = FUNCTION_TYPE;
			    $$ = tempIntPtr;
			  }
                      | RANGETOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = RANGE_TYPE;
			    $$ = tempIntPtr;
			  }
                      | INTEGERTOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = INTEGER_TYPE;
			    $$ = tempIntPtr;
			  }
                      | STRINGTYPETOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = STRING_TYPE;
			    $$ = tempIntPtr;
			  }
                      | BOOLEANTOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = BOOLEAN_TYPE;
			    $$ = tempIntPtr;
			  }
                      | LISTTOKEN OFTOKEN CONSTANTTYPETOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = CONSTANT_LIST_TYPE;
			    $$ = tempIntPtr;
			  }
                      | LISTTOKEN OFTOKEN FUNCTIONTOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = FUNCTION_LIST_TYPE;
			    $$ = tempIntPtr;
			  }
                      | LISTTOKEN OFTOKEN RANGETOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = RANGE_LIST_TYPE;
			    $$ = tempIntPtr;
			  }
                      | LISTTOKEN OFTOKEN INTEGERTOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = INTEGER_LIST_TYPE;
			    $$ = tempIntPtr;
			  }
                      | LISTTOKEN OFTOKEN STRINGTYPETOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = STRING_LIST_TYPE;
			    $$ = tempIntPtr;
			  }
                      | LISTTOKEN OFTOKEN BOOLEANTOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = BOOLEAN_LIST_TYPE;
			    $$ = tempIntPtr;
			  }
;

extendedexternalproctype: VOIDTOKEN
                          {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = VOID_TYPE;
			    $$ = tempIntPtr;
			  }
                      | externalproctype
		          {
			    $$ = $1;
		          }
;


externalproctypesimplelist:   externalproctype
                          {
			    $$ = addElement(NULL, $1);
			  }
                      | externalproctype COMMATOKEN externalproctypesimplelist
                          {
			    $$ = addElement($3, $1);
			  }
;

externalproctypelist:       extendedexternalproctype
                          {
			    $$ = addElement(NULL, $1);
			  }
                      | LPARTOKEN externalproctypesimplelist RPARTOKEN
                          {
			    $$ = $2;
			  }
;


help:                   CONSTANTTOKEN
                          {
			    outputMode(); sollyaPrintf("\"%s\" is recognized as a base 10 constant.\n",$1);
			    free($1);
			  }
                      | DYADICCONSTANTTOKEN
                          {
			    outputMode(); sollyaPrintf("\"%s\" is recognized as a dyadic number constant.\n",$1);
			    free($1);
                          }
                      | HEXCONSTANTTOKEN
                          {
			    outputMode(); sollyaPrintf("\"%s\" is recognized as a double or single precision constant.\n",$1);
			    free($1);
                          }
                      | HEXADECIMALCONSTANTTOKEN
                          {
			    outputMode(); sollyaPrintf("\"%s\" is recognized as a hexadecimal constant.\n",$1);
			    free($1);
                          }
                      | BINARYCONSTANTTOKEN
                          {
			    outputMode(); sollyaPrintf("\"%s_2\" is recognized as a base 2 constant.\n",$1);
			    free($1);
                          }
                      | PITOKEN
                          {
#ifdef HELP_PI_TEXT
			    outputMode(); sollyaPrintf(HELP_PI_TEXT);
#else
			    outputMode(); sollyaPrintf("Ratio circonference and diameter of a circle.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PI"
#endif
#endif
                          }
                      | IDENTIFIERTOKEN
                          {
			    outputMode(); sollyaPrintf("\"%s\" is an identifier.\n",$1);
			    free($1);
                          }
                      | STRINGTOKEN
                          {
			    outputMode(); sollyaPrintf("\"%s\" is a string constant.\n",$1);
			    free($1);
                          }
                      | LPARTOKEN
                          {
			    outputMode(); sollyaPrintf("Left parenthesis.\n");
                          }
                      | RPARTOKEN
                          {
			    outputMode(); sollyaPrintf("Right parenthesis.\n");
                          }
                      | LBRACKETTOKEN
                          {
			    outputMode(); sollyaPrintf("Left bracket - indicates a range.\n");
                          }
                      | RBRACKETTOKEN
                          {
			    outputMode(); sollyaPrintf("Right bracket - indicates a range.\n");
                          }
                      | LBRACKETTOKEN VERTBARTOKEN
                          {
			    outputMode(); sollyaPrintf("Left bracket-bar - indicates a list.\n");
                          }
                      | VERTBARTOKEN RBRACKETTOKEN
                          {
			    outputMode(); sollyaPrintf("Bar-right bracket - indicates a list.\n");
                          }
                      | EQUALTOKEN
                          {
#ifdef HELP_ASSIGNMENT_TEXT
			    outputMode(); sollyaPrintf(HELP_ASSIGNMENT_TEXT);
#else
			    outputMode(); sollyaPrintf("Assignment operator.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ASSIGNMENT"
#endif
#endif
                          }
                      | ASSIGNEQUALTOKEN
                          {
#ifdef HELP_FLOATASSIGNMENT_TEXT
			    outputMode(); sollyaPrintf(HELP_FLOATASSIGNMENT_TEXT);
#else
			    outputMode(); sollyaPrintf("Evaluating assignment operator.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FLOATASSIGNMENT"
#endif
#endif
                          }
                      | COMPAREEQUALTOKEN
                          {
#ifdef HELP_EQUAL_TEXT
			    outputMode(); sollyaPrintf(HELP_EQUAL_TEXT);
#else
			    outputMode(); sollyaPrintf("Equality test.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for EQUAL"
#endif
#endif
                          }
                      | COMMATOKEN
                          {
			    outputMode(); sollyaPrintf("Separator in lists or ranges.\n");
                          }
                      | EXCLAMATIONTOKEN
                          {
#ifdef HELP_NOT_TEXT
			    outputMode(); sollyaPrintf(HELP_NOT_TEXT);
#else
			    outputMode(); sollyaPrintf("Suppresses output on assignments or boolean negation.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for NOT"
#endif
#endif
                          }
                      | STARLEFTANGLETOKEN
                          {
			    outputMode(); sollyaPrintf("Dereferences range bounds.\n");
                          }
                      | LEFTANGLETOKEN
                          {
#ifdef HELP_LT_TEXT
			    outputMode(); sollyaPrintf(HELP_LT_TEXT);
#else
			    outputMode(); sollyaPrintf("Comparison less than.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LT"
#endif
#endif
                          }
                      | LEFTANGLETOKEN EQUALTOKEN
                          {
#ifdef HELP_LE_TEXT
			    outputMode(); sollyaPrintf(HELP_LE_TEXT);
#else
			    outputMode(); sollyaPrintf("Comparison less than or equal to.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LE"
#endif
#endif
                          }
                      | RIGHTANGLEUNDERSCORETOKEN
                          {
			    outputMode(); sollyaPrintf("Dereferences the lower range bound.\n");
                          }
                      | RIGHTANGLEDOTTOKEN
                          {
			    outputMode(); sollyaPrintf("Dereferences the mid-point of a range.\n");
                          }
                      | RIGHTANGLETOKEN EQUALTOKEN
                          {
#ifdef HELP_GE_TEXT
			    outputMode(); sollyaPrintf(HELP_GE_TEXT);
#else
			    outputMode(); sollyaPrintf("Comparison greater than or equal to.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for GE"
#endif
#endif
			  }
                      | DOTTOKEN
                          {
			    outputMode(); sollyaPrintf("Accessing an element in a structured type.\n");
			  }
                      | RIGHTANGLESTARTOKEN
                          {
			    outputMode(); sollyaPrintf("Dereferences the upper range bound.\n");
                          }
                      | RIGHTANGLETOKEN
                          {
#ifdef HELP_GT_TEXT
			    outputMode(); sollyaPrintf(HELP_GT_TEXT);
#else
			    outputMode(); sollyaPrintf("Comparison greater than.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for GT"
#endif
#endif
                          }
                      | DOTSTOKEN
                          {
			    outputMode(); sollyaPrintf("Ellipsis.\n");
                          }
                      | QUESTIONMARKTOKEN
                          {
			    outputMode(); sollyaPrintf("Dereferences global environment variables.\n");
                          }
                      | VERTBARTOKEN
                          {
			    outputMode(); sollyaPrintf("Starts or ends a list.\n");
                          }
                      | ATTOKEN
                          {
#ifdef HELP_CONCAT_TEXT
			    outputMode(); sollyaPrintf(HELP_CONCAT_TEXT);
#else
			    outputMode(); sollyaPrintf("Concatenation of lists or strings.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for CONCAT"
#endif
#endif
                          }
                      | DOUBLECOLONTOKEN
                          {
			    outputMode(); sollyaPrintf("a::b prepends a to list b or appends b to list a, preprending list a to list b if both are lists.\n");
                          }
                      | DOTCOLONTOKEN
                          {
#ifdef HELP_PREPEND_TEXT
			    outputMode(); sollyaPrintf(HELP_PREPEND_TEXT);
#else
			    outputMode(); sollyaPrintf("a.:b prepends a to list b.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PREPEND"
#endif
#endif
                          }
                      | COLONDOTTOKEN
                          {
#ifdef HELP_APPEND_TEXT
			    outputMode(); sollyaPrintf(HELP_APPEND_TEXT);
#else
			    outputMode(); sollyaPrintf("a:.b appends b to list a.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for APPEND"
#endif
#endif
                          }
                      | EXCLAMATIONEQUALTOKEN
                          {
#ifdef HELP_NEQ_TEXT
			    outputMode(); sollyaPrintf(HELP_NEQ_TEXT);
#else
			    outputMode(); sollyaPrintf("Comparison not equal.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for NEQ"
#endif
#endif
                          }
                      | ANDTOKEN
                          {
#ifdef HELP_AND_TEXT
			    outputMode(); sollyaPrintf(HELP_AND_TEXT);
#else
			    outputMode(); sollyaPrintf("Boolean and.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for AND"
#endif
#endif
                          }
                      | ORTOKEN
                          {
#ifdef HELP_OR_TEXT
			    outputMode(); sollyaPrintf(HELP_OR_TEXT);
#else
			    outputMode(); sollyaPrintf("Boolean or.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for OR"
#endif
#endif
                          }
                      | PLUSTOKEN
                          {
#ifdef HELP_PLUS_TEXT
			    outputMode(); sollyaPrintf(HELP_PLUS_TEXT);
#else
			    outputMode(); sollyaPrintf("Addition.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PLUS"
#endif
#endif
                          }
                      | MINUSTOKEN
                          {
#ifdef HELP_MINUS_TEXT
			    outputMode(); sollyaPrintf(HELP_MINUS_TEXT);
#else
			    outputMode(); sollyaPrintf("Substraction.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for MINUS"
#endif
#endif
                          }
                      | APPROXTOKEN
                          {
#ifdef HELP_APPROX_TEXT
			    outputMode(); sollyaPrintf(HELP_APPROX_TEXT);
#else
			    outputMode(); sollyaPrintf("Floating-point approximation of a constant expression.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for APPROX"
#endif
#endif
                          }
                      | MULTOKEN
                          {
#ifdef HELP_MULT_TEXT
			    outputMode(); sollyaPrintf(HELP_MULT_TEXT);
#else
			    outputMode(); sollyaPrintf("Multiplication.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for MULT"
#endif
#endif
                          }
                      | DIVTOKEN
                          {
#ifdef HELP_DIVIDE_TEXT
			    outputMode(); sollyaPrintf(HELP_DIVIDE_TEXT);
#else
			    outputMode(); sollyaPrintf("Division.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DIVIDE"
#endif
#endif
                          }
                      | POWTOKEN
                          {
#ifdef HELP_POWER_TEXT
			    outputMode(); sollyaPrintf(HELP_POWER_TEXT);
#else
			    outputMode(); sollyaPrintf("Exponentiation.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for POWER"
#endif
#endif
                          }
                      | SQRTTOKEN
                          {
#ifdef HELP_SQRT_TEXT
			    outputMode(); sollyaPrintf(HELP_SQRT_TEXT);
#else
			    outputMode(); sollyaPrintf("Square root.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SQRT"
#endif
#endif
                          }
                      | EXPTOKEN
                          {
#ifdef HELP_EXP_TEXT
			    outputMode(); sollyaPrintf(HELP_EXP_TEXT);
#else
			    outputMode(); sollyaPrintf("Exponential.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for EXP"
#endif
#endif
                          }
                      | LOGTOKEN
                          {
#ifdef HELP_LOG_TEXT
			    outputMode(); sollyaPrintf(HELP_LOG_TEXT);
#else
			    outputMode(); sollyaPrintf("Natural logarithm.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LOG"
#endif
#endif
                          }
                      | LOG2TOKEN
                          {
#ifdef HELP_LOG2_TEXT
			    outputMode(); sollyaPrintf(HELP_LOG2_TEXT);
#else
			    outputMode(); sollyaPrintf("Logarithm in base 2.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LOG2"
#endif
#endif
                          }
                      | LOG10TOKEN
                          {
#ifdef HELP_LOG10_TEXT
			    outputMode(); sollyaPrintf(HELP_LOG10_TEXT);
#else
			    outputMode(); sollyaPrintf("Logarithm in base 10.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LOG10"
#endif
#endif
                          }
                      | SINTOKEN
                          {
#ifdef HELP_SIN_TEXT
			    outputMode(); sollyaPrintf(HELP_SIN_TEXT);
#else
			    outputMode(); sollyaPrintf("Sine.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SIN"
#endif
#endif
                          }
                      | COSTOKEN
                          {
#ifdef HELP_COS_TEXT
			    outputMode(); sollyaPrintf(HELP_COS_TEXT);
#else
			    outputMode(); sollyaPrintf("Cosine.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for COS"
#endif
#endif
                          }
                      | TANTOKEN
                          {
#ifdef HELP_TAN_TEXT
			    outputMode(); sollyaPrintf(HELP_TAN_TEXT);
#else
			    outputMode(); sollyaPrintf("Tangent.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for TAN"
#endif
#endif
                          }
                      | ASINTOKEN
                          {
#ifdef HELP_ASIN_TEXT
			    outputMode(); sollyaPrintf(HELP_ASIN_TEXT);
#else
			    outputMode(); sollyaPrintf("Arcsine.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ASIN"
#endif
#endif
                          }
                      | ACOSTOKEN
                          {
#ifdef HELP_ACOS_TEXT
			    outputMode(); sollyaPrintf(HELP_ACOS_TEXT);
#else
			    outputMode(); sollyaPrintf("Arcosine.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ACOS"
#endif
#endif
                          }
                      | ATANTOKEN
                          {
#ifdef HELP_ATAN_TEXT
			    outputMode(); sollyaPrintf(HELP_ATAN_TEXT);
#else
			    outputMode(); sollyaPrintf("Arctangent.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ATAN"
#endif
#endif
                          }
                      | SINHTOKEN
                          {
#ifdef HELP_SINH_TEXT
			    outputMode(); sollyaPrintf(HELP_SINH_TEXT);
#else
			    outputMode(); sollyaPrintf("Hyperbolic sine.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SINH"
#endif
#endif
                          }
                      | COSHTOKEN
                          {
#ifdef HELP_COSH_TEXT
			    outputMode(); sollyaPrintf(HELP_COSH_TEXT);
#else
			    outputMode(); sollyaPrintf("Hyperbolic cosine.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for COSH"
#endif
#endif
                          }
                      | TANHTOKEN
                          {
#ifdef HELP_TANH_TEXT
			    outputMode(); sollyaPrintf(HELP_TANH_TEXT);
#else
			    outputMode(); sollyaPrintf("Hyperbolic tangent.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for TANH"
#endif
#endif
                          }
                      | ASINHTOKEN
                          {
#ifdef HELP_ASINH_TEXT
			    outputMode(); sollyaPrintf(HELP_ASINH_TEXT);
#else
			    outputMode(); sollyaPrintf("Area sine.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ASINH"
#endif
#endif
                          }
                      | ACOSHTOKEN
                          {
#ifdef HELP_ACOSH_TEXT
			    outputMode(); sollyaPrintf(HELP_ACOSH_TEXT);
#else
			    outputMode(); sollyaPrintf("Area cosine.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ACOSH"
#endif
#endif
                          }
                      | ATANHTOKEN
                          {
#ifdef HELP_ATANH_TEXT
			    outputMode(); sollyaPrintf(HELP_ATANH_TEXT);
#else

			    outputMode(); sollyaPrintf("Area tangent.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ATANH"
#endif
#endif
                          }
                      | ABSTOKEN
                          {
#ifdef HELP_ABS_TEXT
			    outputMode(); sollyaPrintf(HELP_ABS_TEXT);
#else
			    outputMode(); sollyaPrintf("Absolute value.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ABS"
#endif
#endif
                          }
                      | ERFTOKEN
                          {
#ifdef HELP_ERF_TEXT
			    outputMode(); sollyaPrintf(HELP_ERF_TEXT);
#else
			    outputMode(); sollyaPrintf("Error function.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ERF"
#endif
#endif
                          }
                      | ERFCTOKEN
                          {
#ifdef HELP_ERFC_TEXT
			    outputMode(); sollyaPrintf(HELP_ERFC_TEXT);
#else
			    outputMode(); sollyaPrintf("Complementary error function.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ERFC"
#endif
#endif
                          }
                      | LOG1PTOKEN
                          {
#ifdef HELP_LOG1P_TEXT
			    outputMode(); sollyaPrintf(HELP_LOG1P_TEXT);
#else
			    outputMode(); sollyaPrintf("Natural logarithm of 1 plus argument.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LOG1P"
#endif
#endif
                          }
                      | EXPM1TOKEN
                          {
#ifdef HELP_EXPM1_TEXT
			    outputMode(); sollyaPrintf(HELP_EXPM1_TEXT);
#else
			    outputMode(); sollyaPrintf("Exponential of argument minus 1.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for EXPM1"
#endif
#endif
                          }
                      | DOUBLETOKEN
                          {
#ifdef HELP_DOUBLE_TEXT
			    outputMode(); sollyaPrintf(HELP_DOUBLE_TEXT);
#else
			    outputMode(); sollyaPrintf("Double precision rounding operator.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DOUBLE"
#endif
#endif
                          }
                      | SINGLETOKEN
                          {
#ifdef HELP_SINGLE_TEXT
			    outputMode(); sollyaPrintf(HELP_SINGLE_TEXT);
#else
			    outputMode(); sollyaPrintf("Single precision rounding operator.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SINGLE"
#endif
#endif
                          }
                      | QUADTOKEN
                          {
#ifdef HELP_QUAD_TEXT
			    outputMode(); sollyaPrintf(HELP_QUAD_TEXT);
#else
			    outputMode(); sollyaPrintf("Quad precision rounding operator.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for QUAD"
#endif
#endif
                          }
                      | HALFPRECISIONTOKEN
                          {
#ifdef HELP_HALFPRECISION_TEXT
			    outputMode(); sollyaPrintf(HELP_HALFPRECISION_TEXT);
#else
			    outputMode(); sollyaPrintf("Half-precision rounding operator.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for HALFPRECISION"
#endif
#endif
                          }
                      | DOUBLEDOUBLETOKEN
                          {
#ifdef HELP_DOUBLEDOUBLE_TEXT
			    outputMode(); sollyaPrintf(HELP_DOUBLEDOUBLE_TEXT);
#else
			    outputMode(); sollyaPrintf("Double-double precision rounding operator.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DOUBLEDOUBLE"
#endif
#endif
                          }
                      | TRIPLEDOUBLETOKEN
                          {
#ifdef HELP_TRIPLEDOUBLE_TEXT
			    outputMode(); sollyaPrintf(HELP_TRIPLEDOUBLE_TEXT);
#else
			    outputMode(); sollyaPrintf("Triple-double precision rounding operator.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for TRIPLEDOUBLE"
#endif
#endif
                          }
                      | DOUBLEEXTENDEDTOKEN
                          {
#ifdef HELP_DOUBLEEXTENDED_TEXT
			    outputMode(); sollyaPrintf(HELP_DOUBLEEXTENDED_TEXT);
#else
			    outputMode(); sollyaPrintf("Double-extended precision rounding operator.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DOUBLEEXTENDED"
#endif
#endif
                          }
                      | CEILTOKEN
                          {
#ifdef HELP_CEIL_TEXT
			    outputMode(); sollyaPrintf(HELP_CEIL_TEXT);
#else
			    outputMode(); sollyaPrintf("Ceiling.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for CEIL"
#endif
#endif
                          }
                      | FLOORTOKEN
                          {
#ifdef HELP_FLOOR_TEXT
			    outputMode(); sollyaPrintf(HELP_FLOOR_TEXT);
#else
			    outputMode(); sollyaPrintf("Floor.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FLOOR"
#endif
#endif
                          }
                      | NEARESTINTTOKEN
                          {
#ifdef HELP_NEARESTINT_TEXT
			    outputMode(); sollyaPrintf(HELP_NEARESTINT_TEXT);
#else
			    outputMode(); sollyaPrintf("Nearest integer with even tie cases rule.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for NEARESTINT"
#endif
#endif
                          }
                      | HEADTOKEN
                          {
#ifdef HELP_HEAD_TEXT
			    outputMode(); sollyaPrintf(HELP_HEAD_TEXT);
#else
			    outputMode(); sollyaPrintf("Head of a list.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for HEAD"
#endif
#endif
                          }
                      | ROUNDCORRECTLYTOKEN
                          {
#ifdef HELP_ROUNDCORRECTLY_TEXT
			    outputMode(); sollyaPrintf(HELP_ROUNDCORRECTLY_TEXT);
#else
			    outputMode(); sollyaPrintf("Round a bounding to the nearest floating-point value such that correct rounding is possible.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ROUNDCORRECTLY"
#endif
#endif
                          }
                      | READFILETOKEN
                          {
#ifdef HELP_READFILE_TEXT
			    outputMode(); sollyaPrintf(HELP_READFILE_TEXT);
#else
			    outputMode(); sollyaPrintf("Reads a file into a string.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for READFILE"
#endif
#endif
                          }
                      | REVERTTOKEN
                          {
#ifdef HELP_REVERT_TEXT
			    outputMode(); sollyaPrintf(HELP_REVERT_TEXT);
#else
			    outputMode(); sollyaPrintf("Reverts a list that is not finally elliptic.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for REVERT"
#endif
#endif
                          }
                      | SORTTOKEN
                          {
#ifdef HELP_SORT_TEXT
			    outputMode(); sollyaPrintf(HELP_SORT_TEXT);
#else
			    outputMode(); sollyaPrintf("Sorts a list of constants in ascending order.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SORT"
#endif
#endif
                          }
                      | TAILTOKEN
                          {
#ifdef HELP_TAIL_TEXT
			    outputMode(); sollyaPrintf(HELP_TAIL_TEXT);
#else
			    outputMode(); sollyaPrintf("Tail of a list.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for TAIL"
#endif
#endif
                          }
                      | PRECTOKEN
                          {
#ifdef HELP_PREC_TEXT
			    outputMode(); sollyaPrintf(HELP_PREC_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable precision.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PREC"
#endif
#endif
                          }
                      | POINTSTOKEN
                          {
#ifdef HELP_POINTS_TEXT
			    outputMode(); sollyaPrintf(HELP_POINTS_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable number of points.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for POINTS"
#endif
#endif
                          }
                      | DIAMTOKEN
                          {
#ifdef HELP_DIAM_TEXT
			    outputMode(); sollyaPrintf(HELP_DIAM_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable diameter.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DIAM"
#endif
#endif
                          }
                      | DISPLAYTOKEN
                          {
#ifdef HELP_DISPLAY_TEXT
			    outputMode(); sollyaPrintf(HELP_DISPLAY_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable display mode.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DISPLAY"
#endif
#endif
                          }
                      | VERBOSITYTOKEN
                          {
#ifdef HELP_VERBOSITY_TEXT
			    outputMode(); sollyaPrintf(HELP_VERBOSITY_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable verbosity.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for VERBOSITY"
#endif
#endif
                          }
                      | CANONICALTOKEN
                          {
#ifdef HELP_CANONICAL_TEXT
			    outputMode(); sollyaPrintf(HELP_CANONICAL_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable canonical output.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for CANONICAL"
#endif
#endif
                          }
                      | AUTOSIMPLIFYTOKEN
                          {
#ifdef HELP_AUTOSIMPLIFY_TEXT
			    outputMode(); sollyaPrintf(HELP_AUTOSIMPLIFY_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable automatic simplification.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for AUTOSIMPLIFY"
#endif
#endif
                          }
                      | TAYLORRECURSIONSTOKEN
                          {
#ifdef HELP_TAYLORRECURSIONS_TEXT
			    outputMode(); sollyaPrintf(HELP_TAYLORRECURSIONS_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable recursions of Taylor evaluation.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for TAYLORRECURSIONS"
#endif
#endif
                          }
                      | TIMINGTOKEN
                          {
#ifdef HELP_TIMING_TEXT
			    outputMode(); sollyaPrintf(HELP_TIMING_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable timing of computations.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for TIMING"
#endif
#endif
                          }
                      | TIMETOKEN
                          {
#ifdef HELP_TIME_TEXT
			    outputMode(); sollyaPrintf(HELP_TIME_TEXT);
#else
			    outputMode(); sollyaPrintf("High-level time procedure.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for TIME"
#endif
#endif
                          }
                      | FULLPARENTHESESTOKEN
                          {
#ifdef HELP_FULLPARENTHESES_TEXT
			    outputMode(); sollyaPrintf(HELP_FULLPARENTHESES_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable fully parenthized mode.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FULLPARENTHESES"
#endif
#endif
                          }
                      | MIDPOINTMODETOKEN
                          {
#ifdef HELP_MIDPOINTMODE_TEXT
			    outputMode(); sollyaPrintf(HELP_MIDPOINTMODE_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable midpoint mode.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for MIDPOINTMODE"
#endif
#endif
                          }
                      | DIEONERRORMODETOKEN
                          {
#ifdef HELP_DIEONERRORMODE_TEXT
			    outputMode(); sollyaPrintf(HELP_DIEONERRORMODE_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable for die-on-error mode.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DIEONERRORMODE"
#endif
#endif
                          }
                      | RATIONALMODETOKEN
                          {
#ifdef HELP_RATIONALMODE_TEXT
			    outputMode(); sollyaPrintf(HELP_RATIONALMODE_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable rational mode.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RATIONALMODE"
#endif
#endif
                          }
                      | SUPPRESSWARNINGSTOKEN
                          {
#ifdef HELP_ROUNDINGWARNINGS_TEXT
			    outputMode(); sollyaPrintf(HELP_ROUNDINGWARNINGS_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable activating warnings about rounding.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ROUNDINGWARNINGS"
#endif
#endif
                          }
                      | HOPITALRECURSIONSTOKEN
                          {
#ifdef HELP_HOPITALRECURSIONS_TEXT
			    outputMode(); sollyaPrintf(HELP_HOPITALRECURSIONS_TEXT);
#else
			    outputMode(); sollyaPrintf("Global environment variable recursions of Hopital evaluation.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for HOPITALRECURSIONS"
#endif
#endif
                          }
                      | ONTOKEN
                          {
#ifdef HELP_ON_TEXT
			    outputMode(); sollyaPrintf(HELP_ON_TEXT);
#else
			    outputMode(); sollyaPrintf("Something is switched on.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ON"
#endif
#endif
                          }
                      | OFFTOKEN
                          {
#ifdef HELP_OFF_TEXT
			    outputMode(); sollyaPrintf(HELP_OFF_TEXT);
#else
			    outputMode(); sollyaPrintf("Something is switched off.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for OFF"
#endif
#endif
                          }
                      | DYADICTOKEN
                          {
#ifdef HELP_DYADIC_TEXT
			    outputMode(); sollyaPrintf(HELP_DYADIC_TEXT);
#else
			    outputMode(); sollyaPrintf("Display mode is dyadic output.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DYADIC"
#endif
#endif
                          }
                      | POWERSTOKEN
                          {
#ifdef HELP_POWERS_TEXT
			    outputMode(); sollyaPrintf(HELP_POWERS_TEXT);
#else
			    outputMode(); sollyaPrintf("Display mode is dyadic output with powers.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for POWERS"
#endif
#endif
                          }
                      | BINARYTOKEN
                          {
#ifdef HELP_BINARY_TEXT
			    outputMode(); sollyaPrintf(HELP_BINARY_TEXT);
#else
			    outputMode(); sollyaPrintf("Display mode is binary.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for BINARY"
#endif
#endif
                          }
                      | HEXADECIMALTOKEN
                          {
#ifdef HELP_HEXADECIMAL_TEXT
			    outputMode(); sollyaPrintf(HELP_HEXADECIMAL_TEXT);
#else
			    outputMode(); sollyaPrintf("Display mode is hexadecimal.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for HEXADECIMAL"
#endif
#endif
                          }
                      | FILETOKEN
                          {
#ifdef HELP_FILE_TEXT
			    outputMode(); sollyaPrintf(HELP_FILE_TEXT);
#else
			    outputMode(); sollyaPrintf("A file will be specified.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FILE"
#endif
#endif
                          }
                      | POSTSCRIPTTOKEN
                          {
#ifdef HELP_POSTSCRIPT_TEXT
			    outputMode(); sollyaPrintf(HELP_POSTSCRIPT_TEXT);
#else
			    outputMode(); sollyaPrintf("A postscript file will be specified.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for POSTSCRIPT"
#endif
#endif
                          }
                      | POSTSCRIPTFILETOKEN
                          {
#ifdef HELP_POSTSCRIPTFILE_TEXT
			    outputMode(); sollyaPrintf(HELP_POSTSCRIPTFILE_TEXT);
#else
			    outputMode(); sollyaPrintf("A postscript file and a file will be specified.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for POSTSCRIPTFILE"
#endif
#endif
                          }
                      | PERTURBTOKEN
                          {
#ifdef HELP_PERTURB_TEXT
			    outputMode(); sollyaPrintf(HELP_PERTURB_TEXT);
#else
			    outputMode(); sollyaPrintf("Perturbation is demanded.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PERTURB"
#endif
#endif
                          }
                      | MINUSWORDTOKEN
                          {
#ifdef HELP_RD_TEXT
			    outputMode(); sollyaPrintf(HELP_RD_TEXT);
#else
			    outputMode(); sollyaPrintf("Round towards minus infinity.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RD"
#endif
#endif
                          }
                      | PLUSWORDTOKEN
                          {
#ifdef HELP_RU_TEXT
			    outputMode(); sollyaPrintf(HELP_RU_TEXT);
#else
			    outputMode(); sollyaPrintf("Round towards plus infinity.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RU"
#endif
#endif
                          }
                      | ZEROWORDTOKEN
                          {
#ifdef HELP_RZ_TEXT
			    outputMode(); sollyaPrintf(HELP_RZ_TEXT);
#else
			    outputMode(); sollyaPrintf("Round towards zero.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RZ"
#endif
#endif
                          }
                      | NEARESTTOKEN
                          {
#ifdef HELP_RN_TEXT
			    outputMode(); sollyaPrintf(HELP_RN_TEXT);
#else
			    outputMode(); sollyaPrintf("Round to nearest.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RN"
#endif
#endif
                          }
                      | HONORCOEFFPRECTOKEN
                          {
#ifdef HELP_HONORCOEFFPREC_TEXT
			    outputMode(); sollyaPrintf(HELP_HONORCOEFFPREC_TEXT);
#else
			    outputMode(); sollyaPrintf("Honorate the precision of the coefficients.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for HONORCOEFFPREC"
#endif
#endif
                          }
                      | TRUETOKEN
                          {
#ifdef HELP_TRUE_TEXT
			    outputMode(); sollyaPrintf(HELP_TRUE_TEXT);
#else
			    outputMode(); sollyaPrintf("Boolean constant true.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for TRUE"
#endif
#endif
                          }
                      | FALSETOKEN
                          {
#ifdef HELP_FALSE_TEXT
			    outputMode(); sollyaPrintf(HELP_FALSE_TEXT);
#else
			    outputMode(); sollyaPrintf("Boolean constant false.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FALSE"
#endif
#endif
                          }
                      | DEFAULTTOKEN
                          {
#ifdef HELP_DEFAULT_TEXT
			    outputMode(); sollyaPrintf(HELP_DEFAULT_TEXT);
#else
			    outputMode(); sollyaPrintf("Default value.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DEFAULT"
#endif
#endif
                          }
                      | MATCHTOKEN
                          {
#ifdef HELP_MATCH_TEXT
			    outputMode(); sollyaPrintf(HELP_MATCH_TEXT);
#else
			    outputMode(); sollyaPrintf("match ... with ... construct.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for MATCH"
#endif
#endif
                          }
                      | WITHTOKEN
                          {
#ifdef HELP_WITH_TEXT
			    outputMode(); sollyaPrintf(HELP_WITH_TEXT);
#else
			    outputMode(); sollyaPrintf("match ... with ... construct.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for WITH"
#endif
#endif
                          }
                      | ABSOLUTETOKEN
                          {
#ifdef HELP_ABSOLUTE_TEXT
			    outputMode(); sollyaPrintf(HELP_ABSOLUTE_TEXT);
#else
			    outputMode(); sollyaPrintf("Consider an absolute error.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ABSOLUTE"
#endif
#endif
                          }
                      | DECIMALTOKEN
                          {
#ifdef HELP_DECIMAL_TEXT
			    outputMode(); sollyaPrintf(HELP_DECIMAL_TEXT);
#else
			    outputMode(); sollyaPrintf("Display mode is decimal.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DECIMAL"
#endif
#endif
                          }
                      | RELATIVETOKEN
                          {
#ifdef HELP_RELATIVE_TEXT
			    outputMode(); sollyaPrintf(HELP_RELATIVE_TEXT);
#else
			    outputMode(); sollyaPrintf("Consider a relative error.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RELATIVE"
#endif
#endif
                          }
                      | FIXEDTOKEN
                          {
#ifdef HELP_FIXED_TEXT
			    outputMode(); sollyaPrintf(HELP_FIXED_TEXT);
#else
			    outputMode(); sollyaPrintf("Consider fixed-point numbers.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FIXED"
#endif
#endif
                          }
                      | FLOATINGTOKEN
                          {
#ifdef HELP_FLOATING_TEXT
			    outputMode(); sollyaPrintf(HELP_FLOATING_TEXT);
#else
			    outputMode(); sollyaPrintf("Consider floating-point numbers.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FLOATING"
#endif
#endif
                          }
                      | ERRORTOKEN
                          {
#ifdef HELP_ERROR_TEXT
			    outputMode(); sollyaPrintf(HELP_ERROR_TEXT);
#else
			    outputMode(); sollyaPrintf("Type error meta-value.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ERROR"
#endif
#endif
                          }
                      | QUITTOKEN
                          {
#ifdef HELP_QUIT_TEXT
			    outputMode(); sollyaPrintf(HELP_QUIT_TEXT);
#else
			    outputMode(); sollyaPrintf("Exit from the tool.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for QUIT"
#endif
#endif
                          }
                      | FALSEQUITTOKEN
                          {
#ifdef HELP_QUIT_TEXT
			    outputMode(); sollyaPrintf(HELP_QUIT_TEXT);
#else
			    outputMode(); sollyaPrintf("Exit from the tool - help is called inside a read macro.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for QUIT"
#endif
#endif
                          }
                      | RESTARTTOKEN
                          {
#ifdef HELP_RESTART_TEXT
			    outputMode(); sollyaPrintf(HELP_RESTART_TEXT);
#else
			    outputMode(); sollyaPrintf("Restart the tool.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RESTART"
#endif
#endif
                          }
                      | LIBRARYTOKEN
                          {
#ifdef HELP_LIBRARY_TEXT
			    outputMode(); sollyaPrintf(HELP_LIBRARY_TEXT);
#else
			    outputMode(); sollyaPrintf("Library binding dereferencer.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LIBRARY"
#endif
#endif
                          }
                      | LIBRARYCONSTANTTOKEN
                          {
#ifdef HELP_LIBRARYCONSTANT_TEXT
			    outputMode(); sollyaPrintf(HELP_LIBRARYCONSTANT_TEXT);
#else
			    outputMode(); sollyaPrintf("Library constant binding dereferencer.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LIBRARYCONSTANT"
#endif
#endif
                          }
                      | DIFFTOKEN
                          {
#ifdef HELP_DIFF_TEXT
			    outputMode(); sollyaPrintf(HELP_DIFF_TEXT);
#else
			    outputMode(); sollyaPrintf("Differentiation: diff(func).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DIFF"
#endif
#endif
                          }
                      | BASHEVALUATETOKEN
                          {
#ifdef HELP_BASHEVALUATE_TEXT
			    outputMode(); sollyaPrintf(HELP_BASHEVALUATE_TEXT);
#else
			    outputMode(); sollyaPrintf("Executes a string as a bash command and returns the output as a string.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for BASHEVALUATE"
#endif
#endif
                          }
                      | SIMPLIFYTOKEN
                          {
#ifdef HELP_SIMPLIFY_TEXT
			    outputMode(); sollyaPrintf(HELP_SIMPLIFY_TEXT);
#else
			    outputMode(); sollyaPrintf("Simplify: simplify(func).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SIMPLIFY"
#endif
#endif
                          }
                      | REMEZTOKEN
                          {
#ifdef HELP_REMEZ_TEXT
			    outputMode(); sollyaPrintf(HELP_REMEZ_TEXT);
#else
			    outputMode(); sollyaPrintf("Remez: remez(func,degree|monoms,range[,weight[,quality]]).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for REMEZ"
#endif
#endif
                          }
                      | MINTOKEN
                          {
#ifdef HELP_MIN_TEXT
			    outputMode(); sollyaPrintf(HELP_MIN_TEXT);
#else
			    outputMode(); sollyaPrintf("min(val1,val2,...,valn): computes the minimum of the constant expressions vali.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for MIN"
#endif
#endif
                          }
                      | MAXTOKEN
                          {
#ifdef HELP_MAX_TEXT
			    outputMode(); sollyaPrintf(HELP_MAX_TEXT);
#else
			    outputMode(); sollyaPrintf("max(val1,val2,...,valn): computes the maximum of the constant expressions vali.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for MAX"
#endif
#endif
                          }
                      | FPMINIMAXTOKEN
                          {
#ifdef HELP_FPMINIMAX_TEXT
			    outputMode(); sollyaPrintf(HELP_FPMINIMAX_TEXT);
#else
			    outputMode(); sollyaPrintf("Fpminimax: fpminimax(func,degree|monoms,formats,range|pointslist[,absolute|relative[,fixed|floating[,constrainedPart[, minimaxpoly]]]]).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FPMINIMAX"
#endif
#endif
                          }
                      | HORNERTOKEN
                          {
#ifdef HELP_HORNER_TEXT
			    outputMode(); sollyaPrintf(HELP_HORNER_TEXT);
#else
			    outputMode(); sollyaPrintf("Horner: horner(func)\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for HORNER"
#endif
#endif
                          }
                      | EXPANDTOKEN
                          {
#ifdef HELP_EXPAND_TEXT
			    outputMode(); sollyaPrintf(HELP_EXPAND_TEXT);
#else
			    outputMode(); sollyaPrintf("Expand: expand(func).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for EXPAND"
#endif
#endif
                          }
                      | SIMPLIFYSAFETOKEN
                          {
#ifdef HELP_SIMPLIFYSAFE_TEXT
			    outputMode(); sollyaPrintf(HELP_SIMPLIFYSAFE_TEXT);
#else
			    outputMode(); sollyaPrintf("Safe simplification: simplifysafe(func).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SIMPLIFYSAFE"
#endif
#endif
                          }
                      | TAYLORTOKEN
                          {
#ifdef HELP_TAYLOR_TEXT
			    outputMode(); sollyaPrintf(HELP_TAYLOR_TEXT);
#else
			    outputMode(); sollyaPrintf("Taylor expansion: taylor(func,degree,point).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for TAYLOR"
#endif
#endif
                          }
                      | TAYLORFORMTOKEN
                          {
#ifdef HELP_TAYLORFORM_TEXT
			    outputMode(); sollyaPrintf(HELP_TAYLORFORM_TEXT);
#else
			    outputMode(); sollyaPrintf("Taylor form computation.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for TAYLORFORM"
#endif
#endif
                          }
                      | AUTODIFFTOKEN
                          {
#ifdef HELP_AUTODIFF_TEXT
			    outputMode(); sollyaPrintf(HELP_AUTODIFF_TEXT);
#else
			    outputMode(); sollyaPrintf("Automatic differentiation.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for AUTODIFF"
#endif
#endif
                          }
                      | DEGREETOKEN
                          {
#ifdef HELP_DEGREE_TEXT
			    outputMode(); sollyaPrintf(HELP_DEGREE_TEXT);
#else
			    outputMode(); sollyaPrintf("Degree of a polynomial: degree(func).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DEGREE"
#endif
#endif
                          }
                      | NUMERATORTOKEN
                          {
#ifdef HELP_NUMERATOR_TEXT
			    outputMode(); sollyaPrintf(HELP_NUMERATOR_TEXT);
#else
			    outputMode(); sollyaPrintf("Numerator of an expression: numerator(func).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for NUMERATOR"
#endif
#endif
                          }
                      | DENOMINATORTOKEN
                          {
#ifdef HELP_DENOMINATOR_TEXT
			    outputMode(); sollyaPrintf(HELP_DENOMINATOR_TEXT);
#else
			    outputMode(); sollyaPrintf("Denominator of an expression: denominator(func).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DENOMINATOR"
#endif
#endif
                          }
                      | SUBSTITUTETOKEN
                          {
#ifdef HELP_SUBSTITUTE_TEXT
			    outputMode(); sollyaPrintf(HELP_SUBSTITUTE_TEXT);
#else
			    outputMode(); sollyaPrintf("Substitute func2 for free variable in func: substitute(func,func2).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SUBSTITUTE"
#endif
#endif
                          }
                      | COEFFTOKEN
                          {
#ifdef HELP_COEFF_TEXT
			    outputMode(); sollyaPrintf(HELP_COEFF_TEXT);
#else
			    outputMode(); sollyaPrintf("i-th coefficient of a polynomial: coeff(func,degree).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for COEFF"
#endif
#endif
                          }
                      | SUBPOLYTOKEN
                          {
#ifdef HELP_SUBPOLY_TEXT
			    outputMode(); sollyaPrintf(HELP_SUBPOLY_TEXT);
#else
			    outputMode(); sollyaPrintf("Subpolynomial consisting in monomials: subpoly(func,list of degrees).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SUBPOLY"
#endif
#endif
                          }
                      | ROUNDCOEFFICIENTSTOKEN
                          {
#ifdef HELP_ROUNDCOEFFICIENTS_TEXT
			    outputMode(); sollyaPrintf(HELP_ROUNDCOEFFICIENTS_TEXT);
#else
			    outputMode(); sollyaPrintf("Round coefficients of a polynomial to format: roundcoefficients(func,list of formats).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ROUNDCOEFFICIENTS"
#endif
#endif
                          }
                      | RATIONALAPPROXTOKEN
                          {
#ifdef HELP_RATIONALAPPROX_TEXT
			    outputMode(); sollyaPrintf(HELP_RATIONALAPPROX_TEXT);
#else
			    outputMode(); sollyaPrintf("Rational approximation: rationalapprox(constant).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RATIONALAPPROX"
#endif
#endif
                          }
                      | ACCURATEINFNORMTOKEN
                          {
#ifdef HELP_ACCURATEINFNORM_TEXT
			    outputMode(); sollyaPrintf(HELP_ACCURATEINFNORM_TEXT);
#else
			    outputMode(); sollyaPrintf("Faithful rounded infinity norm: accurateinfnorm(func,bits,range,domains to exclude).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ACCURATEINFNORM"
#endif
#endif
                          }
                      | ROUNDTOFORMATTOKEN
                          {
#ifdef HELP_ROUND_TEXT
			    outputMode(); sollyaPrintf(HELP_ROUND_TEXT);
#else
			    outputMode(); sollyaPrintf("Round to a given format: round(constant,precision,rounding mode).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ROUND"
#endif
#endif
                          }
                      | EVALUATETOKEN
                          {
#ifdef HELP_EVALUATE_TEXT
			    outputMode(); sollyaPrintf(HELP_EVALUATE_TEXT);
#else
			    outputMode(); sollyaPrintf("Evaluate a function in a point or interval: round(func,constant|range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for EVALUATE"
#endif
#endif
                          }
                      | LENGTHTOKEN
                          {
#ifdef HELP_LENGTH_TEXT
			    outputMode(); sollyaPrintf(HELP_LENGTH_TEXT);
#else
			    outputMode(); sollyaPrintf("Length of a list: length(list).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LENGTH"
#endif
#endif
                          }
                      | PARSETOKEN
                          {
#ifdef HELP_PARSE_TEXT
			    outputMode(); sollyaPrintf(HELP_PARSE_TEXT);
#else
			    outputMode(); sollyaPrintf("Parse a string to function: parse(string).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PARSE"
#endif
#endif
                          }
                      | PRINTTOKEN
                          {
#ifdef HELP_PRINT_TEXT
			    outputMode(); sollyaPrintf(HELP_PRINT_TEXT);
#else
			    outputMode(); sollyaPrintf("Print something: print(thing1, thing2, ...).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PRINT"
#endif
#endif
                          }
                      | PRINTXMLTOKEN
                          {
#ifdef HELP_PRINTXML_TEXT
			    outputMode(); sollyaPrintf(HELP_PRINTXML_TEXT);
#else
			    outputMode(); sollyaPrintf("Print a function in XML: printxml(func).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PRINTXML"
#endif
#endif
                          }
                      | READXMLTOKEN
                          {
#ifdef HELP_READXML_TEXT
			    outputMode(); sollyaPrintf(HELP_READXML_TEXT);
#else
			    outputMode(); sollyaPrintf("Reads a function in XML: readxml(filename).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for READXML"
#endif
#endif
                          }
                      | PLOTTOKEN
                          {
#ifdef HELP_PLOT_TEXT
			    outputMode(); sollyaPrintf(HELP_PLOT_TEXT);
#else
			    outputMode(); sollyaPrintf("Plot (a) function(s) in a range: plot(func,func2,...,range).\n");
			    outputMode(); sollyaPrintf("There are further options.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PLOT"
#endif
#endif
                          }
                      | PRINTHEXATOKEN
                          {
#ifdef HELP_PRINTHEXA_TEXT
			    outputMode(); sollyaPrintf(HELP_PRINTHEXA_TEXT);
#else
			    outputMode(); sollyaPrintf("Print a constant in hexadecimal: printhexa(constant).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PRINTHEXA"
#endif
#endif
                          }
                      | PRINTFLOATTOKEN
                          {
#ifdef HELP_PRINTFLOAT_TEXT
			    outputMode(); sollyaPrintf(HELP_PRINTFLOAT_TEXT);
#else
			    outputMode(); sollyaPrintf("Print a constant in hexadecimal simple precision: printfloat(constant).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PRINTFLOAT"
#endif
#endif
                          }
                      | PRINTBINARYTOKEN
                          {
#ifdef HELP_PRINTBINARY_TEXT
			    outputMode(); sollyaPrintf(HELP_PRINTBINARY_TEXT);
#else
			    outputMode(); sollyaPrintf("Print a constant in binary: printbinary(constant).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PRINTBINARY"
#endif
#endif
                          }
                      | PRINTEXPANSIONTOKEN
                          {
#ifdef HELP_PRINTEXPANSION_TEXT
			    outputMode(); sollyaPrintf(HELP_PRINTEXPANSION_TEXT);
#else
			    outputMode(); sollyaPrintf("Print a polynomial as an expansion of double precision numbers: printexpansion(func).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PRINTEXPANSION"
#endif
#endif
                          }
                      | BASHEXECUTETOKEN
                          {
#ifdef HELP_BASHEXECUTE_TEXT
			    outputMode(); sollyaPrintf(HELP_BASHEXECUTE_TEXT);
#else
			    outputMode(); sollyaPrintf("Execute a command in a shell: bashexecute(string).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for BASHEXECUTE"
#endif
#endif
                          }
                      | EXTERNALPLOTTOKEN
                          {
#ifdef HELP_EXTERNALPLOT_TEXT
			    outputMode(); sollyaPrintf(HELP_EXTERNALPLOT_TEXT);
#else
			    outputMode(); sollyaPrintf("Here should be some help text.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for EXTERNALPLOT"
#endif
#endif
                          }
                      | WRITETOKEN
                          {
#ifdef HELP_WRITE_TEXT
			    outputMode(); sollyaPrintf(HELP_WRITE_TEXT);
#else
			    outputMode(); sollyaPrintf("Write something without adding spaces and newlines: write(thing1, thing2, ...).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for WRITE"
#endif
#endif
                          }
                      | ASCIIPLOTTOKEN
                          {
#ifdef HELP_ASCIIPLOT_TEXT
			    outputMode(); sollyaPrintf(HELP_ASCIIPLOT_TEXT);
#else
			    outputMode(); sollyaPrintf("Plot a function in a range using an ASCII terminal: asciiplot(func,range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ASCIIPLOT"
#endif
#endif
                          }
                      | RENAMETOKEN
                          {
#ifdef HELP_RENAME_TEXT
			    outputMode(); sollyaPrintf(HELP_RENAME_TEXT);
#else
			    outputMode(); sollyaPrintf("Rename free variable string1 to string2: rename(string1, string2).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RENAME"
#endif
#endif
                          }
                      | INFNORMTOKEN
                          {
#ifdef HELP_INFNORM_TEXT
			    outputMode(); sollyaPrintf(HELP_INFNORM_TEXT);
#else
			    outputMode(); sollyaPrintf("Certified infinity norm: infnorm(func,range[,prooffile[,list of funcs]]).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for INFNORM"
#endif
#endif
                          }
                      | SUPNORMTOKEN
                          {
#ifdef HELP_SUPNORM_TEXT
			    outputMode(); sollyaPrintf(HELP_SUPNORM_TEXT);
#else
			    outputMode(); sollyaPrintf("Validated supremum norm: supnorm(poly,func,range,mode,accuracy).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SUPNORM"
#endif
#endif
                          }
                      | FINDZEROSTOKEN
                          {
#ifdef HELP_FINDZEROS_TEXT
			    outputMode(); sollyaPrintf(HELP_FINDZEROS_TEXT);
#else
			    outputMode(); sollyaPrintf("Certified bounding of zeros: findzeros(func,range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FINDZEROS"
#endif
#endif
                          }
                      | FPFINDZEROSTOKEN
                          {
#ifdef HELP_FPFINDZEROS_TEXT
			    outputMode(); sollyaPrintf(HELP_FPFINDZEROS_TEXT);
#else
			    outputMode(); sollyaPrintf("Approximate zeros of a function: fpfindzeros(func,range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FPFINDZEROS"
#endif
#endif
                          }
                      | DIRTYINFNORMTOKEN
                          {
#ifdef HELP_DIRTYINFNORM_TEXT
			    outputMode(); sollyaPrintf(HELP_DIRTYINFNORM_TEXT);
#else
			    outputMode(); sollyaPrintf("Floating-point infinity norm: dirtyinfnorm(func,range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DIRTYINFNORM"
#endif
#endif
                          }
                      | NUMBERROOTSTOKEN
                          {
#ifdef HELP_NUMBERROOTS_TEXT
			    outputMode(); sollyaPrintf(HELP_NUMBERROOTS_TEXT);
#else
			    outputMode(); sollyaPrintf("Computes the number of real roots of a polynomial on a domain.\n");
#endif
                          }
                      | INTEGRALTOKEN
                          {
#ifdef HELP_INTEGRAL_TEXT
			    outputMode(); sollyaPrintf(HELP_INTEGRAL_TEXT);
#else
			    outputMode(); sollyaPrintf("Certified integral: integral(func,range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for INTEGRAL"
#endif
#endif
                          }
                      | DIRTYINTEGRALTOKEN
                          {
#ifdef HELP_DIRTYINTEGRAL_TEXT
			    outputMode(); sollyaPrintf(HELP_DIRTYINTEGRAL_TEXT);
#else
			    outputMode(); sollyaPrintf("Floating-point integral: dirtyintegral(func,range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DIRTYINTEGRAL"
#endif
#endif
                          }
                      | WORSTCASETOKEN
                          {
#ifdef HELP_WORSTCASE_TEXT
			    outputMode(); sollyaPrintf(HELP_WORSTCASE_TEXT);
#else
			    outputMode(); sollyaPrintf("Print all worst-cases under a certain bound: worstcase(func,constant,range,constant,constant[,file]).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for WORSTCASE"
#endif
#endif
                          }
                      | IMPLEMENTPOLYTOKEN
                          {
#ifdef HELP_IMPLEMENTPOLY_TEXT
			    outputMode(); sollyaPrintf(HELP_IMPLEMENTPOLY_TEXT);
#else
			    outputMode(); sollyaPrintf("Implement a polynomial in C: implementpoly(func,range,constant,format,string,string2[,honorcoeffprec[,string3]]).\n");
			    outputMode(); sollyaPrintf("Implements func in range with error constant with entering format named in function\nstring writing to file string2 honoring the precision of the coefficients or not with a proof in file string3.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for IMPLEMENTPOLY"
#endif
#endif
			  }
                      | IMPLEMENTCONSTTOKEN
                          {
#ifdef HELP_IMPLEMENTCONSTANT_TEXT
			    outputMode(); sollyaPrintf(HELP_IMPLEMENTCONSTANT_TEXT);
#else
			    outputMode(); sollyaPrintf("Implement a constant expression in arbitrary precision with MPFR: implementconstant(constant)\n");
			    outputMode(); sollyaPrintf("Generates code able to evaluate the given constant at any precision, with a guaranteed error.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for IMPLEMENTCONST"
#endif
#endif
                          }
                      | CHECKINFNORMTOKEN
                          {
#ifdef HELP_CHECKINFNORM_TEXT
			    outputMode(); sollyaPrintf(HELP_CHECKINFNORM_TEXT);
#else
			    outputMode(); sollyaPrintf("Checks whether an infinity norm is bounded: checkinfnorm(func,range,constant).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for CHECKINFNORM"
#endif
#endif
                          }
                      | ZERODENOMINATORSTOKEN
                          {
#ifdef HELP_ZERODENOMINATORS_TEXT
			    outputMode(); sollyaPrintf(HELP_ZERODENOMINATORS_TEXT);
#else
			    outputMode(); sollyaPrintf("Searches floating-point approximations to zeros of denominators: zerodenominators(func,range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ZERODENOMINATORS"
#endif
#endif
                          }
                      | ISEVALUABLETOKEN
                          {
#ifdef HELP_ISEVALUABLE_TEXT
			    outputMode(); sollyaPrintf(HELP_ISEVALUABLE_TEXT);
#else
			    outputMode(); sollyaPrintf("Tests if func is evaluable on range: isevaluable(func,range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ISEVALUABLE"
#endif
#endif
                          }
                      | SEARCHGALTOKEN
                          {
#ifdef HELP_SEARCHGAL_TEXT
			    outputMode(); sollyaPrintf(HELP_SEARCHGAL_TEXT);
#else
			    outputMode(); sollyaPrintf("Searches Gal values for func (or list of func): searchgal(func|list of func, constant, integer, integer, format|list of formats, constant|list of constants).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SEARCHGAL"
#endif
#endif
                          }
                      | GUESSDEGREETOKEN
                          {
#ifdef HELP_GUESSDEGREE_TEXT
			    outputMode(); sollyaPrintf(HELP_GUESSDEGREE_TEXT);
#else
			    outputMode(); sollyaPrintf("Guesses the degree needed for approximating func: guessdegree(func,range,constant[,weight]).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for GUESSDEGREE"
#endif
#endif
                          }
                      | DIRTYFINDZEROSTOKEN
                          {
#ifdef HELP_DIRTYFINDZEROS_TEXT
			    outputMode(); sollyaPrintf(HELP_DIRTYFINDZEROS_TEXT);
#else
			    outputMode(); sollyaPrintf("Finds zeros of a function dirtily: dirtyfindzeros(func,range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for DIRTYFINDZEROS"
#endif
#endif
                          }
                      | IFTOKEN
                          {
			    outputMode(); sollyaPrintf("If construct: if condition then command or if condition then command else command.\n");
                          }
                      | THENTOKEN
                          {
			    outputMode(); sollyaPrintf("If construct: if condition then command or if condition then command else command.\n");
                          }
                      | ELSETOKEN
                          {
			    outputMode(); sollyaPrintf("If construct: if condition then command else command\n");
                          }
                      | FORTOKEN
                          {
			    outputMode(); sollyaPrintf("For construct: for i from const to const2 [by const3] do command\nor for i in list do command.\n");
                          }
                      | INTOKEN
                          {
#ifdef HELP_IN_TEXT
			    outputMode(); sollyaPrintf(HELP_IN_TEXT);
#else
			    outputMode(); sollyaPrintf("In construct: for in construct and containment operator.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for IN"
#endif
#endif
                          }
                      | FROMTOKEN
                          {
			    outputMode(); sollyaPrintf("For construct: for i from const to const2 [by const3] do command.\n");
                          }
                      | TOTOKEN
                          {
			    outputMode(); sollyaPrintf("For construct: for i from const to const2 [by const3] do command.\n");
                          }
                      | BYTOKEN
                          {
			    outputMode(); sollyaPrintf("For construct: for i from const to const2 by const3 do command.\n");
                          }
                      | DOTOKEN
                          {
			    outputMode(); sollyaPrintf("For construct: for i from const to const2 [by const3] do command.\n");
			    outputMode(); sollyaPrintf("While construct: while condition do command.\n");
                          }
                      | beginsymbol
                          {
			    outputMode(); sollyaPrintf("Begin-end construct: begin command; command; ... end.\n");
                          }
                      | endsymbol
                          {
			    outputMode(); sollyaPrintf("Begin-end construct: begin command; command; ... end.\n");
                          }
                      | WHILETOKEN
                          {
			    outputMode(); sollyaPrintf("While construct: while condition do command.\n");
                          }
                      | INFTOKEN
                          {
#ifdef HELP_INF_TEXT
			    outputMode(); sollyaPrintf(HELP_INF_TEXT);
#else
			    outputMode(); sollyaPrintf("Dereferencing the infimum of a range: inf(range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for INF"
#endif
#endif
                          }
                      | MIDTOKEN
                          {
#ifdef HELP_MID_TEXT
			    outputMode(); sollyaPrintf(HELP_MID_TEXT);
#else
			    outputMode(); sollyaPrintf("Dereferencing the midpoint of a range: mid(range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for MID"
#endif
#endif
                          }
                      | SUPTOKEN
                          {
#ifdef HELP_SUP_TEXT
			    outputMode(); sollyaPrintf(HELP_SUP_TEXT);
#else
			    outputMode(); sollyaPrintf("Dereferencing the supremum of a range: sup(range).\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for SUP"
#endif
#endif
                          }
                      | EXPONENTTOKEN
                          {
#ifdef HELP_EXPONENT_TEXT
			    outputMode(); sollyaPrintf(HELP_EXPONENT_TEXT);
#else
			    outputMode(); sollyaPrintf("exponent(constant): returns an integer such that constant scaled by the power of 2\nof this integer is an odd or zero integer.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for EXPONENT"
#endif
#endif
                          }
                      | MANTISSATOKEN
                          {
#ifdef HELP_MANTISSA_TEXT
			    outputMode(); sollyaPrintf(HELP_MANTISSA_TEXT);
#else
			    outputMode(); sollyaPrintf("mantissa(constant): returns an odd or zero integer equal to constant scaled by an integer power of 2.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for MANTISSA"
#endif
#endif
                          }
                      | PRECISIONTOKEN
                          {
#ifdef HELP_PRECISION_TEXT
			    outputMode(); sollyaPrintf(HELP_PRECISION_TEXT);
#else
			    outputMode(); sollyaPrintf("precision(constant): returns the least number of bits constant can be written on.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PRECISION"
#endif
#endif
                          }
                      | EXECUTETOKEN
                          {
#ifdef HELP_EXECUTE_TEXT
			    outputMode(); sollyaPrintf(HELP_EXECUTE_TEXT);
#else
			    outputMode(); sollyaPrintf("execute(string): executes an %s script contained in a file named string.\n",PACKAGE_NAME);
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for EXECUTE"
#endif
#endif
                          }
                      | ISBOUNDTOKEN
                          {
#ifdef HELP_ISBOUND_TEXT
			    outputMode(); sollyaPrintf(HELP_ISBOUND_TEXT);
#else
			    outputMode(); sollyaPrintf("isbound(identifier): returns a boolean indicating if identifier is bound.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for ISBOUND"
#endif
#endif
                          }
                      | VERSIONTOKEN
                          {
			    outputMode(); sollyaPrintf("Prints the version of the software.\n");
                          }
                      | EXTERNALPROCTOKEN                          {
#ifdef HELP_EXTERNALPROC_TEXT
			    outputMode(); sollyaPrintf(HELP_EXTERNALPROC_TEXT);
#else
			    outputMode(); sollyaPrintf("externalplot(identifier, file, argumentypes -> resulttype): binds identifier to an external procedure with signature argumenttypes -> resulttype in file.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for EXTERNALPROC"
#endif
#endif
                          }
                      | VOIDTOKEN                          {
#ifdef HELP_VOID_TEXT
			    outputMode(); sollyaPrintf(HELP_VOID_TEXT);
#else
			    outputMode(); sollyaPrintf("Represents the void type for externalproc.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for VOID"
#endif
#endif
                          }
                      | CONSTANTTYPETOKEN                          {
#ifdef HELP_CONSTANT_TEXT
			    outputMode(); sollyaPrintf(HELP_CONSTANT_TEXT);
#else
			    outputMode(); sollyaPrintf("Represents the constant type for externalproc.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for CONSTANT"
#endif
#endif
                          }
                      | FUNCTIONTOKEN                          {
#ifdef HELP_FUNCTION_TEXT
			    outputMode(); sollyaPrintf(HELP_FUNCTION_TEXT);
#else
			    outputMode(); sollyaPrintf("Represents the function type for externalproc.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for FUNCTION"
#endif
#endif
                          }
                      | RANGETOKEN                          {
#ifdef HELP_RANGE_TEXT
			    outputMode(); sollyaPrintf(HELP_RANGE_TEXT);
#else
			    outputMode(); sollyaPrintf("Represents the range type for externalproc.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RANGE"
#endif
#endif
                          }
                      | INTEGERTOKEN                          {
#ifdef HELP_INTEGER_TEXT
			    outputMode(); sollyaPrintf(HELP_INTEGER_TEXT);
#else
			    outputMode(); sollyaPrintf("Represents the integer type for externalproc.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for INTEGER"
#endif
#endif
                          }
                      | STRINGTYPETOKEN                          {
#ifdef HELP_STRING_TEXT
			    outputMode(); sollyaPrintf(HELP_STRING_TEXT);
#else
			    outputMode(); sollyaPrintf("Represents the string type for externalproc.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for STRING"
#endif
#endif
                          }
                      | BOOLEANTOKEN                          {
#ifdef HELP_BOOLEAN_TEXT
			    outputMode(); sollyaPrintf(HELP_BOOLEAN_TEXT);
#else
			    outputMode(); sollyaPrintf("Represents the boolean type for externalproc.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for BOOLEAN"
#endif
#endif
                          }
                      | LISTTOKEN                          {
#ifdef HELP_LISTOF_TEXT
			    outputMode(); sollyaPrintf(HELP_LISTOF_TEXT);
#else
			    outputMode(); sollyaPrintf("Represents the list type for externalproc.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LISTOF"
#endif
#endif
                          }
                      | OFTOKEN                          {
#ifdef HELP_LISTOF_TEXT
			    outputMode(); sollyaPrintf(HELP_LISTOF_TEXT);
#else
			    outputMode(); sollyaPrintf("Used in list of type for externalproc.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for LISTOF"
#endif
#endif
                          }
                      | VARTOKEN                          {
#ifdef HELP_VAR_TEXT
			    outputMode(); sollyaPrintf(HELP_VAR_TEXT);
#else
			    outputMode(); sollyaPrintf("Declares a local variable.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for VAR"
#endif
#endif
                          }
                      | NOPTOKEN                          {
#ifdef HELP_NOP_TEXT
			    outputMode(); sollyaPrintf(HELP_NOP_TEXT);
#else
			    outputMode(); sollyaPrintf("Does nothing.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for NOP"
#endif
#endif
                          }
                      | PROCTOKEN                          {
#ifdef HELP_PROC_TEXT
			    outputMode(); sollyaPrintf(HELP_PROC_TEXT);
#else
			    outputMode(); sollyaPrintf("Defines a nameless procedure.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PROC"
#endif
#endif
                          }
                      | PROCEDURETOKEN                     {
#ifdef HELP_PROCEDURE_TEXT
			    outputMode(); sollyaPrintf(HELP_PROCEDURE_TEXT);
#else
			    outputMode(); sollyaPrintf("Defines a named procedure.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for PROCEDURE"
#endif
#endif
                          }
                      | RETURNTOKEN                     {
#ifdef HELP_RETURN_TEXT
			    outputMode(); sollyaPrintf(HELP_RETURN_TEXT);
#else
			    outputMode(); sollyaPrintf("Returns an expression in a procedure.\n");
#if defined(WARN_IF_NO_HELP_TEXT) && WARN_IF_NO_HELP_TEXT
#warning "No help text for RETURN"
#endif
#endif
                          }
                      | HELPTOKEN
                          {
			    outputMode(); sollyaPrintf("Type \"help <keyword>;\" for help on the keyword <keyword>.\nFor example type \"help implementpoly;\" for help on the command \"implementpoly\".\n\n");
			    sollyaPrintf("Possible keywords in %s are:\n",PACKAGE_NAME);
			    sollyaPrintf("- !\n");
			    sollyaPrintf("- !=\n");
			    sollyaPrintf("- &&\n");
			    sollyaPrintf("- (\n");
			    sollyaPrintf("- )\n");
			    sollyaPrintf("- *\n");
			    sollyaPrintf("- *<\n");
			    sollyaPrintf("- +\n");
			    sollyaPrintf("- ,\n");
			    sollyaPrintf("- -\n");
			    sollyaPrintf("- .\n");
			    sollyaPrintf("- ...\n");
			    sollyaPrintf("- .:\n");
			    sollyaPrintf("- /\n");
			    sollyaPrintf("- :.\n");
			    sollyaPrintf("- ::\n");
			    sollyaPrintf("- :=\n");
			    sollyaPrintf("- ;\n");
			    sollyaPrintf("- <\n");
			    sollyaPrintf("- =\n");
			    sollyaPrintf("- ==\n");
			    sollyaPrintf("- >\n");
			    sollyaPrintf("- >*\n");
			    sollyaPrintf("- >.\n");
			    sollyaPrintf("- >_\n");
			    sollyaPrintf("- ?\n");
			    sollyaPrintf("- @\n");
			    sollyaPrintf("- D\n");
			    sollyaPrintf("- DD\n");
			    sollyaPrintf("- DE\n");
			    sollyaPrintf("- Pi\n");
			    sollyaPrintf("- RD\n");
			    sollyaPrintf("- RN\n");
			    sollyaPrintf("- RU\n");
			    sollyaPrintf("- RZ\n");
			    sollyaPrintf("- SG\n");
			    sollyaPrintf("- TD\n");
			    sollyaPrintf("- [\n");
			    sollyaPrintf("- ]\n");
			    sollyaPrintf("- ^\n");
			    sollyaPrintf("- abs\n");
			    sollyaPrintf("- absolute\n");
			    sollyaPrintf("- accurateinfnorm\n");
			    sollyaPrintf("- acos\n");
			    sollyaPrintf("- acosh\n");
			    sollyaPrintf("- asciiplot\n");
			    sollyaPrintf("- asin\n");
			    sollyaPrintf("- asinh\n");
			    sollyaPrintf("- atan\n");
			    sollyaPrintf("- atanh\n");
			    sollyaPrintf("- autodiff\n");
			    sollyaPrintf("- autosimplify\n");
			    sollyaPrintf("- bashexecute\n");
			    sollyaPrintf("- begin\n");
			    sollyaPrintf("- binary\n");
			    sollyaPrintf("- boolean\n");
			    sollyaPrintf("- by\n");
			    sollyaPrintf("- canonical\n");
			    sollyaPrintf("- ceil\n");
			    sollyaPrintf("- checkinfnorm\n");
			    sollyaPrintf("- coeff\n");
			    sollyaPrintf("- constant\n");
			    sollyaPrintf("- cos\n");
			    sollyaPrintf("- cosh\n");
			    sollyaPrintf("- decimal\n");
			    sollyaPrintf("- default\n");
			    sollyaPrintf("- degree\n");
			    sollyaPrintf("- denominator\n");
			    sollyaPrintf("- diam\n");
			    sollyaPrintf("- dieonerrormode\n");
			    sollyaPrintf("- diff\n");
			    sollyaPrintf("- dirtyfindzeros\n");
			    sollyaPrintf("- dirtyinfnorm\n");
			    sollyaPrintf("- dirtyintegral\n");
			    sollyaPrintf("- display\n");
			    sollyaPrintf("- do\n");
			    sollyaPrintf("- double\n");
			    sollyaPrintf("- doubledouble\n");
			    sollyaPrintf("- doubleextended\n");
			    sollyaPrintf("- dyadic\n");
			    sollyaPrintf("- else\n");
			    sollyaPrintf("- end\n");
			    sollyaPrintf("- erf\n");
			    sollyaPrintf("- erfc\n");
			    sollyaPrintf("- error\n");
			    sollyaPrintf("- evaluate\n");
			    sollyaPrintf("- execute\n");
			    sollyaPrintf("- exp\n");
			    sollyaPrintf("- expand\n");
			    sollyaPrintf("- expm1\n");
			    sollyaPrintf("- exponent\n");
			    sollyaPrintf("- externalplot\n");
			    sollyaPrintf("- externalproc\n");
			    sollyaPrintf("- false\n");
			    sollyaPrintf("- file\n");
			    sollyaPrintf("- findzeros\n");
			    sollyaPrintf("- fixed\n");
			    sollyaPrintf("- floating\n");
			    sollyaPrintf("- floor\n");
			    sollyaPrintf("- for\n");
			    sollyaPrintf("- fpfindzeros\n");
			    sollyaPrintf("- fpminimax\n");
			    sollyaPrintf("- from\n");
			    sollyaPrintf("- fullparentheses\n");
			    sollyaPrintf("- function\n");
			    sollyaPrintf("- guessdegree\n");
			    sollyaPrintf("- head\n");
			    sollyaPrintf("- hexadecimal\n");
			    sollyaPrintf("- honorcoeffprec\n");
			    sollyaPrintf("- hopitalrecursions\n");
			    sollyaPrintf("- horner\n");
			    sollyaPrintf("- if\n");
			    sollyaPrintf("- implementpoly\n");
			    sollyaPrintf("- implementconstant\n");
			    sollyaPrintf("- in\n");
			    sollyaPrintf("- inf\n");
			    sollyaPrintf("- infnorm\n");
			    sollyaPrintf("- integer\n");
			    sollyaPrintf("- integral\n");
			    sollyaPrintf("- isbound\n");
			    sollyaPrintf("- isevaluable\n");
			    sollyaPrintf("- length\n");
			    sollyaPrintf("- library\n");
			    sollyaPrintf("- libraryconstant\n");
			    sollyaPrintf("- list\n");
			    sollyaPrintf("- log\n");
			    sollyaPrintf("- log10\n");
			    sollyaPrintf("- log1p\n");
			    sollyaPrintf("- log2\n");
			    sollyaPrintf("- mantissa\n");
			    sollyaPrintf("- max\n");
			    sollyaPrintf("- mid\n");
			    sollyaPrintf("- midpointmode\n");
			    sollyaPrintf("- min\n");
			    sollyaPrintf("- nearestint\n");
			    sollyaPrintf("- numberroots\n");
			    sollyaPrintf("- nop\n");
			    sollyaPrintf("- numerator\n");
			    sollyaPrintf("- of\n");
			    sollyaPrintf("- off\n");
			    sollyaPrintf("- on\n");
			    sollyaPrintf("- parse\n");
			    sollyaPrintf("- perturb\n");
			    sollyaPrintf("- pi\n");
			    sollyaPrintf("- plot\n");
			    sollyaPrintf("- points\n");
			    sollyaPrintf("- postscript\n");
			    sollyaPrintf("- postscriptfile\n");
			    sollyaPrintf("- powers\n");
			    sollyaPrintf("- prec\n");
			    sollyaPrintf("- precision\n");
			    sollyaPrintf("- print\n");
			    sollyaPrintf("- printbinary\n");
			    sollyaPrintf("- printdouble\n");
			    sollyaPrintf("- printexpansion\n");
			    sollyaPrintf("- printfloat\n");
			    sollyaPrintf("- printhexa\n");
			    sollyaPrintf("- printsingle\n");
			    sollyaPrintf("- printxml\n");
			    sollyaPrintf("- proc\n");
			    sollyaPrintf("- procedure\n");
			    sollyaPrintf("- quit\n");
			    sollyaPrintf("- range\n");
			    sollyaPrintf("- rationalapprox\n");
			    sollyaPrintf("- rationalmode\n");
			    sollyaPrintf("- readfile\n");
			    sollyaPrintf("- readxml\n");
			    sollyaPrintf("- relative\n");
			    sollyaPrintf("- remez\n");
			    sollyaPrintf("- rename\n");
			    sollyaPrintf("- restart\n");
			    sollyaPrintf("- return\n");
			    sollyaPrintf("- revert\n");
			    sollyaPrintf("- round\n");
			    sollyaPrintf("- roundcoefficients\n");
			    sollyaPrintf("- roundcorrectly\n");
			    sollyaPrintf("- roundingwarnings\n");
			    sollyaPrintf("- safesimplify\n");
			    sollyaPrintf("- searchgal\n");
			    sollyaPrintf("- simplify\n");
			    sollyaPrintf("- simplifysafe\n");
			    sollyaPrintf("- sin\n");
			    sollyaPrintf("- single\n");
			    sollyaPrintf("- sinh\n");
			    sollyaPrintf("- sort\n");
			    sollyaPrintf("- sqrt\n");
			    sollyaPrintf("- string\n");
			    sollyaPrintf("- subpoly\n");
			    sollyaPrintf("- substitute\n");
			    sollyaPrintf("- sup\n");
			    sollyaPrintf("- supnorm\n");
			    sollyaPrintf("- tail\n");
			    sollyaPrintf("- tan\n");
			    sollyaPrintf("- tanh\n");
			    sollyaPrintf("- taylor\n");
			    sollyaPrintf("- taylorform\n");
			    sollyaPrintf("- taylorrecursions\n");
			    sollyaPrintf("- then\n");
			    sollyaPrintf("- time\n");
			    sollyaPrintf("- timing\n");
			    sollyaPrintf("- to\n");
			    sollyaPrintf("- tripledouble\n");
			    sollyaPrintf("- true\n");
			    sollyaPrintf("- var\n");
			    sollyaPrintf("- verbosity\n");
			    sollyaPrintf("- version\n");
			    sollyaPrintf("- void\n");
			    sollyaPrintf("- while\n");
			    sollyaPrintf("- worstcase\n");
			    sollyaPrintf("- write\n");
			    sollyaPrintf("- zerodenominators\n");
			    sollyaPrintf("- {\n");
			    sollyaPrintf("- |\n");
			    sollyaPrintf("- ||\n");
			    sollyaPrintf("- }\n");
			    sollyaPrintf("- ~\n");
			    sollyaPrintf("\n");
                          }
;


