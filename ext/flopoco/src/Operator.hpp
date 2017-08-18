#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <map>
#include <gmpxx.h>
#include "Target.hpp"
#include "Signal.hpp"
#include "TestCase.hpp"
#include <float.h>
#include <utility>
#include <vector>
#include "FlopocoStream.hpp"
#include "utils.hpp"
#include "Tools/ResourceEstimationHelper.hpp"
#include "Tools/FloorplanningHelper.hpp"

using namespace std;

// variables set by the command-line interface in main.cpp


namespace flopoco {

	// global const variables
	static const map<string, double> emptyDelayMap;
	const std::string tab = "   ";
	
	// Reporting levels
#define LIST 0       // information necessary to the user of FloPoCo
#define INFO 1       // information useful to the user of FloPoCo
#define DETAILED 2   // information that shows how the algorithm works
#define DEBUG 3      // debug info, useful mostly to developers
#define FULL 4       // pure noise



#define INNER_SEPARATOR "................................................................................"
#define DEBUG_SEPARATOR "________________________________________________________________________________"
#define OUTER_SEPARATOR "################################################################################"
#define REPORT(level, stream) {if ((level)<=(verbose)){ cerr << "> " << srcFileName << ": " << stream << endl;}else{}} 
#define THROWERROR(stream) {{ostringstream o; o << " ERROR in " << uniqueName_ << " (" << srcFileName << "): " << stream << endl; throw o.str();}} 


//Floorplanning - direction of placement constraints
#define ABOVE						0
#define UNDER						1
#define TO_LEFT_OF					2
#define TO_RIGHT_OF					3

#define ABOVE_WITH_EXTRA				4
#define UNDER_WITH_EXTRA				5
#define TO_LEFT_OF_WITH_EXTRA				6
#define TO_RIGHT_OF_WITH_EXTRA				7

//Floorplanning - constraint type
#define PLACEMENT 					0
#define CONNECTIVITY					1


/**
 * This is a top-level class representing an Operator.
 * This class is inherited by all classes which will output a VHDL entity.
 */
class Operator
{

	static int uid;                  /**< The counter holding a unique id */

public:

	/** add a sub-operator to this operator */
	void addSubComponent(Operator* op);


	/** add this operator to the global (first-level) list, which is stored in its Target (not really its place, sorry).
	This method should be called by 
	1/ the main / top-level, or  
	2/ for sub-components that are really basic operators, 
	expected to be used several times, *in a way that is independent of the context/timing*.
	Typical example is a table designed to fit in a LUT or parallel row of LUTs
 */
	void addToGlobalOpList();


	/** generates the code for a list of operators and all their subcomponents */
	static void outputVHDLToFile(vector<Operator*> &oplist, ofstream& file);


#if 1
	/** generates the code for this operator and all its subcomponents */
	void outputVHDLToFile(ofstream& file);
#endif

	/** Operator Constructor.
	 * Creates an operator instance with an instantiated target for deployment.
	 * @param target_ The deployment target of the operator.
	 */
	Operator(Target* target, map<string, double> inputDelays = emptyDelayMap);
		

	/** Operator Destructor.
	 */	
	virtual ~Operator() {}


 /*****************************************************************************/
 /*         Paperwork-related methods (for defining an operator entity)       */
 /*****************************************************************************/

	/** Adds an input signal to the operator.
	 * Adds a signal of type Signal::in to the the I/O signal list.
	 * @param name  the name of the signal
	 * @param width the number of bits of the signal.
	 * @param isBus describes if this signal is a bus, that is, an instance of std_logic_vector
	 */
	void addInput  (const std::string name, const int width=1, const bool isBus=true);
	
	/** Adds an input wire (of type std_logic) to the operator.
	 * Adds a signal of type Signal::in to the the I/O signal list.
	 * @param name  the name of the signal
	 */
	void addInput  (const std::string name) {
		addInput (name, 1, false);
	}

	void addInput (const char* name) {
		addInput (name, 1, false);
	}

	/** Adds an output signal to the operator.
	 * Adds a signal of type Signal::out to the the I/O signal list.
	 * @param name  the name of the signal
	 * @param width the number of bits of the signal.
	 * @param numberOfPossibleOutputValues (optional, defaults to 1) set to 2 for a faithfully rounded operator for instance
	 * @param isBus describes if this signal is a bus, that is, an instance of std_logic_vector
	 */	
	void addOutput(const std::string name, const int width=1, const int numberOfPossibleOutputValues=1, const bool isBus=true);
	
	/** Adds an output wire (of type std_logic) with one possible value
	 ** to the operator.
	 * Adds a signal of type Signal::out to the the I/O signal list.
	 * @param name  the name of the signal
	 */	
	void addOutput(const std::string name) {
		addOutput (name, 1, 1, false);
	}

	void addOutput(const char* name) {
		addOutput (name, 1, 1, false);
	}

	/** Adds a floating point (FloPoCo format) input signal to the operator.
	 * Adds a signal of type Signal::in to the the I/O signal list, 
	 * having the FP flag set on true. The total width of this signal will
	 * be wE + wF + 3. (2 bits for exception, 1 for sign)
	 * @param name the name of the signal
	 * @param wE   the width of the exponent
	 * @param wF   the withh of the fraction
	 */
	void addFPInput(const std::string name, const int wE, const int wF);


	/** Adds a floating point (FloPoCo format) output signal to the operator.
	 * Adds a signal of type Signal::out to the the I/O signal list, 
	 * having the FP flag set on true. The total width of this signal will
	 * be wE + wF + 3. (2 bits for exception, 1 for sign)
	 * @param name the name of the signal
	 * @param wE   the width of the exponent
	 * @param wF   the withh of the fraction
	 * @param numberOfPossibleOutputValues (optional, defaults to 1) set to 2 for a faithfully rounded operator for instance
	 */	
	void addFPOutput(const std::string name, const int wE, const int wF, const int numberOfPossibleOutputValues=1);

