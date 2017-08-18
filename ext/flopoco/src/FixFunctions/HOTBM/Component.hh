#ifndef _COMPONENT_HH_
#define _COMPONENT_HH_

#include "Operator.hpp"
#include "Target.hpp"
//#include "PowerAdHoc.hh"
//#include "PowerROM.hh"
//#include "TermPowMult.hh"
//#include "TermROM.hh"
class PowerAdHoc;
class PowerROM;
class TermPowMult;
class TermROM;

class Component: public flopoco::Operator {
	public:
		Component (flopoco::Target* t, PowerAdHoc p, std::string name);
		Component (flopoco::Target* t, PowerROM p, std::string name);
		Component (flopoco::Target* t, TermPowMult tpm, std::string name);
		Component (flopoco::Target* t, TermROM tr, std::string name);
};

#endif

