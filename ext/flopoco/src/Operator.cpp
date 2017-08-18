/*
the base Operator class, every operator should inherit it

Author : Florent de Dinechin, Bogdan Pasca

Initial software.
Copyright © ENS-Lyon, INRIA, CNRS, UCBL,  
2008-2010.
  All rights reserved.

*/


#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include "Operator.hpp"
#include "utils.hpp"


namespace flopoco{
	

	// global variables used through most of FloPoCo,
	// to be encapsulated in something, someday?
	int Operator::uid = 0; //init of the uid static member of Operator
	int verbose=0;
	
	Operator::Operator(Target* target, map<string, double> inputDelays){
		stdLibType_                 = 0; // unfortunately this is the historical default.
		target_                     = target;
		numberOfInputs_             = 0;
		numberOfOutputs_            = 0;
		hasRegistersWithoutReset_   = false;
		hasRegistersWithAsyncReset_ = false;
		hasRegistersWithSyncReset_  = false;
		hasClockEnable_             = false;
		pipelineDepth_              = 0;
		currentCycle_               = 0;
		criticalPath_               = 0;
		needRecirculationSignal_    = false;
		inputDelayMap               = inputDelays;
		myuid                       = getNewUId();
		architectureName_			= "arch";
		indirectOperator_           = NULL;
		hasDelay1Feedbacks_         = false;
		

		// Currently we set the pipeline and clockenable from the global target. 
		// This is relatively safe from command line, in the sense that they can only be changed by the command-line,
		// so each sub-component of an operator will share the same target.
		// Il also makes the subcomponent calls easier: pass clock and ce without thinking about it.
		// It is not very elegant because if the operator is eventually combinatorial, it will nevertheless have a clock and rst signal.
		if (target_->isPipelined())
			setSequential();
		else
			setCombinatorial();	
		
		setClockEnable(target_->useClockEnable());

		vhdl.disableParsing(!target_->isPipelined());	

		//------- Resource estimation and floorplanning ----------------
		resourceEstimate << "Starting Resource estimation report for entity: " << uniqueName_ << " --------------- " << endl;
		resourceEstimateReport << "";
		
		reHelper = new ResourceEstimationHelper(target_, this);
		reHelper->initResourceEstimation();
		
		reActive = false;
		
		//--------------------------------------------------------------
		floorplan << "";
		
		flpHelper = new FloorplanningHelper(target_, this);
		flpHelper->initFloorplanning(0.75); 							// ratio (to what degree will the sub-components' bounding boxes 
																		// be filled), see Tools/FloorplanningHelper
		//--------------------------------------------------------------
	}
	




	void Operator::addSubComponent(Operator* op) {
		oplist.push_back(op);
	}




	void Operator::addToGlobalOpList() {
		bool alreadyPresent=false;
		// We assume all the operators added to GlobalOpList are unpipelined.

		vector<Operator*> * globalOpListRef=target_->getGlobalOpListRef();
			for (unsigned i=0; i<globalOpListRef->size(); i++){
					if( getName() == (*globalOpListRef)[i]->getName() ){
					alreadyPresent=true;
					REPORT(DEBUG,"Operator::addToGlobalOpListRef(): " << uniqueName_ <<" already present in globalOpList");
				}
			}
			if(!alreadyPresent)
				globalOpListRef->push_back(this);

	}
	


	
	void Operator::addInput(const std::string name, const int width, const bool isBus) {
		if (signalMap_.find(name) != signalMap_.end()) {
			std::ostringstream o;
			o << srcFileName << " (" << uniqueName_ << "): ERROR in addInput, signal " << name<< " seems to already exist";
			throw o.str();
		}
		Signal *s = new Signal(name, Signal::in, width, isBus) ; // default TTL and cycle OK
		s->setCycle(0);
		ioList_.push_back(s);
		signalMap_[name] = s ;
		numberOfInputs_ ++;
		declareTable[name] = s->getCycle();
	}
	
	void Operator::addOutput(const std::string name, const int width, const int numberOfPossibleOutputValues, const bool isBus) {
		if (signalMap_.find(name) != signalMap_.end()) {
			std::ostringstream o;
			o  << srcFileName << " (" << uniqueName_ << "): ERROR in addOutput, signal " << name << " seems to already exist";
			throw o.str();
		}
		Signal *s = new Signal(name, Signal::out, width, isBus) ;
		s -> setNumberOfPossibleValues(numberOfPossibleOutputValues);
		ioList_.push_back(s);
		for(int i=0; i<numberOfPossibleOutputValues; i++) 
			testCaseSignals_.push_back(s);
		signalMap_[name] = s ;
		numberOfOutputs_ ++;
		//		declareTable[name] = s->getCycle();
	}
	
	void Operator::addFPInput(const std::string name, const int wE, const int wF) {
		if (signalMap_.find(name) != signalMap_.end()) {
			std::ostringstream o;
			o << srcFileName << " (" << uniqueName_ << "): ERROR in addFPInput, signal " << name<< " seems to already exist";
			throw o.str();
		}
		Signal *s = new Signal(name, Signal::in, wE, wF);
		s->setCycle(0);
		ioList_.push_back(s);
		signalMap_[name] = s ;
		numberOfInputs_ ++;
		declareTable[name] = s->getCycle();
	}
	
	void Operator::addFPOutput(const std::string name, const int wE, const int wF, const int numberOfPossibleOutputValues) {
		if (signalMap_.find(name) != signalMap_.end()) {
			std::ostringstream o;
			o << srcFileName << " (" << uniqueName_ << "): ERROR in addFPOutput, signal " << name<< " seems to already exist";
			throw o.str();
		}
		Signal *s = new Signal(name, Signal::out, wE, wF) ;
		s -> setNumberOfPossibleValues(numberOfPossibleOutputValues);
		ioList_.push_back(s);
		for(int i=0; i<numberOfPossibleOutputValues; i++) 
			testCaseSignals_.push_back(s);
		signalMap_[name] = s ;
		numberOfOutputs_ ++;
		//		declareTable[name] = s->getCycle();
	}
	
	
	void Operator::addIEEEInput(const std::string name, const int wE, const int wF) {
		if (signalMap_.find(name) != signalMap_.end()) {
			std::ostringstream o;
			o << srcFileName << " (" << uniqueName_ << "): ERROR in addIEEEInput, signal " << name<< " seems to already exist";
			throw o.str();
		}
		Signal *s = new Signal(name, Signal::in, wE, wF, true);
		s->setCycle(0);
		ioList_.push_back(s);
		signalMap_[name] = s ;
		numberOfInputs_ ++;
		declareTable[name] = s->getCycle();
	}
	
	void Operator::addIEEEOutput(const std::string name, const int wE, const int wF, const int numberOfPossibleOutputValues) {
		if (signalMap_.find(name) != signalMap_.end()) {
			std::ostringstream o;
			o << srcFileName << " (" << uniqueName_ << "): ERROR in addIEEEOutput, signal " << name<< " seems to already exist";
			throw o.str();
		}
		Signal *s = new Signal(name, Signal::out, wE, wF, true) ;
		s -> setNumberOfPossibleValues(numberOfPossibleOutputValues);
		ioList_.push_back(s);
		for(int i=0; i<numberOfPossibleOutputValues; i++) 
			testCaseSignals_.push_back(s);
		signalMap_[name] = s ;
		numberOfOutputs_ ++;
		//		declareTable[name] = s->getCycle();
	}
	
	
	
	Signal * Operator::getSignalByName(string name) {
		ostringstream e;
		if(signalMap_.find(name) ==  signalMap_.end()) {
			e << srcFileName << " (" << uniqueName_ << "): ERROR in getSignalByName, signal " << name<< " not declared";
			throw e.str();
		}
		return signalMap_[name];
	}
	
