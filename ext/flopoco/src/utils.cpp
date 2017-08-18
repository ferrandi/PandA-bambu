/*
  utility functions for FloPoCo
 
  Author: Florent de Dinechin
 

  This file is part of the FloPoCo project
  developed by the Arenaire team at Ecole Normale Superieure de Lyon
  
  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
  2008-2010.
  All rights reserved.
*/

#include <iostream>
#include <sstream>
#include "utils.hpp"
#include <cstdlib>
#include <iomanip>
#include "math.h"
#include <functional>
#include <algorithm>
#include <cctype>
#include <gmp.h>
#include <gmpxx.h>
using namespace std;


namespace flopoco{
        /** Initialization of FloPoCoRandomState state */
        gmp_randstate_t FloPoCoRandomState::m_state;

		bool FloPoCoRandomState::isInit_ = false;

        void FloPoCoRandomState::init(int n, bool force) {
			// if isInit_ is set, we do not initialize the random state again
			if (isInit_ && !force) return;
			gmp_randinit_mt(m_state);
			gmp_randseed_ui(m_state,n);
			isInit_ = true;
        };

        //gmp_randstate_t* FloPoCoRandomState::getState() { return m_state;};
	/** return a string representation of an mpz_class on a given number of bits */
	string unsignedBinary(mpz_class x, int size){
		string s;
		mpz_class po2, number;
		char bit;

		if(x<0) {
			cerr<<"unsigned_binary: Positive number expected, got x="<<x.get_d()<<endl;
			exit(EXIT_FAILURE);
		}
		po2 = ((mpz_class) 1)<<size;
		number=x;
		
		for (int i = 0; i < size ; i++) {
			po2 = po2>>1;
			if (number >= po2) {
				bit = '1';
				number -= po2;
			}
			else {
				bit = '0';
			}
			s +=  bit;
		}
		return s;
	}

	/** return the binary representation of a floating point number in the
		 FPLibrary/FloPoCo format */
	string fp2bin(mpfr_t x, int wE, int wF){
		mpfr_t mpx, one, two;
		ostringstream s;

		// copy the input
		mpfr_init2 (mpx, wF+1);
		mpfr_set (mpx, x, GMP_RNDN);


		// exception bits
		if(mpfr_nan_p (mpx)) {
			s << "11";
			for(int i=0; i<wE+wF+1; i++)
				s<< "0";
			return s.str();
		}

		// sign bit
		string sign;
		if(mpfr_sgn(mpx)<0) {
			sign="1";
			mpfr_neg(mpx, mpx, GMP_RNDN);
		}
		else
			sign="0";

		if(mpfr_zero_p (mpx)) {
			s << "00" << sign;
			for(int i=0; i<wE+wF; i++)
				s<< "0";
			return s.str();
		}

		if(mpfr_inf_p (mpx)) {
			s << "10"<<sign;
			for(int i=0; i<wE+wF; i++)
				s<< "0";
			return s.str();
		}

		// otherwise normal number

		// compute exponent and mantissa
		mpz_class exponent = 0;
		mpz_class biased_exponent;

		mpfr_init2(one, 2);
		mpfr_set_d(one, 1.0, GMP_RNDN);
		mpfr_init2(two, 2);
		mpfr_set_d(two, 2.0, GMP_RNDN);

		while(mpfr_less_p(mpx,one)) {
			mpfr_mul(mpx, mpx, two, GMP_RNDN);
			exponent --;
		}
		while(mpfr_greaterequal_p(mpx, two)) {
			mpfr_div(mpx, mpx, two, GMP_RNDN);
			exponent ++;
		}

		// add exponent bias
		biased_exponent = exponent + (mpz_class(1)<<(wE-1)) - 1;

		if ( biased_exponent<0 )  {
			cerr << "fp2bin warning: underflow, flushing to zero"<<endl;
			s << "00" << sign;
			for(int i=0; i<wE+wF; i++)
				s<< "0";
			return s.str();
		}

		if (biased_exponent >= (mpz_class(1)<<wE) )  {
			cerr << "fp2bin warning: overflow, returning infinity"<<endl;
			s << "10"<<sign;
			for(int i=0; i<wE+wF; i++)
				s<< "0";
			return s.str();
		}

		// normal number
		s << "01" << sign;

		// exponent
		s << unsignedBinary(biased_exponent, wE);
		
		// significand
	
		mpfr_sub(mpx, mpx, one, GMP_RNDN);
		for (int i=0; i<wF; i++) {
			mpfr_mul(mpx, mpx, two, GMP_RNDN);
			if(mpfr_greaterequal_p(mpx, one)) {
				s << "1";
				mpfr_sub(mpx, mpx, one, GMP_RNDN);
			}
			else
				s << "0";
		}

		mpfr_clear(mpx);
		mpfr_clear(one);
		mpfr_clear(two);
		return s.str();
	}


