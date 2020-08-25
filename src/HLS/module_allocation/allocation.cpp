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
 * @file allocation.cpp
 * @brief Wrapper for technology used by the high-level synthesis flow.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "allocation.hpp"
#include "HDL_manager.hpp"            // for structural_managerRef, langu...
#include "NP_functionality.hpp"       // for NP_functionalityRef
#include "Parameter.hpp"              // for ParameterConstRef
#include "allocation_information.hpp" // for technology_nodeRef, node_kin...
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "clb_model.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp" // for HAVE_EXPERIMENTAL
#include "config_HAVE_FLOPOCO.hpp"      // for HAVE_FLOPOCO
#include "cpu_time.hpp"                 // for START_TIME, STOP_TIME
#include "dbgPrintHelper.hpp"           // for DEBUG_LEVEL_VERY_PEDANTIC
#include "design_flow_step_factory.hpp" // for DesignFlowManagerConstRef
#include "exceptions.hpp"               // for THROW_ASSERT, THROW_ERROR
#include "functions.hpp"
#include "hls.hpp"                   // for HLS_constraintsRef
#include "hls_constraints.hpp"       // for ENCODE_FU_LIB
#include "hls_flow_step_factory.hpp" // for HLS_managerRef
#include "hls_target.hpp"
#include "language_writer.hpp"
#include "library_manager.hpp" // for library_managerRef, library_...
#include "memory.hpp"
#include "memory_allocation.hpp"
#include "moduleGenerator.hpp"     // for structural_objectRef, struct...
#include "op_graph.hpp"            // for STORE, ADDR_EXPR, ASSERT_EXPR
#include "schedule.hpp"            // for FunctionBehaviorConstRef
#include "string_manipulation.hpp" // for STR GET_CLASS
#include "structural_manager.hpp"
#include "structural_objects.hpp" // for PROXY_PREFIX, module, CLOCK_...
#include "target_device.hpp"
#include "technology_manager.hpp" // for PROXY_LIBRARY, WORK_LIBRARY
#include "technology_node.hpp"    // for functional_unit, operation
#include "time_model.hpp"         // for ParameterConstRef
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp" // for GET_NODE, GET_INDEX_NODE
#include "tree_reindex.hpp"
#include "typed_node_info.hpp" // for GET_NAME, ENTRY, EXIT, GET_TYPE
#include "utility.hpp"         // for INFINITE_UINT, ASSERT_PARAMETER
#include <cstddef>             // for size_t
#include <limits>              // for numeric_limits

/// STL includes
#include <algorithm>
#include <tuple>
#include <utility>
#include <vector>

#include "custom_map.hpp"
#include "custom_set.hpp"

static bool is_other_port(const structural_objectRef& port)
{
   const auto p = GetPointer<port_o>(port);
   return p->get_is_memory() or p->get_is_global() or p->get_is_extern() or p->get_port_interface() != port_o::port_interface::PI_DEFAULT;
}

static bool is_a_skip_operation(const std::string& op_name)
{
   if(op_name == "mult_expr" || op_name == "widen_mult_expr" || op_name == "dot_prod_expr")
      return true;
   else
      return false;
}

static inline std::string encode_op_type(const std::string& op_name, const std::string& fu_supported_types)
{
   return op_name + ":" + fu_supported_types;
}

static inline std::string encode_op_type_prec(const std::string& op_name, const std::string& fu_supported_types, node_kind_prec_infoRef node_info)
{
   std::string op_type = encode_op_type(op_name, fu_supported_types);
   const size_t n_ins = node_info->input_prec.size();
   for(size_t ind = 0; ind < n_ins; ++ind)
   {
      if(node_info->base128_input_nelem[ind] == 0)
         op_type += ":" + STR(node_info->input_prec[ind]);
      else
         op_type += ":" + STR(node_info->input_prec[ind]) + ":" + STR(node_info->base128_input_nelem[ind]);
   }
   if(node_info->base128_output_nelem == 0)
      op_type += ":" + STR(node_info->output_prec);
   else
      op_type += ":" + STR(node_info->output_prec) + ":" + STR(node_info->base128_output_nelem);
   return op_type;
}

allocation::allocation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

allocation::~allocation() = default;

void allocation::Initialize()
{
   HLSFunctionStep::Initialize();
   if(HLS->allocation_information)
   {
      HLS->allocation_information->Clear();
   }
   else
   {
      HLS->allocation_information = AllocationInformationRef(new AllocationInformation(HLSMgr, funId, parameters));
   }
   HLS->allocation_information->Initialize();
   allocation_information = HLS->allocation_information;
   fu_list.clear();
   HLS_T = HLSMgr->get_HLS_target();
   TM = HLS_T->get_technology_manager();
}

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> allocation::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         if(not parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_BIT_VALUE, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::WHOLE_APPLICATION));
         }
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::CALLED_FUNCTIONS));
         ret.insert(std::make_tuple(HLSFlowStep_Type::INITIALIZE_HLS, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

technology_nodeRef allocation::extract_bambu_provided(const std::string& library_name, operation* curr_op, const std::string& bambu_provided_resource_)
{
   technology_nodeRef current_fu;
   std::string function_name;
   bool build_proxy = false;
   bool build_wrapper = false;
   const auto bambu_provided_resource = functions::get_function_name_cleaned(bambu_provided_resource_);
   if(HLSMgr->Rfuns->is_a_proxied_function(bambu_provided_resource))
   {
      if(HLSMgr->Rfuns->is_a_shared_function(funId, bambu_provided_resource))
      {
         function_name = WRAPPED_PROXY_PREFIX + bambu_provided_resource;
         build_wrapper = true;
      }
      else
      {
         THROW_ASSERT(HLSMgr->Rfuns->is_a_proxied_shared_function(funId, bambu_provided_resource), "expected a proxy module");
         function_name = PROXY_PREFIX + bambu_provided_resource;
         build_proxy = true;
      }
   }
   else
   {
      function_name = bambu_provided_resource;
   }
   const library_managerRef libraryManager = TM->get_library_manager(library_name);
   THROW_ASSERT(libraryManager->is_fu(function_name), "functional unit not yet synthesized: " + function_name + "(" + library_name + ")");
   current_fu = libraryManager->get_fu(function_name);
   THROW_ASSERT(current_fu, "functional unit not yet synthesized: " + function_name + "(" + library_name + ")");
   const std::vector<technology_nodeRef>& op_vec = GetPointer<functional_unit>(current_fu)->get_operations();
   bool has_current_op = false;

   std::string op_name = curr_op->get_name();
   for(const auto& op_it : op_vec)
      if(GetPointer<operation>(op_it)->get_name() == op_name)
         has_current_op = true;

   if(!has_current_op)
   {
      technology_nodeRef op = technology_nodeRef(new operation());
      auto* cop = GetPointer<operation>(op);
      auto* ref_op = GetPointer<operation>(op_vec[0]);
      cop->operation_name = op_name;
      cop->time_m = ref_op->time_m;
#if HAVE_EXPERIMENTAL
      cop->power_m = ref_op->power_m;
#endif
      cop->commutative = curr_op->commutative;
      cop->bounded = ref_op->bounded;
      cop->supported_types = curr_op->supported_types;
      cop->pipe_parameters = curr_op->pipe_parameters;
      GetPointer<functional_unit>(current_fu)->add(op);
   }
   else
   {
      for(const auto& op_it : op_vec)
         if(GetPointer<operation>(op_it)->get_name() == op_name)
         {
            auto* fu_ob = GetPointer<functional_unit>(TM->get_fu(bambu_provided_resource, WORK_LIBRARY));
            THROW_ASSERT(fu_ob, "Functional unit not found for " + bambu_provided_resource);
            auto* op_ob = GetPointer<operation>(fu_ob->get_operation(bambu_provided_resource));
            GetPointer<operation>(op_it)->bounded = op_ob->bounded;
         }
   }

   if(build_wrapper)
      BuildProxyWrapper(GetPointer<functional_unit>(current_fu), bambu_provided_resource, WORK_LIBRARY);
   if(build_proxy)
      BuildProxyFunction(GetPointer<functional_unit>(current_fu));
   return current_fu;
}

static std::string fix_identifier(std::string port_name, language_writerRef writer)
{
   std::string name = HDL_manager::convert_to_identifier(GetPointer<language_writer>(writer), port_name);
   return name;
}

static void connectClockAndReset(structural_managerRef& SM, structural_objectRef& interfaceObj, structural_objectRef& component)
{
   // Clock and Reset connection
   structural_objectRef port_ck = component->find_member(CLOCK_PORT_NAME, port_o_K, component);
   structural_objectRef clock = interfaceObj->find_member(CLOCK_PORT_NAME, port_o_K, interfaceObj);
   if(port_ck and clock)
   {
      SM->add_connection(port_ck, clock);
   }

   structural_objectRef port_rst = component->find_member(RESET_PORT_NAME, port_o_K, component);
   structural_objectRef reset = interfaceObj->find_member(RESET_PORT_NAME, port_o_K, interfaceObj);
   if(port_rst and reset)
   {
      SM->add_connection(port_rst, reset);
   }
}

void allocation::BuildProxyWrapper(functional_unit* current_fu, const std::string& orig_fun_name, const std::string& orig_library_name)
{
   const library_managerRef orig_libraryManager = TM->get_library_manager(orig_library_name);
   THROW_ASSERT(orig_libraryManager->is_fu(orig_fun_name), "functional unit not yet synthesized: " + orig_fun_name + "(" + orig_library_name + ")");
   technology_nodeRef orig_fun = orig_libraryManager->get_fu(orig_fun_name);
   THROW_ASSERT(orig_fun, "functional unit not yet synthesized: " + orig_fun_name + "(" + orig_library_name + ")");

   structural_managerRef orig_fu_SM = GetPointer<functional_unit>(orig_fun)->CM;
   structural_objectRef orig_top_obj = orig_fu_SM->get_circ();
   orig_top_obj->set_id(orig_fun_name + "_i");
   const module* orig_fu_module = GetPointer<module>(orig_top_obj);

   structural_managerRef wrapper_SM = current_fu->CM;
   structural_objectRef wrapper_obj = wrapper_SM->get_circ();
   connectClockAndReset(wrapper_SM, wrapper_obj, orig_top_obj);
   // create the selector signal to tell proxied and unproxied calls apart
   structural_type_descriptorRef bool_signal_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   // select the operations mode of the functional unit, used to drive the proxy selector
   const functional_unit::operation_vec& ops = current_fu->get_operations();
   std::vector<structural_objectRef> op_ports;
   for(const auto& o : ops)
   {
      const std::string op_name = GetPointer<operation>(o)->get_name();
      if(!boost::algorithm::starts_with(op_name, WRAPPED_PROXY_PREFIX))
      {
         const std::string sel_port_name = "sel_" + op_name;
         structural_objectRef new_sel_port = wrapper_obj->find_member(sel_port_name, port_o_K, wrapper_obj);
         /*
          * sometimes the selector port is already present in the proxy.
          * This happens in particular this happens for abort and
          * __builtin_abort.
          * In this case there is no need to add the new port.
          */
         if(not new_sel_port)
         {
            wrapper_SM->add_port(sel_port_name, port_o::IN, wrapper_obj, bool_signal_type);
            new_sel_port = wrapper_obj->find_member(sel_port_name, port_o_K, wrapper_obj);
         }
         op_ports.push_back(new_sel_port);
      }
   }
   // build a binary tree of ors to drive the proxy selector
   structural_objectRef selector_signal = wrapper_SM->add_sign("proxy_selector____out_sel", wrapper_obj, bool_signal_type);
   if(not op_ports.empty())
   {
      structural_objectRef or_gate = wrapper_SM->add_module_from_technology_library("proxy_selector____or_gate", OR_GATE_STD, HLS->HLS_T->get_technology_manager()->get_library(OR_GATE_STD), wrapper_obj, HLS->HLS_T->get_technology_manager());
      structural_objectRef or_in = or_gate->find_member("in", port_vector_o_K, or_gate);
      auto* port = GetPointer<port_o>(or_in);
      port->add_n_ports(static_cast<unsigned int>(op_ports.size()), or_in);
      unsigned int port_n = 0;
      for(const auto& p : op_ports)
      {
         wrapper_SM->add_connection(p, port->get_port(port_n));
         port_n++;
      }
      structural_objectRef or_out = or_gate->find_member("out1", port_o_K, or_gate);
      wrapper_SM->add_connection(or_out, selector_signal);
   }

   auto inPortSize = static_cast<unsigned int>(orig_fu_module->get_in_port_size());
   for(unsigned int port_id = 0; port_id < inPortSize; port_id++)
   {
      structural_objectRef curr_port = orig_fu_module->get_in_port(port_id);
      const std::string port_name = curr_port->get_id();
      if(port_name != CLOCK_PORT_NAME and port_name != RESET_PORT_NAME)
      {
         if(is_other_port(curr_port))
         {
            structural_objectRef wrapper_mem_port = wrapper_obj->find_member(port_name, port_o_K, wrapper_obj);
            structural_objectRef wrapped_mem_port = orig_top_obj->find_member(port_name, port_o_K, orig_top_obj);
            wrapper_SM->add_connection(wrapper_mem_port, wrapped_mem_port);
            continue;
         }
         const std::string proxy_port_name = PROXY_PREFIX + port_name;
         // insert wDataMux
         const auto addwDataMux = [&](const std::string offset) {
            structural_objectRef wrapped_fu_port = orig_top_obj->find_member(port_name, port_o_K, orig_top_obj);
            if(offset != "")
               wrapped_fu_port = wrapped_fu_port->find_member(offset, port_o_K, wrapped_fu_port);
            structural_type_descriptorRef wrapped_port_type = wrapped_fu_port->get_typeRef();
            auto bitwidth_size = STD_GET_SIZE(wrapped_port_type);

            structural_objectRef mux = wrapper_SM->add_module_from_technology_library("proxy_mux_____" + port_name + offset, MUX_GATE_STD, HLS->HLS_T->get_technology_manager()->get_library(MUX_GATE_STD), wrapper_obj, HLS->HLS_T->get_technology_manager());
            structural_objectRef mux_in1 = mux->find_member("in1", port_o_K, mux);
            GetPointer<port_o>(mux_in1)->type_resize(bitwidth_size);
            structural_objectRef mux_in2 = mux->find_member("in2", port_o_K, mux);
            GetPointer<port_o>(mux_in2)->type_resize(bitwidth_size);
            structural_objectRef mux_out = mux->find_member("out1", port_o_K, mux);
            GetPointer<port_o>(mux_out)->type_resize(bitwidth_size);
            structural_objectRef mux_sel = mux->find_member("sel", port_o_K, mux);
            structural_type_descriptorRef mux_out_type = mux_out->get_typeRef();
            const std::string tmp_signal_name = "muxed_in_" + port_name + offset;
            structural_objectRef proxied_in_signal = wrapper_SM->add_sign(tmp_signal_name, wrapper_obj, mux_out_type);

            structural_objectRef proxied_call_port = wrapper_obj->find_member(proxy_port_name, port_o_K, wrapper_obj);
            if(offset != "")
               proxied_call_port = proxied_call_port->find_member(offset, port_o_K, proxied_call_port);
            GetPointer<port_o>(proxied_call_port)->type_resize(bitwidth_size);
            structural_objectRef local_call_port = wrapper_obj->find_member(port_name, port_o_K, wrapper_obj);
            if(offset != "")
               local_call_port = local_call_port->find_member(offset, port_o_K, local_call_port);
            GetPointer<port_o>(local_call_port)->type_resize(bitwidth_size);

            wrapper_SM->add_connection(mux_sel, selector_signal);
            wrapper_SM->add_connection(mux_in1, local_call_port);
            wrapper_SM->add_connection(mux_in2, proxied_call_port);
            wrapper_SM->add_connection(mux_out, proxied_in_signal);
            wrapper_SM->add_connection(proxied_in_signal, wrapped_fu_port);
         };

         if(curr_port->get_kind() == port_o_K)
         {
            addwDataMux("");
         }
         else
         {
            for(unsigned int pindex = 0; pindex < GetPointer<port_o>(curr_port)->get_ports_size(); ++pindex)
            {
               addwDataMux(STR(pindex));
            }
         }
      }
   }
   auto outPortSize = static_cast<unsigned int>(orig_fu_module->get_out_port_size());
   for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
   {
      structural_objectRef curr_port = orig_fu_module->get_out_port(currentPort);
      const std::string port_name = curr_port->get_id();
      if(is_other_port(curr_port))
      {
         structural_objectRef wrapper_mem_port = wrapper_obj->find_member(port_name, port_o_K, wrapper_obj);
         structural_objectRef wrapped_mem_port = orig_top_obj->find_member(port_name, port_o_K, orig_top_obj);
         wrapper_SM->add_connection(wrapper_mem_port, wrapped_mem_port);
         continue;
      }
      const std::string proxy_port_name = PROXY_PREFIX + port_name;
      const auto addwData = [&](const std::string offset) {
         structural_objectRef local_port = wrapper_obj->find_member(port_name, port_o_K, wrapper_obj);
         if(offset != "")
            local_port = local_port->find_member(offset, port_o_K, local_port);
         structural_objectRef proxied_port = wrapper_obj->find_member(proxy_port_name, port_o_K, wrapper_obj);
         if(offset != "")
            proxied_port = proxied_port->find_member(offset, port_o_K, proxied_port);
         structural_objectRef wrapped_fu_port = orig_top_obj->find_member(port_name, port_o_K, orig_top_obj);
         if(offset != "")
            wrapped_fu_port = wrapped_fu_port->find_member(offset, port_o_K, wrapped_fu_port);
         structural_type_descriptorRef wrapped_port_type = wrapped_fu_port->get_typeRef();
         auto bitwidth_size = STD_GET_SIZE(wrapped_port_type);
         GetPointer<port_o>(local_port)->type_resize(bitwidth_size);
         GetPointer<port_o>(proxied_port)->type_resize(bitwidth_size);
         const std::string tmp_signal_name = "tmp_out_" + port_name + offset;
         structural_objectRef out_signal_tmp = wrapper_SM->add_sign(tmp_signal_name, wrapper_obj, wrapped_port_type);
         wrapper_SM->add_connection(wrapped_fu_port, out_signal_tmp);
         wrapper_SM->add_connection(out_signal_tmp, local_port);
         wrapper_SM->add_connection(out_signal_tmp, proxied_port);
      };
      if(curr_port->get_kind() == port_o_K)
      {
         addwData("");
      }
      else
      {
         for(unsigned int pindex = 0; pindex < GetPointer<port_o>(curr_port)->get_ports_size(); ++pindex)
         {
            addwData(STR(pindex));
         }
      }
   }

   memory::propagate_memory_parameters(orig_top_obj, wrapper_SM);
}

void allocation::add_proxy_function_wrapper(const std::string& library_name, technology_nodeRef techNode_obj, const std::string& orig_fun_name)
{
   const std::string wrapped_fu_name = WRAPPED_PROXY_PREFIX + orig_fun_name;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - adding proxy function wrapper " + wrapped_fu_name);
   structural_managerRef structManager_obj = GetPointer<functional_unit>(techNode_obj)->CM;
   structural_objectRef fu_obj = structManager_obj->get_circ();
   auto* fu_module = GetPointer<module>(fu_obj);

   structural_objectRef wrapper_top;
   structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
   structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(wrapped_fu_name));
   CM->set_top_info(wrapped_fu_name, module_type);
   wrapper_top = CM->get_circ();
   /// add description and license
   GetPointer<module>(wrapper_top)->set_description("Proxy wrapper for function: " + wrapped_fu_name);
   GetPointer<module>(wrapper_top)->set_copyright(fu_module->get_copyright());
   GetPointer<module>(wrapper_top)->set_authors(fu_module->get_authors());
   GetPointer<module>(wrapper_top)->set_license(fu_module->get_license());
   GetPointer<module>(wrapper_top)->set_multi_unit_multiplicity(fu_module->get_multi_unit_multiplicity());
   if(fu_module->ExistsParameter(MEMORY_PARAMETER))
   {
      GetPointer<module>(wrapper_top)->AddParameter(MEMORY_PARAMETER, fu_module->GetDefaultParameter(MEMORY_PARAMETER));
      GetPointer<module>(wrapper_top)->SetParameter(MEMORY_PARAMETER, fu_module->GetParameter(MEMORY_PARAMETER));
   }
   // handle input ports
   auto inPortSize = static_cast<unsigned int>(fu_module->get_in_port_size());
   for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_in_port(currentPort);
      const std::string port_name = curr_port->get_id();
      {
         structural_objectRef generated_port;
         if(curr_port->get_kind() == port_vector_o_K)
            generated_port = CM->add_port_vector(port_name, port_o::IN, port_o::PARAMETRIC_PORT, wrapper_top, curr_port->get_typeRef());
         else
            generated_port = CM->add_port(port_name, port_o::IN, wrapper_top, curr_port->get_typeRef());
         curr_port->copy(generated_port);
      }
   }
   for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_in_port(currentPort);
      if(not is_other_port(curr_port))
      {
         const std::string port_name = curr_port->get_id();
         if(port_name != CLOCK_PORT_NAME && port_name != RESET_PORT_NAME)
         {
            structural_objectRef proxy_generated_port;
            const std::string proxy_port_name = PROXY_PREFIX + port_name;
            if(curr_port->get_kind() == port_vector_o_K)
               proxy_generated_port = CM->add_port_vector(proxy_port_name, port_o::IN, port_o::PARAMETRIC_PORT, wrapper_top, curr_port->get_typeRef());
            else
               proxy_generated_port = CM->add_port(proxy_port_name, port_o::IN, wrapper_top, curr_port->get_typeRef());
            curr_port->copy(proxy_generated_port);
            GetPointer<port_o>(proxy_generated_port)->set_id(proxy_port_name);
         }
      }
   }
   // handle output ports
   auto outPortSize = static_cast<unsigned int>(fu_module->get_out_port_size());
   for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_out_port(currentPort);
      const std::string port_name = curr_port->get_id();
      {
         structural_objectRef generated_port;
         if(curr_port->get_kind() == port_vector_o_K)
            generated_port = CM->add_port_vector(port_name, port_o::OUT, port_o::PARAMETRIC_PORT, wrapper_top, curr_port->get_typeRef());
         else
            generated_port = CM->add_port(port_name, port_o::OUT, wrapper_top, curr_port->get_typeRef());
         curr_port->copy(generated_port);
      }
   }
   for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_out_port(currentPort);
      if(not is_other_port(curr_port))
      {
         structural_objectRef proxy_generated_port;
         const std::string proxy_port_name = PROXY_PREFIX + curr_port->get_id();
         if(curr_port->get_kind() == port_vector_o_K)
            proxy_generated_port = CM->add_port_vector(proxy_port_name, port_o::OUT, port_o::PARAMETRIC_PORT, wrapper_top, curr_port->get_typeRef());
         else
            proxy_generated_port = CM->add_port(proxy_port_name, port_o::OUT, wrapper_top, curr_port->get_typeRef());
         curr_port->copy(proxy_generated_port);
         GetPointer<port_o>(proxy_generated_port)->set_id(proxy_port_name);
      }
   }

   const NP_functionalityRef& np = fu_module->get_NP_functionality();
   std::string orig_np_library = np->get_NP_functionality(NP_functionality::LIBRARY);
   orig_np_library.replace(0, orig_fun_name.size(), wrapped_fu_name);
   CM->add_NP_functionality(wrapper_top, NP_functionality::LIBRARY, orig_np_library);
   TM->add_resource(PROXY_LIBRARY, wrapped_fu_name, CM);
   fu_obj->set_owner(wrapper_top);
   GetPointer<module>(wrapper_top)->add_internal_object(fu_obj);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Created proxy wrapper " + wrapped_fu_name + " and added to library " + PROXY_LIBRARY);

   auto wrapper_tn = TM->get_fu(wrapped_fu_name, PROXY_LIBRARY);
   auto* wrapper_fu = GetPointer<functional_unit>(wrapper_tn);
   auto* orig_fu = GetPointer<functional_unit>(techNode_obj);
   wrapper_fu->ordered_attributes = orig_fu->ordered_attributes;
   wrapper_fu->attributes = orig_fu->attributes;
   wrapper_fu->clock_period = orig_fu->clock_period;
   wrapper_fu->clock_period_resource_fraction = orig_fu->clock_period_resource_fraction;
   wrapper_fu->area_m = orig_fu->area_m;
   wrapper_fu->fu_template_name = orig_fu->fu_template_name;
   wrapper_fu->fu_template_parameters = orig_fu->fu_template_parameters;
   wrapper_fu->characterizing_constant_value = orig_fu->characterizing_constant_value;
   wrapper_fu->memory_type = orig_fu->memory_type;
   wrapper_fu->channels_type = orig_fu->channels_type;
   wrapper_fu->memory_ctrl_type = orig_fu->memory_ctrl_type;
   wrapper_fu->bram_load_latency = orig_fu->bram_load_latency;
   const functional_unit::operation_vec& ops = orig_fu->get_operations();
   for(const auto& op : ops)
   {
      auto* current_op = GetPointer<operation>(op);
      std::string op_name = current_op->get_name();
      TM->add_operation(PROXY_LIBRARY, wrapped_fu_name, op_name);
      auto* proxy_op = GetPointer<operation>(wrapper_fu->get_operation(op_name));
      proxy_op->time_m = current_op->time_m;
      proxy_op->commutative = current_op->commutative;
      proxy_op->bounded = current_op->bounded;
      proxy_op->supported_types = current_op->supported_types;
      proxy_op->pipe_parameters = current_op->pipe_parameters;
   }
   /// add a fictitious operation to allow bus merging
   TM->add_operation(PROXY_LIBRARY, wrapped_fu_name, wrapped_fu_name);
   auto* wrapper_fictious_op = GetPointer<operation>(wrapper_fu->get_operation(wrapped_fu_name));
   wrapper_fictious_op->time_m = time_model::create_model(HLS_T->get_target_device()->get_type(), parameters);

   /// automatically build proxy wrapper HDL description
   BuildProxyWrapper(wrapper_fu, orig_fu->get_name(), library_name);
}

