/*

Copyright 2006-2011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668,

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2)

and by

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
#include "parser.h"
#include "general.h"
#include "chain.h"

#define YY_NO_UNPUT 1

%}

%option noyywrap
%option always-interactive
%option reentrant
%option bison-bridge
%option nounput

%x readstate
%x readstate2
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

READ            ("read"|"#include")

COMMENTSTART    "/*"
COMMENTEND      "*/"

ONELINECOMMENT  ("//"|"#!")([^\n])*"\n"

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
SUPPRESSWARNINGS "roundingwarnings"
RATIONALMODE    "rationalmode"
DIEONERRORMODE  "dieonerrormode"

LIBRARY         "library"
LIBRARYCONSTANT "libraryconstant"

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

VERSION         "version"

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
  YY_BUFFER_STATE *currStatePtr;
%}




{COMMENTSTART}  {     BEGIN(commentstate); }

<commentstate>{COMMENTEND} { BEGIN(INITIAL); }

<commentstate>. { // Eat up comments

                 }

<commentstate>[\n] { // Eat up newlines in comments

		}



{ONELINECOMMENT} {  // Eat up comments
                     carriageReturnLexed();
                 }


{READ}          {
                      BEGIN(readstate);
                }

<readstate>{STRING}  {
                       if (newReadFilename != NULL) free(newReadFilename);
		       newReadFilename = NULL;
		       newReadFilename = (char*) safeCalloc(yyleng - 1,sizeof(char));
		       strncpy(newReadFilename,yytext+1,yyleng-2);
		       BEGIN(readstate2);
                }

<readstate>[ \t]     { /* Eat up spaces and tabulators */
		}

<readstate>.         { /* otherwise */
			printMessage(1,"The character \"%s\" cannot be recognized. Will ignore it.\n",
				yytext);
		}

<readstate2>{SEMICOLON} {
                      newReadFilenameTemp = (char *) safeCalloc(strlen(newReadFilename)+1,sizeof(char));
		      demaskString(newReadFilenameTemp,newReadFilename);
		      temp_fd = fopen(newReadFilenameTemp,"r");
		      if (temp_fd == NULL) {
                        if (verbosity >= 1) {
                           changeToWarningMode();
                           sollyaPrintf("Warning: the file \"%s\" could not be opened for reading: ",newReadFilenameTemp);
                           sollyaPrintf("\"%s\".\n",strerror(errno));
			   sollyaPrintf("The last command will have no effect. No memory will be lost.\n");
                           considerDyingOnError();
                           restoreMode();
                        }
                        promptToBePrinted = 1;
		      } else {
			newReadFileStarted();
			currStatePtr = (YY_BUFFER_STATE *) safeMalloc(sizeof(YY_BUFFER_STATE));
			*currStatePtr = YY_CURRENT_BUFFER;
			readStack = addElement(readStack, (void *) currStatePtr);
			tempFDPtr = (FILE **) safeMalloc(sizeof(FILE *));
			*tempFDPtr = yyin;
			readStack2 = addElement(readStack2, (void *) tempFDPtr);
			yyin = temp_fd;
			yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE, scanner), scanner);
		      }
		      free(newReadFilenameTemp);
		      BEGIN(INITIAL);
                }

<readstate2>[ \t]     { /* Eat up spaces and tabulators */
		}

<readstate2>.         { /* otherwise */
			printMessage(1,"The character \"%s\" cannot be recognized. Will ignore it.\n",
				yytext);
		}

<<EOF>>         {
                      if (readStack == NULL) {
			if (!eliminatePromptBackup) sollyaPrintf("\n");
			yyterminate();
		      } else {
			fclose(yyin);
			yyin = *((FILE **) (readStack2->value));
			free(readStack2->value);
			readStackTemp = readStack2->next;
			free(readStack2);
			readStack2 = readStackTemp;
			yy_delete_buffer(YY_CURRENT_BUFFER, scanner);
			yy_switch_to_buffer(*((YY_BUFFER_STATE *) (readStack->value)),scanner);
			free(readStack->value);
			readStackTemp = readStack->next;
			free(readStack);
			readStack = readStackTemp;
		      }
                }