	bool Operator::isSignalDeclared(string name){
		ostringstream e;
		if(signalMap_.find(name) ==  signalMap_.end()) {
			return false;
		}
		return true;
	}
	
	void Operator::setName(std::string prefix, std::string postfix){
		ostringstream pr, po;
		if (prefix.length()>0)
			pr << prefix << "_"; 
		else 
			pr << "";
		if (postfix.length()>0)
			po << "_"<<postfix;
		else
			po << "";
		uniqueName_ = pr.str() + uniqueName_ + po.str();
	}
	
	void Operator::setName(std::string operatorName){
		uniqueName_ = operatorName;
	}
	
	void Operator::setNameWithFreq(std::string operatorName){
		std::ostringstream o;
		o <<  operatorName <<  "_" ;
		if(target_->isPipelined()) 
			o << target_->frequencyMHz() ;
		else
			o << "comb";
		uniqueName_ = o.str();
	}
	
	void  Operator::changeName(std::string operatorName){
		commentedName_ = uniqueName_;
		uniqueName_ = operatorName;
	}
	
	
	string Operator::getName() const{
		return uniqueName_;
	}
	
	int Operator::getIOListSize() const{
		return ioList_.size();
	}
	
	vector<Signal*> * Operator::getIOList(){
		return &ioList_; 
	}
	
	Signal * Operator::getIOListSignal(int i){
		return ioList_[i];
	}
	
	
	
	void  Operator::outputVHDLSignalDeclarations(std::ostream& o) {
		for (unsigned int i=0; i < this->signalList_.size(); i++){
			Signal* s = this->signalList_[i];
			o<<tab<<  s->toVHDL() << ";" << endl;
		}
		
	}
	
	
	
	void Operator::outputVHDLComponent(std::ostream& o, std::string name) {
		unsigned int i;
		o << tab << "component " << name << " is" << endl;
		if (ioList_.size() > 0)
		{
			o << tab << tab << "port ( ";
			if(isSequential()) {
				// add clk, rst, etc. signals which are not member of iolist
				if(hasClockEnable()) 
					o << "clk, rst, ce : in std_logic;" <<endl;
				else if(isRecirculatory())
					o << "clk, rst stall_s: in std_logic;" <<endl;
				else 
					o << "clk, rst : in std_logic;" <<endl;
			}

			for (i=0; i<this->ioList_.size(); i++){
				Signal* s = this->ioList_[i];
				if (i>0 || isSequential()) // align signal names 
					o<<tab<<"          ";
				o<<  s->toVHDL();
				if(i < this->ioList_.size()-1)  o<<";" << endl;
			}
			o << tab << ");"<<endl;
		}
		o << tab << "end component;" << endl;
	}
	
	void Operator::outputVHDLComponent(std::ostream& o) {
		this->outputVHDLComponent(o,  this->uniqueName_); 
	}
	

	
	void Operator::outputVHDLEntity(std::ostream& o) {
		unsigned int i;
		o << "entity " << uniqueName_ << " is" << endl;
		if (ioList_.size() > 0)
		{
			o << tab << "port ( ";
			if(isSequential()) {
					// add clk, rst, etc. signals which are not member of iolist
				if(hasClockEnable()) 
					o << "clk, rst, ce : in std_logic;" <<endl;
				else if(isRecirculatory()) 
					o << "clk, rst stall_s: in std_logic;" <<endl;
				else 
					o << "clk, rst : in std_logic;" <<endl;
			}
			
			for (i=0; i<this->ioList_.size(); i++){
				Signal* s = this->ioList_[i];
				if (i>0 || isSequential()) // align signal names 
					o<<"          ";
				o<<  s->toVHDL();
				if(i < this->ioList_.size()-1)  o<<";" << endl;
			}
			
			o << tab << ");"<<endl;
		}
		o << "end entity;" << endl << endl;
	}
	
	
	void Operator::setCopyrightString(std::string authorsYears){
		copyrightString_ = authorsYears;
	}
	
	
	void Operator::licence(std::ostream& o){
		licence(o, copyrightString_);
	}
	
	
	void Operator::licence(std::ostream& o, std::string authorsyears){
		o<<"--------------------------------------------------------------------------------"<<endl;
		// centering the unique name
		int s, i;
		if(uniqueName_.size()<76) s = (76-uniqueName_.size())/2; else s=0;
		o<<"--"; for(i=0; i<s; i++) o<<" "; o  << uniqueName_ << endl; 
		
		// if this operator was renamed from the command line, show the original name
		if(commentedName_!="") {
			if(commentedName_.size()<74) s = (74-commentedName_.size())/2; else s=0;
			o<<"--"; for(i=0; i<s; i++) o<<" "; o << "(" << commentedName_ << ")" << endl; 
		}
		
		o<<"-- This operator is part of the Infinite Virtual Library FloPoCoLib"<<endl;
		o<<"-- All rights reserved "<<endl;
		o<<"-- Authors: " << authorsyears <<endl;
		o<<"--------------------------------------------------------------------------------"<<endl;
	}
	
	
	
	void Operator::pipelineInfo(std::ostream& o){
		if(isSequential())
			o<<"-- Pipeline depth: " <<getPipelineDepth() << " cycles"  <<endl <<endl;
		else 
			o<<"-- combinatorial"  <<endl <<endl;
	}
	

	void Operator::stdLibs(std::ostream& o){
		o << "library ieee;"<<endl
		  << "use ieee.std_logic_1164.all;"<<endl;
		  
		if(stdLibType_==0){
			o << "use ieee.std_logic_arith.all;"<<endl
			  << "use ieee.std_logic_unsigned.all;"<<endl; 
		}
		if(stdLibType_==-1){
			o << "use ieee.std_logic_arith.all;"<<endl
			  << "use ieee.std_logic_signed.all;"<<endl; 
		}
		if(stdLibType_==1){
			o << "use ieee.numeric_std.all;"<<endl;			
		}
		if(stdLibType_==2){
			o << "use ieee.numeric_std.all;"<<endl
			  << "use ieee.std_logic_signed.all;"<<endl; 
		}
		if(stdLibType_==3){
			o << "use ieee.numeric_std.all;"<<endl
			  << "use ieee.std_logic_unsigned.all;"<<endl;
		}

		o << "library std;" << endl
		  << "use std.textio.all;"<< endl 
		  << "library work;"<<endl<< endl;
	};


	void Operator::outputVHDL(std::ostream& o) {
		this->outputVHDL(o, this->uniqueName_); 
	}
	
	bool Operator::isSequential() {
		return isSequential_; 
	}
	
	bool Operator::isRecirculatory() {
		return needRecirculationSignal_; 
	}
	
	void Operator::setSequential() {
		isSequential_=true; 
		vhdl.disableParsing(false); 
	}
	
	void Operator::setCombinatorial() {
		isSequential_=false;
		vhdl.disableParsing(true); 
	}
	
	void Operator::setRecirculationSignal() {
		needRecirculationSignal_ = true;
	}
	
	
	int Operator::getPipelineDepth() {
		return pipelineDepth_; 
	}
	
