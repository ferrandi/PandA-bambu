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
 * @file CSE.hpp
 * @brief CSE analysis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef CSE_HPP
#define CSE_HPP

#include "function_frontend_flow_step.hpp"

#include "custom_map.hpp"
#include <boost/tuple/tuple.hpp>

#include "tree_common.hpp"

#include "refcount.hpp"
/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(CSE);
REF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(tree_manager);
class gimple_assign;
class statement_list;
//@}

#if NO_ABSEIL_HASH

/**
 * Definition of hash function for InstrumentInstructionWriter::InstrumentationType
 */
namespace std
{
   template <>
   struct hash<enum kind> : public unary_function<enum kind, size_t>
   {
      size_t operator()(enum kind t) const
      {
         hash<int> hasher;
         return hasher(static_cast<int>(t));
      }
   };
} // namespace std
#endif

/**
 * @brief CSE analysis
 */
class CSE : public FunctionFrontendFlowStep
{
 private:
   /// The scheduling solution
   ScheduleRef schedule;

   /// tree manager
   const tree_managerRef TM;

   /// The statement list
   statement_list* sl;

   /// when true PHI_OPT step has to restart
   bool restart_phi_opt;

   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /// define the type of the unique table key
   typedef std::pair<enum kind, std::vector<unsigned int>> CSE_tuple_key_type;

   /// check if the statement has an equivalent in the unique table
   tree_nodeRef hash_check(tree_nodeRef tn, vertex bb, std::map<vertex, CustomUnorderedMapStable<CSE_tuple_key_type, tree_nodeRef>>& unique_table);

   /// check if the gimple assignment is a load, store or a memcpy/memset
   bool check_loads(const gimple_assign* ga, unsigned int right_part_index, tree_nodeRef right_part);

 public:
   /**
    * Constructor.
    * @param _Param is the set of the parameters
    * @param _AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   CSE(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~CSE() override;
   /**
    * perform CSE analysis
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec)
    */
   void Initialize() override;
};

#endif /* CSE_HPP */