	/** Adds a IEEE floating point input signal to the operator.
	 * Adds a signal of type Signal::in to the the I/O signal list, 
	 * having the FP flag set on true. The total width of this signal will
	 * be wE + wF + 1.  (1 bit for sign)
	 * @param name the name of the signal
	 * @param wE   the width of the exponent
	 * @param wF   the withh of the fraction
	 */
	void addIEEEInput(const std::string name, const int wE, const int wF);


	/** Adds a floating point output signal to the operator.
	 * Adds a signal of type Signal::out to the the I/O signal list, 
	 * having the FP flag set on true. The total width of this signal will
	 * be wE + wF + 1. (1 bit for sign)
	 * @param name the name of the signal
	 * @param wE   the width of the exponent
	 * @param wF   the withh of the fraction
	 * @param numberOfPossibleOutputValues (optional, defaults to 1) set to 2 for a faithfully rounded operator for instance
	 */	
	void addIEEEOutput(const std::string name, const int wE, const int wF, const int numberOfPossibleOutputValues=1);

	

	/** sets the copyright string, should be authors + year
	 * @param authorsYears the names of the authors and the years of their contributions
	 */
	void setCopyrightString(std::string authorsYears);


	/** use the Synopsys de-facto standard ieee.std_logic_unsigned for this entity
	 */
	void useStdLogicUnsigned() {
		stdLibType_ = 0;
	};

	/** use the Synopsys de-facto standard ieee.std_logic_unsigned for this entity
	 */
	void useStdLogicSigned() {
		stdLibType_ = -1;
	};


	/** use the real IEEE standard ieee.numeric_std for this entity
	 */
	void useNumericStd() {
		stdLibType_ = 1;
	};
	
	/** 
	 * use the real IEEE standard ieee.numeric_std for this entity, also 
	 * with support for signed operations on bit vectors
	 */
	void useNumericStd_Signed() {
		stdLibType_ = 2;
	};
	
	/** 
	 * use the real IEEE standard ieee.numeric_std for this entity, also 
	 * with support for unsigned operations on bit vectors
	 */
	void useNumericStd_Unsigned() {
		stdLibType_ = 3;
	};

	int getStdLibType() {
		return stdLibType_; 
	};

	/** Sets Operator name to given name, with either the frequency appended, or "comb" for combinatorial.
	 * @param operatorName new name of the operator
	*/
	void setNameWithFreq(std::string operatorName = "UnknownOperator");

	/** Sets Operator name to given name.
	 * @param operatorName new name of the operator
	*/
	void setName(std::string operatorName = "UnknownOperator");
	
	/** This method should be used by an operator to change the default name of a sub-component. The default name becomes the commented name.
	 * @param operatorName new name of the operator
	*/
	void changeName(std::string operatorName);
	
	/** Sets Operator name to prefix_(uniqueName_)_postfix
	 * @param prefix the prefix string which will be placed in front of the operator name
	 *               formed with the operator internal parameters
	 * @param postfix the postfix string which will be placed at the end of the operator name
	 *                formed with the operator internal parameters
	*/
	void setName(std::string prefix, std::string postfix);
		
	/** Return the operator name. 
	 * Returns a string value representing the name of the operator. 
	 * @return operator name
	 */
	string getName() const;

	/** produces a new unique identifier */
	static int getNewUId(){
		Operator::uid++;
		return Operator::uid;
	}




 /*****************************************************************************/
 /*        VHDL-related methods (for defining an operator architecture)       */
 /*****************************************************************************/


	/* Functions related to pipeline management */
	// TODO We should introduce a notion of pipetime, which is (cycle, critical path) in lexicographic order.

	/** Define the current cycle, and resets the critical path 
	 * @param the new value of the current cycle */
	void setCycle(int cycle, bool report=true) ;

	/** Return the current cycle 
	 * @return the current cycle */
	int getCurrentCycle(); 

	/** advance the current cycle by 1, and resets the critical path 
	 * @param the new value of the current cycle */
	void nextCycle(bool report=true) ;

	/** Define the current cycle, and reset the critical path
	 * @param the new value of the current cycle */
	void previousCycle(bool report=true) ;

	/** get the critical path of the current cycle so far */
	double getCriticalPath() ;

	/** Set or reset the critical path of the current cycle  */
	void setCriticalPath(double delay) ;

	/** Adds to the critical path of the current stage, and insert a pipeline stage if needed
	 * @param the delay to add to the critical path of current pipeline stage */
	void addToCriticalPath(double delay) ;

	bool manageCriticalPath(double delay=0.0, bool report=true);


	/** get the critical path delay associated to a given output of the operator
	 * @param the name of the output */
	double getOutputDelay(string s); 

	/** Set the current cycle to that of a signal and reset the critical path. It may increase or decrease current cycle. 
	 * @param name is the signal name. It must have been defined before 
	 * @param report is a boolean, if true it will report the cycle 
	 */
	void setCycleFromSignal(string name, bool report=true) ;

	/** Set the current cycle and the critical path. It may increase or decrease current cycle. 
	 * @param name is the signal name. It must have been defined before. 
	 * @param criticalPath is the critical path delay associated to this signal: typically getDelay(name)
	 * @param report is a boolean, if true it will report the cycle 
	 */
	void setCycleFromSignal(string name, double criticalPath, bool report=true) ;
	// TODO: FIXME  
	// param criticalPath is the critical path delay associated to this signal: typically getDelay(name)
	// Shouldn't this be the default behaviour?
	// Check current use and fix.



