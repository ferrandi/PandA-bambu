/*
  Tools for performing resource estimations
 
  Authors:   Matei Istoan, Florent de Dinechin

  Initial software.
  Copyright © ENS-Lyon, INRIA, CNRS, UCBL, 
  2012 

  All Rights Reserved
*/


#ifndef RESOURCESTIMATIONHELPER_HPP
#define RESOURCESTIMATIONHELPER_HPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include "Operator.hpp"
#include "utils.hpp"

using namespace std;

namespace flopoco{


	class ResourceEstimationHelper
	{
	public:
		ResourceEstimationHelper(Target* target_, Operator* op_);
	
		/** The destructor */
		~ResourceEstimationHelper();
		
		
		//--Logging functions-------------------------------------------
		//---General resources------------------------------------------
		/**
		 * Initialize data structures used for resource estimation
		 */
		void initResourceEstimation();
		
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
		
		//---Even more particular resource logging----------------------
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
		
		//--Resource usage statistics-----------------------------------
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
		
		//--Utility functions for generating resource usage statistics--
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
		std::string addAutomaticResourceEstimations();
		/////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		//class public variables
		Target* target;
		Operator* parentOp;
		
		int 				estimatedCountFF;				/**< The estimated count of flip-flops used in the design */
		int 				estimatedCountLUT;				/**< The estimated count of function generators used in the design */
		int 				estimatedCountMultiplier;		/**< The estimated count of dedicated multipliers used in the design */
		int 				estimatedCountMemory;			/**< The estimated count of block memory elements used in the design */
		
		//more particular resource statistics
		int 				estimatedCountDSP;				/**< The estimated count of DSPs used in the design */
		int 				estimatedCountRAM;				/**< The estimated count of Block RAMs used in the design */
		int 				estimatedCountROM;				/**< The estimated count of Distributed used in the design */	
		int 				estimatedCountSRL;				/**< The estimated count of Shift Registers used in the design */
		int 				estimatedCountWire;				/**< The estimated count of wire elements (based on the signals used) used in the design */
		int 				estimatedCountIOB;				/**< The estimated count of Input/Output ports used in the design */
		
		//even more particular resource statistics
		int 				estimatedCountMux;				/**< The estimated count of multiplexers used in the design */
		int 				estimatedCountCounter;			/**< The estimated count of counters used in the design */
		int 				estimatedCountAccumulator;		/**< The estimated count of accumulators used in the design */
		int 				estimatedCountDecoder;			/**< The estimated count of decoders used in the design */
		int 				estimatedCountArithOp;			/**< The estimated count of arithmetic operations used in the design */
		int 				estimatedCountAdderSubtracter;	/**< The estimated count of adders-subtracters used in the design */
		int 				estimatedCountReg;				/**< The estimated count of registers used in the design */
		int 				estimatedCountFSM;				/**< The estimated count of Finite State Machines used in the design */
		
		//detailed resource estimations statistics
		vector<string>		estimatedSignalNames;			/**< The names of signals which have a manual resource estimation 
																	(DOES NOT CONCERN REGISTERS DUE TO PIPELINING) */
		vector<string>		estimatedPortNames;				/**< The names of ports which have a manual resource estimation */
		
		map<int, int>		estimatedLUTTypes;				/**< The list of different LUT types, based on their widths */
		
		map<int, int>		estimatedAdderTypes;				/**< The list of different adder types, based on their widths */
		map<int, int>		estimatedMultiplierTypes;		/**< The list of different multiplier types, based on their widths */
		map<int, int>		estimatedRegisterTypes;			/**< The list of different registers types, based on their widths */
		map<int, int>		estimatedShifterTypes;			/**< The list of different shifter types, based on their widths */
		map<int, int>		estimatedCounterTypes;			/**< The list of different counter types, based on their widths */
		map<int, int>		estimatedArithOpTypes;			/**< The list of different arithmetic operator types, based on their widths */
	};
	
}
#endif
