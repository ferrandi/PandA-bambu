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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

#include "hls_step.hpp"

// tree/ include
#include "tree_node.hpp"

CONSTREF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(Discrepancy);

class VcdSignalSelection : public HLS_step
{
 protected:
   const tree_managerRef TM;

   const DiscrepancyRef Discr;

   /// The name of the present state signal
   std::string present_state_name;

   /**
    * Return the set of analyses in relationship with this design step
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Selects the initial set of function parameters to skip, iterating on
    * the reached functions with a body and inserting the parm_decl
    * tree_nodes in param_to_skip
    * \param[in] reached_body_fun_ids  List of ids of the reached functions with body
    * \param[out] address_parameters The set of parm_decl tree_nodes representing addresses
    */
   void SelectInitialAddrParam(const CustomOrderedSet<unsigned int>& reached_body_fun_ids, CustomUnorderedMap<unsigned int, TreeNodeSet>& address_parameters);

   /**
    * Determines if the tree_node tn assigns an ssa_name representing an
    * address. The ssa to be skipped at the beginning are
    *    - variables representing addresses (pointers, arrays, vectors)
    *    - integer variables resulting from pointers casted to integer
    *    - ssa_name referred to a function's parameter marked as to skip
    *    - ssa_name to which is assigned the return value of a function to skip
    * If curr_tn must be skipped it is added to ssa_to_skip.
    * \param[in] tn The node to analyze
    * \param[in] addr_fun_ids List of ids of the reached functions that return an address value
    * \param[in] call_id_to_called_id Map the id of a call to the id of the called function
    */
   void InitialSsaIsAddress(const tree_nodeConstRef& tn, const CustomUnorderedSet<unsigned int>& addr_fun_ids, const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id);

   /**
    * Determines if the tree_node tn assigns an ssa_name representing an
    * address.
    * \param[in] tn The node to analyze
    */
   void InitialPhiResIsAddress(const tree_nodeConstRef& tn);

   /**
    * Selects the initial set of ssa to skip, iterating on the reached
    * functions with a body and inserting the ssa tree_nodes in ssa_to_skip
    * \param[in] reached_body_fun_ids  List of ids of the reached functions with body
    * \param[in] addr_fun_ids List of ids of the reached functions that return an address value
    * \param[in] call_id_to_called_id Map the id of a call to the id of the called function
    */
   void SelectInitialSsa(const CustomOrderedSet<unsigned int>& reached_body_fun_ids, const CustomUnorderedSet<unsigned int>& addr_fun_ids, const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id);

   /**
    * Single step used in the loop of PropagateAddrParamToSsa()
    */
   void SingleStepPropagateParamToSsa(const TreeNodeMap<size_t>& used_ssa, const TreeNodeSet& address_parameters);

   /**
    * Propagates the information on the ssa to skip to all the ssa in the
    * functions with body. This propagation mark to skip all the ssa_name
    * tree_nodes wich are referred to a param_decl already marked to be
    * skipped. The propagation is intra procedural.
    * \param[in] address_parameters Set of parm_decl tree_nodes of the parameters to skip
    * \param[in] reached_body_functions List of the ids of the reached functions with body
    */
   void PropagateAddrParamToSsa(const CustomUnorderedMap<unsigned int, TreeNodeSet>& address_parameters, const CustomOrderedSet<unsigned int>& reached_body_fun_ids);

   /**
    * Single step used in the loop of PropagateAddrSsa()
    */
   void SingleStepPropagateAddrSsa(const tree_nodeRef& curr_tn);

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
   void DetectInvalidReturns(const CustomOrderedSet<unsigned int>& reached_body_functions, CustomUnorderedSet<unsigned int>& addr_fun_ids);

   /**
    * Propagates the information on the ssa to skip to all the ssa in the
    * functions with body. This propagation is based on the ssa_name and
    * param_decl already marked to skip.
    * \param[in] address_parameters Set of parm_decl tree_nodes of the parameters to skip
    * \param[in] reached_body_functions List of the ids of the reached functions with body
    * \param[in, out] addr_fun_ids List of ids of the reached functions that return an address value
    */
   void InProcedurePropagateAddr(const CustomUnorderedMap<unsigned int, TreeNodeSet>& address_parameters, const CustomOrderedSet<unsigned int>& reached_body_functions, CustomUnorderedSet<unsigned int>& addr_fun_ids);

   /**
    * Propagates the information on the parameters to skip across function
    * calls. If a function is called with a parameter which is a ssa marked
    * to skip, then the parm_decl associated to that paramter must be marked
    * to skip. This propagation is based on the ssa names already marked to
    * be skipped
    * \param[in, out] address_parameters  Set of parm_decl tree_nodes of the parameters to skip
    * \param[in] reached_body_functions List of the ids of the reached functions with body
    * \param[in] addr_fun_ids List of ids of the reached functions that return an address value
    * \param[in] fu_id_to_call_ids  Maps a function id to the set of ids of operations where it calls other functions
    * \param[in] call_id_to_called_id  Maps the id of a call operation to the id of the called function
    */
   void CrossPropagateAddrSsa(CustomUnorderedMap<unsigned int, TreeNodeSet>& address_parameters, const CustomOrderedSet<unsigned int>& reached_body_functions, const CustomUnorderedSet<unsigned int>& addr_fun_ids,
                              const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& fu_id_to_call_ids, const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id);

   /**
    * Compute the ssa representing address values. They must be manipulated
    * in a different way by the discrepancy analysis
    * \param[in] fu_id_to_call_ids  Maps a function id to the set of ids of operations where it calls other functions
    * \param[in] call_id_to_called_id  Maps the id of a call operation to the id of the called function
    */
   void SelectAddrSsa(const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& fu_id_to_call_ids, const CustomUnorderedMap<unsigned int, UnorderedSetStdStable<unsigned int>>& call_id_to_called_id);

   /**
    * Checks if type_index represents an address type
    */
   bool IsAddressType(const unsigned int type_index) const;

   void SelectInternalSignals(CustomUnorderedMap<unsigned int, UnorderedSetStdStable<std::string>>& fun_id_to_sig_names) const;

 public:
   /**
    * Constructor
    */
   VcdSignalSelection(const ParameterConstRef Param, const HLS_managerRef HLSMgr, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~VcdSignalSelection() override;

   /**
    * Executes the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   bool HasToBeExecuted() const override;
};
