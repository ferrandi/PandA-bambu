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
 *              Copyright (C) 2025-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file BackendWrapper.cpp
 * @brief This class handles the instantiation of the backend environment based on target device information
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "BackendWrapper.hpp"

#include "BambuParameterRegistry.hpp"
#include "CompilerWrapper.hpp"
#include "HDL_manager.hpp"
#include "Parameter.hpp"
#include "ToolManager.hpp"
#include "area_info.hpp"
#include "bambu_results_xml.hpp"
#include "call_graph_manager.hpp"
#include "compiler_constants.hpp"
#include "cpu_time.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "function_behavior.hpp"
#include "generic_device.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "schedule.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"

#include "algorithms/loops_detection/loops.hpp"
#include <filesystem>
#include <map>
#include <mutex>
#include <regex>
#include <unordered_set>
#include <utility>
#include <vector>

#include "config_PANDA_LIB_INSTALLDIR.hpp"
#include <thread>

#define PARAM_clk_period_default (1.0 / 50)

namespace
{
   PANDA_REGISTER_PARAMETER("planner-context-xml", PandaParamType::Bool, "0",
                            "Enrich bambu_results.xml with per-function resource summaries, derived planner "
                            "features, loop hierarchy, and call hierarchy for downstream design-space exploration.",
                            "reporting");

   inline pugi::xml_node get_child(pugi::xml_node& node, const pugi::char_t* name)
   {
      if(const auto child = node.child(name))
      {
         return child;
      }
      return node.append_child(name);
   }

   inline pugi::xml_attribute get_attribute(pugi::xml_node& node, const pugi::char_t* name)
   {
      if(const auto attr = node.attribute(name))
      {
         return attr;
      }
      return node.append_attribute(name);
   }

   inline bool try_append_child(pugi::xml_node& node, const pugi::char_t* name, const pugi::char_t* text)
   {
      for(const auto& c : node.children(name))
      {
         if(!std::strcmp(c.text().as_string(), text))
         {
            return false;
         }
      }
      node.append_child(name).text() = text;
      return true;
   }

   void archive_previous_results_once(const std::filesystem::path& env_filename)
   {
      static std::mutex archive_mutex;
      static std::unordered_set<std::string> archived_for_run;

      const auto key = std::filesystem::absolute(env_filename).lexically_normal().string();
      const std::lock_guard<std::mutex> lock(archive_mutex);
      if(!archived_for_run.emplace(key).second || !std::filesystem::exists(env_filename))
      {
         return;
      }

      const auto stem = env_filename.stem().string();
      const auto ext = env_filename.extension().string();
      for(size_t index = 0; /**/; ++index)
      {
         const auto archived_filename = env_filename.parent_path() / (stem + "_" + std::to_string(index) + ext);
         if(std::filesystem::exists(archived_filename))
         {
            continue;
         }
         std::filesystem::rename(env_filename, archived_filename);
         return;
      }
   }