	//this function should not change it's parameter
	std::string unsignedFixPointNumber(mpfr_t x, int msb, int lsb, int margins)
	{
		int size = msb-lsb+1;
		mpz_class h;

		//create a clone of x
		mpfr_t xClone;
		mpfr_prec_t xPrec;

		xPrec = mpfr_get_prec(x);
		mpfr_init2(xClone, xPrec);
		mpfr_set(xClone, x, GMP_RNDN);
				
		//mpfr_mul_2si(x, x, -lsb, GMP_RNDN); // exact
		mpfr_mul_2si(xClone, xClone, -lsb, GMP_RNDN); // exact
		
		mpfr_get_z(h.get_mpz_t(), xClone,  GMP_RNDN); // rounding takes place here

		if(h<0){
			std::ostringstream o;
			o <<  "Error, negative input to unsignedFixPointNumber :" << printMPFR(xClone, 40);
			throw o.str();
		}

		mpfr_clear(xClone);

		ostringstream result;
		if(margins==0||margins==-1)
			result<<"\"";
		result << unsignedBinary(h, size);
		if(margins==0||margins==1)
			result<<"\"";
		return result.str(); 
	}


	string printMPFR(mpfr_t x, int n){
		ostringstream s;
		s << setprecision(n) << mpfr_get_d(x, GMP_RNDN);
		// TODO: sth like the following
		//		mpfr_out_str (s.get_stream(), 10, n, x, GMP_RNDN);

		return s.str();
	}


	/** Print out binary writing of an integer on "size" bits */
	// TODO remove this function
	void printBinNum(ostream& o, uint64_t x, int size)
	{
		uint64_t po2 = ((uint64_t) 1)<<size; 
		char bit;

		if(size>=64){
			cerr << "\n printBinNum: size larger than 64" << endl;
			exit(1);    
		}

		if ((x<0) || (x >= po2) ) {
			cerr << "\n printBinNum: input out of range" << endl;
			exit(1);
		}
		for (int i = 0; i < size ; i++) {
			po2 = po2>>1;
			if (x >= po2) {
				bit = '1';
				x -= po2;
			}
			else {
				bit = '0';
			}
			o << bit;
		}
	}

	void printBinNumGMP(ostream& o, mpz_class x, int size){
		mpz_class px;  
		if(x<0) {
			o<<"-";
			px=-x;
		}
		else {
			//		o<<" "; removed because VHDL " 110" (with a space) is bad
			px=x;
		}
		printBinPosNumGMP(o, px, size);
	}

	void printBinPosNumGMP(ostream& o, mpz_class x, int size)
	{
		o << unsignedBinary(x,size);
	}

	double iround(double number, int bits)
	{
		double shift = intpow2(bits);
		double x = number * shift;
		return floor(x + 0.5) / shift;
	}

	double ifloor(double number, int bits)
	{
		double shift = intpow2(bits);
		return floor(number * shift) / shift;
	}

	//  2 ^ power
	double intpow2(int power)
	{
		double x = 1.0;
		if(power>0){ 
			for (int i = 0; i < power; i++)
				x *= 2;
		} else if(power<0) {
			for (int i = 0; i < -power; i++)
				x /= 2;
		}
		return x;
	}

	mpz_class mpzpow2(unsigned int power)
	{
		mpz_class x = 1;
		if(power>0){ 
			for (unsigned int i = 0; i < power; i++)
				x *= 2;
		}
		return x;
	}


// 	#if 0
// //  2 ^ -minusPower. Exact, no round
// 	double invintpow2(unsigned int minusPower)
// 	{
// 		double x = 1;
// 		for (int i = 0; i < minusPower; i++)
// 			x /= 2;
// 		return x;
// 	}
// #endif

	// How many bits does it take to write number ?
	int intlog2(double number)
	{
		double po2 = 1.0; int result = 0;
		while (po2 <= number) {
			po2 *= 2;
			result++;
		}
		return result;
	}

	int intlog(mpz_class base, mpz_class number)
	{
		mpz_class poBase = 1; 
		int result = 0;
		while (poBase <= number) {
			poBase *= base;
			result++;
		}
		return result;
	}

	int intlog2(mpz_class number)
	{
		mpz_class po2 = 1; 
		int result = 0;
		while (po2 <= number) {
			po2 *= 2;
			result++;
		}
		return result;
	}