	int getCycleFromSignal(string name, bool report = false);
		
	
	/** advance the current cycle to that of a signal. It may only increase current cycle. To synchronize
		 two or more signals, first call setCycleFromSignal() on the
		 first, then syncCycleFromSignal() on the remaining ones. It
		 will synchronize to the latest of these signals.  
		 * @param name is the signal name. It must have been defined before 
		 * @param report is a boolean, if true it will report the cycle 
		 */
	bool syncCycleFromSignal(string name, bool report=true) ;

	/** advance the current cycle to that of a signal, updating critical paths.
		 * @param name is the signal name. It must have been defined before 
		 * @param criticalPath is a double, the critical path already consumed up to the signal passed as first arg. 

		 We have three cases:
		 1/ currentCycle > name.cycle, then do nothing
		 2/ currentCycle < name.cycle, then advance currentCycle to name.cycle, and set the current critical path to criticalPath
		 3/ currentCycle = name.cycle: set critical path to the max of the two critical paths.
		 */
	bool syncCycleFromSignal(string name, double criticalPath, bool report=true) ;


	/** sets the delay of the signal with name given by first argument 
		@param name the name of the signal
		@param delay the delay to be associated with the name	
	**/
	void setSignalDelay(string name, double delay);

	/** returns the delay on the signal with the name denoted by the argument 
		@param name signal Name
		@return delay of this signal
	*/
	double getSignalDelay(string name);


	/** Declares a signal implicitely by having it appearing on the Left Hand Side of a VHDL assignment
	 * @param name is the name of the signal
	 * @param width is the width of the signal (optional, default 1)
	 * @param isbus: a signal of width 1 is declared as std_logic when false, as std_logic_vector when true (optional, default false)
	 * @param regType: the registring type of this signal. See also the Signal Class for more info
	 * @return name
	 */
	string declare(string name, const int width, bool isbus=true, Signal::SignalType regType = Signal::wire );

	/** Declares a signal of length 1 as in the previous declare() function, but as std_logic by default
	 * @param name is the name of the signal
	 * @param isbus: if true, declares the signal as std_logic_vector; else declares the signal as std_logic
	 * @param regType: the registring type of this signal. See also the Signal Class for mor info
	 * @return name
	 */
	string declare(string name, Signal::SignalType regType = Signal::wire ) {
		return declare(name, 1, false, regType);
	}

	// TODO: add methods that allow for signals with reset (when rewriting LongAcc for the new framework)


#if 1
	/** use a signal on the Right 
	 * @param name is the name of the signal
	 * @return name
	 */
	string use(string name);

	string use(string name, int delay);
#endif

	/** Declare an output mapping for an instance of a sub-component
	 * Also declares the local signal implicitely, with width taken from the component 	
	 * @param op is a pointer to the subcomponent
	 * @param componentPortName is the name of the port on the component
	 * @param actualSignalName is the name of the signal in This mapped to this port
	 * @param newSignal_ (by default true), defined wheter or not actualSignalName has to be declared as a new signal by outPortMap
	 * @return name
	 */
	void outPortMap(Operator* op, string componentPortName, string actualSignalName, bool newSignal = true);


	/** use a signal as input of a subcomponent
	 * @param componentPortName is the name of the port on the component
	 * @param actualSignalName is the name of the signal (of this) mapped to this port
	 */
	void inPortMap(Operator* op, string componentPortName, string actualSignalName);

	/** use a constant signal as input of a subcomponent. 
	 * @param componentPortName is the name of the port on the component
	 * @param actualSignal is the constant signal to be mapped to this port
	 */
	void inPortMapCst(Operator* op, string componentPortName, string actualSignal);

	/** returns the VHDL for an instance of a sub-component. 
	 * @param op represents the operator to be port mapped 
	 * @param instanceName is the name of the instance as a label
	 * @return name
	 */
	string instance(Operator* op, string instanceName);




	/** adds attributes to the generated VHDL so that the tools use embedded RAM blocks for an instance
	 * @ param t a pointer to this instance
	 */
	void useHardRAM(Operator* t); 
	void useSoftRAM(Operator* t); 

	/** define architecture name for this operator (by default : arch)
	 *	@param[in] 	architectureName		- new name for the operator architecture
	 **/
	void setArchitectureName(string architectureName) {
		architectureName_ = architectureName;
	};	


	/**
	 * A new architecture inline function
	 * @param[in,out] o 	- the stream to which the new architecture line will be added
	 * @param[in]     name	- the name of the entity corresponding to this architecture
	 **/
	inline void newArchitecture(std::ostream& o, std::string name){
		o << "architecture " << architectureName_ << " of " << name  << " is" << endl;
	}
	
	/**
	 * A begin architecture inline function 
	 * @param[in,out] o 	- the stream to which the begin line will be added
	 **/
	inline void beginArchitecture(std::ostream& o){
		o << "begin" << endl;
	}

	/**
	 * A end architecture inline function 
	 * @param[in,out] o 	- the stream to which the begin line will be added
	 **/
	inline void endArchitecture(std::ostream& o){
		o << "end architecture;" << endl << endl;
	}





 /*****************************************************************************/
 /*        Testing-related methods (for defining an operator testbench)       */
 /*****************************************************************************/

	/**
	 * Gets the correct value associated to one or more inputs.
	 * @param tc the test case, filled with the input values, to be filled with the output values.
	 * @see FPAdder for an example implementation
	 */
	virtual void emulate(TestCase * tc);
		
	/**
	 * Append standard test cases to a test case list. Standard test
	 * cases are operator-dependent and should include any specific
	 * corner cases you may think of. Never mind removing a standard test case because you think it is no longer useful!
	 * @param tcl a TestCaseList
	 */
	virtual void buildStandardTestCases(TestCaseList* tcl);