void allocation::BuildProxyFunctionVerilog(functional_unit* current_fu)
{
   const functional_unit::operation_vec& ops = current_fu->get_operations();
   structural_managerRef CM = current_fu->CM;
   structural_objectRef top = CM->get_circ();
   auto* fu_module = GetPointer<module>(top);
   auto inPortSize = static_cast<unsigned int>(fu_module->get_in_port_size());
   auto outPortSize = static_cast<unsigned int>(fu_module->get_out_port_size());
   language_writerRef writer = language_writer::create_writer(HDLWriter_Language::VERILOG, HLSMgr->get_HLS_target()->get_technology_manager(), parameters);

   structural_type_descriptorRef b_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   std::string sel_guard;
   for(const auto& op : ops)
   {
      auto* current_op = GetPointer<operation>(op);
      std::string op_name = current_op->get_name();
      if(boost::algorithm::starts_with(op_name, PROXY_PREFIX))
         continue;
      std::string sel_port_name = "sel_" + op_name;
      structural_objectRef sel_port = fu_module->find_member(sel_port_name, port_o_K, top);
      if(!sel_port)
      {
         CM->add_port(sel_port_name, port_o::IN, top, b_type);
      }
      if(sel_guard == "")
         sel_guard = sel_port_name;
      else
         sel_guard = "(" + sel_guard + "|" + sel_port_name + ")";
   }

   std::string verilog_description;
   for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_in_port(currentPort);
      if(is_other_port(curr_port))
         continue;
      std::string port_name = curr_port->get_id();
      if(port_name != CLOCK_PORT_NAME && port_name != RESET_PORT_NAME)
      {
         if(verilog_description != "")
            verilog_description += "\n";
         if(port_name == START_PORT_NAME)
            verilog_description = verilog_description + "assign " + PROXY_PREFIX + port_name + " = " + fix_identifier(port_name, writer) + ";";
         else if(fu_module->find_member(PROXY_PREFIX + port_name, port_o_K, top))
            verilog_description = verilog_description + "assign " + PROXY_PREFIX + port_name + " = " + sel_guard + " ? " + fix_identifier(port_name, writer) + " : 0;";
      }
   }
   for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_out_port(currentPort);
      if(is_other_port(curr_port))
         continue;
      std::string port_name = curr_port->get_id();
      if(verilog_description != "")
         verilog_description += "\n";
      verilog_description = verilog_description + "assign " + fix_identifier(port_name, writer) + " = " + PROXY_PREFIX + port_name + ";";
   }
   add_escape(verilog_description, "\\");
   CM->add_NP_functionality(top, NP_functionality::VERILOG_PROVIDED, verilog_description);
}

void allocation::BuildProxyFunctionVHDL(functional_unit* current_fu)
{
   const functional_unit::operation_vec& ops = current_fu->get_operations();
   structural_managerRef CM = current_fu->CM;
   structural_objectRef top = CM->get_circ();
   auto* fu_module = GetPointer<module>(top);
   auto inPortSize = static_cast<unsigned int>(fu_module->get_in_port_size());
   auto outPortSize = static_cast<unsigned int>(fu_module->get_out_port_size());
   language_writerRef writer = language_writer::create_writer(HDLWriter_Language::VHDL, HLSMgr->get_HLS_target()->get_technology_manager(), parameters);

   structural_type_descriptorRef b_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   std::string sel_guard;
   for(const auto& op : ops)
   {
      auto* current_op = GetPointer<operation>(op);
      std::string op_name = current_op->get_name();
      if(boost::algorithm::starts_with(op_name, PROXY_PREFIX))
         continue;
      std::string sel_port_name = "sel_" + op_name;
      structural_objectRef sel_port = fu_module->find_member(sel_port_name, port_o_K, top);
      if(!sel_port)
      {
         CM->add_port(sel_port_name, port_o::IN, top, b_type);
      }
      if(sel_guard == "")
         sel_guard = fix_identifier(sel_port_name, writer);
      else
         sel_guard = sel_guard + " or " + fix_identifier(sel_port_name, writer);
   }

   sel_guard = "(" + sel_guard + ") = '1'";

   std::string VHDL_description;
   VHDL_description += "begin";
   for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_in_port(currentPort);
      if(is_other_port(curr_port))
         continue;
      std::string port_name = curr_port->get_id();
      if(port_name != CLOCK_PORT_NAME && port_name != RESET_PORT_NAME)
      {
         if(VHDL_description != "")
            VHDL_description += "\n";
         if(port_name == START_PORT_NAME)
            VHDL_description = VHDL_description + fix_identifier(PROXY_PREFIX + port_name, writer) + " <= " + fix_identifier(port_name, writer) + ";";
         else if(fu_module->find_member(PROXY_PREFIX + port_name, port_o_K, top))
            VHDL_description = VHDL_description + fix_identifier(PROXY_PREFIX + port_name, writer) + " <= " + fix_identifier(port_name, writer) + " when (" + sel_guard + ") else " +
                               (curr_port->get_typeRef()->type == structural_type_descriptor::BOOL ? "'0'" : "(others => '0')") + ";";
      }
   }
   for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_out_port(currentPort);
      if(is_other_port(curr_port))
         continue;
      std::string port_name = curr_port->get_id();
      if(VHDL_description != "")
         VHDL_description += "\n";
      VHDL_description = VHDL_description + fix_identifier(port_name, writer) + " <= " + fix_identifier(PROXY_PREFIX + port_name, writer) + ";";
   }
   add_escape(VHDL_description, "\\");
   CM->add_NP_functionality(top, NP_functionality::VHDL_PROVIDED, VHDL_description);
}