	mpz_class popcnt(mpz_class number)
	{
		if (number < 0) throw "popcnt: positive argument required";
		mpz_class res(0), x(number);
		while (x != 0) {
			res += (x & 1);
			x >>= 1;
		}
		return res;
	}

	mpz_class maxExp(int wE){
		return mpz_class(1)<<(wE-1);
	}

	mpz_class minExp(int wE){
		return 1 - (mpz_class(1)<<(wE-1));
	}

	mpz_class bias(int wE){
		return (mpz_class(1)<<(wE-1)) - 1;
	}


#ifdef _WIN32
	mpz_class getLargeRandom(int n)
	{
		mpz_class o = 0;
		mpz_class tmp;
		int iterations = n / 10;
		int quotient = n % 10;
		int r;


		for (int i=0; i<=iterations-1;i++){
			r = rand() % 1024;
			tmp = r;
			o = o + (tmp<<(i*10));
		}
		if (quotient>0){
			r = rand() % int((intpow2(quotient)-1));
			tmp = r;
			o = o + (tmp<<(iterations*10));
		}
	
		return o;
	}
#else
	mpz_class getLargeRandom(int n)
	{
		mpz_class o;
		mpz_urandomb(o.get_mpz_t(), FloPoCoRandomState::m_state, n);
		return o;
	}

#endif 

	string zg(int n, int margins){
		ostringstream left,full, right, zeros;
		int i;

		for (i=1; i<=n;i++)
			zeros<<"0";

		left<<"\""<<zeros.str();
		full<<left.str()<<"\"";
		right<<zeros.str()<<"\"";

		switch(margins){
		case -2: return zeros.str(); break;
		case -1: return left.str();  break;
		case  0: return full.str();  break;
		case  1: return right.str(); break;
		default: return full.str(); 
		}
	
		//default (will not get here)
		return "";
	}
	
	string og(int n, int margins){
		ostringstream left,full, right, ones;
		int i;
		
		for (i=1; i<=n;i++)
			ones<<"1";
		
		left<<"\""<<ones.str();
		full<<left.str()<<"\"";
		right<<ones.str()<<"\"";
		
		switch(margins){
			case -2: return ones.str(); break;
			case -1: return left.str();  break;
			case  0: return full.str();  break;
			case  1: return right.str(); break;
			default: return full.str(); 
		}
		
		//default (will not get here)
		return "";
	}
	
	int oneGenerator(int n)
	{
		int result;
		
		result = 0;
		for(int i=0; i<n; i++)
		{
			result = (result<<1) + 1;
		}
		
		return result;
	}
	

	// Does not handle multi-byte encodings.
	char vhdlizeChar(char c)
	{
		if(c=='+')
			return 'P';

		if(c=='-')
			return 'M';

		if(c=='*')
			return 'X';

		if(c=='*')
			return 'D';

		if(isalnum(c)) {
			return c;
		}
		if(isspace(c)) {
			return '_';
		}
		if(ispunct(c)) {
			return '_';
		}
		return 'x';
	}

	bool bothUnderscore(char a, char b)
	{
		return (a == '_') && (b == '_');
	}

	string vhdlize(string const & expr)
	{
		string result(expr.size(), 0);
		transform(expr.begin(), expr.end(), result.begin(), ptr_fun(vhdlizeChar));
	
		// Multiple consecutive underscores are forbidden in VHDL identifiers!
		string::iterator newend = unique(result.begin(), result.end(), bothUnderscore);
	
		// Leading underscores and numbers are forbidden in VHDL identifiers!
		//	if(isdigit(*result.begin()) || *result.begin() == '_')
		//		*result.begin() = 'x';
		// but this function becomes unusable with numbers if we enforce this...
		if(*result.begin() == '_')
			*result.begin() = 'x';
		// Trailing underscores are forbidden in VHDL identifiers!
		if(*(newend-1) == '_')
		  newend--;
		return result.substr(0, newend - result.begin());
	}

	string vhdlize(double num)
	{
		ostringstream oss;
		oss << num;
		return vhdlize(oss.str());
	}

	string vhdlize(int num)
	{
		ostringstream oss;
		if (num<0) 
		  oss << "M" << (-num);
		else
		  oss << num;
		return oss.str();
	}

	string mpz2string(mpz_class x)
	{
		return x.get_str(10);	
	}

	double getMaxInputDelays( map<string, double> inputDelays )
	{
		double maxInputDelay = 0;
		map<string, double>::iterator iter;
		for (iter = inputDelays.begin(); iter!=inputDelays.end();++iter)
			if (iter->second > maxInputDelay)
				maxInputDelay = iter->second;
			
		return maxInputDelay;
	}

