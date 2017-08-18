/*

Copyright 2007-2011 by

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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "expression.h"
#include "miniparser.h"
#include "general.h"
#include "chain.h"

#define YY_NO_UNPUT 1

%}

%option noyywrap
%option always-interactive
%option prefix="miniyy"
%option reentrant
%option bison-bridge
%option nounput

%x commentstate

CHAR		[a-zA-Z]
NUMBER		[0-9]
HEXNUMBER       (([0-9])|([ABCDEFabcdef]))
SPECIAL         [_$§]

ACONSTANT        ((({NUMBER}+|({NUMBER}*"."{NUMBER}+))(((((" ")*)(([eE]))([+-])?{NUMBER}+)?)))|("infty"|"nan"|"NAN"|"NaN"|"@Inf@"|"@NaN@"))
CONSTANT        ({ACONSTANT})|((("%")({NUMBER})({NUMBER}+)("%"))({ACONSTANT}))
DYADICCONSTANT  ({NUMBER}+)([bB])([+-]?)({NUMBER}+)
HEXCONSTANT     ("0x")({HEXNUMBER}{16}|{HEXNUMBER}{8})

HEXADECIMALCONSTANT ("0x"|"0X")(({HEXNUMBER}*"."{HEXNUMBER}+)|({HEXNUMBER}+((".")?)))(("p"|"P")(([+-])?)({NUMBER}*))

BINARYCONSTANT  (([0-1])+|(([0-1])*"."([0-1])+))"_2"

AMIDPOINTCONSTANT        (({NUMBER}+|({NUMBER}*"."{NUMBER}+))(("~"({NUMBER})+("/")({NUMBER})+("~")))(((((" ")*)(([eE]))([+-])?{NUMBER}+)?)))
MIDPOINTCONSTANT         (({AMIDPOINTCONSTANT})|(("-")((" ")*)({AMIDPOINTCONSTANT})))

IDENTIFIER        ({CHAR}|{SPECIAL})({CHAR}|{NUMBER}|{SPECIAL})*

DOT             "."

LPAR            "("
RPAR            ")"

LBRACKET        "["
RBRACKET        "]"

PI              ("pi")|("Pi")

PLUS            "+"
MINUS           "-"
APPROX          "~"
MUL             "*"
DIV             "/"
POW             "^"
SQRT            "sqrt"
EXP             "exp"
LOG             "log"
LOG2            "log2"
LOG10           "log10"
SIN             "sin"
COS             "cos"
TAN             "tan"
ASIN            "asin"
ACOS            "acos"
ATAN            "atan"
SINH            "sinh"
COSH            "cosh"
TANH            "tanh"
ASINH           "asinh"
ACOSH           "acosh"
ATANH           "atanh"
ABS             "abs"
ERF             "erf"
ERFC            "erfc"
LOG1P           "log1p"
EXPM1           "expm1"

EQUAL           "="
COMMA           ","
PREC            "prec"
POINTS          "points"
EXCLAMATION     "!"
DOUBLECOLON     "::"
COLON           ":"
DOTCOLON        ".:"
COLONDOT        ":."
ASSIGNEQUAL     ":="

SEMICOLON       ";"
QUIT            "quit"
PRINT           "print"
DIFF            "diff"
SIMPLIFY        "simplify"
CANONICAL       "canonical"
PLOT            "plot"
REMEZ           "remez"
FPMINIMAX       "fpminimax"
INFNORM         "infnorm"
SUPNORM         "supnorm"
DIAM            "diam"

DOUBLELONG       "double"
SINGLELONG       "single"
QUADLONG         "quad"
HALFPRECISIONLONG "halfprecision"
DOUBLEDOUBLELONG "doubledouble"
TRIPLEDOUBLELONG "tripledouble"
DOUBLEEXTENDEDLONG "doubleextended"


DOUBLESHORT      "D"
SINGLESHORT      "SG"
QUADSHORT        "QD"
HALFPRECISIONSHORT "HP"
DOUBLEDOUBLESHORT "DD"
TRIPLEDOUBLESHORT "TD"
DOUBLEEXTENDEDSHORT "DE"


DOUBLE          ({DOUBLELONG}|{DOUBLESHORT})
SINGLE          ({SINGLELONG}|{SINGLESHORT})
QUAD            ({QUADLONG}|{QUADSHORT})
HALFPRECISION   ({HALFPRECISIONLONG}|{HALFPRECISIONSHORT})
DOUBLEDOUBLE    ({DOUBLEDOUBLELONG}|{DOUBLEDOUBLESHORT})
TRIPLEDOUBLE    ({TRIPLEDOUBLELONG}|{TRIPLEDOUBLESHORT})
DOUBLEEXTENDED  ({DOUBLEEXTENDEDLONG}|{DOUBLEEXTENDEDSHORT})

HORNER          "horner"
DEGREE          "degree"
EXPAND          "expand"

SIMPLIFYSAFE1   "safesimplify"
SIMPLIFYSAFE2   "simplifysafe"

SIMPLIFYSAFE    ({SIMPLIFYSAFE1}|{SIMPLIFYSAFE2})

TAYLOR          "taylor"
TAYLORFORM      "taylorform"
AUTODIFF        "autodiff"

FINDZEROS       "findzeros"
FPFINDZEROS     "fpfindzeros"
DIRTYINFNORM    "dirtyinfnorm"
NUMBERROOTS     "numberroots"

EVALUATE        "evaluate"


NUMERATOR       "numerator"
DENOMINATOR     "denominator"

DYADIC          "dyadic"
DISPLAY         "display"
ON              "on"
OFF             "off"
POWERS          "powers"
BINARY          "binary"
HEXADECIMAL     "hexadecimal"

INTEGRAL        "integral"
DIRTYINTEGRAL   "dirtyintegral"

STRINGDELIMITER [\"]
OCTALCHAR       [01234567]
OCTAL           ({OCTALCHAR})(({OCTALCHAR})?)(({OCTALCHAR})?)
HEXACHAR        [0123456789ABCDEFabcdef]
HEXA            ({HEXACHAR})(({HEXACHAR})?)
STRING          ({STRINGDELIMITER})((("\\\\")|(("\\")[\"\'\?ntabfrv])|(("\\")({OCTAL}))|(("\\x")({HEXA}))|([^\"\\]))*)({STRINGDELIMITER})


VERBOSITY       "verbosity"

WORSTCASE       "worstcase"

LEFTANGLE       "<"
RIGHTANGLEUNDERSCORE ">_"
RIGHTANGLEDOT ">."
RIGHTANGLE      ">"
STARLEFTANGLE   "*<"
RIGHTANGLESTAR  ">*"

COMPAREEQUAL    "=="
EXCLAMATIONEQUAL "!="

AND             "&&"
OR              "||"

SUBSTITUTE      "substitute"

DOTS            "..."

IMPLEMENTPOLY   "implementpoly"
IMPLEMENTCONST  "implementconstant"

CHECKINFNORM    "checkinfnorm"

TAYLORRECURSIONS "taylorrecursions"

PRINTHEXA       ("printhexa"|"printdouble")
PRINTFLOAT      ("printfloat"|"printsingle")
PRINTBINARY     "printbinary"

ROUNDCOEFFICIENTS "roundcoefficients"


RESTART         "restart"


ZERODENOMINATORS "zerodenominators"
ISEVALUABLE     "isevaluable"
HONORCOEFFPREC  "honorcoeffprec"

ACCURATEINFNORM  "accurateinfnorm"


FILE            "file"
POSTSCRIPT      "postscript"
POSTSCRIPTFILE  "postscriptfile"

PRINTEXPANSION  "printexpansion"

BASHEXECUTE     "bashexecute"
BASHEVALUATE    "bashevaluate"
EXTERNALPLOT    "externalplot"
PERTURB         "perturb"

COEFF           "coeff"
SUBPOLY         "subpoly"

QUESTIONMARK    "?"

SEARCHGAL       "searchgal"


RATIONALAPPROX  "rationalapprox"

READ            "read"

COMMENTSTART    "/*"
COMMENTEND      "*/"

ONELINECOMMENT  ("//")([^\n])*"\n"

WRITE           "write"

ASCIIPLOT       "asciiplot"


ROUNDTOFORMAT   "round"
MINUSWORD       "RD"
PLUSWORD        "RU"
ZEROWORD        "RZ"
NEAREST         "RN"


GUESSDEGREE     "guessdegree"

PARSE           "parse"

AUTOSIMPLIFY    "autosimplify"

TIMING          "timing"
TIME            "time"
FULLPARENTHESES "fullparentheses"
MIDPOINTMODE    "midpointmode"
DIEONERRORMODE  "dieonerrormode"
SUPPRESSWARNINGS "roundingwarnings"
RATIONALMODE    "rationalmode"

LIBRARY         "library"

HOPITALRECURSIONS "hopitalrecursions"

HELP            "help"

DIRTYFINDZEROS  "dirtyfindzeros"

CEIL            "ceil"
FLOOR           "floor"
NEARESTINT      "nearestint"

HEAD            "head"
TAIL            "tail"

VERTBAR         "|"
AT              "@"

IF              "if"
THEN            "then"
ELSE            "else"
FOR             "for"
IN              "in"
FROM            "from"
TO              "to"
BY              "by"
DO              "do"
BEGIN           "begin" 
END             "end"
LBRACE          "{"
RBRACE          "}"