   std::string get_benchmark_name(const ParameterConstRef& parameters)
   {
      std::string name;
      if(parameters->isOption(OPT_benchmark_name))
      {
         name += parameters->getOption<std::string>(OPT_benchmark_name) + ":";
      }
      if(parameters->isOption(OPT_configuration_name))
      {
         name += parameters->getOption<std::string>(OPT_configuration_name);
      }
      if(name.empty() || !parameters->IsParameter("simple-benchmark-name") ||
         parameters->GetParameter<int>("simple-benchmark-name") == 0)
      {
         if(parameters->isOption(OPT_top_functions_names))
         {
            const auto top_symbol = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names).front();
            name += name.empty() ? top_symbol : (":" + top_symbol);
         }
      }
      return name;
   }

   std::string shell_single_quote(const std::string& value)
   {
      std::string escaped;
      escaped.reserve(value.size() + 2);
      escaped += "'";
      for(const auto c : value)
      {
         if(c == '\'')
         {
            escaped += "'\\''";
         }
         else
         {
            escaped += c;
         }
      }
      escaped += "'";
      return escaped;
   }

   bool planner_context_xml_enabled(const ParameterConstRef& parameters)
   {
      return parameters->IsParameter("planner-context-xml") && parameters->GetParameter<bool>("planner-context-xml");
   }

   std::string xml_safe_name(const std::string& prefix, const std::string& raw)
   {
      std::string safe = prefix;
      safe.reserve(prefix.size() + raw.size());
      for(const char ch : raw)
      {
         if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))
         {
            safe.push_back(ch);
         }
         else
         {
            safe.push_back('_');
         }
      }
      return safe;
   }

   void append_planner_feature(pugi::xml_node& feature_table, const hlsRef& implementation,
                               const std::string& function_name, const std::string& category, const std::string& name,
                               const unsigned int value)
   {
      auto feature = feature_table.append_child(xml_safe_name("feature_", name).c_str());
      get_attribute(feature, "function_id") = implementation->functionId;
      get_attribute(feature, "function_name") = function_name.c_str();
      get_attribute(feature, "category") = category.c_str();
      get_attribute(feature, "name") = name.c_str();
      get_attribute(feature, "value") = value;
   }

   void compute_planner_context_resources(const structural_objectRef circ, const technology_managerRef TM,
                                          std::map<std::string, unsigned int>& resources)
   {
      const module_o* mod = GetPointer<module_o>(circ);
      auto process_resources = [&](const structural_objectRef c) {
         THROW_ASSERT(c, "unexpected condition");
         const structural_type_descriptorRef id_type = c->get_typeRef();
         if(c->get_kind() != module_o_K)
         {
            return;
         }
         compute_planner_context_resources(c, TM, resources);
         if(c->get_id() == "Controller_i" || c->get_id() == "Datapath_i")
         {
            return;
         }
         const std::string library = TM->get_library(id_type->id_type);
         if(library == WORK_LIBRARY || library == PROXY_LIBRARY)
         {
            return;
         }
         resources[id_type->id_type]++;
      };
      for(unsigned int l = 0; l < mod->get_internal_objects_size(); l++)
      {
         const structural_objectRef obj = mod->get_internal_object(l);
         process_resources(obj);
      }
      const NP_functionalityRef NPF = mod->get_NP_functionality();
      if(NPF and NPF->exist_NP_functionality(NP_functionality::IP_COMPONENT))
      {
         const std::string ip_cores = NPF->get_NP_functionality(NP_functionality::IP_COMPONENT);
         const std::vector<std::string> ip_cores_list = string_to_container<std::vector<std::string>>(ip_cores, ",");
         for(const auto& ip_core : ip_cores_list)
         {
            const std::vector<std::string> ip_core_vec = string_to_container<std::vector<std::string>>(ip_core, ":");
            if(ip_core_vec.size() < 1 or ip_core_vec.size() > 2)
            {
               THROW_ERROR("Malformed IP component definition \"" + ip_core + "\"");
            }
            std::string library, component_name;
            if(ip_core_vec.size() == 2)
            {
               library = ip_core_vec[0];
               component_name = ip_core_vec[1];
            }
            else
            {
               component_name = ip_core_vec[0];
               library = TM->get_library(component_name);
            }
            const technology_nodeRef tn = TM->get_fu(component_name, library);
            structural_objectRef core_cir;
            if(tn->get_kind() == functional_unit_K)
            {
               core_cir = GetPointer<functional_unit>(tn)->CM->get_circ();
            }
            else if(tn->get_kind() == functional_unit_template_K &&
                    GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU))
            {
               core_cir = GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->CM->get_circ();
            }
            else
            {
               THROW_ERROR("Unexpected pattern");
            }
            process_resources(core_cir);
         }
      }
   }

   unsigned int count_matching_resources(const std::map<std::string, unsigned int>& resources, const std::string& token)
   {
      unsigned int total = 0;
      for(const auto& resource : resources)
      {
         if(resource.first.find(token) != std::string::npos)
         {
            total += resource.second;
         }
      }
      return total;
   }

   unsigned int count_total_resources(const std::map<std::string, unsigned int>& resources)
   {
      unsigned int total = 0;
      for(const auto& resource : resources)
      {
         total += resource.second;
      }
      return total;
   }

   void append_planner_resource_summary(pugi::xml_node& function, const hlsRef& implementation,
                                        const std::string& function_name)
   {
      if(!implementation->datapath)
      {
         return;
      }
      std::map<std::string, unsigned int> resources;
      compute_planner_context_resources(implementation->datapath->get_circ(),
                                        implementation->HLS_D->get_technology_manager(), resources);
      auto resource_summary = function.append_child("resource_summary");
      get_attribute(resource_summary, "function_id") = implementation->functionId;
      get_attribute(resource_summary, "function_name") = function_name.c_str();
      auto feature_table = get_child(function, "feature_table");
      get_attribute(feature_table, "function_id") = implementation->functionId;
      get_attribute(feature_table, "function_name") = function_name.c_str();
      for(const auto& resource : resources)
      {
         auto resource_node = resource_summary.append_child(xml_safe_name("resource_", resource.first).c_str());
         get_attribute(resource_node, "function_id") = implementation->functionId;
         get_attribute(resource_node, "function_name") = function_name.c_str();
         get_attribute(resource_node, "type") = resource.first.c_str();
         get_attribute(resource_node, "count") = resource.second;
      }

      append_planner_feature(feature_table, implementation, function_name, "resource_summary",
                             "resource.unique_type_count", static_cast<unsigned int>(resources.size()));
      append_planner_feature(feature_table, implementation, function_name, "resource_summary",
                             "resource.total_instance_count", count_total_resources(resources));

      for(const auto& family_name : {"mul_node_FU", "ARRAY_1D_STD", "MEMORY_CTRL"})
      {
         append_planner_feature(feature_table, implementation, function_name, "resource_family",
                                "resource_family." + std::string(family_name),
                                count_matching_resources(resources, family_name));
      }
   }

   void append_call_hierarchy(pugi::xml_node& function, const HLS_managerRef& hls_manager, const hlsRef& implementation,
                              const std::string& function_name)
   {
      const auto function_id = implementation->functionId;
      const auto& CGM = hls_manager->CGetCallGraphManager();
      const auto root_function_id = CGM.GetRootFunction(function_id);
      const auto root_id = root_function_id ? root_function_id : function_id;
      const auto root_name = ir_helper::GetFunctionName(hls_manager->get_ir_manager()->GetIRNode(root_id));
      const auto is_root = CGM.GetRootFunctions().count(function_id) != 0;
      const auto direct_callees = CGM.get_called_by(function_id);
      auto reachable_body_functions = CGM.GetReachedFunctionsFrom(function_id);
      reachable_body_functions.erase(function_id);

      get_attribute(function, "root_function_id") = root_id;
      get_attribute(function, "root_function_name") = root_name.c_str();
      get_attribute(function, "is_root") = is_root;

      auto feature_table = get_child(function, "feature_table");
      get_attribute(feature_table, "function_id") = implementation->functionId;
      get_attribute(feature_table, "function_name") = function_name.c_str();
      append_planner_feature(feature_table, implementation, function_name, "call_graph", "call.is_root",
                             is_root ? 1U : 0U);
      append_planner_feature(feature_table, implementation, function_name, "call_graph",
                             "call.direct_body_callee_count", static_cast<unsigned int>(direct_callees.size()));
      append_planner_feature(feature_table, implementation, function_name, "call_graph",
                             "call.descendant_body_function_count",
                             static_cast<unsigned int>(reachable_body_functions.size()));

      auto call_hierarchy = function.append_child("call_hierarchy");
      get_attribute(call_hierarchy, "function_id") = implementation->functionId;
      get_attribute(call_hierarchy, "function_name") = function_name.c_str();
      get_attribute(call_hierarchy, "root_function_id") = root_id;
      get_attribute(call_hierarchy, "root_function_name") = root_name.c_str();
      get_attribute(call_hierarchy, "is_root") = is_root;
      auto callees = call_hierarchy.append_child("callees");
      for(const auto callee_id : direct_callees)
      {
         const auto callee_name = ir_helper::GetFunctionName(hls_manager->get_ir_manager()->GetIRNode(callee_id));
         auto callee = callees.append_child(xml_safe_name("callee_", STR(callee_id)).c_str());
         get_attribute(callee, "id") = callee_id;
         get_attribute(callee, "name") = callee_name.c_str();
         get_attribute(callee, "has_body") = CGM.GetReachedBodyFunctions().count(callee_id) != 0;
         get_attribute(callee, "root_function_id") = CGM.GetRootFunction(callee_id);
      }
   }

   void append_loop_hierarchy(pugi::xml_node& function, const FunctionBehaviorConstRef& FB,
                              const hlsRef& implementation, const std::string& function_name)
   {
      const auto loops = FB->getConstLoops();
      if(!loops)
      {
         return;
      }

      const auto bb_graph = FB->GetBBGraph(FunctionBehavior::FBB);
      auto loop_hierarchy = function.append_child("loop_hierarchy");
      get_attribute(loop_hierarchy, "function_id") = implementation->functionId;
      get_attribute(loop_hierarchy, "function_name") = function_name.c_str();
      auto feature_table = get_child(function, "feature_table");
      get_attribute(feature_table, "function_id") = implementation->functionId;
      get_attribute(feature_table, "function_name") = function_name.c_str();

      unsigned int loop_count = 0;
      unsigned int max_depth = 0;
      unsigned int pipelined_loop_count = 0;

      for(const auto& loop : loops->getList())
      {
         if(!loop || loop->getLoopDepth() == 0)
         {
            continue;
         }
         ++loop_count;
         max_depth = std::max(max_depth, loop->getLoopDepth());

         auto loop_node = loop_hierarchy.append_child(xml_safe_name("loop_", STR(loop->getLoopId())).c_str());
         get_attribute(loop_node, "function_id") = implementation->functionId;
         get_attribute(loop_node, "function_name") = function_name.c_str();
         get_attribute(loop_node, "id") = loop->getLoopId();
         get_attribute(loop_node, "depth") = loop->getLoopDepth();
         get_attribute(loop_node, "reducible") = loop->isReducible();
         get_attribute(loop_node, "innermost") = loop->isInnermost();
         get_attribute(loop_node, "pipelinable") = loop->isPipelinable();
         get_attribute(loop_node, "block_count") = static_cast<unsigned int>(loop->getBlocks().size());
         const auto parent = loop->getParent();
         get_attribute(loop_node, "parent_id") = (parent && parent->getLoopDepth() != 0) ? parent->getLoopId() : 0;
         get_attribute(loop_node, "pipeline_ii") = 0;

         if(loop->isReducible())
         {
            const auto header_bb = bb_graph.CGetNodeInfo(loop->getHeader()).block->number;
            get_attribute(loop_node, "header_bb") = header_bb;
            if(implementation->Rsch && !FB->is_function_pipelined())
            {
               const auto pipeline_info = implementation->Rsch->CGetLoopPipelinedInfo();
               if(const auto it = pipeline_info.find(header_bb); it != pipeline_info.end())
               {
                  get_attribute(loop_node, "pipeline_ii") = it->second.II;
                  if(it->second.II > 0)
                  {
                     ++pipelined_loop_count;
                  }
               }
            }
         }
         else
         {
            get_attribute(loop_node, "header_bb") = 0;
         }

         auto blocks = loop_node.append_child("blocks");
         for(const auto bb_vertex : loop->getBlocks())
         {
            const auto bb_number = bb_graph.CGetNodeInfo(bb_vertex).block->number;
            auto block = blocks.append_child(xml_safe_name("bb_", STR(bb_number)).c_str());
            get_attribute(block, "id") = bb_number;
         }

         auto children = loop_node.append_child("children");
         for(const auto& child : loop->getSubLoops())
         {
            if(!child || child->getLoopDepth() == 0)
            {
               continue;
            }
            auto child_ref = children.append_child(xml_safe_name("loop_ref_", STR(child->getLoopId())).c_str());
            get_attribute(child_ref, "id") = child->getLoopId();
         }
      }

      for(const auto& entry : {std::pair<std::string, unsigned int>{"loop.count", loop_count},
                               std::pair<std::string, unsigned int>{"loop.max_depth", max_depth},
                               std::pair<std::string, unsigned int>{"loop.pipelined_count", pipelined_loop_count}})
      {
         append_planner_feature(feature_table, implementation, function_name, "loop", entry.first, entry.second);
      }
   }
} // namespace