void allocation::BuildProxyFunction(functional_unit* current_fu)
{
   const auto writer = static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language));
   switch(writer)
   {
      case HDLWriter_Language::VHDL:
      {
         BuildProxyFunctionVHDL(current_fu);
         break;
      }
      case HDLWriter_Language::VERILOG:
      {
         BuildProxyFunctionVerilog(current_fu);
         break;
      }
      case HDLWriter_Language::SYSTEM_VERILOG:
#if HAVE_EXPERIMENTAL
      case HDLWriter_Language::SYSTEMC:
      case HDLWriter_Language::BLIF:
      case HDLWriter_Language::EDIF:
#endif
      default:
         THROW_UNREACHABLE("");
   }
}

void allocation::add_proxy_function_module(const HLS_constraintsRef HLS_C, technology_nodeRef techNode_obj, const std::string& orig_fun_name)
{
   const std::string proxied_fu_name = PROXY_PREFIX + orig_fun_name;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - adding proxied function " + proxied_fu_name);
   structural_managerRef structManager_obj = GetPointer<functional_unit>(techNode_obj)->CM;
   structural_objectRef fu_obj = structManager_obj->get_circ();
   auto* fu_module = GetPointer<module>(fu_obj);

   structural_objectRef top;
   structural_managerRef CM = structural_managerRef(new structural_manager(parameters));
   structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(proxied_fu_name));
   CM->set_top_info(proxied_fu_name, module_type);
   top = CM->get_circ();
   /// add description and license
   GetPointer<module>(top)->set_description("Proxy module for function: " + proxied_fu_name);
   GetPointer<module>(top)->set_copyright(fu_module->get_copyright());
   GetPointer<module>(top)->set_authors(fu_module->get_authors());
   GetPointer<module>(top)->set_license(fu_module->get_license());
   GetPointer<module>(top)->set_multi_unit_multiplicity(fu_module->get_multi_unit_multiplicity());

   /*
    * The proxy is a module instantiated in the datapath of the caller.
    * It is used to call a shared function module that is allocated in one of
    * the dominators of the caller.
    * We can imagine this with three layers
    *
    *    CALLER LAYER
    *
    *          ^
    * ----|----|---------
    *     v
    *
    *    PROXY LAYER
    *
    *          ^
    * ----|----|---------
    *     v
    *
    *   CALLED LAYER
    *   (proxied function)
    *
    * The called layer connects the signals from the proxy to the real
    * instance of the called function, wherever it was allocated.
    * It is not handled here.
    * Here we're building the component that implements the proxy layer and we
    * have to add the ports for communication with the caller layer and the
    * called layer.
    */
   auto inPortSize = static_cast<unsigned int>(fu_module->get_in_port_size());
   auto outPortSize = static_cast<unsigned int>(fu_module->get_out_port_size());
   // analyze the input signals of the proxed function, i.e. the function called through the proxy
   for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_in_port(currentPort);
      if(is_other_port(curr_port))
         continue;

      const std::string port_name = curr_port->get_id();
      // clock and reset are not propagated because the proxied functions have their own
      if(port_name != CLOCK_PORT_NAME and port_name != RESET_PORT_NAME)
      {
         /*
          * add an input port to the proxy for every input port of the proxied function.
          * This connects the caller layer to the proxy
          */
         structural_objectRef generated_port;
         if(curr_port->get_kind() == port_vector_o_K)
            generated_port = CM->add_port_vector(port_name, port_o::IN, port_o::PARAMETRIC_PORT, top, curr_port->get_typeRef());
         else
            generated_port = CM->add_port(port_name, port_o::IN, top, curr_port->get_typeRef());
         curr_port->copy(generated_port);
      }
   }
   // analyze the output signals of the proxied function
   for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_out_port(currentPort);
      if(is_other_port(curr_port))
         continue;
      /*
       * Add an output port to the proxy for every output port of the proxied function.
       * These connect the proxy to the caller layer
       */
      structural_objectRef generated_port;
      const std::string port_name = curr_port->get_id();
      if(curr_port->get_kind() == port_vector_o_K)
         generated_port = CM->add_port_vector(port_name, port_o::OUT, port_o::PARAMETRIC_PORT, top, curr_port->get_typeRef());
      else
         generated_port = CM->add_port(port_name, port_o::OUT, top, curr_port->get_typeRef());
      curr_port->copy(generated_port);
   }
   // analyze the input signals of the proxied function, i.e. the function called through the proxy
   for(unsigned int currentPort = 0; currentPort < inPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_in_port(currentPort);
      if(is_other_port(curr_port))
         continue;

      const std::string port_name = curr_port->get_id();
      // clock and reset are not propagated because the proxied functions have their own
      if(port_name != CLOCK_PORT_NAME and port_name != RESET_PORT_NAME)
      {
         /*
          * add an output port to the proxy for every input port of the proxied function.
          * This connects the proxy to the called layer.
          * The connection is outgoing from the proxy to the called proxied function.
          */
         structural_objectRef proxy_generated_port;
         const std::string proxied_port_name = PROXY_PREFIX + port_name;
         if(curr_port->get_kind() == port_vector_o_K)
            proxy_generated_port = CM->add_port_vector(proxied_port_name, port_o::OUT, port_o::PARAMETRIC_PORT, top, curr_port->get_typeRef());
         else
            proxy_generated_port = CM->add_port(proxied_port_name, port_o::OUT, top, curr_port->get_typeRef());
         curr_port->copy(proxy_generated_port);
         GetPointer<port_o>(proxy_generated_port)->set_port_direction(port_o::OUT);
         GetPointer<port_o>(proxy_generated_port)->set_is_memory(true);
         GetPointer<port_o>(proxy_generated_port)->set_id(proxied_port_name);
      }
   }
   // analyze the output signals of the proxied function
   for(unsigned int currentPort = 0; currentPort < outPortSize; ++currentPort)
   {
      structural_objectRef curr_port = fu_module->get_out_port(currentPort);
      if(is_other_port(curr_port))
         continue;
      /*
       * add an input port to the proxy for every output port of the proxied function.
       * This connects the proxy to the called layer.
       * The connection is incoming from the called proxied function to the proxy itself
       */
      const std::string proxied_port_name = PROXY_PREFIX + curr_port->get_id();
      structural_objectRef proxy_generated_port;
      if(curr_port->get_kind() == port_vector_o_K)
         proxy_generated_port = CM->add_port_vector(proxied_port_name, port_o::IN, port_o::PARAMETRIC_PORT, top, curr_port->get_typeRef());
      else
         proxy_generated_port = CM->add_port(proxied_port_name, port_o::IN, top, curr_port->get_typeRef());
      curr_port->copy(proxy_generated_port);
      GetPointer<port_o>(proxy_generated_port)->set_port_direction(port_o::IN);
      GetPointer<port_o>(proxy_generated_port)->set_is_memory(true);
      GetPointer<port_o>(proxy_generated_port)->set_id(proxied_port_name);
   }

   const NP_functionalityRef& np = fu_module->get_NP_functionality();
   std::string orig_np_library = np->get_NP_functionality(NP_functionality::LIBRARY);
   orig_np_library.replace(0, orig_fun_name.size(), proxied_fu_name);
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, orig_np_library);
   TM->add_resource(PROXY_LIBRARY, proxied_fu_name, CM);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Created proxy module " + proxied_fu_name + " and added to library " + PROXY_LIBRARY);

   auto proxy_tn = TM->get_fu(proxied_fu_name, PROXY_LIBRARY);
   auto* proxy_fu = GetPointer<functional_unit>(proxy_tn);
   auto* orig_fu = GetPointer<functional_unit>(techNode_obj);
   proxy_fu->ordered_attributes = orig_fu->ordered_attributes;
   proxy_fu->attributes = orig_fu->attributes;
   proxy_fu->clock_period = orig_fu->clock_period;
   proxy_fu->clock_period_resource_fraction = orig_fu->clock_period_resource_fraction;
   proxy_fu->area_m = orig_fu->area_m;
   proxy_fu->fu_template_name = orig_fu->fu_template_name;
   proxy_fu->fu_template_parameters = orig_fu->fu_template_parameters;
   proxy_fu->characterizing_constant_value = orig_fu->characterizing_constant_value;
   proxy_fu->memory_type = orig_fu->memory_type;
   proxy_fu->channels_type = orig_fu->channels_type;
   proxy_fu->memory_ctrl_type = orig_fu->memory_ctrl_type;
   proxy_fu->bram_load_latency = orig_fu->bram_load_latency;
   const functional_unit::operation_vec& ops = orig_fu->get_operations();
   for(const auto& op : ops)
   {
      auto* current_op = GetPointer<operation>(op);
      std::string op_name = current_op->get_name();
      TM->add_operation(PROXY_LIBRARY, proxied_fu_name, op_name);
      auto* proxy_op = GetPointer<operation>(proxy_fu->get_operation(op_name));
      proxy_op->time_m = current_op->time_m;
      proxy_op->commutative = current_op->commutative;
      proxy_op->bounded = current_op->bounded;
      proxy_op->supported_types = current_op->supported_types;
      proxy_op->pipe_parameters = current_op->pipe_parameters;
   }
   /// add a fictitious operation to allow bus merging
   TM->add_operation(PROXY_LIBRARY, proxied_fu_name, proxied_fu_name);
   auto* proxy_fictious_op = GetPointer<operation>(proxy_fu->get_operation(proxied_fu_name));
   proxy_fictious_op->time_m = time_model::create_model(HLS_T->get_target_device()->get_type(), parameters);

   /// automatically build proxy description
   BuildProxyFunction(proxy_fu);

   std::string key_new = ENCODE_FU_LIB(proxied_fu_name, PROXY_LIBRARY);
   HLS_C->tech_constraints[key_new] = 1;
}

void allocation::add_tech_constraint(technology_nodeRef cur_fu, unsigned int tech_constrain_value, unsigned int pos, bool proxy_constrained)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Specialized unit: " + (cur_fu->get_name()) + " in position: " + STR(pos) + (proxy_constrained ? "(proxy)" : ""));
   /// check resource constraints for indirect memory accesses
   auto last_fu = static_cast<unsigned int>(allocation_information->list_of_FU.size());
   allocation_information->list_of_FU.push_back(cur_fu);
   bool is_memory_ctrl = allocation_information->is_indirect_access_memory_unit(last_fu);
   if(is_memory_ctrl || allocation_information->get_number_channels(pos) >= 1)
   {
      allocation_information->tech_constraints.push_back(allocation_information->get_number_channels(pos));
   }
   else if(proxy_constrained)
   {
      allocation_information->tech_constraints.push_back(1);
   }
   else
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Constrained " + STR(pos) + "=" + cur_fu->get_name() + "->" << STR(tech_constrain_value));
      allocation_information->tech_constraints.push_back(tech_constrain_value);
   }
}

bool allocation::check_templated_units(double clock_period, node_kind_prec_infoRef node_info, const library_managerRef library, technology_nodeRef current_fu, operation* curr_op)
{
   std::string required_prec = "";
   std::string template_suffix = "";
   const size_t n_ins = node_info->input_prec.size();
   for(size_t ind = 0; ind < n_ins; ++ind)
   {
      if(node_info->base128_input_nelem[ind] == 0)
      {
         required_prec += STR(node_info->input_prec[ind]) + " ";
         template_suffix += STR(node_info->input_prec[ind]) + "_";
      }
      else
      {
         required_prec += STR(node_info->input_prec[ind]) + " " + STR(node_info->base128_input_nelem[ind]) + " ";
         template_suffix += STR(node_info->input_prec[ind]) + "_" + STR(node_info->base128_input_nelem[ind]) + "_";
      }
   }
   if(node_info->base128_output_nelem == 0)
   {
      required_prec += STR(node_info->output_prec);
      template_suffix += STR(node_info->output_prec);
   }
   else
   {
      required_prec += STR(node_info->output_prec) + " " + STR(node_info->base128_output_nelem);
      template_suffix += STR(node_info->output_prec) + "_" + STR(node_info->base128_output_nelem);
   }
   std::string fu_template_parameters = GetPointer<functional_unit>(current_fu)->fu_template_parameters;
   if(!boost::algorithm::starts_with(fu_template_parameters, required_prec))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not support required precision " + STR(required_prec) + "(" + fu_template_parameters + ")");
      return true;
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "- required_prec: \"" + required_prec);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "- template_suffix: \"" + template_suffix + "\"");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "- fu_template_parameters: \"" + fu_template_parameters + "\"");
   std::string pipeline_id =
       get_compliant_pipelined_unit(clock_period, curr_op->pipe_parameters, current_fu, curr_op->get_name(), library->get_library_name(), template_suffix, node_info->input_prec.size() > 1 ? node_info->input_prec[1] : node_info->input_prec[0]);
   if(not curr_op->pipe_parameters.empty())
   {
      if(pipeline_id != "")
         required_prec += " " + pipeline_id;
      // if the computed parameters is different from what was used to build this specialization skip it.
      if(required_prec != fu_template_parameters)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + required_prec + " vs. " + fu_template_parameters);
         return true;
      }
   }
   if(pipeline_id == "")
   {
      if(curr_op->time_m->get_cycles() == 0 && allocation_information->time_m_execution_time(curr_op) > clock_period)
         THROW_WARNING("No functional unit exists for the given clock period: the fastest unit will be used as multi-cycle unit (" + GetPointer<functional_unit>(current_fu)->fu_template_name +
                       "): " + STR(allocation_information->time_m_execution_time(curr_op)));
   }
   return false;
}

bool allocation::check_for_memory_compliancy(bool Has_extern_allocated_data, technology_nodeRef current_fu, const std::string& memory_ctrl_type, std::string channels_type)
{
   std::string memory_type = GetPointer<functional_unit>(current_fu)->memory_type;
   std::string bram_load_latency = GetPointer<functional_unit>(current_fu)->bram_load_latency;

   /// LOAD/STORE operations allocated on memories have been already allocated
   if(memory_type != "")
   {
      return true;
   }

   /// LOAD/STORE operations on proxys have been already managed
   if(channels_type != "" && (memory_ctrl_type == MEMORY_CTRL_TYPE_PROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_PROXYN || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXYN))
   {
      return true;
   }
#if !HAVE_EXPERIMENTAL
   if(GetPointer<functional_unit>(current_fu)->functional_unit_name == "MEMORY_CTRL_P1N")
   {
      return true;
   }
#endif

   const auto channel_type_to_be_used = parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type);
   if(channels_type != "")
   {
      switch(channel_type_to_be_used)
      {
         case(MemoryAllocation_ChannelsType::MEM_ACC_11):
         case(MemoryAllocation_ChannelsType::MEM_ACC_N1):
         {
            if(channels_type.find(CHANNELS_TYPE_MEM_ACC_NN) != std::string::npos or channels_type.find(CHANNELS_TYPE_MEM_ACC_P1N) != std::string::npos or channels_type.find(CHANNELS_TYPE_MEM_ACC_CS) != std::string::npos)
               return true;
            break;
         }
         case(MemoryAllocation_ChannelsType::MEM_ACC_NN):
         {
            if(channels_type.find(CHANNELS_TYPE_MEM_ACC_NN) == std::string::npos)
               return true;
            break;
         }
         case(MemoryAllocation_ChannelsType::MEM_ACC_P1N):
         {
            if(channels_type.find(CHANNELS_TYPE_MEM_ACC_P1N) == std::string::npos)
            {
               return true;
            }
            break;
         }
         case(MemoryAllocation_ChannelsType::MEM_ACC_CS):
         {
            if(channels_type.find(CHANNELS_TYPE_MEM_ACC_CS) == std::string::npos)
            {
               return true;
            }
            break;
         }
         default:
            THROW_UNREACHABLE("");
      }
   }
   bool are_operations_bounded = memory_ctrl_type != "";
   const functional_unit::operation_vec& Operations = GetPointer<functional_unit>(current_fu)->get_operations();
   const functional_unit::operation_vec::const_iterator it_o_end = Operations.end();
   for(auto it_o = Operations.begin(); it_o_end != it_o && are_operations_bounded; ++it_o)
      if(!GetPointer<operation>(*it_o)->is_bounded())
         are_operations_bounded = false;

   if(Has_extern_allocated_data && are_operations_bounded && memory_ctrl_type != "")
   {
      return true;
   }
   if(!Has_extern_allocated_data && !are_operations_bounded && memory_ctrl_type != "")
   {
      return true;
   }

   if(memory_ctrl_type != "" && parameters->getOption<std::string>(OPT_memory_controller_type) != memory_ctrl_type)
   {
      return true;
   }
   if(bram_load_latency != "")
   {
      if(bram_load_latency == "2" && parameters->getOption<std::string>(OPT_bram_high_latency) != "")
         return true;
      if(bram_load_latency == "3" && parameters->getOption<std::string>(OPT_bram_high_latency) != "_3")
         return true;
      if(bram_load_latency == "4" && parameters->getOption<std::string>(OPT_bram_high_latency) != "_4")
         return true;
      if(bram_load_latency != "2" && bram_load_latency != "3" && bram_load_latency != "4")
         THROW_ERROR("unexpected bram_load_latency");
   }
   return false;
}

