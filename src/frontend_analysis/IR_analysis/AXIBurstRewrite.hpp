/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file AXIBurstRewrite.hpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef AXI_BURST_REWRITE_HPP
#define AXI_BURST_REWRITE_HPP

#include "AXIBurstAnalysis.hpp"

#include "ir_manipulation.hpp"

#include <cstdint>
#include <list>

namespace InterfaceInferInternal
{
   struct AXIBurstRewriteContext
   {
      const function_val_node* fd;
      unsigned int root_id;
      unsigned int argument_id;
      unsigned int address_bitsize;
      const std::string& argument_name;
      const std::string& bundle_name;
      const std::string& function_name;
      std::uint64_t bitwidth;
      unsigned int alignment;
      bool is_read;
      bool is_write;
      bool burst_feature_enabled;
      bool force_legacy;
      bool same_base_duplex_active;
      const FunctionArchitecture::iface_attrs& iface_attrs;
      const BundleBurstDecision* decision;
      const ir_nodeConstRef& interface_datatype;
      const application_managerRef& AppM;
      const ir_managerRef& TM;
      const ir_manipulationRef& ir_man;
      std::list<ir_nodeRef>& read_statements;
      std::list<ir_nodeRef>& write_statements;
      std::map<std::string, ir_nodeRef>& channel_read_vdefs;
      std::map<std::string, ir_nodeRef>& channel_write_vdefs;
      std::map<unsigned int, std::map<std::string, std::list<unsigned int>>>& interface_io;
      std::set<std::string>& operations_read;
      std::set<std::string>& operations_write;
      std::set<unsigned int>& modified;
      int output_level;
   };

   void ApplyAXIBurstDecision(const AXIBurstRewriteContext& context);

   ir_nodeRef createInterfaceCast(ir_nodeRef value, const ir_nodeConstRef& target_type,
                                  const ir_manipulationRef& ir_man, unsigned scope,
                                  std::vector<ir_nodeRef>& intermediate_stmts, const std::string& context);
   void serializeInterfaceAccess(const ir_nodeRef& stmt, const std::string& interface_fname,
                                 std::map<std::string, ir_nodeRef>& last_interface_vdefs,
                                 const ir_manipulationRef& ir_man);
   void preserveBlockingInterfaceAccess(const ir_nodeRef& stmt, const ir_nodeRef& original_stmt,
                                        const std::string& interface_fname,
                                        std::map<std::string, ir_nodeRef>& last_interface_vdefs,
                                        const ir_manipulationRef& ir_man);
   void LowerReadBurstRegions(const statement_list_node* sl, const function_val_node* fd,
                              const std::vector<ReadBurstRegion>& regions, const ir_nodeRef& expected_base,
                              const std::string& bundle_name, const std::string& argument_name,
                              const ir_nodeConstRef& interface_datatype, const application_managerRef& AppM,
                              const ir_managerRef& TM, const ir_manipulationRef& ir_man,
                              std::map<std::string, ir_nodeRef>& channel_read_vdefs,
                              std::map<unsigned int, std::map<std::string, std::list<unsigned int>>>& interface_io,
                              std::set<std::string>& operationsR, std::set<unsigned int>& modified);
   void LowerWriteBurstRegion(const statement_list_node* sl, const function_val_node* fd,
                              const WriteBurstRegion& region, const ir_nodeRef& expected_base,
                              const std::string& bundle_name, const std::string& argument_name,
                              const ir_nodeConstRef& interface_datatype, const application_managerRef& AppM,
                              const ir_managerRef& TM, const ir_manipulationRef& ir_man,
                              std::map<std::string, ir_nodeRef>& channel_write_vdefs,
                              std::map<unsigned int, std::map<std::string, std::list<unsigned int>>>& interface_io,
                              std::set<std::string>& operationsW, std::set<unsigned int>& modified);
   std::vector<ir_nodeRef> lowerRuntimeGuardedDuplex(
       const statement_list_node* const_sl, const function_val_node* fd, const SameBaseDuplexProof& proof,
       const std::string& bundle_name, const std::string& read_argument_name, const std::string& write_argument_name,
       const ir_nodeConstRef& interface_datatype, unsigned int address_bitsize, const application_managerRef& AppM,
       const ir_managerRef& TM, const ir_manipulationRef& ir_man, std::map<std::string, ir_nodeRef>& channel_read_vdefs,
       std::map<std::string, ir_nodeRef>& channel_write_vdefs,
       std::map<unsigned int, std::map<std::string, std::list<unsigned int>>>& interface_io);
} // namespace InterfaceInferInternal

#endif // AXI_BURST_REWRITE_HPP