	/**
	 * Generate Random Test case identified by an integer . There is a default
	 * implementation using a uniform random generator, but most
	 * operators are not exercised efficiently using such a
	 * generator. For instance, in FPAdder, the random number generator
	 * should be biased to favor exponents which are relatively close
	 * so that an effective addition takes place.
	 * This function create a new TestCase (to be free after use)
	 * See FPExp.cpp for an example of overloading this method.
	 * @param i the identifier of the test case to be generated
	 * @return TestCase*
	 */
	virtual TestCase* buildRandomTestCase(int i);





 /*****************************************************************************/
 /*     From this point, we have methods that are not needed in normal use    */
 /*****************************************************************************/




	/**
	 * Append random test cases to a test case list. There is a default
	 * implementation using a uniform random generator, but most
	 * operators are not exercised efficiently using such a
	 * generator. For instance, in FPAdder, the random number generator
	 * should be biased to favor exponents which are relatively close
	 * so that an effective addition takes place.
	 * In most cases you do need to overload this method, 
	 * but simply overload  buildRandomTestCase(int i) 
	 * which is called by the default implementation of buildRandomTestCaseList
	 * @param tcl a TestCaseList
	 * @param n the number of random test cases to add
	 */
	virtual void buildRandomTestCaseList(TestCaseList* tcl, int n);



	

	/** build all the signal declarations from signals implicitely declared by declare().
	 */
	string buildVHDLSignalDeclarations();

	/** build all the component declarations from the list built by instance().
	 */
	string buildVHDLComponentDeclarations();

	/** build all the registers from signals implicitely delayed by declare() 
	 *	 This is the 2.0 equivalent of outputVHDLSignalRegisters
	 */
	string buildVHDLRegisters();

	/** build all the type declarations.
	 */
	string buildVHDLTypeDeclarations();

	/** output the VHDL constants. */
	string buildVHDLConstantDeclarations();

	/** output the VHDL constants. */
	string buildVHDLAttributes();





	/** the main function outputs the VHDL for the operator.
		 If you use the modern (post-0.10) framework you no longer need to overload this method,
		 the default will do.
	 * @param o the stream where the entity will outputted
	 * @param name the name of the architecture
	 */
	virtual void outputVHDL(std::ostream& o, std::string name);
	
	/** the main function outputs the VHDL for the operator 
	 * @param o the stream where the entity will outputted
	 */	
	void outputVHDL(std::ostream& o);   // calls the previous with name = uniqueName





	/** True if the operator needs a clock signal; 
	 * It will also get a rst but doesn't need to use it.
	 */	
	bool isSequential();  


        /** True if the operator need a recirculation signal 
         *  TODO : change name
         */
        bool isRecirculatory();
	
	/** Set the operator to sequential.
		 You shouldn't need to use this method for standard operators 
		 (Operator::Operator()  calls it according to Target)
	 */	
	void setSequential(); 
	
	/** Set the operator to combinatorial
		 You shouldn't need to use this method for standard operators 
		 (Operator::Operator()  calls it according to Target)
	 */	
	void setCombinatorial();



        /** Set the operator to need a recirculation signal in order to 
                  trigger the pipeline work
         */
        void setRecirculationSignal();
	
	/** Indicates that it is not a warning if there is feedback of one cycle, but it
		is an error if a feedback of more than one cycle happens.
		*/
	void setHasDelay1Feedbacks()
	{
		hasDelay1Feedbacks_=true;
	}


	bool hasDelay1Feedbacks(){
		return hasDelay1Feedbacks_;
	}
	



	
	

	/** Returns a pointer to the signal having the name s. Throws an exception if the signal is not yet declared.
	  * @param s then name of the signal we want to return
	  * @return the pointer to the signal having name s 
	  */
	Signal* getSignalByName(string s);

	vector<Signal*> getSignalList(){
		return signalList_;
	};

	bool isSignalDeclared(string name);
		




	/** DEPRECATED Outputs component declaration 
	 * @param o the stream where the component is outputed
	 * @param name the name of the VHDL component we want to output to o
	 */
	virtual void outputVHDLComponent(std::ostream& o, std::string name);
	
	/**  DEPRECATED Outputs the VHDL component code of the current operator
	 * @param o the stream where the component is outputed
	 */
	void outputVHDLComponent(std::ostream& o);  
		
		

	/** Return the number of input+output signals 
	 * @return the size of the IO list. The total number of input and output signals
	 *         of the architecture.
	 */
	int getIOListSize() const;
	
	/** Returns a pointer to the list containing the IO signals.
	 * @return pointer to ioList 
	 */
	vector<Signal*> * getIOList();

	/** passes the IOList by value.
	 * @return the ioList 
	 */
	vector<Signal*> getIOListV(){
		return ioList_;
	}

	
	/** Returns a pointer a signal from the ioList.
	 * @param the index of the signal in the list
	 * @return pointer to the i'th signal of ioList 
	 */
	Signal * getIOListSignal(int i);
		
	



	/** DEPRECATED, better use setCopyrightString
		 Output the licence
	 * @param o the stream where the licence is going to be outputted
	 * @param authorsYears the names of the authors and the years of their contributions
	 */
	void licence(std::ostream& o, std::string authorsYears);


	/**  Output the licence, using copyrightString_
	 * @param o the stream where the licence is going to be outputted
	 */
	void licence(std::ostream& o);


	void pipelineInfo(std::ostream& o, std::string authorsYears);


	void pipelineInfo(std::ostream& o);