bool allocation::check_type_and_precision(operation* curr_op, node_kind_prec_infoRef node_info)
{
   if(node_info->node_kind != "" && !curr_op->is_type_supported(node_info->node_kind))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not supported type " + node_info->node_kind);
      return true; // FU does not support the operation type
   }
   // Check for correct precision
   if(!curr_op->is_type_supported(node_info->node_kind, node_info->input_prec, node_info->base128_input_nelem))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not supported precision");
      return true; // FU does support the operation type, but does not support correct precision
   }
   return false;
}

bool allocation::check_proxies(const library_managerRef library, const std::string& fu_name_)
{
   const auto fu_name = functions::get_function_name_cleaned(fu_name_);
   if(HLSMgr->Rfuns->is_a_proxied_function(fu_name))
      return true;
   if(library->get_library_name() == PROXY_LIBRARY)
   {
      if(boost::algorithm::starts_with(fu_name, WRAPPED_PROXY_PREFIX))
      {
         std::string original_function_name = fu_name.substr(std::string(WRAPPED_PROXY_PREFIX).size());
         if(!HLSMgr->Rfuns->is_a_shared_function(funId, original_function_name))
            return true;
      }
      else
      {
         THROW_ASSERT(fu_name.compare(0, std::string(PROXY_PREFIX).size(), PROXY_PREFIX) == 0, "expected a proxy module");
         std::string original_function_name = fu_name.substr(std::string(PROXY_PREFIX).size());
         if(!HLSMgr->Rfuns->is_a_proxied_shared_function(funId, original_function_name))
            return true;
      }
   }
   return false;
}

bool allocation::check_generated_bambu_flopoco(bool skip_softfloat_resources, structural_managerRef structManager_obj, std::string& bambu_provided_resource, bool skip_flopoco_resources, technology_nodeRef current_fu)
{
   if(structManager_obj) /// generated cannot be directly considered for allocation
   {
      structural_objectRef modobj = structManager_obj->get_circ();
      auto* mod = GetPointer<module>(modobj);

      if(mod->get_generated())
         return true;

      const NP_functionalityRef& np = mod->get_NP_functionality();
      if(skip_flopoco_resources)
      {
         if(np && np->get_NP_functionality(NP_functionality::FLOPOCO_PROVIDED) != "")
            return true;
      }
      if(skip_softfloat_resources)
      {
         if(np && np->get_NP_functionality(NP_functionality::BAMBU_PROVIDED) != "")
            return true;
      }
      else if(np)
         bambu_provided_resource = np->get_NP_functionality(NP_functionality::BAMBU_PROVIDED);
   }
   else if(GetPointer<functional_unit>(current_fu)->fu_template_name != "")
   {
      std::string tfname = GetPointer<functional_unit>(current_fu)->fu_template_name;
      technology_nodeRef tfu = get_fu(tfname);
      if(!tfu || !GetPointer<functional_unit_template>(tfu) || !GetPointer<functional_unit_template>(tfu)->FU || !GetPointer<functional_unit>(GetPointer<functional_unit_template>(tfu)->FU)->CM)
         return true;
      structural_managerRef tcm = GetPointer<functional_unit>(GetPointer<functional_unit_template>(tfu)->FU)->CM;
      structural_objectRef tmodobj = tcm->get_circ();
      auto* tmod = GetPointer<module>(tmodobj);
      const NP_functionalityRef& tnp = tmod->get_NP_functionality();
      if(tnp && skip_flopoco_resources && tnp->get_NP_functionality(NP_functionality::FLOPOCO_PROVIDED) != "")
         return true;
      if(tnp && skip_softfloat_resources && tnp->get_NP_functionality(NP_functionality::BAMBU_PROVIDED) != "")
         return true;
      else if(tnp)
         bambu_provided_resource = tnp->get_NP_functionality(NP_functionality::BAMBU_PROVIDED);
   }
   return false;
}

DesignFlowStep_Status allocation::InternalExec()
{
   const HLS_constraintsRef HLS_C = HLS->HLS_C;
   const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(funId);
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   const std::map<unsigned int, memory_symbolRef>& function_vars = HLSMgr->Rmem->get_function_vars(funId);
   double clock_period = HLS_C->get_clock_period_resource_fraction() * HLS_C->get_clock_period();
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      START_TIME(step_time);
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Module allocation information for function " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() + ":");
   unsigned long long int base_address = HLSMgr->base_address;
   bool Has_extern_allocated_data = ((HLSMgr->Rmem->get_memory_address() - base_address) > 0 && parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::EXT_PIPELINED_BRAM) ||
                                    (HLSMgr->Rmem->has_unknown_addresses() && parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::ALL_BRAM &&
                                     parameters->getOption<MemoryAllocation_Policy>(OPT_memory_allocation_policy) != MemoryAllocation_Policy::EXT_PIPELINED_BRAM);
   IntegrateTechnologyLibraries();

#if HAVE_FLOPOCO
   bool skip_flopoco_resources = false;
#else
   bool skip_flopoco_resources = true;
#endif
   bool skip_softfloat_resources = true;
   if(parameters->isOption(OPT_soft_float) && parameters->getOption<bool>(OPT_soft_float))
   {
      skip_flopoco_resources = true;
      skip_softfloat_resources = false;
   }
   auto tech_vec = HLS_C->tech_constraints;
   auto binding_constraints = HLS_C->binding_constraints;
   CustomUnorderedMap<std::string, std::map<unsigned int, unsigned int>> fu_name_to_id;
   CustomOrderedSet<vertex> vertex_analysed;
   OpVertexSet support_ops(function_behavior->CGetOpGraph(FunctionBehavior::CFG));
   support_ops.insert(HLS->operations.begin(), HLS->operations.end());
   const OpGraphConstRef g = function_behavior->CGetOpGraph(FunctionBehavior::CFG, support_ops);
   OpVertexSet vertex_to_analyse(g);
   graph::vertex_iterator v, v_end;
   std::map<std::string, technology_nodeRef> new_fu;
   bool gimple_return_allocated_p = false;
   unsigned int gimple_return_current_id = 0;
   for(boost::tie(v, v_end) = boost::vertices(*g); v != v_end; ++v)
   {
      std::string current_op = tree_helper::normalized_ID(g->CGetOpNodeInfo(*v)->GetOperation());
      const auto node_id = g->CGetOpNodeInfo(*v)->GetNodeId();
      const auto node_operation = [&]() -> std::string {
         if(node_id == ENTRY_ID)
            return "Entry";
         if(node_id == EXIT_ID)
            return "Exit";
         return GetPointer<const gimple_node>(TreeM->CGetTreeNode(node_id))->operation;
      }();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Processing operation: " + current_op + " - " + GET_NAME(g, *v) + (node_id and node_id != ENTRY_ID and node_id != EXIT_ID ? " - " + TreeM->get_tree_node_const(node_id)->ToString() : ""));
      technology_nodeRef current_fu;
      if(GET_TYPE(g, *v) & (TYPE_STORE | TYPE_LOAD))
      {
         const tree_nodeRef curr_tn = TreeM->get_tree_node_const(node_id);
         auto* me = GetPointer<gimple_assign>(curr_tn);
         THROW_ASSERT(me, "only gimple_assign's are allowed as memory operations");
         unsigned int var = 0;
         if(GET_TYPE(g, *v) & TYPE_STORE)
            var = tree_helper::get_base_index(TreeM, GET_INDEX_NODE(me->op0));
         else
            var = tree_helper::get_base_index(TreeM, GET_INDEX_NODE(me->op1));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Variable is " + (var != 0 ? TreeM->CGetTreeNode(var)->ToString() : "0"));
         if(var == 0 ||
            (function_vars.find(var) == function_vars.end() && (!HLSMgr->Rmem->has_proxied_internal_variables(funId) || HLSMgr->Rmem->get_proxied_internal_variables(funId).find(var) == HLSMgr->Rmem->get_proxied_internal_variables(funId).end())))
         {
            vertex_to_analyse.insert(*v);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Operation " + current_op + " queued for allocation");
            continue;
         }
         THROW_ASSERT(allocation_information->vars_to_memory_units.find(var) != allocation_information->vars_to_memory_units.end(), "Not existing memory unit associated with the variable");
         allocation_information->binding[node_id] = std::pair<std::string, unsigned int>(current_op, allocation_information->vars_to_memory_units[var]);
         allocation_information->node_id_to_fus[std::pair<unsigned int, std::string>(node_id, node_operation)].insert(allocation_information->vars_to_memory_units[var]);
         current_fu = allocation_information->list_of_FU[allocation_information->vars_to_memory_units[var]];
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Operation " + current_op + " named " + GET_NAME(g, *v) + " mapped onto " + current_fu->get_name() + ", found in library " + TM->get_library(current_op) + " in position " + STR(allocation_information->vars_to_memory_units[var]));
      }
      else if(GIMPLE_RETURN == current_op)
      {
         if(!gimple_return_allocated_p)
         {
            unsigned int current_size = allocation_information->get_number_fu_types();
            gimple_return_current_id = current_size;
            current_fu = get_fu(GIMPLE_RETURN_STD);
            allocation_information->list_of_FU.push_back(current_fu);
            allocation_information->tech_constraints.push_back(1);
            allocation_information->id_to_fu_names[gimple_return_current_id] = std::make_pair(current_fu->get_name(), TM->get_library(current_fu->get_name()));
            allocation_information->is_vertex_bounded_rel.insert(gimple_return_current_id);
            allocation_information->precision_map[gimple_return_current_id] = 0;
            gimple_return_allocated_p = true;
         }
         else
            current_fu = allocation_information->list_of_FU[gimple_return_current_id];
         allocation_information->binding[node_id] = std::pair<std::string, unsigned int>(current_op, gimple_return_current_id);
         allocation_information->node_id_to_fus[std::pair<unsigned int, std::string>(node_id, node_operation)].insert(gimple_return_current_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Operation " + current_op + " named " + GET_NAME(g, *v) + " mapped onto " + current_fu->get_name() + ", found in library " + TM->get_library(current_op) + " in position " + STR(gimple_return_current_id));
      }
      // direct mapping FUs
      else if(ASSIGN == current_op || ASSERT_EXPR == current_op || ADDR_EXPR == current_op || READ_COND == current_op || MULTI_READ_COND == current_op || NOP_EXPR == current_op || CONVERT_EXPR == current_op || SWITCH_COND == current_op ||
              GIMPLE_LABEL == current_op || GIMPLE_GOTO == current_op || GIMPLE_PRAGMA == current_op || ENTRY == current_op || EXIT == current_op || NOP == current_op || GIMPLE_PHI == current_op || GIMPLE_NOP == current_op ||
              VIEW_CONVERT_EXPR == current_op || EXTRACT_BIT_EXPR == current_op || LUT_EXPR == current_op)
      {
         unsigned int current_size = allocation_information->get_number_fu_types();
         if(current_op == ASSIGN)
         {
            unsigned int modify_tree_index = g->CGetOpNodeInfo(*v)->GetNodeId();
            tree_nodeRef modify_node = TreeM->get_tree_node_const(modify_tree_index);
            auto* gms = GetPointer<gimple_assign>(modify_node);
            unsigned int left_type_index;
            tree_nodeRef left_type_node = tree_helper::get_type_node(GET_NODE(gms->op0), left_type_index);
            if(tree_helper::is_a_complex(TreeM, left_type_index))
               current_fu = get_fu(ASSIGN_VECTOR_BOOL_STD);
            else if(tree_helper::is_int(TreeM, left_type_index))
               current_fu = get_fu(ASSIGN_SIGNED_STD);
            else if(tree_helper::is_real(TreeM, left_type_index))
               current_fu = get_fu(ASSIGN_REAL_STD);
            else if(tree_helper::is_a_vector(TreeM, left_type_index))
            {
               const unsigned int element_type = tree_helper::GetElements(TreeM, left_type_index);
               if(tree_helper::is_int(TreeM, element_type))
                  current_fu = get_fu(ASSIGN_VEC_SIGNED_STD);
               else if(tree_helper::is_unsigned(TreeM, element_type))
                  current_fu = get_fu(ASSIGN_VEC_UNSIGNED_STD);
               else
               {
                  THROW_ERROR("unexpected type");
               }
            }
            else
               current_fu = get_fu(ASSIGN_UNSIGNED_STD);
         }
         else if(current_op == ASSERT_EXPR)
         {
            unsigned int modify_tree_index = g->CGetOpNodeInfo(*v)->GetNodeId();
            tree_nodeRef modify_node = TreeM->get_tree_node_const(modify_tree_index);
            auto* gms = GetPointer<gimple_assign>(modify_node);
            unsigned int left_type_index;
            tree_nodeRef left_type_node = tree_helper::get_type_node(GET_NODE(gms->op0), left_type_index);
            if(tree_helper::is_int(TreeM, left_type_index))
               current_fu = get_fu(ASSERT_EXPR_SIGNED_STD);
            else if(tree_helper::is_real(TreeM, left_type_index))
               current_fu = get_fu(ASSERT_EXPR_REAL_STD);
            else
               current_fu = get_fu(ASSERT_EXPR_UNSIGNED_STD);
         }
         else if(current_op == EXTRACT_BIT_EXPR)
         {
            unsigned int modify_tree_index = g->CGetOpNodeInfo(*v)->GetNodeId();
            tree_nodeRef modify_node = TreeM->get_tree_node_const(modify_tree_index);
            auto* gms = GetPointer<gimple_assign>(modify_node);
            auto ebe = GetPointer<extract_bit_expr>(GET_NODE(gms->op1));
            ;
            bool intOP0 = tree_helper::is_int(TreeM, GET_INDEX_NODE(ebe->op0));
            if(intOP0)
               current_fu = get_fu(EXTRACT_BIT_EXPR_SIGNED_STD);
            else
               current_fu = get_fu(EXTRACT_BIT_EXPR_UNSIGNED_STD);
         }
         else if(current_op == LUT_EXPR)
            current_fu = get_fu(LUT_EXPR_STD);
         else if(current_op == ADDR_EXPR)
            current_fu = get_fu(ADDR_EXPR_STD);
         else if(current_op == NOP_EXPR)
         {
            unsigned int modify_tree_index = g->CGetOpNodeInfo(*v)->GetNodeId();

            // std::cout << NOP_EXPR << "->" << modify_tree_index << std::endl;
            tree_nodeRef modify_node = TreeM->get_tree_node_const(modify_tree_index);
            auto* gms = GetPointer<gimple_assign>(modify_node);

            unsigned int left_type_index;
            tree_nodeRef left_type_node = tree_helper::get_type_node(GET_NODE(gms->op0), left_type_index);
            auto* ne = GetPointer<nop_expr>(GET_NODE(gms->op1));
            unsigned int right_type_index;
            tree_nodeRef right_type_node = tree_helper::get_type_node(GET_NODE(ne->op), right_type_index);

            bool unsignedR = tree_helper::is_unsigned(TreeM, right_type_index);
            bool unsignedL = tree_helper::is_unsigned(TreeM, left_type_index);
            bool intR = tree_helper::is_int(TreeM, right_type_index);
            bool intL = tree_helper::is_int(TreeM, left_type_index);
            bool enumR = tree_helper::is_an_enum(TreeM, right_type_index);
            bool enumL = tree_helper::is_an_enum(TreeM, left_type_index);
            bool boolR = tree_helper::is_bool(TreeM, right_type_index);
            bool boolL = tree_helper::is_bool(TreeM, left_type_index);
            bool is_a_pointerR = tree_helper::is_a_pointer(TreeM, right_type_index);
            bool is_a_pointerL = tree_helper::is_a_pointer(TreeM, left_type_index);
            bool is_realR = tree_helper::is_real(TreeM, right_type_index);
            bool is_realL = tree_helper::is_real(TreeM, left_type_index);

            bool vector_boolR = tree_helper::is_a_vector(TreeM, right_type_index) and tree_helper::is_bool(TreeM, tree_helper::GetElements(TreeM, tree_helper::get_type_index(TreeM, right_type_index)));
            bool vector_intL = tree_helper::is_a_vector(TreeM, left_type_index) and tree_helper::is_int(TreeM, tree_helper::GetElements(TreeM, tree_helper::get_type_index(TreeM, left_type_index)));
            bool vector_unsignedL = tree_helper::is_a_vector(TreeM, left_type_index) and tree_helper::is_unsigned(TreeM, tree_helper::GetElements(TreeM, tree_helper::get_type_index(TreeM, left_type_index)));

            if((unsignedR || is_a_pointerR || boolR) && (unsignedL || is_a_pointerL || boolL))
               current_fu = get_fu(UUDATA_CONVERTER_STD);
            else if((intR || enumR) && (unsignedL || is_a_pointerL || boolL))
               current_fu = get_fu(IUDATA_CONVERTER_STD);
            else if((unsignedR || is_a_pointerR || boolR) && (intL || enumL))
               current_fu = get_fu(UIDATA_CONVERTER_STD);
            else if((intR || enumR) && (intL || enumL))
               current_fu = get_fu(IIDATA_CONVERTER_STD);
            else if(is_realR && is_realL)
            {
               if(!skip_flopoco_resources)
                  current_fu = get_fu(FFDATA_CONVERTER_STD);
               else if(!skip_softfloat_resources)
               {
                  unsigned int prec_in = tree_helper::size(TreeM, right_type_index);
                  unsigned int prec_out = tree_helper::size(TreeM, left_type_index);
                  allocation_information->extract_bambu_provided_name(prec_in, prec_out, HLSMgr, current_fu);
               }
               else
                  THROW_ERROR("missing resource for floating point to floating point conversion");
            }
            else if(vector_boolR and vector_intL)
            {
               current_fu = get_fu(BIVECTOR_CONVERTER_STD);
            }
            else if(vector_boolR and vector_unsignedL)
            {
               current_fu = get_fu(BUVECTOR_CONVERTER_STD);
            }
            else
               THROW_ERROR(std::string("Nop_Expr pattern not supported ") + STR(modify_tree_index) + " - Left type " + STR(left_type_index) + " is " + TreeM->get_tree_node_const(left_type_index)->ToString() + " - Right type " + STR(right_type_index) +
                           " is " + TreeM->get_tree_node_const(right_type_index)->ToString());
         }
         else if(current_op == CONVERT_EXPR)
         {
            unsigned int modify_tree_index = g->CGetOpNodeInfo(*v)->GetNodeId();

            // std::cout << CONVERT_EXPR << "->" << modify_tree_index << std::endl;
            tree_nodeRef modify_node = TreeM->get_tree_node_const(modify_tree_index);
            auto* gms = GetPointer<gimple_assign>(modify_node);
            unsigned int left_type_index;
            tree_nodeRef left_type_node = tree_helper::get_type_node(GET_NODE(gms->op0), left_type_index);
            auto* ce = GetPointer<convert_expr>(GET_NODE(gms->op1));
            unsigned int right_type_index;
            tree_nodeRef right_type_node = tree_helper::get_type_node(GET_NODE(ce->op), right_type_index);

            bool unsignedR = tree_helper::is_unsigned(TreeM, right_type_index);
            bool unsignedL = tree_helper::is_unsigned(TreeM, left_type_index);
            bool intR = tree_helper::is_int(TreeM, right_type_index);
            bool intL = tree_helper::is_int(TreeM, left_type_index);
            bool boolR = tree_helper::is_bool(TreeM, right_type_index);
            bool boolL = tree_helper::is_bool(TreeM, left_type_index);
            bool is_a_pointerR = tree_helper::is_a_pointer(TreeM, right_type_index);
            bool is_a_pointerL = tree_helper::is_a_pointer(TreeM, left_type_index);

            if((unsignedR || is_a_pointerR || boolR) && (unsignedL || is_a_pointerL || boolL))
               current_fu = get_fu(UUCONVERTER_EXPR_STD);
            else if(intR && (unsignedL || is_a_pointerL || boolL))
               current_fu = get_fu(IUCONVERTER_EXPR_STD);
            else if((unsignedR || is_a_pointerR || boolR) && intL)
               current_fu = get_fu(UICONVERTER_EXPR_STD);
            else if(intR && intL)
               current_fu = get_fu(IICONVERTER_EXPR_STD);
            else
               THROW_UNREACHABLE("CONVERT_EXPR pattern not supported in statement " + STR(modify_tree_index) + ". Left type is " + STR(left_type_index) + " " + STR(TreeM->CGetTreeNode(left_type_index)) + " - Right type is " + STR(right_type_index) + " " +
                                 STR(TreeM->CGetTreeNode(right_type_index)));
         }
         else if(current_op == READ_COND)
            current_fu = get_fu(READ_COND_STD);
         else if(current_op == MULTI_READ_COND)
            current_fu = get_fu(MULTI_READ_COND_STD);
         else if(current_op == SWITCH_COND)
            current_fu = get_fu(SWITCH_COND_STD);
         else if(current_op == GIMPLE_LABEL)
            current_fu = get_fu(GIMPLE_LABEL_STD);
         else if(current_op == GIMPLE_GOTO)
            current_fu = get_fu(GIMPLE_GOTO_STD);
         else if(current_op == GIMPLE_PRAGMA)
            current_fu = get_fu(GIMPLE_PRAGMA_STD);
         else if(current_op == ENTRY)
            current_fu = get_fu(ENTRY_STD);
         else if(current_op == EXIT)
            current_fu = get_fu(EXIT_STD);
         else if(current_op == NOP)
            current_fu = get_fu(NOP_STD);
         else if(current_op == GIMPLE_PHI)
            current_fu = get_fu(GIMPLE_PHI_STD);
         else if(current_op == GIMPLE_NOP)
            current_fu = get_fu(GIMPLE_NOP_STD);
         else if(current_op == VIEW_CONVERT_EXPR)
         {
            unsigned int modify_tree_index = g->CGetOpNodeInfo(*v)->GetNodeId();
            tree_nodeRef modify_node = TreeM->get_tree_node_const(modify_tree_index);
            auto* gms = GetPointer<gimple_assign>(modify_node);
            auto* vce = GetPointer<view_convert_expr>(GET_NODE(gms->op1));
            unsigned int right_type_index;
            tree_nodeRef right_type_node = tree_helper::get_type_node(GET_NODE(vce->op), right_type_index);
            if(tree_helper::is_int(TreeM, right_type_index))
               current_fu = get_fu(VIEW_CONVERT_STD_INT);
            else if(tree_helper::is_real(TreeM, right_type_index))
               current_fu = get_fu(VIEW_CONVERT_STD_REAL);
            else
               current_fu = get_fu(VIEW_CONVERT_STD_UINT);
         }
         else
            THROW_ERROR("Unexpected operation");
         // FU must exist
         THROW_ASSERT(current_fu, std::string("Not found ") + current_op + " in library " + TM->get_library(current_op));
         unsigned int current_id;
#if 0
         if(artificial_allocation.find(current_fu) == artificial_allocation.end())
         {
            allocation_information->list_of_FU.push_back(current_fu);
            allocation_information->tech_constraints.push_back(INFINITE_UINT);
            current_id = current_size;
            artificial_allocation[current_fu] = current_id;
         }
         else
            current_id = artificial_allocation.find(current_fu)->second;
#else
         allocation_information->list_of_FU.push_back(current_fu);
         allocation_information->tech_constraints.push_back(1);
         current_id = current_size;
#endif
         THROW_ASSERT(allocation_information->tech_constraints.size() == allocation_information->list_of_FU.size(), "Something of wrong happen");
#if 1
         allocation_information->is_vertex_bounded_rel.insert(current_id);
         allocation_information->binding[node_id] = std::pair<std::string, unsigned int>(current_op, current_id);
#endif
         allocation_information->node_id_to_fus[std::pair<unsigned int, std::string>(node_id, node_operation)].insert(current_id);
         allocation_information->id_to_fu_names[current_id] = std::make_pair(current_fu->get_name(), TM->get_library(current_op));
         unsigned int out_var = HLSMgr->get_produced_value(HLS->functionId, *v);
         if(out_var)
         {
            unsigned int type_index = tree_helper::get_type_index(TreeM, out_var);
            if(tree_helper::is_a_vector(TreeM, type_index))
            {
               const unsigned int element_type = tree_helper::GetElements(TreeM, type_index);
               const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TreeM, element_type));
               allocation_information->precision_map[current_size] = element_size;
            }
            else
               allocation_information->precision_map[current_size] = tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, out_var));
         }
         else
            allocation_information->precision_map[current_size] = 0;
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  . Operation " + current_op + " mapped onto " + current_fu->get_name() + ", found in library " + TM->get_library(current_op));
      }
      // Constrained FUs
      else if(binding_constraints.find(GET_NAME(g, *v)) != binding_constraints.end())
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  . Current node is under constraints");
         const std::pair<std::string, std::pair<std::string, unsigned int>>& defined_binding = binding_constraints.find(GET_NAME(g, *v))->second;
         std::string fu_name = defined_binding.first;
         std::string fu_library = defined_binding.second.first;
         unsigned int fu_index = defined_binding.second.second;
         std::string key = ENCODE_FU_LIB(fu_name, fu_library);
         if(fu_name_to_id.find(key) != fu_name_to_id.end())
         {
            if(fu_name_to_id[key].find(fu_index) != fu_name_to_id[key].end())
               allocation_information->binding[node_id] = std::pair<std::string, unsigned int>(current_op, fu_name_to_id[key][fu_index]);
            else
            {
               unsigned int current_size = allocation_information->get_number_fu_types();
               fu_name_to_id[key][fu_index] = current_size;
               current_fu = TM->get_fu(fu_name, fu_library);
               THROW_ASSERT(current_fu, std::string("Not found") + fu_name + " in library " + fu_library);
               allocation_information->list_of_FU.push_back(current_fu);
               allocation_information->tech_constraints.push_back(1);
               allocation_information->binding[node_id] = std::pair<std::string, unsigned int>(current_op, current_size);
               allocation_information->node_id_to_fus[std::pair<unsigned int, std::string>(node_id, node_operation)].insert(current_size);
               allocation_information->id_to_fu_names[current_size] = std::make_pair(fu_name, fu_library);
               if(tech_vec.find(key) != tech_vec.end())
                  tech_vec[key]--;
               unsigned int out_var = HLSMgr->get_produced_value(HLS->functionId, *v);
               if(out_var)
               {
                  unsigned int type_index = tree_helper::get_type_index(TreeM, out_var);
                  if(tree_helper::is_a_vector(TreeM, type_index))
                  {
                     const unsigned int element_type = tree_helper::GetElements(TreeM, type_index);
                     const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TreeM, element_type));
                     allocation_information->precision_map[current_size] = element_size;
                  }
                  else
                     allocation_information->precision_map[current_size] = tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, out_var));
               }
               else
                  allocation_information->precision_map[current_size] = 0;
            }
         }
         else
         {
            unsigned int current_size = allocation_information->get_number_fu_types();
            fu_name_to_id[key][fu_index] = current_size;
            current_fu = TM->get_fu(fu_name, fu_library);
            THROW_ASSERT(current_fu, std::string("Not found") + fu_name + " in library " + fu_library);
            allocation_information->list_of_FU.push_back(current_fu);
            allocation_information->tech_constraints.push_back(1);
            allocation_information->binding[node_id] = std::pair<std::string, unsigned int>(current_op, current_size);
            allocation_information->node_id_to_fus[std::pair<unsigned int, std::string>(node_id, node_operation)].insert(current_size);
            allocation_information->id_to_fu_names[current_size] = std::make_pair(fu_name, fu_library);
            if(tech_vec.find(key) != tech_vec.end())
               tech_vec[key]--;
            unsigned int out_var = HLSMgr->get_produced_value(HLS->functionId, *v);
            if(out_var)
            {
               unsigned int type_index = tree_helper::get_type_index(TreeM, out_var);
               if(tree_helper::is_a_vector(TreeM, type_index))
               {
                  const unsigned int element_type = tree_helper::GetElements(TreeM, type_index);
                  const unsigned int element_size = static_cast<unsigned int>(tree_helper::size(TreeM, element_type));
                  allocation_information->precision_map[current_size] = element_size;
               }
               else
                  allocation_information->precision_map[current_size] = tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, out_var));
            }
            else
               allocation_information->precision_map[current_size] = 0;
         }
      }
      else
      {
         vertex_to_analyse.insert(*v);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Operation " + current_op + " queued for allocation");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Starting allocation of operations in queued vertices");

   for(const auto& tv : tech_vec)
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_VERY_PEDANTIC, output_level, "---Resource constraint on " + tv.first + ": " + STR(tv.second));

   std::string bambu_provided_resource;
   for(const auto& lib_name : TM->get_library_list())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering library " + lib_name);
      const library_managerRef library = TM->get_library_manager(lib_name);
      /// skip library of internal components
      if(lib_name == "STD_COMMON")
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered library " + lib_name);
         continue;
      }
      for(const auto& fu : library->get_library_fu())
      {
         technology_nodeRef current_fu = fu.second;
         if(GetPointer<functional_unit_template>(current_fu))
            continue;

         std::string channels_type = GetPointer<functional_unit>(current_fu)->channels_type;
         std::string memory_ctrl_type = GetPointer<functional_unit>(current_fu)->memory_ctrl_type;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering functional unit: " + current_fu->get_name());
         if(check_for_memory_compliancy(Has_extern_allocated_data, current_fu, memory_ctrl_type, channels_type))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped for memory compliance");
            continue;
         }

         /// check proxy functions
         if((HLSMgr->Rfuns->has_shared_functions(HLS->functionId) || HLSMgr->Rfuns->has_proxied_shared_functions(HLS->functionId)))
         {
            if(check_proxies(library, fu.first))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because proxy");
               continue;
            }
         }
         else if(lib_name == PROXY_LIBRARY)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because proxy");
            continue;
         }

         const auto tech_constrain_it =
             GetPointer<functional_unit>(current_fu)->fu_template_name != "" ? tech_vec.find(ENCODE_FU_LIB(GetPointer<functional_unit>(current_fu)->fu_template_name, lib_name)) : tech_vec.find(ENCODE_FU_LIB(current_fu->get_name(), lib_name));

         if(tech_constrain_it != tech_vec.end() && tech_constrain_it->second == 0)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because of constraint");
            continue; // forced to use 0 FUs of current ones
         }

         unsigned int tech_constrain_value = tech_constrain_it == tech_vec.end() ? INFINITE_UINT : tech_constrain_it->second;

         bambu_provided_resource = "";
         structural_managerRef structManager_obj = GetPointer<functional_unit>(current_fu)->CM;

         /// check for generated module and bambu/flopoco resources
         if(check_generated_bambu_flopoco(skip_softfloat_resources, structManager_obj, bambu_provided_resource, skip_flopoco_resources, current_fu))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because of generated module");
            continue;
         }

         unsigned int current_fu_id = allocation_information->get_number_fu_types();

         unsigned int current_id = current_fu_id;

         bool lib_is_proxy_or_work = lib_name == WORK_LIBRARY || lib_name == PROXY_LIBRARY || lib_name == INTERFACE_LIBRARY;
         for(const auto& ops : GetPointer<functional_unit>(current_fu)->get_operations())
         {
            auto* curr_op = GetPointer<operation>(ops);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering operation: " + (ops)->get_name());
            std::string curr_op_name = curr_op->get_name();
            for(const auto vert : vertex_to_analyse)
            {
               const auto vert_node_id = g->CGetOpNodeInfo(vert)->GetNodeId();
               const auto vert_node_operation = [&]() -> std::string {
                  if(vert_node_id == ENTRY_ID)
                     return "Entry";
                  if(vert_node_id == EXIT_ID)
                     return "Exit";
                  return GetPointer<const gimple_node>(TreeM->CGetTreeNode(vert_node_id))->operation;
               }();
               if(tree_helper::normalized_ID(g->CGetOpNodeInfo(vert)->GetOperation()) != curr_op_name)
                  continue;
               else if((not lib_is_proxy_or_work) && TM->get_fu(tree_helper::normalized_ID(g->CGetOpNodeInfo(vert)->GetOperation()), WORK_LIBRARY) && GET_TYPE(g, vert) != TYPE_MEMCPY)
                  continue;
               // else if(lib_is_proxy_or_work && GET_TYPE(g, vert) == TYPE_MEMCPY) continue;

               node_kind_prec_infoRef node_info(new node_kind_prec_info());
               HLS_manager::io_binding_type constant_id;
               bool isMemory = memory_ctrl_type != "";
               THROW_ASSERT((GET_TYPE(g, vert) & (TYPE_LOAD | TYPE_STORE)) == 0 or isMemory, "unexpected condition: " + g->CGetOpNodeInfo(vert)->GetOperation());

               if(!isMemory)
               {
                  allocation_information->GetNodeTypePrec(vert, g, node_info, constant_id, tech_constrain_value != INFINITE_UINT);
               }
               else
                  node_info->node_kind = "VECTOR_BOOL";

               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering vertex " + GET_NAME(g, vert));
               /// Check for correct type and precision
               if(check_type_and_precision(curr_op, node_info))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because of type and precision");
                  continue;
               }

               /// in case the operation is pipelined check the clock period
               THROW_ASSERT(curr_op->time_m, "expected a time model for " + current_fu->get_name() + " for operation " + curr_op_name);
               if(curr_op->time_m->get_cycles() >= 1 && allocation_information->time_m_stage_period(curr_op) > clock_period && memory_ctrl_type == "" && GetPointer<functional_unit>(current_fu)->fu_template_name == "")
               {
                  THROW_ERROR("Functional unit " + current_fu->get_name() + " not compliant with the given clock period " + STR(clock_period));
                  // continue;
               }
               else if(curr_op->time_m->get_cycles() >= 1)
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                "Functional unit " + current_fu->get_name() + " compliant with the given clock period " + STR(clock_period) + " stage period " + STR(allocation_information->time_m_stage_period(curr_op)));

               if(GetPointer<functional_unit>(current_fu)->fu_template_name != "")
               {
                  // check if specialized unit is compliant with the vertex
                  if(check_templated_units(clock_period, node_info, library, current_fu, curr_op))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because of template");
                     continue;
                  }
               }

               std::string current_op;
               std::string specialized_fuName = "";

               bool has_to_be_generated = structManager_obj && (GetPointer<module>(structManager_obj->get_circ())->get_NP_functionality()->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR) ||
                                                                GetPointer<module>(structManager_obj->get_circ())->get_NP_functionality()->exist_NP_functionality(NP_functionality::VHDL_GENERATOR));
               if(has_to_be_generated)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Unit has to be specialized");
                  bool varargs_fu = GetPointer<module>(structManager_obj->get_circ())->is_var_args();
                  moduleGeneratorRef modGen = moduleGeneratorRef(new moduleGenerator(HLSMgr, parameters));
                  if(varargs_fu)
                  {
                     std::vector<HLS_manager::io_binding_type> required_variables = HLSMgr->get_required_values(funId, vert);
                     std::string asm_unique_id;
                     if(g->CGetOpNodeInfo(vert)->GetOperation() == GIMPLE_ASM)
                        asm_unique_id = STR(g->CGetOpNodeInfo(vert)->GetNodeId());
                     unsigned int firstIndexToSpecialize = 0;
                     auto mod = GetPointer<module>(structManager_obj->get_circ());
                     for(auto Pindex = 0u; Pindex < mod->get_in_port_size(); ++Pindex)
                     {
                        const structural_objectRef& port_obj = mod->get_in_port(Pindex);
                        auto port_name = port_obj->get_id();
                        if(GetPointer<port_o>(port_obj)->get_is_var_args())
                           break;
                        if(port_name != CLOCK_PORT_NAME && port_name != RESET_PORT_NAME && port_name != START_PORT_NAME)
                           ++firstIndexToSpecialize;
                     }
                     THROW_ASSERT(required_variables.size() >= firstIndexToSpecialize, "unexpected condition:" + STR(required_variables.size()) + " " + STR(firstIndexToSpecialize));
                     current_op = current_fu->get_name() + asm_unique_id + modGen->get_specialized_name(firstIndexToSpecialize, required_variables, function_behavior);
                  }
                  else
                     current_op = current_fu->get_name() + "_modgen";
                  specialized_fuName = current_op;
                  std::string fu_name = current_fu->get_name();

                  std::string check_lib = TM->get_library(specialized_fuName);
                  if(check_lib == lib_name)
                  {
                     new_fu[specialized_fuName] = get_fu(specialized_fuName);
                  }
                  else if(new_fu.find(specialized_fuName) == new_fu.end())
                  {
                     if(varargs_fu)
                        modGen->specialize_fu(fu_name, vert, lib_name, TM, function_behavior, specialized_fuName, new_fu, HLS_T->get_target_device()->get_type());
                     else
                     {
                        modGen->create_generic_module(fu_name, lib_name, TM, specialized_fuName, HLS_T->get_target_device()->get_type(), HLSMgr);
                        const library_managerRef libraryManager = TM->get_library_manager(lib_name);
                        technology_nodeRef new_techNode_obj = libraryManager->get_fu(specialized_fuName);
                        THROW_ASSERT(new_techNode_obj, "not expected");
                        new_fu.insert(std::make_pair(specialized_fuName, new_techNode_obj));
                     }
                  }
               }
               else if(node_info->node_kind != "" && !isMemory)
                  current_op = encode_op_type_prec(curr_op_name, curr_op->get_type_supported_string(), node_info);
               else if(node_info->node_kind != "")
                  current_op = encode_op_type(curr_op_name, curr_op->get_type_supported_string());
               else
                  current_op = curr_op_name;

               std::string library_name = lib_name;
               if(bambu_provided_resource != "")
               {
                  if(HLSMgr->Rfuns->is_a_proxied_function(functions::get_function_name_cleaned(bambu_provided_resource)))
                     library_name = PROXY_LIBRARY;
                  else
                     library_name = WORK_LIBRARY;

                  current_fu = extract_bambu_provided(library_name, curr_op, bambu_provided_resource);
               }

               unsigned int max_prec = node_info->input_prec.begin() == node_info->input_prec.end() ? 0 : *std::max_element(node_info->input_prec.begin(), node_info->input_prec.end());
               if(isMemory || lib_is_proxy_or_work || tech_constrain_value != INFINITE_UINT || bambu_provided_resource != "")
               {
                  constant_id = HLS_manager::io_binding_type(0, 0);
                  max_prec = 0;
               }

               std::map<technology_nodeRef, std::map<unsigned int, std::map<HLS_manager::io_binding_type, unsigned int>>>::iterator techMap;
               std::string functionalUnitName = "";
               unsigned int specializedId = current_id;
               const library_managerRef libraryManager = TM->get_library_manager(library_name);
               if(has_to_be_generated)
               {
                  functionalUnitName = specialized_fuName;
                  techMap = fu_list.find(new_fu.find(functionalUnitName)->second);
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Specialized unit: " + new_fu.find(functionalUnitName)->first);
                  if(techMap != fu_list.end() && techMap->second.find(max_prec) != techMap->second.end() && techMap->second.find(max_prec)->second.find(constant_id) != techMap->second.find(max_prec)->second.end())
                  {
                     specializedId = techMap->second.find(max_prec)->second.find(constant_id)->second;
                  }
                  else
                  {
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "Insert into list of unit to add: " + new_fu.find(functionalUnitName)->first + " prec=" + STR(max_prec) + " constant_id=" + STR(std::get<0>(constant_id)) + "-" + STR(std::get<1>(constant_id)));
                     fu_list[new_fu.find(functionalUnitName)->second][max_prec][constant_id] = current_id;
                     allocation_information->precision_map[current_id] = max_prec;
                     if(channels_type == CHANNELS_TYPE_MEM_ACC_NN && memory_ctrl_type != "")
                     {
                        auto n_ports = parameters->getOption<unsigned int>(OPT_channels_number);
                        set_number_channels(specializedId, n_ports);
                     }
                     else if(memory_ctrl_type != "")
                        set_number_channels(specializedId, 1);
                     auto fuUnit = new_fu.find(functionalUnitName)->second;
                     if(fuUnit->get_kind() == functional_unit_K)
                     {
                        auto fuUnitModule = GetPointer<functional_unit>(fuUnit)->CM->get_circ();
                        if(GetPointer<module>(fuUnitModule))
                        {
                           auto multiplicity = GetPointer<module>(fuUnitModule)->get_multi_unit_multiplicity();
                           if(multiplicity)
                           {
                              PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Added multiplicity of " + STR(multiplicity) + " to " + functionalUnitName);
                              set_number_channels(specializedId, multiplicity);
                           }
                        }
                     }
                     add_tech_constraint(fuUnit, tech_constrain_value, current_id, library_name == PROXY_LIBRARY);
                     current_id++;
                  }
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not generated functional unit");
                  functionalUnitName = current_fu->get_name();
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Functional unit name is " + functionalUnitName);
                  techMap = fu_list.find(libraryManager->get_fu(functionalUnitName));
                  if(techMap != fu_list.end() && techMap->second.find(max_prec) != techMap->second.end() && techMap->second.find(max_prec)->second.find(constant_id) != techMap->second.find(max_prec)->second.end())
                  {
                     specializedId = techMap->second.find(max_prec)->second.find(constant_id)->second;
                  }
                  else
                  {
                     fu_list[libraryManager->get_fu(functionalUnitName)][max_prec][constant_id] = current_id;
                     allocation_information->precision_map[current_id] = max_prec;
                     if(channels_type == CHANNELS_TYPE_MEM_ACC_P1N and memory_ctrl_type != "")
                     {
                        auto n_ports = parameters->getOption<unsigned int>(OPT_memory_banks_number);
                        set_number_channels(specializedId, n_ports);
                     }
                     else if(channels_type == CHANNELS_TYPE_MEM_ACC_NN && memory_ctrl_type != "")
                     {
                        auto n_ports = parameters->getOption<unsigned int>(OPT_channels_number);
                        set_number_channels(specializedId, n_ports);
                     }
                     else if(memory_ctrl_type != "")
                        set_number_channels(specializedId, 1);
                     add_tech_constraint(libraryManager->get_fu(functionalUnitName), tech_constrain_value, current_id, library_name == PROXY_LIBRARY);
                     current_id++;
                  }
               }

               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "    . Match found for vertex: " + GET_NAME(g, vert));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "      . Adding candidate FU: " + functionalUnitName + " for operation: " + current_op + " in position " + STR(specializedId));
               vertex_analysed.insert(vert);
               allocation_information->node_id_to_fus[std::pair<unsigned int, std::string>(vert_node_id, vert_node_operation)].insert(specializedId);
               allocation_information->id_to_fu_names[specializedId] = std::make_pair(functionalUnitName, library_name);
               if(node_info->is_single_bool_test_cond_expr)
                  allocation_information->single_bool_test_cond_expr_units.insert(specializedId);
               if(node_info->is_simple_pointer_plus_expr)
                  allocation_information->simple_pointer_plus_expr.insert(specializedId);
               if(library_name == PROXY_LIBRARY)
               {
                  if(boost::algorithm::starts_with(functionalUnitName, WRAPPED_PROXY_PREFIX))
                  {
                     std::string original_function_name = functionalUnitName.substr(std::string(WRAPPED_PROXY_PREFIX).size());
                     allocation_information->proxy_wrapped_units[specializedId] = original_function_name;
                  }
                  else
                  {
                     THROW_ASSERT(functionalUnitName.compare(0, std::string(PROXY_PREFIX).size(), PROXY_PREFIX) == 0, "expected a proxy module");
                     std::string original_function_name = functionalUnitName.substr(std::string(PROXY_PREFIX).size());
                     allocation_information->proxy_function_units[specializedId] = original_function_name;
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered vertex " + GET_NAME(g, vert));
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered operation: " + (ops)->get_name());
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered functional unit: " + current_fu->get_name());
      }
      for(auto iter_new_fu = new_fu.begin(); iter_new_fu != new_fu.end(); ++iter_new_fu)
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding functional unit: " + iter_new_fu->first + " in " + lib_name);
         TM->add(iter_new_fu->second, lib_name);
      }
      new_fu.clear();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered library " + lib_name);
   }