WHILEDEF        "while"

TRUE            "true"
FALSE           "false"
DEFAULT         "default"

MATCH           "match"
WITH            "with"

RENAME          "rename"

LENGTH          "length"

ABSOLUTE        "absolute"
RELATIVE        "relative"
DECIMAL         "decimal"
FIXED           "fixed"
FLOATING        "floating"

ERROR           "error"

PRINTXML        "printxml"

INF             "inf"
MID             "mid"
SUP             "sup"

MIN             "min"
MAX             "max"

READXML         "readxml"

MANTISSA        "mantissa"
EXPONENT        "exponent"
PRECISION       "precision"

REVERT          "revert"
SORT            "sort"

READFILE        "readfile"

ROUNDCORRECTLY  "roundcorrectly"

EXECUTE         "execute"

ISBOUND         "isbound"

EXTERNALPROC    "externalproc"
VOID            "void"
CONSTANTTYPE    "constant"
FUNCTION        "function"
RANGE           "range"
INTEGER         "integer"
STRINGTYPE      "string"
BOOLEAN         "boolean"
LIST            "list"
OF              "of"

VAR             "var"

PROC            "proc"
PROCEDURE       "procedure"
RETURN          "return"
NOP             "nop"


%%

%{

%}




{COMMENTSTART}  {     BEGIN(commentstate); }

<commentstate>{COMMENTEND} { BEGIN(INITIAL); }

<commentstate>. { // Eat up comments

                 }

<commentstate>[\n] { // Eat up newlines in comments

		}



{ONELINECOMMENT} {  // Eat up comments

                 }

<<EOF>>         {
                      yyterminate();
                }
{MIDPOINTCONSTANT}                          {
					      constBuffer2 = (char *) safeCalloc(yyleng+1,sizeof(char));
					      if (removeSpaces(constBuffer2,yytext)) {
						printMessage(2,"Information: removed spaces in scientific notation constant \"%s\", it will be considered as \"%s\"\n",yytext,constBuffer2);
					      }
					      yylval->value = constBuffer2;
                                              return MIDPOINTCONSTANTTOKEN; }
{CONSTANT}                                  {
					      constBuffer2 = (char *) safeCalloc(yyleng+1,sizeof(char));
					      if (removeSpaces(constBuffer2,yytext)) {
						printMessage(2,"Information: removed spaces in scientific notation constant \"%s\", it will be considered as \"%s\"\n",yytext,constBuffer2);
					      }
					      yylval->value = constBuffer2;
                                               return CONSTANTTOKEN; }
{DYADICCONSTANT} 			    {
                                              constBuffer = (char *) safeCalloc(yyleng+1,sizeof(char));
					      strncpy(constBuffer,yytext,yyleng);
					      yylval->value = constBuffer;
                                               return DYADICCONSTANTTOKEN; }
{HEXADECIMALCONSTANT}     		    { constBuffer = (char *) safeCalloc(yyleng+1,sizeof(char));
					      strncpy(constBuffer,yytext,yyleng);
					      yylval->value = constBuffer;
                                              return HEXADECIMALCONSTANTTOKEN; }
{HEXCONSTANT}     			    { constBuffer = (char *) safeCalloc(yyleng+1,sizeof(char));
					      strncpy(constBuffer,yytext,yyleng);
					      yylval->value = constBuffer;
                                               return HEXCONSTANTTOKEN; }
{BINARYCONSTANT}  			    { constBuffer = (char *) safeCalloc(yyleng-1,sizeof(char));
					      strncpy(constBuffer,yytext,yyleng-2);
					      yylval->value = constBuffer;
                                               return BINARYCONSTANTTOKEN; }

