/* A Bison parser, made by GNU Bison 2.4.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2009, 2010 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         miniyyparse
#define yylex           miniyylex
#define yyerror         miniyyerror
#define yylval          miniyylval
#define yychar          miniyychar
#define yydebug         miniyydebug
#define yynerrs         miniyynerrs


/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 55 "miniparser.y"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "expression.h"
#include "assignment.h"
#include "chain.h"
#include "general.h"
#include "execute.h"
#include "miniparser.h"

#define YYERROR_VERBOSE 1

extern int miniyylex(YYSTYPE *lvalp, void *scanner);

void miniyyerror(void *myScanner, char *message) {
    printMessage(1,"Warning: %s. Will try to continue parsing (expecting \";\"). May leak memory.\n",message);
}



/* Line 189 of yacc.c  */
#line 103 "miniparser.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     CONSTANTTOKEN = 258,
     MIDPOINTCONSTANTTOKEN = 259,
     DYADICCONSTANTTOKEN = 260,
     HEXCONSTANTTOKEN = 261,
     HEXADECIMALCONSTANTTOKEN = 262,
     BINARYCONSTANTTOKEN = 263,
     PITOKEN = 264,
     IDENTIFIERTOKEN = 265,
     STRINGTOKEN = 266,
     LPARTOKEN = 267,
     RPARTOKEN = 268,
     LBRACKETTOKEN = 269,
     RBRACKETTOKEN = 270,
     EQUALTOKEN = 271,
     ASSIGNEQUALTOKEN = 272,
     COMPAREEQUALTOKEN = 273,
     COMMATOKEN = 274,
     EXCLAMATIONTOKEN = 275,
     SEMICOLONTOKEN = 276,
     STARLEFTANGLETOKEN = 277,
     LEFTANGLETOKEN = 278,
     RIGHTANGLEUNDERSCORETOKEN = 279,
     RIGHTANGLEDOTTOKEN = 280,
     RIGHTANGLESTARTOKEN = 281,
     RIGHTANGLETOKEN = 282,
     DOTSTOKEN = 283,
     DOTTOKEN = 284,
     QUESTIONMARKTOKEN = 285,
     VERTBARTOKEN = 286,
     ATTOKEN = 287,
     DOUBLECOLONTOKEN = 288,
     COLONTOKEN = 289,
     DOTCOLONTOKEN = 290,
     COLONDOTTOKEN = 291,
     EXCLAMATIONEQUALTOKEN = 292,
     APPROXTOKEN = 293,
     ANDTOKEN = 294,
     ORTOKEN = 295,
     PLUSTOKEN = 296,
     MINUSTOKEN = 297,
     MULTOKEN = 298,
     DIVTOKEN = 299,
     POWTOKEN = 300,
     SQRTTOKEN = 301,
     EXPTOKEN = 302,
     LOGTOKEN = 303,
     LOG2TOKEN = 304,
     LOG10TOKEN = 305,
     SINTOKEN = 306,
     COSTOKEN = 307,
     TANTOKEN = 308,
     ASINTOKEN = 309,
     ACOSTOKEN = 310,
     ATANTOKEN = 311,
     SINHTOKEN = 312,
     COSHTOKEN = 313,
     TANHTOKEN = 314,
     ASINHTOKEN = 315,
     ACOSHTOKEN = 316,
     ATANHTOKEN = 317,
     ABSTOKEN = 318,
     ERFTOKEN = 319,
     ERFCTOKEN = 320,
     LOG1PTOKEN = 321,
     EXPM1TOKEN = 322,
     DOUBLETOKEN = 323,
     SINGLETOKEN = 324,
     QUADTOKEN = 325,
     HALFPRECISIONTOKEN = 326,
     DOUBLEDOUBLETOKEN = 327,
     TRIPLEDOUBLETOKEN = 328,
     DOUBLEEXTENDEDTOKEN = 329,
     CEILTOKEN = 330,
     FLOORTOKEN = 331,
     NEARESTINTTOKEN = 332,
     HEADTOKEN = 333,
     REVERTTOKEN = 334,
     SORTTOKEN = 335,
     TAILTOKEN = 336,
     MANTISSATOKEN = 337,
     EXPONENTTOKEN = 338,
     PRECISIONTOKEN = 339,
     ROUNDCORRECTLYTOKEN = 340,
     PRECTOKEN = 341,
     POINTSTOKEN = 342,
     DIAMTOKEN = 343,
     DISPLAYTOKEN = 344,
     VERBOSITYTOKEN = 345,
     CANONICALTOKEN = 346,
     AUTOSIMPLIFYTOKEN = 347,
     TAYLORRECURSIONSTOKEN = 348,
     TIMINGTOKEN = 349,
     FULLPARENTHESESTOKEN = 350,
     MIDPOINTMODETOKEN = 351,
     DIEONERRORMODETOKEN = 352,
     SUPPRESSWARNINGSTOKEN = 353,
     HOPITALRECURSIONSTOKEN = 354,
     RATIONALMODETOKEN = 355,
     ONTOKEN = 356,
     OFFTOKEN = 357,
     DYADICTOKEN = 358,
     POWERSTOKEN = 359,
     BINARYTOKEN = 360,
     HEXADECIMALTOKEN = 361,
     FILETOKEN = 362,
     POSTSCRIPTTOKEN = 363,
     POSTSCRIPTFILETOKEN = 364,
     PERTURBTOKEN = 365,
     MINUSWORDTOKEN = 366,
     PLUSWORDTOKEN = 367,
     ZEROWORDTOKEN = 368,
     NEARESTTOKEN = 369,
     HONORCOEFFPRECTOKEN = 370,
     TRUETOKEN = 371,
     FALSETOKEN = 372,
     DEFAULTTOKEN = 373,
     MATCHTOKEN = 374,
     WITHTOKEN = 375,
     ABSOLUTETOKEN = 376,
     DECIMALTOKEN = 377,
     RELATIVETOKEN = 378,
     FIXEDTOKEN = 379,
     FLOATINGTOKEN = 380,
     ERRORTOKEN = 381,
     LIBRARYTOKEN = 382,
     DIFFTOKEN = 383,
     BASHEVALUATETOKEN = 384,
     SIMPLIFYTOKEN = 385,
     REMEZTOKEN = 386,
     FPMINIMAXTOKEN = 387,
     HORNERTOKEN = 388,
     EXPANDTOKEN = 389,
     SIMPLIFYSAFETOKEN = 390,
     TAYLORTOKEN = 391,
     TAYLORFORMTOKEN = 392,
     AUTODIFFTOKEN = 393,
     DEGREETOKEN = 394,
     NUMERATORTOKEN = 395,
     DENOMINATORTOKEN = 396,
     SUBSTITUTETOKEN = 397,
     COEFFTOKEN = 398,
     SUBPOLYTOKEN = 399,
     ROUNDCOEFFICIENTSTOKEN = 400,
     RATIONALAPPROXTOKEN = 401,
     ACCURATEINFNORMTOKEN = 402,
     ROUNDTOFORMATTOKEN = 403,
     EVALUATETOKEN = 404,
     LENGTHTOKEN = 405,
     INFTOKEN = 406,
     MIDTOKEN = 407,
     SUPTOKEN = 408,
     MINTOKEN = 409,
     MAXTOKEN = 410,
     READXMLTOKEN = 411,
     PARSETOKEN = 412,
     PRINTTOKEN = 413,
     PRINTXMLTOKEN = 414,
     PLOTTOKEN = 415,
     PRINTHEXATOKEN = 416,
     PRINTFLOATTOKEN = 417,
     PRINTBINARYTOKEN = 418,
     PRINTEXPANSIONTOKEN = 419,
     BASHEXECUTETOKEN = 420,
     EXTERNALPLOTTOKEN = 421,
     WRITETOKEN = 422,
     ASCIIPLOTTOKEN = 423,
     RENAMETOKEN = 424,
     INFNORMTOKEN = 425,
     SUPNORMTOKEN = 426,
     FINDZEROSTOKEN = 427,
     FPFINDZEROSTOKEN = 428,
     DIRTYINFNORMTOKEN = 429,
     NUMBERROOTSTOKEN = 430,
     INTEGRALTOKEN = 431,
     DIRTYINTEGRALTOKEN = 432,
     WORSTCASETOKEN = 433,
     IMPLEMENTPOLYTOKEN = 434,
     IMPLEMENTCONSTTOKEN = 435,
     CHECKINFNORMTOKEN = 436,
     ZERODENOMINATORSTOKEN = 437,
     ISEVALUABLETOKEN = 438,
     SEARCHGALTOKEN = 439,
     GUESSDEGREETOKEN = 440,
     DIRTYFINDZEROSTOKEN = 441,
     IFTOKEN = 442,
     THENTOKEN = 443,
     ELSETOKEN = 444,
     FORTOKEN = 445,
     INTOKEN = 446,
     FROMTOKEN = 447,
     TOTOKEN = 448,
     BYTOKEN = 449,
     DOTOKEN = 450,
     BEGINTOKEN = 451,
     ENDTOKEN = 452,
     LEFTCURLYBRACETOKEN = 453,
     RIGHTCURLYBRACETOKEN = 454,
     WHILETOKEN = 455,
     READFILETOKEN = 456,
     ISBOUNDTOKEN = 457,
     EXECUTETOKEN = 458,
     FALSERESTARTTOKEN = 459,
     FALSEQUITTOKEN = 460,
     EXTERNALPROCTOKEN = 461,
     VOIDTOKEN = 462,
     CONSTANTTYPETOKEN = 463,
     FUNCTIONTOKEN = 464,
     RANGETOKEN = 465,
     INTEGERTOKEN = 466,
     STRINGTYPETOKEN = 467,
     BOOLEANTOKEN = 468,
     LISTTOKEN = 469,
     OFTOKEN = 470,
     VARTOKEN = 471,
     PROCTOKEN = 472,
     TIMETOKEN = 473,
     PROCEDURETOKEN = 474,
     RETURNTOKEN = 475,
     NOPTOKEN = 476
   };
#endif
/* Tokens.  */
#define CONSTANTTOKEN 258
#define MIDPOINTCONSTANTTOKEN 259
#define DYADICCONSTANTTOKEN 260
#define HEXCONSTANTTOKEN 261
#define HEXADECIMALCONSTANTTOKEN 262
#define BINARYCONSTANTTOKEN 263
#define PITOKEN 264
#define IDENTIFIERTOKEN 265
#define STRINGTOKEN 266
#define LPARTOKEN 267
#define RPARTOKEN 268
#define LBRACKETTOKEN 269
#define RBRACKETTOKEN 270
#define EQUALTOKEN 271
#define ASSIGNEQUALTOKEN 272
#define COMPAREEQUALTOKEN 273
#define COMMATOKEN 274
#define EXCLAMATIONTOKEN 275
#define SEMICOLONTOKEN 276
#define STARLEFTANGLETOKEN 277
#define LEFTANGLETOKEN 278
#define RIGHTANGLEUNDERSCORETOKEN 279
#define RIGHTANGLEDOTTOKEN 280
#define RIGHTANGLESTARTOKEN 281
#define RIGHTANGLETOKEN 282
#define DOTSTOKEN 283
#define DOTTOKEN 284
#define QUESTIONMARKTOKEN 285
#define VERTBARTOKEN 286
#define ATTOKEN 287
#define DOUBLECOLONTOKEN 288
#define COLONTOKEN 289
#define DOTCOLONTOKEN 290
#define COLONDOTTOKEN 291
#define EXCLAMATIONEQUALTOKEN 292
#define APPROXTOKEN 293
#define ANDTOKEN 294
#define ORTOKEN 295
#define PLUSTOKEN 296
#define MINUSTOKEN 297
#define MULTOKEN 298
#define DIVTOKEN 299
#define POWTOKEN 300
#define SQRTTOKEN 301
#define EXPTOKEN 302
#define LOGTOKEN 303
#define LOG2TOKEN 304
#define LOG10TOKEN 305
#define SINTOKEN 306
#define COSTOKEN 307
#define TANTOKEN 308
#define ASINTOKEN 309
#define ACOSTOKEN 310
#define ATANTOKEN 311
#define SINHTOKEN 312
#define COSHTOKEN 313
#define TANHTOKEN 314
#define ASINHTOKEN 315
#define ACOSHTOKEN 316
#define ATANHTOKEN 317
#define ABSTOKEN 318
#define ERFTOKEN 319
#define ERFCTOKEN 320
#define LOG1PTOKEN 321
#define EXPM1TOKEN 322
#define DOUBLETOKEN 323
#define SINGLETOKEN 324
#define QUADTOKEN 325
#define HALFPRECISIONTOKEN 326
#define DOUBLEDOUBLETOKEN 327
#define TRIPLEDOUBLETOKEN 328
#define DOUBLEEXTENDEDTOKEN 329
#define CEILTOKEN 330
#define FLOORTOKEN 331
#define NEARESTINTTOKEN 332
#define HEADTOKEN 333
#define REVERTTOKEN 334
#define SORTTOKEN 335
#define TAILTOKEN 336
#define MANTISSATOKEN 337
#define EXPONENTTOKEN 338
#define PRECISIONTOKEN 339
#define ROUNDCORRECTLYTOKEN 340
#define PRECTOKEN 341
#define POINTSTOKEN 342
#define DIAMTOKEN 343
#define DISPLAYTOKEN 344
#define VERBOSITYTOKEN 345
#define CANONICALTOKEN 346
#define AUTOSIMPLIFYTOKEN 347
#define TAYLORRECURSIONSTOKEN 348
#define TIMINGTOKEN 349
#define FULLPARENTHESESTOKEN 350
#define MIDPOINTMODETOKEN 351
#define DIEONERRORMODETOKEN 352
#define SUPPRESSWARNINGSTOKEN 353
#define HOPITALRECURSIONSTOKEN 354
#define RATIONALMODETOKEN 355
#define ONTOKEN 356
#define OFFTOKEN 357
#define DYADICTOKEN 358
#define POWERSTOKEN 359
#define BINARYTOKEN 360
#define HEXADECIMALTOKEN 361
#define FILETOKEN 362
#define POSTSCRIPTTOKEN 363
#define POSTSCRIPTFILETOKEN 364
#define PERTURBTOKEN 365
#define MINUSWORDTOKEN 366
#define PLUSWORDTOKEN 367
#define ZEROWORDTOKEN 368
#define NEARESTTOKEN 369
#define HONORCOEFFPRECTOKEN 370
#define TRUETOKEN 371
#define FALSETOKEN 372
#define DEFAULTTOKEN 373
#define MATCHTOKEN 374
#define WITHTOKEN 375
#define ABSOLUTETOKEN 376
#define DECIMALTOKEN 377
#define RELATIVETOKEN 378
#define FIXEDTOKEN 379
#define FLOATINGTOKEN 380
#define ERRORTOKEN 381
#define LIBRARYTOKEN 382
#define DIFFTOKEN 383
#define BASHEVALUATETOKEN 384
#define SIMPLIFYTOKEN 385
#define REMEZTOKEN 386
#define FPMINIMAXTOKEN 387
#define HORNERTOKEN 388
#define EXPANDTOKEN 389
#define SIMPLIFYSAFETOKEN 390
#define TAYLORTOKEN 391
#define TAYLORFORMTOKEN 392
#define AUTODIFFTOKEN 393
#define DEGREETOKEN 394
#define NUMERATORTOKEN 395
#define DENOMINATORTOKEN 396
#define SUBSTITUTETOKEN 397
#define COEFFTOKEN 398
#define SUBPOLYTOKEN 399
#define ROUNDCOEFFICIENTSTOKEN 400
#define RATIONALAPPROXTOKEN 401
#define ACCURATEINFNORMTOKEN 402
#define ROUNDTOFORMATTOKEN 403
#define EVALUATETOKEN 404
#define LENGTHTOKEN 405
#define INFTOKEN 406
#define MIDTOKEN 407
#define SUPTOKEN 408
#define MINTOKEN 409
#define MAXTOKEN 410
#define READXMLTOKEN 411
#define PARSETOKEN 412
#define PRINTTOKEN 413
#define PRINTXMLTOKEN 414
#define PLOTTOKEN 415
#define PRINTHEXATOKEN 416
#define PRINTFLOATTOKEN 417
#define PRINTBINARYTOKEN 418
#define PRINTEXPANSIONTOKEN 419
#define BASHEXECUTETOKEN 420
#define EXTERNALPLOTTOKEN 421
#define WRITETOKEN 422
#define ASCIIPLOTTOKEN 423
#define RENAMETOKEN 424
#define INFNORMTOKEN 425
#define SUPNORMTOKEN 426
#define FINDZEROSTOKEN 427
#define FPFINDZEROSTOKEN 428
#define DIRTYINFNORMTOKEN 429
#define NUMBERROOTSTOKEN 430
#define INTEGRALTOKEN 431
#define DIRTYINTEGRALTOKEN 432
#define WORSTCASETOKEN 433
#define IMPLEMENTPOLYTOKEN 434
#define IMPLEMENTCONSTTOKEN 435
#define CHECKINFNORMTOKEN 436
#define ZERODENOMINATORSTOKEN 437
#define ISEVALUABLETOKEN 438
#define SEARCHGALTOKEN 439
#define GUESSDEGREETOKEN 440
#define DIRTYFINDZEROSTOKEN 441
#define IFTOKEN 442
#define THENTOKEN 443
#define ELSETOKEN 444
#define FORTOKEN 445
#define INTOKEN 446
#define FROMTOKEN 447
#define TOTOKEN 448
#define BYTOKEN 449
#define DOTOKEN 450
#define BEGINTOKEN 451
#define ENDTOKEN 452
#define LEFTCURLYBRACETOKEN 453
#define RIGHTCURLYBRACETOKEN 454
#define WHILETOKEN 455
#define READFILETOKEN 456
#define ISBOUNDTOKEN 457
#define EXECUTETOKEN 458
#define FALSERESTARTTOKEN 459
#define FALSEQUITTOKEN 460
#define EXTERNALPROCTOKEN 461
#define VOIDTOKEN 462
#define CONSTANTTYPETOKEN 463
#define FUNCTIONTOKEN 464
#define RANGETOKEN 465
#define INTEGERTOKEN 466
#define STRINGTYPETOKEN 467
#define BOOLEANTOKEN 468
#define LISTTOKEN 469
#define OFTOKEN 470
#define VARTOKEN 471
#define PROCTOKEN 472
#define TIMETOKEN 473
#define PROCEDURETOKEN 474
#define RETURNTOKEN 475
#define NOPTOKEN 476




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 88 "miniparser.y"

  doubleNode *dblnode;
  struct entryStruct *association;
  char *value;
  node *tree;
  chain *list;
  int *integerval;
  int count;
  void *other;