	void Operator::outputFinalReport(int level) {

		if (getIndirectOperator()!=NULL){ // interface operator
			if(getOpList().size()!=1){
				ostringstream o;
				o << "!?! Operator " << getUniqueName() << " is an interface operator with " << getOpList().size() << "children";
				throw o.str();
			}
			getOpListR()[0]->outputFinalReport(level);
		}

		else{ // Hard operator
			for (unsigned i=0; i< getOpList().size(); i++)
				if (! getOpListR().empty())
					getOpListR()[i]->outputFinalReport(level+1);	
			
			ostringstream tabs, ctabs;
			for (int i=0;i<level-1;i++){
				tabs << "|" << tab;
				ctabs << "|" << tab;
			}
			
			if (level>0){
				tabs << "|" << "---";
				ctabs << "|" << tab;
			}
			
			cerr << tabs.str() << "Entity " << uniqueName_ << endl;
			if(this->getPipelineDepth()!=0)
				cerr << ctabs.str() << tab << "Pipeline depth = " << getPipelineDepth() << endl;
			else
				cerr << ctabs.str() << tab << "Not pipelined"<< endl;
		}
	}


	void Operator::setCycle(int cycle, bool report) {
		criticalPath_ = 0;
		// lexing part
		vhdl.flush(currentCycle_);
		if(isSequential()) {
			currentCycle_=cycle;
			vhdl.setCycle(currentCycle_);
			if(report){
				vhdl << tab << "----------------Synchro barrier, entering cycle " << currentCycle_ << "----------------" << endl ;
			}
			// automatically update pipeline depth of the operator 
			if (currentCycle_ > pipelineDepth_) 
				pipelineDepth_ = currentCycle_;
		}
	}
	
	int Operator::getCurrentCycle(){
		return currentCycle_;
	} 
	
	void Operator::nextCycle(bool report) {
		// lexing part
		vhdl.flush(currentCycle_);
		
		if(isSequential()) {
			
			currentCycle_ ++; 
			vhdl.setCycle(currentCycle_);
			if(report)
				vhdl << tab << "----------------Synchro barrier, entering cycle " << currentCycle_ << "----------------" << endl;
			
			criticalPath_ = 0;
			// automatically update pipeline depth of the operator 
			if (currentCycle_ > pipelineDepth_) 
				pipelineDepth_ = currentCycle_;
		}
	}

	void Operator::previousCycle(bool report) {
		// lexing part
		vhdl.flush(currentCycle_);
		
		if(isSequential()) {
			
			currentCycle_ --; 
			vhdl.setCycle(currentCycle_);
			if(report)
				vhdl << tab << "----------------Synchro barrier, entering cycle " << currentCycle_ << "----------------" << endl;
			
		}
	}
	
	
	void Operator::setCycleFromSignal(string name, bool report) {
		setCycleFromSignal(name, 0.0, report);
	}
	
	
	void Operator::setCycleFromSignal(string name, double criticalPath, bool report) {
		// lexing part
		vhdl.flush(currentCycle_);
		
		ostringstream e;
		e << srcFileName << " (" << uniqueName_ << "): ERROR in setCycleFromSignal, "; // just in case
		
		if(isSequential()) {
			Signal* s;
			try {
				s=getSignalByName(name);
			}
			catch (string e2) {
				e << endl << tab << e2;
				throw e.str();
			}
			
			if( s->getCycle() < 0 ) {
				ostringstream o;
				o << "signal " << name<< " doesn't have (yet?) a valid cycle";
				throw o.str();
			} 
			
			currentCycle_ = s->getCycle();
			criticalPath_ = criticalPath;
			vhdl.setCycle(currentCycle_);
			
			if(report)
				vhdl << tab << "---------------- cycle " << currentCycle_ << "----------------" << endl ;
			// automatically update pipeline depth of the operator 
			if (currentCycle_ > pipelineDepth_) 
				pipelineDepth_ = currentCycle_;
		}
	}
	
	
	int Operator::getCycleFromSignal(string name, bool report) {
		// lexing part
		vhdl.flush(currentCycle_);
		
		ostringstream e;
		e << srcFileName << " (" << uniqueName_ << "): ERROR in getCycleFromSignal, "; // just in case
		
		if(isSequential()) {
			Signal* s;
			try {
				s=getSignalByName(name);
			}
			catch (string e2) {
				e << endl << tab << e2;
				throw e.str();
			}
			
			if( s->getCycle() < 0 ) {
				ostringstream o;
				o << "signal " << name<< " doesn't have (yet?) a valid cycle";
				throw o.str();
			} 
			
			return s->getCycle();
		}else{
			return 0; //if combinatorial everything happens at cycle 0
		}
	}
	
	
	bool Operator::syncCycleFromSignal(string name, bool report) {
		return(syncCycleFromSignal(name, 0.0, report));
	}



	bool Operator::syncCycleFromSignal(string name, double criticalPath, bool report) {

		bool advanceCycle = false;

		// lexing part
		vhdl.flush(currentCycle_);
		ostringstream e;
		e << srcFileName << " (" << uniqueName_ << "): ERROR in syncCycleFromSignal, "; // just in case
		
		if(isSequential()) {
			Signal* s;
			try {
				s=getSignalByName(name);
			}
			catch (string e2) {
				e << endl << tab << e2;
				throw e.str();
			}
			
			if( s->getCycle() < 0 ) {
				ostringstream o;
				o << "signal " << name << " doesn't have (yet?) a valid cycle";
				throw o.str();
			} 

			if (s->getCycle() == currentCycle_){
				advanceCycle = false;
				if (criticalPath>criticalPath_)
					criticalPath_=criticalPath ;
			}
						
			if (s->getCycle() > currentCycle_){
				advanceCycle = true;
				currentCycle_ = s->getCycle();
				criticalPath_= criticalPath;
				vhdl.setCycle(currentCycle_);
			}
			
			// if (s->getCycle() < currentCycle_) do nothing: 
			//   the argument signal will be delayed, so its critical path will be 0

			// cout << tab << "----------------Synchro barrier on " << s->getName() << ",  entering cycle " << currentCycle_ << "----------------"  ;

			if(report && advanceCycle)
				vhdl << tab << "----------------Synchro barrier, entering cycle " << currentCycle_ << "----------------" << endl ;

			// automatically update pipeline depth of the operator 
			if (currentCycle_ > pipelineDepth_) 
				pipelineDepth_ = currentCycle_;
		}
		
		return advanceCycle;
	}

	void Operator::setSignalDelay(string name, double delay){
		Signal* s;
		try {
			s=getSignalByName(name);
		}
		catch (string e2) {
			cout << "WARNING: signal " << name << " was not found in file " << srcFileName << " when called using setSignalDelay" << endl;
			return;
		}

		s->setDelay(delay);		
	}	

	double Operator::getSignalDelay(string name){
		Signal* s;
		try {
			s=getSignalByName(name);
		}
		catch (string e2) {
			cout << "WARNING: signal " << name << " was not found in file " << srcFileName << " when called using getSignalDelay" << endl;
			return 0.0;
		}

		return s->getDelay();		
	}

	double Operator::getCriticalPath() {return criticalPath_;}
	
	void Operator::setCriticalPath(double delay) {criticalPath_=delay;}
	
	void Operator::addToCriticalPath(double delay){
		criticalPath_ += delay;
	}
	
//	bool Operator::manageCriticalPath(double delay, bool report){
//		//		criticalPath_ += delay;
//		if ( target_->ffDelay() + (criticalPath_ + delay) + target_->localWireDelay() > (1.0/target_->frequency())){
//			nextCycle(report); //TODO Warning
//			criticalPath_ = min(delay, 1.0/target_->frequency());
//			return true;
//		}
//		else{
//			criticalPath_ += delay;
//			return false;
//		}
//	}