{PI}              			    {  return PITOKEN; }

{STRING}          			    {
					      constBuffer = (char *) safeCalloc(yyleng-1,sizeof(char));
					      constBuffer2 = (char *) safeCalloc(yyleng-1,sizeof(char));
					      strncpy(constBuffer2,yytext+1,yyleng-2);
					      demaskString(constBuffer,constBuffer2);
					      free(constBuffer2);
					      yylval->value = constBuffer;
                                               return STRINGTOKEN; }

{LPAR}            			    {  return LPARTOKEN; }
{RPAR}            			    {  return RPARTOKEN; }
{LBRACKET}        			    {  return LBRACKETTOKEN; }
{RBRACKET}        			    {  return RBRACKETTOKEN; }
{EXCLAMATIONEQUAL}			    {  return EXCLAMATIONEQUALTOKEN; }
{COMPAREEQUAL}                              {  return COMPAREEQUALTOKEN; }
{EQUAL}           			    {  return EQUALTOKEN; }
{ASSIGNEQUAL}          			    {  return ASSIGNEQUALTOKEN; }
{COMMA}           			    {  return COMMATOKEN; }
{EXCLAMATION}				    {  return EXCLAMATIONTOKEN; }
{SEMICOLON}       			    {  return SEMICOLONTOKEN; }
{LEFTANGLE}       			    {  return LEFTANGLETOKEN; }
{STARLEFTANGLE}   			    {  return STARLEFTANGLETOKEN; }
{RIGHTANGLEUNDERSCORE}			    {  return RIGHTANGLEUNDERSCORETOKEN; }
{RIGHTANGLEDOT} 			    {  return RIGHTANGLEDOTTOKEN; }
{RIGHTANGLESTAR}			    {  return RIGHTANGLESTARTOKEN; }
{RIGHTANGLE}      			    {  return RIGHTANGLETOKEN; }
{DOTS}            			    {  return DOTSTOKEN; }
{DOT}            			    {  return DOTTOKEN; }
{QUESTIONMARK}				    {  return QUESTIONMARKTOKEN; }
{AND}         				    {  return ANDTOKEN; }
{OR}         				    {  return ORTOKEN; }
{VERTBAR}				    {  return VERTBARTOKEN; }
{VAR}					    {  return VARTOKEN; }
{PROC}					    {  return PROCTOKEN; }
{PROCEDURE}				    {  return PROCEDURETOKEN; }
{RETURN}				    {  return RETURNTOKEN; }
{NOP}	         			    {  return NOPTOKEN; }
{AT}					    {  return ATTOKEN; }
{DOUBLECOLON}				    {  return DOUBLECOLONTOKEN; }
{COLON}  				    {  return COLONTOKEN; }
{DOTCOLON}				    {  return DOTCOLONTOKEN; }
{COLONDOT}				    {  return COLONDOTTOKEN; }


{PLUS}            			    {  return PLUSTOKEN; }
{MINUS}           			    {  return MINUSTOKEN; }
{MUL}          				    {  return MULTOKEN; }
{DIV}            			    {  return DIVTOKEN; }
{POW}            			    {  return POWTOKEN; }