BackendWrapper::BackendWrapper(const ParameterConstRef& _Param, const generic_deviceRef& _device,
                               const std::vector<std::string>& _backend_ids)
    : Param(_Param),
      device(_device),
      backend_ids(_backend_ids),
      debug_level(Param->get_class_debug_level(GET_CLASS(*this))),
      output_level(_Param->getOption<int>(OPT_output_level)),
      backend_launch_filename()
{
}

void BackendWrapper::init(const HLS_managerRef& HLSMgr)
{
   std::list<std::string> hdl_files;

   const auto top_symbols = Param->getOption<std::vector<std::string>>(OPT_top_functions_names);
   THROW_ASSERT(top_symbols.size() == 1, "Expected single top function name");
   const auto top_fnode = HLSMgr->get_ir_manager()->GetFunction(top_symbols.front());
   const auto top_hls_name =
       HDL_manager::convert_to_identifier(HLSMgr->get_HLS(top_fnode->index)->top->get_circ()->get_id());
   const auto top_fu_name = ir_helper::GetFunctionName(top_fnode);
   hdl_files.insert(hdl_files.end(), HLSMgr->hdl_files.begin(), HLSMgr->hdl_files.end());
   hdl_files.insert(hdl_files.end(), HLSMgr->aux_files.begin(), HLSMgr->aux_files.end());

   init(top_fu_name, top_hls_name, hdl_files, HLSMgr);
}

