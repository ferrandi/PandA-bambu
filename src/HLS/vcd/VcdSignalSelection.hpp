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
 *              Copyright (C) 2015-2026 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */
#ifndef VCD_SIGNAL_SELECTION_HPP
#define VCD_SIGNAL_SELECTION_HPP
#include "hls_step.hpp"
#include "ir_node.hpp"

CONSTREF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(Discrepancy);

class VcdSignalSelection : public HLS_step
{
 protected:
   const ir_managerRef TM;

   const DiscrepancyRef Discr;

   /// The name of the present state signal
   std::string present_state_name;

   /**
    * Return the set of analyses in relationship with this design step
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Selects the initial set of function parameters to skip, iterating on
    * the reached functions with a body and inserting the argument_val_node
    * IR nodes in param_to_skip
    * \param[in] reached_body_fun_ids  List of ids of the reached functions with body
    * \param[out] address_parameters The set of argument_val_node IR nodes representing addresses
    */
   void SelectInitialAddrParam(const CustomOrderedSet<unsigned int>& reached_body_fun_ids,
                               CustomUnorderedMap<unsigned int, IRNodeSet>& address_parameters);

   /**
    * Determines if the ir_node tn assigns an ssa_node representing an
    * address. The ssa to be skipped at the beginning are
    *    - variables representing addresses (pointers, arrays, vectors)
    *    - integer variables resulting from pointers casted to integer
    *    - ssa_node referred to a function's parameter marked as to skip
    *    - ssa_node to which is assigned the return value of a function to skip
    * If curr_tn must be skipped it is added to ssa_to_skip.
    * \param[in] tn The node to analyze
    * \param[in] addr_fun_ids List of ids of the reached functions that return an address value
    * \param[in] call_id_to_called_id Map the id of a call to the id of the called function
    */
   void InitialSsaIsAddress(
       const ir_nodeConstRef& tn, const CustomUnorderedSet<unsigned int>& addr_fun_ids,
       const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id);

   /**
    * Determines if the ir_node tn assigns an ssa_node representing an
    * address.
    * \param[in] tn The node to analyze
    */
   void InitialPhiResIsAddress(const ir_nodeConstRef& tn);

   /**
    * Selects the initial set of ssa to skip, iterating on the reached
    * functions with a body and inserting the ssa IR nodes in ssa_to_skip
    * \param[in] reached_body_fun_ids  List of ids of the reached functions with body
    * \param[in] addr_fun_ids List of ids of the reached functions that return an address value
    * \param[in] call_id_to_called_id Map the id of a call to the id of the called function
    */
   void
   SelectInitialSsa(const CustomOrderedSet<unsigned int>& reached_body_fun_ids,
                    const CustomUnorderedSet<unsigned int>& addr_fun_ids,
                    const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id);

   /**
    * Single step used in the loop of PropagateAddrParamToSsa()
    */
   void SingleStepPropagateParamToSsa(const IRNodeMap<size_t>& used_ssa, const IRNodeSet& address_parameters);

   /**
    * Propagates the information on the ssa to skip to all the ssa in the
    * functions with body. This propagation mark to skip all the ssa_node
    * IR nodes wich are referred to a param_decl already marked to be
    * skipped. The propagation is intra procedural.
    * \param[in] address_parameters Set of argument_val_node IR nodes of the parameters to skip
    * \param[in] reached_body_fun_ids List of the ids of the reached functions with body
    */
   void PropagateAddrParamToSsa(const CustomUnorderedMap<unsigned int, IRNodeSet>& address_parameters,
                                const CustomOrderedSet<unsigned int>& reached_body_fun_ids);

   /**
    * Single step used in the loop of PropagateAddrSsa()
    */
   void SingleStepPropagateAddrSsa(const ir_nodeRef& curr_tn, IRNodeSet& new_address_ssa);

   /**
    * Propagates the information on the ssa representing addresses across
    * all the ssa in the functions with body. The propagation is intra procedural.
    */
   void PropagateAddrSsa();

   /**
    * Detects return statements resulting in values to be skipped in the
    * discrepancy analysis
    * \param[in] reached_body_functions List of the ids of the reached functions with body
    * \param[in, out] addr_fun_ids List of ids of the reached functions that return an address value
    */
   void DetectInvalidReturns(const CustomOrderedSet<unsigned int>& reached_body_functions,
                             CustomUnorderedSet<unsigned int>& addr_fun_ids);

   /**
    * Propagates the information on the ssa to skip to all the ssa in the
    * functions with body. This propagation is based on the ssa_node and
    * param_decl already marked to skip.
    * \param[in] address_parameters Set of argument_val_node IR nodes of the parameters to skip
    * \param[in] reached_body_functions List of the ids of the reached functions with body
    * \param[in, out] addr_fun_ids List of ids of the reached functions that return an address value
    */
   void InProcedurePropagateAddr(const CustomUnorderedMap<unsigned int, IRNodeSet>& address_parameters,
                                 const CustomOrderedSet<unsigned int>& reached_body_functions,
                                 CustomUnorderedSet<unsigned int>& addr_fun_ids);

   /**
    * Propagates the information on the parameters to skip across function
    * calls. If a function is called with a parameter which is a ssa marked
    * to skip, then the argument_val_node associated to that paramter must be marked
    * to skip. This propagation is based on the ssa names already marked to
    * be skipped
    * \param[in, out] address_parameters  Set of argument_val_node IR nodes of the parameters to skip
    * \param[in] reached_body_functions List of the ids of the reached functions with body
    * \param[in] addr_fun_ids List of ids of the reached functions that return an address value
    * \param[in] fu_id_to_call_ids  Maps a function id to the set of ids of operations where it calls other functions
    * \param[in] call_id_to_called_id  Maps the id of a call operation to the id of the called function
    */
   void CrossPropagateAddrSsa(
       CustomUnorderedMap<unsigned int, IRNodeSet>& address_parameters,
       const CustomOrderedSet<unsigned int>& reached_body_functions,
       const CustomUnorderedSet<unsigned int>& addr_fun_ids,
       const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& fu_id_to_call_ids,
       const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id);

   /**
    * Compute the ssa representing address values. They must be manipulated
    * in a different way by the discrepancy analysis
    * \param[in] fu_id_to_call_ids  Maps a function id to the set of ids of operations where it calls other functions
    * \param[in] call_id_to_called_id  Maps the id of a call operation to the id of the called function
    */
   void
   SelectAddrSsa(const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& fu_id_to_call_ids,
                 const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id);

   /**
    * Checks if type_index represents an address type
    */
   bool IsAddressType(const ir_nodeRef& tn) const;

   void SelectInternalSignals(
       CustomUnorderedMap<unsigned int, UnorderedSetStdStable<std::string>>& fun_id_to_sig_names) const;

 public:
   VcdSignalSelection(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
                      const DesignFlowManager& design_flow_manager);

   bool HasToBeExecuted() const override;

   DesignFlowStep_Status Exec() override;
};
#endif
