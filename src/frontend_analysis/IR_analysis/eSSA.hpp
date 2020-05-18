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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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
 * @file eSSA.hpp
 * @brief
 *
 * @author Michele Fiorito <michele2.fiorito@mail.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef ESSA_HPP
#define ESSA_HPP

#include "basic_block.hpp"
#include "custom_map.hpp"
#include "function_frontend_flow_step.hpp"
#include "tree_node.hpp"

REF_FORWARD_DECL(Operand);
REF_FORWARD_DECL(tree_node);
CONSTREF_FORWARD_DECL(tree_node);
class ValueInfo;
class DFSInfo;

class eSSA : public FunctionFrontendFlowStep
{
 public:
   using ValueInfoLookup = CustomMap<tree_nodeConstRef, unsigned int>;

 private:
   BBGraphRef DT;
   unsigned int bb_ver;
   unsigned int bv_ver;

   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   bool renameUses(CustomSet<OperandRef>& OpSet, ValueInfoLookup& ValueInfoNums, std::vector<ValueInfo>& ValueInfos, CustomMap<unsigned int, DFSInfo>& DFSInfos, CustomSet<std::pair<unsigned int, unsigned int>>& EdgeUsesOnly, statement_list* sl);

 public:
   /**
    * Constructor.
    * @param _Param is the set of the parameters
    * @param _AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   eSSA(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~eSSA() override;

   /**
    * compute the e-SSA form
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   bool HasToBeExecuted() const override;
};

#endif // !ESSA_HPP