	bool Operator::manageCriticalPath(double delay, bool report){
		//		criticalPath_ += delay;
				if(isSequential()) {
					if ( target_->ffDelay() + (criticalPath_ + delay) > (1.0/target_->frequency())){
						nextCycle(report); //TODO Warning
						criticalPath_ = min(delay, 1.0/target_->frequency());
						return true;
					}
					else{
						criticalPath_ += delay;
						return false;
					}
				}
				else {
					criticalPath_ += delay;
					return false;
				}
	}

	
	double Operator::getOutputDelay(string s) {return outDelayMap[s];}  // TODO add checks
	
	string Operator::declare(string name, const int width, bool isbus, Signal::SignalType regType) {
		Signal* s;
		ostringstream e;
		// check the signals doesn't already exist
		if(signalMap_.find(name) !=  signalMap_.end()) {
			e << srcFileName << " (" << uniqueName_ << "): ERROR in declare(), signal " << name<< " already exists";
			throw e.str();
		}
		// construct the signal (lifeSpan and cycle are reset to 0 by the constructor)
		s = new Signal(name, regType, width, isbus);
		if((regType==Signal::registeredWithoutReset) || (regType==Signal::registeredWithZeroInitialiser))
			hasRegistersWithoutReset_ = true;
		if(regType==Signal::registeredWithSyncReset)
			hasRegistersWithSyncReset_ = true;
		if(regType==Signal::registeredWithAsyncReset)
			hasRegistersWithAsyncReset_ = true;
		
		// define its cycle 
		if(isSequential())
			s->setCycle(this->currentCycle_);
		
		// add this signal to the declare table
		declareTable[name] = s->getCycle();
		
		// add the signal to signalMap and signalList
		signalList_.push_back(s);    
		signalMap_[name] = s ;
		return name;
	}
	
	
	#if 1
	string Operator::use(string name) {
		ostringstream e;
		e << "ERROR in use(), "; // just in case
		
		if(isSequential()) {
			Signal *s;
			try {
				s=getSignalByName(name);
			}
			catch (string e2) {
				e << endl << tab << e2;
				throw e.str();
			}
			if(s->getCycle() < 0) {
				e << "signal " << name<< " doesn't have (yet?) a valid cycle";
				throw e.str();
			} 
			if(s->getCycle() > currentCycle_) {
				ostringstream e;
				e << "active cycle of signal " << name<< " is later than current cycle, cannot delay it";
				throw e.str();
			} 
			// update the lifeSpan of s
			s->updateLifeSpan( currentCycle_ - s->getCycle() );
			//return s->delayedName( currentCycle_ - s->getCycle() );
			return s->delayedName( 0 );
		}
		else
			return name;
	}
	
	string Operator::use(string name, int delay) {
		
		ostringstream e;
		e << "ERROR in use(), "; // just in case
		
		if(isSequential()) {
			Signal *s;
			try {
				s=getSignalByName(name);
			}
			catch (string e2) {
				e << endl << tab << e2;
				throw e.str();
			}
			// update the lifeSpan of s
			
			s->updateLifeSpan( delay );
			//return s->delayedName( currentCycle_ - s->getCycle() );
			return s->delayedName( delay );
		}else
			return name;
	}
	
	#endif
	
	void Operator::outPortMap(Operator* op, string componentPortName, string actualSignalName, bool newSignal){
		Signal* formal;
		Signal* s;
		ostringstream e;
		e << srcFileName << " (" << uniqueName_ << "): ERROR in outPortMap() for entity " << op->getName()  << ", "; // just in case
		// check the signals doesn't already exist
		if(signalMap_.find(actualSignalName) !=  signalMap_.end() && newSignal) {
			e << "signal " << actualSignalName << " already exists";
			throw e.str();
		}
		try {
			formal=op->getSignalByName(componentPortName);
		}
		catch (string e2) {
			e << endl << tab << e2;
			throw e.str();
		}
		if (formal->type()!=Signal::out){
			e << "signal " << componentPortName << " of component " << op->getName() 
			<< " doesn't seem to be an output port";
			throw e.str();
		}
		if (newSignal) {
			int width = formal -> width();
			bool isbus = formal -> isBus();
			// construct the signal (lifeSpan and cycle are reset to 0 by the constructor)
			s = new Signal(actualSignalName, Signal::wire, width, isbus);
			// define its cycle 
			if(isSequential())
				s->setCycle( this->currentCycle_ + op->getPipelineDepth() );
		
			// add this signal to the declare table
			declareTable[actualSignalName] = s->getCycle();
			
			// add the signal to signalMap and signalList
			signalList_.push_back(s);    
			signalMap_[actualSignalName] = s ;
		};
		// add the mapping to the mapping list of Op
		op->portMap_[componentPortName] = actualSignalName;
	}
	
	
	void Operator::inPortMap(Operator* op, string componentPortName, string actualSignalName){
		Signal* formal;
		ostringstream e;
		string name;
		e  << srcFileName << " (" << uniqueName_ << "): ERROR in inPortMap() for entity " << op->getName() << ","; // just in case
		
		if(isSequential()) {
			Signal *s;
			try {
				s=getSignalByName(actualSignalName);
			}
			catch (string e2) {
				e << endl << tab << e2;
				throw e.str();
			}
			if(s->getCycle() < 0) {
				ostringstream e;
				e << "signal " << actualSignalName<< " doesn't have (yet?) a valid cycle";
				throw e.str();
			} 
			if(s->getCycle() > currentCycle_) {
				ostringstream e;
				e << "active cycle of signal " << actualSignalName<< " is later than current cycle, cannot delay it";
				throw e.str();
			} 
			// update the lifeSpan of s
			s->updateLifeSpan( currentCycle_ - s->getCycle() );
			name = s->delayedName( currentCycle_ - s->getCycle() );
		}
		else
			name = actualSignalName;
		
		try {
			formal=op->getSignalByName(componentPortName);
		}
		catch (string e2) {
			e << endl << tab << e2;
			throw e.str();
		}
		if (formal->type()!=Signal::in){
			e << "signal " << componentPortName << " of component " << op->getName() 
			<< " doesn't seem to be an input port";
			throw e.str();
		}
		
		// add the mapping to the mapping list of Op
		op->portMap_[componentPortName] = name;
	}
	
	
	
