/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2022-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
#include "custom_set.hpp"
#include "refcount.hpp"

#include <list>
#include <map>
#include <set>

REF_FORWARD_DECL(ir_node);
CONSTREF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(FunctionArchitecture);
struct statement_list_node;
struct function_val_node;
struct assign_stmt;
struct node_stmt;

class InterfaceInfer : public ApplicationFrontendFlowStep
{
 private:
   enum class m_axi_type;
   enum class datatype;
   struct interface_info;

   bool already_executed;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;

   void ChasePointerInterfaceRecurse(CustomOrderedSet<unsigned>& Visited, ir_nodeRef ptr_var,
                                     std::list<ir_nodeRef>& writeStmt, std::list<ir_nodeRef>& readStmt,
                                     interface_info& info);

   void ChasePointerInterface(ir_nodeRef ptr_var, std::list<ir_nodeRef>& writeStmt, std::list<ir_nodeRef>& readStmt,
                              interface_info& info);

   void forwardInterface(const ir_nodeRef& fnode, const ir_nodeRef& parm_node, const interface_info& info);

   void setReadInterface(ir_nodeRef stmt, const std::string& arg_name, std::set<std::string>& operationsR,
                         ir_nodeConstRef interface_datatype, std::map<std::string, ir_nodeRef>& channel_read_vdefs,
                         const ir_manipulationRef ir_man, const ir_managerRef TM);

   void setWriteInterface(ir_nodeRef stmt, const std::string& arg_name, std::set<std::string>& operationsW,
                          ir_nodeConstRef interface_datatype, std::map<std::string, ir_nodeRef>& channel_write_vdefs,
                          const ir_manipulationRef ir_man, const ir_managerRef TM);

   void create_resource_Read_simple(const std::set<std::string>& operations, const interface_info& info,
                                    FunctionArchitectureRef func_arch, bool IO_port, bool unused_port,
                                    unsigned int root_id) const;

   void create_resource_Write_simple(const std::set<std::string>& operations, const interface_info& info,
                                     FunctionArchitectureRef func_arch, bool IO_port) const;

   void create_resource_array(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW,
                              const interface_info& info, FunctionArchitectureRef func_arch, bool unused_port,
                              unsigned int root_id) const;

   void create_resource_m_axi(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW,
                              const interface_info& info, FunctionArchitectureRef func_arch, bool unused_port,
                              unsigned root_id) const;

   void create_resource(const std::set<std::string>& operationsR, const std::set<std::string>& operationsW,
                        const interface_info& info, FunctionArchitectureRef func_arch, bool unused_port,
                        unsigned int root_id) const;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of the parameters
    */
   InterfaceInfer(const application_managerRef AppM, const DesignFlowManager& design_flow_manager,
                  const ParameterConstRef parameters);

   bool HasToBeExecuted() const override;

   /**
    * Execute this step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};

#endif
