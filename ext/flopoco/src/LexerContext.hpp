#ifndef __LEXER_CONTEXT_HPP__
#define __LEXER_CONTEXT_HPP__

#include <iostream>
#include <vector>

using namespace std;

class LexerContext {
public:
	void* scanner;
	int result;
	istream* is;
	ostream* os;
	int yyTheCycle;
	vector<pair<string, int> > theUseTable;

public:
	LexerContext(istream* is = &cin, ostream* os = &cout) {
		init_scanner();
		this->is = is;
		this->os = os;
		yyTheCycle=0;
	}

	//these methods are generated in VHDLLexer.cpp 

	void lex();

	virtual ~LexerContext() { destroy_scanner();}

protected:
	void init_scanner();

	void destroy_scanner();
};



#endif