#ifndef NDEBUG
   // For debug purpose
   allocation_information->print_allocated_resources();
#endif

   // Check if each operation has been analysed
   for(const auto ve : vertex_to_analyse)
   {
      if(vertex_analysed.find(ve) != vertex_analysed.end())
         continue;

      node_kind_prec_infoRef node_info(new node_kind_prec_info());
      HLS_manager::io_binding_type constant_id;
      allocation_information->GetNodeTypePrec(ve, g, node_info, constant_id, false);
      std::string precisions;
      const size_t n_ins = node_info->input_prec.size();
      for(size_t ind = 0; ind < n_ins; ++ind)
      {
         if(node_info->real_input_nelem[ind] == 0)
            precisions += " " + STR(node_info->input_prec[ind]);
         else
            precisions += " " + STR(node_info->input_prec[ind]) + ":" + STR(node_info->real_input_nelem[ind]);
      }
      if(node_info->real_output_nelem == 0)
         precisions += " " + STR(node_info->output_prec);
      else
         precisions += " " + STR(node_info->output_prec) + ":" + STR(node_info->real_output_nelem);
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                     "---Operation for which does not exist a functional unit in the resource library: " + tree_helper::normalized_ID(g->CGetOpNodeInfo(ve)->GetOperation()) + " in vertex: " + GET_NAME(g, ve) + " with vertex type: " + node_info->node_kind +
                         " and vertex prec:" + precisions);
   }
   if(vertex_to_analyse.size() > static_cast<size_t>(vertex_analysed.size()))
      THROW_ERROR("Vertices not completely allocated");
   /// These data structure are filled only once
   if(!allocation_information->node_id_to_fus.empty())
   {
      for(const auto& op : allocation_information->node_id_to_fus)
      {
         for(auto fu_unit : op.second)
         {
            allocation_information->n_complex_operations += static_cast<unsigned int>(allocation_information->get_DSPs(fu_unit));
            allocation_information->fus_to_node_id[fu_unit].insert(op.first.first);
         }
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of complex operations: " + STR(allocation_information->n_complex_operations));
   }
   if(!allocation_information->node_id_to_fus.empty() and bb_version == 0)
   {
      for(const auto& op : allocation_information->node_id_to_fus)
      {
         for(auto fu_unit : op.second)
         {
            if(allocation_information->memory_units.find(fu_unit) != allocation_information->memory_units.end())
               HLSMgr->Rmem->increment_n_mem_operations(allocation_information->memory_units.find(fu_unit)->second);
            else if(allocation_information->proxy_memory_units.find(fu_unit) != allocation_information->proxy_memory_units.end())
               HLSMgr->Rmem->increment_n_mem_operations(allocation_information->proxy_memory_units.find(fu_unit)->second);
         }
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of complex operations: " + STR(allocation_information->n_complex_operations));
   }
#if 0
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of functional unit types: " + STR(allocation_information->tech_constraints.size()));
   if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "-->");
      for(decltype(allocation_information->get_number_fu_types()) index_fu = 0u; index_fu < allocation_information->get_number_fu_types(); index_fu++)
      {
         INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "---" + allocation_information->get_fu_name(index_fu).first);
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "<--");
   }