	void Operator::inPortMapCst(Operator* op, string componentPortName, string actualSignal){
		Signal* formal;
		ostringstream e;
		string name;
		e << srcFileName << " (" << uniqueName_ << "): ERROR in inPortMapCst() for entity " << op->getName()  << ", "; // just in case
		
		try {
			formal=op->getSignalByName(componentPortName);
		}
		catch (string e2) {
			e << endl << tab << e2;
			throw e.str();
		}
		if (formal->type()!=Signal::in){
			e << "signal " << componentPortName << " of component " << op->getName() 
			<< " doesn't seem to be an input port";
			throw e.str();
		}
		
		// add the mapping to the mapping list of Op
		op->portMap_[componentPortName] = actualSignal;
	}
	
	
	string Operator::instance(Operator* op, string instanceName){
		ostringstream o;
		// TODO add checks here? Check that all the signals are covered for instance
		
		o << tab << instanceName << ": " << op->getName();
		if (op->isSequential()) 
			o << "  -- pipelineDepth="<< op->getPipelineDepth() << " maxInDelay=" << getMaxInputDelays(op->inputDelayMap);
		o << endl;
		o << tab << tab << "port map ( ";
		// build vhdl and erase portMap_
		map<string,string>::iterator it;
		if(op->isSequential()) {
			o << "clk  => clk";
			o << "," << endl << tab << tab << "           rst  => rst";
			if (op->isRecirculatory()) {
				o << "," << endl << tab << tab << "           stall_s => stall_s";
			};
			if (op->hasClockEnable()) {
				o << "," << endl << tab << tab << "           ce => ce";
			};
		}
		
		for (it=op->portMap_.begin()  ; it != op->portMap_.end(); it++ ) {
			bool outputSignal = false;
			for ( int k = 0; k < int(op->ioList_.size()); k++){
				if ((op->ioList_[k]->type() == Signal::out) && ( op->ioList_[k]->getName() == (*it).first )){ 
					outputSignal = true;
				}
			}
			
			bool parsing = vhdl.isParsing();
			
			if ( outputSignal && parsing){
				vhdl.flush(currentCycle_);
				vhdl.disableParsing(true);
			}
			
			if (it!=op->portMap_.begin() || op->isSequential())				
				o << "," << endl <<  tab << tab << "           ";
				
				o << (*it).first << " => "  << (*it).second;
			
			if ( outputSignal && parsing ){
				vhdl << o.str();
				vhdl.flush(currentCycle_);
				o.str("");
				vhdl.disableParsing(!parsing);
			}
			//op->portMap_.erase(it);
		}
		o << ");" << endl;
		
		
		//Floorplanning related-----------------------------------------
		floorplan << manageFloorplan();
		flpHelper->addToFlpComponentList(op->getName());
		flpHelper->addToInstanceNames(op->getName(), instanceName);
		//--------------------------------------------------------------
		
		
		// add the operator to the subcomponent list 
		subComponents_[op->getName()]  = op;
		return o.str();
	}


	
	string Operator::buildVHDLSignalDeclarations() {
		ostringstream o;
		for(unsigned int i=0; i<signalList_.size(); i++) {
			Signal *s = signalList_[i];
			o << s->toVHDLDeclaration() << endl;
		}
		//now the signals from the I/O List which have the cycle>0
		for (unsigned int i=0; i<ioList_.size(); i++) {
			Signal *s = ioList_[i];
			if (s->getLifeSpan()>0){
				o << s->toVHDLDeclaration() << endl;	
			}
			
		}
		
		return o.str();	
	}
	
	
	void Operator::useHardRAM(Operator* t) {
		if (target_->getVendor() == "Xilinx") 
		{
			addAttribute("rom_extract", "string", t->getName()+": component", "yes");
			addAttribute("rom_style", "string", t->getName()+": component", "block");
		}
		if (target_->getVendor() == "Altera") 
			addAttribute("altera_attribute", "string", t->getName()+": component", "-name ALLOW_ANY_ROM_SIZE_FOR_RECOGNITION ON");
	}
	
	void Operator::useSoftRAM(Operator* t) {
		if (target_->getVendor() == "Xilinx") 
		{
			addAttribute("rom_extract", "string", t->getName()+": component", "yes");
			addAttribute("rom_style", "string", t->getName()+": component", "distributed");
		}
		if (target_->getVendor() == "Altera") 
			addAttribute("altera_attribute", "string", t->getName()+": component", "-name ALLOW_ANY_ROM_SIZE_FOR_RECOGNITION OFF");
	}
	
	
	
	string Operator::buildVHDLComponentDeclarations() {
		ostringstream o;
		for(map<string, Operator*>::iterator it = subComponents_.begin(); it !=subComponents_.end(); it++) {
			Operator *op = it->second;
			op->outputVHDLComponent(o);
			o<< endl;
		}
		return o.str();	
	}
	
	
	void Operator::addConstant(std::string name, std::string t, mpz_class v) {
		ostringstream tmp; 
		tmp << v;
		constants_[name] =  make_pair(t, tmp.str());
	}
	
	void Operator::addType(std::string name, std::string value) {
		types_ [name] =  value;
	}
	
	void Operator::addConstant(std::string name, std::string t, int v) {
		ostringstream tmp; 
		tmp << v;
		constants_[name] =  make_pair(t, tmp.str());
	}
	
	void Operator::addConstant(std::string name, std::string t, string v) {
		constants_[name] =  make_pair(t, v);
	}
	
	
	void Operator::addAttribute(std::string attributeName,  std::string attributeType,  std::string object, std::string value ) {
		// TODO add some checks ?
		attributes_[attributeName] = attributeType;
		pair<string,string> p = make_pair(attributeName,object);
		attributesValues_[p] = value;
	}
	
	
	string Operator::buildVHDLTypeDeclarations() {
		ostringstream o;
		for(map<string, string >::iterator it = types_.begin(); it !=types_.end(); it++) {
			string name  = it->first;
			string value = it->second;
			o <<  "type " << name << " is "  << value << ";" << endl;
		}
		return o.str();	
	}
	
	
	string Operator::buildVHDLConstantDeclarations() {
		ostringstream o;
		for(map<string, pair<string, string> >::iterator it = constants_.begin(); it !=constants_.end(); it++) {
			string name  = it->first;
			string type = it->second.first;
			string value = it->second.second;
			o <<  "constant " << name << ": " << type << " := " << value << ";" << endl;
		}
		return o.str();	
	}
	
	
	
	string Operator::buildVHDLAttributes() {
		ostringstream o;
		// First add the declarations of attribute names
		for(map<string, string>::iterator it = attributes_.begin(); it !=attributes_.end(); it++) {
			string name  = it->first;
			string type = it->second;
			o <<  "attribute " << name << ": " << type << ";" << endl;
		}
		// Then add the declarations of attribute values
		for(map<pair<string, string>, string>::iterator it = attributesValues_.begin(); it !=attributesValues_.end(); it++) {
			string name  = it->first.first;
			string object = it->first.second;
			string value = it->second;
			if(attributes_[name]=="string")
				value = '"' + value + '"';
			o <<  "attribute " << name << " of " << object << " is " << value << ";" << endl;
		}
		return o.str();	
	}
	



