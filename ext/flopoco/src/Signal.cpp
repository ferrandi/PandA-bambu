#include <iostream>
#include <sstream>
#include "Signal.hpp"

using namespace std;

namespace flopoco{


	Signal::Signal(const string name, const Signal::SignalType type, const int width, const bool isBus) : 
		name_(name), type_(type), width_(width), numberOfPossibleValues_(1), lifeSpan_(0),  cycle_(0),	
		isFP_(false), isIEEE_(false), wE_(0), wF_(0), isBus_(isBus), delay_(0.0) {
	}

	Signal::Signal(const string name, const Signal::SignalType type, const int wE, const int wF, const bool ieeeFormat) : 
		name_(name), type_(type), width_(wE+wF+3), numberOfPossibleValues_(1), 
		lifeSpan_(0), cycle_(0),
		isFP_(true), isIEEE_(false), wE_(wE), wF_(wF), isBus_(false), delay_(0.0)
	{
		if(ieeeFormat) { // correct some of the initializations above
			width_=wE+wF+1;
			isFP_=false;
			isIEEE_=true;
		}
	}

	Signal::~Signal(){}

	const string& Signal::getName() const { 
		return name_; 
	}


	int Signal::width() const{return width_;}
	
	int Signal::wE() const {return(wE_);}

	int Signal::wF() const {return(wF_);}
	
	bool Signal::isFP() const {return isFP_;}

	bool Signal::isIEEE() const {return isIEEE_;}

	bool Signal::isBus() const {return isBus_;}

	Signal::SignalType Signal::type() const {return type_;}
	
	string Signal::toVHDL() {
		ostringstream o; 
		if(type()==Signal::wire || type()==Signal::registeredWithoutReset || type()==Signal::registeredWithAsyncReset || type()==Signal::registeredWithSyncReset || type()==Signal::registeredWithZeroInitialiser) 
			o << "signal ";
		o << getName();
		o << " : ";
		if (type()==Signal::in)
			o << "in ";
		if(type()==Signal::out)
			o << "out ";
	
		if ((1==width())&&(!isBus_)) 
			o << "std_logic" ;
		else 
			if(isFP_) 
				o << " std_logic_vector(" << wE() <<"+"<<wF() << "+2 downto 0)";
			else
				o << " std_logic_vector(" << width()-1 << " downto 0)";
		return o.str();
	}



	string Signal::delayedName(int delay){
		ostringstream o;
#if 0
		o << getName();
		if(delay>0) {
			for (int i=0; i<delay; i++){
				o  << "_d";
			}
		}
#else // someday we need to civilize pipe signal names
		o << getName();
		if(delay>0) 
			o << "_d" << delay;
#endif
		return o.str();
	}


	string Signal::toVHDLDeclaration() {
		ostringstream o; 
		o << "signal ";
		if (type_!=Signal::in)
			o << getName() << (lifeSpan_ > 0 ? ", ": "");
		if (lifeSpan_ > 0)
			o << getName() << "_d" << 1;
		for (int i=2; i<=lifeSpan_; i++) {
			o << ", " << getName() << "_d" << i;
		}
		o << " : ";
		if ((1==width())&&(!isBus_)) {
			o << "std_logic" ;
			if(type()==Signal::registeredWithZeroInitialiser){
				o << " := '0'";
			}
		}else {
			if(isFP_) 
				o << " std_logic_vector(" << wE() <<"+"<<wF() << "+2 downto 0)";
			else
				o << " std_logic_vector(" << width()-1 << " downto 0)";
			if(type()==Signal::registeredWithZeroInitialiser){
				o << ":= (others=>'0')";
			}
		}
		o << ";";
		return o.str();
	}

	void Signal::setCycle(int cycle) {
		cycle_ = cycle;
	}

	int Signal::getCycle() {
		return cycle_;
	}

	void Signal::updateLifeSpan(int delay) {
		if(delay>lifeSpan_)
			lifeSpan_=delay;
	}

	int Signal::getLifeSpan() {
		return lifeSpan_;
	}

	double Signal::getDelay(){
		return delay_;
	}
	
	void Signal::setDelay(double delay){
		delay_ = delay;	
	}

	void  Signal::setNumberOfPossibleValues(int n){
		numberOfPossibleValues_ = n;
	}


	int  Signal::getNumberOfPossibleValues(){
		return numberOfPossibleValues_;
	}



	std::string Signal::valueToVHDL(mpz_class v, bool quot){
		std::string r;

		/* Get base 2 representation */
		r = v.get_str(2);

		/* Some checks */
		if ((int) r.size() > width())	{
			std::ostringstream o;
			o << "Error in " <<  __FILE__ << "@" << __LINE__ << ": value (" << r << ") is larger than signal " << getName();
			throw o.str();
		}

		/* Do padding */
		while ((int)r.size() < width())
			r = "0" + r;

		/* Put apostrophe / quot */
		if (!quot) return r;
		if ((width() > 1) || ( isBus() ))
			return "\"" + r + "\"";
		else
			return "'" + r + "'";
	}


	std::string Signal::valueToVHDLHex(mpz_class v, bool quot){
		std::string o;

		/* Get base 16 representation */
		o = v.get_str(16);

		/* Some check */
		/* XXX: Too permissive */
		if ((int)o.size() * 4 > width() + 4)	{
			std::ostringstream o;
			o << "Error in " <<  __FILE__ << "@" << __LINE__ << ": value is larger than signal " << getName();
			throw o.str();
		}

		/* Do padding */
		while ((int)o.size() * 4 < width())
			o = "0" + o;

		/* Put apostrophe / quot */
		if (!quot) return o;
		if (width() > 1)
			return "x\"" + o + "\"";
		else
			return "'" + o + "'";
	}
	
}