	/** Output the standard library paperwork 
	 * @param o the stream where the libraries will be written to
	 */
	void stdLibs(std::ostream& o);

		
	/** DEPRECATED  Output the VHDL entity of the current operator.
	 * @param o the stream where the entity will be outputted
	 */
	void outputVHDLEntity(std::ostream& o);
	
	/** DEPRECATED  output all the signal declarations 
	 * @param o the stream where the signal deca
	 */
	void outputVHDLSignalDeclarations(std::ostream& o);


	/** Add a VHDL type declaration. */
 	void addType(std::string name, std::string def);

	/** Add a VHDL constant. This may make the code easier to read, but more difficult to debug. */
	void addConstant(std::string name, std::string ctype, int cvalue);

	void addConstant(std::string name, std::string ctype, mpz_class cvalue);
	
	void addConstant(std::string name, std::string ctype, string cvalue);
	

	/** Add attribute, declaring the attribute name if it is not done already.
	 */ 
	void addAttribute(std::string attributeName,  std::string attributeType,  std::string object, std::string value );

	/**
	 * A new line inline function
	 * @param[in,out] o the stream to which the new line will be added
	 **/
	inline void newLine(std::ostream& o) {	o<<endl; }



	/** Final report function, prints to the terminal.  By default
	 * reports the pipeline depth, but feel free to overload if you have any
	 * thing useful to tell to the end user
	*/
	virtual void outputFinalReport(int level);	
	
	
	/** Gets the pipeline depth of this operator 
	 * @return the pipeline depth of the operator
	*/
	int getPipelineDepth();

	/**
	* @return the output map containing the signal -> delay associations 
	*/	
	map<string, double> getOutDelayMap();
	
	/**
	* @return the output map containing the signal -> declaration cycle 
	*/	
	map<string, int> getDeclareTable();

	Target* getTarget(){
		return target_;
	}

	string getUniqueName(){
		return uniqueName_;
	}

	string getArchitectureName(){
		return architectureName_;
	}
	
	vector<Signal*> getTestCaseSignals(){
		return testCaseSignals_;
	}
	
	map<string, string> getPortMap(){
		return portMap_;
	}
	
	
	map<string, double> getInputDelayMap(){
		return inputDelayMap;
	}
	
	map<string, Operator*> getSubComponents(){
		return subComponents_;
	}
	
	string getSrcFileName(){
		return srcFileName;
	}
	
	int getOperatorCost(){
		return cost;
	}

	int getNumberOfInputs(){
		return numberOfInputs_;
	}
	
	int getNumberOfOutputs(){
		return numberOfOutputs_;
	}
	
	map<string, Signal*> getSignalMap(){
		return signalMap_;
	}

	map<string, pair<string, string> > getConstants(){
		return constants_;
	}
	
	map<string, string> getAttributes(){
		return attributes_;
	}
	
	map<string, string> getTypes(){
		return types_;
	}
	
	map<pair<string,string>, string> getAttributesValues(){
		return attributesValues_;
	}

	bool getHasRegistersWithoutReset(){
		return hasRegistersWithoutReset_;
	}

	bool getHasRegistersWithAsyncReset(){
		return hasRegistersWithAsyncReset_;
	}

	bool getHasRegistersWithSyncReset(){
		return hasRegistersWithSyncReset_;
	}

	bool hasReset() {
		return hasRegistersWithSyncReset_ || hasRegistersWithAsyncReset_;
	}

	bool hasClockEnable(){
		return hasClockEnable_;
	}

	void setClockEnable(bool val){
		hasClockEnable_=val;
	}

	string getCopyrightString(){
		return copyrightString_;
	}

	bool getNeedRecirculationSignal(){
		return needRecirculationSignal_;
	}
	
	Operator* getIndirectOperator(){
		return indirectOperator_;
	}

	void setIndirectOperator(Operator* op);
	
	vector<Operator*> getOpList(){
		return oplist;
	}


	vector<Operator*>& getOpListR(){
		return oplist;
	}

	
	bool hasComponent(string s);
	
	void cleanup(vector<Operator*> *ol, Operator* op);
	
	FlopocoStream* getFlopocoVHDLStream(){
		return &vhdl;
	}

	void parse2();

	
	void setuid(int mm){
		myuid = mm;
	}
	
	int getuid(){
		return myuid;
	}


	// TODO this probably doesn't belong here.
	/** Extend the sign of a signal of this operator given by name */
	string signExtend(string name, int w);
	
	/** Extend a signal of this operator given by name, by left-padding with zeroes */
	string zeroExtend(string name, int w);

	int level; //printing issues




	/** add a comment line in to vhdl stream */
	void addComment(string comment, string align = tab);

	/** add a full line of '-' with comment centered within */
	void addFullComment(string comment, int lineLength = 80);


	/** Completely replace "this" with a copy of another operator. */
	void cloneOperator(Operator *op);	
	
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Functions used for resource estimations
	
	//--Logging functions
	
	/**
	 * Add @count flip-flops to the total estimate
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addFF(int count = 1);
	
	/**
	 * Add @count registers to increase the total flip-flop estimate 
	 * from the register characteristics
	 * @param count (by default 1) the number of registers to add
	 * @param width the width of each register
	 * @return the string describing the performed operation
	 */
	std::string addReg(int width, int count = 1);
	
	/**
	 * Add @count function generators to the total estimate
	 * Suggest Look-Up Table type (based on number of inputs), in order
	 * to obtain more accurate predictions
	 * @param count (by default 1) the number of elements to add
	 * @param nrInputs number of inputs of the LUT (0 for default option 
	 * of target technology)
	 * @return the string describing the performed operation
	 */
	std::string addLUT(int nrInputs = 0, int count = 1); 
	