	string  Operator::buildVHDLRegisters() {
		ostringstream o;
		
		// execute only if the operator is sequential, otherwise output nothing
		string recTab = "";
		if ( isRecirculatory() || hasClockEnable() )
			recTab = tab;
		if (isSequential()){
			// First registers without reset
			o << tab << "process(clk)" << endl;
			o << tab << tab << "begin" << endl;
			o << tab << tab << tab << "if clk'event and clk = '1' then" << endl;
			if (isRecirculatory()) o << tab << tab << tab << tab << "if stall_s = '0' then" << endl;
			else if (hasClockEnable()) o << tab << tab << tab << tab << "if ce = '1' then" << endl;
			for(unsigned int i=0; i<signalList_.size(); i++) {
				Signal *s = signalList_[i];
				if ((s->type() == Signal::registeredWithoutReset) || (s->type()==Signal::registeredWithZeroInitialiser) || (s->type() == Signal::wire)) 
					if(s->getLifeSpan() >0) {
						for(int j=1; j <= s->getLifeSpan(); j++)
							o << recTab << tab << tab <<tab << tab << s->delayedName(j) << " <=  " << s->delayedName(j-1) <<";" << endl;
					}
			}
			for(unsigned int i=0; i<ioList_.size(); i++) {
				Signal *s = ioList_[i];
				if(s->getLifeSpan() >0) {
					for(int j=1; j <= s->getLifeSpan(); j++)
						o << recTab << tab << tab <<tab << tab << s->delayedName(j) << " <=  " << s->delayedName(j-1) <<";" << endl;
				}
			}
			if (isRecirculatory() || hasClockEnable()) 
				o << tab << tab << tab << tab << "end if;" << endl;
			o << tab << tab << tab << "end if;\n";
			o << tab << tab << "end process;\n"; 
			
			// then registers with asynchronous reset
			if (hasRegistersWithAsyncReset_) {
				o << tab << "process(clk, rst)" << endl;
				o << tab << tab << "begin" << endl;
				o << tab << tab << tab << "if rst = '1' then" << endl;
				for(unsigned int i=0; i<signalList_.size(); i++) {
					Signal *s = signalList_[i];
					if (s->type() == Signal::registeredWithAsyncReset)  
						if(s->getLifeSpan() >0) {
							for(int j=1; j <= s->getLifeSpan(); j++){
								if ( (s->width()>1) || (s->isBus()))
									o << tab << tab <<tab << tab << s->delayedName(j) << " <=  (others => '0');" << endl;
								else
									o << tab <<tab << tab << tab << s->delayedName(j) << " <=  '0';" << endl;
							}
						}
				}			
				o << tab << tab << tab << "elsif clk'event and clk = '1' then" << endl;
				if (isRecirculatory()) o << tab << tab << tab << tab << "if stall_s = '0' then" << endl;
				else if (hasClockEnable()) o << tab << tab << tab << tab << "if ce = '1' then" << endl;
				for(unsigned int i=0; i<signalList_.size(); i++) {
					Signal *s = signalList_[i];
					if (s->type() == Signal::registeredWithAsyncReset)  
						if(s->getLifeSpan() >0) {
							for(int j=1; j <= s->getLifeSpan(); j++)
								o << recTab << tab <<tab << tab << tab << s->delayedName(j) << " <=  " << s->delayedName(j-1) <<";" << endl;
						}
				}
			if (isRecirculatory() || hasClockEnable()) 
				o << tab << tab << tab << tab << "end if;" << endl;
				o << tab << tab << tab << "end if;" << endl;
				o << tab << tab <<"end process;" << endl;
			}
			
			// then registers with synchronous reset
			if (hasRegistersWithSyncReset_) {
				o << tab << "process(clk, rst)" << endl;
				o << tab << tab << "begin" << endl;
				o << tab << tab << tab << "if clk'event and clk = '1' then" << endl;
				o << tab << tab << tab << tab << "if rst = '1' then" << endl;
				for(unsigned int i=0; i<signalList_.size(); i++) {
					Signal *s = signalList_[i];
					if (s->type() == Signal::registeredWithSyncReset)  
						if(s->getLifeSpan() >0) {
							for(int j=1; j <= s->getLifeSpan(); j++){
								if ( (s->width()>1) || (s->isBus()))
									o << tab <<tab << tab <<tab << tab << s->delayedName(j) << " <=  (others => '0');" << endl;
								else
									o << tab <<tab << tab <<tab << tab << s->delayedName(j) << " <=  '0';" << endl;
							}
						}
				}			
				o << tab << tab << tab << tab << "else" << endl;
				if (isRecirculatory()) o << tab << tab << tab << tab << "if stall_s = '0' then" << endl;
				else if (hasClockEnable()) o << tab << tab << tab << tab << "if ce = '1' then" << endl;
				for(unsigned int i=0; i<signalList_.size(); i++) {
					Signal *s = signalList_[i];
					if (s->type() == Signal::registeredWithSyncReset)  
						if(s->getLifeSpan() >0) {
							for(int j=1; j <= s->getLifeSpan(); j++)
								o << tab <<tab << tab <<tab << tab << s->delayedName(j) << " <=  " << s->delayedName(j-1) <<";" << endl;
						}
				}			
				if (isRecirculatory() || hasClockEnable()) 
					o << tab << tab << tab << tab << "end if;" << endl;
				o << tab << tab << tab << tab << "end if;" << endl;
				o << tab << tab << tab << "end if;" << endl;
				o << tab << tab << "end process;" << endl;
			}
		}
		return o.str();
	}
	
	
	void Operator::buildStandardTestCases(TestCaseList* tcl) {
		// Each operator should overload this method. If not, it is mostly harmless but deserves a warning.
		cerr << "WARNING: No standard test cases implemented for this operator" << endl;
	}
	
	
	
	
	void Operator::buildRandomTestCaseList(TestCaseList* tcl, int n){
		
		TestCase *tc;
		/* Generate test cases using random input numbers */
		for (int i = 0; i < n; i++) {
			// TODO free all this memory when exiting TestBench
			tc = buildRandomTestCase(i); 
			tcl->add(tc);
		}
	}
	
	TestCase* Operator::buildRandomTestCase(int i){
		TestCase *tc = new TestCase(this);
		/* Generate test cases using random input numbers */
		// TODO free all this memory when exiting TestBench
		/* Fill inputs */
		for (unsigned int j = 0; j < ioList_.size(); j++) {
			Signal* s = ioList_[j]; 
			if (s->type() == Signal::in) {
				mpz_class a = getLargeRandom(s->width());
				tc->addInput(s->getName(), a);
			}
		}
		/* Get correct outputs */
		emulate(tc);
		
		//		cout << tc->getInputVHDL();
		//    cout << tc->getExpectedOutputVHDL();
		
		
		// add to the test case list
		return tc;
	}
	
	map<string, double> Operator::getOutDelayMap(){
		return outDelayMap;
	}
	
	map<string, int> Operator::getDeclareTable(){
		return declareTable;
	}
	
	void Operator::outputVHDL(std::ostream& o, std::string name) {
		if (! vhdl.isEmpty() ){
			licence(o);
			pipelineInfo(o);
			stdLibs(o);
			outputVHDLEntity(o);
			newArchitecture(o,name);
			o << buildVHDLComponentDeclarations();	
			o << buildVHDLSignalDeclarations();
			o << buildVHDLTypeDeclarations();
			o << buildVHDLConstantDeclarations();
			o << buildVHDLAttributes();
			beginArchitecture(o);		
			o<<buildVHDLRegisters();
			if(getIndirectOperator())
				o << getIndirectOperator()->vhdl.str();
			else
				o << vhdl.str();
			endArchitecture(o);
		}
	}
	
	void Operator::parse2(){
		REPORT(DEBUG, "Starting second-level parsing for operator "<<srcFileName);
		vector<pair<string,int> >:: iterator iterUse;
		map<string, int>::iterator iterDeclare;
		
		string name;
		int declareCycle, useCycle;
		
		string str (vhdl.str());
		
		/* parse the useTable and check that the declarations are ok */
		for (iterUse = (vhdl.useTable).begin(); iterUse!=(vhdl.useTable).end();++iterUse){
			name     = (*iterUse).first;
			useCycle = (*iterUse).second;
			
			ostringstream tSearch;
			ostringstream tReplace;
			string replaceString;
			
			tSearch << "__"<<name<<"__"<<useCycle<<"__"; 
			string searchString (tSearch.str());
			
			iterDeclare = declareTable.find(name);
			declareCycle = iterDeclare->second;
			
			if (iterDeclare != declareTable.end()){
				tReplace << use(name, useCycle - declareCycle); 
				replaceString = tReplace.str();
				if (useCycle<declareCycle){
					if(!hasDelay1Feedbacks_){
						cerr << srcFileName << " (" << uniqueName_ << "): WARNING: Signal " << name <<" defined @ cycle "<<declareCycle<<" and used @ cycle " << useCycle <<endl;
						cerr << srcFileName << " (" << uniqueName_ << "): If this is a feedback signal you may ignore this warning"<<endl;
					}else{
						if(declareCycle - useCycle != 1){
							cerr << srcFileName << " (" << uniqueName_ << "): ERROR: Signal " << name <<" defined @ cycle "<<declareCycle<<" and used @ cycle " << useCycle <<endl;
							exit(1);
						}
					}
				}
			}else{
				/* parse the declare by hand and check lower/upper case */
				bool found = false;
				string tmp;
				for (iterDeclare = declareTable.begin(); iterDeclare!=declareTable.end();++iterDeclare){
					tmp = iterDeclare->first;
					if ( (to_lowercase(tmp)).compare(to_lowercase(name))==0){
						found = true;
						break;
					}
				}
				
				if (found == true){
					cerr  << srcFileName << " (" << uniqueName_ << "): ERROR: Clash on signal:"<<name<<". Definition used signal name "<<tmp<<". Check signal case!"<<endl;
					exit(-1);
				}
				
				tReplace << name;
				replaceString = tReplace.str();
			}
			
			if ( searchString != replaceString ){
				string::size_type pos = 0;
				while ( (pos = str.find(searchString, pos)) != string::npos ) {
					str.replace( pos, searchString.size(), replaceString );
					pos++;
				}
			}				
		}
		for (iterDeclare = declareTable.begin(); iterDeclare!=declareTable.end();++iterDeclare){
			name = iterDeclare->first;
			useCycle = iterDeclare->second;
			
			ostringstream tSearch;
			tSearch << "__"<<name<<"__"<<useCycle<<"__"; 
			//			cout << "searching for: " << tSearch.str() << endl;
			string searchString (tSearch.str());
			
			ostringstream tReplace;
			tReplace << name;
			string replaceString(tReplace.str());
			
			if ( searchString != replaceString ){
				
				string::size_type pos = 0;
				while ( (pos = str.find(searchString, pos)) != string::npos ) {
					str.replace( pos, searchString.size(), replaceString );
					pos++;
				}
			}				
		}
		vhdl.setSecondLevelCode(str);
		REPORT(DEBUG, "   ... done second-level parsing for operator "<<srcFileName);
	}
	