/* Line 214 of yacc.c  */
#line 594 "miniparser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 606 "miniparser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  292
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   7711

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  222
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  46
/* YYNRULES -- Number of rules.  */
#define YYNRULES  377
/* YYNRULES -- Number of states.  */
#define YYNSTATES  1043

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   476

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    17,
      19,    23,    28,    32,    35,    38,    43,    46,    50,    56,
      64,    74,    80,    83,    87,    90,    94,    97,    99,   103,
     109,   116,   122,   127,   136,   146,   155,   163,   170,   178,
     185,   191,   201,   212,   222,   231,   240,   250,   259,   267,
     279,   292,   304,   315,   317,   319,   321,   326,   330,   335,
     342,   350,   357,   362,   367,   372,   377,   382,   395,   400,
     407,   415,   422,   427,   432,   439,   447,   460,   467,   479,
     481,   483,   487,   489,   492,   494,   497,   501,   505,   512,
     516,   520,   524,   528,   532,   536,   540,   544,   548,   552,
     556,   560,   564,   568,   572,   576,   580,   584,   588,   592,
     596,   600,   604,   608,   612,   616,   620,   624,   628,   632,
     636,   640,   644,   648,   652,   654,   658,   660,   664,   666,
     668,   673,   675,   680,   682,   686,   690,   693,   698,   700,
     704,   708,   712,   716,   721,   726,   730,   732,   736,   740,
     744,   748,   752,   756,   758,   760,   763,   766,   768,   771,
     774,   778,   782,   787,   792,   797,   802,   804,   808,   813,
     818,   820,   822,   824,   826,   828,   830,   832,   834,   836,
     838,   840,   842,   844,   846,   848,   850,   852,   854,   856,
     858,   860,   862,   864,   866,   868,   870,   872,   874,   876,
     878,   880,   882,   884,   886,   888,   893,   898,   902,   904,
     906,   908,   910,   914,   918,   920,   922,   926,   933,   940,
     943,   948,   950,   953,   963,   970,   979,   985,   994,  1000,
    1008,  1013,  1019,  1021,  1023,  1025,  1027,  1029,  1031,  1033,
    1038,  1042,  1048,  1055,  1057,  1061,  1067,  1073,  1079,  1083,
    1087,  1091,  1095,  1100,  1105,  1110,  1115,  1120,  1127,  1132,
    1141,  1146,  1151,  1162,  1167,  1172,  1177,  1182,  1191,  1200,
    1209,  1214,  1219,  1224,  1231,  1238,  1245,  1252,  1259,  1268,
    1277,  1284,  1289,  1294,  1301,  1314,  1321,  1328,  1335,  1342,
    1349,  1356,  1371,  1376,  1385,  1392,  1399,  1404,  1413,  1420,
    1425,  1430,  1435,  1440,  1445,  1450,  1455,  1460,  1465,  1470,
    1475,  1480,  1487,  1492,  1497,  1502,  1507,  1512,  1517,  1522,
    1527,  1532,  1537,  1542,  1547,  1552,  1557,  1562,  1567,  1572,
    1577,  1582,  1587,  1592,  1597,  1602,  1607,  1612,  1617,  1622,
    1627,  1632,  1637,  1642,  1645,  1646,  1649,  1652,  1655,  1658,
    1661,  1664,  1667,  1670,  1673,  1676,  1679,  1682,  1685,  1688,
    1691,  1693,  1695,  1697,  1699,  1701,  1703,  1707,  1711,  1715,
    1719,  1723,  1727,  1729,  1731,  1733,  1737,  1739
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     223,     0,    -1,   224,    -1,   245,    -1,     1,    -1,   196,
      -1,   198,    -1,   197,    -1,   199,    -1,   235,    -1,   225,
     230,   226,    -1,   225,   231,   230,   226,    -1,   225,   231,
     226,    -1,   225,   226,    -1,   187,   228,    -1,   200,   245,
     195,   227,    -1,   190,   229,    -1,   245,   188,   227,    -1,
     245,   188,   227,   189,   227,    -1,    10,   192,   245,   193,
     245,   195,   227,    -1,    10,   192,   245,   193,   245,   194,
     245,   195,   227,    -1,    10,   191,   245,   195,   227,    -1,
     227,    21,    -1,   227,    21,   230,    -1,   232,    21,    -1,
     232,    21,   231,    -1,   216,   233,    -1,    10,    -1,    10,
      19,   233,    -1,    12,    13,   225,   230,   226,    -1,    12,
      13,   225,   231,   230,   226,    -1,    12,    13,   225,   231,
     226,    -1,    12,    13,   225,   226,    -1,    12,    13,   225,
     230,   220,   245,    21,   226,    -1,    12,    13,   225,   231,
     230,   220,   245,    21,   226,    -1,    12,    13,   225,   231,
     220,   245,    21,   226,    -1,    12,    13,   225,   220,   245,
      21,   226,    -1,    12,   233,    13,   225,   230,   226,    -1,
      12,   233,    13,   225,   231,   230,   226,    -1,    12,   233,
      13,   225,   231,   226,    -1,    12,   233,    13,   225,   226,
      -1,    12,   233,    13,   225,   230,   220,   245,    21,   226,
      -1,    12,   233,    13,   225,   231,   230,   220,   245,    21,
     226,    -1,    12,   233,    13,   225,   231,   220,   245,    21,
     226,    -1,    12,   233,    13,   225,   220,   245,    21,   226,
      -1,    12,    10,    16,    28,    13,   225,   230,   226,    -1,
      12,    10,    16,    28,    13,   225,   231,   230,   226,    -1,
      12,    10,    16,    28,    13,   225,   231,   226,    -1,    12,
      10,    16,    28,    13,   225,   226,    -1,    12,    10,    16,
      28,    13,   225,   230,   220,   245,    21,   226,    -1,    12,
      10,    16,    28,    13,   225,   231,   230,   220,   245,    21,
     226,    -1,    12,    10,    16,    28,    13,   225,   231,   220,
     245,    21,   226,    -1,    12,    10,    16,    28,    13,   225,
     220,   245,    21,   226,    -1,   205,    -1,   204,    -1,   221,
      -1,   221,    12,   245,    13,    -1,   221,    12,    13,    -1,
     158,    12,   241,    13,    -1,   158,    12,   241,    13,    27,
     245,    -1,   158,    12,   241,    13,    27,    27,   245,    -1,
     160,    12,   245,    19,   241,    13,    -1,   161,    12,   245,
      13,    -1,   162,    12,   245,    13,    -1,   163,    12,   245,
      13,    -1,   164,    12,   245,    13,    -1,   165,    12,   245,
      13,    -1,   166,    12,   245,    19,   245,    19,   245,    19,
     245,    19,   241,    13,    -1,   167,    12,   241,    13,    -1,
     167,    12,   241,    13,    27,   245,    -1,   167,    12,   241,
      13,    27,    27,   245,    -1,   168,    12,   245,    19,   245,
      13,    -1,   159,    12,   245,    13,    -1,   203,    12,   245,
      13,    -1,   159,    12,   245,    13,    27,   245,    -1,   159,
      12,   245,    13,    27,    27,   245,    -1,   178,    12,   245,
      19,   245,    19,   245,    19,   245,    19,   241,    13,    -1,
     169,    12,    10,    19,    10,    13,    -1,   206,    12,    10,
      19,   245,    19,   267,    42,    27,   265,    13,    -1,   236,
      -1,   241,    -1,   219,    10,   234,    -1,   239,    -1,   240,
      20,    -1,   237,    -1,   237,    20,    -1,    10,    16,   245,
      -1,    10,    17,   245,    -1,    10,    16,   127,    12,   245,
      13,    -1,   247,    16,   245,    -1,   247,    17,   245,    -1,
     238,    16,   245,    -1,   238,    17,   245,    -1,   253,    29,
      10,    -1,    86,    16,   245,    -1,    87,    16,   245,    -1,
      88,    16,   245,    -1,    89,    16,   245,    -1,    90,    16,
     245,    -1,    91,    16,   245,    -1,    92,    16,   245,    -1,
      93,    16,   245,    -1,    94,    16,   245,    -1,    95,    16,
     245,    -1,    96,    16,   245,    -1,    97,    16,   245,    -1,
     100,    16,   245,    -1,    98,    16,   245,    -1,    99,    16,
     245,    -1,    86,    16,   245,    -1,    87,    16,   245,    -1,
      88,    16,   245,    -1,    89,    16,   245,    -1,    90,    16,
     245,    -1,    91,    16,   245,    -1,    92,    16,   245,    -1,
      93,    16,   245,    -1,    94,    16,   245,    -1,    95,    16,
     245,    -1,    96,    16,   245,    -1,    97,    16,   245,    -1,
     100,    16,   245,    -1,    98,    16,   245,    -1,    99,    16,
     245,    -1,   245,    -1,   245,    19,   241,    -1,   244,    -1,
     244,   243,   242,    -1,    19,    -1,    21,    -1,    29,    10,
      16,   245,    -1,   246,    -1,   119,   246,   120,   254,    -1,
     248,    -1,   245,    39,   248,    -1,   245,    40,   248,    -1,
      20,   248,    -1,   253,    14,   245,    15,    -1,   249,    -1,
     248,    18,   249,    -1,   248,   191,   249,    -1,   248,    23,
     249,    -1,   248,    27,   249,    -1,   248,    23,    16,   249,
      -1,   248,    27,    16,   249,    -1,   248,    37,   249,    -1,
     251,    -1,   249,    41,   251,    -1,   249,    42,   251,    -1,
     249,    32,   251,    -1,   249,    33,   251,    -1,   249,    35,
     251,    -1,   249,    36,   251,    -1,    41,    -1,    42,    -1,
      41,   250,    -1,    42,   250,    -1,   252,    -1,   250,   252,
      -1,    38,   252,    -1,   251,    43,   252,    -1,   251,    44,
     252,    -1,   251,    43,   250,   252,    -1,   251,    44,   250,
     252,    -1,   251,    43,    38,   252,    -1,   251,    44,    38,
     252,    -1,   253,    -1,   253,    45,   252,    -1,   253,    45,
     250,   252,    -1,   253,    45,    38,   252,    -1,   101,    -1,
     102,    -1,   103,    -1,   104,    -1,   105,    -1,   106,    -1,
     107,    -1,   108,    -1,   109,    -1,   110,    -1,   111,    -1,
     112,    -1,   113,    -1,   114,    -1,   115,    -1,   116,    -1,
     207,    -1,   117,    -1,   118,    -1,   122,    -1,   121,    -1,
     123,    -1,   124,    -1,   125,    -1,   126,    -1,    68,    -1,
      69,    -1,    70,    -1,    71,    -1,    74,    -1,    72,    -1,
      73,    -1,    11,    -1,   256,    -1,    10,    -1,   202,    12,
      10,    13,    -1,    10,    12,   241,    13,    -1,    10,    12,
      13,    -1,   257,    -1,   259,    -1,   260,    -1,   261,    -1,
      12,   245,    13,    -1,   198,   242,   199,    -1,   263,    -1,
     247,    -1,   253,    29,    10,    -1,   253,    29,    10,    12,
     241,    13,    -1,    12,   245,    13,    12,   241,    13,    -1,
     217,   234,    -1,   218,    12,   227,    13,    -1,   255,    -1,
     255,   254,    -1,   245,    34,   225,   231,   230,   220,   245,
      21,   226,    -1,   245,    34,   225,   231,   230,   226,    -1,
     245,    34,   225,   231,   220,   245,    21,   226,    -1,   245,
      34,   225,   231,   226,    -1,   245,    34,   225,   230,   220,
     245,    21,   226,    -1,   245,    34,   225,   230,   226,    -1,
     245,    34,   225,   220,   245,    21,   226,    -1,   245,    34,
     225,   226,    -1,   245,    34,    12,   245,    13,    -1,     3,
      -1,     4,    -1,     5,    -1,     6,    -1,     7,    -1,     8,
      -1,     9,    -1,    14,    31,    31,    15,    -1,    14,    40,
      15,    -1,    14,    31,   258,    31,    15,    -1,    14,    31,
     258,    28,    31,    15,    -1,   245,    -1,   258,    19,   245,
      -1,   258,    19,    28,    19,   245,    -1,    14,   245,    19,
     245,    15,    -1,    14,   245,    21,   245,    15,    -1,    14,
     245,    15,    -1,    22,   245,    26,    -1,    22,   245,    25,
      -1,    22,   245,    24,    -1,   153,    12,   245,    13,    -1,
     152,    12,   245,    13,    -1,   151,    12,   245,    13,    -1,
     128,    12,   245,    13,    -1,   129,    12,   245,    13,    -1,
     129,    12,   245,    19,   245,    13,    -1,   130,    12,   245,
      13,    -1,   131,    12,   245,    19,   245,    19,   241,    13,
      -1,   154,    12,   241,    13,    -1,   155,    12,   241,    13,
      -1,   132,    12,   245,    19,   245,    19,   245,    19,   241,
      13,    -1,   133,    12,   245,    13,    -1,    91,    12,   245,
      13,    -1,   134,    12,   245,    13,    -1,   135,    12,   245,
      13,    -1,   136,    12,   245,    19,   245,    19,   245,    13,
      -1,   137,    12,   245,    19,   245,    19,   241,    13,    -1,
     138,    12,   245,    19,   245,    19,   245,    13,    -1,   139,
      12,   245,    13,    -1,   140,    12,   245,    13,    -1,   141,
      12,   245,    13,    -1,   142,    12,   245,    19,   245,    13,
      -1,   143,    12,   245,    19,   245,    13,    -1,   144,    12,
     245,    19,   245,    13,    -1,   145,    12,   245,    19,   245,
      13,    -1,   146,    12,   245,    19,   245,    13,    -1,   147,
      12,   245,    19,   245,    19,   241,    13,    -1,   148,    12,
     245,    19,   245,    19,   245,    13,    -1,   149,    12,   245,
      19,   245,    13,    -1,   157,    12,   245,    13,    -1,   156,
      12,   245,    13,    -1,   170,    12,   245,    19,   241,    13,
      -1,   171,    12,   245,    19,   245,    19,   245,    19,   245,
      19,   245,    13,    -1,   172,    12,   245,    19,   245,    13,
      -1,   173,    12,   245,    19,   245,    13,    -1,   174,    12,
     245,    19,   245,    13,    -1,   175,    12,   245,    19,   245,
      13,    -1,   176,    12,   245,    19,   245,    13,    -1,   177,
      12,   245,    19,   245,    13,    -1,   179,    12,   245,    19,
     245,    19,   245,    19,   245,    19,   245,    19,   241,    13,
      -1,   180,    12,   241,    13,    -1,   181,    12,   245,    19,
     245,    19,   245,    13,    -1,   182,    12,   245,    19,   245,
      13,    -1,   183,    12,   245,    19,   245,    13,    -1,   184,
      12,   241,    13,    -1,   185,    12,   245,    19,   245,    19,
     241,    13,    -1,   186,    12,   245,    19,   245,    13,    -1,
      78,    12,   245,    13,    -1,    85,    12,   245,    13,    -1,
     201,    12,   245,    13,    -1,    79,    12,   245,    13,    -1,
      80,    12,   245,    13,    -1,    82,    12,   245,    13,    -1,
      83,    12,   245,    13,    -1,    84,    12,   245,    13,    -1,
      81,    12,   245,    13,    -1,    46,    12,   245,    13,    -1,
      47,    12,   245,    13,    -1,   209,    12,   245,    13,    -1,
     209,    12,   245,    19,   245,    13,    -1,    48,    12,   245,
      13,    -1,    49,    12,   245,    13,    -1,    50,    12,   245,
      13,    -1,    51,    12,   245,    13,    -1,    52,    12,   245,
      13,    -1,    53,    12,   245,    13,    -1,    54,    12,   245,
      13,    -1,    55,    12,   245,    13,    -1,    56,    12,   245,
      13,    -1,    57,    12,   245,    13,    -1,    58,    12,   245,
      13,    -1,    59,    12,   245,    13,    -1,    60,    12,   245,
      13,    -1,    61,    12,   245,    13,    -1,    62,    12,   245,
      13,    -1,    63,    12,   245,    13,    -1,    64,    12,   245,
      13,    -1,    65,    12,   245,    13,    -1,    66,    12,   245,
      13,    -1,    67,    12,   245,    13,    -1,    68,    12,   245,
      13,    -1,    69,    12,   245,    13,    -1,    70,    12,   245,
      13,    -1,    71,    12,   245,    13,    -1,    72,    12,   245,
      13,    -1,    73,    12,   245,    13,    -1,    74,    12,   245,
      13,    -1,    75,    12,   245,    13,    -1,    76,    12,   245,
      13,    -1,    77,    12,   245,    13,    -1,   150,    12,   245,
      13,    -1,    16,    30,    -1,    -1,    86,   262,    -1,    87,
     262,    -1,    88,   262,    -1,    89,   262,    -1,    90,   262,
      -1,    91,   262,    -1,    92,   262,    -1,    93,   262,    -1,
      94,   262,    -1,    95,   262,    -1,    96,   262,    -1,    97,
     262,    -1,   100,   262,    -1,    98,   262,    -1,    99,   262,
      -1,   208,    -1,   209,    -1,   210,    -1,   211,    -1,   212,
      -1,   213,    -1,   214,   215,   208,    -1,   214,   215,   209,
      -1,   214,   215,   210,    -1,   214,   215,   211,    -1,   214,
     215,   212,    -1,   214,   215,   213,    -1,   207,    -1,   264,
      -1,   264,    -1,   264,    19,   266,    -1,   265,    -1,    12,
     266,    13,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   393,   393,   402,   406,   412,   416,   422,   426,   432,
     436,   440,   444,   448,   452,   456,   460,   466,   470,   478,
     483,   488,   496,   500,   506,   510,   516,   523,   527,   533,
     537,   541,   545,   549,   553,   557,   561,   565,   569,   573,
     577,   581,   585,   589,   593,   597,   601,   605,   609,   613,
     617,   621,   625,   633,   637,   641,   645,   649,   653,   657,
     661,   665,   669,   673,   677,   681,   685,   689,   693,   697,
     701,   705,   709,   713,   717,   721,   725,   729,   735,   740,
     744,   748,   755,   759,   763,   767,   773,   778,   783,   788,
     793,   798,   802,   808,   815,   819,   823,   827,   831,   835,
     839,   843,   847,   851,   855,   859,   863,   867,   871,   877,
     881,   885,   889,   893,   897,   901,   905,   909,   913,   917,
     921,   925,   929,   933,   939,   943,   949,   953,   959,   963,
     969,   979,   983,   989,   993,   997,  1001,  1007,  1016,  1020,
    1024,  1028,  1032,  1036,  1040,  1044,  1050,  1054,  1058,  1062,
    1066,  1070,  1074,  1080,  1084,  1088,  1092,  1099,  1103,  1110,
    1114,  1118,  1122,  1129,  1136,  1140,  1146,  1150,  1154,  1161,
    1168,  1172,  1176,  1180,  1184,  1188,  1192,  1196,  1200,  1204,
    1208,  1212,  1216,  1220,  1224,  1228,  1232,  1236,  1240,  1244,
    1248,  1252,  1256,  1260,  1264,  1268,  1272,  1276,  1280,  1284,
    1288,  1292,  1296,  1307,  1311,  1316,  1321,  1326,  1331,  1335,
    1339,  1343,  1347,  1351,  1355,  1359,  1364,  1369,  1374,  1378,
    1382,  1388,  1392,  1398,  1402,  1406,  1410,  1414,  1418,  1422,
    1426,  1430,  1436,  1441,  1446,  1450,  1454,  1458,  1462,  1470,
    1474,  1478,  1482,  1488,  1492,  1496,  1502,  1506,  1510,  1516,
    1520,  1524,  1528,  1532,  1536,  1542,  1546,  1550,  1554,  1558,
    1562,  1566,  1570,  1574,  1578,  1582,  1586,  1590,  1594,  1598,
    1602,  1606,  1610,  1614,  1618,  1622,  1626,  1630,  1634,  1638,
    1642,  1646,  1650,  1654,  1658,  1662,  1666,  1670,  1674,  1678,
    1682,  1686,  1690,  1694,  1698,  1702,  1706,  1710,  1714,  1718,
    1722,  1726,  1730,  1734,  1738,  1742,  1746,  1750,  1754,  1758,
    1762,  1766,  1770,  1774,  1778,  1782,  1786,  1790,  1794,  1798,
    1802,  1806,  1810,  1814,  1818,  1822,  1826,  1830,  1834,  1838,
    1842,  1846,  1850,  1854,  1858,  1862,  1866,  1870,  1874,  1878,
    1882,  1886,  1890,  1896,  1901,  1907,  1911,  1915,  1919,  1923,
    1927,  1931,  1935,  1939,  1943,  1947,  1951,  1955,  1959,  1963,
    1970,  1976,  1982,  1988,  1994,  2000,  2006,  2012,  2018,  2024,
    2030,  2036,  2044,  2050,  2057,  2061,  2067,  2071
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CONSTANTTOKEN", "MIDPOINTCONSTANTTOKEN",
  "DYADICCONSTANTTOKEN", "HEXCONSTANTTOKEN", "HEXADECIMALCONSTANTTOKEN",
  "BINARYCONSTANTTOKEN", "PITOKEN", "IDENTIFIERTOKEN", "STRINGTOKEN",
  "LPARTOKEN", "RPARTOKEN", "LBRACKETTOKEN", "RBRACKETTOKEN", "EQUALTOKEN",
  "ASSIGNEQUALTOKEN", "COMPAREEQUALTOKEN", "COMMATOKEN",
  "EXCLAMATIONTOKEN", "SEMICOLONTOKEN", "STARLEFTANGLETOKEN",
  "LEFTANGLETOKEN", "RIGHTANGLEUNDERSCORETOKEN", "RIGHTANGLEDOTTOKEN",
  "RIGHTANGLESTARTOKEN", "RIGHTANGLETOKEN", "DOTSTOKEN", "DOTTOKEN",
  "QUESTIONMARKTOKEN", "VERTBARTOKEN", "ATTOKEN", "DOUBLECOLONTOKEN",
  "COLONTOKEN", "DOTCOLONTOKEN", "COLONDOTTOKEN", "EXCLAMATIONEQUALTOKEN",
  "APPROXTOKEN", "ANDTOKEN", "ORTOKEN", "PLUSTOKEN", "MINUSTOKEN",
  "MULTOKEN", "DIVTOKEN", "POWTOKEN", "SQRTTOKEN", "EXPTOKEN", "LOGTOKEN",
  "LOG2TOKEN", "LOG10TOKEN", "SINTOKEN", "COSTOKEN", "TANTOKEN",
  "ASINTOKEN", "ACOSTOKEN", "ATANTOKEN", "SINHTOKEN", "COSHTOKEN",
  "TANHTOKEN", "ASINHTOKEN", "ACOSHTOKEN", "ATANHTOKEN", "ABSTOKEN",
  "ERFTOKEN", "ERFCTOKEN", "LOG1PTOKEN", "EXPM1TOKEN", "DOUBLETOKEN",
  "SINGLETOKEN", "QUADTOKEN", "HALFPRECISIONTOKEN", "DOUBLEDOUBLETOKEN",
  "TRIPLEDOUBLETOKEN", "DOUBLEEXTENDEDTOKEN", "CEILTOKEN", "FLOORTOKEN",
  "NEARESTINTTOKEN", "HEADTOKEN", "REVERTTOKEN", "SORTTOKEN", "TAILTOKEN",
  "MANTISSATOKEN", "EXPONENTTOKEN", "PRECISIONTOKEN",
  "ROUNDCORRECTLYTOKEN", "PRECTOKEN", "POINTSTOKEN", "DIAMTOKEN",
  "DISPLAYTOKEN", "VERBOSITYTOKEN", "CANONICALTOKEN", "AUTOSIMPLIFYTOKEN",
  "TAYLORRECURSIONSTOKEN", "TIMINGTOKEN", "FULLPARENTHESESTOKEN",
  "MIDPOINTMODETOKEN", "DIEONERRORMODETOKEN", "SUPPRESSWARNINGSTOKEN",
  "HOPITALRECURSIONSTOKEN", "RATIONALMODETOKEN", "ONTOKEN", "OFFTOKEN",
  "DYADICTOKEN", "POWERSTOKEN", "BINARYTOKEN", "HEXADECIMALTOKEN",
  "FILETOKEN", "POSTSCRIPTTOKEN", "POSTSCRIPTFILETOKEN", "PERTURBTOKEN",
  "MINUSWORDTOKEN", "PLUSWORDTOKEN", "ZEROWORDTOKEN", "NEARESTTOKEN",
  "HONORCOEFFPRECTOKEN", "TRUETOKEN", "FALSETOKEN", "DEFAULTTOKEN",
  "MATCHTOKEN", "WITHTOKEN", "ABSOLUTETOKEN", "DECIMALTOKEN",
  "RELATIVETOKEN", "FIXEDTOKEN", "FLOATINGTOKEN", "ERRORTOKEN",
  "LIBRARYTOKEN", "DIFFTOKEN", "BASHEVALUATETOKEN", "SIMPLIFYTOKEN",
  "REMEZTOKEN", "FPMINIMAXTOKEN", "HORNERTOKEN", "EXPANDTOKEN",
  "SIMPLIFYSAFETOKEN", "TAYLORTOKEN", "TAYLORFORMTOKEN", "AUTODIFFTOKEN",
  "DEGREETOKEN", "NUMERATORTOKEN", "DENOMINATORTOKEN", "SUBSTITUTETOKEN",
  "COEFFTOKEN", "SUBPOLYTOKEN", "ROUNDCOEFFICIENTSTOKEN",
  "RATIONALAPPROXTOKEN", "ACCURATEINFNORMTOKEN", "ROUNDTOFORMATTOKEN",
  "EVALUATETOKEN", "LENGTHTOKEN", "INFTOKEN", "MIDTOKEN", "SUPTOKEN",
  "MINTOKEN", "MAXTOKEN", "READXMLTOKEN", "PARSETOKEN", "PRINTTOKEN",
  "PRINTXMLTOKEN", "PLOTTOKEN", "PRINTHEXATOKEN", "PRINTFLOATTOKEN",
  "PRINTBINARYTOKEN", "PRINTEXPANSIONTOKEN", "BASHEXECUTETOKEN",
  "EXTERNALPLOTTOKEN", "WRITETOKEN", "ASCIIPLOTTOKEN", "RENAMETOKEN",
  "INFNORMTOKEN", "SUPNORMTOKEN", "FINDZEROSTOKEN", "FPFINDZEROSTOKEN",
  "DIRTYINFNORMTOKEN", "NUMBERROOTSTOKEN", "INTEGRALTOKEN",
  "DIRTYINTEGRALTOKEN", "WORSTCASETOKEN", "IMPLEMENTPOLYTOKEN",
  "IMPLEMENTCONSTTOKEN", "CHECKINFNORMTOKEN", "ZERODENOMINATORSTOKEN",
  "ISEVALUABLETOKEN", "SEARCHGALTOKEN", "GUESSDEGREETOKEN",
  "DIRTYFINDZEROSTOKEN", "IFTOKEN", "THENTOKEN", "ELSETOKEN", "FORTOKEN",
  "INTOKEN", "FROMTOKEN", "TOTOKEN", "BYTOKEN", "DOTOKEN", "BEGINTOKEN",
  "ENDTOKEN", "LEFTCURLYBRACETOKEN", "RIGHTCURLYBRACETOKEN", "WHILETOKEN",
  "READFILETOKEN", "ISBOUNDTOKEN", "EXECUTETOKEN", "FALSERESTARTTOKEN",
  "FALSEQUITTOKEN", "EXTERNALPROCTOKEN", "VOIDTOKEN", "CONSTANTTYPETOKEN",
  "FUNCTIONTOKEN", "RANGETOKEN", "INTEGERTOKEN", "STRINGTYPETOKEN",
  "BOOLEANTOKEN", "LISTTOKEN", "OFTOKEN", "VARTOKEN", "PROCTOKEN",
  "TIMETOKEN", "PROCEDURETOKEN", "RETURNTOKEN", "NOPTOKEN", "$accept",
  "startsymbol", "startsymbolwitherr", "beginsymbol", "endsymbol",
  "command", "ifcommand", "forcommand", "commandlist",
  "variabledeclarationlist", "variabledeclaration", "identifierlist",
  "procbody", "simplecommand", "assignment", "simpleassignment",
  "structuring", "stateassignment", "stillstateassignment", "thinglist",
  "structelementlist", "structelementseparator", "structelement", "thing",
  "supermegaterm", "indexing", "megaterm", "hyperterm", "unaryplusminus",
  "term", "subterm", "basicthing", "matchlist", "matchelement", "constant",
  "list", "simplelist", "range", "debound", "headfunction",
  "egalquestionmark", "statedereference", "externalproctype",
  "extendedexternalproctype", "externalproctypesimplelist",
  "externalproctypelist", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,   466,   467,   468,   469,   470,   471,   472,   473,   474,
     475,   476
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   222,   223,   224,   224,   225,   225,   226,   226,   227,
     227,   227,   227,   227,   227,   227,   227,   228,   228,   229,
     229,   229,   230,   230,   231,   231,   232,   233,   233,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   234,   234,   234,   234,   234,   234,   234,
     234,   234,   234,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   235,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   235,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   236,   236,   236,   236,   237,   237,   237,   237,
     237,   237,   237,   238,   239,   239,   239,   239,   239,   239,
     239,   239,   239,   239,   239,   239,   239,   239,   239,   240,
     240,   240,   240,   240,   240,   240,   240,   240,   240,   240,
     240,   240,   240,   240,   241,   241,   242,   242,   243,   243,
     244,   245,   245,   246,   246,   246,   246,   247,   248,   248,
     248,   248,   248,   248,   248,   248,   249,   249,   249,   249,
     249,   249,   249,   250,   250,   250,   250,   251,   251,   251,
     251,   251,   251,   251,   251,   251,   252,   252,   252,   252,
     253,   253,   253,   253,   253,   253,   253,   253,   253,   253,
     253,   253,   253,   253,   253,   253,   253,   253,   253,   253,
     253,   253,   253,   253,   253,   253,   253,   253,   253,   253,
     253,   253,   253,   253,   253,   253,   253,   253,   253,   253,
     253,   253,   253,   253,   253,   253,   253,   253,   253,   253,
     253,   254,   254,   255,   255,   255,   255,   255,   255,   255,
     255,   255,   256,   256,   256,   256,   256,   256,   256,   257,
     257,   257,   257,   258,   258,   258,   259,   259,   259,   260,
     260,   260,   260,   260,   260,   261,   261,   261,   261,   261,
     261,   261,   261,   261,   261,   261,   261,   261,   261,   261,
     261,   261,   261,   261,   261,   261,   261,   261,   261,   261,
     261,   261,   261,   261,   261,   261,   261,   261,   261,   261,
     261,   261,   261,   261,   261,   261,   261,   261,   261,   261,
     261,   261,   261,   261,   261,   261,   261,   261,   261,   261,
     261,   261,   261,   261,   261,   261,   261,   261,   261,   261,
     261,   261,   261,   261,   261,   261,   261,   261,   261,   261,
     261,   261,   261,   261,   261,   261,   261,   261,   261,   261,
     261,   261,   261,   262,   262,   263,   263,   263,   263,   263,
     263,   263,   263,   263,   263,   263,   263,   263,   263,   263,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   265,   265,   266,   266,   267,   267
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     4,     3,     2,     2,     4,     2,     3,     5,     7,
       9,     5,     2,     3,     2,     3,     2,     1,     3,     5,
       6,     5,     4,     8,     9,     8,     7,     6,     7,     6,
       5,     9,    10,     9,     8,     8,     9,     8,     7,    11,
      12,    11,    10,     1,     1,     1,     4,     3,     4,     6,
       7,     6,     4,     4,     4,     4,     4,    12,     4,     6,
       7,     6,     4,     4,     6,     7,    12,     6,    11,     1,
       1,     3,     1,     2,     1,     2,     3,     3,     6,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     1,     3,     1,     3,     1,     1,
       4,     1,     4,     1,     3,     3,     2,     4,     1,     3,
       3,     3,     3,     4,     4,     3,     1,     3,     3,     3,
       3,     3,     3,     1,     1,     2,     2,     1,     2,     2,
       3,     3,     4,     4,     4,     4,     1,     3,     4,     4,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     4,     4,     3,     1,     1,
       1,     1,     3,     3,     1,     1,     3,     6,     6,     2,
       4,     1,     2,     9,     6,     8,     5,     8,     5,     7,
       4,     5,     1,     1,     1,     1,     1,     1,     1,     4,
       3,     5,     6,     1,     3,     5,     5,     5,     3,     3,
       3,     3,     4,     4,     4,     4,     4,     6,     4,     8,
       4,     4,    10,     4,     4,     4,     4,     8,     8,     8,
       4,     4,     4,     6,     6,     6,     6,     6,     8,     8,
       6,     4,     4,     6,    12,     6,     6,     6,     6,     6,
       6,    14,     4,     8,     6,     6,     4,     8,     6,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     6,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     2,     0,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       1,     1,     1,     1,     1,     1,     3,     3,     3,     3,
       3,     3,     1,     1,     1,     3,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     4,   232,   233,   234,   235,   236,   237,   238,   204,
     202,     0,     0,     0,     0,     0,   153,   154,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     195,   196,   197,   198,   200,   201,   199,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   344,   344,
     344,   344,   344,   344,   344,   344,   344,   344,   344,   344,
     344,   344,   344,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   187,
     188,     0,   190,   189,   191,   192,   193,   194,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   186,     0,     0,
       0,     0,     2,     3,   131,   215,   133,   138,     0,   146,
     157,   166,   203,   208,   209,   210,   211,   214,     0,     0,
       0,     0,     0,   136,     0,   159,   155,   156,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   345,
     346,   347,   348,   349,     0,   350,   351,   352,   353,   354,
     355,   356,   358,   359,   357,     0,   131,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   126,     0,     0,     0,     0,
     219,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   158,     0,     0,     0,
       0,     0,   207,     0,   124,   212,     0,   243,     0,   240,
     248,     0,     0,   251,   250,   249,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   343,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   213,   128,   129,     0,
       0,     0,     0,    27,     0,     0,   204,   344,   344,   344,
     344,   344,   344,   344,   344,   344,   344,   344,   344,   344,
     344,   344,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     5,     6,     0,
       0,    54,    53,     0,     0,    55,     0,     0,     9,    79,
      84,     0,    82,     0,    80,   215,   166,   134,   135,   139,
       0,   141,     0,   142,   145,   140,   149,   150,   151,   152,
     147,   148,     0,     0,   160,     0,     0,   161,     0,   216,
       0,     0,   167,   206,     0,     0,   239,     0,     0,     0,
       0,     0,   308,   309,   312,   313,   314,   315,   316,   317,
     318,   319,   320,   321,   322,   323,   324,   325,   326,   327,
     328,   329,   330,   331,   332,   333,   334,   335,   336,   337,
     338,   339,   340,   341,   299,   302,   303,   307,   304,   305,
     306,   300,   264,     0,   132,   221,   255,   256,     0,   258,
       0,     0,   263,   265,   266,     0,     0,     0,   270,   271,
     272,     0,     0,     0,     0,     0,     0,     0,     0,   342,
     254,   253,   252,   260,   261,   282,   281,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   292,     0,     0,     0,
     296,     0,     0,     0,   127,   301,   205,   310,     0,     0,
       0,     6,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    14,     0,     0,    16,     0,     0,
       0,     0,     0,     7,     8,     0,    13,     0,     0,     0,
       0,   220,    85,     0,     0,    83,     0,     0,     0,   143,
     144,   164,   162,   165,   163,   137,     0,   169,   168,   125,
       0,     0,   244,     0,   241,   246,   247,     0,   222,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   130,     0,     0,
      27,    28,     0,    32,     0,     0,     0,     0,    86,    87,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   107,   108,   106,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    81,    57,     0,    26,    22,    10,
      12,     0,    24,    91,    92,    89,    90,   216,     0,   218,
       0,   242,     0,     0,   257,     0,     0,     0,     0,     0,
     273,   274,   275,   276,   277,     0,     0,   280,   283,     0,
     285,   286,   287,   288,   289,   290,     0,     0,   294,   295,
       0,   298,   311,     0,     0,     0,    29,     0,    31,     0,
       0,    40,     0,     0,     0,    58,    72,     0,    62,    63,
      64,    65,    66,     0,    68,     0,     0,     0,    17,     0,
       0,    15,    73,     0,    56,    23,    11,    25,   217,   245,
       0,     0,   230,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    30,     0,     0,    37,     0,    39,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   231,     0,     0,   228,     0,   226,     0,   259,     0,
     267,   268,   269,   278,   279,     0,     0,   293,   297,     0,
      48,     0,     0,    36,     0,     0,     0,     0,     0,     0,
       0,    38,    88,     0,    59,     0,    74,    61,     0,     0,
      69,    71,    77,     0,    18,    21,     0,     0,     0,     0,
       0,     0,   224,     0,     0,     0,     0,     0,    45,     0,
      47,     0,    33,    35,     0,    44,     0,     0,     0,    60,
      75,     0,    70,     0,     0,     0,     0,   372,   360,   361,
     362,   363,   364,   365,     0,   373,   376,     0,   229,     0,
       0,     0,   262,     0,     0,     0,     0,     0,     0,    46,
      34,    41,    43,     0,     0,     0,     0,    19,   374,     0,
       0,     0,   227,   225,     0,     0,     0,    52,     0,     0,
       0,    42,     0,     0,     0,     0,   377,   366,   367,   368,
     369,   370,   371,     0,   223,   284,     0,    49,    51,     0,
       0,     0,    20,   375,     0,     0,    50,     0,     0,    78,
     291,    67,    76
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,   151,   152,   466,   656,   657,   644,   647,   658,   659,
     660,   425,   290,   468,   469,   470,   471,   472,   473,   474,
     284,   419,   285,   314,   154,   155,   156,   157,   158,   159,
     160,   161,   554,   555,   162,   163,   318,   164,   165,   166,
     219,   167,   975,   976,   999,   977
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -966
static const yytype_int16 yypact[] =
{
    3605,  -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,    -5,
    -966,  5981,  3821,  6629,  5981,  7493,   116,   116,    28,    42,
      57,   162,   165,   211,   252,   268,   275,   278,   306,   312,
     318,   340,   345,   351,   357,   364,   392,   399,   416,   425,
     469,   474,   515,   529,   545,   556,   567,   584,   588,   601,
     628,   638,   643,   648,   694,   700,   712,   717,   111,   111,
     111,   111,   111,    35,   111,   111,   111,   111,   111,   111,
     111,   111,   111,  -966,  -966,  -966,  -966,  -966,  -966,  -966,
    -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,
    -966,  5981,  -966,  -966,  -966,  -966,  -966,  -966,   720,   722,
     735,   749,   750,   756,   780,   785,   804,   811,   818,   819,
     827,   828,   830,   833,   860,   870,   882,   887,   888,   904,
     907,   909,   914,   941,   953,   963,   968,   970,   975,   977,
     981,   992,  1003,  1014,  1019,  1021,  1026,  1028,  1032,  1045,
    1050,  1055,  1060,  1062,   393,  1067,  1068,  -966,  1072,  1095,
    1097,   339,  -966,   196,  -966,  -966,    30,  1152,  7493,   426,
    -966,    16,  -966,  -966,  -966,  -966,  -966,  -966,  4037,    43,
    4253,   330,   724,    30,  1187,  -966,  -966,  -966,  5981,  5981,
    5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,
    5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,
    5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,
    5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,   303,  -966,
    -966,  -966,  -966,  -966,  5981,  -966,  -966,  -966,  -966,  -966,
    -966,  -966,  -966,  -966,  -966,   196,   279,  5981,  5981,  5981,
    5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,
    5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,
    5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,
    5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,
    5981,  5981,  5981,   457,   217,    88,  5981,   510,  5981,    89,
    -966,  3386,  -966,  6629,  6629,  6629,  6197,  6413,  6629,  6629,
    6629,  6629,  6629,  6629,  6629,  6629,  -966,  6845,  7061,  5981,
     554,  7277,  -966,   420,   132,  1103,   450,   196,    24,  -966,
    -966,  5981,  5981,  -966,  -966,  -966,    50,    55,    61,    64,
      75,    79,    83,    85,    92,    95,    97,    99,   104,   108,
     113,   115,   120,   136,   272,   276,   280,   286,   297,   301,
     304,   310,   314,   316,   319,   321,   325,   333,   338,   362,
     366,   368,   370,   385,   387,   390,  -966,   400,  5981,   402,
      45,   404,   530,   734,   406,   408,   410,   737,   739,   764,
     418,   421,   423,   769,   771,   774,   779,   782,   809,   813,
     815,   451,   453,   455,   459,   458,   460,   461,   463,   817,
     822,   825,   847,   862,   896,   899,   903,   905,   471,   911,
     915,   921,   492,   927,   929,   509,  -966,  -966,  -966,   393,
     472,   632,    47,   147,   -85,   634,   946,   642,   741,   765,
     775,   778,    81,   858,   895,   901,   958,  1004,  1090,  1102,
    1109,  1114,  1105,  1138,  1140,  1142,  1143,  1145,  1147,  1151,
    1154,  1168,  1170,  1171,  1174,  5981,   853,  -966,   393,  5981,
    1177,  -966,  -966,  1178,   866,  1179,  2072,   697,  -966,  -966,
     805,   590,  -966,  1063,  -966,   593,    17,    30,    30,  1152,
    6629,  1152,  6629,  1152,  1152,  1152,   426,   426,   426,   426,
     426,   426,  7493,  7493,  -966,  7493,  7493,  -966,   101,  1183,
    7493,  7493,  -966,  -966,  5981,  5981,  -966,  4469,   798,   912,
     327,   572,  -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,
    -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,
    -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,
    -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,  -966,
    -966,  -966,  -966,   122,  -966,  5981,  -966,  -966,  5981,  -966,
    5981,  5981,  -966,  -966,  -966,  5981,  5981,  5981,  -966,  -966,
    -966,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  -966,
    -966,  -966,  -966,  -966,  -966,  -966,  -966,  5981,  5981,  5981,
    5981,  5981,  5981,  5981,  5981,  5981,  -966,  5981,  5981,  5981,
    -966,  5981,  5981,  5981,  -966,  -966,  -966,  -966,  5981,  1085,
    1188,  -966,  1196,   -85,  4685,  5981,  4901,  4901,  4901,  4901,
    4901,  4901,  4901,  4901,  4901,  4901,  4901,  4901,  4901,  4901,
    4901,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,  5981,
    5981,  5981,  1199,  5981,  -966,    41,   527,  -966,    31,  5981,
    1204,  1095,  5117,  -966,  -966,  1188,  -966,  1128,   183,  3167,
    1194,  -966,  -966,  5981,  5981,  -966,  5981,  5981,  1209,  1152,
    1152,  -966,  -966,  -966,  -966,  -966,  5981,  -966,  -966,  -966,
    1222,  1217,   196,  1224,  -966,  -966,  -966,    29,  -966,   476,
     933,   938,   951,   957,   960,   483,   491,   495,   500,   504,
     962,   966,   506,  1227,   973,   508,   511,   513,   523,   541,
     543,   979,   984,   546,   629,   989,   631,   196,   633,  1228,
    1297,  -966,  5981,  -966,  -155,  2291,  1415,  1311,   196,   196,
      -7,   308,   412,   489,   498,   838,   851,   868,  1058,  1081,
    1084,  1088,  1092,  1094,  1096,  1371,   635,   995,   640,   644,
     646,   649,   652,   997,  1372,  1006,  1368,  1008,  3386,  5981,
    5981,  3386,   665,  1369,  -966,  -966,   668,  -966,  3386,  -966,
    -966,   183,  1173,   196,   196,   196,   196,  1005,  1377,  -966,
    5981,  -966,  5981,  1634,  -966,  5981,  5981,  5981,  5981,  5981,
    -966,  -966,  -966,  -966,  -966,  5981,  5981,  -966,  -966,  5981,
    -966,  -966,  -966,  -966,  -966,  -966,  5981,  5981,  -966,  -966,
    5981,  -966,  -966,   -85,   129,  5981,  -966,  5981,  -966,   215,
    5981,  -966,   257,  2510,  5981,  1364,  1379,  5981,  -966,  -966,
    -966,  -966,  -966,  5981,  1380,  5981,  1394,  5981,  1219,    33,
      39,  -966,  -966,  5981,  -966,  -966,  -966,  -966,  -966,   196,
     674,  5981,  -966,   503,  2729,  1396,  1011,   677,  1397,   681,
    1398,   686,  1013,  1020,   688,  1417,  1853,   183,   493,   656,
    5981,  -966,   745,  5981,  -966,  5981,  -966,   547,   696,  5333,
    5549,  1418,  1024,  5765,   698,  1419,  1030,  3386,  3386,  5981,
    1037,  -966,   829,  5981,  -966,  5981,  -966,   551,  -966,  5981,
    -966,  -966,  -966,  -966,  -966,  5981,  5981,  -966,  -966,  5981,
    -966,   552,  2948,  -966,   183,   183,   864,   183,  1098,  1101,
    5981,  -966,  -966,  5981,   196,  5981,   196,  -966,  5981,  5981,
     196,  -966,  -966,  5981,  -966,  -966,   -30,    94,   183,  1108,
    1122,  5981,  -966,  1420,  1042,  1047,  1125,  5981,  -966,  5981,
    -966,   555,  -966,  -966,   183,  -966,   183,   183,  1130,   196,
     196,  1049,   196,  1052,  5981,  3386,  1238,  -966,  -966,  -966,
    -966,  -966,  -966,  -966,  1213,  -966,  -966,  1392,  -966,   183,
     183,  1132,  -966,  5981,  5981,   183,  1135,  1137,  5981,  -966,
    -966,  -966,  -966,   183,  5981,  5981,    36,  -966,  1435,  1423,
    1012,  1428,  -966,  -966,   183,   702,  1054,  -966,   183,   183,
    1139,  -966,  1056,  1071,  3386,  1238,  -966,  -966,  -966,  -966,
    -966,  -966,  -966,  1231,  -966,  -966,  5981,  -966,  -966,   183,
    5981,  5981,  -966,  -966,  1445,  1446,  -966,  1447,  1522,  -966,
    -966,  -966,  -966
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -966,  -966,  -966,  -395,   188,  -283,  -966,  -966,  -492,  -553,
    -966,  -606,   891,  -966,  -966,  -966,  -966,  -966,  -966,  -134,
    1184,  -966,  -966,     0,  1513,   -28,   -10,  -260,   -11,   928,
     -13,    44,  1051,  -966,  -966,  -966,  -966,  -966,  -966,  -966,
     -44,  -966,  -965,   585,   592,  -966
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -124
static const yytype_int16 yytable[] =
{
     153,   998,   175,   173,   721,   176,   177,   168,   467,   293,
     294,   169,   172,  -109,   174,   220,   221,   222,   223,   225,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   612,
     309,   309,   293,   294,   313,   479,   481,   483,   484,   485,
     178,   782,   653,   507,   654,   310,   668,   224,   295,   767,
     998,   218,   508,   296,   179,   509,   315,   297,   557,   725,
     607,   311,   311,   512,   558,   815,   608,   298,   513,   180,
     293,   294,   293,   294,   514,   293,   294,   515,   293,   294,
     293,   294,   293,   294,   293,   294,   293,   294,   516,   293,
     294,   235,   517,   224,   293,   294,   518,   621,   519,   423,
     293,   294,   424,   293,   294,   520,   966,   417,   521,   418,
     522,   457,   523,   611,   293,   294,   675,   524,   293,   294,
     724,   525,   293,   294,   293,   294,   526,   218,   527,   395,
     396,   293,   294,   528,   293,   294,   293,   294,   293,   294,
     293,   294,   408,   293,   294,   306,   412,   293,   294,   529,
     867,   504,   293,   294,   293,   294,   687,    16,    17,   293,
     294,   293,   294,   609,   964,   965,   610,   771,   293,   294,
     317,   293,   294,   823,   181,   293,   294,   182,   326,   327,
     328,   329,   330,   331,   332,   333,   334,   335,   336,   337,
     338,   339,   340,   341,   342,   343,   344,   345,   346,   347,
     348,   349,   350,   351,   352,   353,   354,   355,   356,   357,
     358,   359,   360,   361,   362,   363,   364,   365,   726,   847,
     669,   299,   670,   183,   367,   457,   761,   611,   888,   758,
     854,  1014,   889,   819,   822,   293,   294,   369,   370,   371,
     372,   373,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   386,   387,   388,   389,   390,   391,
     392,   393,   394,   475,   184,   397,   398,   399,   400,   401,
     402,   403,   404,   405,   406,   407,   845,   409,   410,   411,
     185,   413,   414,   477,   478,   530,   420,   186,   422,   531,
     187,   853,   783,   532,   494,   497,   493,   496,   502,   533,
     501,   967,   968,   969,   970,   971,   972,   973,   974,   498,
     534,   293,   294,   912,   535,   293,   294,   536,   188,   293,
     294,   510,   511,   537,   189,   293,   294,   538,  -110,   539,
     190,   877,   540,   366,   541,   476,   293,   294,   542,   292,
     293,   294,   685,   293,   294,   319,   543,   293,   294,   293,
     294,   544,   191,   293,   294,   293,   294,   192,   293,   294,
     293,   294,   897,   193,   293,   294,   293,   294,   553,   194,
     679,   680,   293,   294,   911,   545,   195,   293,   294,   546,
     653,   547,   654,   548,   220,   221,   222,   223,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   549,   368,
     550,   293,   294,   551,   196,   293,   294,   293,   294,   293,
     294,   197,   653,   552,   654,   556,   416,   559,   866,   562,
     951,   563,   283,   564,   293,   294,   293,   294,   198,   293,
     294,   568,  -111,   503,   569,   870,   570,   199,   475,   293,
     294,   293,   294,   293,   294,   293,   294,   293,   294,   293,
     294,   293,   294,   703,   653,   645,   654,   293,   294,   648,
     293,   294,   293,   294,   579,   506,   580,   415,   581,   307,
     308,   583,   582,   584,   585,   838,   586,   873,   841,   671,
     672,   200,   673,   674,   596,   605,   201,   677,   678,   784,
     293,   294,   293,   294,   293,   294,   790,   745,   293,   294,
     293,   294,   293,   294,   791,   600,   754,   682,   792,  -112,
     476,   293,   294,   793,   914,   293,   294,   794,  -113,   797,
     421,   800,   293,   294,   801,   603,   802,   202,   293,   294,
     293,   294,   293,   294,   293,   294,   803,   293,   294,   293,
     294,   203,   778,   293,   294,   293,   294,   293,   294,   560,
     293,   294,   293,   294,   804,   553,   805,   204,   689,   808,
     690,   691,   293,   294,   499,   692,   693,   694,   205,   293,
     294,   695,   696,   697,   698,   699,   700,   701,   702,   206,
     293,   294,   293,   294,   475,   293,   294,   686,   704,   705,
     706,   707,   708,   709,   710,   711,   207,   712,   713,   714,
     208,   715,   716,   717,   934,   935,   663,   664,   718,   666,
     667,   293,   294,   209,   728,   729,   730,   731,   732,   733,
     734,   735,   736,   737,   738,   739,   740,   741,   742,   743,
     744,   475,   746,   747,   748,   749,   750,   751,   752,   753,
     210,   755,   809,   757,   811,   606,   812,   613,   826,   762,
     211,   855,   766,   828,   858,   212,   476,   829,   616,   830,
     213,   860,   831,   773,   774,   832,   775,   776,   293,   294,
     293,   294,   293,   294,   293,   294,   865,   915,   842,   293,
     294,   844,   997,   293,   294,   293,   294,   891,   293,   294,
     900,   293,   294,   881,   902,   293,   294,   475,   475,   904,
     653,   907,   654,   476,   293,   294,   214,   293,   294,   922,
     661,   931,   215,   293,   294,  1025,   293,   294,   759,   760,
     293,   294,   814,   893,   216,   293,   294,   293,   294,   217,
     475,  1032,   237,   475,   238,   293,   294,   293,   294,   320,
     475,   293,   294,   321,   653,   322,   654,   239,   653,   653,
     654,   654,   653,   561,   654,   475,   565,   617,   566,   839,
     840,   240,   241,   293,   294,   943,   917,   920,   242,   476,
     476,   941,   947,   293,   294,   988,   293,   294,   293,   294,
     849,   618,   850,   567,   293,   294,   856,   857,   571,   859,
     572,   619,   243,   573,   620,   475,   861,   244,   574,   862,
     723,   575,   476,   293,   294,   476,   863,   864,   293,   294,
     293,   294,   476,   293,   294,   868,   245,   869,   293,   294,
     872,   293,   294,   246,   878,   662,   475,   476,   576,   683,
     247,   248,   577,   882,   578,   884,   587,   886,   475,   249,
     250,   588,   251,   890,   589,   252,   769,   770,   293,   294,
     938,   892,   293,   294,   293,   294,   293,   294,  -114,   475,
     475,   293,   294,   646,   293,   294,   590,   476,   293,   294,
     916,  -115,   253,   918,   622,   919,   651,   293,   294,   924,
     926,   591,   254,   930,   475,   954,   293,   294,  -116,   936,
     293,   294,  1035,   939,   255,   940,  1037,  1038,   476,   256,
     257,   293,   294,   293,   294,   944,   945,   293,   294,   946,
     476,   623,   816,   818,   821,   592,   258,   624,   593,   259,
     958,   260,   594,   959,   595,   960,   261,   684,   961,   962,
     597,   476,   476,   963,   598,   293,   294,   475,   293,   294,
     599,   981,   293,   294,   293,   294,   601,   986,   602,   987,
     293,   294,   785,   262,   293,   294,   476,   786,   168,   846,
     293,   294,   614,   615,   996,   263,   293,   294,   293,   294,
     787,   852,   293,   294,   625,   264,   788,   293,   294,   789,
     265,   795,   266,  1005,  1006,   796,   475,   267,  1010,   268,
     293,   294,   799,   269,  1012,  1013,   293,   294,   806,   293,
     294,   293,   294,   807,   270,   293,   294,   871,   810,   476,
     874,   876,   293,   294,   827,   271,   833,   676,   293,   294,
     626,   -93,   -93,   293,   294,   835,   272,   837,   293,   294,
     899,   273,   905,   274,   293,   294,   293,   294,   275,   906,
     276,   894,   896,   928,   277,   293,   294,   293,   294,   933,
     293,   294,   293,   294,   910,   913,   937,   278,   476,   293,
     294,   983,   279,   293,   294,   921,   984,   280,   994,   293,
     294,   995,   281,  1026,   282,  1030,   293,   294,  -117,   286,
     287,   293,   294,   665,   288,   942,   293,   294,   293,   294,
    1031,   293,   294,   293,   294,   293,   294,   293,   294,   948,
     950,  -118,   952,   953,  -119,   955,   627,   289,  -120,   291,
     293,   294,  -122,   719,  -123,   505,  -121,   631,   628,   956,
     293,   294,   957,   293,   294,   629,   978,   293,   294,   979,
     630,   293,   294,   293,   294,   293,   294,   293,   294,   989,
     293,   294,   990,   980,   991,   992,   985,   293,   294,   768,
     632,   993,   633,  1004,   634,   635,  1008,   636,  1009,   637,
    1029,   293,   294,   638,   293,   294,   639,  1002,  1003,   293,
     294,   293,   294,  1007,   293,   294,   293,   294,   293,   294,
     640,  1011,   641,   642,   300,   301,   643,   302,   303,   649,
     650,   652,  1024,   304,   305,   676,  1027,  1028,   720,     2,
       3,     4,     5,     6,     7,     8,   426,    10,    11,   756,
      12,   323,   324,   325,   763,   772,    13,  1036,    14,   777,
    1017,  1018,  1019,  1020,  1021,  1022,   293,   294,   486,   487,
     488,   489,   490,   491,    15,   779,   780,    16,    17,   781,
     798,   813,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,   610,    92,    93,    94,
      95,    96,    97,   824,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   442,   443,   444,   445,   446,   447,
     448,   449,   450,   451,   452,   453,   128,   129,   130,   131,
     132,   133,   134,   135,   454,   136,   137,   138,   139,   140,
     141,   142,   143,   455,   825,   834,   456,   836,   843,   655,
     848,   879,   457,   653,   458,   654,   459,   145,   146,   460,
     461,   462,   463,   147,   885,   148,   880,   883,   887,   898,
     901,   903,   655,   149,   150,   464,   722,   465,     2,     3,
       4,     5,     6,     7,     8,   426,    10,    11,  1000,    12,
     908,   927,   932,   982,  1001,    13,  1016,    14,   967,   968,
     969,   970,   971,   972,   973,   974,   968,   969,   970,   971,
     972,   973,   974,    15,  1015,  1023,    16,    17,  1039,  1040,
    1041,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,   427,   428,   429,   430,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,  1042,    92,    93,    94,    95,
      96,    97,   764,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   442,   443,   444,   445,   446,   447,   448,
     449,   450,   451,   452,   453,   128,   129,   130,   131,   132,
     133,   134,   135,   454,   136,   137,   138,   139,   140,   141,
     142,   143,   455,   604,   236,   456,   688,  1033,  1034,     0,
       0,   457,   653,   458,   654,   459,   145,   146,   460,   461,
     462,   463,   147,     0,   148,     0,     0,     0,     0,     0,
       0,   655,   149,   150,   464,   820,   465,     2,     3,     4,
       5,     6,     7,     8,   426,    10,    11,     0,    12,     0,
       0,     0,     0,     0,    13,     0,    14,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    15,     0,     0,    16,    17,     0,     0,     0,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
     427,   428,   429,   430,   431,   432,   433,   434,   435,   436,
     437,   438,   439,   440,   441,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,     0,    92,    93,    94,    95,    96,
      97,     0,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   442,   443,   444,   445,   446,   447,   448,   449,
     450,   451,   452,   453,   128,   129,   130,   131,   132,   133,
     134,   135,   454,   136,   137,   138,   139,   140,   141,   142,
     143,   455,     0,     0,   456,     0,     0,     0,     0,     0,
     457,   653,   458,   654,   459,   145,   146,   460,   461,   462,
     463,   147,     0,   148,     0,     0,     0,     0,     0,     0,
     655,   149,   150,   464,   851,   465,     2,     3,     4,     5,
       6,     7,     8,   426,    10,    11,     0,    12,     0,     0,
       0,     0,     0,    13,     0,    14,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    15,     0,     0,    16,    17,     0,     0,     0,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,   427,
     428,   429,   430,   431,   432,   433,   434,   435,   436,   437,
     438,   439,   440,   441,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,     0,    92,    93,    94,    95,    96,    97,
       0,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   128,   129,   130,   131,   132,   133,   134,
     135,   454,   136,   137,   138,   139,   140,   141,   142,   143,
     455,     0,     0,   456,     0,     0,     0,     0,     0,   457,
     653,   458,   654,   459,   145,   146,   460,   461,   462,   463,
     147,     0,   148,     0,     0,     0,     0,     0,     0,   655,
     149,   150,   464,   909,   465,     2,     3,     4,     5,     6,
       7,     8,   426,    10,    11,     0,    12,     0,     0,     0,
       0,     0,    13,     0,    14,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      15,     0,     0,    16,    17,     0,     0,     0,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,   427,   428,
     429,   430,   431,   432,   433,   434,   435,   436,   437,   438,
     439,   440,   441,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,     0,    92,    93,    94,    95,    96,    97,     0,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     442,   443,   444,   445,   446,   447,   448,   449,   450,   451,
     452,   453,   128,   129,   130,   131,   132,   133,   134,   135,
     454,   136,   137,   138,   139,   140,   141,   142,   143,   455,
       0,     0,   456,     0,     0,     0,     0,     0,   457,   653,
     458,   654,   459,   145,   146,   460,   461,   462,   463,   147,
       0,   148,     0,     0,     0,     0,     0,     0,   655,   149,
     150,   464,     0,   465,     2,     3,     4,     5,     6,     7,
       8,   426,    10,    11,     0,    12,     0,     0,     0,     0,
       0,    13,     0,    14,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    15,
       0,     0,    16,    17,     0,     0,     0,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,   427,   428,   429,
     430,   431,   432,   433,   434,   435,   436,   437,   438,   439,
     440,   441,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,     0,    92,    93,    94,    95,    96,    97,     0,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   442,
     443,   444,   445,   446,   447,   448,   449,   450,   451,   452,
     453,   128,   129,   130,   131,   132,   133,   134,   135,   454,
     136,   137,   138,   139,   140,   141,   142,   143,   455,     0,
       0,   456,     0,     0,     0,     0,     0,   457,   653,   458,
     654,   459,   145,   146,   460,   461,   462,   463,   147,     0,
     148,     0,     0,     0,     0,     0,     0,     0,   149,   150,
     464,   817,   465,     2,     3,     4,     5,     6,     7,     8,
     426,    10,    11,     0,    12,     0,     0,     0,     0,     0,
      13,     0,    14,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    15,     0,
       0,    16,    17,     0,     0,     0,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,   427,   428,   429,   430,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
       0,    92,    93,    94,    95,    96,    97,     0,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   442,   443,
     444,   445,   446,   447,   448,   449,   450,   451,   452,   453,
     128,   129,   130,   131,   132,   133,   134,   135,   454,   136,
     137,   138,   139,   140,   141,   142,   143,   455,     0,     0,
     456,     0,     0,     0,     0,     0,   457,   653,   458,   654,
     459,   145,   146,   460,   461,   462,   463,   147,     0,   148,
       0,     0,     0,     0,     0,     0,     0,   149,   150,   464,
     875,   465,     2,     3,     4,     5,     6,     7,     8,   426,
      10,    11,     0,    12,     0,     0,     0,     0,     0,    13,
       0,    14,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    15,     0,     0,
      16,    17,     0,     0,     0,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,   427,   428,   429,   430,   431,
     432,   433,   434,   435,   436,   437,   438,   439,   440,   441,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,     0,
      92,    93,    94,    95,    96,    97,     0,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   128,
     129,   130,   131,   132,   133,   134,   135,   454,   136,   137,
     138,   139,   140,   141,   142,   143,   455,     0,     0,   456,
       0,     0,     0,     0,     0,   457,   653,   458,   654,   459,
     145,   146,   460,   461,   462,   463,   147,     0,   148,     0,
       0,     0,     0,     0,     0,     0,   149,   150,   464,   895,
     465,     2,     3,     4,     5,     6,     7,     8,   426,    10,
      11,     0,    12,     0,     0,     0,     0,     0,    13,     0,
      14,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    15,     0,     0,    16,
      17,     0,     0,     0,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,   427,   428,   429,   430,   431,   432,
     433,   434,   435,   436,   437,   438,   439,   440,   441,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,     0,    92,
      93,    94,    95,    96,    97,     0,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   128,   129,
     130,   131,   132,   133,   134,   135,   454,   136,   137,   138,
     139,   140,   141,   142,   143,   455,     0,     0,   456,     0,
       0,     0,     0,     0,   457,   653,   458,   654,   459,   145,
     146,   460,   461,   462,   463,   147,     0,   148,     0,     0,
       0,     0,     0,     0,     0,   149,   150,   464,   949,   465,
       2,     3,     4,     5,     6,     7,     8,   426,    10,    11,
       0,    12,     0,     0,     0,     0,     0,    13,     0,    14,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    15,     0,     0,    16,    17,
       0,     0,     0,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,   427,   428,   429,   430,   431,   432,   433,
     434,   435,   436,   437,   438,   439,   440,   441,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,     0,    92,    93,
      94,    95,    96,    97,     0,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   442,   443,   444,   445,   446,
     447,   448,   449,   450,   451,   452,   453,   128,   129,   130,
     131,   132,   133,   134,   135,   454,   136,   137,   138,   139,
     140,   141,   142,   143,   455,     0,     0,   456,     0,     0,
       0,     0,     0,   457,   653,   458,   654,   459,   145,   146,
     460,   461,   462,   463,   147,     0,   148,     0,     0,     0,
       0,     0,     0,     0,   149,   150,   464,     0,   465,     2,
       3,     4,     5,     6,     7,     8,   426,    10,    11,     0,
      12,     0,     0,     0,     0,     0,    13,     0,    14,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    15,     0,     0,    16,    17,     0,
       0,     0,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,     0,    92,    93,    94,
      95,    96,    97,     0,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   442,   443,   444,   445,   446,   447,
     448,   449,   450,   451,   452,   453,   128,   129,   130,   131,
     132,   133,   134,   135,   454,   136,   137,   138,   139,   140,
     141,   142,   143,   455,     0,     0,   456,     0,     0,     0,
       0,     0,   457,     0,   458,     0,   459,   145,   146,   460,
     461,   462,   463,   147,     0,   148,     0,     0,     0,     0,
       0,     0,     0,   149,   150,   464,     1,   465,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,     0,    12,
       0,     0,     0,     0,     0,    13,     0,    14,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    15,     0,     0,    16,    17,     0,     0,
       0,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,     0,    92,    93,    94,    95,
      96,    97,     0,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   128,   129,   130,   131,   132,
     133,   134,   135,     0,   136,   137,   138,   139,   140,   141,
     142,   143,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   144,     0,     0,   145,   146,     0,     0,
       0,     0,   147,     0,   148,     0,     0,     0,     0,     0,
       0,     0,   149,   150,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,     0,    12,     0,     0,     0,     0,
       0,    13,     0,    14,     0,     0,     0,     0,     0,     0,
       0,     0,   170,     0,     0,     0,     0,     0,     0,    15,
       0,   171,    16,    17,     0,     0,     0,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,     0,    92,    93,    94,    95,    96,    97,     0,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   128,   129,   130,   131,   132,   133,   134,   135,     0,
     136,   137,   138,   139,   140,   141,   142,   143,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   144,
       0,     0,   145,   146,     0,     0,     0,     0,   147,     0,
     148,     0,     0,     0,     0,     0,     0,     0,   149,   150,
       2,     3,     4,     5,     6,     7,     8,     9,    10,    11,
     312,    12,     0,     0,     0,     0,     0,    13,     0,    14,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    15,     0,     0,    16,    17,
       0,     0,     0,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,     0,    92,    93,
      94,    95,    96,    97,     0,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   128,   129,   130,
     131,   132,   133,   134,   135,     0,   136,   137,   138,   139,
     140,   141,   142,   143,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   144,     0,     0,   145,   146,
       0,     0,     0,     0,   147,     0,   148,     0,     0,     0,
       0,     0,     0,     0,   149,   150,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,     0,    12,     0,     0,
       0,     0,     0,    13,     0,    14,     0,     0,     0,     0,
       0,     0,     0,     0,   316,     0,     0,     0,     0,     0,
       0,    15,     0,     0,    16,    17,     0,     0,     0,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,     0,    92,    93,    94,    95,    96,    97,
       0,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   128,   129,   130,   131,   132,   133,   134,
     135,     0,   136,   137,   138,   139,   140,   141,   142,   143,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   144,     0,     0,   145,   146,     0,     0,     0,     0,
     147,     0,   148,     0,     0,     0,     0,     0,     0,     0,
     149,   150,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,     0,    12,     0,     0,     0,     0,     0,    13,
       0,    14,     0,     0,     0,     0,     0,   681,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    15,     0,     0,
      16,    17,     0,     0,     0,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,     0,
      92,    93,    94,    95,    96,    97,     0,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   128,
     129,   130,   131,   132,   133,   134,   135,     0,   136,   137,
     138,   139,   140,   141,   142,   143,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   144,     0,     0,
     145,   146,     0,     0,     0,     0,   147,     0,   148,     0,
       0,     0,     0,     0,     0,     0,   149,   150,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,     0,    12,
       0,     0,     0,     0,     0,    13,     0,    14,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    15,     0,     0,    16,    17,     0,     0,
       0,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,     0,    92,    93,    94,    95,
      96,    97,   727,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   128,   129,   130,   131,   132,
     133,   134,   135,     0,   136,   137,   138,   139,   140,   141,
     142,   143,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   144,     0,     0,   145,   146,     0,     0,
       0,     0,   147,     0,   148,     0,     0,     0,     0,     0,
       0,     0,   149,   150,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,     0,    12,     0,     0,     0,     0,
       0,    13,     0,    14,     0,     0,     0,     0,     0,     0,
       0,   366,     0,     0,     0,     0,     0,     0,     0,    15,
       0,     0,    16,    17,     0,     0,     0,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,     0,    92,    93,    94,    95,    96,    97,     0,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   128,   129,   130,   131,   132,   133,   134,   135,     0,
     136,   137,   138,   139,   140,   141,   142,   143,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   144,
       0,     0,   145,   146,     0,     0,     0,     0,   147,     0,
     148,     0,     0,     0,     0,     0,     0,     0,   149,   150,
       2,     3,     4,     5,     6,     7,     8,     9,    10,    11,
     765,    12,     0,     0,     0,     0,     0,    13,     0,    14,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    15,     0,     0,    16,    17,
       0,     0,     0,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,     0,    92,    93,
      94,    95,    96,    97,     0,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   128,   129,   130,
     131,   132,   133,   134,   135,     0,   136,   137,   138,   139,
     140,   141,   142,   143,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   144,     0,     0,   145,   146,
       0,     0,     0,     0,   147,     0,   148,     0,     0,     0,
       0,     0,     0,     0,   149,   150,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,     0,    12,     0,     0,
       0,     0,     0,    13,     0,    14,     0,     0,     0,     0,
     923,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    15,     0,     0,    16,    17,     0,     0,     0,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,     0,    92,    93,    94,    95,    96,    97,
       0,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   128,   129,   130,   131,   132,   133,   134,
     135,     0,   136,   137,   138,   139,   140,   141,   142,   143,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   144,     0,     0,   145,   146,     0,     0,     0,     0,
     147,     0,   148,     0,     0,     0,     0,     0,     0,     0,
     149,   150,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,     0,    12,     0,     0,     0,     0,     0,    13,
       0,    14,     0,     0,     0,     0,   925,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    15,     0,     0,
      16,    17,     0,     0,     0,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,     0,
      92,    93,    94,    95,    96,    97,     0,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   128,
     129,   130,   131,   132,   133,   134,   135,     0,   136,   137,
     138,   139,   140,   141,   142,   143,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   144,     0,     0,
     145,   146,     0,     0,     0,     0,   147,     0,   148,     0,
       0,     0,     0,     0,     0,     0,   149,   150,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,     0,    12,
       0,     0,     0,     0,     0,    13,     0,    14,     0,     0,
       0,     0,   929,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    15,     0,     0,    16,    17,     0,     0,
       0,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,     0,    92,    93,    94,    95,
      96,    97,     0,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   128,   129,   130,   131,   132,
     133,   134,   135,     0,   136,   137,   138,   139,   140,   141,
     142,   143,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   144,     0,     0,   145,   146,     0,     0,
       0,     0,   147,     0,   148,     0,     0,     0,     0,     0,
       0,     0,   149,   150,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,     0,    12,     0,     0,     0,     0,
       0,    13,     0,    14,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    15,
       0,     0,    16,    17,     0,     0,     0,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,     0,    92,    93,    94,    95,    96,    97,     0,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   128,   129,   130,   131,   132,   133,   134,   135,     0,
     136,   137,   138,   139,   140,   141,   142,   143,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   144,
       0,     0,   145,   146,     0,     0,     0,     0,   147,     0,
     148,     0,     0,     0,     0,     0,     0,     0,   149,   150,
       2,     3,     4,     5,     6,     7,     8,     9,    10,    11,
       0,    12,     0,   480,     0,     0,     0,     0,     0,    14,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    15,     0,     0,    16,    17,
       0,     0,     0,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,     0,     0,    92,    93,
      94,    95,    96,    97,     0,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   128,   129,   130,
     131,   132,   133,   134,   135,     0,   136,   137,   138,   139,
     140,   141,   142,   143,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   144,     0,     0,   145,   146,
       0,     0,     0,     0,   147,     0,   148,     0,     0,     0,
       0,     0,     0,     0,   149,   150,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,     0,    12,     0,   482,
       0,     0,     0,     0,     0,    14,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    15,     0,     0,    16,    17,     0,     0,     0,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,     0,     0,    92,    93,    94,    95,    96,    97,
       0,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   128,   129,   130,   131,   132,   133,   134,
     135,     0,   136,   137,   138,   139,   140,   141,   142,   143,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   144,     0,     0,   145,   146,     0,     0,     0,     0,
     147,     0,   148,     0,     0,     0,     0,     0,     0,     0,
     149,   150,     2,     3,     4,     5,     6,     7,     8,     9,
      10,    11,     0,    12,     0,     0,     0,     0,     0,     0,
       0,    14,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    15,     0,     0,
      16,    17,     0,     0,     0,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,     0,     0,
      92,    93,    94,    95,    96,    97,     0,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   128,
     129,   130,   131,   132,   133,   134,   135,     0,   136,   137,
     138,   139,   140,   141,   142,   143,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   144,     0,     0,
     145,   146,     0,     0,     0,     0,   147,     0,   148,     0,
       0,     0,     0,     0,     0,     0,   149,   150,     2,     3,
       4,     5,     6,     7,     8,     9,    10,    11,     0,    12,
       0,     0,     0,     0,     0,     0,     0,    14,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   492,     0,     0,    16,    17,     0,     0,
       0,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,     0,     0,    92,    93,    94,    95,
      96,    97,     0,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   128,   129,   130,   131,   132,
     133,   134,   135,     0,   136,   137,   138,   139,   140,   141,
     142,   143,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   144,     0,     0,   145,   146,     0,     0,
       0,     0,   147,     0,   148,     0,     0,     0,     0,     0,
       0,     0,   149,   150,     2,     3,     4,     5,     6,     7,
       8,     9,    10,    11,     0,    12,     0,     0,     0,     0,
       0,     0,     0,    14,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   495,
       0,     0,    16,    17,     0,     0,     0,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
       0,     0,    92,    93,    94,    95,    96,    97,     0,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   128,   129,   130,   131,   132,   133,   134,   135,     0,
     136,   137,   138,   139,   140,   141,   142,   143,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   144,
       0,     0,   145,   146,     0,     0,     0,     0,   147,     0,
     148,     0,     0,     0,     0,     0,     0,     0,   149,   150,
       2,     3,     4,     5,     6,     7,     8,     9,    10,    11,
       0,    12,     0,     0,     0,     0,     0,     0,     0,    14,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   500,     0,     0,    16,    17,
       0,     0,     0,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,     0,     0,    92,    93,
      94,    95,    96,    97,     0,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   128,   129,   130,
     131,   132,   133,   134,   135,     0,   136,   137,   138,   139,
     140,   141,   142,   143,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   144,     0,     0,   145,   146,
       0,     0,     0,     0,   147,     0,   148,     0,     0,     0,
       0,     0,     0,     0,   149,   150,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,     0,    12,     0,     0,
       0,     0,     0,     0,     0,    14,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,     0,     0,    92,    93,    94,    95,    96,    97,
       0,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   128,   129,   130,   131,   132,   133,   134,
     135,     0,   136,   137,   138,   139,   140,   141,   142,   143,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   144,     0,     0,   145,   146,     0,     0,     0,     0,
     147,     0,   148,     0,     0,     0,     0,     0,     0,     0,
     149,   150
};

static const yytype_int16 yycheck[] =
{
       0,   966,    15,    13,   610,    16,    17,    12,   291,    39,
      40,    11,    12,    20,    14,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,   424,
      14,    14,    39,    40,   168,   295,   296,   297,   298,   299,
      12,    12,   197,    19,   199,    29,    29,    12,    18,   655,
    1015,    16,    28,    23,    12,    31,    13,    27,    13,   612,
      13,    45,    45,    13,    19,   220,    19,    37,    13,    12,
      39,    40,    39,    40,    13,    39,    40,    13,    39,    40,
      39,    40,    39,    40,    39,    40,    39,    40,    13,    39,
      40,    91,    13,    12,    39,    40,    13,    16,    13,    10,
      39,    40,    13,    39,    40,    13,    12,    19,    13,    21,
      13,   196,    13,   198,    39,    40,    15,    13,    39,    40,
     612,    13,    39,    40,    39,    40,    13,    16,    13,   263,
     264,    39,    40,    13,    39,    40,    39,    40,    39,    40,
      39,    40,   276,    39,    40,   158,   280,    39,    40,    13,
      21,    19,    39,    40,    39,    40,    34,    41,    42,    39,
      40,    39,    40,    16,   194,   195,    19,   659,    39,    40,
     170,    39,    40,   726,    12,    39,    40,    12,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   613,   772,
     480,   191,   482,    12,   224,   196,   195,   198,   195,   188,
     783,   195,   193,   725,   726,    39,    40,   237,   238,   239,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   251,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,   262,   291,    12,   265,   266,   267,   268,   269,
     270,   271,   272,   273,   274,   275,   768,   277,   278,   279,
      12,   281,   282,   293,   294,    13,   286,    12,   288,    13,
      12,   783,   687,    13,   307,   308,   307,   308,   311,    13,
     311,   207,   208,   209,   210,   211,   212,   213,   214,   309,
      13,    39,    40,   866,    13,    39,    40,    13,    12,    39,
      40,   321,   322,    13,    12,    39,    40,    13,    20,    13,
      12,   823,    13,    30,    13,   291,    39,    40,    13,     0,
      39,    40,    15,    39,    40,    15,    13,    39,    40,    39,
      40,    13,    12,    39,    40,    39,    40,    12,    39,    40,
      39,    40,   854,    12,    39,    40,    39,    40,   368,    12,
     504,   505,    39,    40,   866,    13,    12,    39,    40,    13,
     197,    13,   199,    13,   428,   429,   430,   431,   432,   433,
     434,   435,   436,   437,   438,   439,   440,   441,    13,   120,
      13,    39,    40,    13,    12,    39,    40,    39,    40,    39,
      40,    12,   197,    13,   199,    13,   199,    13,   813,    13,
     912,    13,    29,    13,    39,    40,    39,    40,    12,    39,
      40,    13,    20,    13,    13,   220,    13,    12,   466,    39,
      40,    39,    40,    39,    40,    39,    40,    39,    40,    39,
      40,    39,    40,   587,   197,   455,   199,    39,    40,   459,
      39,    40,    39,    40,    13,    15,    13,    10,    13,    43,
      44,    13,    13,    13,    13,   758,    13,   220,   761,   492,
     493,    12,   495,   496,    13,    13,    12,   500,   501,    13,
      39,    40,    39,    40,    39,    40,    13,   631,    39,    40,
      39,    40,    39,    40,    13,    13,   640,   507,    13,    20,
     466,    39,    40,    13,    21,    39,    40,    13,    20,    13,
      10,    13,    39,    40,    13,    16,    13,    12,    39,    40,
      39,    40,    39,    40,    39,    40,    13,    39,    40,    39,
      40,    12,   676,    39,    40,    39,    40,    39,    40,    19,
      39,    40,    39,    40,    13,   555,    13,    12,   558,    13,
     560,   561,    39,    40,    10,   565,   566,   567,    12,    39,
      40,   571,   572,   573,   574,   575,   576,   577,   578,    12,
      39,    40,    39,    40,   612,    39,    40,    15,   588,   589,
     590,   591,   592,   593,   594,   595,    12,   597,   598,   599,
      12,   601,   602,   603,   887,   888,    16,    17,   608,    16,
      17,    39,    40,    12,   614,   615,   616,   617,   618,   619,
     620,   621,   622,   623,   624,   625,   626,   627,   628,   629,
     630,   659,   632,   633,   634,   635,   636,   637,   638,   639,
      12,   641,    13,   643,    13,    13,    13,    13,    13,   649,
      12,   785,   652,    13,   788,    12,   612,    13,    16,    13,
      12,   795,    13,   663,   664,    13,   666,   667,    39,    40,
      39,    40,    39,    40,    39,    40,   810,    21,    13,    39,
      40,    13,   965,    39,    40,    39,    40,    13,    39,    40,
      13,    39,    40,   827,    13,    39,    40,   725,   726,    13,
     197,    13,   199,   659,    39,    40,    12,    39,    40,    13,
      13,    13,    12,    39,    40,    13,    39,    40,   191,   192,
      39,    40,   722,   220,    12,    39,    40,    39,    40,    12,
     758,  1014,    12,   761,    12,    39,    40,    39,    40,    15,
     768,    39,    40,    19,   197,    21,   199,    12,   197,   197,
     199,   199,   197,    19,   199,   783,    19,    16,    19,   759,
     760,    12,    12,    39,    40,   899,    21,   220,    12,   725,
     726,   220,   220,    39,    40,   220,    39,    40,    39,    40,
     780,    16,   782,    19,    39,    40,   786,   787,    19,   789,
      19,    16,    12,    19,    16,   823,   796,    12,    19,   799,
     612,    19,   758,    39,    40,   761,   806,   807,    39,    40,
      39,    40,   768,    39,    40,   815,    12,   817,    39,    40,
     820,    39,    40,    12,   824,    20,   854,   783,    19,    31,
      12,    12,    19,   833,    19,   835,    19,   837,   866,    12,
      12,    19,    12,   843,    19,    12,   658,   659,    39,    40,
      21,   851,    39,    40,    39,    40,    39,    40,    20,   887,
     888,    39,    40,    10,    39,    40,    19,   823,    39,    40,
     870,    20,    12,   873,    16,   875,    10,    39,    40,   879,
     880,    19,    12,   883,   912,    21,    39,    40,    20,   889,
      39,    40,  1026,   893,    12,   895,  1030,  1031,   854,    12,
      12,    39,    40,    39,    40,   905,   906,    39,    40,   909,
     866,    16,   724,   725,   726,    19,    12,    16,    19,    12,
     920,    12,    19,   923,    19,   925,    12,    15,   928,   929,
      19,   887,   888,   933,    19,    39,    40,   965,    39,    40,
      19,   941,    39,    40,    39,    40,    19,   947,    19,   949,
      39,    40,    19,    12,    39,    40,   912,    19,    12,   771,
      39,    40,    16,    17,   964,    12,    39,    40,    39,    40,
      19,   783,    39,    40,    16,    12,    19,    39,    40,    19,
      12,    19,    12,   983,   984,    19,  1014,    12,   988,    12,
      39,    40,    19,    12,   994,   995,    39,    40,    19,    39,
      40,    39,    40,    19,    12,    39,    40,   819,    19,   965,
     822,   823,    39,    40,    19,    12,    19,    12,    39,    40,
      16,    16,    17,    39,    40,    19,    12,    19,    39,    40,
      19,    12,    19,    12,    39,    40,    39,    40,    12,    19,
      12,   853,   854,    19,    12,    39,    40,    39,    40,    19,
      39,    40,    39,    40,   866,   867,    19,    12,  1014,    39,
      40,    19,    12,    39,    40,   877,    19,    12,    19,    39,
      40,    19,    12,    19,    12,    19,    39,    40,    20,    12,
      12,    39,    40,    20,    12,   897,    39,    40,    39,    40,
      19,    39,    40,    39,    40,    39,    40,    39,    40,   911,
     912,    20,   914,   915,    20,   917,    16,    12,    20,    12,
      39,    40,    20,    28,    20,    12,    20,    12,    16,    21,
      39,    40,    21,    39,    40,    16,   938,    39,    40,    21,
      16,    39,    40,    39,    40,    39,    40,    39,    40,   951,
      39,    40,   954,    21,   956,   957,    21,    39,    40,    21,
      12,    21,    12,    21,    12,    12,    21,    12,    21,    12,
      21,    39,    40,    12,    39,    40,    12,   979,   980,    39,
      40,    39,    40,   985,    39,    40,    39,    40,    39,    40,
      12,   993,    12,    12,    32,    33,    12,    35,    36,    12,
      12,    12,  1004,    41,    42,    12,  1008,  1009,    10,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    10,
      14,    24,    25,    26,    10,    21,    20,  1029,    22,    10,
     208,   209,   210,   211,   212,   213,    39,    40,   300,   301,
     302,   303,   304,   305,    38,    13,    19,    41,    42,    15,
      13,    13,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,    19,   121,   122,   123,
     124,   125,   126,    12,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,    13,    13,   190,    19,    19,   216,
      13,    27,   196,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   206,   207,    10,   209,    27,    27,   189,    13,
      13,    13,   216,   217,   218,   219,   220,   221,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,   215,    14,
      13,    13,    13,    13,    42,    20,    13,    22,   207,   208,
     209,   210,   211,   212,   213,   214,   208,   209,   210,   211,
     212,   213,   214,    38,    19,    27,    41,    42,    13,    13,
      13,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,    13,   121,   122,   123,   124,
     125,   126,   651,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   419,    91,   190,   555,  1015,  1023,    -1,
      -1,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,    -1,   209,    -1,    -1,    -1,    -1,    -1,
      -1,   216,   217,   218,   219,   220,   221,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    -1,    14,    -1,
      -1,    -1,    -1,    -1,    20,    -1,    22,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    38,    -1,    -1,    41,    42,    -1,    -1,    -1,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,    -1,   121,   122,   123,   124,   125,
     126,    -1,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,    -1,    -1,   190,    -1,    -1,    -1,    -1,    -1,
     196,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,   207,    -1,   209,    -1,    -1,    -1,    -1,    -1,    -1,
     216,   217,   218,   219,   220,   221,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    -1,
      -1,    -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    -1,    -1,    41,    42,    -1,    -1,    -1,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,    -1,   121,   122,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,    -1,    -1,   190,    -1,    -1,    -1,    -1,    -1,   196,
     197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
     207,    -1,   209,    -1,    -1,    -1,    -1,    -1,    -1,   216,
     217,   218,   219,   220,   221,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    -1,    14,    -1,    -1,    -1,
      -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      38,    -1,    -1,    41,    42,    -1,    -1,    -1,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,    -1,   121,   122,   123,   124,   125,   126,    -1,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
      -1,    -1,   190,    -1,    -1,    -1,    -1,    -1,   196,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,   207,
      -1,   209,    -1,    -1,    -1,    -1,    -1,    -1,   216,   217,
     218,   219,    -1,   221,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    -1,    -1,    -1,
      -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,
      -1,    -1,    41,    42,    -1,    -1,    -1,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,    -1,   121,   122,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,    -1,
      -1,   190,    -1,    -1,    -1,    -1,    -1,   196,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,   207,    -1,
     209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,   218,
     219,   220,   221,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    -1,    14,    -1,    -1,    -1,    -1,    -1,
      20,    -1,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,
      -1,    41,    42,    -1,    -1,    -1,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
      -1,   121,   122,   123,   124,   125,   126,    -1,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,    -1,    -1,
     190,    -1,    -1,    -1,    -1,    -1,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   207,    -1,   209,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,   218,   219,
     220,   221,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    -1,    -1,    -1,    -1,    20,
      -1,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,
      41,    42,    -1,    -1,    -1,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,    -1,
     121,   122,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,    -1,    -1,   190,
      -1,    -1,    -1,    -1,    -1,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,   207,    -1,   209,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   217,   218,   219,   220,
     221,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    -1,    14,    -1,    -1,    -1,    -1,    -1,    20,    -1,
      22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    41,
      42,    -1,    -1,    -1,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,    -1,   121,
     122,   123,   124,   125,   126,    -1,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,    -1,    -1,   190,    -1,
      -1,    -1,    -1,    -1,   196,   197,   198,   199,   200,   201,
     202,   203,   204,   205,   206,   207,    -1,   209,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   217,   218,   219,   220,   221,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    41,    42,
      -1,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,    -1,   121,   122,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,   158,   159,   160,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,    -1,    -1,   190,    -1,    -1,
      -1,    -1,    -1,   196,   197,   198,   199,   200,   201,   202,
     203,   204,   205,   206,   207,    -1,   209,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,   218,   219,    -1,   221,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    38,    -1,    -1,    41,    42,    -1,
      -1,    -1,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,    -1,   121,   122,   123,
     124,   125,   126,    -1,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,    -1,    -1,   190,    -1,    -1,    -1,
      -1,    -1,   196,    -1,   198,    -1,   200,   201,   202,   203,
     204,   205,   206,   207,    -1,   209,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   217,   218,   219,     1,   221,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    -1,    -1,    -1,    -1,    20,    -1,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    -1,    -1,    41,    42,    -1,    -1,
      -1,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,    -1,   121,   122,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   170,   171,   172,   173,   174,
     175,   176,   177,    -1,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   198,    -1,    -1,   201,   202,    -1,    -1,
      -1,    -1,   207,    -1,   209,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,   218,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    -1,    -1,    -1,
      -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,    38,
      -1,    40,    41,    42,    -1,    -1,    -1,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,    -1,   121,   122,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   170,   171,   172,   173,   174,   175,   176,   177,    -1,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   198,
      -1,    -1,   201,   202,    -1,    -1,    -1,    -1,   207,    -1,
     209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,   218,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    41,    42,
      -1,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,    -1,   121,   122,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,   171,   172,
     173,   174,   175,   176,   177,    -1,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   198,    -1,    -1,   201,   202,
      -1,    -1,    -1,    -1,   207,    -1,   209,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,   218,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    -1,
      -1,    -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    -1,    -1,    41,    42,    -1,    -1,    -1,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,    -1,   121,   122,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   170,   171,   172,   173,   174,   175,   176,
     177,    -1,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   198,    -1,    -1,   201,   202,    -1,    -1,    -1,    -1,
     207,    -1,   209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     217,   218,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    -1,    -1,    -1,    -1,    20,
      -1,    22,    -1,    -1,    -1,    -1,    -1,    28,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,
      41,    42,    -1,    -1,    -1,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,    -1,
     121,   122,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,
     171,   172,   173,   174,   175,   176,   177,    -1,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   198,    -1,    -1,
     201,   202,    -1,    -1,    -1,    -1,   207,    -1,   209,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   217,   218,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    -1,    -1,    -1,    -1,    20,    -1,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    -1,    -1,    41,    42,    -1,    -1,
      -1,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,    -1,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   170,   171,   172,   173,   174,
     175,   176,   177,    -1,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   198,    -1,    -1,   201,   202,    -1,    -1,
      -1,    -1,   207,    -1,   209,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,   218,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    -1,    -1,    -1,
      -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    30,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,
      -1,    -1,    41,    42,    -1,    -1,    -1,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,    -1,   121,   122,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   170,   171,   172,   173,   174,   175,   176,   177,    -1,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   198,
      -1,    -1,   201,   202,    -1,    -1,    -1,    -1,   207,    -1,
     209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,   218,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    -1,    -1,    -1,    -1,    -1,    20,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    41,    42,
      -1,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,    -1,   121,   122,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,   171,   172,
     173,   174,   175,   176,   177,    -1,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   198,    -1,    -1,   201,   202,
      -1,    -1,    -1,    -1,   207,    -1,   209,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,   218,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    -1,
      -1,    -1,    -1,    20,    -1,    22,    -1,    -1,    -1,    -1,
      27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    -1,    -1,    41,    42,    -1,    -1,    -1,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,    -1,   121,   122,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   170,   171,   172,   173,   174,   175,   176,
     177,    -1,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   198,    -1,    -1,   201,   202,    -1,    -1,    -1,    -1,
     207,    -1,   209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     217,   218,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    -1,    -1,    -1,    -1,    20,
      -1,    22,    -1,    -1,    -1,    -1,    27,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,
      41,    42,    -1,    -1,    -1,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,    -1,
     121,   122,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,
     171,   172,   173,   174,   175,   176,   177,    -1,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   198,    -1,    -1,
     201,   202,    -1,    -1,    -1,    -1,   207,    -1,   209,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   217,   218,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    -1,    -1,    -1,    -1,    20,    -1,    22,    -1,    -1,
      -1,    -1,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    -1,    -1,    41,    42,    -1,    -1,
      -1,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,    -1,   121,   122,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   170,   171,   172,   173,   174,
     175,   176,   177,    -1,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   198,    -1,    -1,   201,   202,    -1,    -1,
      -1,    -1,   207,    -1,   209,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,   218,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    -1,    -1,    -1,
      -1,    20,    -1,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,
      -1,    -1,    41,    42,    -1,    -1,    -1,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,    -1,   121,   122,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   170,   171,   172,   173,   174,   175,   176,   177,    -1,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   198,
      -1,    -1,   201,   202,    -1,    -1,    -1,    -1,   207,    -1,
     209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,   218,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    -1,    16,    -1,    -1,    -1,    -1,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    41,    42,
      -1,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,    -1,    -1,   121,   122,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,   171,   172,
     173,   174,   175,   176,   177,    -1,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   198,    -1,    -1,   201,   202,
      -1,    -1,    -1,    -1,   207,    -1,   209,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,   218,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    16,
      -1,    -1,    -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    -1,    -1,    41,    42,    -1,    -1,    -1,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,    -1,    -1,   121,   122,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   170,   171,   172,   173,   174,   175,   176,
     177,    -1,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   198,    -1,    -1,   201,   202,    -1,    -1,    -1,    -1,
     207,    -1,   209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     217,   218,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    22,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,
      41,    42,    -1,    -1,    -1,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,    -1,    -1,
     121,   122,   123,   124,   125,   126,    -1,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,
     171,   172,   173,   174,   175,   176,   177,    -1,   179,   180,
     181,   182,   183,   184,   185,   186,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   198,    -1,    -1,
     201,   202,    -1,    -1,    -1,    -1,   207,    -1,   209,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   217,   218,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    14,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    -1,    -1,    41,    42,    -1,    -1,
      -1,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,    -1,    -1,   121,   122,   123,   124,
     125,   126,    -1,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   170,   171,   172,   173,   174,
     175,   176,   177,    -1,   179,   180,   181,   182,   183,   184,
     185,   186,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   198,    -1,    -1,   201,   202,    -1,    -1,
      -1,    -1,   207,    -1,   209,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   217,   218,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    14,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,
      -1,    -1,    41,    42,    -1,    -1,    -1,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
      -1,    -1,   121,   122,   123,   124,   125,   126,    -1,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     139,   140,   141,   142,   143,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   170,   171,   172,   173,   174,   175,   176,   177,    -1,
     179,   180,   181,   182,   183,   184,   185,   186,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   198,
      -1,    -1,   201,   202,    -1,    -1,    -1,    -1,   207,    -1,
     209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   217,   218,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    38,    -1,    -1,    41,    42,
      -1,    -1,    -1,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,    -1,    -1,   121,   122,
     123,   124,   125,   126,    -1,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,   171,   172,
     173,   174,   175,   176,   177,    -1,   179,   180,   181,   182,
     183,   184,   185,   186,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   198,    -1,    -1,   201,   202,
      -1,    -1,    -1,    -1,   207,    -1,   209,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   217,   218,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    14,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,    -1,    -1,   121,   122,   123,   124,   125,   126,
      -1,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   170,   171,   172,   173,   174,   175,   176,
     177,    -1,   179,   180,   181,   182,   183,   184,   185,   186,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   198,    -1,    -1,   201,   202,    -1,    -1,    -1,    -1,
     207,    -1,   209,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     217,   218
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,     1,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    14,    20,    22,    38,    41,    42,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   121,   122,   123,   124,   125,   126,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,   170,   171,
     172,   173,   174,   175,   176,   177,   179,   180,   181,   182,
     183,   184,   185,   186,   198,   201,   202,   207,   209,   217,
     218,   223,   224,   245,   246,   247,   248,   249,   250,   251,
     252,   253,   256,   257,   259,   260,   261,   263,    12,   245,
      31,    40,   245,   248,   245,   252,   250,   250,    12,    12,
      12,    12,    12,    12,    12,    12,    12,    12,    12,    12,
      12,    12,    12,    12,    12,    12,    12,    12,    12,    12,
      12,    12,    12,    12,    12,    12,    12,    12,    12,    12,
      12,    12,    12,    12,    12,    12,    12,    12,    16,   262,
     262,   262,   262,   262,    12,   262,   262,   262,   262,   262,
     262,   262,   262,   262,   262,   245,   246,    12,    12,    12,
      12,    12,    12,    12,    12,    12,    12,    12,    12,    12,
      12,    12,    12,    12,    12,    12,    12,    12,    12,    12,
      12,    12,    12,    12,    12,    12,    12,    12,    12,    12,
      12,    12,    12,    12,    12,    12,    12,    12,    12,    12,
      12,    12,    12,    29,   242,   244,    12,    12,    12,    12,
     234,    12,     0,    39,    40,    18,    23,    27,    37,   191,
      32,    33,    35,    36,    41,    42,   252,    43,    44,    14,
      29,    45,    13,   241,   245,    13,    31,   245,   258,    15,
      15,    19,    21,    24,    25,    26,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,    30,   245,   120,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   241,   241,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   241,   245,
     245,   245,   241,   245,   245,    10,   199,    19,    21,   243,
     245,    10,   245,    10,    13,   233,    10,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   178,   187,   190,   196,   198,   200,
     203,   204,   205,   206,   219,   221,   225,   227,   235,   236,
     237,   238,   239,   240,   241,   247,   253,   248,   248,   249,
      16,   249,    16,   249,   249,   249,   251,   251,   251,   251,
     251,   251,    38,   250,   252,    38,   250,   252,   245,    10,
      38,   250,   252,    13,    19,    12,    15,    19,    28,    31,
     245,   245,    13,    13,    13,    13,    13,    13,    13,    13,
      13,    13,    13,    13,    13,    13,    13,    13,    13,    13,
      13,    13,    13,    13,    13,    13,    13,    13,    13,    13,
      13,    13,    13,    13,    13,    13,    13,    13,    13,    13,
      13,    13,    13,   245,   254,   255,    13,    13,    19,    13,
      19,    19,    13,    13,    13,    19,    19,    19,    13,    13,
      13,    19,    19,    19,    19,    19,    19,    19,    19,    13,
      13,    13,    13,    13,    13,    13,    13,    19,    19,    19,
      19,    19,    19,    19,    19,    19,    13,    19,    19,    19,
      13,    19,    19,    16,   242,    13,    13,    13,    19,    16,
      19,   198,   225,    13,    16,    17,    16,    16,    16,    16,
      16,    16,    16,    16,    16,    16,    16,    16,    16,    16,
      16,    12,    12,    12,    12,    12,    12,    12,    12,    12,
      12,    12,    12,    12,   228,   245,    10,   229,   245,    12,
      12,    10,    12,   197,   199,   216,   226,   227,   230,   231,
     232,    13,    20,    16,    17,    20,    16,    17,    29,   249,
     249,   252,   252,   252,   252,    15,    12,   252,   252,   241,
     241,    28,   245,    31,    15,    15,    15,    34,   254,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   241,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,    28,
      10,   233,   220,   226,   230,   231,   225,   127,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   241,   245,   245,   245,   245,
     245,   245,   245,   245,   241,   245,    10,   245,   188,   191,
     192,   195,   245,    10,   234,    13,   245,   233,    21,   226,
     226,   230,    21,   245,   245,   245,   245,    10,   241,    13,
      19,    15,    12,   225,    13,    19,    19,    19,    19,    19,
      13,    13,    13,    13,    13,    19,    19,    13,    13,    19,
      13,    13,    13,    13,    13,    13,    19,    19,    13,    13,
      19,    13,    13,    13,   245,   220,   226,   220,   226,   230,
     220,   226,   230,   231,    12,    13,    13,    19,    13,    13,
      13,    13,    13,    19,    13,    19,    19,    19,   227,   245,
     245,   227,    13,    19,    13,   230,   226,   231,    13,   245,
     245,   220,   226,   230,   231,   241,   245,   245,   241,   245,
     241,   245,   245,   245,   245,   241,   225,    21,   245,   245,
     220,   226,   245,   220,   226,   220,   226,   230,   245,    27,
      27,   241,   245,    27,   245,    10,   245,   189,   195,   193,
     245,    13,   245,   220,   226,   220,   226,   230,    13,    19,
      13,    13,    13,    13,    13,    19,    19,    13,    13,   220,
     226,   230,   231,   226,    21,    21,   245,    21,   245,   245,
     220,   226,    13,    27,   245,    27,   245,    13,    19,    27,
     245,    13,    13,    19,   227,   227,   245,    19,    21,   245,
     245,   220,   226,   241,   245,   245,   245,   220,   226,   220,
     226,   230,   226,   226,    21,   226,    21,    21,   245,   245,
     245,   245,   245,   245,   194,   195,    12,   207,   208,   209,
     210,   211,   212,   213,   214,   264,   265,   267,   226,    21,
      21,   245,    13,    19,    19,    21,   245,   245,   220,   226,
     226,   226,   226,    21,    19,    19,   245,   227,   264,   266,
     215,    42,   226,   226,    21,   245,   245,   226,    21,    21,
     245,   226,   245,   245,   195,    19,    13,   208,   209,   210,
     211,   212,   213,    27,   226,    13,    19,   226,   226,    21,
      19,    19,   227,   266,   265,   241,   226,   241,   241,    13,
      13,    13,    13
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (myScanner, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, myScanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, myScanner); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *myScanner)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, myScanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void *myScanner;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (myScanner);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *myScanner)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, myScanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void *myScanner;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, myScanner);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, void *myScanner)
#else
static void
yy_reduce_print (yyvsp, yyrule, myScanner)
    YYSTYPE *yyvsp;
    int yyrule;
    void *myScanner;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , myScanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, myScanner); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, void *myScanner)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, myScanner)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    void *myScanner;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (myScanner);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void *myScanner);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */





/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *myScanner)
#else
int
yyparse (myScanner)
    void *myScanner;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1464 of yacc.c  */
#line 394 "miniparser.y"
    {
			    minitree = (yyvsp[(1) - (1)].tree);
			    (yyval.other) = NULL;
			    YYACCEPT;
			  }
    break;

  case 3:

/* Line 1464 of yacc.c  */
#line 403 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 4:

/* Line 1464 of yacc.c  */
#line 407 "miniparser.y"
    {
			    (yyval.tree) = NULL;
			  }
    break;

  case 5:

/* Line 1464 of yacc.c  */
#line 413 "miniparser.y"
    {
			    (yyval.other) = NULL;
			  }
    break;

  case 6:

/* Line 1464 of yacc.c  */
#line 417 "miniparser.y"
    {
			    (yyval.other) = NULL;
			  }
    break;

  case 7:

/* Line 1464 of yacc.c  */
#line 423 "miniparser.y"
    {
			    (yyval.other) = NULL;
			  }
    break;

  case 8:

/* Line 1464 of yacc.c  */
#line 427 "miniparser.y"
    {
			    (yyval.other) = NULL;
			  }
    break;

  case 9:

/* Line 1464 of yacc.c  */
#line 433 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 10:

/* Line 1464 of yacc.c  */
#line 437 "miniparser.y"
    {
			    (yyval.tree) = makeCommandList((yyvsp[(2) - (3)].list));
                          }
    break;

  case 11:

/* Line 1464 of yacc.c  */
#line 441 "miniparser.y"
    {
			    (yyval.tree) = makeCommandList(concatChains((yyvsp[(2) - (4)].list), (yyvsp[(3) - (4)].list)));
                          }
    break;

  case 12:

/* Line 1464 of yacc.c  */
#line 445 "miniparser.y"
    {
			    (yyval.tree) = makeCommandList((yyvsp[(2) - (3)].list));
                          }
    break;

  case 13:

/* Line 1464 of yacc.c  */
#line 449 "miniparser.y"
    {
			    (yyval.tree) = makeNop();
                          }
    break;

  case 14:

/* Line 1464 of yacc.c  */
#line 453 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(2) - (2)].tree);
			  }
    break;

  case 15:

/* Line 1464 of yacc.c  */
#line 457 "miniparser.y"
    {
			    (yyval.tree) = makeWhile((yyvsp[(2) - (4)].tree), (yyvsp[(4) - (4)].tree));
			  }
    break;

  case 16:

/* Line 1464 of yacc.c  */
#line 461 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(2) - (2)].tree);
			  }
    break;

  case 17:

/* Line 1464 of yacc.c  */
#line 467 "miniparser.y"
    {
			    (yyval.tree) = makeIf((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
                          }
    break;

  case 18:

/* Line 1464 of yacc.c  */
#line 471 "miniparser.y"
    {
			    (yyval.tree) = makeIfElse((yyvsp[(1) - (5)].tree),(yyvsp[(3) - (5)].tree),(yyvsp[(5) - (5)].tree));
                          }
    break;

  case 19:

/* Line 1464 of yacc.c  */
#line 479 "miniparser.y"
    {
			    (yyval.tree) = makeFor((yyvsp[(1) - (7)].value), (yyvsp[(3) - (7)].tree), (yyvsp[(5) - (7)].tree), makeConstantDouble(1.0), (yyvsp[(7) - (7)].tree));
			    free((yyvsp[(1) - (7)].value));
                          }
    break;

  case 20:

/* Line 1464 of yacc.c  */
#line 484 "miniparser.y"
    {
			    (yyval.tree) = makeFor((yyvsp[(1) - (9)].value), (yyvsp[(3) - (9)].tree), (yyvsp[(5) - (9)].tree), (yyvsp[(7) - (9)].tree), (yyvsp[(9) - (9)].tree));
			    free((yyvsp[(1) - (9)].value));
                          }
    break;

  case 21:

/* Line 1464 of yacc.c  */
#line 489 "miniparser.y"
    {
			    (yyval.tree) = makeForIn((yyvsp[(1) - (5)].value), (yyvsp[(3) - (5)].tree), (yyvsp[(5) - (5)].tree));
			    free((yyvsp[(1) - (5)].value));
                          }
    break;

  case 22:

/* Line 1464 of yacc.c  */
#line 497 "miniparser.y"
    {
			    (yyval.list) = addElement(NULL, (yyvsp[(1) - (2)].tree));
			  }
    break;

  case 23:

/* Line 1464 of yacc.c  */
#line 501 "miniparser.y"
    {
			    (yyval.list) = addElement((yyvsp[(3) - (3)].list), (yyvsp[(1) - (3)].tree));
			  }
    break;

  case 24:

/* Line 1464 of yacc.c  */
#line 507 "miniparser.y"
    {
			    (yyval.list) = addElement(NULL, (yyvsp[(1) - (2)].tree));
			  }
    break;

  case 25:

/* Line 1464 of yacc.c  */
#line 511 "miniparser.y"
    {
			    (yyval.list) = addElement((yyvsp[(3) - (3)].list), (yyvsp[(1) - (3)].tree));
			  }
    break;

  case 26:

/* Line 1464 of yacc.c  */
#line 517 "miniparser.y"
    {
			    (yyval.tree) = makeVariableDeclaration((yyvsp[(2) - (2)].list));
			  }
    break;

  case 27:

/* Line 1464 of yacc.c  */
#line 524 "miniparser.y"
    {
			    (yyval.list) = addElement(NULL, (yyvsp[(1) - (1)].value));
			  }
    break;

  case 28:

/* Line 1464 of yacc.c  */
#line 528 "miniparser.y"
    {
			    (yyval.list) = addElement((yyvsp[(3) - (3)].list), (yyvsp[(1) - (3)].value));
			  }
    break;

  case 29:

/* Line 1464 of yacc.c  */
#line 534 "miniparser.y"
    {
			    (yyval.tree) = makeProc(NULL, makeCommandList((yyvsp[(4) - (5)].list)), makeUnit());
                          }
    break;

  case 30:

/* Line 1464 of yacc.c  */
#line 538 "miniparser.y"
    {
			    (yyval.tree) = makeProc(NULL, makeCommandList(concatChains((yyvsp[(4) - (6)].list), (yyvsp[(5) - (6)].list))), makeUnit());
                          }
    break;

  case 31:

/* Line 1464 of yacc.c  */
#line 542 "miniparser.y"
    {
			    (yyval.tree) = makeProc(NULL, makeCommandList((yyvsp[(4) - (5)].list)), makeUnit());
                          }
    break;

  case 32:

/* Line 1464 of yacc.c  */
#line 546 "miniparser.y"
    {
			    (yyval.tree) = makeProc(NULL, makeCommandList(addElement(NULL,makeNop())), makeUnit());
                          }
    break;

  case 33:

/* Line 1464 of yacc.c  */
#line 550 "miniparser.y"
    {
			    (yyval.tree) = makeProc(NULL, makeCommandList((yyvsp[(4) - (8)].list)), (yyvsp[(6) - (8)].tree));
                          }
    break;

  case 34:

/* Line 1464 of yacc.c  */
#line 554 "miniparser.y"
    {
			    (yyval.tree) = makeProc(NULL, makeCommandList(concatChains((yyvsp[(4) - (9)].list), (yyvsp[(5) - (9)].list))), (yyvsp[(7) - (9)].tree));
                          }
    break;

  case 35:

/* Line 1464 of yacc.c  */
#line 558 "miniparser.y"
    {
			    (yyval.tree) = makeProc(NULL, makeCommandList((yyvsp[(4) - (8)].list)), (yyvsp[(6) - (8)].tree));
                          }
    break;

  case 36:

/* Line 1464 of yacc.c  */
#line 562 "miniparser.y"
    {
			    (yyval.tree) = makeProc(NULL, makeCommandList(addElement(NULL,makeNop())), (yyvsp[(5) - (7)].tree));
                          }
    break;

  case 37:

/* Line 1464 of yacc.c  */
#line 566 "miniparser.y"
    {
			    (yyval.tree) = makeProc((yyvsp[(2) - (6)].list), makeCommandList((yyvsp[(5) - (6)].list)), makeUnit());
                          }
    break;

  case 38:

/* Line 1464 of yacc.c  */
#line 570 "miniparser.y"
    {
			    (yyval.tree) = makeProc((yyvsp[(2) - (7)].list), makeCommandList(concatChains((yyvsp[(5) - (7)].list), (yyvsp[(6) - (7)].list))), makeUnit());
                          }
    break;

  case 39:

/* Line 1464 of yacc.c  */
#line 574 "miniparser.y"
    {
			    (yyval.tree) = makeProc((yyvsp[(2) - (6)].list), makeCommandList((yyvsp[(5) - (6)].list)), makeUnit());
                          }
    break;

  case 40:

/* Line 1464 of yacc.c  */
#line 578 "miniparser.y"
    {
			    (yyval.tree) = makeProc((yyvsp[(2) - (5)].list), makeCommandList(addElement(NULL,makeNop())), makeUnit());
                          }
    break;

  case 41:

/* Line 1464 of yacc.c  */
#line 582 "miniparser.y"
    {
			    (yyval.tree) = makeProc((yyvsp[(2) - (9)].list), makeCommandList((yyvsp[(5) - (9)].list)), (yyvsp[(7) - (9)].tree));
                          }
    break;

  case 42:

/* Line 1464 of yacc.c  */
#line 586 "miniparser.y"
    {
			    (yyval.tree) = makeProc((yyvsp[(2) - (10)].list), makeCommandList(concatChains((yyvsp[(5) - (10)].list), (yyvsp[(6) - (10)].list))), (yyvsp[(8) - (10)].tree));
                          }
    break;

  case 43:

/* Line 1464 of yacc.c  */
#line 590 "miniparser.y"
    {
			    (yyval.tree) = makeProc((yyvsp[(2) - (9)].list), makeCommandList((yyvsp[(5) - (9)].list)), (yyvsp[(7) - (9)].tree));
                          }
    break;

  case 44:

/* Line 1464 of yacc.c  */
#line 594 "miniparser.y"
    {
			    (yyval.tree) = makeProc((yyvsp[(2) - (8)].list), makeCommandList(addElement(NULL, makeNop())), (yyvsp[(6) - (8)].tree));
                          }
    break;

  case 45:

/* Line 1464 of yacc.c  */
#line 598 "miniparser.y"
    {
			    (yyval.tree) = makeProcIllim((yyvsp[(2) - (8)].value), makeCommandList((yyvsp[(7) - (8)].list)), makeUnit());
                          }
    break;

  case 46:

/* Line 1464 of yacc.c  */
#line 602 "miniparser.y"
    {
			    (yyval.tree) = makeProcIllim((yyvsp[(2) - (9)].value), makeCommandList(concatChains((yyvsp[(7) - (9)].list), (yyvsp[(8) - (9)].list))), makeUnit());
                          }
    break;

  case 47:

/* Line 1464 of yacc.c  */
#line 606 "miniparser.y"
    {
			    (yyval.tree) = makeProcIllim((yyvsp[(2) - (8)].value), makeCommandList((yyvsp[(7) - (8)].list)), makeUnit());
                          }
    break;

  case 48:

/* Line 1464 of yacc.c  */
#line 610 "miniparser.y"
    {
			    (yyval.tree) = makeProcIllim((yyvsp[(2) - (7)].value), makeCommandList(addElement(NULL,makeNop())), makeUnit());
                          }
    break;

  case 49:

/* Line 1464 of yacc.c  */
#line 614 "miniparser.y"
    {
			    (yyval.tree) = makeProcIllim((yyvsp[(2) - (11)].value), makeCommandList((yyvsp[(7) - (11)].list)), (yyvsp[(9) - (11)].tree));
                          }
    break;

  case 50:

/* Line 1464 of yacc.c  */
#line 618 "miniparser.y"
    {
			    (yyval.tree) = makeProcIllim((yyvsp[(2) - (12)].value), makeCommandList(concatChains((yyvsp[(7) - (12)].list), (yyvsp[(8) - (12)].list))), (yyvsp[(10) - (12)].tree));
                          }
    break;

  case 51:

/* Line 1464 of yacc.c  */
#line 622 "miniparser.y"
    {
			    (yyval.tree) = makeProcIllim((yyvsp[(2) - (11)].value), makeCommandList((yyvsp[(7) - (11)].list)), (yyvsp[(9) - (11)].tree));
                          }
    break;

  case 52:

/* Line 1464 of yacc.c  */
#line 626 "miniparser.y"
    {
			    (yyval.tree) = makeProcIllim((yyvsp[(2) - (10)].value), makeCommandList(addElement(NULL, makeNop())), (yyvsp[(8) - (10)].tree));
                          }
    break;

  case 53:

/* Line 1464 of yacc.c  */
#line 634 "miniparser.y"
    {
			    (yyval.tree) = makeQuit();
			  }
    break;

  case 54:

/* Line 1464 of yacc.c  */
#line 638 "miniparser.y"
    {
			    (yyval.tree) = makeFalseRestart();
			  }
    break;

  case 55:

/* Line 1464 of yacc.c  */
#line 642 "miniparser.y"
    {
			    (yyval.tree) = makeNop();
			  }
    break;

  case 56:

/* Line 1464 of yacc.c  */
#line 646 "miniparser.y"
    {
			    (yyval.tree) = makeNopArg((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 57:

/* Line 1464 of yacc.c  */
#line 650 "miniparser.y"
    {
			    (yyval.tree) = makeNopArg(makeDefault());
			  }
    break;

  case 58:

/* Line 1464 of yacc.c  */
#line 654 "miniparser.y"
    {
			    (yyval.tree) = makePrint((yyvsp[(3) - (4)].list));
			  }
    break;

  case 59:

/* Line 1464 of yacc.c  */
#line 658 "miniparser.y"
    {
			    (yyval.tree) = makeNewFilePrint((yyvsp[(6) - (6)].tree), (yyvsp[(3) - (6)].list));
			  }
    break;

  case 60:

/* Line 1464 of yacc.c  */
#line 662 "miniparser.y"
    {
			    (yyval.tree) = makeAppendFilePrint((yyvsp[(7) - (7)].tree), (yyvsp[(3) - (7)].list));
			  }
    break;

  case 61:

/* Line 1464 of yacc.c  */
#line 666 "miniparser.y"
    {
			    (yyval.tree) = makePlot(addElement((yyvsp[(5) - (6)].list), (yyvsp[(3) - (6)].tree)));
			  }
    break;

  case 62:

/* Line 1464 of yacc.c  */
#line 670 "miniparser.y"
    {
			    (yyval.tree) = makePrintHexa((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 63:

/* Line 1464 of yacc.c  */
#line 674 "miniparser.y"
    {
			    (yyval.tree) = makePrintFloat((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 64:

/* Line 1464 of yacc.c  */
#line 678 "miniparser.y"
    {
			    (yyval.tree) = makePrintBinary((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 65:

/* Line 1464 of yacc.c  */
#line 682 "miniparser.y"
    {
			    (yyval.tree) = makePrintExpansion((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 66:

/* Line 1464 of yacc.c  */
#line 686 "miniparser.y"
    {
			    (yyval.tree) = makeBashExecute((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 67:

/* Line 1464 of yacc.c  */
#line 690 "miniparser.y"
    {
			    (yyval.tree) = makeExternalPlot(addElement(addElement(addElement(addElement((yyvsp[(11) - (12)].list),(yyvsp[(9) - (12)].tree)),(yyvsp[(7) - (12)].tree)),(yyvsp[(5) - (12)].tree)),(yyvsp[(3) - (12)].tree)));
			  }
    break;

  case 68:

/* Line 1464 of yacc.c  */
#line 694 "miniparser.y"
    {
			    (yyval.tree) = makeWrite((yyvsp[(3) - (4)].list));
			  }
    break;

  case 69:

/* Line 1464 of yacc.c  */
#line 698 "miniparser.y"
    {
			    (yyval.tree) = makeNewFileWrite((yyvsp[(6) - (6)].tree), (yyvsp[(3) - (6)].list));
			  }
    break;

  case 70:

/* Line 1464 of yacc.c  */
#line 702 "miniparser.y"
    {
			    (yyval.tree) = makeAppendFileWrite((yyvsp[(7) - (7)].tree), (yyvsp[(3) - (7)].list));
			  }
    break;

  case 71:

/* Line 1464 of yacc.c  */
#line 706 "miniparser.y"
    {
			    (yyval.tree) = makeAsciiPlot((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 72:

/* Line 1464 of yacc.c  */
#line 710 "miniparser.y"
    {
			    (yyval.tree) = makePrintXml((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 73:

/* Line 1464 of yacc.c  */
#line 714 "miniparser.y"
    {
			    (yyval.tree) = makeExecute((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 74:

/* Line 1464 of yacc.c  */
#line 718 "miniparser.y"
    {
			    (yyval.tree) = makePrintXmlNewFile((yyvsp[(3) - (6)].tree),(yyvsp[(6) - (6)].tree));
			  }
    break;

  case 75:

/* Line 1464 of yacc.c  */
#line 722 "miniparser.y"
    {
			    (yyval.tree) = makePrintXmlAppendFile((yyvsp[(3) - (7)].tree),(yyvsp[(7) - (7)].tree));
			  }
    break;

  case 76:

/* Line 1464 of yacc.c  */
#line 726 "miniparser.y"
    {
			    (yyval.tree) = makeWorstCase(addElement(addElement(addElement(addElement((yyvsp[(11) - (12)].list), (yyvsp[(9) - (12)].tree)), (yyvsp[(7) - (12)].tree)), (yyvsp[(5) - (12)].tree)), (yyvsp[(3) - (12)].tree)));
			  }
    break;

  case 77:

/* Line 1464 of yacc.c  */
#line 730 "miniparser.y"
    {
			    (yyval.tree) = makeRename((yyvsp[(3) - (6)].value), (yyvsp[(5) - (6)].value));
			    free((yyvsp[(3) - (6)].value));
			    free((yyvsp[(5) - (6)].value));
			  }
    break;

  case 78:

/* Line 1464 of yacc.c  */
#line 736 "miniparser.y"
    {
			    (yyval.tree) = makeExternalProc((yyvsp[(3) - (11)].value), (yyvsp[(5) - (11)].tree), addElement((yyvsp[(7) - (11)].list), (yyvsp[(10) - (11)].integerval)));
			    free((yyvsp[(3) - (11)].value));
			  }
    break;

  case 79:

/* Line 1464 of yacc.c  */
#line 741 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 80:

/* Line 1464 of yacc.c  */
#line 745 "miniparser.y"
    {
			    (yyval.tree) = makeAutoprint((yyvsp[(1) - (1)].list));
			  }
    break;

  case 81:

/* Line 1464 of yacc.c  */
#line 749 "miniparser.y"
    {
			    (yyval.tree) = makeAssignment((yyvsp[(2) - (3)].value), (yyvsp[(3) - (3)].tree));
			    free((yyvsp[(2) - (3)].value));
			  }
    break;

  case 82:

/* Line 1464 of yacc.c  */
#line 756 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 83:

/* Line 1464 of yacc.c  */
#line 760 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (2)].tree);
			  }
    break;

  case 84:

/* Line 1464 of yacc.c  */
#line 764 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 85:

/* Line 1464 of yacc.c  */
#line 768 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (2)].tree);
			  }
    break;

  case 86:

/* Line 1464 of yacc.c  */
#line 774 "miniparser.y"
    {
			    (yyval.tree) = makeAssignment((yyvsp[(1) - (3)].value), (yyvsp[(3) - (3)].tree));
			    free((yyvsp[(1) - (3)].value));
			  }
    break;

  case 87:

/* Line 1464 of yacc.c  */
#line 779 "miniparser.y"
    {
			    (yyval.tree) = makeFloatAssignment((yyvsp[(1) - (3)].value), (yyvsp[(3) - (3)].tree));
			    free((yyvsp[(1) - (3)].value));
			  }
    break;

  case 88:

/* Line 1464 of yacc.c  */
#line 784 "miniparser.y"
    {
			    (yyval.tree) = makeLibraryBinding((yyvsp[(1) - (6)].value), (yyvsp[(5) - (6)].tree));
			    free((yyvsp[(1) - (6)].value));
			  }
    break;

  case 89:

/* Line 1464 of yacc.c  */
#line 789 "miniparser.y"
    {
			    (yyval.tree) = makeAssignmentInIndexing((yyvsp[(1) - (3)].dblnode)->a,(yyvsp[(1) - (3)].dblnode)->b,(yyvsp[(3) - (3)].tree));
			    free((yyvsp[(1) - (3)].dblnode));
			  }
    break;

  case 90:

/* Line 1464 of yacc.c  */
#line 794 "miniparser.y"
    {
			    (yyval.tree) = makeFloatAssignmentInIndexing((yyvsp[(1) - (3)].dblnode)->a,(yyvsp[(1) - (3)].dblnode)->b,(yyvsp[(3) - (3)].tree));
			    free((yyvsp[(1) - (3)].dblnode));
			  }
    break;

  case 91:

/* Line 1464 of yacc.c  */
#line 799 "miniparser.y"
    {
			    (yyval.tree) = makeProtoAssignmentInStructure((yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree));
			  }
    break;

  case 92:

/* Line 1464 of yacc.c  */
#line 803 "miniparser.y"
    {
			    (yyval.tree) = makeProtoFloatAssignmentInStructure((yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree));
			  }
    break;

  case 93:

/* Line 1464 of yacc.c  */
#line 809 "miniparser.y"
    {
			    (yyval.tree) = makeStructAccess((yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].value));
			    free((yyvsp[(3) - (3)].value));
			  }
    break;

  case 94:

/* Line 1464 of yacc.c  */
#line 816 "miniparser.y"
    {
			    (yyval.tree) = makePrecAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 95:

/* Line 1464 of yacc.c  */
#line 820 "miniparser.y"
    {
			    (yyval.tree) = makePointsAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 96:

/* Line 1464 of yacc.c  */
#line 824 "miniparser.y"
    {
			    (yyval.tree) = makeDiamAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 97:

/* Line 1464 of yacc.c  */
#line 828 "miniparser.y"
    {
			    (yyval.tree) = makeDisplayAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 98:

/* Line 1464 of yacc.c  */
#line 832 "miniparser.y"
    {
			    (yyval.tree) = makeVerbosityAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 99:

/* Line 1464 of yacc.c  */
#line 836 "miniparser.y"
    {
			    (yyval.tree) = makeCanonicalAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 100:

/* Line 1464 of yacc.c  */
#line 840 "miniparser.y"
    {
			    (yyval.tree) = makeAutoSimplifyAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 101:

/* Line 1464 of yacc.c  */
#line 844 "miniparser.y"
    {
			    (yyval.tree) = makeTaylorRecursAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 102:

/* Line 1464 of yacc.c  */
#line 848 "miniparser.y"
    {
			    (yyval.tree) = makeTimingAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 103:

/* Line 1464 of yacc.c  */
#line 852 "miniparser.y"
    {
			    (yyval.tree) = makeFullParenAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 104:

/* Line 1464 of yacc.c  */
#line 856 "miniparser.y"
    {
			    (yyval.tree) = makeMidpointAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 105:

/* Line 1464 of yacc.c  */
#line 860 "miniparser.y"
    {
			    (yyval.tree) = makeDieOnErrorAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 106:

/* Line 1464 of yacc.c  */
#line 864 "miniparser.y"
    {
			    (yyval.tree) = makeRationalModeAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 107:

/* Line 1464 of yacc.c  */
#line 868 "miniparser.y"
    {
			    (yyval.tree) = makeSuppressWarningsAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 108:

/* Line 1464 of yacc.c  */
#line 872 "miniparser.y"
    {
			    (yyval.tree) = makeHopitalRecursAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 109:

/* Line 1464 of yacc.c  */
#line 878 "miniparser.y"
    {
			    (yyval.tree) = makePrecStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 110:

/* Line 1464 of yacc.c  */
#line 882 "miniparser.y"
    {
			    (yyval.tree) = makePointsStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 111:

/* Line 1464 of yacc.c  */
#line 886 "miniparser.y"
    {
			    (yyval.tree) = makeDiamStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 112:

/* Line 1464 of yacc.c  */
#line 890 "miniparser.y"
    {
			    (yyval.tree) = makeDisplayStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 113:

/* Line 1464 of yacc.c  */
#line 894 "miniparser.y"
    {
			    (yyval.tree) = makeVerbosityStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 114:

/* Line 1464 of yacc.c  */
#line 898 "miniparser.y"
    {
			    (yyval.tree) = makeCanonicalStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 115:

/* Line 1464 of yacc.c  */
#line 902 "miniparser.y"
    {
			    (yyval.tree) = makeAutoSimplifyStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 116:

/* Line 1464 of yacc.c  */
#line 906 "miniparser.y"
    {
			    (yyval.tree) = makeTaylorRecursStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 117:

/* Line 1464 of yacc.c  */
#line 910 "miniparser.y"
    {
			    (yyval.tree) = makeTimingStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 118:

/* Line 1464 of yacc.c  */
#line 914 "miniparser.y"
    {
			    (yyval.tree) = makeFullParenStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 119:

/* Line 1464 of yacc.c  */
#line 918 "miniparser.y"
    {
			    (yyval.tree) = makeMidpointStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 120:

/* Line 1464 of yacc.c  */
#line 922 "miniparser.y"
    {
			    (yyval.tree) = makeDieOnErrorStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 121:

/* Line 1464 of yacc.c  */
#line 926 "miniparser.y"
    {
			    (yyval.tree) = makeRationalModeStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 122:

/* Line 1464 of yacc.c  */
#line 930 "miniparser.y"
    {
			    (yyval.tree) = makeSuppressWarningsStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 123:

/* Line 1464 of yacc.c  */
#line 934 "miniparser.y"
    {
			    (yyval.tree) = makeHopitalRecursStillAssign((yyvsp[(3) - (3)].tree));
			  }
    break;

  case 124:

/* Line 1464 of yacc.c  */
#line 940 "miniparser.y"
    {
			    (yyval.list) = addElement(NULL, (yyvsp[(1) - (1)].tree));
			  }
    break;

  case 125:

/* Line 1464 of yacc.c  */
#line 944 "miniparser.y"
    {
			    (yyval.list) = addElement((yyvsp[(3) - (3)].list), (yyvsp[(1) - (3)].tree));
			  }
    break;

  case 126:

/* Line 1464 of yacc.c  */
#line 950 "miniparser.y"
    {
			    (yyval.list) = addElement(NULL, (yyvsp[(1) - (1)].association));
			  }
    break;

  case 127:

/* Line 1464 of yacc.c  */
#line 954 "miniparser.y"
    {
			    (yyval.list) = addElement((yyvsp[(3) - (3)].list), (yyvsp[(1) - (3)].association));
			  }
    break;

  case 128:

/* Line 1464 of yacc.c  */
#line 960 "miniparser.y"
    {
			    (yyval.other) = NULL;
			  }
    break;

  case 129:

/* Line 1464 of yacc.c  */
#line 964 "miniparser.y"
    {
			    (yyval.other) = NULL;
			  }
    break;

  case 130:

/* Line 1464 of yacc.c  */
#line 970 "miniparser.y"
    {
			    (yyval.association) = (entry *) safeMalloc(sizeof(entry));
			    (yyval.association)->name = (char *) safeCalloc(strlen((yyvsp[(2) - (4)].value)) + 1, sizeof(char));
			    strcpy((yyval.association)->name,(yyvsp[(2) - (4)].value));
			    free((yyvsp[(2) - (4)].value));
			    (yyval.association)->value = (void *) ((yyvsp[(4) - (4)].tree));
			  }
    break;

  case 131:

/* Line 1464 of yacc.c  */
#line 980 "miniparser.y"
    {
			   (yyval.tree) = (yyvsp[(1) - (1)].tree);
			 }
    break;

  case 132:

/* Line 1464 of yacc.c  */
#line 984 "miniparser.y"
    {
			    (yyval.tree) = makeMatch((yyvsp[(2) - (4)].tree),(yyvsp[(4) - (4)].list));
			  }
    break;

  case 133:

/* Line 1464 of yacc.c  */
#line 990 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 134:

/* Line 1464 of yacc.c  */
#line 994 "miniparser.y"
    {
			    (yyval.tree) = makeAnd((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 135:

/* Line 1464 of yacc.c  */
#line 998 "miniparser.y"
    {
			    (yyval.tree) = makeOr((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 136:

/* Line 1464 of yacc.c  */
#line 1002 "miniparser.y"
    {
			    (yyval.tree) = makeNegation((yyvsp[(2) - (2)].tree));
			  }
    break;

  case 137:

/* Line 1464 of yacc.c  */
#line 1008 "miniparser.y"
    {
			    (yyval.dblnode) = (doubleNode *) safeMalloc(sizeof(doubleNode));
			    (yyval.dblnode)->a = (yyvsp[(1) - (4)].tree);
			    (yyval.dblnode)->b = (yyvsp[(3) - (4)].tree);
			  }
    break;

  case 138:

/* Line 1464 of yacc.c  */
#line 1017 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 139:

/* Line 1464 of yacc.c  */
#line 1021 "miniparser.y"
    {
			    (yyval.tree) = makeCompareEqual((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 140:

/* Line 1464 of yacc.c  */
#line 1025 "miniparser.y"
    {
			    (yyval.tree) = makeCompareIn((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 141:

/* Line 1464 of yacc.c  */
#line 1029 "miniparser.y"
    {
			    (yyval.tree) = makeCompareLess((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 142:

/* Line 1464 of yacc.c  */
#line 1033 "miniparser.y"
    {
			    (yyval.tree) = makeCompareGreater((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 143:

/* Line 1464 of yacc.c  */
#line 1037 "miniparser.y"
    {
			    (yyval.tree) = makeCompareLessEqual((yyvsp[(1) - (4)].tree), (yyvsp[(4) - (4)].tree));
			  }
    break;

  case 144:

/* Line 1464 of yacc.c  */
#line 1041 "miniparser.y"
    {
			    (yyval.tree) = makeCompareGreaterEqual((yyvsp[(1) - (4)].tree), (yyvsp[(4) - (4)].tree));
			  }
    break;

  case 145:

/* Line 1464 of yacc.c  */
#line 1045 "miniparser.y"
    {
			    (yyval.tree) = makeCompareNotEqual((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 146:

/* Line 1464 of yacc.c  */
#line 1051 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 147:

/* Line 1464 of yacc.c  */
#line 1055 "miniparser.y"
    {
			    (yyval.tree) = makeAdd((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 148:

/* Line 1464 of yacc.c  */
#line 1059 "miniparser.y"
    {
			    (yyval.tree) = makeSub((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 149:

/* Line 1464 of yacc.c  */
#line 1063 "miniparser.y"
    {
			    (yyval.tree) = makeConcat((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 150:

/* Line 1464 of yacc.c  */
#line 1067 "miniparser.y"
    {
			    (yyval.tree) = makeAddToList((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 151:

/* Line 1464 of yacc.c  */
#line 1071 "miniparser.y"
    {
			    (yyval.tree) = makePrepend((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 152:

/* Line 1464 of yacc.c  */
#line 1075 "miniparser.y"
    {
			    (yyval.tree) = makeAppend((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 153:

/* Line 1464 of yacc.c  */
#line 1081 "miniparser.y"
    {
                            (yyval.count) = 0;
                          }
    break;

  case 154:

/* Line 1464 of yacc.c  */
#line 1085 "miniparser.y"
    {
                            (yyval.count) = 1;
                          }
    break;

  case 155:

/* Line 1464 of yacc.c  */
#line 1089 "miniparser.y"
    {
  	                    (yyval.count) = (yyvsp[(2) - (2)].count);
  	                  }
    break;

  case 156:

/* Line 1464 of yacc.c  */
#line 1093 "miniparser.y"
    {
  	                    (yyval.count) = (yyvsp[(2) - (2)].count)+1;
                          }
    break;

  case 157:

/* Line 1464 of yacc.c  */
#line 1100 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
                          }
    break;

  case 158:

/* Line 1464 of yacc.c  */
#line 1104 "miniparser.y"
    {
			    tempNode = (yyvsp[(2) - (2)].tree);
			    for (tempInteger=0;tempInteger<(yyvsp[(1) - (2)].count);tempInteger++)
			      tempNode = makeNeg(tempNode);
			    (yyval.tree) = tempNode;
			  }
    break;

  case 159:

/* Line 1464 of yacc.c  */
#line 1111 "miniparser.y"
    {
			    (yyval.tree) = makeEvalConst((yyvsp[(2) - (2)].tree));
                          }
    break;

  case 160:

/* Line 1464 of yacc.c  */
#line 1115 "miniparser.y"
    {
			    (yyval.tree) = makeMul((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
                          }
    break;

  case 161:

/* Line 1464 of yacc.c  */
#line 1119 "miniparser.y"
    {
			    (yyval.tree) = makeDiv((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
                          }
    break;

  case 162:

/* Line 1464 of yacc.c  */
#line 1123 "miniparser.y"
    {
			    tempNode = (yyvsp[(4) - (4)].tree);
			    for (tempInteger=0;tempInteger<(yyvsp[(3) - (4)].count);tempInteger++)
			      tempNode = makeNeg(tempNode);
			    (yyval.tree) = makeMul((yyvsp[(1) - (4)].tree), tempNode);
			  }
    break;

  case 163:

/* Line 1464 of yacc.c  */
#line 1130 "miniparser.y"
    {
			    tempNode = (yyvsp[(4) - (4)].tree);
			    for (tempInteger=0;tempInteger<(yyvsp[(3) - (4)].count);tempInteger++)
			      tempNode = makeNeg(tempNode);
			    (yyval.tree) = makeDiv((yyvsp[(1) - (4)].tree), tempNode);
			  }
    break;

  case 164:

/* Line 1464 of yacc.c  */
#line 1137 "miniparser.y"
    {
			    (yyval.tree) = makeMul((yyvsp[(1) - (4)].tree), makeEvalConst((yyvsp[(4) - (4)].tree)));
			  }
    break;

  case 165:

/* Line 1464 of yacc.c  */
#line 1141 "miniparser.y"
    {
			    (yyval.tree) = makeDiv((yyvsp[(1) - (4)].tree), makeEvalConst((yyvsp[(4) - (4)].tree)));
			  }
    break;

  case 166:

/* Line 1464 of yacc.c  */
#line 1147 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
                          }
    break;

  case 167:

/* Line 1464 of yacc.c  */
#line 1151 "miniparser.y"
    {
			    (yyval.tree) = makePow((yyvsp[(1) - (3)].tree), (yyvsp[(3) - (3)].tree));
                          }
    break;

  case 168:

/* Line 1464 of yacc.c  */
#line 1155 "miniparser.y"
    {
			    tempNode = (yyvsp[(4) - (4)].tree);
			    for (tempInteger=0;tempInteger<(yyvsp[(3) - (4)].count);tempInteger++)
			      tempNode = makeNeg(tempNode);
			    (yyval.tree) = makePow((yyvsp[(1) - (4)].tree), tempNode);
			  }
    break;

  case 169:

/* Line 1464 of yacc.c  */
#line 1162 "miniparser.y"
    {
			    (yyval.tree) = makePow((yyvsp[(1) - (4)].tree), makeEvalConst((yyvsp[(4) - (4)].tree)));
			  }
    break;

  case 170:

/* Line 1464 of yacc.c  */
#line 1169 "miniparser.y"
    {
			    (yyval.tree) = makeOn();
			  }
    break;

  case 171:

/* Line 1464 of yacc.c  */
#line 1173 "miniparser.y"
    {
			    (yyval.tree) = makeOff();
			  }
    break;

  case 172:

/* Line 1464 of yacc.c  */
#line 1177 "miniparser.y"
    {
			    (yyval.tree) = makeDyadic();
			  }
    break;

  case 173:

/* Line 1464 of yacc.c  */
#line 1181 "miniparser.y"
    {
			    (yyval.tree) = makePowers();
			  }
    break;

  case 174:

/* Line 1464 of yacc.c  */
#line 1185 "miniparser.y"
    {
			    (yyval.tree) = makeBinaryThing();
			  }
    break;

  case 175:

/* Line 1464 of yacc.c  */
#line 1189 "miniparser.y"
    {
			    (yyval.tree) = makeHexadecimalThing();
			  }
    break;

  case 176:

/* Line 1464 of yacc.c  */
#line 1193 "miniparser.y"
    {
			    (yyval.tree) = makeFile();
			  }
    break;

  case 177:

/* Line 1464 of yacc.c  */
#line 1197 "miniparser.y"
    {
			    (yyval.tree) = makePostscript();
			  }
    break;

  case 178:

/* Line 1464 of yacc.c  */
#line 1201 "miniparser.y"
    {
			    (yyval.tree) = makePostscriptFile();
			  }
    break;

  case 179:

/* Line 1464 of yacc.c  */
#line 1205 "miniparser.y"
    {
			    (yyval.tree) = makePerturb();
			  }
    break;

  case 180:

/* Line 1464 of yacc.c  */
#line 1209 "miniparser.y"
    {
			    (yyval.tree) = makeRoundDown();
			  }
    break;

  case 181:

/* Line 1464 of yacc.c  */
#line 1213 "miniparser.y"
    {
			    (yyval.tree) = makeRoundUp();
			  }
    break;

  case 182:

/* Line 1464 of yacc.c  */
#line 1217 "miniparser.y"
    {
			    (yyval.tree) = makeRoundToZero();
			  }
    break;

  case 183:

/* Line 1464 of yacc.c  */
#line 1221 "miniparser.y"
    {
			    (yyval.tree) = makeRoundToNearest();
			  }
    break;

  case 184:

/* Line 1464 of yacc.c  */
#line 1225 "miniparser.y"
    {
			    (yyval.tree) = makeHonorCoeff();
			  }
    break;

  case 185:

/* Line 1464 of yacc.c  */
#line 1229 "miniparser.y"
    {
			    (yyval.tree) = makeTrue();
			  }
    break;

  case 186:

/* Line 1464 of yacc.c  */
#line 1233 "miniparser.y"
    {
			    (yyval.tree) = makeUnit();
			  }
    break;

  case 187:

/* Line 1464 of yacc.c  */
#line 1237 "miniparser.y"
    {
			    (yyval.tree) = makeFalse();
			  }
    break;

  case 188:

/* Line 1464 of yacc.c  */
#line 1241 "miniparser.y"
    {
			    (yyval.tree) = makeDefault();
			  }
    break;

  case 189:

/* Line 1464 of yacc.c  */
#line 1245 "miniparser.y"
    {
			    (yyval.tree) = makeDecimal();
			  }
    break;

  case 190:

/* Line 1464 of yacc.c  */
#line 1249 "miniparser.y"
    {
			    (yyval.tree) = makeAbsolute();
			  }
    break;

  case 191:

/* Line 1464 of yacc.c  */
#line 1253 "miniparser.y"
    {
			    (yyval.tree) = makeRelative();
			  }
    break;

  case 192:

/* Line 1464 of yacc.c  */
#line 1257 "miniparser.y"
    {
			    (yyval.tree) = makeFixed();
			  }
    break;

  case 193:

/* Line 1464 of yacc.c  */
#line 1261 "miniparser.y"
    {
			    (yyval.tree) = makeFloating();
			  }
    break;

  case 194:

/* Line 1464 of yacc.c  */
#line 1265 "miniparser.y"
    {
			    (yyval.tree) = makeError();
			  }
    break;

  case 195:

/* Line 1464 of yacc.c  */
#line 1269 "miniparser.y"
    {
			    (yyval.tree) = makeDoubleSymbol();
			  }
    break;

  case 196:

/* Line 1464 of yacc.c  */
#line 1273 "miniparser.y"
    {
			    (yyval.tree) = makeSingleSymbol();
			  }
    break;

  case 197:

/* Line 1464 of yacc.c  */
#line 1277 "miniparser.y"
    {
			    (yyval.tree) = makeQuadSymbol();
			  }
    break;

  case 198:

/* Line 1464 of yacc.c  */
#line 1281 "miniparser.y"
    {
			    (yyval.tree) = makeHalfPrecisionSymbol();
			  }
    break;

  case 199:

/* Line 1464 of yacc.c  */
#line 1285 "miniparser.y"
    {
			    (yyval.tree) = makeDoubleextendedSymbol();
			  }
    break;

  case 200:

/* Line 1464 of yacc.c  */
#line 1289 "miniparser.y"
    {
			    (yyval.tree) = makeDoubleDoubleSymbol();
			  }
    break;

  case 201:

/* Line 1464 of yacc.c  */
#line 1293 "miniparser.y"
    {
			    (yyval.tree) = makeTripleDoubleSymbol();
			  }
    break;

  case 202:

/* Line 1464 of yacc.c  */
#line 1297 "miniparser.y"
    {
			    tempString = safeCalloc(strlen((yyvsp[(1) - (1)].value)) + 1, sizeof(char));
			    strcpy(tempString, (yyvsp[(1) - (1)].value));
			    free((yyvsp[(1) - (1)].value));
			    tempString2 = safeCalloc(strlen(tempString) + 1, sizeof(char));
			    strcpy(tempString2, tempString);
			    free(tempString);
			    (yyval.tree) = makeString(tempString2);
			    free(tempString2);
			  }
    break;

  case 203:

/* Line 1464 of yacc.c  */
#line 1308 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 204:

/* Line 1464 of yacc.c  */
#line 1312 "miniparser.y"
    {
			    (yyval.tree) = makeTableAccess((yyvsp[(1) - (1)].value));
			    free((yyvsp[(1) - (1)].value));
			  }
    break;

  case 205:

/* Line 1464 of yacc.c  */
#line 1317 "miniparser.y"
    {
			    (yyval.tree) = makeIsBound((yyvsp[(3) - (4)].value));
			    free((yyvsp[(3) - (4)].value));
			  }
    break;

  case 206:

/* Line 1464 of yacc.c  */
#line 1322 "miniparser.y"
    {
			    (yyval.tree) = makeTableAccessWithSubstitute((yyvsp[(1) - (4)].value), (yyvsp[(3) - (4)].list));
			    free((yyvsp[(1) - (4)].value));
			  }
    break;

  case 207:

/* Line 1464 of yacc.c  */
#line 1327 "miniparser.y"
    {
			    (yyval.tree) = makeTableAccessWithSubstitute((yyvsp[(1) - (3)].value), NULL);
			    free((yyvsp[(1) - (3)].value));
			  }
    break;

  case 208:

/* Line 1464 of yacc.c  */
#line 1332 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 209:

/* Line 1464 of yacc.c  */
#line 1336 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 210:

/* Line 1464 of yacc.c  */
#line 1340 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 211:

/* Line 1464 of yacc.c  */
#line 1344 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 212:

/* Line 1464 of yacc.c  */
#line 1348 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(2) - (3)].tree);
			  }
    break;

  case 213:

/* Line 1464 of yacc.c  */
#line 1352 "miniparser.y"
    {
			    (yyval.tree) = makeStructure((yyvsp[(2) - (3)].list));
			  }
    break;

  case 214:

/* Line 1464 of yacc.c  */
#line 1356 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(1) - (1)].tree);
			  }
    break;

  case 215:

/* Line 1464 of yacc.c  */
#line 1360 "miniparser.y"
    {
			    (yyval.tree) = makeIndex((yyvsp[(1) - (1)].dblnode)->a, (yyvsp[(1) - (1)].dblnode)->b);
			    free((yyvsp[(1) - (1)].dblnode));
			  }
    break;

  case 216:

/* Line 1464 of yacc.c  */
#line 1365 "miniparser.y"
    {
			    (yyval.tree) = makeStructAccess((yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].value));
			    free((yyvsp[(3) - (3)].value));
			  }
    break;

  case 217:

/* Line 1464 of yacc.c  */
#line 1370 "miniparser.y"
    {
			    (yyval.tree) = makeApply(makeStructAccess((yyvsp[(1) - (6)].tree),(yyvsp[(3) - (6)].value)),(yyvsp[(5) - (6)].list));
			    free((yyvsp[(3) - (6)].value));
			  }
    break;

  case 218:

/* Line 1464 of yacc.c  */
#line 1375 "miniparser.y"
    {
			    (yyval.tree) = makeApply((yyvsp[(2) - (6)].tree),(yyvsp[(5) - (6)].list));
			  }
    break;

  case 219:

/* Line 1464 of yacc.c  */
#line 1379 "miniparser.y"
    {
			    (yyval.tree) = (yyvsp[(2) - (2)].tree);
			  }
    break;

  case 220:

/* Line 1464 of yacc.c  */
#line 1383 "miniparser.y"
    {
			    (yyval.tree) = makeTime((yyvsp[(3) - (4)].tree));
                          }
    break;

  case 221:

/* Line 1464 of yacc.c  */
#line 1389 "miniparser.y"
    {
			    (yyval.list) = addElement(NULL,(yyvsp[(1) - (1)].tree));
			  }
    break;

  case 222:

/* Line 1464 of yacc.c  */
#line 1393 "miniparser.y"
    {
			    (yyval.list) = addElement((yyvsp[(2) - (2)].list),(yyvsp[(1) - (2)].tree));
			  }
    break;

  case 223:

/* Line 1464 of yacc.c  */
#line 1399 "miniparser.y"
    {
			    (yyval.tree) = makeMatchElement((yyvsp[(1) - (9)].tree),makeCommandList(concatChains((yyvsp[(4) - (9)].list), (yyvsp[(5) - (9)].list))),(yyvsp[(7) - (9)].tree));
			  }
    break;

  case 224:

/* Line 1464 of yacc.c  */
#line 1403 "miniparser.y"
    {
			    (yyval.tree) = makeMatchElement((yyvsp[(1) - (6)].tree),makeCommandList(concatChains((yyvsp[(4) - (6)].list), (yyvsp[(5) - (6)].list))),makeUnit());
			  }
    break;

  case 225:

/* Line 1464 of yacc.c  */
#line 1407 "miniparser.y"
    {
			    (yyval.tree) = makeMatchElement((yyvsp[(1) - (8)].tree),makeCommandList((yyvsp[(4) - (8)].list)),(yyvsp[(6) - (8)].tree));
			  }
    break;

  case 226:

/* Line 1464 of yacc.c  */
#line 1411 "miniparser.y"
    {
			    (yyval.tree) = makeMatchElement((yyvsp[(1) - (5)].tree),makeCommandList((yyvsp[(4) - (5)].list)),makeUnit());
			  }
    break;

  case 227:

/* Line 1464 of yacc.c  */
#line 1415 "miniparser.y"
    {
			    (yyval.tree) = makeMatchElement((yyvsp[(1) - (8)].tree),makeCommandList((yyvsp[(4) - (8)].list)),(yyvsp[(6) - (8)].tree));
			  }
    break;

  case 228:

/* Line 1464 of yacc.c  */
#line 1419 "miniparser.y"
    {
			    (yyval.tree) = makeMatchElement((yyvsp[(1) - (5)].tree),makeCommandList((yyvsp[(4) - (5)].list)),makeUnit());
			  }
    break;

  case 229:

/* Line 1464 of yacc.c  */
#line 1423 "miniparser.y"
    {
			    (yyval.tree) = makeMatchElement((yyvsp[(1) - (7)].tree), makeCommandList(addElement(NULL,makeNop())), (yyvsp[(5) - (7)].tree));
			  }
    break;

  case 230:

/* Line 1464 of yacc.c  */
#line 1427 "miniparser.y"
    {
			    (yyval.tree) = makeMatchElement((yyvsp[(1) - (4)].tree), makeCommandList(addElement(NULL,makeNop())), makeUnit());
			  }
    break;

  case 231:

/* Line 1464 of yacc.c  */
#line 1431 "miniparser.y"
    {
			    (yyval.tree) = makeMatchElement((yyvsp[(1) - (5)].tree), makeCommandList(addElement(NULL,makeNop())), (yyvsp[(4) - (5)].tree));
			  }
    break;

  case 232:

/* Line 1464 of yacc.c  */
#line 1437 "miniparser.y"
    {
			    (yyval.tree) = makeDecimalConstant((yyvsp[(1) - (1)].value));
			    free((yyvsp[(1) - (1)].value));
			  }
    break;

  case 233:

/* Line 1464 of yacc.c  */
#line 1442 "miniparser.y"
    {
			    (yyval.tree) = makeMidpointConstant((yyvsp[(1) - (1)].value));
			    free((yyvsp[(1) - (1)].value));
			  }
    break;

  case 234:

/* Line 1464 of yacc.c  */
#line 1447 "miniparser.y"
    {
			    (yyval.tree) = makeDyadicConstant((yyvsp[(1) - (1)].value));
			  }
    break;

  case 235:

/* Line 1464 of yacc.c  */
#line 1451 "miniparser.y"
    {
			    (yyval.tree) = makeHexConstant((yyvsp[(1) - (1)].value));
			  }
    break;

  case 236:

/* Line 1464 of yacc.c  */
#line 1455 "miniparser.y"
    {
			    (yyval.tree) = makeHexadecimalConstant((yyvsp[(1) - (1)].value));
			  }
    break;

  case 237:

/* Line 1464 of yacc.c  */
#line 1459 "miniparser.y"
    {
			    (yyval.tree) = makeBinaryConstant((yyvsp[(1) - (1)].value));
			  }
    break;

  case 238:

/* Line 1464 of yacc.c  */
#line 1463 "miniparser.y"
    {
			    (yyval.tree) = makePi();
			  }
    break;

  case 239:

/* Line 1464 of yacc.c  */
#line 1471 "miniparser.y"
    {
			    (yyval.tree) = makeEmptyList();
			  }
    break;

  case 240:

/* Line 1464 of yacc.c  */
#line 1475 "miniparser.y"
    {
			    (yyval.tree) = makeEmptyList();
			  }
    break;

  case 241:

/* Line 1464 of yacc.c  */
#line 1479 "miniparser.y"
    {
			    (yyval.tree) = makeRevertedList((yyvsp[(3) - (5)].list));
			  }
    break;

  case 242:

/* Line 1464 of yacc.c  */
#line 1483 "miniparser.y"
    {
			    (yyval.tree) = makeRevertedFinalEllipticList((yyvsp[(3) - (6)].list));
			  }
    break;

  case 243:

/* Line 1464 of yacc.c  */
#line 1489 "miniparser.y"
    {
			    (yyval.list) = addElement(NULL, (yyvsp[(1) - (1)].tree));
			  }
    break;

  case 244:

/* Line 1464 of yacc.c  */
#line 1493 "miniparser.y"
    {
			    (yyval.list) = addElement((yyvsp[(1) - (3)].list), (yyvsp[(3) - (3)].tree));
			  }
    break;

  case 245:

/* Line 1464 of yacc.c  */
#line 1497 "miniparser.y"
    {
			    (yyval.list) = addElement(addElement((yyvsp[(1) - (5)].list), makeElliptic()), (yyvsp[(5) - (5)].tree));
			  }
    break;

  case 246:

/* Line 1464 of yacc.c  */
#line 1503 "miniparser.y"
    {
			    (yyval.tree) = makeRange((yyvsp[(2) - (5)].tree), (yyvsp[(4) - (5)].tree));
			  }
    break;

  case 247:

/* Line 1464 of yacc.c  */
#line 1507 "miniparser.y"
    {
			    (yyval.tree) = makeRange((yyvsp[(2) - (5)].tree), (yyvsp[(4) - (5)].tree));
			  }
    break;

  case 248:

/* Line 1464 of yacc.c  */
#line 1511 "miniparser.y"
    {
			    (yyval.tree) = makeRange((yyvsp[(2) - (3)].tree), copyThing((yyvsp[(2) - (3)].tree)));
			  }
    break;

  case 249:

/* Line 1464 of yacc.c  */
#line 1517 "miniparser.y"
    {
			    (yyval.tree) = makeDeboundMax((yyvsp[(2) - (3)].tree));
			  }
    break;

  case 250:

/* Line 1464 of yacc.c  */
#line 1521 "miniparser.y"
    {
			    (yyval.tree) = makeDeboundMid((yyvsp[(2) - (3)].tree));
			  }
    break;

  case 251:

/* Line 1464 of yacc.c  */
#line 1525 "miniparser.y"
    {
			    (yyval.tree) = makeDeboundMin((yyvsp[(2) - (3)].tree));
			  }
    break;

  case 252:

/* Line 1464 of yacc.c  */
#line 1529 "miniparser.y"
    {
			    (yyval.tree) = makeDeboundMax((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 253:

/* Line 1464 of yacc.c  */
#line 1533 "miniparser.y"
    {
			    (yyval.tree) = makeDeboundMid((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 254:

/* Line 1464 of yacc.c  */
#line 1537 "miniparser.y"
    {
			    (yyval.tree) = makeDeboundMin((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 255:

/* Line 1464 of yacc.c  */
#line 1543 "miniparser.y"
    {
			    (yyval.tree) = makeDiff((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 256:

/* Line 1464 of yacc.c  */
#line 1547 "miniparser.y"
    {
			    (yyval.tree) = makeBashevaluate(addElement(NULL,(yyvsp[(3) - (4)].tree)));
			  }
    break;

  case 257:

/* Line 1464 of yacc.c  */
#line 1551 "miniparser.y"
    {
			    (yyval.tree) = makeBashevaluate(addElement(addElement(NULL,(yyvsp[(5) - (6)].tree)),(yyvsp[(3) - (6)].tree)));
			  }
    break;

  case 258:

/* Line 1464 of yacc.c  */
#line 1555 "miniparser.y"
    {
			    (yyval.tree) = makeSimplify((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 259:

/* Line 1464 of yacc.c  */
#line 1559 "miniparser.y"
    {
			    (yyval.tree) = makeRemez(addElement(addElement((yyvsp[(7) - (8)].list), (yyvsp[(5) - (8)].tree)), (yyvsp[(3) - (8)].tree)));
			  }
    break;

  case 260:

/* Line 1464 of yacc.c  */
#line 1563 "miniparser.y"
    {
			    (yyval.tree) = makeMin((yyvsp[(3) - (4)].list));
			  }
    break;

  case 261:

/* Line 1464 of yacc.c  */
#line 1567 "miniparser.y"
    {
			    (yyval.tree) = makeMax((yyvsp[(3) - (4)].list));
			  }
    break;

  case 262:

/* Line 1464 of yacc.c  */
#line 1571 "miniparser.y"
    {
			    (yyval.tree) = makeFPminimax(addElement(addElement(addElement((yyvsp[(9) - (10)].list), (yyvsp[(7) - (10)].tree)), (yyvsp[(5) - (10)].tree)), (yyvsp[(3) - (10)].tree)));
			  }
    break;

  case 263:

/* Line 1464 of yacc.c  */
#line 1575 "miniparser.y"
    {
			    (yyval.tree) = makeHorner((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 264:

/* Line 1464 of yacc.c  */
#line 1579 "miniparser.y"
    {
			    (yyval.tree) = makeCanonicalThing((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 265:

/* Line 1464 of yacc.c  */
#line 1583 "miniparser.y"
    {
			    (yyval.tree) = makeExpand((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 266:

/* Line 1464 of yacc.c  */
#line 1587 "miniparser.y"
    {
			    (yyval.tree) = makeSimplifySafe((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 267:

/* Line 1464 of yacc.c  */
#line 1591 "miniparser.y"
    {
			    (yyval.tree) = makeTaylor((yyvsp[(3) - (8)].tree), (yyvsp[(5) - (8)].tree), (yyvsp[(7) - (8)].tree));
			  }
    break;

  case 268:

/* Line 1464 of yacc.c  */
#line 1595 "miniparser.y"
    {
                            (yyval.tree) = makeTaylorform(addElement(addElement((yyvsp[(7) - (8)].list), (yyvsp[(5) - (8)].tree)), (yyvsp[(3) - (8)].tree)));
			  }
    break;

  case 269:

/* Line 1464 of yacc.c  */
#line 1599 "miniparser.y"
    {
                            (yyval.tree) = makeAutodiff(addElement(addElement(addElement(NULL, (yyvsp[(7) - (8)].tree)), (yyvsp[(5) - (8)].tree)), (yyvsp[(3) - (8)].tree)));
			  }
    break;

  case 270:

/* Line 1464 of yacc.c  */
#line 1603 "miniparser.y"
    {
			    (yyval.tree) = makeDegree((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 271:

/* Line 1464 of yacc.c  */
#line 1607 "miniparser.y"
    {
			    (yyval.tree) = makeNumerator((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 272:

/* Line 1464 of yacc.c  */
#line 1611 "miniparser.y"
    {
			    (yyval.tree) = makeDenominator((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 273:

/* Line 1464 of yacc.c  */
#line 1615 "miniparser.y"
    {
			    (yyval.tree) = makeSubstitute((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 274:

/* Line 1464 of yacc.c  */
#line 1619 "miniparser.y"
    {
			    (yyval.tree) = makeCoeff((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 275:

/* Line 1464 of yacc.c  */
#line 1623 "miniparser.y"
    {
			    (yyval.tree) = makeSubpoly((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 276:

/* Line 1464 of yacc.c  */
#line 1627 "miniparser.y"
    {
			    (yyval.tree) = makeRoundcoefficients((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 277:

/* Line 1464 of yacc.c  */
#line 1631 "miniparser.y"
    {
			    (yyval.tree) = makeRationalapprox((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 278:

/* Line 1464 of yacc.c  */
#line 1635 "miniparser.y"
    {
			    (yyval.tree) = makeAccurateInfnorm(addElement(addElement((yyvsp[(7) - (8)].list), (yyvsp[(5) - (8)].tree)), (yyvsp[(3) - (8)].tree)));
			  }
    break;

  case 279:

/* Line 1464 of yacc.c  */
#line 1639 "miniparser.y"
    {
			    (yyval.tree) = makeRoundToFormat((yyvsp[(3) - (8)].tree), (yyvsp[(5) - (8)].tree), (yyvsp[(7) - (8)].tree));
			  }
    break;

  case 280:

/* Line 1464 of yacc.c  */
#line 1643 "miniparser.y"
    {
			    (yyval.tree) = makeEvaluate((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 281:

/* Line 1464 of yacc.c  */
#line 1647 "miniparser.y"
    {
			    (yyval.tree) = makeParse((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 282:

/* Line 1464 of yacc.c  */
#line 1651 "miniparser.y"
    {
			    (yyval.tree) = makeReadXml((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 283:

/* Line 1464 of yacc.c  */
#line 1655 "miniparser.y"
    {
			    (yyval.tree) = makeInfnorm(addElement((yyvsp[(5) - (6)].list), (yyvsp[(3) - (6)].tree)));
			  }
    break;

  case 284:

/* Line 1464 of yacc.c  */
#line 1659 "miniparser.y"
    {
			    (yyval.tree) = makeSupnorm(addElement(addElement(addElement(addElement(addElement(NULL,(yyvsp[(11) - (12)].tree)),(yyvsp[(9) - (12)].tree)),(yyvsp[(7) - (12)].tree)),(yyvsp[(5) - (12)].tree)),(yyvsp[(3) - (12)].tree)));
			  }
    break;

  case 285:

/* Line 1464 of yacc.c  */
#line 1663 "miniparser.y"
    {
			    (yyval.tree) = makeFindZeros((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 286:

/* Line 1464 of yacc.c  */
#line 1667 "miniparser.y"
    {
			    (yyval.tree) = makeFPFindZeros((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 287:

/* Line 1464 of yacc.c  */
#line 1671 "miniparser.y"
    {
			    (yyval.tree) = makeDirtyInfnorm((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 288:

/* Line 1464 of yacc.c  */
#line 1675 "miniparser.y"
    {
			    (yyval.tree) = makeNumberRoots((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 289:

/* Line 1464 of yacc.c  */
#line 1679 "miniparser.y"
    {
			    (yyval.tree) = makeIntegral((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 290:

/* Line 1464 of yacc.c  */
#line 1683 "miniparser.y"
    {
			    (yyval.tree) = makeDirtyIntegral((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 291:

/* Line 1464 of yacc.c  */
#line 1687 "miniparser.y"
    {
			    (yyval.tree) = makeImplementPoly(addElement(addElement(addElement(addElement(addElement((yyvsp[(13) - (14)].list), (yyvsp[(11) - (14)].tree)), (yyvsp[(9) - (14)].tree)), (yyvsp[(7) - (14)].tree)), (yyvsp[(5) - (14)].tree)), (yyvsp[(3) - (14)].tree)));
			  }
    break;

  case 292:

/* Line 1464 of yacc.c  */
#line 1691 "miniparser.y"
    {
			    (yyval.tree) = makeImplementConst((yyvsp[(3) - (4)].list));
			  }
    break;

  case 293:

/* Line 1464 of yacc.c  */
#line 1695 "miniparser.y"
    {
			    (yyval.tree) = makeCheckInfnorm((yyvsp[(3) - (8)].tree), (yyvsp[(5) - (8)].tree), (yyvsp[(7) - (8)].tree));
			  }
    break;

  case 294:

/* Line 1464 of yacc.c  */
#line 1699 "miniparser.y"
    {
			    (yyval.tree) = makeZeroDenominators((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 295:

/* Line 1464 of yacc.c  */
#line 1703 "miniparser.y"
    {
			    (yyval.tree) = makeIsEvaluable((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 296:

/* Line 1464 of yacc.c  */
#line 1707 "miniparser.y"
    {
			    (yyval.tree) = makeSearchGal((yyvsp[(3) - (4)].list));
			  }
    break;

  case 297:

/* Line 1464 of yacc.c  */
#line 1711 "miniparser.y"
    {
			    (yyval.tree) = makeGuessDegree(addElement(addElement((yyvsp[(7) - (8)].list), (yyvsp[(5) - (8)].tree)), (yyvsp[(3) - (8)].tree)));
			  }
    break;

  case 298:

/* Line 1464 of yacc.c  */
#line 1715 "miniparser.y"
    {
			    (yyval.tree) = makeDirtyFindZeros((yyvsp[(3) - (6)].tree), (yyvsp[(5) - (6)].tree));
			  }
    break;

  case 299:

/* Line 1464 of yacc.c  */
#line 1719 "miniparser.y"
    {
			    (yyval.tree) = makeHead((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 300:

/* Line 1464 of yacc.c  */
#line 1723 "miniparser.y"
    {
			    (yyval.tree) = makeRoundCorrectly((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 301:

/* Line 1464 of yacc.c  */
#line 1727 "miniparser.y"
    {
			    (yyval.tree) = makeReadFile((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 302:

/* Line 1464 of yacc.c  */
#line 1731 "miniparser.y"
    {
			    (yyval.tree) = makeRevert((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 303:

/* Line 1464 of yacc.c  */
#line 1735 "miniparser.y"
    {
			    (yyval.tree) = makeSort((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 304:

/* Line 1464 of yacc.c  */
#line 1739 "miniparser.y"
    {
			    (yyval.tree) = makeMantissa((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 305:

/* Line 1464 of yacc.c  */
#line 1743 "miniparser.y"
    {
			    (yyval.tree) = makeExponent((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 306:

/* Line 1464 of yacc.c  */
#line 1747 "miniparser.y"
    {
			    (yyval.tree) = makePrecision((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 307:

/* Line 1464 of yacc.c  */
#line 1751 "miniparser.y"
    {
			    (yyval.tree) = makeTail((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 308:

/* Line 1464 of yacc.c  */
#line 1755 "miniparser.y"
    {
			    (yyval.tree) = makeSqrt((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 309:

/* Line 1464 of yacc.c  */
#line 1759 "miniparser.y"
    {
			    (yyval.tree) = makeExp((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 310:

/* Line 1464 of yacc.c  */
#line 1763 "miniparser.y"
    {
			    (yyval.tree) = makeProcedureFunction((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 311:

/* Line 1464 of yacc.c  */
#line 1767 "miniparser.y"
    {
			    (yyval.tree) = makeSubstitute(makeProcedureFunction((yyvsp[(3) - (6)].tree)),(yyvsp[(5) - (6)].tree));
			  }
    break;

  case 312:

/* Line 1464 of yacc.c  */
#line 1771 "miniparser.y"
    {
			    (yyval.tree) = makeLog((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 313:

/* Line 1464 of yacc.c  */
#line 1775 "miniparser.y"
    {
			    (yyval.tree) = makeLog2((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 314:

/* Line 1464 of yacc.c  */
#line 1779 "miniparser.y"
    {
			    (yyval.tree) = makeLog10((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 315:

/* Line 1464 of yacc.c  */
#line 1783 "miniparser.y"
    {
			    (yyval.tree) = makeSin((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 316:

/* Line 1464 of yacc.c  */
#line 1787 "miniparser.y"
    {
			    (yyval.tree) = makeCos((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 317:

/* Line 1464 of yacc.c  */
#line 1791 "miniparser.y"
    {
			    (yyval.tree) = makeTan((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 318:

/* Line 1464 of yacc.c  */
#line 1795 "miniparser.y"
    {
			    (yyval.tree) = makeAsin((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 319:

/* Line 1464 of yacc.c  */
#line 1799 "miniparser.y"
    {
			    (yyval.tree) = makeAcos((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 320:

/* Line 1464 of yacc.c  */
#line 1803 "miniparser.y"
    {
			    (yyval.tree) = makeAtan((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 321:

/* Line 1464 of yacc.c  */
#line 1807 "miniparser.y"
    {
			    (yyval.tree) = makeSinh((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 322:

/* Line 1464 of yacc.c  */
#line 1811 "miniparser.y"
    {
			    (yyval.tree) = makeCosh((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 323:

/* Line 1464 of yacc.c  */
#line 1815 "miniparser.y"
    {
			    (yyval.tree) = makeTanh((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 324:

/* Line 1464 of yacc.c  */
#line 1819 "miniparser.y"
    {
			    (yyval.tree) = makeAsinh((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 325:

/* Line 1464 of yacc.c  */
#line 1823 "miniparser.y"
    {
			    (yyval.tree) = makeAcosh((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 326:

/* Line 1464 of yacc.c  */
#line 1827 "miniparser.y"
    {
			    (yyval.tree) = makeAtanh((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 327:

/* Line 1464 of yacc.c  */
#line 1831 "miniparser.y"
    {
			    (yyval.tree) = makeAbs((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 328:

/* Line 1464 of yacc.c  */
#line 1835 "miniparser.y"
    {
			    (yyval.tree) = makeErf((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 329:

/* Line 1464 of yacc.c  */
#line 1839 "miniparser.y"
    {
			    (yyval.tree) = makeErfc((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 330:

/* Line 1464 of yacc.c  */
#line 1843 "miniparser.y"
    {
			    (yyval.tree) = makeLog1p((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 331:

/* Line 1464 of yacc.c  */
#line 1847 "miniparser.y"
    {
			    (yyval.tree) = makeExpm1((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 332:

/* Line 1464 of yacc.c  */
#line 1851 "miniparser.y"
    {
			    (yyval.tree) = makeDouble((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 333:

/* Line 1464 of yacc.c  */
#line 1855 "miniparser.y"
    {
			    (yyval.tree) = makeSingle((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 334:

/* Line 1464 of yacc.c  */
#line 1859 "miniparser.y"
    {
			    (yyval.tree) = makeQuad((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 335:

/* Line 1464 of yacc.c  */
#line 1863 "miniparser.y"
    {
			    (yyval.tree) = makeHalfPrecision((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 336:

/* Line 1464 of yacc.c  */
#line 1867 "miniparser.y"
    {
			    (yyval.tree) = makeDoubledouble((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 337:

/* Line 1464 of yacc.c  */
#line 1871 "miniparser.y"
    {
			    (yyval.tree) = makeTripledouble((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 338:

/* Line 1464 of yacc.c  */
#line 1875 "miniparser.y"
    {
			    (yyval.tree) = makeDoubleextended((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 339:

/* Line 1464 of yacc.c  */
#line 1879 "miniparser.y"
    {
			    (yyval.tree) = makeCeil((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 340:

/* Line 1464 of yacc.c  */
#line 1883 "miniparser.y"
    {
			    (yyval.tree) = makeFloor((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 341:

/* Line 1464 of yacc.c  */
#line 1887 "miniparser.y"
    {
			    (yyval.tree) = makeNearestInt((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 342:

/* Line 1464 of yacc.c  */
#line 1891 "miniparser.y"
    {
			    (yyval.tree) = makeLength((yyvsp[(3) - (4)].tree));
			  }
    break;

  case 343:

/* Line 1464 of yacc.c  */
#line 1897 "miniparser.y"
    {
			    (yyval.other) = NULL;
			  }
    break;

  case 344:

/* Line 1464 of yacc.c  */
#line 1901 "miniparser.y"
    {
			    (yyval.other) = NULL;
			  }
    break;

  case 345:

/* Line 1464 of yacc.c  */
#line 1908 "miniparser.y"
    {
			    (yyval.tree) = makePrecDeref();
			  }
    break;

  case 346:

/* Line 1464 of yacc.c  */
#line 1912 "miniparser.y"
    {
			    (yyval.tree) = makePointsDeref();
			  }
    break;

  case 347:

/* Line 1464 of yacc.c  */
#line 1916 "miniparser.y"
    {
			    (yyval.tree) = makeDiamDeref();
			  }
    break;

  case 348:

/* Line 1464 of yacc.c  */
#line 1920 "miniparser.y"
    {
			    (yyval.tree) = makeDisplayDeref();
			  }
    break;

  case 349:

/* Line 1464 of yacc.c  */
#line 1924 "miniparser.y"
    {
			    (yyval.tree) = makeVerbosityDeref();
			  }
    break;

  case 350:

/* Line 1464 of yacc.c  */
#line 1928 "miniparser.y"
    {
			    (yyval.tree) = makeCanonicalDeref();
			  }
    break;

  case 351:

/* Line 1464 of yacc.c  */
#line 1932 "miniparser.y"
    {
			    (yyval.tree) = makeAutoSimplifyDeref();
			  }
    break;

  case 352:

/* Line 1464 of yacc.c  */
#line 1936 "miniparser.y"
    {
			    (yyval.tree) = makeTaylorRecursDeref();
			  }
    break;

  case 353:

/* Line 1464 of yacc.c  */
#line 1940 "miniparser.y"
    {
			    (yyval.tree) = makeTimingDeref();
			  }
    break;

  case 354:

/* Line 1464 of yacc.c  */
#line 1944 "miniparser.y"
    {
			    (yyval.tree) = makeFullParenDeref();
			  }
    break;

  case 355:

/* Line 1464 of yacc.c  */
#line 1948 "miniparser.y"
    {
			    (yyval.tree) = makeMidpointDeref();
			  }
    break;

  case 356:

/* Line 1464 of yacc.c  */
#line 1952 "miniparser.y"
    {
			    (yyval.tree) = makeDieOnErrorDeref();
			  }
    break;

  case 357:

/* Line 1464 of yacc.c  */
#line 1956 "miniparser.y"
    {
			    (yyval.tree) = makeRationalModeDeref();
			  }
    break;

  case 358:

/* Line 1464 of yacc.c  */
#line 1960 "miniparser.y"
    {
			    (yyval.tree) = makeSuppressWarningsDeref();
			  }
    break;

  case 359:

/* Line 1464 of yacc.c  */
#line 1964 "miniparser.y"
    {
			    (yyval.tree) = makeHopitalRecursDeref();
			  }
    break;

  case 360:

/* Line 1464 of yacc.c  */
#line 1971 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = CONSTANT_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 361:

/* Line 1464 of yacc.c  */
#line 1977 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = FUNCTION_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 362:

/* Line 1464 of yacc.c  */
#line 1983 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = RANGE_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 363:

/* Line 1464 of yacc.c  */
#line 1989 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = INTEGER_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 364:

/* Line 1464 of yacc.c  */
#line 1995 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = STRING_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 365:

/* Line 1464 of yacc.c  */
#line 2001 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = BOOLEAN_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 366:

/* Line 1464 of yacc.c  */
#line 2007 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = CONSTANT_LIST_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 367:

/* Line 1464 of yacc.c  */
#line 2013 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = FUNCTION_LIST_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 368:

/* Line 1464 of yacc.c  */
#line 2019 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = RANGE_LIST_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 369:

/* Line 1464 of yacc.c  */
#line 2025 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = INTEGER_LIST_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 370:

/* Line 1464 of yacc.c  */
#line 2031 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = STRING_LIST_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 371:

/* Line 1464 of yacc.c  */
#line 2037 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = BOOLEAN_LIST_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 372:

/* Line 1464 of yacc.c  */
#line 2045 "miniparser.y"
    {
			    tempIntPtr = (int *) safeMalloc(sizeof(int));
			    *tempIntPtr = VOID_TYPE;
			    (yyval.integerval) = tempIntPtr;
			  }
    break;

  case 373:

/* Line 1464 of yacc.c  */
#line 2051 "miniparser.y"
    {
			    (yyval.integerval) = (yyvsp[(1) - (1)].integerval);
		          }
    break;

  case 374:

/* Line 1464 of yacc.c  */
#line 2058 "miniparser.y"
    {
			    (yyval.list) = addElement(NULL, (yyvsp[(1) - (1)].integerval));
			  }
    break;

  case 375:

/* Line 1464 of yacc.c  */
#line 2062 "miniparser.y"
    {
			    (yyval.list) = addElement((yyvsp[(3) - (3)].list), (yyvsp[(1) - (3)].integerval));
			  }
    break;

  case 376:

/* Line 1464 of yacc.c  */
#line 2068 "miniparser.y"
    {
			    (yyval.list) = addElement(NULL, (yyvsp[(1) - (1)].integerval));
			  }
    break;

  case 377:

/* Line 1464 of yacc.c  */
#line 2072 "miniparser.y"
    {
			    (yyval.list) = (yyvsp[(2) - (3)].list);
			  }
    break;



/* Line 1464 of yacc.c  */
#line 7549 "miniparser.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (myScanner, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (myScanner, yymsg);
	  }
	else
	  {
	    yyerror (myScanner, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, myScanner);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, myScanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (myScanner, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, myScanner);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, myScanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