void BackendWrapper::init(const std::string& top_fu_name, const structural_managerRef SM,
                          const std::list<std::string>& hdl_files)
{
   init(top_fu_name, HDL_manager::convert_to_identifier(SM->get_circ()->get_id()), hdl_files);
}

void BackendWrapper::init(const std::string& top_fu_name, const std::string& top_hls_name,
                          const std::list<std::string>& hdl_files, const HLS_managerRef& hls_manager)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Generating backend scripts");
   const auto out_dir = Param->getOption<std::filesystem::path>(OPT_output_directory);
   const auto out_hls_dir = Param->getOption<std::filesystem::path>(OPT_output_hls_directory);
   const auto sim_dir = out_hls_dir / "simulation";

   pugi::xml_document config_xml = LoadResults(Param);
   auto application = config_xml.child("application");
   get_attribute(application, "args") = Param->getOption<std::string>(OPT_cat_args).c_str();
   get_attribute(application, "version") = Param->PrintVersion().c_str();
   get_attribute(application, "timestamp") = TimeStamp::GetCurrentTimeStamp().c_str();
   get_attribute(application, "benchmark") = get_benchmark_name(Param).c_str();
   get_attribute(application, "verbosity") = Param->getOption<int>(OPT_output_level);
   // get_attribute(application, "HLS_execution_time") = HLSMgr->HLS_execution_time / 1000;

   auto sources = get_child(application, "sources");
   if(Param->isOption(OPT_default_compiler))
   {
      CompilerWrapper compiler(Param, Param->getOption<CompilerWrapper_CompilerTarget>(OPT_default_compiler));
      auto cflags = compiler.GetCompilerParameters("");
      std::cmatch what;
      std::string kill_printf;
      if(std::regex_search(cflags.c_str(), what, std::regex("\\s*(\\-D'?printf[^=]*='?)'*")))
      {
         kill_printf.append(what[1].first, what[1].second);
         cflags.erase(static_cast<size_t>(what[0].first - cflags.c_str()),
                      static_cast<size_t>(what[0].second - what[0].first));
      }
      // Bash variable is initialized using double quotes, thus will evaluate escaped characters. Then, it is necessary
      // to add a further escaping to retain original string semantics. (i.e. \"var\" -> CFLAGS="\"var\"" -> "var",
      // while we need it to still be \"var\", thus we need to write CFLAGS="\\\"var\\\"")
      cflags = std::regex_replace(cflags, std::regex("(\\\\(\"|\\\\|n))"), "\\\\$1");

      get_attribute(sources, "compiler") = compiler.GetCompiler().cc.c_str();
      get_attribute(sources, "cflags") = cflags.c_str();
   }
   if(Param->isOption(OPT_input_format) &&
      Param->getOption<Parameters_FileFormat>(OPT_input_format) != Parameters_FileFormat::FF_RAW)
   {
      const auto in_files = string_to_container<std::vector<std::filesystem::path>>(
          Param->getOption<std::string>(OPT_input_file), STR_CST_string_separator);
      for(const auto& ifile : in_files)
      {
         if(ifile.filename() != std::filesystem::path(STR_CST_libopenmp_filename).filename())
         {
            try_append_child(sources, "file", std::filesystem::proximate(ifile, out_dir).c_str());
         }
      }
   }

   if(Param->isOption(OPT_generate_testbench) && Param->getOption<bool>(OPT_generate_testbench))
   {
      auto testbench = get_child(application, "testbench");
      get_attribute(testbench, "top_module") =
          (Param->isOption(OPT_top_design_name) ? Param->getOption<std::string>(OPT_top_design_name) : top_hls_name)
              .c_str();
      get_attribute(testbench, "symbol") = top_fu_name.c_str();
      get_attribute(testbench, "m_symbol") = cxa_prefix_mangled(top_fu_name, "__m_").c_str();
      get_attribute(testbench, "m_pp_symbol") = cxa_prefix_mangled(top_fu_name, "__m_pp_").c_str();
      get_attribute(testbench, "cflags") = Param->isOption(OPT_tb_extra_cc_options) ?
                                               Param->getOption<std::string>(OPT_tb_extra_cc_options).c_str() :
                                               "";
      if(Param->isOption(OPT_testbench_input_file) &&
         is_elf(Param->getOption<std::filesystem::path>(OPT_testbench_input_file)))
      {
         get_attribute(testbench, "elf") =
             std::filesystem::proximate(Param->getOption<std::string>(OPT_testbench_input_file), out_dir).c_str();
      }
      else if(Param->isOption(OPT_testbench_input_file) || Param->isOption(OPT_testbench_input_string))
      {
         get_attribute(testbench, "elf") = std::filesystem::proximate(sim_dir / "testbench", out_dir).c_str();
      }
      else
      {
         get_attribute(testbench, "elf") = "";
      }
      const auto generated_tb = std::filesystem::proximate(sim_dir, out_dir) / "generated_tb.c";
      if(Param->isOption(OPT_testbench_input_file))
      {
         const auto tb_files = Param->getOption<std::vector<std::string>>(OPT_testbench_input_file);
         if(!is_elf(tb_files.front()))
         {
            for(const auto& filename : tb_files)
            {
               if(ends_with(filename, ".xml"))
               {
                  try_append_child(testbench, "file", generated_tb.c_str());
               }
               else
               {
                  try_append_child(testbench, "file", std::filesystem::proximate(filename, out_dir).c_str());
               }
            }
         }
      }
      else if(Param->isOption(OPT_testbench_input_string))
      {
         try_append_child(testbench, "file", generated_tb.c_str());
      }
      if(Param->isOption(OPT_no_parse_files))
      {
         const auto np_files = string_to_container<std::vector<std::filesystem::path>>(
             Param->getOption<std::string>(OPT_no_parse_files), STR_CST_string_separator);
         for(const auto& np_file : np_files)
         {
            try_append_child(testbench, "file", std::filesystem::proximate(np_file, out_dir).c_str());
         }
      }
   }

   auto outputs = get_child(application, "outputs");
   for(const auto& hdl_file : hdl_files)
   {
      const auto filename = std::filesystem::proximate(hdl_file, out_dir);
      try_append_child(outputs, "file", filename.c_str());
   }

   auto top_module = get_child(application, "top_module");
   const auto fu = [&]() -> const functional_unit* {
      const auto TM = device->get_technology_manager();
      const auto resource_name = "_" + top_fu_name;
      if(auto library = TM->get_library(resource_name); library.size())
      {
         return GetPointerS<const functional_unit>(TM->get_fu(resource_name, library));
      }
      if(auto library = TM->get_library(top_fu_name); library.size())
      {
         return GetPointerS<const functional_unit>(TM->get_fu(top_fu_name, library));
      }
      return nullptr;
   }();
   get_attribute(top_module, "name") = top_hls_name.c_str();
   get_attribute(top_module, "clock_name") =
       Param->isOption(OPT_clock_name) ? Param->getOption<std::string>(OPT_clock_name).c_str() : CLOCK_PORT_NAME;
   get_attribute(top_module, "combinational") = fu && fu->logical_type == functional_unit::COMBINATIONAL;

   if(Param->isOption(OPT_VHDL_library))
   {
      auto vhdl_library = get_child(application, "vhdl_library");
      get_attribute(vhdl_library, "sources") = Param->getOption<std::string>(OPT_VHDL_library).c_str();
   }

   auto target = get_child(application, "target");
   const auto is_time_unit_PS =
       device->has_parameter("USE_TIME_UNIT_PS") && device->get_parameter<int>("USE_TIME_UNIT_PS") == 1;
   auto clk_period =
       Param->isOption(OPT_clock_period) ? Param->getOption<double>(OPT_clock_period) : PARAM_clk_period_default;
   if(fu && fu->get_clock_period() != 0.0)
   {
      clk_period = fu->get_clock_period();
   }
   get_attribute(target, "period") = (is_time_unit_PS ? 1000.0 : 1.0) * clk_period;
   get_attribute(target, "period_ps") = 1000.0 * clk_period;
   get_attribute(target, "frequency") = (is_time_unit_PS ? 1000.0 : 1.0) * 1000.0 / clk_period;
   get_attribute(target, "connect_iob") = Param->isOption(OPT_connect_iob) && Param->getOption<bool>(OPT_connect_iob);
   for(const auto c : {"vendor", "family", "model", "package", "speed_grade"})
   {
      get_attribute(target, c) = device->get_parameter<std::string>(c).c_str();
   }

   auto backend = get_child(application, "backend");
   get_attribute(backend, "bambu_results") = BAMBU_RESULTS_FILENAME;
   get_attribute(backend, "sdc_ext_file") = Param->isOption(OPT_backend_sdc_extensions) ?
                                                Param->getOption<std::string>(OPT_backend_sdc_extensions).c_str() :
                                                "";
   get_attribute(backend, "fast") = Param->IsParameter("fast-backend") && Param->GetParameter<bool>("fast-backend");
   get_attribute(backend, "assert") = Param->isOption(OPT_assert_debug) && Param->getOption<bool>(OPT_assert_debug);
   const auto vcd_filename = std::filesystem::proximate(sim_dir, out_dir) / "test.vcd";
   get_attribute(backend, "vcd") =
       Param->isOption(OPT_generate_vcd) && Param->getOption<bool>(OPT_generate_vcd) ? vcd_filename.c_str() : "";
   get_attribute(backend, "discrepancy") = Param->isOption(OPT_discrepancy) && Param->getOption<bool>(OPT_discrepancy);
   if(Param->isOption(OPT_parallel_backend))
   {
      get_attribute(backend, "parallel") = Param->getOption<std::string>(OPT_parallel_backend).c_str();
   }

   if(hls_manager)
   {
      if(const auto old_hls_results = application.child("hls_results"))
      {
         application.remove_child(old_hls_results);
      }
      auto hls_results = application.append_child("hls_results");
      get_attribute(hls_results, "planner_context_xml") = planner_context_xml_enabled(Param);
      for(const auto& implementation : hls_manager->GetAllImplementations())
      {
         const auto function_id = implementation->functionId;
         const auto function_name = ir_helper::GetFunctionName(hls_manager->get_ir_manager()->GetIRNode(function_id));
         const auto FB = hls_manager->CGetFunctionBehavior(function_id);
         auto function = hls_results.append_child(("function_" + STR(function_id)).c_str());
         get_attribute(function, "id") = function_id;
         get_attribute(function, "name") = function_name.c_str();
         get_attribute(function, "function_pipelined") = FB->is_function_pipelined();
         get_attribute(function, "function_ii") = FB->is_function_pipelined() ? FB->get_initiation_time() : 0;

         if(implementation->Rsch && !FB->is_function_pipelined())
         {
            for(const auto& [bb_index, info] : implementation->Rsch->CGetLoopPipelinedInfo())
            {
               auto loop = function.append_child(("loop_" + STR(bb_index)).c_str());
               get_attribute(loop, "bb") = bb_index;
               get_attribute(loop, "ii") = info.II;
            }
         }
         if(planner_context_xml_enabled(Param))
         {
            append_planner_resource_summary(function, implementation, function_name);
            append_call_hierarchy(function, hls_manager, implementation, function_name);
            append_loop_hierarchy(function, FB, implementation, function_name);
         }
      }
   }

   StoreResults(config_xml, Param);

   backend_launch_filename = out_dir / ("evaluate_" + top_fu_name + ".sh");
   std::ofstream script(backend_launch_filename);
   const auto bambu_hls_root = relocate_install_path(PANDA_LIB_INSTALLDIR).parent_path().parent_path();
   script << "#!/usr/bin/env bash\n"
             "##########################################################\n"
             "#     Automatically generated by the PandA framework     #\n"
             "##########################################################\n"
             "if [ -n \"$APPDIR\" ]; then\n"
             "  export LD_LIBRARY_PATH=`sed -E \"s,${APPDIR}/[^\\:]+[\\:^](\\:|\\$),,g\" <<< $LD_LIBRARY_PATH`\n"
             "fi\n"
             "SWD=`dirname $(readlink -e $0)`\n"
          << ": ${BAMBU_HLS:=\"" << bambu_hls_root.string() << "\"}\n"
          << "BAMBU_HLS_OUTDIR=\"${SWD}\"\n"
             "BAMBU_HLS_RESULTS=\"${BAMBU_HLS_OUTDIR}/bambu_results.xml\"\n"
             "bambu_results() { xmlq \"${BAMBU_HLS_RESULTS}\" \"$1\"; }\n"
             "export BAMBU_HLS\n"
             "export BAMBU_HLS_OUTDIR\n"
             "export BAMBU_HLS_RESULTS\n"
             "export -f bambu_results\n"
             "export PATH=\"${BAMBU_HLS_OUTDIR}/HLS_output/utils:${PATH}\"\n"
             "\n"
             "BACKENDS=()\n";

   for(const auto& [var_name, var_value] : device->get_device_bash_vars())
   {
      script << "export " << var_name << "=" << shell_single_quote(var_value) << "\n";
   }
   script << "\n";

   const auto lib_backend_dir = relocate_install_path(PANDA_LIB_INSTALLDIR "/libtech/backend");
   const auto out_utils_dir = out_hls_dir / "utils";
   std::filesystem::create_directories(out_utils_dir);
   std::filesystem::copy(lib_backend_dir / "utils", out_utils_dir,
                         std::filesystem::copy_options::recursive | std::filesystem::copy_options::update_existing);

   const auto backend_setup = [&](const std::string& backend_id) {
      const auto backend_env = device->has_backend(backend_id) ? device->get_backend(backend_id) : backend_id;
      const auto backend_env_directory = lib_backend_dir / backend_env;
      if(!std::filesystem::exists(backend_env_directory))
      {
         THROW_ERROR_USAGE("Unknown backend flow: " + backend_env +
                           ". Check device/backend configuration and selected backend options.");
      }
      const auto backend_outdir = out_hls_dir / (backend_env + "_backend");
      std::filesystem::create_directories(backend_outdir);
      std::filesystem::copy(backend_env_directory, backend_outdir,
                            std::filesystem::copy_options::recursive | std::filesystem::copy_options::update_existing);

      const auto backend_specific_launch = backend_outdir / "launch.sh";
      std::filesystem::permissions(backend_specific_launch,
                                   std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                       std::filesystem::perms::others_exec,
                                   std::filesystem::perm_options::add);
      return "BACKENDS+=(\"" + std::filesystem::proximate(backend_specific_launch, out_dir).string() + "\")\n";
   };

   if(device->has_backend("env"))
   {
      script << backend_setup("env");
   }

   for(const auto& backend_id : backend_ids)
   {
      const auto is_muted = starts_with(backend_id, "#");
      if(is_muted)
      {
         script << "# " << backend_setup(backend_id.substr(1));
      }
      else
      {
         script << backend_setup(backend_id);
      }
   }

   script << "\n"
             "cd \"${SWD}\"\n"
             "set -e\n"
             "for backend in ${BACKENDS[@]};\n"
             "do\n"
             "  backend_dir=`dirname ${backend}`\n"
             "  backend_log=\"${backend_dir}/launch.log\"\n"
             "  step_id=`basename $(dirname ${backend})`\n"
             "  step_id=\"${step_id%%_backend}\"\n"
             "  step_id=\"${step_id//_/ }\"\n"
             "  step_results=\"${backend_dir}/`basename ${BAMBU_HLS_RESULTS}`\"\n"
             "  echo \"BACKEND STEP ${step_id}\"\n"
             "  (./${backend} \"$@\" 2>&1 | tee \"${backend_log}\"; exit ${PIPESTATUS[0]})\n"
             "  if [ -f \"${step_results}\" ]; then\n"
             "    xmlmerge -i \"${BAMBU_HLS_RESULTS}\" \"${step_results}\"\n"
             "  fi\n"
             "done\n"
             "\n";

   script.close();
   std::filesystem::permissions(backend_launch_filename,
                                std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                    std::filesystem::perms::others_exec,
                                std::filesystem::perm_options::add);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Generated backend scripts");
}