{MIDPOINTCONSTANT}                          {
					      constBuffer2 = (char *) safeCalloc(yyleng+1,sizeof(char));
					      if (removeSpaces(constBuffer2,yytext)) {
						printMessage(2,"Information: removed spaces in scientific notation constant \"%s\", it will be considered as \"%s\"\n",yytext,constBuffer2);
					      }
					      yylval->value = constBuffer2;
                                              newTokenLexed(); return MIDPOINTCONSTANTTOKEN; }
{CONSTANT}                                  {
					      constBuffer2 = (char *) safeCalloc(yyleng+1,sizeof(char));
					      if (removeSpaces(constBuffer2,yytext)) {
						printMessage(2,"Information: removed spaces in scientific notation constant \"%s\", it will be considered as \"%s\"\n",yytext,constBuffer2);
					      }
					      yylval->value = constBuffer2;
                                              newTokenLexed(); return CONSTANTTOKEN; }
{DYADICCONSTANT} 			    {
                                              constBuffer = (char *) safeCalloc(yyleng+1,sizeof(char));
					      strncpy(constBuffer,yytext,yyleng);
					      yylval->value = constBuffer;
                                              newTokenLexed(); return DYADICCONSTANTTOKEN; }
{HEXADECIMALCONSTANT}     		    { constBuffer = (char *) safeCalloc(yyleng+1,sizeof(char));
					      strncpy(constBuffer,yytext,yyleng);
					      yylval->value = constBuffer;
                                              newTokenLexed(); return HEXADECIMALCONSTANTTOKEN; }
{HEXCONSTANT}     			    { constBuffer = (char *) safeCalloc(yyleng+1,sizeof(char));
					      strncpy(constBuffer,yytext,yyleng);
					      yylval->value = constBuffer;
                                              newTokenLexed(); return HEXCONSTANTTOKEN; }
{BINARYCONSTANT}  			    { constBuffer = (char *) safeCalloc(yyleng-1,sizeof(char));
					      strncpy(constBuffer,yytext,yyleng-2);
					      yylval->value = constBuffer;
                                              newTokenLexed(); return BINARYCONSTANTTOKEN; }

{PI}              			    { newTokenLexed(); return PITOKEN; }

{STRING}          			    {
					      constBuffer = (char *) safeCalloc(yyleng-1,sizeof(char));
					      constBuffer2 = (char *) safeCalloc(yyleng-1,sizeof(char));
					      strncpy(constBuffer2,yytext+1,yyleng-2);
					      demaskString(constBuffer,constBuffer2);
					      free(constBuffer2);
					      yylval->value = constBuffer;
                                              newTokenLexed(); return STRINGTOKEN; }

{LPAR}            			    { newTokenLexed(); return LPARTOKEN; }
{RPAR}            			    { newTokenLexed(); return RPARTOKEN; }
{LBRACKET}        			    { newTokenLexed(); return LBRACKETTOKEN; }
{RBRACKET}        			    { newTokenLexed(); return RBRACKETTOKEN; }
{EXCLAMATIONEQUAL}			    { newTokenLexed(); return EXCLAMATIONEQUALTOKEN; }
{COMPAREEQUAL}                              { newTokenLexed(); return COMPAREEQUALTOKEN; }
{EQUAL}           			    { newTokenLexed(); return EQUALTOKEN; }
{ASSIGNEQUAL}          			    { newTokenLexed(); return ASSIGNEQUALTOKEN; }
{COMMA}           			    { newTokenLexed(); return COMMATOKEN; }
{EXCLAMATION}				    { newTokenLexed(); return EXCLAMATIONTOKEN; }
{SEMICOLON}       			    { newTokenLexed(); return SEMICOLONTOKEN; }
{LEFTANGLE}       			    { newTokenLexed(); return LEFTANGLETOKEN; }
{STARLEFTANGLE}   			    { newTokenLexed(); return STARLEFTANGLETOKEN; }
{RIGHTANGLEUNDERSCORE}			    { newTokenLexed(); return RIGHTANGLEUNDERSCORETOKEN; }
{RIGHTANGLEDOT} 			    { newTokenLexed(); return RIGHTANGLEDOTTOKEN; }
{RIGHTANGLESTAR}			    { newTokenLexed(); return RIGHTANGLESTARTOKEN; }
{RIGHTANGLE}      			    { newTokenLexed(); return RIGHTANGLETOKEN; }
{DOTS}            			    { newTokenLexed(); return DOTSTOKEN; }
{DOT}            			    { newTokenLexed(); return DOTTOKEN; }
{QUESTIONMARK}				    { newTokenLexed(); return QUESTIONMARKTOKEN; }
{AND}         				    { newTokenLexed(); return ANDTOKEN; }
{OR}         				    { newTokenLexed(); return ORTOKEN; }
{VERTBAR}				    { newTokenLexed(); return VERTBARTOKEN; }
{VAR}					    { newTokenLexed(); return VARTOKEN; }
{PROC}					    { newTokenLexed(); return PROCTOKEN; }
{PROCEDURE}				    { newTokenLexed(); return PROCEDURETOKEN; }
{RETURN}				    { newTokenLexed(); return RETURNTOKEN; }
{NOP}	         			    { newTokenLexed(); return NOPTOKEN; }
{AT}					    { newTokenLexed(); return ATTOKEN; }
{DOUBLECOLON}				    { newTokenLexed(); return DOUBLECOLONTOKEN; }
{COLON}  				    { newTokenLexed(); return COLONTOKEN; }
{DOTCOLON}				    { newTokenLexed(); return DOTCOLONTOKEN; }
{COLONDOT}				    { newTokenLexed(); return COLONDOTTOKEN; }