#endif
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      STOP_TIME(step_time);
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Time to perform module allocation: " + print_cpu_time(step_time) + " seconds");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");

   const CallGraphManagerConstRef call_graph_manager = HLSMgr->CGetCallGraphManager();
   const auto called_functions = call_graph_manager->GetReachedBodyFunctionsFrom(funId);
   for(auto const called_function : called_functions)
   {
      if(called_function == funId)
         continue;
      const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(called_function);
      last_bb_ver[called_function] = FB->GetBBVersion();
   }

   return DesignFlowStep_Status::SUCCESS;
}

std::string allocation::get_compliant_pipelined_unit(double clock, const std::string& pipe_parameter, const technology_nodeRef current_fu, const std::string& curr_op, const std::string& library_name, const std::string& template_suffix,
                                                     unsigned int module_prec)
{
   if(pipe_parameter == "")
      return "";
   THROW_ASSERT(GetPointer<functional_unit>(current_fu), "expected a functional unit object");
   auto* fu = GetPointer<functional_unit>(current_fu);

   THROW_ASSERT(fu->fu_template_name != "", "expected a template_name for a pipelined unit");
   if(precomputed_pipeline_unit.find(fu->fu_template_name + "_" + template_suffix) != precomputed_pipeline_unit.end())
   {
      std::string compliant_id = precomputed_pipeline_unit.find(fu->fu_template_name + "_" + template_suffix)->second;
      if(pipe_parameter == compliant_id)
         return pipe_parameter;
      else
         return "";
   }
   const technology_nodeRef fu_template = HLS_T->get_technology_manager()->get_fu(fu->fu_template_name, library_name);
   const functional_unit_template* fu_temp = GetPointer<functional_unit_template>(fu_template);
   THROW_ASSERT(fu_temp, "expected a template functional unit for a pipelined unit");
   bool is_flopoco_provided = false;
   structural_managerRef tcm = GetPointer<functional_unit>(fu_temp->FU)->CM;
   if(tcm)
   {
      structural_objectRef tmodobj = tcm->get_circ();
      auto* tmod = GetPointer<module>(tmodobj);
      const NP_functionalityRef& np = tmod->get_NP_functionality();
      if(np->get_NP_functionality(NP_functionality::FLOPOCO_PROVIDED) != "")
         is_flopoco_provided = true;
   }
   technology_nodeRef fun_temp_operation = GetPointer<functional_unit>(fu_temp->FU)->get_operation(curr_op);
   THROW_ASSERT(fun_temp_operation, "operation not present in the template description");
   auto* template_op = GetPointer<operation>(fun_temp_operation);
   std::string temp_pipe_parameters = template_op->pipe_parameters;
   std::vector<std::string> parameters_split = SplitString(temp_pipe_parameters, "|");
   THROW_ASSERT(parameters_split.size() > 0, "unexpected pipe_parameter format");
   for(auto& el_indx : parameters_split)
   {
      std::vector<std::string> parameters_pairs = SplitString(el_indx, ":");
      if(parameters_pairs[0] == "*")
      {
         temp_pipe_parameters = parameters_pairs[1];
         break;
      }
      else if(parameters_pairs[0] == "DSPs_y_sizes" and std::find(allocation_information->DSP_y_db.begin(), allocation_information->DSP_y_db.end(), module_prec) != allocation_information->DSP_y_db.end())
      {
         temp_pipe_parameters = parameters_pairs[1];
         break;
      }
      else if(boost::lexical_cast<unsigned int>(parameters_pairs[0]) == module_prec)
      {
         temp_pipe_parameters = parameters_pairs[1];
         break;
      }
   }
   THROW_ASSERT(temp_pipe_parameters != "", "expected some pipe_parameters for the the template operation");
   std::string fastest_pipe_parameter = "0";
   double fastest_stage_period = std::numeric_limits<double>::max();
   std::vector<std::string> pipe_parameters = SplitString(temp_pipe_parameters, ",");
   const std::vector<std::string>::const_iterator st_end = pipe_parameters.end();
   std::vector<std::string>::const_iterator st_next;
   unsigned int skip_pipe_parameter = 0;
   if(is_flopoco_provided)
      skip_pipe_parameter = std::max(1u, parameters->getOption<unsigned int>(OPT_skip_pipe_parameter));
   else if(is_a_skip_operation(curr_op))
      skip_pipe_parameter = parameters->getOption<unsigned int>(OPT_skip_pipe_parameter);
   for(auto st = st_next = pipe_parameters.begin(); st != st_end; ++st)
   {
      ++st_next;
      const technology_nodeRef fu_cur_obj = HLS_T->get_technology_manager()->get_fu(fu->fu_template_name + "_" + template_suffix + "_" + *st, library_name);
      if(fu_cur_obj)
      {
         area_modelRef a_m = GetPointer<functional_unit>(fu_cur_obj)->area_m;
         THROW_ASSERT(a_m, "Area information not specified for unit " + fu->fu_template_name + "_" + template_suffix + "_" + *st);
         bool has_DSPs = (GetPointer<clb_model>(a_m) && GetPointer<clb_model>(a_m)->get_resource_value(clb_model::DSP) > 0);
         double DSP_allocation_coefficient = allocation_information->DSP_allocation_coefficient;
         double dsp_multiplier = has_DSPs ? allocation_information->DSPs_margin * DSP_allocation_coefficient : 1.0;
         double dsp_multiplier_stage = has_DSPs ? allocation_information->DSPs_margin_stage * DSP_allocation_coefficient : 1.0;

         if(fu_cur_obj)
         {
            const functional_unit* fu_cur = GetPointer<functional_unit>(fu_cur_obj);
            auto* fu_cur_operation = GetPointer<operation>(fu_cur->get_operation(curr_op));
            if(fu_cur_operation->time_m->get_cycles() >= 1 && allocation_information->time_m_stage_period(fu_cur_operation) < fastest_stage_period)
            {
               fastest_pipe_parameter = *st;
               fastest_stage_period = allocation_information->time_m_stage_period(fu_cur_operation);
            }
            if((*st == "0" && dsp_multiplier * allocation_information->time_m_execution_time(fu_cur_operation) < clock && allocation_information->time_m_execution_time(fu_cur_operation) != 0.0) ||
               (fu_cur_operation->time_m->get_cycles() >= 1 && dsp_multiplier_stage * allocation_information->time_m_stage_period(fu_cur_operation) < clock && *st != "0"))
            {
               if(skip_pipe_parameter && st_next != st_end)
                  --skip_pipe_parameter;
               else
               {
                  precomputed_pipeline_unit[fu->fu_template_name + "_" + template_suffix] = *st;
                  if(*st == pipe_parameter)
                  {
                     return pipe_parameter;
                  }
                  else
                  {
                     return "";
                  }
               }
            }
         }
      }
   }
   if(fastest_pipe_parameter == "0") /// in case no pipelined version exist it returns the one not pipelined
   {
      precomputed_pipeline_unit[fu->fu_template_name + "_" + template_suffix] = fastest_pipe_parameter;
   }
   else
   {
      /// in case clock is not compatible with any of the pipelined version it returns the fastest pipelined version available
      THROW_WARNING("No functional unit exists for the given clock period: the fastest pipelined unit will be used (" + fu->fu_template_name + "): " + STR(fastest_stage_period));
      precomputed_pipeline_unit[fu->fu_template_name + "_" + template_suffix] = fastest_pipe_parameter;
   }
   if(pipe_parameter == fastest_pipe_parameter)
   {
      return fastest_pipe_parameter;
   }
   else
      return "";
}

void allocation::set_number_channels(unsigned int fu_name, unsigned int n_ports)
{
   allocation_information->nports_map[fu_name] = n_ports;
}

technology_nodeRef allocation::get_fu(const std::string& fu_name)
{
   std::string library_name = HLS_T->get_technology_manager()->get_library(fu_name);
   if(library_name == "")
      return technology_nodeRef();
   return HLS_T->get_technology_manager()->get_fu(fu_name, library_name);
}

bool allocation::is_ram_not_timing_compliant(const HLS_constraintsRef HLS_C, unsigned int var, technology_nodeRef current_fu)
{
   if(!parameters->IsParameter("variable-mem-lat") || parameters->GetParameter<int>("variable-mem-lat") == 0)
      return false;
   if(HLSMgr->Rmem->is_read_only_variable(var))
      return false;
   auto n_ref = static_cast<unsigned int>(HLSMgr->Rmem->get_maximum_references(var));
   double clock_period = HLS_C->get_clock_period_resource_fraction() * HLS_C->get_clock_period();
   double controller_delay = 0; // too overestimated allocation_information->EstimateControllerDelay();
   const functional_unit* fu_cur = GetPointer<functional_unit>(current_fu);
   auto* load_operation = GetPointer<operation>(fu_cur->get_operation("STORE"));
   double ex_time = allocation_information->time_m_execution_time(load_operation);
   unsigned int n_channels = n_ref > parameters->isOption(OPT_channels_number) ? parameters->getOption<unsigned int>(OPT_channels_number) : 1;
   double mux_delay = allocation_information->estimate_muxNto1_delay(32, n_ref / n_channels);
   double setup = allocation_information->get_setup_hold_time(); // for the PHIs
   return n_ref / n_channels > 1 && (controller_delay + ex_time + mux_delay + setup) > clock_period;
}

std::string allocation::get_synch_ram_latency(const std::string& ram_template, const std::string& latency_postfix, const HLS_constraintsRef HLS_C, unsigned int var)
{
   std::string new_lat;
   technology_nodeRef current_fu = get_fu(ram_template + latency_postfix);
   bool is_synchronous_ram_not_timing_compliant = is_ram_not_timing_compliant(HLS_C, var, current_fu);
   if(is_synchronous_ram_not_timing_compliant)
   {
      new_lat = latency_postfix == "" ? std::string("3") : std::string("4");
      current_fu = get_fu(ram_template + allocation_information->get_latency_string(new_lat));
      allocation_information->sync_ram_var_latency[var] = new_lat;
   }
   else
   {
      new_lat = latency_postfix == "" ? "2" : (latency_postfix == "_3" ? "3" : "4");
      allocation_information->sync_ram_var_latency[var] = new_lat;
   }
   return new_lat;
}

