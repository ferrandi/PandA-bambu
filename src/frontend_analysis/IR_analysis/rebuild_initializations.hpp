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
 * @file rebuild_initializations.hpp
 * @brief rebuild initialization where it is possible
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef rebuild_initializations_HPP
#define rebuild_initializations_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(rebuild_initialization);
REF_FORWARD_DECL(tree_node);
class mem_ref;
//@}

#include "custom_map.hpp"
#include <list>
#include <string>

/**
 * Rebuild initialization function flow front-end step
 */
class rebuild_initialization : public FunctionFrontendFlowStep
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param DesignFlowManagerConstRef is the design flow manager
    */
   rebuild_initialization(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~rebuild_initialization() override;

   /**
    * Rebuild initialization function flow front-end step.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

/**
 * Rebuild initialization function flow front-end step done after IR_lowering
 */
class rebuild_initialization2 : public FunctionFrontendFlowStep
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * @brief extract_var_decl_ppe
    * @param vd_index is the variable decl index
    * @param vd_node is the variable decl tree re-index node
    */
   bool extract_var_decl_ppe(tree_nodeRef addr_assign_op1, unsigned& vd_index, tree_nodeRef& vd_node);

   /**
    * @brief extract_var_decl return the variable decl referred by the mem_ref given it is resolvable
    * @param me is the memory reference node
    * @param vd_index is the variable decl index
    * @param vd_node is the variable decl tree re-index node
    * @param addr_assign_op1 is the pointer expression used by the mem_ref
    * @return true in case it is possible to compute the variable decl referred, false otherwise
    */
   bool extract_var_decl(const mem_ref* me, unsigned& vd_index, tree_nodeRef& vd_node, tree_nodeRef& addr_assign_op1);

   /**
    * @brief look_for_ROMs transforms the IR by looking for an initial sequence of writes followed
    * by read only instructions. In case the writes have constant offset and
    * the written values are constants a constant array may be defined.
    */
   bool look_for_ROMs();

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param DesignFlowManagerConstRef is the design flow manager
    */
   rebuild_initialization2(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~rebuild_initialization2() override;

   /**
    * Rebuild initialization function flow front-end step.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif
