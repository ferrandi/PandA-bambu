/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file AXIBurstAnalysis.hpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef AXI_BURST_ANALYSIS_HPP
#define AXI_BURST_ANALYSIS_HPP

#include "HLS/axi_interface_profile.hpp"
#include "HLS/hls_manager.hpp"
#include "application_manager.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "custom_map.hpp"
#include "ir_basic_block.hpp"
#include "ir_node.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

#define ENCODE_FDNAME(arg_name, MODE, interface_type) \
   ((arg_name) + STR_CST_interface_parameter_keyword + (MODE) + (interface_type))

namespace InterfaceInferInternal
{
   struct OriginalMemoryAccess
   {
      ir_nodeRef statement;
      ir_nodeRef base;
      unsigned int owner_argument_index;
      std::string bundle;
      bool is_write;
      bool ordinary;
      bool classified;
   };

   struct ReadBurstRegion
   {
      unsigned int preheader;
      unsigned int header;
      ir_nodeRef branch;
      ir_nodeRef guard;
      ir_nodeRef bound;
      ir_nodeRef loop_trip_bound;
      ir_nodeRef loop_trip_compare;
      ir_nodeRef effective_bound;
      ir_nodeRef effective_compare;
      unsigned int access_bb = 0;
      bool has_direction_guard = false;
      ir_nodeRef base;
      ir_nodeRef induction;
      std::vector<ir_nodeRef> reads;
   };

   struct WriteBurstRegion
   {
      unsigned int preheader;
      unsigned int header;
      unsigned int exit;
      ir_nodeRef guard;
      ir_nodeRef bound;
      ir_nodeRef loop_trip_bound;
      ir_nodeRef loop_trip_compare;
      ir_nodeRef effective_bound;
      ir_nodeRef effective_compare;
      unsigned int access_bb = 0;
      bool has_direction_guard = false;
      ir_nodeRef base;
      ir_nodeRef induction;
      ir_nodeRef store;
   };

   struct SameBaseDuplexProof
   {
      unsigned int owner_argument_index = 0;
      unsigned int read_owner_argument_index = 0;
      unsigned int write_owner_argument_index = 0;
      bool distinct_bases = false;
      bool runtime_range_guard = false;
      std::string bundle;
      ir_nodeRef base;
      ir_nodeRef read_base;
      ir_nodeRef write_base;
      ir_nodeRef load_statement;
      ir_nodeRef store_statement;
      ReadBurstRegion read_region{};
      WriteBurstRegion write_region{};
   };

   struct BundleBurstDecision
   {
      struct DirectionalAccesses
      {
         std::vector<OriginalMemoryAccess> reads;
         std::vector<OriginalMemoryAccess> writes;
      };
      bool read_requested = false;
      bool write_requested = false;
      bool read_recognized = false;
      bool write_recognized = false;
      bool bundle_has_memory_read = false;
      bool bundle_has_memory_write = false;
      std::string read_fallback_reason;
      std::string write_fallback_reason;
      bool same_base_duplex_candidate = false;
      std::string same_base_duplex_reason = "read/write profile not requested";
      bool same_base_duplex_profile_eligible = false;
      std::string same_base_duplex_profile_reason = "read/write profile not requested";
      bool same_base_duplex_activated = false;
      std::string same_base_duplex_activation_reason = "bundle is not in the frozen activation plan";
      std::unique_ptr<SameBaseDuplexProof> same_base_duplex_proof;
      AXIReadBurstProfile read_profile{};
      AXIWriteBurstProfile write_profile{};
      unsigned int read_max_burst_length = 0;
      std::map<unsigned int, DirectionalAccesses> accesses_by_argument;
      std::map<unsigned int, std::vector<ReadBurstRegion>> read_regions;
      std::map<unsigned int, WriteBurstRegion> write_regions;
   };

   struct FrozenDuplexContribution
   {
      unsigned int root_id = 0;
      unsigned int owner_argument_index = 0;
      std::string bundle;
      SameBaseDuplexProof proof;
      AXIReadBurstProfile read_profile{};
      AXIWriteBurstProfile write_profile{};
      unsigned int read_max_burst_length = 0;
      unsigned int address_bitsize = 0;
      unsigned int data_bitsize = 32;
      std::uint32_t latency = 0;
   };

   void PreanalyzeBursts(bool burst_feature_enabled, unsigned int effective_burst_type, unsigned int root_id,
                         const function_val_node* fd, const FunctionArchitectureRef& func_arch,
                         const std::vector<OriginalMemoryAccess>& original_memory_accesses,
                         const std::vector<OriginalMemoryAccess>& burst_memory_accesses, bool function_effects_safe,
                         bool original_memory_effects_safe,
                         const std::map<unsigned int, std::map<std::string, FrozenDuplexContribution>>& frozen_by_root,
                         const std::map<std::string, std::string>& bundle_vetoes, const application_managerRef& AppM,
                         const ir_managerRef& TM, std::map<std::string, BundleBurstDecision>& burst_decisions,
                         int debug_level);

   bool IsDataflowArchitecture(const FunctionArchitectureRef& arch);
   bool GetDuplexContribution(unsigned int root_id, const FunctionArchitectureRef& arch, const std::string& bundle,
                              bool burst_feature_enabled, unsigned int effective_burst_type,
                              unsigned int address_bitsize, const application_managerRef& AppM, const ir_managerRef& TM,
                              const CallGraphManager& CGM, FrozenDuplexContribution& contribution, std::string& reason);

   bool hasOnlyOrdinaryReadEffects(const statement_list_node* sl,
                                   const CustomOrderedSet<unsigned int>& function_memory);
   bool hasOnlyOrdinaryMemoryEffects(const statement_list_node* sl);
   std::vector<OriginalMemoryAccess> CollectOriginalMemoryAccesses(const function_val_node* fd, unsigned int root_id,
                                                                   const FunctionArchitectureRef& func_arch,
                                                                   const application_managerRef& AppM,
                                                                   const ir_managerRef& TM, bool& all_classified);
   bool RecognizeSameBaseDuplex(const statement_list_node* sl, const function_val_node* fd, unsigned int root_id,
                                const std::string& bundle_name,
                                const std::vector<OriginalMemoryAccess>& original_accesses,
                                const std::vector<OriginalMemoryAccess>& burst_accesses, bool original_effects_safe,
                                const application_managerRef& AppM, const ir_managerRef& TM,
                                const FunctionArchitectureRef& func_arch, SameBaseDuplexProof& proof,
                                std::string& reason);
   bool isConstantValue(const ir_nodeRef& value, long long expected);
   bool isUnsigned32Type(const ir_nodeConstRef& type);
   bool hasBlockPath(const statement_list_node* sl, unsigned int source, unsigned int target);
   std::string get_decl_name(ir_nodeRef tn);
   bool recognizeReadBurstRegion(const statement_list_node* sl, const std::vector<ir_nodeRef>& reads,
                                 const ir_nodeRef& expected_base,
                                 const std::vector<OriginalMemoryAccess>& original_memory_accesses,
                                 const std::string& expected_bundle, ReadBurstRegion& result,
                                 const ir_nodeRef& paired_store = ir_nodeRef());
   bool recognizeWriteBurstRegion(const statement_list_node* sl, const std::vector<ir_nodeRef>& stores,
                                  const ir_nodeRef& expected_base,
                                  const std::vector<OriginalMemoryAccess>& original_memory_accesses,
                                  const std::string& expected_bundle, WriteBurstRegion& result,
                                  const ir_nodeRef& paired_read = ir_nodeRef());
   struct AXIBurstApplicationPlan
   {
      std::map<unsigned int, std::map<std::string, FrozenDuplexContribution>> frozen_by_root;
      std::map<std::string, FrozenDuplexContribution> frozen_by_bundle;
      std::map<std::string, std::string> bundle_vetoes;
      std::set<std::string> force_legacy_bundles;
   };

   AXIBurstApplicationPlan AnalyzeAXIBurstApplication(const std::vector<unsigned int>& sorted_roots,
                                                      bool burst_feature_enabled, unsigned int effective_burst_type,
                                                      unsigned int address_bitsize,
                                                      const ModuleArchitectureRef& module_arch,
                                                      const application_managerRef& AppM, const ir_managerRef& TM,
                                                      const CallGraphManager& CGM);
} // namespace InterfaceInferInternal

#endif // AXI_BURST_ANALYSIS_HPP