	/**
	 * Add @count multipliers to the total estimate
	 * NOTE: also increases the DSP count
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addMultiplier(int count = 1);
	
	/**
	 * Add @count multipliers each having inputs of bitwidths @widthX and 
	 * @widthY, respectively
	 * The user can also chose to what degree the multipliers are 
	 * implemented in logic (a number between 0 and 1)
	 * NOTE: also increases the DSP count
	 * @param count (by default 1) the number of elements to add
	 * @param width the bitwidth of the multipliers
	 * @param ratio (by default 1) the ratio to which the multipliers 
	 * are implemented in logic (0 for 0%, 1 for 100%)
	 * @return the string describing the performed operation
	 */
	std::string addMultiplier(int widthX, int widthY, double ratio = 1, int count = 1);
	
	/**
	 * Add @count adders/subtracters each having inputs of bitwidths @widthX and 
	 * @widthY, respectively
	 * The user can also chose to what degree the adders/subtracters are 
	 * implemented in logic (a number between 0 and 1)
	 * NOTE: can also increase the DSP count
	 * @param count (by default 1) the number of elements to add
	 * @param width the bitwidth of the multipliers
	 * @param ratio (by default 0) the ratio to which the multipliers 
	 * are implemented in logic (0 for 0%, 1 for 100%)
	 * @return the string describing the performed operation
	 */
	std::string addAdderSubtracter(int widthX, int widthY, double ratio = 0, int count = 1);
	
	/**
	 * Add @count memories to the total estimate, each having @size 
	 * words of @width bits
	 * The memories can be either RAM or ROM, depending on the value of
	 * the @type parameter
	 * NOTE: Defaults to adding RAM memories
	 * @param count (by default 1) the number of elements to add
	 * @param size the number of words of the memory
	 * @param width the bitwidth of each of the memory's word
	 * @param type (by default 0) the type of the memory  
	 * (0 for RAM, 1 for ROM)
	 * @return the string describing the performed operation
	 */
	std::string addMemory(int size, int width, int type = 0, int count = 1);
	
	//---More particular resource logging
	/**
	 * Add @count DSP(s) to the total estimate
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addDSP(int count = 1);
	
	/**
	 * Add @count RAM(s) to the total estimate
	 * NOTE: For a more precise description of the memory being added, use the
	 * @addMemory() function with the corresponding parameters
	 * NOTE: adds memories with the default widths and sizes
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addRAM(int count = 1);
	
	/**
	 * Add @count ROM(s) to the total estimate
	 * NOTE: For a more precise description of the memory being added, use the
	 * @addMemory() function with the corresponding parameters
	 * NOTE: adds memories with the default widths and sizes
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addROM(int count = 1);
	
	/**
	 * Add @count Shift Registers to the total estimate, each having a
	 * bitwidth of @width bits
	 * NOTE: this function also modifies the total number of LUTs and FFs 
	 * in the design; this aspect should be considered so as not to result 
	 * in counting the resources multiple times and overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param width the bitwidth of the registers
	 * @param depth the depth of the shift register
	 * @return the string describing the performed operation
	 */
	std::string addSRL(int width, int depth, int count = 1);
	
	/**
	 * Add @count wire elements to the total estimate
	 * The estimation can be done in conjunction with the declaration of a 
	 * certain signal, in which specify the signal's name is specified 
	 * through the @signalName parameter
	 * NOTE: it is not advised to use the function without specifying 
	 * the signal's name, as it results in duplication of resource count
	 * NOTE: if @signalName is provided, @count can be omitted, as it 
	 * serves no purpose
	 * @param count (by default 1) the number of elements to add
	 * @param signalName (by default the empty string) the name of the
	 * corresponding signal
	 * @return the string describing the performed operation
	 */
	std::string addWire(int count = 1, std::string signalName = "");
	
	/**
	 * Add @count I/O ports to the total estimate
	 * The estimation can be done in conjunction with the declaration 
	 * of a certain port, in which specify the port's name is specified 
	 * through the @portName parameter
	 * NOTE: it is not advised to use the function without specifying 
	 * the port's name, as it results in duplication of resource count
	 * NOTE: if @portName is provided, @count can be omitted, as it 
	 * serves no purpose
	 * @param count (by default 1) the number of elements to add
	 * @param portName (by default the empty string) the name of the
	 * corresponding port
	 * @return the string describing the performed operation
	 */
	std::string addIOB(int count = 1, std::string portName = "");
	
	//---Even more particular resource logging
	/**
	 * Add @count multiplexers to the total estimate, each having 
	 * @nrInputs inputs of @width bitwidths
	 * NOTE: this function also modifies the total number of LUTs in 
	 * the design; this aspect should be considered so as not to result 
	 * in counting the resources multiple times and overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param nrInputs (by default 2) the number of inputs to the MUX
	 * @param width the bitwidth of the inputs and the output
	 * @return the string describing the performed operation
	 */
	std::string addMux(int width, int nrInputs = 2, int count = 1);
	
	/**
	 * Add @count counters to the total estimate, each having 
	 * @width bitwidth
	 * NOTE: this function also modifies the total number of LUTs and 
	 * FFs in the design; this aspect should be considered so as not to 
	 * result in counting the resources multiple times and overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param width the bitwidth of the counter
	 * @return the string describing the performed operation
	 */
	std::string addCounter(int width, int count = 1);
	
	/**
	 * Add @count accumulators to the total estimate, each having
	 * @width bitwidth
	 * NOTE: this function also modifies the total number of LUTs and 
	 * FFs and DSPs in the design; this aspect should be considered so 
	 * as not to result in counting the resources multiple times and 
	 * overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param width the bitwidth of the accumulator
	 * @param useDSP (by default false) whether the use of DSPs is allowed
	 * @return the string describing the performed operation
	 */
	std::string addAccumulator(int width, bool useDSP = false, int count = 1);
	