pugi::xml_document BackendWrapper::run() const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Executing backend");

   ToolManager tool(Param);
   THROW_ASSERT(!backend_launch_filename.empty(), "Backend script not yet generated.");
   tool.configure(backend_launch_filename, "");

   std::vector<std::string> parameters, input_files, output_files;
   if(Param->isOption(OPT_testbench_argv))
   {
      parameters = Param->getOption<std::vector<std::string>>(OPT_testbench_argv);
   }
   const std::string backend_file_output =
       Param->getOption<std::string>(OPT_output_temporary_directory) + "/backend_output";
   (void)tool.execute(parameters, input_files, output_files, backend_file_output);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Executed backend");
   return LoadResults(Param);
}

std::string BackendWrapper::get_flow_name() const
{
   return device->get_parameter<std::string>("");
}

pugi::xml_document BackendWrapper::LoadResults(const ParameterConstRef& Param)
{
   const auto env_filename = Param->getOption<std::filesystem::path>(OPT_output_directory) / BAMBU_RESULTS_FILENAME;
   archive_previous_results_once(env_filename);
   pugi::xml_document res;
   if(!std::filesystem::exists(env_filename))
   {
      res.append_child("application");
   }
   else
   {
      const auto parser = res.load_file(env_filename.c_str());
      if(parser.status == pugi::xml_parse_status::status_file_not_found)
      {
         THROW_WARNING("Backend results XML file not found: " + env_filename.string());
      }
      else if(parser.status != pugi::xml_parse_status::status_ok)
      {
         THROW_ERROR("Invalid backend results XML format.");
      }
   }
   return res;
}

void BackendWrapper::StoreResults(const pugi::xml_document& res, const ParameterConstRef& Param)
{
   const auto env_filename = Param->getOption<std::filesystem::path>(OPT_output_directory) / BAMBU_RESULTS_FILENAME;
   if(!res.save_file(env_filename.c_str(), "\t", pugi::format_indent_attributes))
   {
      THROW_ERROR("Unable to save bambuhls.env.xml file.");
   }
}

void BackendWrapper::ParseResults(const pugi::xml_document& res, area_info& resource_info, time_info& timing_info)
{
   const auto app = res.child("application");
   const auto resources = app.child("resources");
   for(const auto& attr : resources.attributes())
   {
      const auto res_key = area_info::to_resource_type(attr.name());
      if(res_key != area_info::ERROR)
      {
         resource_info.resources[res_key] = attr.as_double();
      }
   }

   const auto timing = app.child("timing");
   timing_info.set_execution_time(resources.attribute("DELAY").as_double(), timing.attribute("LATENCY").as_uint());
}
