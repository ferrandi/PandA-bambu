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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file dead_code_elimination.hpp
 * @brief Eliminate dead code
 *
 * @author Andrea Cuoccio <andrea.cuoccio@gmail.com>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef DEAD_CODE_ELIMINATION_HPP
#define DEAD_CODE_ELIMINATION_HPP
/// Super class include
#include "function_frontend_flow_step.hpp"

/// Utility include
#include "refcount.hpp"

/// behaviour include
#include "call_graph.hpp"

/// Frontend include
#include "Parameter.hpp"

/// Tree include

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(bloc);
class statement_list;
class gimple_node;
//@}

class dead_code_elimination : public FunctionFrontendFlowStep
{
 private:
   std::map<unsigned int, bool> last_writing_memory;

   std::map<unsigned int, bool> last_reading_memory;

   bool restart_if_opt;

   bool restart_mwi_opt;

   bool restart_mem;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void kill_uses(const tree_managerRef& TM, const tree_nodeRef& op0) const;

   /**
    *
    * @param gc
    * @param TM
    * @param cur_stmt
    * @param bb
    * @return tree_nodeRef
    */
   tree_nodeRef add_gimple_nop(const tree_managerRef& TM, const tree_nodeRef& cur_stmt, const blocRef& bb);

   void fix_sdc_motion(tree_nodeRef removedStmt) const;

   blocRef move2emptyBB(const tree_managerRef& TM, const unsigned int new_bbi, const statement_list* sl,
                        const blocRef& bb_pred, const unsigned int cand_bb_dest, const unsigned int bb_dest) const;

 public:
   /**
    * Constructor
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the index of the function
    * @param design_flow_manager is the design flow manager
    */
   dead_code_elimination(const ParameterConstRef _parameters, const application_managerRef AppM,
                         unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~dead_code_elimination() override;

   /**
    * Performs dead code elimination.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   /**
    * Replace virtual ssa definition with gimple nop
    * @param TM tree manager instance
    * @param vdef virtual ssa
    * @return tree_nodeRef generated gimple nop statement
    */
   static tree_nodeRef kill_vdef(const tree_managerRef& TM, const tree_nodeRef& vdef);

   static void fix_sdc_motion(DesignFlowManagerConstRef design_flow_manager, unsigned int function_id,
                              tree_nodeRef removedStmt);
};

#endif