	/**
	 * Add @count decoder to the total estimate, each decoding an input 
	 * signal of wIn bits to an output signal of wOut bits
	 * NOTE: this function also modifies the total number of LUTs and 
	 * FFs and RAMs in the design; this aspect should be considered so 
	 * as not to result in counting the resources multiple times and 
	 * overestimate
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addDecoder(int wIn, int wOut, int count = 1);
	
	/**
	 * Add @count arithmetic operator to the total estimate, each having
	 * @nrInputs of @width bitwidths
	 * NOTE: this function also modifies the total number of LUTs in 
	 * the design; this aspect should be considered so as not to result 
	 * in counting the resources multiple times and overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param nrInputs (by default 2) the number of inputs of the gate
	 * @param width the bitwidth of the inputs
	 * @return the string describing the performed operation
	 */
	std::string addArithOp(int width, int nrInputs = 2, int count = 1);
	
	/**
	 * Add @count Finite State Machine to the total estimate, each 
	 * having @nrStates states, @nrTransitions transitions
	 * NOTE: this function also modifies the total number of LUTs and 
	 * FFs and ROMs in the design; this aspect should be considered so 
	 * as not to result in counting the resources multiple times and 
	 * overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param nrStates the number of states of the FSM
	 * @param nrTransitions (by default 0) the number of transitions of 
	 * the FSM
	 * @return the string describing the performed operation
	 */
	std::string addFSM(int nrStates, int nrTransitions = 0, int count = 1);
	
	//--Resource usage statistics
	/**
	 * Generate statistics regarding resource utilization in the design,
	 * based on the user's approximations
	 * @param detailLevel (by default 0, basic resource estimations) 
	 * the level of detail to which the resource utilizations are 
	 * reported (0 - basic report; 1 - include the more specific 
	 * resources; 2 - include all statistics)
	 * @return a formatted string containing the statistics
	 */
	std::string generateStatistics(int detailLevel = 0);
	
	//--Utility functions related to the generation of resource usage statistics
	/**
	 * Count registers that are due to design pipelining
	 * @return the string describing the performed operation
	 */
	std::string addPipelineFF();
	
	/**
	 * Count wires from declared signals
	 * @return the string describing the performed operation
	 */
	std::string addWireCount();
	
	/**
	 * Count I/O ports from declared inputs and outputs
	 * @return the string describing the performed operation
	 */
	std::string addPortCount();
	
	/**
	 * Count resources added from components
	 * @return the string describing the performed operation
	 */
	std::string addComponentResourceCount();
	
	/**
	 * Perform automatic operations related to resource estimation; this includes:
	 * 		- count registers added due to pipelining framework
	 * 		- count input/output ports
	 * 		- count resources in subcomponents
	 * Should not be used together with the manual estimation functions addWireCount, addPortCount, addComponentResourceCount!
	 * @return the string describing the performed operation
	 */
	void addAutomaticResourceEstimations();
	/////////////////////////////////////////////////////////////////////////////////////////////////
	
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Functions used for floorplanning
	/**
	 * NOTE: Floorplanning should be used only is resource estimation is 
	 * also used. The floorplanning tools rely on the data provided by 
	 * the resource estimation.
	 */
	
	
	/**
	 * Count the resources that have been added (as glue logic), since 
	 * the last module has been instantiated. It will create a virtual 
	 * module that is placed between the real modules, and that accounts 
	 * for the space needed for the glue logic.
	 * Possibly to be integrated in the instance() method, as the 
	 * process can be done without the intervention of the user.
	 * Uses and updates the pastEstimation... set of variables.
	 * @return the string summarizing the operation
	 */
	std::string manageFloorplan();
	
	/**
	 * Add a new placement constraint between the @source and @sink 
	 * modules. The constraint should be read as: "@sink is @type of @source".
	 * The type of the constraint should be one of the following 
	 * predefined constants: TO_LEFT_OF, TO_RIGHT_OF, ABOVE, UNDER.
	 * NOTE: @source and @sink are the operators' names, NOT 
	 * the instances' names
	 * @param source the source sub-component
	 * @param sink the sink sub-component
	 * @param type the constraint type (has as value predefined constant)
	 * @return the string summarizing the operation
	 */
	std::string addPlacementConstraint(std::string source, std::string sink, int type);
	
	/**
	 * Add a new connectivity constraint between the @source and @sink 
	 * modules. The constraint should be read as: "@sink is connected 
	 * to @source by @nrWires wires".
	 * NOTE: @source and @sink are the operators' names, NOT 
	 * the instances' names
	 * @param source the source sub-component
	 * @param sink the sink sub-component
	 * @param nrWires the number of wires that connect the two modules
	 * @return the string summarizing the operation
	 */
	std::string addConnectivityConstraint(std::string source, std::string sink, int nrWires);
	
	/**
	 * Add a new aspect constraint for @source module. The constraint 
	 * should be read as: "@source's width is @ratio times larger than 
	 * its width".
	 * @param source the source sub-component
	 * @param ratio the aspect ratio
	 * @return the string summarizing the operation
	 */
	std::string addAspectConstraint(std::string source, double ratio);
	
	/**
	 * Add a new constraint for @source module, regarding the contents 
	 * of the module. The constraint gives an indication on the possible 
	 * size/shape constraints, depending what the module contains.
	 * @param source the source sub-component
	 * @param value the type of content constraint
	 * @param length the length, if needed, of the component (for 
	 * example for adders or multipliers)
	 * @return the string summarizing the operation
	 */
	std::string addContentConstraint(std::string source, int value, int length);
	