{SQRT}            			    {  return SQRTTOKEN; }
{EXP}             			    {  return EXPTOKEN; }
{LOG}             			    {  return LOGTOKEN; }
{LOG2}            			    {  return LOG2TOKEN; }
{LOG10}           			    {  return LOG10TOKEN; }
{SIN}             			    {  return SINTOKEN; }
{COS}             			    {  return COSTOKEN; }
{TAN}             			    {  return TANTOKEN; }
{ASIN}            			    {  return ASINTOKEN; }
{ACOS}            			    {  return ACOSTOKEN; }
{ATAN}            			    {  return ATANTOKEN; }
{SINH}            			    {  return SINHTOKEN; }
{COSH}            			    {  return COSHTOKEN; }
{TANH}            			    {  return TANHTOKEN; }
{ASINH}           			    {  return ASINHTOKEN; }
{ACOSH}           			    {  return ACOSHTOKEN; }
{ATANH}           			    {  return ATANHTOKEN; }
{ABS}             			    {  return ABSTOKEN; }
{ERF}             			    {  return ERFTOKEN; }
{ERFC}            			    {  return ERFCTOKEN; }
{LOG1P}           			    {  return LOG1PTOKEN; }
{EXPM1}           			    {  return EXPM1TOKEN; }
{DOUBLE}          			    {  return DOUBLETOKEN; }
{SINGLE}          			    {  return SINGLETOKEN; }
{QUAD}          			    {  return QUADTOKEN; }
{HALFPRECISION}          		    {  return HALFPRECISIONTOKEN; }
{DOUBLEDOUBLE}				    {  return DOUBLEDOUBLETOKEN; }
{TRIPLEDOUBLE}    			    {  return TRIPLEDOUBLETOKEN; }
{DOUBLEEXTENDED}  			    {  return DOUBLEEXTENDEDTOKEN; }
{CEIL}            			    {  return CEILTOKEN; }
{FLOOR}           			    {  return FLOORTOKEN; }

{PREC}            			    {  return PRECTOKEN; }
{POINTS}          			    {  return POINTSTOKEN; }
{DIAM}            			    {  return DIAMTOKEN; }
{DISPLAY}          			    {  return DISPLAYTOKEN; }
{VERBOSITY}       			    {  return VERBOSITYTOKEN; }
{CANONICAL}       			    {  return CANONICALTOKEN; }
{AUTOSIMPLIFY}    			    {  return AUTOSIMPLIFYTOKEN; }
{TAYLORRECURSIONS}			    {  return TAYLORRECURSIONSTOKEN; }
{TIMING}          			    {  return TIMINGTOKEN; }
{TIME}          			    {  return TIMETOKEN; }
{FULLPARENTHESES} 			    {  return FULLPARENTHESESTOKEN; }
{MIDPOINTMODE}    			    {  return MIDPOINTMODETOKEN; }
{DIEONERRORMODE}    			    {  return DIEONERRORMODETOKEN; }
{SUPPRESSWARNINGS}			    {  return SUPPRESSWARNINGSTOKEN; }
{RATIONALMODE}			            {  return RATIONALMODETOKEN; }
{HOPITALRECURSIONS}			    {  return HOPITALRECURSIONSTOKEN; }

