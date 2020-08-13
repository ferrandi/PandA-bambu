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
#include "tree_manager.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(bloc);
class statement_list;
class gimple_node;
//@}

class dead_code_elimination : public FunctionFrontendFlowStep
{
 private:
   std::map<unsigned int, bool> last_writing_memory;

   std::map<unsigned int, bool> last_reading_memory;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void fix_sdc_motion(tree_nodeRef removedStmt) const;
   void kill_uses(const tree_managerRef TM, tree_nodeRef op0) const;
   void kill_vdef(const tree_managerRef TM, tree_nodeRef vdef);
   unsigned move2emptyBB(const tree_managerRef TM, statement_list* sl, unsigned pred, blocRef bb_pred, unsigned cand_bb_dest, unsigned bb_dest) const;
   void add_gimple_nop(gimple_node* gc, const tree_managerRef TM, tree_nodeRef cur_stmt, blocRef bb);

 public:
   /**
    * Constructor
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the index of the function
    * @param design_flow_manager is the design flow manager
    */
   dead_code_elimination(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

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

   static void fix_sdc_motion(DesignFlowManagerConstRef design_flow_manager, unsigned int function_id, tree_nodeRef removedStmt);
};

#endif
