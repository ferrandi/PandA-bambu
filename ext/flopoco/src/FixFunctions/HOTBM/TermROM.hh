#ifndef _TERMROM_HH_
#define _TERMROM_HH_

#include <map>

#include "Term.hh"
#include "Polynomial.hh"
#include "Util.hh"



class TermROM : public Term {
public:
	TermROM(int d_, double *k, Param &p_);
	~TermROM();

	void roundTables(int g, bool full = true, double *kAdjust = NULL);

	double estimArea();
	double estimDelay();

	double eval(long long int a, long long int b) const;
	long long int evalRound(long long int a, long long int b) const;

	friend Component::Component (flopoco::Target*, TermROM, std::string);
	flopoco::Operator* toComponent (flopoco::Target* t, std::string name) {
		return new Component (t, *this, name);
	}

protected:
	PWPolynomial calcErrTab(double shift = 0);

	TermROMParam &tp;
	Polynomial *pList;

	long long int *table;
	int wTable;
	
	enum TableSign {
		SignPositive,	/**< unsigned values in table */
		SignNegative,	/**< unsigned values, opposite */
		SignMixed		/**< 2's-complement */
	};
	
	TableSign signTable;
};

#endif // _TERMROM_HH_