{ON}              			    {  return ONTOKEN; }
{OFF}             			    {  return OFFTOKEN; }
{DYADIC}				    {  return DYADICTOKEN; }
{POWERS}          			    {  return POWERSTOKEN; }
{BINARY}          			    {  return BINARYTOKEN; }
{HEXADECIMAL}          			    {  return HEXADECIMALTOKEN; }
{FILE}            			    {  return FILETOKEN; }
{POSTSCRIPT}      			    {  return POSTSCRIPTTOKEN; }
{POSTSCRIPTFILE}  			    {  return POSTSCRIPTFILETOKEN; }
{PERTURB}         			    {  return PERTURBTOKEN; }
{MINUSWORD}       			    {  return MINUSWORDTOKEN; }
{PLUSWORD}        			    {  return PLUSWORDTOKEN; }
{ZEROWORD}        			    {  return ZEROWORDTOKEN; }
{NEAREST}         			    {  return NEARESTTOKEN; }
{HONORCOEFFPREC} 			    {  return HONORCOEFFPRECTOKEN; }
{TRUE}					    {  return TRUETOKEN; }
{FALSE}					    {  return FALSETOKEN; }
{DEFAULT}				    {  return DEFAULTTOKEN; }
{MATCH} 				    {  return MATCHTOKEN; }
{WITH}  				    {  return WITHTOKEN; }
{HEAD}   				    {  return HEADTOKEN; }
{ROUNDCORRECTLY}   		       	    {  return ROUNDCORRECTLYTOKEN; }
{REVERT}   				    {  return REVERTTOKEN; }
{SORT}   				    {  return SORTTOKEN; }
{MANTISSA}   				    {  return MANTISSATOKEN; }
{EXPONENT}   				    {  return EXPONENTTOKEN; }
{PRECISION}   				    {  return PRECISIONTOKEN; }
{TAIL}   				    {  return TAILTOKEN; }
{INF}   				    {  return INFTOKEN; }
{MID}   				    {  return MIDTOKEN; }
{SUP}   				    {  return SUPTOKEN; }
{MIN}   				    {  return MINTOKEN; }
{MAX}   				    {  return MAXTOKEN; }
{READXML}   				    {  return READXMLTOKEN; }
{LENGTH}   				    {  return LENGTHTOKEN; }
{ABSOLUTE}   				    {  return ABSOLUTETOKEN; }
{RELATIVE}   				    {  return RELATIVETOKEN; }
{DECIMAL}   				    {  return DECIMALTOKEN; }
{FIXED}   				    {  return FIXEDTOKEN; }
{FLOATING}   				    {  return FLOATINGTOKEN; }

{ERROR}   				    {  return ERRORTOKEN; }
{READFILE}   				    {  return READFILETOKEN; }
{ISBOUND}   				    {  return ISBOUNDTOKEN; }

{QUIT}             			    {
			                        return FALSEQUITTOKEN;
                                            }
{RESTART}         			    { return FALSERESTARTTOKEN; }

{LIBRARY}         			    {  return LIBRARYTOKEN; }

{DIFF}            			    {  return DIFFTOKEN; }
{SIMPLIFY}				    {  return SIMPLIFYTOKEN; }
{REMEZ}           			    {  return REMEZTOKEN; }
{FPMINIMAX}           			    {  return FPMINIMAXTOKEN; }
{HORNER}          			    {  return HORNERTOKEN; }
{EXPAND}          			    {  return EXPANDTOKEN; }
{SIMPLIFYSAFE}				    {  return SIMPLIFYSAFETOKEN; }
{TAYLOR}         			    {  return TAYLORTOKEN; }
{TAYLORFORM}         			    {  return TAYLORFORMTOKEN; }
{AUTODIFF}         			    {  return AUTODIFFTOKEN; }
{DEGREE}          			    {  return DEGREETOKEN; }
{NUMERATOR}       			    {  return NUMERATORTOKEN; }
{DENOMINATOR}     			    {  return DENOMINATORTOKEN; }
{SUBSTITUTE}      			    {  return SUBSTITUTETOKEN; }
{COEFF}           			    {  return COEFFTOKEN; }
{SUBPOLY}         			    {  return SUBPOLYTOKEN; }
{ROUNDCOEFFICIENTS} 			    {  return ROUNDCOEFFICIENTSTOKEN; }
{RATIONALAPPROX}  			    {  return RATIONALAPPROXTOKEN; }
{APPROX}           			    {  return APPROXTOKEN; }
{ACCURATEINFNORM}  			    {  return ACCURATEINFNORMTOKEN; }
{ROUNDTOFORMAT}   			    {  return ROUNDTOFORMATTOKEN; }
{EVALUATE}        			    {  return EVALUATETOKEN; }

{PARSE}           			    {  return PARSETOKEN; }

{PRINT}           			    {  return PRINTTOKEN; }
{PRINTXML}           			    {  return PRINTXMLTOKEN; }
{EXECUTE}           			    {  return EXECUTETOKEN; }
{PLOT}            			    {  return PLOTTOKEN; }
{PRINTHEXA}       			    {  return PRINTHEXATOKEN; }
{PRINTFLOAT}       			    {  return PRINTFLOATTOKEN; }
{PRINTBINARY}     			    {  return PRINTBINARYTOKEN; }
{PRINTEXPANSION}  			    {  return PRINTEXPANSIONTOKEN; }
{BASHEXECUTE}     			    {  return BASHEXECUTETOKEN; }
{BASHEVALUATE}     			    {  return BASHEVALUATETOKEN; }
{EXTERNALPLOT}    			    {  return EXTERNALPLOTTOKEN; }
{WRITE}           			    {  return WRITETOKEN; }
{ASCIIPLOT}       			    {  return ASCIIPLOTTOKEN; }
{RENAME}         			    {  return RENAMETOKEN; }


