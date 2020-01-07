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
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef HLS_HPP
#define HLS_HPP

#include "custom_map.hpp"
#include "custom_set.hpp"
#include <iosfwd>
#include <string>

#include "graph.hpp"
#include "op_graph.hpp"
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(ChainingInformation);
REF_FORWARD_DECL(conn_binding);
REF_FORWARD_DECL(DesignFlowStep);
REF_FORWARD_DECL(fu_binding);
REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(HLS_constraints);
REF_FORWARD_DECL(HLS_target);
REF_FORWARD_DECL(liveness);
CONSTREF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(reg_binding);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(StateTransitionGraphManager);
REF_FORWARD_DECL(StorageValueInformation);
REF_FORWARD_DECL(structural_manager);
/// forward decl of xml Element
class xml_element;
enum class HLSFlowStep_Type;
//@}

#include "utility.hpp"

/**
 * @class hls
 * @ingroup HLS
 * Data structure that contains all information about high level synthesis process
 */
class hls
{
 public:
   /// this is the identifier of the function to be implemented
   unsigned int functionId;

   /// The type of controller to be instantiated
   HLSFlowStep_Type controller_type;

   /// The type of module binding to be adopted
   HLSFlowStep_Type module_binding_algorithm;

   /// The type of chaining algorithm to be adopted
   HLSFlowStep_Type chaining_algorithm;

   /// The type of liveness algorithm to be adopted
   HLSFlowStep_Type liveness_algorithm;

   /// Set representing the subset of operations in the specification to be implemented
   OpVertexSet operations;

   // -------------- High Level Synthesis Specific -------------- //

   /// reference to the information representing the target for the synthesis
   const HLS_targetRef HLS_T;

   /// store the HLS constraints
   const HLS_constraintsRef HLS_C;

   // -------------- High Level Synthesis Results -------------- //

   /// Store the technology information
   AllocationInformationRef allocation_information;

   /// Store the refcounted scheduling of the operations.
   ScheduleRef Rsch;

   /// Store the refcounted functional unit binding of the operations.
   fu_bindingRef Rfu;

   /// Store the refcounted state transition graph
   StateTransitionGraphManagerRef STG;

   /// datastructure containing the variable liveness
   livenessRef Rliv;

   /// datastructure for storage values
   StorageValueInformationRef storage_value_information;

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

   /// Store the description of the control flow checker
   structural_managerRef control_flow_checker;

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
   hls(const ParameterConstRef Param, unsigned int function_id, OpVertexSet operations, const HLS_targetRef HLS_T, const HLS_constraintsRef HLS_C);

   /**
    * Destructor
    */
   ~hls();

   /**
    * Loads previous HLS results from XML node
    * @param rootnode is the pointer to the node containing all the intermediate results
    */
   void xload(const xml_element* rootnode, const OpGraphConstRef data);

   /**
    * Writes current HLS results to XML node
    * @param rootnode is the pointer to the node where all the intermediate results have to be stored
    */
   void xwrite(xml_element* rootnode, const OpGraphConstRef data);

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
typedef refcount<hls> hlsRef;

#endif