	string printInputDelays( map <string, double> inputDelays){
		ostringstream o;
		map<string, double>::iterator iter;
		for (iter = inputDelays.begin(); iter!=inputDelays.end();++iter)
			o << "The delay for " << iter->first << " is " << iter->second << ";   ";			
		return o.str();
	}

	string printMapContent( map <string, int> inputDelays){
		ostringstream o;
		map<string, int>::iterator iter;
		for (iter = inputDelays.begin(); iter!=inputDelays.end();++iter)
			o  << endl << "   " << iter->first << " cycle " << iter->second;			
		return o.str();
	}
	
	string printVectorContent( vector< pair<string, int> > table){
		ostringstream o;
		vector< pair<string, int> >::iterator iter;
		for (iter = table.begin(); iter!=table.end();++iter)
			o  << endl <<"   " << (*iter).first << " cycle " << (*iter).second;			
		return o.str();
	}

	string join( std::string id, int n)
	{
		ostringstream o;
		o << id << n;
		return o.str();
	}

	string join( std::string id, string sep, int n)
	{
		ostringstream o;
		o << id << sep << n;
		return o.str();
	}

	string join( std::string id, int n1, int n2)
	{
		ostringstream o;
		o << id << n1 << n2;
		return o.str();
	}

	string join( std::string id, int n1, int n2, int n3)
	{
		ostringstream o;
		o << id << n1 << n2 << n3;
		return o.str();
	}


	string join( std::string id1, int n, std::string id2)
	{
		ostringstream o;
		o << id1 << n << id2;
		return o.str();
	}


	string join( std::string id, int n, std::string id2, int n2)
	{
		ostringstream o;
		o << id << n << id2 << n2;
		return o.str();
	}
	
	string join( std::string id, int n, std::string id2, int n2, std::string id3)
	{
		ostringstream o;
		o << id << n << id2 << n2 << id3;
		return o.str();
	}
	
	string join( std::string id, int n, std::string id2, int n2, std::string id3, int n3)
	{
		ostringstream o;
		o << id << n << id2 << n2 << id3 << n3;
		return o.str();
	}

	string join( std::string id, std::string id2, int n2, std::string id3)
	{
		ostringstream o;
		o << id << id2 << n2 << id3;
		return o.str();
	}
	
	string join( std::string id, string n)
	{
		ostringstream o;
		o << id << n;
		return o.str();
	}


	string range( int left, int right)
	{
		ostringstream o;
		if (left>=right) o<<"("<<left<<" downto " << (right>0?right:0) << ")";
		else             o<<"("<<(left>0?left:0)<<" to "     << right << ")";

		return o.str();
	}

	string rangeAssign( int left, int right, std::string s)
	{
		if (left >= right){
			ostringstream o;
			o<<"("<<left<<" downto " << right <<" => " << s << ")";
			return o.str();
		}else
			return "\"\"";
	}

	string of( int x)
	{
		ostringstream o;
		o << "("<<x<<")";
		return o.str();
	}

	string align( int left, string s, int right ){
		ostringstream tmp;
		
		tmp << "(" << (left>0?zg(left,0) + " & ":"") << s << (right>0?" & " +zg(right, 0):"") << ")";
		return tmp.str(); 
	}

	string to_lowercase(const std::string& s){
		std::string t;
		for (std::string::const_iterator i = s.begin(); i != s.end(); ++i)
		t += tolower(*i);
		return t;
	}
	
	map<string, double> inDelayMap(string s, double d){
		map<string, double> m;
		m[s] = d;
		return m;
	}

	mpz_class bitVectorToSigned(mpz_class x, int w){
		mpz_class r;
		// sanity checks
 		if (x >= (mpz_class(1) << w)){
			ostringstream error;
			error << "bitVectorToSigned: input " << x << " does not fit on " << w << " bits";
			throw error.str();
		}
 		if (x < mpz_class(0)){
			ostringstream error;
			error << "bitVectorToSigned: negative input " << x;
			throw error.str();
		}

		if (x >= (mpz_class(1) << (w-1)))
			r = x - (mpz_class(1) << w);
		else
			r = x;
		return r;
	}

	mpz_class signedToBitVector(mpz_class x, int w){
		// sanity checks
		if (  (x > (mpz_class(1) << (w-1))) || (x <= -(mpz_class(1) << (w-1))) ){
			ostringstream error;
			error << "signedToBitVector: input " << x << " out of range for two's complement on " << w << " bits";
			throw error.str();
		}
		mpz_class r;
		if (x < 0)
			r = x + (mpz_class(1) << w);
		else
			r = x;
		return r;
	}
}
