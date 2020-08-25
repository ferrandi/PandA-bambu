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
 *              Copyright (C) 2018-2020 Politecnico di Milano
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
 * @file interface_infer.hpp
 * @brief Restructure the top level function to adhere the specified interface.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef INTERFACE_INFER_HPP
#define INTERFACE_INFER_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// utility includes
#include "refcount.hpp"

REF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_manager);
class statement_list;
class function_decl;
class parm_decl;
class ssa_name;

#include "custom_set.hpp"
#include <list>
#include <set>

class interface_infer : public FunctionFrontendFlowStep
{
 private:
   enum class m_axi_type
   {
      none,
      direct,
      axi_slave
   };

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void Computepar2ssa(statement_list* sl, std::map<unsigned, unsigned>& par2ssa);

   void classifyArgRecurse(CustomOrderedSet<unsigned>& Visited, ssa_name* argSSA, unsigned int destBB, statement_list* sl, bool& canBeMovedToBB2, bool& isRead, bool& isWrite, bool& unkwown_pattern, std::list<tree_nodeRef>& writeStmt,
                           std::list<tree_nodeRef>& readStmt);
   void classifyArg(statement_list* sl, tree_nodeRef argSSANode, bool& canBeMovedToBB2, bool& isRead, bool& isWrite, bool& unkwown_pattern, std::list<tree_nodeRef>& writeStmt, std::list<tree_nodeRef>& readStmt);
   void addGimpleNOPxVirtual(tree_nodeRef origStmt, const tree_managerRef TM, CustomOrderedSet<unsigned>& writeVdef);

   void create_Read_function(tree_nodeRef refStmt, const std::string& argName_string, tree_nodeRef origStmt, unsigned int destBB, const std::string& fdName, tree_nodeRef argSSANode, tree_nodeRef aType, tree_nodeRef readType,
                             const std::list<tree_nodeRef>& usedStmt_defs, const tree_manipulationRef tree_man, const tree_managerRef TM, bool commonRWSignature);
   void create_Write_function(const std::string& argName_string, tree_nodeRef origStmt, const std::string& fdName, tree_nodeRef writeValue, tree_nodeRef aType, tree_nodeRef writeType, const tree_manipulationRef tree_man, const tree_managerRef TM,
                              bool commonRWSignature, CustomOrderedSet<unsigned>& writeVdef);

   void create_resource_Read_simple(const std::set<std::string>& operations, const std::string& argName_string, const std::string& interfaceType, unsigned int inputBitWidth, bool IO_port, unsigned n_resources, unsigned rwBWsize);
   void create_resource_Write_simple(const std::set<std::string>& operations, const std::string& argName_string, const std::string& interfaceType, unsigned int inputBitWidth, bool IO_port, bool isDiffSize, unsigned n_resources, bool is_real,
                                     unsigned rwBWsize);
   void create_resource_array(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW, const std::string& argName_string, const std::string& interfaceType, unsigned int inputBitWidth, unsigned int arraySize, unsigned n_resources,
                              unsigned alignment, bool is_real, unsigned rwBWsize);
   void create_resource_m_axi(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW, const std::string& argName_string, const std::string& portNameSpecializer, const std::string& interfaceType, unsigned int inputBitWidth,
                              unsigned n_resources, m_axi_type mat, unsigned rwBWsize);
   void create_resource(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW, const std::string& argName_string, const std::string& interfaceType, unsigned int inputBitWidth, bool isDiffSize, const std::string& fname,
                        unsigned n_resources, unsigned alignment, bool isReal, unsigned rwBWsize);

   void ComputeResourcesAlignment(unsigned& n_resources, unsigned& alignment, unsigned int inputBitWidth, bool is_acType, bool is_signed, bool is_fixed);

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param function_id is the node id of the function analyzed.
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   interface_infer(const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~interface_infer() override;

   /**
    * Performs the loops analysis
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;
};
#endif
