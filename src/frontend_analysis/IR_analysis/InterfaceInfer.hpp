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
 *              Copyright (C) 2022-2022 Politecnico di Milano
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
 * @file InterfaceInfer.hpp
 * @brief Load parsed protocol interface attributes
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef INTERFACE_INFER_HPP
#define INTERFACE_INFER_HPP

#include "application_frontend_flow_step.hpp"

/// utility includes
#include "refcount.hpp"

REF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_manager);
struct statement_list;
struct function_decl;
struct gimple_assign;
struct gimple_node;

#include "custom_set.hpp"
#include <list>
#include <set>

class InterfaceInfer : public ApplicationFrontendFlowStep
{
 private:
   enum class m_axi_type
   {
      none,
      direct,
      axi_slave
   };

   bool already_executed;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   void classifyArgRecurse(CustomOrderedSet<unsigned>& Visited, tree_nodeRef ssa_var, const statement_list* sl,
                           std::list<tree_nodeRef>& writeStmt, std::list<tree_nodeRef>& readStmt);

   void classifyArg(statement_list* sl, tree_nodeRef ssa_var, std::list<tree_nodeRef>& writeStmt,
                    std::list<tree_nodeRef>& readStmt);

   void create_Read_function(tree_nodeRef origStmt, const std::string& arg_name, const std::string& fdName,
                             tree_nodeRef aType, tree_nodeRef readType, const tree_manipulationRef tree_man,
                             const tree_managerRef TM, bool commonRWSignature);

   void create_Write_function(const std::string& arg_name, tree_nodeRef origStmt, const std::string& fdName,
                              tree_nodeRef writeValue, tree_nodeRef aType, tree_nodeRef writeType,
                              const tree_manipulationRef tree_man, const tree_managerRef TM, bool commonRWSignature);

   void create_resource_Read_simple(const std::set<std::string>& operations, const std::string& arg_name,
                                    const std::string& interfaceType, unsigned long long input_bw, bool IO_port,
                                    unsigned int n_resources, unsigned long long rwBWsize, unsigned int top_id) const;

   void create_resource_Write_simple(const std::set<std::string>& operations, const std::string& arg_name,
                                     const std::string& interfaceType, unsigned long long input_bw, bool IO_port,
                                     bool isDiffSize, unsigned int n_resources, bool is_real,
                                     unsigned long long rwBWsize, unsigned int top_id) const;

   void create_resource_array(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW,
                              const std::string& bundle_name, const std::string& interfaceType,
                              unsigned long long input_bw, unsigned int arraySize, unsigned n_resources,
                              unsigned int alignment, bool is_real, unsigned long long rwBWsize,
                              unsigned int top_id) const;

   void create_resource_m_axi(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW,
                              const std::string& arg_name, const std::string& bundle_name,
                              const std::string& interfaceType, unsigned long long input_bw, unsigned int n_resources,
                              m_axi_type mat, unsigned long long rwBWsize, unsigned int top_id) const;

   void create_resource(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW,
                        const std::string& arg_name, const std::string& interfaceType, unsigned long long input_bw,
                        bool isDiffSize, const std::string& fname, unsigned int n_resources, unsigned int alignment,
                        bool isReal, unsigned long long rwBWsize, unsigned int top_id) const;

   void ComputeResourcesAlignment(unsigned int& n_resources, unsigned int& alignment, unsigned long long input_bw,
                                  bool is_acType, bool is_signed, bool is_fixed);

   void FixReadWriteCall(const gimple_assign* ga, gimple_node* newGN, const tree_manipulationRef tree_man,
                         tree_nodeRef new_call, statement_list* sl, const tree_managerRef TM, tree_nodeRef origStmt,
                         unsigned int destBB, const std::string& fname, const std::string& arg_name);

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param _Param is the set of the parameters
    */
   InterfaceInfer(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager,
                  const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~InterfaceInfer() override;

   bool HasToBeExecuted() const override;

   void Initialize() override;

   /**
    * Execute this step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};

#endif