{PLUS}            			    { newTokenLexed(); return PLUSTOKEN; }
{MINUS}           			    { newTokenLexed(); return MINUSTOKEN; }
{APPROX}           			    { newTokenLexed(); return APPROXTOKEN; }
{MUL}          				    { newTokenLexed(); return MULTOKEN; }
{DIV}            			    { newTokenLexed(); return DIVTOKEN; }
{POW}            			    { newTokenLexed(); return POWTOKEN; }

{SQRT}            			    { newTokenLexed(); return SQRTTOKEN; }
{EXP}             			    { newTokenLexed(); return EXPTOKEN; }
{LOG}             			    { newTokenLexed(); return LOGTOKEN; }
{LOG2}            			    { newTokenLexed(); return LOG2TOKEN; }
{LOG10}           			    { newTokenLexed(); return LOG10TOKEN; }
{SIN}             			    { newTokenLexed(); return SINTOKEN; }
{COS}             			    { newTokenLexed(); return COSTOKEN; }
{TAN}             			    { newTokenLexed(); return TANTOKEN; }
{ASIN}            			    { newTokenLexed(); return ASINTOKEN; }
{ACOS}            			    { newTokenLexed(); return ACOSTOKEN; }
{ATAN}            			    { newTokenLexed(); return ATANTOKEN; }
{SINH}            			    { newTokenLexed(); return SINHTOKEN; }
{COSH}            			    { newTokenLexed(); return COSHTOKEN; }
{TANH}            			    { newTokenLexed(); return TANHTOKEN; }
{ASINH}           			    { newTokenLexed(); return ASINHTOKEN; }
{ACOSH}           			    { newTokenLexed(); return ACOSHTOKEN; }
{ATANH}           			    { newTokenLexed(); return ATANHTOKEN; }
{ABS}             			    { newTokenLexed(); return ABSTOKEN; }
{ERF}             			    { newTokenLexed(); return ERFTOKEN; }
{ERFC}            			    { newTokenLexed(); return ERFCTOKEN; }
{LOG1P}           			    { newTokenLexed(); return LOG1PTOKEN; }
{EXPM1}           			    { newTokenLexed(); return EXPM1TOKEN; }
{DOUBLE}          			    { newTokenLexed(); return DOUBLETOKEN; }
{SINGLE}          			    { newTokenLexed(); return SINGLETOKEN; }
{QUAD}          			    { newTokenLexed(); return QUADTOKEN; }
{HALFPRECISION}          		    { newTokenLexed(); return HALFPRECISIONTOKEN; }
{DOUBLEDOUBLE}				    { newTokenLexed(); return DOUBLEDOUBLETOKEN; }
{TRIPLEDOUBLE}    			    { newTokenLexed(); return TRIPLEDOUBLETOKEN; }
{DOUBLEEXTENDED}  			    { newTokenLexed(); return DOUBLEEXTENDEDTOKEN; }
{CEIL}            			    { newTokenLexed(); return CEILTOKEN; }
{FLOOR}           			    { newTokenLexed(); return FLOORTOKEN; }
{NEARESTINT}           			    { newTokenLexed(); return NEARESTINTTOKEN; }

