%option reentrant
%option prefix="Flopoco_"
%option noyywrap
%option yylineno


%{
	#include "LexerContext.hpp"
	#include <stdio.h>
	#include <iostream>
	#include <string.h>
	#include <utility>
	#include <vector>
	#include <ctype.h>
	#include <map>

	#define YY_EXTRA_TYPE LexerContext*
	#define YY_INPUT(buf, result, max_size) {\
		char c; \
		(*yyextra->is).get(c); \
		if (yyextra->is->eof()) \
			result = YY_NULL; \
		else { \
			buf[0] = c; \
			result = 1; \
		}\
	}


%}


upper_case_letter	  [A-Z]
digit 			  [0-9]
special_character   	  [\#\&\'\(\)\*\+\,\-\.\/\:\;\<\=\>\_\|]
space_character 	  [ \t]
format_effector		  [\t\v\r\l\f]
end_of_line		  \n
lower_case_letter 	  [a-z]
other_special_character   [\!\$\@\?\[\\\]\^\`\{\}\~]

graphic_character	  ({basic_graphic_character}|{lower_case_letter}|{other_special_character})
basic_graphic_character	  ({upper_case_letter}|{digit}|{special_character}|{space_character})
letter		   	  ({upper_case_letter}|{lower_case_letter})
letter_or_digit	   	  ({letter}|{digit})
decimal_literal	   	  {integer}(\.{integer})?({exponent})?
integer		   	  {digit}(_?{digit})*
exponent	   	  ([eE][-+]?{integer})
base		   	  {integer}
based_integer	   	  {extended_digit}(_?{extended_digit})*
extended_digit	   	  ({digit}|[a-fA-F])
base_specifier	  	  (B|b|O|o|X|x)


%%

"abs"|"ABS" |
"access"|"ACCESS" |
"after"|"AFTER" |
"alias"|"ALIAS" |
"all"|"ALL" |
"and"|"AND" |
"architecture"|"ARCHITECTURE" |
"array"|"ARRAY" |
"assert"|"ASSERT" |
"attribute"|"ATTRIBUTE" |
"begin"|"BEGIN" |
"block"|"BLOCK" |
"body"|"BODY" |
"buffer"|"BUFFER" |
"bus"|"BUS" |
"case"|"CASE" |
"component"|"COMPONENT" |
"configuration"|"CONFIGURATION" |
"constant"|"CONSTANT" |
"disconnect"|"DISCONNECT" |
"downto"|"DOWNTO" |
"else"|"ELSE" |
"elsif"|"ELSIF" |
"end"|"END" |
"entity"|"ENTITY" | 
"exit"|"EXIT" |
"file"|"FILE" |
"for"|"FOR" |
"function"|"FUNCTION" |
"generate"|"GENERATE" |
"generic"|"GENERIC" |
"guarded"|"GUARDED" |
"if"|"IF" |
"in"|"IN" |
"inout"|"INOUT" |
"is"|"IS" |
"label"|"LABEL" |
"library"|"LIBRARY" |
"linkage"|"LINKAGE" |
"loop"|"LOOP" |
"map"|"MAP" |
"mod"|"MOD" |
"nand"|"NAND" |
"new"|"NEW" |
"next"|"NEXT" |
"nor"|"NOR" |
"not"|"NOT" |
"null"|"NULL" |
"of"|"OF" |
"on"|"ON" |
"open"|"OPEN" |
"or"|"OR" |
"others"|"OTHERS" |
"out"|"OUT" |
"package"|"PACKAGE" |
"port"|"PORT" |
"procedure"|"PROCEDURE" |
"process"|"PROCESS" |
"range"|"RANGE" |
"record"|"RECORD" |
"register"|"REGISTER" |
"rem"|"REM" |
"report"|"REPORT" |
"return"|"RETURN" |
"select"|"SELECT" |
"severity"|"SEVERITY" |
"signal"|"SIGNAL" |
"subtype"|"SUBTYPE" |
"then"|"THEN" |
"to"|"TO" |
"transport"|"TRANSPORT" |
"type"|"TYPE" |
"units"|"UNITS" |
"until"|"UNTIL" |
"use"|"USE" |
"variable"|"VARIABLE" |
"wait"|"WAIT" |
"when"|"WHEN" |
"while"|"WHILE" |
"with"|"WITH" |
"xor"|"XOR"  { (*yyextra->os) << yytext; }





{letter}(_?{letter_or_digit})*({space_character})*"=>" { (*yyextra->os) << yytext; } 

[ \t\n] (*yyextra->os) << yytext;



{letter}(_?{letter_or_digit})* { (*yyextra->os) << "__" << yytext << "__" << (yyextra->yyTheCycle) <<"__";
								  std::pair< std::string, int> tmp;
								  tmp.first = yytext;
								  tmp.second = (yyextra->yyTheCycle);
								  yyextra->theUseTable.push_back ( tmp );		
								} 

\&|\'|\(|\)|"**"|\*|\+|\,|\-|":="|\:|\;|"<="|">="|\<|\>|=|\/=|"=>"|"<>"|\||!|\.|\/ { (*yyextra->os) << yytext; }	


({decimal_literal})|({base}#{based_integer}(\.{based_integer})?#({exponent})?)|({base}:{based_integer}(\.{based_integer})?:({exponent})?)		{
			(*yyextra->os) << yytext; 
			}

'({graphic_character}|\"|\%)' 	{ (*yyextra->os) << yytext; }

(\"({graphic_character}|(\"\")|\%)*\")|(\%({graphic_character}|(\%\%)|\")*\%) { (*yyextra->os) << yytext; }

{base_specifier}(\"{extended_digit}(_?{extended_digit})*\"|\%{extended_digit}(_?{extended_digit})*\%) 	{ (*yyextra->os) << yytext; }

\-\-.*$ 	{
			/* comment */
			(*yyextra->os) << yytext;
			}

(.) (*yyextra->os) << yytext;

%%

void LexerContext::init_scanner() {
	yylex_init(&scanner);
	yyset_extra(this, scanner);
}

void LexerContext::destroy_scanner() {
	yylex_destroy(scanner);
}

void LexerContext::lex() {
	yylex(scanner);
}