{INFNORM}         			    {  return INFNORMTOKEN; }
{SUPNORM}         			    {  return SUPNORMTOKEN; }
{FINDZEROS}       			    {  return FINDZEROSTOKEN; }
{FPFINDZEROS}     			    {  return FPFINDZEROSTOKEN; }
{DIRTYINFNORM}    			    {  return DIRTYINFNORMTOKEN; }
{NUMBERROOTS}    			    {  return NUMBERROOTSTOKEN; }
{INTEGRAL}        			    {  return INTEGRALTOKEN; }
{DIRTYINTEGRAL}				    {  return DIRTYINTEGRALTOKEN; }
{WORSTCASE}       			    {  return WORSTCASETOKEN; }
{IMPLEMENTPOLY}				    {  return IMPLEMENTPOLYTOKEN; }
{IMPLEMENTCONST}                            {  return IMPLEMENTCONSTTOKEN; }
{CHECKINFNORM}    			    {  return CHECKINFNORMTOKEN; }
{ZERODENOMINATORS}			    {  return ZERODENOMINATORSTOKEN; }
{ISEVALUABLE}     			    {  return ISEVALUABLETOKEN; }
{SEARCHGAL}       			    {  return SEARCHGALTOKEN; }
{GUESSDEGREE}     			    {  return GUESSDEGREETOKEN; }
{DIRTYFINDZEROS}  			    {  return DIRTYFINDZEROSTOKEN; }

{IF}					    {  return IFTOKEN; }
{THEN}					    {  return THENTOKEN; }
{ELSE}					    {  return ELSETOKEN; }
{FOR}					    {  return FORTOKEN; }
{IN}					    {  return INTOKEN; }
{FROM}					    {  return FROMTOKEN; }
{TO}					    {  return TOTOKEN; }
{BY}					    {  return BYTOKEN; }
{DO}					    {  return DOTOKEN; }
{BEGIN}					    {  return BEGINTOKEN; }
{END}					    {  return ENDTOKEN; }
{LBRACE}				    {  return LEFTCURLYBRACETOKEN; }
{RBRACE}				    {  return RIGHTCURLYBRACETOKEN; }
{WHILEDEF}				    {  return WHILETOKEN; }

{EXTERNALPROC}                              {  return EXTERNALPROCTOKEN; }
{VOID}                                      {  return VOIDTOKEN; }
{CONSTANTTYPE}                              {  return CONSTANTTYPETOKEN; }
{FUNCTION}                                  {  return FUNCTIONTOKEN; }
{RANGE}                                     {  return RANGETOKEN; }
{INTEGER}                                   {  return INTEGERTOKEN; }
{STRINGTYPE}                                {  return STRINGTYPETOKEN; }
{BOOLEAN}                                   {  return BOOLEANTOKEN; }
{LIST}                                      {  return LISTTOKEN; }
{OF}                                        {  return OFTOKEN; }



[ \t]		{ /* Eat up spaces and tabulators */
		}

[\n]		{

		}

{IDENTIFIER}        			    { constBuffer = (char *) safeCalloc(yyleng+1,sizeof(char));
					      strncpy(constBuffer,yytext,yyleng);
					      yylval->value = constBuffer;
					       return IDENTIFIERTOKEN; }


.		{ /* otherwise */
			printMessage(1,"The character \"%s\" cannot be recognized. Will ignore it.\n",
				yytext);
}

%%

void *startMiniparser(void *scanner, char *str) {
  YY_BUFFER_STATE buf;

  buf = miniyy_scan_string(str, scanner);
  miniyy_switch_to_buffer(buf, scanner);

  return NULL;
}

void endMiniparser(void *buf, void *scanner) {
  miniyy_delete_buffer(buf, scanner);
}