	void  Operator::setIndirectOperator(Operator* op){
		indirectOperator_=op;
		if(op!=NULL) 	{		
			op->setuid(getuid()); //the selected implemetation becomes this operator 
			
			// TODO outDelayMap["R"] = op->getOutputDelay("R"); //populate output delays
			setCycle(op->getPipelineDepth());
			op->setName (getName() );//accordingly set the name of the implementation
				
			signalList_ = op->signalList_;
			subComponents_ = op->subComponents_;
			ioList_ = op->ioList_;
		 }
	}

	void Operator::cleanup(vector<Operator*> *ol, Operator* op){
		//iterate through all the components of op
		map<string, Operator*>::iterator it;
	
		for (it = op->subComponents_.begin(); it!= op->subComponents_.end(); it++)
			cleanup(ol, it->second);
			
		for (unsigned j=0; j< (*ol).size(); j++){
			if ((*ol)[j]->myuid == op->myuid){
				(*ol).erase((*ol).begin()+j);
			}
		}	
	}

	string Operator::signExtend(string name, int w){
		ostringstream e;
		e << srcFileName << " (" << uniqueName_ << "): ERROR in signExtend, "; // just in case

		Signal* s;
		try {
			s=getSignalByName(name);
		}
		catch (string e2) {
			e << endl << tab << e2;
			throw e.str();
		}

		//get the signals's width 
		if (w == s->width()){
			//nothing to do
			return name;
		}else if (w < s->width()){
			cout << "WARNING: you required a sign extension to "<<w<<" bits of signal " << name << " whose width is " << s->width() << endl;
			return name;
		}else{
			ostringstream n;
			n << "(";
			for (int i=0; i< w - s->width(); i++){
				n<< name << of ( s->width() -1 ) << " & ";
			}
			n << name << ")";
			string r = n.str();
			return r;
		}
	}

	string Operator::zeroExtend(string name, int w){
		ostringstream e;
		e << srcFileName << " (" << uniqueName_ << "): ERROR in zeroExtend, "; // just in case

		Signal* s;
		try {
			s=getSignalByName(name);
		}
		catch (string e2) {
			e << endl << tab << e2;
			throw e.str();
		}

		//get the signals's width 
		if (w == s->width()){
			//nothing to do
			return name;
		}else if (w < s->width()){
			cout << "WARNING: you required a zero extension to "<<w<<" bits of signal " << name << " whose width is " << s->width() << endl;
			return name;
		}else{
			ostringstream n;
			n << "(" << zg(w-s->width())<<" &" <<name << ")";
			string r = n.str();
			return r;
		}
	}
	
	void Operator::emulate(TestCase * tc) {
		throw std::string("emulate() not implemented for ") + uniqueName_;
	}
	
	bool Operator::hasComponent(string s){
		map<string, Operator*>::iterator theIterator;
		
		theIterator = subComponents_.find(s);
		if (theIterator != subComponents_.end() )
			return true;
		else
			return false;
	}

	void Operator::addComment(string comment, string align){
		vhdl << align << "-- " << comment << endl;
	}

	void Operator::addFullComment(string comment, int lineLength) {
		string align = "--";
		// - 2 for the two spaces
		for (unsigned i = 2; i < (lineLength - 2- comment.size()) / 2; i++) align += "-";
		vhdl << align << " " << comment << " " << align << endl; 
	}




	void Operator::outputVHDLToFile(vector<Operator*> &oplist, ofstream& file){
		string srcFileName = "Operator.cpp"; // for REPORT
		for(unsigned i=0; i<oplist.size(); i++) {
			try {
				REPORT(FULL, "---------------OPERATOR: "<<oplist[i]->getName() <<"-------------");
				REPORT(FULL, "  DECLARE LIST" << printMapContent(oplist[i]->getDeclareTable()));
				REPORT(FULL, "  USE LIST" << printVectorContent(  (oplist[i]->getFlopocoVHDLStream())->getUseTable()) );
				
				// check for subcomponents 
				if (! oplist[i]->getOpListR().empty() ){
					//recursively call to print subcomponent
					outputVHDLToFile(oplist[i]->getOpListR(), file);	
				}
				oplist[i]->getFlopocoVHDLStream()->flush();

				/* second parse is only for sequential operators */
				if (oplist[i]->isSequential()){
					REPORT (FULL, "  2nd PASS");
					oplist[i]->parse2();
				}
				oplist[i]->outputVHDL(file);			
			
			} catch (std::string s) {
					cerr << "Exception while generating '" << oplist[i]->getName() << "': " << s <<endl;
			}
		}
	}




#if 1
	void Operator::outputVHDLToFile(ofstream& file){
		vector<Operator*> oplist;

		REPORT(DEBUG, "outputVHDLToFile");

		//build a copy of the global oplist hidden in Target (if it exists):
		vector<Operator*> *goplist = target_->getGlobalOpListRef();
		for (unsigned i=0; i<goplist->size(); i++)
			oplist.push_back((*goplist)[i]);
		// add self (and all its subcomponents) to this list
		oplist.push_back(this);
		// generate the code
		Operator::outputVHDLToFile(oplist, file);
	}
	
#endif
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Functions used for resource estimations

	
	//--Logging functions
	
	std::string Operator::addFF(int count){
		
		reActive = true;
		return reHelper->addFF(count);
	}
	
	std::string Operator::addLUT(int nrInputs, int count){
		
		reActive = true;
		return reHelper->addLUT(nrInputs, count);
	}
	
	std::string Operator::addReg(int width, int count){
				
		reActive = true;
		return reHelper->addReg(width, count);
	}
	
	//TODO: verify increase in the DSP count
	std::string Operator::addMultiplier(int count){
		
		reActive = true;
		return reHelper->addMultiplier(count);
	}
	
	//TODO: verify increase in the DSP count 
	std::string Operator::addMultiplier(int widthX, int widthY, double ratio, int count){
		
		reActive = true;
		return reHelper->addMultiplier(widthX, widthY, ratio, count);
	}
	
	//TODO: verify increase in the element count
	std::string Operator::addAdderSubtracter(int widthX, int widthY, double ratio, int count){
		
		reActive = true;
		return reHelper->addAdderSubtracter(widthX, widthY, ratio, count);
	}
	