{PREC}            			    { newTokenLexed(); return PRECTOKEN; }
{POINTS}          			    { newTokenLexed(); return POINTSTOKEN; }
{DIAM}            			    { newTokenLexed(); return DIAMTOKEN; }
{DISPLAY}          			    { newTokenLexed(); return DISPLAYTOKEN; }
{VERBOSITY}       			    { newTokenLexed(); return VERBOSITYTOKEN; }
{CANONICAL}       			    { newTokenLexed(); return CANONICALTOKEN; }
{AUTOSIMPLIFY}    			    { newTokenLexed(); return AUTOSIMPLIFYTOKEN; }
{TAYLORRECURSIONS}			    { newTokenLexed(); return TAYLORRECURSIONSTOKEN; }
{TIMING}          			    { newTokenLexed(); return TIMINGTOKEN; }
{TIME}          			    { newTokenLexed(); return TIMETOKEN; }
{FULLPARENTHESES} 			    { newTokenLexed(); return FULLPARENTHESESTOKEN; }
{MIDPOINTMODE}    			    { newTokenLexed(); return MIDPOINTMODETOKEN; }
{DIEONERRORMODE}    			    { newTokenLexed(); return DIEONERRORMODETOKEN; }
{SUPPRESSWARNINGS}			    { newTokenLexed(); return SUPPRESSWARNINGSTOKEN; }
{RATIONALMODE}			            { newTokenLexed(); return RATIONALMODETOKEN; }
{HOPITALRECURSIONS}			    { newTokenLexed(); return HOPITALRECURSIONSTOKEN; }

{ON}              			    { newTokenLexed(); return ONTOKEN; }
{OFF}             			    { newTokenLexed(); return OFFTOKEN; }
{DYADIC}				    { newTokenLexed(); return DYADICTOKEN; }
{POWERS}          			    { newTokenLexed(); return POWERSTOKEN; }
{BINARY}          			    { newTokenLexed(); return BINARYTOKEN; }
{HEXADECIMAL}          			    { newTokenLexed(); return HEXADECIMALTOKEN; }
{FILE}            			    { newTokenLexed(); return FILETOKEN; }
{POSTSCRIPT}      			    { newTokenLexed(); return POSTSCRIPTTOKEN; }
{POSTSCRIPTFILE}  			    { newTokenLexed(); return POSTSCRIPTFILETOKEN; }
{PERTURB}         			    { newTokenLexed(); return PERTURBTOKEN; }
{MINUSWORD}       			    { newTokenLexed(); return MINUSWORDTOKEN; }
{PLUSWORD}        			    { newTokenLexed(); return PLUSWORDTOKEN; }
{ZEROWORD}        			    { newTokenLexed(); return ZEROWORDTOKEN; }
{NEAREST}         			    { newTokenLexed(); return NEARESTTOKEN; }
{HONORCOEFFPREC} 			    { newTokenLexed(); return HONORCOEFFPRECTOKEN; }
{TRUE}					    { newTokenLexed(); return TRUETOKEN; }
{FALSE}					    { newTokenLexed(); return FALSETOKEN; }
{DEFAULT}				    { newTokenLexed(); return DEFAULTTOKEN; }
{MATCH} 				    { newTokenLexed(); return MATCHTOKEN; }
{WITH}  				    { newTokenLexed(); return WITHTOKEN; }
{HEAD}   				    { newTokenLexed(); return HEADTOKEN; }
{ROUNDCORRECTLY}   		       	    { newTokenLexed(); return ROUNDCORRECTLYTOKEN; }
{REVERT}   				    { newTokenLexed(); return REVERTTOKEN; }
{SORT}   				    { newTokenLexed(); return SORTTOKEN; }
{MANTISSA}   				    { newTokenLexed(); return MANTISSATOKEN; }
{EXPONENT}   				    { newTokenLexed(); return EXPONENTTOKEN; }
{PRECISION}   				    { newTokenLexed(); return PRECISIONTOKEN; }
{TAIL}   				    { newTokenLexed(); return TAILTOKEN; }
{INF}   				    { newTokenLexed(); return INFTOKEN; }
{MID}   				    { newTokenLexed(); return MIDTOKEN; }
{SUP}   				    { newTokenLexed(); return SUPTOKEN; }
{MIN}   				    { newTokenLexed(); return MINTOKEN; }
{MAX}   				    { newTokenLexed(); return MAXTOKEN; }
{READXML}   				    { newTokenLexed(); return READXMLTOKEN; }
{LENGTH}   				    { newTokenLexed(); return LENGTHTOKEN; }
{ABSOLUTE}   				    { newTokenLexed(); return ABSOLUTETOKEN; }
{RELATIVE}   				    { newTokenLexed(); return RELATIVETOKEN; }
{DECIMAL}   				    { newTokenLexed(); return DECIMALTOKEN; }
{FIXED}   				    { newTokenLexed(); return FIXEDTOKEN; }
{FLOATING}   				    { newTokenLexed(); return FLOATINGTOKEN; }

