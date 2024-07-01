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
 * @file mem_dominator_allocation.hpp
 * @brief Class to allocate memories in HLS based on dominators
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef MEMORY_DOMINATOR_ALLOCATION_HPP
#define MEMORY_DOMINATOR_ALLOCATION_HPP

#include "custom_set.hpp"
#include "memory_allocation.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(CallGraphManager);
//@}

class mem_dominator_allocation : public memory_allocation
{
 protected:
   /// user defined base address
   unsigned long long int user_defined_base_address;

   std::map<std::string, std::set<std::string>> user_internal_objects;

   std::map<std::string, std::set<std::string>> user_external_objects;

   /// function checking if the current variable has to allocated inside the accelerator or outside
   virtual bool is_internal_obj(unsigned int var_id, unsigned int fun_id, bool multiple_top_call_graph);

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    */
   mem_dominator_allocation(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
                            const DesignFlowManagerConstRef design_flow_manager,
                            const HLSFlowStepSpecializationConstRef hls_flow_step_specialization,
                            const HLSFlowStep_Type hls_flow_step_type = HLSFlowStep_Type::DOMINATOR_MEMORY_ALLOCATION);

   /**
    * Destructor
    */
   virtual ~mem_dominator_allocation() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};

#endif