	//TODO: take into account the memory type (RAM or ROM); depending on 
	//		the type, might be implemented through distributed memory or
	//		dedicated memory blocks
	std::string Operator::addMemory(int size, int width, int type, int count){
		
		reActive = true;
		return reHelper->addMemory(size, width, type, count);
	}
	
	//---More particular resource logging
	std::string Operator::addDSP(int count){
		
		reActive = true;
		return reHelper->addDSP(count);
	}
	
	std::string Operator::addRAM(int count){
		
		reActive = true;
		return reHelper->addRAM(count);
	}
	
	std::string Operator::addROM(int count){
		
		reActive = true;
		return reHelper->addROM(count);
	}
	
	//TODO: should count the shift registers according to their bitwidths
	std::string Operator::addSRL(int width, int depth, int count){
				
		reActive = true;
		return reHelper->addSRL(width, depth, count);
	}
	
	std::string Operator::addWire(int count, std::string signalName){
		
		reActive = true;
		return reHelper->addWire(count, signalName);
	}
	
	std::string Operator::addIOB(int count, std::string portName){
		
		reActive = true;
		return reHelper->addIOB(count, portName);
	}
	
	//---Even more particular resource logging-------------------------
	
	//TODO: get a more accurate count of the number of multiplexers 
	//		needed; currently specific resources are not taken into account
	std::string Operator::addMux(int width, int nrInputs, int count){
				
		reActive = true;
		return reHelper->addMux(width, nrInputs, count);
	}
	
	//TODO: count the counters according to their bitwidth
	//TODO: get estimations when using specific resources (like DSPs)
	//		involves also changes to getLUTPerCounter() getFFPerCounter()
	std::string Operator::addCounter(int width, int count){
				
		reActive = true;
		return reHelper->addCounter(width, count);
	}
	
	//TODO: count the accumulators according to their bitwidth
	std::string Operator::addAccumulator(int width, bool useDSP, int count){
		
		reActive = true;
		return reHelper->addAccumulator(width, useDSP, count);
	}
	
	//TODO: count the decoders according to their input and output 
	//		bitwidths
	std::string Operator::addDecoder(int wIn, int wOut, int count){
		
		reActive = true;
		return reHelper->addDecoder(wIn, wOut, count);
	}
	
	std::string Operator::addArithOp(int width, int nrInputs, int count){
		
		reActive = true;
		return reHelper->addArithOp(width, nrInputs, count);
	}
	
	//TODO: find a better approximation for the resources
	//		currently just logic corresponding to the multiplexers
	//TODO: find a better approximation for the resources
	//		currently just logic corresponding to the state register
	//TODO: find a better approximation for the resources
	//		for now, RAM blocks are not used
	std::string Operator::addFSM(int nrStates, int nrTransitions, int count){
		
		reActive = true;
		return reHelper->addFSM(nrStates, nrTransitions, count);
	}
	
	//--Resource usage statistics---------------------------------------
	std::string Operator::generateStatistics(int detailLevel){
		
		reActive = true;
		return reHelper->generateStatistics(detailLevel);
	}
	
	//--Utility functions related to the generation of resource usage statistics
	
	//TODO: find a more precise way to determine the required number of
	//		registers due to pipeline
	std::string Operator::addPipelineFF(){
		
		reActive = true;
		return reHelper->addPipelineFF();
	}
	
	std::string Operator::addWireCount(){
		
		reActive = true;
		return reHelper->addWireCount();
	}
	
	std::string Operator::addPortCount(){
		
		reActive = true;
		return reHelper->addPortCount();
	}
	
	//TODO: add function to add resource count from specified component
	std::string Operator::addComponentResourceCount(){
		
		reActive = true;
		return reHelper->addComponentResourceCount();
	}
	
	void Operator::addAutomaticResourceEstimations(){
				
		reActive = true;
		resourceEstimate << reHelper->addAutomaticResourceEstimations();
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Functions used for floorplanning
	
	std::string Operator::manageFloorplan(){
		
		return flpHelper->manageFloorplan();
	}
	
	std::string Operator::addPlacementConstraint(std::string source, std::string sink, int type){
		
		return flpHelper->addPlacementConstraint(source, sink, type);
	}
	
	std::string Operator::addConnectivityConstraint(std::string source, std::string sink, int nrWires){
		
		return flpHelper->addConnectivityConstraint(source, sink, nrWires);
	}
	
	std::string Operator::addAspectConstraint(std::string source, double ratio){
		
		return flpHelper->addAspectConstraint(source, ratio);
	}
	
	std::string Operator::addContentConstraint(std::string source, int value, int length){
		
		return flpHelper->addContentConstraint(source, value, length);
	}
	
	std::string Operator::processConstraints(){
				
		return flpHelper->processConstraints();
	}
	
	std::string Operator::createVirtualGrid(){
				
		return flpHelper->createVirtualGrid();
	}
	
	std::string Operator::createPlacementGrid(){
				
		return flpHelper->createPlacementGrid();
	}
	
	std::string Operator::createConstraintsFile(){
		
		return flpHelper->createConstraintsFile();
	}
	
	std::string Operator::createPlacementForComponent(std::string moduleName){
				
		return flpHelper->createPlacementForComponent(moduleName);
	}
	
	std::string Operator::createFloorplan(){
				
		return flpHelper->createFloorplan();
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////

	/** Completely replace "this" with a copy of another operator. */
	void  Operator::cloneOperator(Operator *op){
		stdLibType_ = op->stdLibType_;
		subComponents_ = op->getSubComponents();
		signalList_ = op->getSignalList();	
		ioList_     = op->getIOListV();
		target_           = op->getTarget();
		uniqueName_       = op->getUniqueName();
		architectureName_ = op->getArchitectureName();
		testCaseSignals_ = op->getTestCaseSignals();	
		portMap_ = op->getPortMap();
		outDelayMap = map<string,double>(op->getOutDelayMap());
		inputDelayMap = op->getInputDelayMap();
		vhdl.vhdlCodeBuffer << op->vhdl.vhdlCodeBuffer.str();
		vhdl.vhdlCode       << op->vhdl.vhdlCode.str();
		vhdl.currentCycle_   = op->vhdl.currentCycle_;	
		vhdl.useTable        = op->vhdl.useTable;
		srcFileName = op->getSrcFileName();
		declareTable = op->getDeclareTable();
		cost = op->getOperatorCost();
		numberOfInputs_  = op->getNumberOfInputs();
		numberOfOutputs_ = op->getNumberOfOutputs();
		isSequential_    = op->isSequential();
		pipelineDepth_   = op->getPipelineDepth();
		signalMap_ = op->getSignalMap();
		constants_ = op->getConstants();
		attributes_ = op->getAttributes();
		types_ = op->getTypes();
		attributesValues_ = op->getAttributesValues();

		hasRegistersWithoutReset_   = op->getHasRegistersWithoutReset();
		hasRegistersWithAsyncReset_ = op->getHasRegistersWithAsyncReset();
		hasRegistersWithSyncReset_  = op->getHasRegistersWithSyncReset();
		hasClockEnable_             = op->hasClockEnable();
		copyrightString_            = op->getCopyrightString();
		currentCycle_               = op->getCurrentCycle();
		criticalPath_               = op->getCriticalPath();
		needRecirculationSignal_    = op->getNeedRecirculationSignal();
		indirectOperator_           = op->getIndirectOperator();
		hasDelay1Feedbacks_         = op->hasDelay1Feedbacks();

		oplist                      = op->getOpList();
	}
	
	
}