{ERROR}   				    { newTokenLexed(); return ERRORTOKEN; }
{READFILE}   				    { newTokenLexed(); return READFILETOKEN; }
{ISBOUND}   				    { newTokenLexed(); return ISBOUNDTOKEN; }

{QUIT}            			    {
                                              newTokenLexed();
                                              if (readStack != NULL) {
			                        return FALSEQUITTOKEN;
                                              }
                                              return QUITTOKEN;
                                            }
{RESTART}         			    { newTokenLexed(); return RESTARTTOKEN; }
{VERSION}         			    { newTokenLexed(); return VERSIONTOKEN; }

{LIBRARY}         			    { newTokenLexed(); return LIBRARYTOKEN; }
{LIBRARYCONSTANT}         		    { newTokenLexed(); return LIBRARYCONSTANTTOKEN; }

{DIFF}            			    { newTokenLexed(); return DIFFTOKEN; }
{SIMPLIFY}				    { newTokenLexed(); return SIMPLIFYTOKEN; }
{REMEZ}           			    { newTokenLexed(); return REMEZTOKEN; }
{FPMINIMAX}           			    { newTokenLexed(); return FPMINIMAXTOKEN; }
{HORNER}          			    { newTokenLexed(); return HORNERTOKEN; }
{EXPAND}          			    { newTokenLexed(); return EXPANDTOKEN; }
{SIMPLIFYSAFE}				    { newTokenLexed(); return SIMPLIFYSAFETOKEN; }
{TAYLOR}         			    { newTokenLexed(); return TAYLORTOKEN; }
{TAYLORFORM}         			    { newTokenLexed(); return TAYLORFORMTOKEN; }
{AUTODIFF}         			    { newTokenLexed(); return AUTODIFFTOKEN; }
{DEGREE}          			    { newTokenLexed(); return DEGREETOKEN; }
{NUMERATOR}       			    { newTokenLexed(); return NUMERATORTOKEN; }
{DENOMINATOR}     			    { newTokenLexed(); return DENOMINATORTOKEN; }
{SUBSTITUTE}      			    { newTokenLexed(); return SUBSTITUTETOKEN; }
{COEFF}           			    { newTokenLexed(); return COEFFTOKEN; }
{SUBPOLY}         			    { newTokenLexed(); return SUBPOLYTOKEN; }
{ROUNDCOEFFICIENTS} 			    { newTokenLexed(); return ROUNDCOEFFICIENTSTOKEN; }
{RATIONALAPPROX}  			    { newTokenLexed(); return RATIONALAPPROXTOKEN; }
{ACCURATEINFNORM}  			    { newTokenLexed(); return ACCURATEINFNORMTOKEN; }
{ROUNDTOFORMAT}   			    { newTokenLexed(); return ROUNDTOFORMATTOKEN; }
{EVALUATE}        			    { newTokenLexed(); return EVALUATETOKEN; }

{PARSE}           			    { newTokenLexed(); return PARSETOKEN; }

{PRINT}           			    { newTokenLexed(); return PRINTTOKEN; }
{PRINTXML}           			    { newTokenLexed(); return PRINTXMLTOKEN; }
{EXECUTE}           			    { newTokenLexed(); return EXECUTETOKEN; }
{PLOT}            			    { newTokenLexed(); return PLOTTOKEN; }
{PRINTHEXA}       			    { newTokenLexed(); return PRINTHEXATOKEN; }
{PRINTFLOAT}       			    { newTokenLexed(); return PRINTFLOATTOKEN; }
{PRINTBINARY}     			    { newTokenLexed(); return PRINTBINARYTOKEN; }
{PRINTEXPANSION}  			    { newTokenLexed(); return PRINTEXPANSIONTOKEN; }
{BASHEXECUTE}     			    { newTokenLexed(); return BASHEXECUTETOKEN; }
{BASHEVALUATE}     			    { newTokenLexed(); return BASHEVALUATETOKEN; }
{EXTERNALPLOT}    			    { newTokenLexed(); return EXTERNALPLOTTOKEN; }
{WRITE}           			    { newTokenLexed(); return WRITETOKEN; }
{ASCIIPLOT}       			    { newTokenLexed(); return ASCIIPLOTTOKEN; }
{RENAME}         			    { newTokenLexed(); return RENAMETOKEN; }