	/**
	 * Process the placement and connectivity constraints that the 
	 * user has input using the corresponding functions.
	 * Start by processing the placement constraints and then, when 
	 * needed, process the connectivity constraints
	 * @return the string summarizing the operation
	 */
	std::string processConstraints();
	
	/**
	 * Create the virtual grid for the sub-components.
	 * @return the string summarizing the operation
	 */
	std::string createVirtualGrid();
	
	/**
	 * Transform the virtual placement grid into the actual placement on 
	 * the device, ready to generate the actual constraints file.
	 * @return the string summarizing the operation
	 */
	std::string createPlacementGrid();
	
	/**
	 * Create the file that will contain the floorplanning constraints.
	 * @return the string summarizing the operation
	 */
	std::string createConstraintsFile();
	
	/**
	 * Generate the placement for a given module.
	 * @param moduleName the name of the module
	 * @return the string summarizing the operation
	 */
	std::string createPlacementForComponent(std::string moduleName);
	
	/**
	 * Create the floorplan, according the flow described in each 
	 * function and according to the user placed constraints.
	 */
	std::string createFloorplan();
	/////////////////////////////////////////////////////////////////////////////////////////////////



	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////BEWARE: don't add anything below without adding it to cloneOperator, too

	map<string, Operator*> subComponents_;					/**< The list of sub-components */
	vector<Signal*>     signalList_;      					/**< The list of internal signals of the operator */
	vector<Signal*>     ioList_;          					/**< The list of I/O signals of the operator */

	FlopocoStream       vhdl;             					/**< The internal stream to which the constructor will build the VHDL code */
	int                 numberOfTests;    					/**< The number of tests, set by TestBench before this operator is tested */
	
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Variables used for resource estimations
	std::ostringstream 	resourceEstimate;					/**< The log of resource estimations made by the user */
	std::ostringstream 	resourceEstimateReport;				/**< The final report of resource estimations made by the user */
	
	ResourceEstimationHelper* reHelper;						/**< Performs all the necessary operations for resource estimation */
	
	bool reActive;											/**< Shows if any resource estimation operations have been performed */
	/////////////////////////////////////////////////////////////////////////////////////////////////
	
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Variables used for floorplanning
	std::ostringstream 			floorplan;					/**< Stream containing the floorplanning operations */
	
	FloorplanningHelper*		flpHelper;					/**< Tools for floorplanning */
	/////////////////////////////////////////////////////////////////////////////////////////////////
	

protected:    
	Target*             target_;          					/**< The target on which the operator will be deployed */
	string              uniqueName_;      					/**< By default, a name derived from the operator class and the parameters */
	string 				architectureName_;					/**< Name of the operator architecture */
	vector<Signal*>     testCaseSignals_; 					/**< The list of pointers to the signals in a test case entry. Its size also gives the dimension of a test case */

	map<string, string> portMap_;         					/**< Port map for an instance of this operator */
	map<string, double> outDelayMap;      					/**< Slack delays on the outputs */
	map<string, double> inputDelayMap;      				/**< Slack delays on the inputs */
	string              srcFileName;      					/**< Used to debug and report.  */
	map<string, int>    declareTable;     					/**< Table containing the name and declaration cycle of the signal */
	int                 myuid;              				/**<unique id>*/
	int                 cost;             					/**< the cost of the operator depending on different metrics */
	vector<Operator*>   oplist;                     /**< A list of all the sub-operators */
	

private:
	int                    stdLibType_;                 	/**< 0 will use the Synopsys ieee.std_logic_unsigned, -1 uses std_logic_unsigned, 1 uses ieee numeric_std  (preferred) */
	int                    numberOfInputs_;             	/**< The number of inputs of the operator */
	int                    numberOfOutputs_;            	/**< The number of outputs of the operator */
	bool                   isSequential_;               	/**< True if the operator needs a clock signal*/
	int                    pipelineDepth_;              	/**< The pipeline depth of the operator. 0 for combinatorial circuits */
	map<string, Signal*>   signalMap_;                  	/**< A container of tuples for recovering the signal based on it's name */ 
	map<string, pair<string, string> > constants_;      	/**< The list of constants of the operator: name, <type, value> */
	map<string, string>    attributes_;                  	/**< The list of attribute declarations (name, type) */
	map<string, string>    types_;                      	/**< The list of type declarations (name, type) */
	map<pair<string,string>, string >  attributesValues_;	/**< attribute values <attribute name, object (component, signal, etc)> ,  value> */
	bool                   hasRegistersWithoutReset_;   	/**< True if the operator has registers without a reset */
	bool                   hasRegistersWithAsyncReset_; 	/**< True if the operator has registers having an asynch reset */
	bool                   hasRegistersWithSyncReset_;  	/**< True if the operator has registers having a synch reset */
	string                 commentedName_;              	/**< Usually is the default name of the architecture.  */
	string                 copyrightString_;            	/**< Authors and years.  */
	int                    currentCycle_;               	/**< The current cycle, when building a pipeline */
	double                 criticalPath_;               	/**< The current delay of the current pipeline stage */
	bool                   needRecirculationSignal_;    	/**< True if the operator has registers having a recirculation signal  */
	bool                   hasClockEnable_;    	          /**< True if the operator has a clock enable signal  */
	int					    hasDelay1Feedbacks_;		/**< True if this operator has feedbacks of one cyle, and no more than one cycle (i.e. an error if the distance is more). False gives warnings */
	Operator*              indirectOperator_;              /**< NULL if this operator is just an interface operator to several possible implementations, otherwise points to the instance*/

};

	// global variables used through most of FloPoCo,
	// to be encapsulated in something, someday?
	
	extern int verbose;

} //namespace
#endif
