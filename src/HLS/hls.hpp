/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file hls.hpp
 * @brief Data structure definition for high-level synthesis flow.
 *
 * This struct contains all information useful to high-level synthesis flow.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef HLS_HPP
#define HLS_HPP

#include "op_graph.hpp"
#include "refcount.hpp"

#include <iostream>

class OpGraph;
class StorageValueInformation;
class xml_element;
enum class HLSFlowStep_Type;
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(ChainingInformation);
REF_FORWARD_DECL(conn_binding);
REF_FORWARD_DECL(fu_binding);
REF_FORWARD_DECL(HLS_constraints);
REF_FORWARD_DECL(HLS_device);
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(liveVariables);
REF_FORWARD_DECL(reg_binding);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(StateTransitionGraphManager);
REF_FORWARD_DECL(FSMInfo);
REF_FORWARD_DECL(structural_manager);

/**
 * @class hls
 * @ingroup HLS
 * Data structure that contains all information about high level synthesis process
 */
struct hls
{
   /// this is the identifier of the function to be implemented
   unsigned int functionId;

   /// The type of module binding to be adopted
   HLSFlowStep_Type module_binding_algorithm;

   /// The type of chaining algorithm to be adopted
   HLSFlowStep_Type chaining_algorithm;

   /// The type of live variable algorithm to be adopted
   HLSFlowStep_Type liveVariableAlgorithm;

   /// Set representing the subset of operations in the specification to be implemented
   OpVertexSet operations;

   // -------------- High Level Synthesis Specific -------------- //

   /// reference to the information representing the target for the synthesis
   const HLS_deviceRef HLS_D;

   /// store the HLS constraints
   const HLS_constraintsRef HLS_C;

   // -------------- High Level Synthesis Results -------------- //

   /// Store the technology information
   AllocationInformationRef allocation_information;

   /// Store the refcounted scheduling of the operations.
   ScheduleRef Rsch;

   /// Store the refcounted functional unit binding of the operations.
   fu_bindingRef Rfu;

   /// Store the finite state machine metadata
   FSMInfoRef fsm_info;

   /// data-structure containing the live variable sets
   liveVariablesRef Rliv;

   /// data-structure for storage values
   std::unique_ptr<StorageValueInformation> storage_value_information;

   /// Store the refcounted register binding of the variables.
   reg_bindingRef Rreg;

   /// Store the refcounted register group binding of the variables.
   reg_bindingRef RregGroup;

   /// Store the refcounted interconnection of datapath elements.
   conn_bindingRef Rconn;

   /// Store the refcounted chaining info
   ChainingInformationRef chaining_information;

   // -------------- high level synthesis structural representation -------------- //

   /// true when the module has registered inputs
   bool registered_inputs;
   /// true when the done port is registered
   bool registered_done_port;

   /// The number of call points to this function
   size_t call_sites_number;

   /// Store the datapath description.
   structural_managerRef datapath;

   /// Store the controller description.
   structural_managerRef controller;

   /// Store the top description.
   structural_managerRef top;

   // -------------- Parameters -------------- //

   /// class containing all the parameters
   const ParameterConstRef Param;

   /// debugging level of the class
   int debug_level;

   /// verbosity level of the class
   int output_level;

   /// HLS execution time
   long HLS_execution_time;

   // -------------- Constructor & Destructor -------------- //

   /**
    *
    */
   hls(const ParameterConstRef Param, unsigned int function_id, OpVertexSet operations, const HLS_deviceRef HLS_D,
       const HLS_constraintsRef HLS_C);

   /**
    * Loads previous HLS results from XML node
    * @param rootnode is the pointer to the node containing all the intermediate results
    * @param data is the operation graph used to decode the serialized data
    */
   void xload(const xml_element* rootnode, const OpGraph& data);

   /**
    * Writes current HLS results to XML node
    * @param rootnode is the pointer to the node where all the intermediate results have to be stored
    * @param data is the operation graph used to encode the serialized data
    */
   void xwrite(xml_element* rootnode, const OpGraph& data);

   // -------------- Printing helpers -------------- //

   /**
    * Prints the hls solution available up to now. It prints information about:
    *   - scheduling
    *   - functional units binding
    *   - total number of control steps
    *   - register binding (if register allocation has been performed)
    *   - connection binding (if it has been performed)
    * @param os is the stream where the information have to be printed
    */
   void print(std::ostream& os) const;

   /**
    * Prints on stream the scheduling (if it has been performed). It prints information about
    *   - Operation name
    *   - Control step where it will start its execution
    *   - Functional unit where it has been mapped
    */
   void PrintScheduling() const;

   /**
    * Prints on stream the register binding (if it has been performed). It prints information about
    *   - Variable name
    *   - Operations which variable is live beetween
    *   - Register where the variable has been stored
    * @param os is the stream where the information have to be printed
    */
   void print_register_binding(std::ostream& os) const;

   /**
    * Prints on stream the register grouping (if it has been performed). It prints information about
    *   - Variable name
    *   - Operations which variable is live beetween
    *   - Register where the variable has been stored
    * @param os is the stream where the information have to be printed
    */
   void print_register_grouping(std::ostream& os) const;

   /**
    * Prints on stream the connection binding (if it has been performed). It prints information about
    *   - Connection source and target
    *   - Element used to perform connection
    *   - Variables that could cross the connection
    * @param os is the stream where the information have to be printed
    */
   void print_connection_binding(std::ostream& os) const;

   /**
    * Prints the summary of allocated resources
    */
   void PrintResources() const;

   /**
    * Friend definition of the << operator.
    */
   friend std::ostream& operator<<(std::ostream& os, const hls& s)
   {
      s.print(os);
      return os;
   }
};
/// refcount definition of the class
using hlsRef = refcount<hls>;

#endif