void allocation::IntegrateTechnologyLibraries()
{
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   const FunctionBehaviorConstRef function_behavior = HLSMgr->CGetFunctionBehavior(funId);
   const HLS_constraintsRef HLS_C = HLS->HLS_C;
   double clock_period = HLS_C->get_clock_period_resource_fraction() * HLS_C->get_clock_period();

   std::string latency_postfix = "";
   if(parameters->getOption<std::string>(OPT_bram_high_latency) != "")
      latency_postfix = parameters->getOption<std::string>(OPT_bram_high_latency);
   for(const auto& l : HLSMgr->Rmem->get_function_vars(funId))
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - analyzing variable " + STR(l.first));
      unsigned int var = l.first;
      technology_nodeRef current_fu;
      unsigned int n_ports = 1;

      if(HLSMgr->Rmem->has_callSite_base_address(var))
         continue;

      bool is_async_var = false;

      THROW_ASSERT(parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) != MemoryAllocation_ChannelsType::MEM_ACC_P1N, "unexpected condition");
      bool unaligned_access_p = parameters->isOption(OPT_unaligned_access) && parameters->getOption<bool>(OPT_unaligned_access);
      if(parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_11)
      {
         if(HLSMgr->Rmem->is_sds_var(var))
         {
            if((HLSMgr->Rmem->has_all_pointers_resolved() && HLSMgr->Rmem->does_need_addr(var)) || (!HLSMgr->Rmem->has_all_pointers_resolved() && !HLSMgr->Rmem->is_private_memory(var)))
               current_fu = get_fu(ARRAY_1D_STD_BRAM_SDS_BUS + latency_postfix);
            else
            {
               if(parameters->getOption<bool>(OPT_use_asynchronous_memories) && (AllocationInformation::can_be_asynchronous_ram(TreeM, var, parameters->getOption<unsigned int>(OPT_distram_threshold), HLSMgr->Rmem->is_read_only_variable(var))))
               {
                  current_fu = get_fu(ARRAY_1D_STD_DISTRAM_SDS);
                  bool is_asynchronous_ram_not_timing_compliant = is_ram_not_timing_compliant(HLS_C, var, current_fu);
                  if(is_asynchronous_ram_not_timing_compliant)
                     current_fu = get_fu(ARRAY_1D_STD_BRAM_SDS + allocation_information->get_latency_string(get_synch_ram_latency(ARRAY_1D_STD_BRAM_SDS, latency_postfix, HLS_C, var)));
                  else
                  {
                     is_async_var = true;
                  }
               }
               else
                  current_fu = get_fu(ARRAY_1D_STD_BRAM_SDS + allocation_information->get_latency_string(get_synch_ram_latency(ARRAY_1D_STD_BRAM_SDS, latency_postfix, HLS_C, var)));
            }
         }
         else if(!unaligned_access_p && HLSMgr->Rmem->get_bram_bitsize() == 8 && HLSMgr->Rmem->get_bus_data_bitsize() == 8 && !HLSMgr->Rmem->is_private_memory(var))
            current_fu = get_fu(ARRAY_1D_STD_BRAM_SDS_BUS + latency_postfix);
         else
            current_fu = get_fu(ARRAY_1D_STD_BRAM + latency_postfix);
      }
      else if(parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_N1)
      {
         if(HLSMgr->Rmem->is_sds_var(var))
         {
            if((HLSMgr->Rmem->has_all_pointers_resolved() && HLSMgr->Rmem->does_need_addr(var)) || (!HLSMgr->Rmem->has_all_pointers_resolved() && !HLSMgr->Rmem->is_private_memory(var)))
               current_fu = get_fu(ARRAY_1D_STD_BRAM_N1_SDS_BUS + latency_postfix);
            else
            {
               if(parameters->getOption<bool>(OPT_use_asynchronous_memories) && AllocationInformation::can_be_asynchronous_ram(TreeM, var, parameters->getOption<unsigned int>(OPT_distram_threshold), HLSMgr->Rmem->is_read_only_variable(var)))
               {
                  current_fu = get_fu(ARRAY_1D_STD_DISTRAM_N1_SDS);
                  bool is_asynchronous_ram_not_timing_compliant = is_ram_not_timing_compliant(HLS_C, var, current_fu);
                  if(is_asynchronous_ram_not_timing_compliant)
                     current_fu = get_fu(ARRAY_1D_STD_BRAM_N1_SDS + allocation_information->get_latency_string(get_synch_ram_latency(ARRAY_1D_STD_BRAM_N1_SDS, latency_postfix, HLS_C, var)));
                  else
                  {
                     is_async_var = true;
                  }
               }
               else
                  current_fu = get_fu(ARRAY_1D_STD_BRAM_N1_SDS + allocation_information->get_latency_string(get_synch_ram_latency(ARRAY_1D_STD_BRAM_N1_SDS, latency_postfix, HLS_C, var)));
            }
         }
         else if(!unaligned_access_p && HLSMgr->Rmem->get_bram_bitsize() == 8 && HLSMgr->Rmem->get_bus_data_bitsize() == 8 && !HLSMgr->Rmem->is_private_memory(var))
            current_fu = get_fu(ARRAY_1D_STD_BRAM_N1_SDS_BUS + latency_postfix);
         else
            current_fu = get_fu(ARRAY_1D_STD_BRAM_N1 + latency_postfix);
         n_ports = parameters->getOption<unsigned int>(OPT_channels_number);
      }
      else if(parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_NN)
      {
         if(HLSMgr->Rmem->is_sds_var(var))
         {
            if((HLSMgr->Rmem->has_all_pointers_resolved() && HLSMgr->Rmem->does_need_addr(var)) || (!HLSMgr->Rmem->has_all_pointers_resolved() && !HLSMgr->Rmem->is_private_memory(var)))
               current_fu = get_fu(ARRAY_1D_STD_BRAM_NN_SDS_BUS + latency_postfix);
            else
            {
               if(parameters->getOption<bool>(OPT_use_asynchronous_memories) && AllocationInformation::can_be_asynchronous_ram(TreeM, var, parameters->getOption<unsigned int>(OPT_distram_threshold), HLSMgr->Rmem->is_read_only_variable(var)))
               {
                  current_fu = get_fu(ARRAY_1D_STD_DISTRAM_NN_SDS);
                  bool is_asynchronous_ram_not_timing_compliant = is_ram_not_timing_compliant(HLS_C, var, current_fu);
                  if(is_asynchronous_ram_not_timing_compliant)
                     current_fu = get_fu(ARRAY_1D_STD_BRAM_NN_SDS + allocation_information->get_latency_string(get_synch_ram_latency(ARRAY_1D_STD_BRAM_NN_SDS, latency_postfix, HLS_C, var)));
                  else
                  {
                     is_async_var = true;
                  }
               }
               else
                  current_fu = get_fu(ARRAY_1D_STD_BRAM_NN_SDS + allocation_information->get_latency_string(get_synch_ram_latency(ARRAY_1D_STD_BRAM_NN_SDS, latency_postfix, HLS_C, var)));
            }
         }
         else if(!unaligned_access_p && HLSMgr->Rmem->get_bram_bitsize() == 8 && HLSMgr->Rmem->get_bus_data_bitsize() == 8 && !HLSMgr->Rmem->is_private_memory(var))
            current_fu = get_fu(ARRAY_1D_STD_BRAM_NN_SDS_BUS + latency_postfix);
         else
            current_fu = get_fu(ARRAY_1D_STD_BRAM_NN + latency_postfix);
         n_ports = parameters->getOption<unsigned int>(OPT_channels_number);
      }
      else
         THROW_ERROR("type of channel based organization not yet supported");

      unsigned int current_size = allocation_information->get_number_fu_types();
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - allocating unit " + current_fu->get_name() + " for variable " + function_behavior->CGetBehavioralHelper()->PrintVariable(l.first) + " in position " + STR(current_size));
      allocation_information->list_of_FU.push_back(current_fu);
      if(HLSMgr->Rmem->is_sds_var(var) && HLSMgr->Rmem->is_read_only_variable(var) && (is_async_var || (parameters->isOption(OPT_rom_duplication) && parameters->getOption<bool>(OPT_rom_duplication))))
      {
         allocation_information->tech_constraints.push_back(INFINITE_UINT);
         set_number_channels(current_size, n_ports);
      }
      else
      {
         allocation_information->tech_constraints.push_back(n_ports);
         set_number_channels(current_size, n_ports);
      }
      allocation_information->id_to_fu_names[current_size] = std::make_pair(current_fu->get_name(), TM->get_library(current_fu->get_name()));
      THROW_ASSERT(allocation_information->tech_constraints.size() == allocation_information->list_of_FU.size(), "Something of wrong happened");
      allocation_information->vars_to_memory_units[var] = current_size;
      allocation_information->memory_units[current_size] = var;
      allocation_information->memory_units_sizes[current_size] = tree_helper::size(TreeM, var) / 8;
      allocation_information->precision_map[current_size] = 0;
      /// check clock constraints compatibility
      auto* fu_br = GetPointer<functional_unit>(current_fu);
      technology_nodeRef op_store_node = fu_br->get_operation("STORE");
      auto* op_store = GetPointer<operation>(op_store_node);
      double store_delay = allocation_information->time_m_execution_time(op_store) - allocation_information->get_correction_time(current_size, "STORE", 0) + allocation_information->get_setup_hold_time();
      if(store_delay > clock_period)
         THROW_ERROR("clock constraint too tight: BRAMs for this device cannot run so fast... (" + current_fu->get_name() + ":" + STR(store_delay) + ">" + STR(clock_period) + ")");
   }

   /// allocate proxies
   if(HLSMgr->Rmem->has_proxied_internal_variables(funId))
   {
      for(const auto& proxied_var_id : HLSMgr->Rmem->get_proxied_internal_variables(funId))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - analyzing proxied variable " + STR(proxied_var_id));
         technology_nodeRef current_fu;
         unsigned int n_ports = 1;
         if(parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_11)
         {
            if(HLSMgr->Rmem->is_sds_var(proxied_var_id))
            {
               if((HLSMgr->Rmem->has_all_pointers_resolved() && HLSMgr->Rmem->does_need_addr(proxied_var_id)) || (!HLSMgr->Rmem->has_all_pointers_resolved() && !HLSMgr->Rmem->is_private_memory(proxied_var_id)))
                  current_fu = get_fu(PROXY_CTRL + latency_postfix);
               else
               {
                  if(parameters->getOption<bool>(OPT_use_asynchronous_memories) &&
                     AllocationInformation::can_be_asynchronous_ram(TreeM, proxied_var_id, parameters->getOption<unsigned int>(OPT_distram_threshold), HLSMgr->Rmem->is_read_only_variable(proxied_var_id)))
                  {
                     technology_nodeRef a_fu = get_fu(ARRAY_1D_STD_DISTRAM_SDS);
                     bool is_asynchronous_ram_not_timing_compliant = is_ram_not_timing_compliant(HLS_C, proxied_var_id, a_fu);
                     if(is_asynchronous_ram_not_timing_compliant)
                        current_fu = get_fu(PROXY_CTRL + allocation_information->get_latency_string(get_synch_ram_latency(ARRAY_1D_STD_BRAM_SDS, latency_postfix, HLS_C, proxied_var_id)));
                     else
                        current_fu = get_fu(DPROXY_CTRL);
                  }
                  else
                     current_fu = get_fu(PROXY_CTRL + allocation_information->get_latency_string(get_synch_ram_latency(ARRAY_1D_STD_BRAM_SDS, latency_postfix, HLS_C, proxied_var_id)));
               }
            }
            else
               current_fu = get_fu(PROXY_CTRL + latency_postfix);
         }
         else if(parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_N1 || parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_NN)
         {
            if(HLSMgr->Rmem->is_sds_var(proxied_var_id))
            {
               if((HLSMgr->Rmem->has_all_pointers_resolved() && HLSMgr->Rmem->does_need_addr(proxied_var_id)) || (!HLSMgr->Rmem->has_all_pointers_resolved() && !HLSMgr->Rmem->is_private_memory(proxied_var_id)))
                  current_fu = get_fu(PROXY_CTRLN + latency_postfix);
               else
               {
                  bool is_nn = parameters->getOption<MemoryAllocation_ChannelsType>(OPT_channels_type) == MemoryAllocation_ChannelsType::MEM_ACC_NN;
                  if(parameters->getOption<bool>(OPT_use_asynchronous_memories) &&
                     AllocationInformation::can_be_asynchronous_ram(TreeM, proxied_var_id, parameters->getOption<unsigned int>(OPT_distram_threshold), HLSMgr->Rmem->is_read_only_variable(proxied_var_id)))
                  {
                     technology_nodeRef a_fu = get_fu(is_nn ? ARRAY_1D_STD_DISTRAM_NN_SDS : ARRAY_1D_STD_DISTRAM_N1_SDS);
                     bool is_asynchronous_ram_not_timing_compliant = is_ram_not_timing_compliant(HLS_C, proxied_var_id, a_fu);
                     if(is_asynchronous_ram_not_timing_compliant)
                        current_fu = get_fu(PROXY_CTRLN + allocation_information->get_latency_string(get_synch_ram_latency(is_nn ? ARRAY_1D_STD_BRAM_NN_SDS : ARRAY_1D_STD_BRAM_N1_SDS, latency_postfix, HLS_C, proxied_var_id)));
                     else
                        current_fu = get_fu(DPROXY_CTRLN);
                  }
                  else
                     current_fu = get_fu(PROXY_CTRLN + allocation_information->get_latency_string(get_synch_ram_latency(is_nn ? ARRAY_1D_STD_BRAM_NN_SDS : ARRAY_1D_STD_BRAM_N1_SDS, latency_postfix, HLS_C, proxied_var_id)));
               }
            }
            else
               current_fu = get_fu(PROXY_CTRLN + latency_postfix);
            n_ports = parameters->getOption<unsigned int>(OPT_channels_number);
         }
         else
            THROW_ERROR("type of channel based organization not yet supported");
         unsigned int current_size = allocation_information->get_number_fu_types();
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - allocating unit " + current_fu->get_name() + " for variable " + function_behavior->CGetBehavioralHelper()->PrintVariable(proxied_var_id) + " in position " + STR(current_size));
         allocation_information->list_of_FU.push_back(current_fu);
         allocation_information->tech_constraints.push_back(n_ports);
         set_number_channels(current_size, n_ports);
         allocation_information->id_to_fu_names[current_size] = std::make_pair(current_fu->get_name(), TM->get_library(current_fu->get_name()));
         THROW_ASSERT(allocation_information->tech_constraints.size() == allocation_information->list_of_FU.size(), "Something of wrong happened");
         allocation_information->vars_to_memory_units[proxied_var_id] = current_size;
         allocation_information->proxy_memory_units[current_size] = proxied_var_id;
         allocation_information->precision_map[current_size] = 0;
      }
   }

   /// add shared functions to the technology manager
   if(HLSMgr->Rfuns->has_shared_functions(funId))
   {
      for(const auto& shared_fu_name : HLSMgr->Rfuns->get_shared_functions(funId))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - adding proxy function wrapper " + shared_fu_name);
         const std::string library_name = TM->get_library(shared_fu_name);
         if(library_name != "")
         {
            const library_managerRef libraryManager = TM->get_library_manager(library_name);
            technology_nodeRef techNode_obj = libraryManager->get_fu(shared_fu_name);
            THROW_ASSERT(techNode_obj, "function not yet built: " + shared_fu_name);
            const std::string wrapped_fu_name = WRAPPED_PROXY_PREFIX + shared_fu_name;
            technology_nodeRef wrapper_tn = TM->get_fu(wrapped_fu_name, PROXY_LIBRARY);
            if(!wrapper_tn)
            {
               structural_managerRef structManager_obj = GetPointer<functional_unit>(techNode_obj)->CM;
               if(structManager_obj && (GetPointer<module>(structManager_obj->get_circ())->get_NP_functionality()->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR) or
                                        GetPointer<module>(structManager_obj->get_circ())->get_NP_functionality()->exist_NP_functionality(NP_functionality::VHDL_GENERATOR)))
               {
                  moduleGeneratorRef modGen = moduleGeneratorRef(new moduleGenerator(HLSMgr, parameters));
                  std::string new_shared_fu_name = shared_fu_name + "_modgen";
                  modGen->create_generic_module(shared_fu_name, libraryManager->get_library_name(), TM, new_shared_fu_name, HLS_T->get_target_device()->get_type(), HLSMgr);
                  techNode_obj = libraryManager->get_fu(new_shared_fu_name);
                  THROW_ASSERT(techNode_obj, "function not yet built: " + new_shared_fu_name);
               }
               add_proxy_function_wrapper(library_name, techNode_obj, shared_fu_name);
               wrapper_tn = TM->get_fu(wrapped_fu_name, PROXY_LIBRARY);
            }
            THROW_ASSERT(wrapper_tn, "Module not added");
            std::string key_new = ENCODE_FU_LIB(shared_fu_name, PROXY_LIBRARY);
            HLS_C->tech_constraints[key_new] = 1; // HLS_C->tech_constraints.find(key_old)->second;
            /// allocate it
            unsigned int current_size = allocation_information->get_number_fu_types();
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - allocating unit " + wrapper_tn->get_name() + " in position " + STR(current_size));
            allocation_information->list_of_FU.push_back(wrapper_tn);
            allocation_information->tech_constraints.push_back(1);
            allocation_information->id_to_fu_names[current_size] = std::make_pair(wrapper_tn->get_name(), PROXY_LIBRARY);
            THROW_ASSERT(allocation_information->tech_constraints.size() == allocation_information->list_of_FU.size(), "Something of wrong happened");
            allocation_information->precision_map[current_size] = 0;
            fu_list[wrapper_tn][0][HLS_manager::io_binding_type(0, 0)] = current_size;
            allocation_information->proxy_wrapped_units[current_size] = shared_fu_name;
         }
      }
   }

   /// add shared function proxies to the technology manager
   if(HLSMgr->Rfuns->has_proxied_shared_functions(funId))
   {
      for(const auto& original_proxied_fu_name : HLSMgr->Rfuns->get_proxied_shared_functions(funId))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - adding proxy function module " + original_proxied_fu_name);
         const std::string library_name = TM->get_library(original_proxied_fu_name);
         if(library_name != "")
         {
            const library_managerRef libraryManager = TM->get_library_manager(library_name);
            technology_nodeRef techNode_obj = libraryManager->get_fu(original_proxied_fu_name);
            THROW_ASSERT(techNode_obj, "function not yet built: " + original_proxied_fu_name);
            technology_nodeRef proxy_tn = TM->get_fu(PROXY_PREFIX + original_proxied_fu_name, PROXY_LIBRARY);
            if(!proxy_tn)
            {
               add_proxy_function_module(HLS_C, techNode_obj, original_proxied_fu_name);
            }
         }
      }
   }
}

bool allocation::HasToBeExecuted() const
{
   if(HLSFunctionStep::HasToBeExecuted())
      return true;
   else
   {
      std::map<unsigned int, unsigned int> cur_bb_ver;
      const CallGraphManagerConstRef call_graph_manager = HLSMgr->CGetCallGraphManager();
      CustomOrderedSet<unsigned int> funcs = call_graph_manager->GetReachedBodyFunctions();
      if(funId and funcs.find(funId) == funcs.end())
         return false;
      const auto called_functions = call_graph_manager->GetReachedBodyFunctionsFrom(funId);
      for(auto const called_function : called_functions)
      {
         if(called_function == funId)
            continue;
         const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(called_function);
         cur_bb_ver[called_function] = FB->GetBBVersion();
      }
      return not std::equal(cur_bb_ver.begin(), cur_bb_ver.end(), last_bb_ver.begin());
   }
}

void allocation::PrintInitialIR() const
{
   const std::string file_name = parameters->getOption<std::string>(OPT_output_temporary_directory) + "before_" + GetName() + ".tm";
   std::ofstream raw_file(file_name.c_str());
   TM->print(raw_file);
   raw_file.close();
}