{INFNORM}         			    { newTokenLexed(); return INFNORMTOKEN; }
{SUPNORM}         			    { newTokenLexed(); return SUPNORMTOKEN; }
{FINDZEROS}       			    { newTokenLexed(); return FINDZEROSTOKEN; }
{FPFINDZEROS}     			    { newTokenLexed(); return FPFINDZEROSTOKEN; }
{DIRTYINFNORM}    			    { newTokenLexed(); return DIRTYINFNORMTOKEN; }
{NUMBERROOTS}    			    { newTokenLexed(); return NUMBERROOTSTOKEN; }
{INTEGRAL}        			    { newTokenLexed(); return INTEGRALTOKEN; }
{DIRTYINTEGRAL}				    { newTokenLexed(); return DIRTYINTEGRALTOKEN; }
{WORSTCASE}       			    { newTokenLexed(); return WORSTCASETOKEN; }
{IMPLEMENTPOLY}				    { newTokenLexed(); return IMPLEMENTPOLYTOKEN; }
{IMPLEMENTCONST}                            { newTokenLexed(); return IMPLEMENTCONSTTOKEN; }
{CHECKINFNORM}    			    { newTokenLexed(); return CHECKINFNORMTOKEN; }
{ZERODENOMINATORS}			    { newTokenLexed(); return ZERODENOMINATORSTOKEN; }
{ISEVALUABLE}     			    { newTokenLexed(); return ISEVALUABLETOKEN; }
{SEARCHGAL}       			    { newTokenLexed(); return SEARCHGALTOKEN; }
{GUESSDEGREE}     			    { newTokenLexed(); return GUESSDEGREETOKEN; }
{DIRTYFINDZEROS}  			    { newTokenLexed(); return DIRTYFINDZEROSTOKEN; }

{IF}					    { newTokenLexed(); return IFTOKEN; }
{THEN}					    { newTokenLexed(); return THENTOKEN; }
{ELSE}					    { newTokenLexed(); return ELSETOKEN; }
{FOR}					    { newTokenLexed(); return FORTOKEN; }
{IN}					    { newTokenLexed(); return INTOKEN; }
{FROM}					    { newTokenLexed(); return FROMTOKEN; }
{TO}					    { newTokenLexed(); return TOTOKEN; }
{BY}					    { newTokenLexed(); return BYTOKEN; }
{DO}					    { newTokenLexed(); return DOTOKEN; }
{BEGIN}					    { newTokenLexed(); return BEGINTOKEN; }
{END}					    { newTokenLexed(); return ENDTOKEN; }
{LBRACE}				    { newTokenLexed(); return LEFTCURLYBRACETOKEN; }
{RBRACE}				    { newTokenLexed(); return RIGHTCURLYBRACETOKEN; }
{WHILEDEF}				    { newTokenLexed(); return WHILETOKEN; }

{EXTERNALPROC}                              { newTokenLexed(); return EXTERNALPROCTOKEN; }
{VOID}                                      { newTokenLexed(); return VOIDTOKEN; }
{CONSTANTTYPE}                              { newTokenLexed(); return CONSTANTTYPETOKEN; }
{FUNCTION}                                  { newTokenLexed(); return FUNCTIONTOKEN; }
{RANGE}                                     { newTokenLexed(); return RANGETOKEN; }
{INTEGER}                                   { newTokenLexed(); return INTEGERTOKEN; }
{STRINGTYPE}                                { newTokenLexed(); return STRINGTYPETOKEN; }
{BOOLEAN}                                   { newTokenLexed(); return BOOLEANTOKEN; }
{LIST}                                      { newTokenLexed(); return LISTTOKEN; }
{OF}                                        { newTokenLexed(); return OFTOKEN; }

{HELP}            			    { newTokenLexed(); return HELPTOKEN; }


[ \t]		{ /* Eat up spaces and tabulators */
		}

[\n]		{
                     carriageReturnLexed();
		}

{IDENTIFIER}        			    { constBuffer = (char *) safeCalloc(yyleng+1,sizeof(char));
					      strncpy(constBuffer,yytext,yyleng);
					      yylval->value = constBuffer;
					      newTokenLexed(); return IDENTIFIERTOKEN; }

.		{ /* otherwise */
			printMessage(1,"The character \"%s\" cannot be recognized. Will ignore it.\n",
				yytext);
		}

%%


char *getCurrentLexSymbol() {
  char *str;

  str = (char *) safeCalloc(yyget_leng(scanner)+1,sizeof(char));
  strcpy(str,yyget_text(scanner));

  return str;
}
