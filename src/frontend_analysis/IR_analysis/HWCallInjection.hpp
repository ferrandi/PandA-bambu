/*
 *                 _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *               _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *              _/      _/    _/ _/    _/ _/   _/ _/    _/
 *             _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *           ***********************************************
 *                            PandA Project
 *                   URL: http://panda.dei.polimi.it
 *                     Politecnico di Milano - DEIB
 *                      System Architectures Group
 *           ***********************************************
 *            Copyright (C) 2004-2020 Politecnico di Milano
 *
 * This file is part of the PandA framework.
 *
 * The PandA framework is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HW_CALL_INJECTION_HPP
#define HW_CALL_INJECTION_HPP

#include "function_frontend_flow_step.hpp"

#include "custom_map.hpp"
#include <list>

#include "refcount.hpp"

REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(HWCallInjection);
REF_FORWARD_DECL(tree_node);

class HWCallInjection : public FunctionFrontendFlowStep
{
 private:
   static unsigned int builtinWaitCallDeclIdx;

   /// True if already executed
   bool already_executed;

   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType RT) const override;

   bool isHardwareCall(tree_nodeRef FD);

   void buildBuiltinCall(const blocRef block, const tree_nodeRef stmt);

 public:
   HWCallInjection(const ParameterConstRef Param, const application_managerRef AppM, unsigned int funId, const DesignFlowManagerConstRef DFM);

   ~HWCallInjection() override;

   DesignFlowStep_Status InternalExec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};

#endif
