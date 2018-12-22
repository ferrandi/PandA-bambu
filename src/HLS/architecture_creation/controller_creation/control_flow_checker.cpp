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
 *              Copyright (c) 2017-2018 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */

// include class header
#include "control_flow_checker.hpp"

// include from behavior/
#include "function_behavior.hpp"

// include from circuit/
#include "structural_manager.hpp"

// include from HLS/
#include "hls.hpp"

#include "hls_manager.hpp"
#include "hls_target.hpp"

// include from HLS/
#include "Discrepancy.hpp"
#include "language_writer.hpp"
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"

// include from  tree/
#include "behavioral_helper.hpp"

// includes from ./
#include "Parameter.hpp"
#include "copyrights_strings.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

ControlFlowChecker::ControlFlowChecker(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : HLSFunctionStep(_Param, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::CONTROL_FLOW_CHECKER)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

ControlFlowChecker::~ControlFlowChecker()
{
}

const std::unordered_set<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ControlFlowChecker::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_stg_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      case PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
         break;
      }
   }
   return ret;
}

static bool is_one_hot(const HLS_managerRef HLSMgr)
{
   std::string vendor;
   const auto tgt_device = HLSMgr->get_HLS_target()->get_target_device();
   if(tgt_device->has_parameter("vendor"))
   {
      vendor = tgt_device->get_parameter<std::string>("vendor");
      boost::algorithm::to_lower(vendor);
   }
   bool one_hot_encoding = false;
   if(HLSMgr->get_parameter()->getOption<std::string>(OPT_fsm_encoding) == "one-hot")
      one_hot_encoding = true;
   else if(HLSMgr->get_parameter()->getOption<std::string>(OPT_fsm_encoding) == "auto" && vendor == "xilinx")
      one_hot_encoding = true;

   return one_hot_encoding;
}

static unsigned int comp_state_bitsize(bool one_hot_encoding, const HLS_managerRef HLSMgr, const unsigned int f_id, unsigned max_value)
{
   unsigned int n_states = HLSMgr->get_HLS(f_id)->STG->get_number_of_states();
   unsigned int bitsnumber = language_writer::bitnumber(n_states - 1);
   if(max_value != n_states - 1)
      bitsnumber = language_writer::bitnumber(max_value);

   unsigned int state_bitsize = one_hot_encoding ? (max_value + 1) : bitsnumber;

   return state_bitsize;
}

static std::string create_control_flow_checker(size_t epp_trace_bitsize, const unsigned int f_id, const StateTransitionGraphManagerRef& STG, const HLS_managerRef HLSMgr, bool one_hot_encoding, unsigned max_value, unsigned int state_bitsize,
                                               int debug_level)
{
   /// adjust in case states are not consecutive
   const auto& eppstg = STG->CGetEPPStg();
   const auto& stg = STG->CGetStg();
   const auto& stg_info = stg->CGetStateTransitionGraphInfo();
   std::string result = "// internal signals\n"
                        "reg checker_state;\n"
                        "reg next_checker_state;\n"
                        "wire is_checking;\n"
                        "reg [EPP_TRACE_BITSIZE - 1 : 0] epp_counter;\n"
                        "reg [EPP_TRACE_BITSIZE - 1 : 0] prev_epp_counter;\n"
                        "wire [EPP_TRACE_BITSIZE - 1 : 0] next_epp_counter;\n"
                        "wire [EPP_TRACE_BITSIZE - 1 : 0] epp_incremented_counter;\n"
                        "reg [EPP_TRACE_BITSIZE - 1 : 0] epp_increment_val;\n"
                        "reg [EPP_TRACE_BITSIZE - 1 : 0] epp_reset_val;\n"
                        "reg epp_to_reset;\n"
                        "reg to_check_now;\n"
                        "reg to_check_prev;\n"
                        "reg next_to_check_prev;\n"
                        "reg [BITSIZE_out_mismatch_trace_offset - 1 : 0] epp_trace_offset;\n"
                        "reg [BITSIZE_out_mismatch_trace_offset - 1 : 0] prev_epp_trace_offset;\n"
                        "wire [BITSIZE_out_mismatch_trace_offset - 1 : 0] next_prev_epp_trace_offset;\n"
                        "wire [BITSIZE_out_mismatch_trace_offset - 1 : 0] next_epp_trace_offset;\n"
                        "wire [1 : 0] trace_offset_increment;\n"
                        "wire prev_trace_offset_increment;\n"
                        "wire [EPP_TRACE_BITSIZE - 1 : 0] epp_trace_memory [0 : EPP_TRACE_LENGTH-1];\n"
                        "wire [BITSIZE_out_mismatch_trace_offset - 1 : 0] mismatch_trace_offset;\n"
                        "wire mismatch_now;\n"
                        "wire mismatch_prev;\n"
                        "\n";

   result += "initial\n"
             "begin\n"
             "  $readmemb(MEMORY_INIT_file, epp_trace_memory, 0, EPP_TRACE_LENGTH-1);\n"
             "end\n\n";

   result += "assign epp_incremented_counter = epp_counter + epp_increment_val;\n"
             "assign next_epp_counter = epp_to_reset ? epp_reset_val : epp_incremented_counter;\n\n";

   result += "assign trace_offset_increment = to_check_now + next_to_check_prev;\n"
             "assign next_epp_trace_offset = epp_trace_offset + trace_offset_increment;\n\n"
             "assign prev_trace_offset_increment = to_check_now && next_to_check_prev;\n"
             "assign next_prev_epp_trace_offset = epp_trace_offset + prev_trace_offset_increment;\n\n";

   result += "assign is_checking = checker_state || start_port;\n"
             "assign mismatch_now = to_check_now && (epp_counter != epp_trace_memory[epp_trace_offset]);\n"
             "assign mismatch_prev = to_check_prev && (prev_epp_counter != epp_trace_memory[prev_epp_trace_offset]);\n"
             "assign out_mismatch = is_checking && (mismatch_now || mismatch_prev);\n"
             "assign out_mismatch_id = out_mismatch ? EPP_MISMATCH_ID : 0;\n"
             "assign mismatch_trace_offset = mismatch_prev ? prev_epp_trace_offset : epp_trace_offset;\n"
             "assign out_mismatch_trace_offset = out_mismatch ? mismatch_trace_offset : 0;\n\n";

   result += "// update state of the checker\n"
             "always @(*)\n"
             "begin\n"
             "   case (checker_state)\n"
             "   0:\n"
             "   begin\n"
             "      if (start_port && ! done_port)\n"
             "         next_checker_state = 1;\n"
             "   end\n"
             "   1:\n"
             "   begin\n"
             "      if (done_port)\n"
             "         next_checker_state = 0;\n"
             "   end\n"
             "   default:\n"
             "      next_checker_state = 0;\n"
             "   endcase\n"
             "end\n\n";

   result += "// compute if this state is to check\n"
             "always @(*)\n"
             "begin\n"
             "   case (" PRESENT_STATE_PORT_NAME ")\n"
             "   default:\n"
             "   to_check_now = 0;\n";

   const auto encode_one_hot = [](unsigned int nstates, unsigned int val) -> std::string {
      std::string res;
      for(unsigned int i = 0; i < nstates; ++i)
         res = (val == i ? "1" : "0") + res;
      return res;
   };
   bool is_first = true;
   for(const auto s_id : HLSMgr->RDiscr->fu_id_to_states_to_check.at(f_id))
   {
      std::string state_string;
      if(one_hot_encoding)
         state_string = STR(state_bitsize) + "'b" + encode_one_hot(max_value + 1, s_id);
      else
         state_string = STR(state_bitsize) + "'d" + STR(s_id);
      if(not is_first)
         result += ',';
      result += "\n   " + state_string;
      is_first = false;
   }

   result += ":\n"
             "   to_check_now = 1;\n";

   result += "   endcase\n"
             "end\n\n";

   result += "// compute if at the next cycle we have to check the previous state\n"
             "always @(*)\n"
             "begin\n"
             "   case (" PRESENT_STATE_PORT_NAME ")\n"
             "   default:\n"
             "   next_to_check_prev = 0;\n";

   const auto fsm_entry_node = stg_info->entry_node;
   const auto fsm_exit_node = stg_info->exit_node;
   BOOST_FOREACH(vertex state, boost::vertices(*stg))
   {
      if(state == fsm_entry_node or state == fsm_exit_node)
         continue;
      bool a = true;
      BOOST_FOREACH(EdgeDescriptor out_edge, boost::out_edges(state, *stg))
      {
         if(stg->GetSelector(out_edge) & TransitionInfo::StateTransitionType::ST_EDGE_FEEDBACK)
         {
            const vertex dst = boost::target(out_edge, *stg);
            const auto dst_id = stg_info->vertex_to_state_id.at(dst);
            if(HLSMgr->RDiscr->fu_id_to_feedback_states_to_check.at(f_id).find(dst_id) != HLSMgr->RDiscr->fu_id_to_feedback_states_to_check.at(f_id).end())
            {
               if(a)
               {
                  const auto s_id = stg_info->vertex_to_state_id.at(state);
                  std::string state_string;
                  if(one_hot_encoding)
                     state_string = STR(state_bitsize) + "'b" + encode_one_hot(max_value + 1, s_id);
                  else
                     state_string = STR(state_bitsize) + "'d" + STR(s_id);

                  result += "   " + state_string +
                            ":\n"
                            "   begin\n"
                            "   case (" NEXT_STATE_PORT_NAME ")\n";

                  a = false;
               }
               std::string state_string;
               if(one_hot_encoding)
                  state_string = STR(state_bitsize) + "'b" + encode_one_hot(max_value + 1, dst_id);
               else
                  state_string = STR(state_bitsize) + "'d" + STR(dst_id);
               result += "   " + state_string + ":\n";
            }
         }
      }
      if(not a)
      {
         result += "         next_to_check_prev = 1;\n"
                   "   endcase\n"
                   "   end\n";
      }
   }

   result += "   endcase\n"
             "end\n\n";

   result += "// compute EPP increments and resets\n"
             "always @(*)\n"
             "begin\n\n"
             "   epp_to_reset = 0;\n"
             "   epp_increment_val = 0;\n"
             "   epp_reset_val = 0;\n\n";

   std::string initial_state_string, initial_epp_counter;

   OutEdgeIterator o_e_it, o_e_end;
   boost::tie(o_e_it, o_e_end) = boost::out_edges(fsm_entry_node, *stg);
   vertex initial_state = boost::target(*o_e_it, *stg);
   const auto initial_state_id = stg_info->vertex_to_state_id.at(initial_state);
   if(one_hot_encoding)
      initial_state_string = STR(state_bitsize) + "'b" + encode_one_hot(max_value + 1, initial_state_id);
   else
      initial_state_string = STR(state_bitsize) + "'d" + STR(initial_state_id);

   initial_epp_counter = STR(stg->CGetTransitionInfo(*o_e_it)->get_epp_increment());

   result += "   if (" NEXT_STATE_PORT_NAME " == " + initial_state_string +
             ")\n"
             "   begin\n"
             "      epp_reset_val = " +
             initial_epp_counter +
             ";\n"
             "      epp_to_reset = 1'b1;\n"
             "   end\n\n";

   const auto& astg = STG->CGetAstg();
   std::map<unsigned int, std::map<unsigned int, size_t>> present_to_next_to_increment;
   std::map<unsigned int, std::map<unsigned int, size_t>> present_to_next_to_reset;
   BOOST_FOREACH(EdgeDescriptor e, boost::edges(*astg))
   {
      const vertex src = boost::source(e, *stg);
      const vertex dst = boost::target(e, *stg);
      if(src == fsm_entry_node or dst == fsm_exit_node)
         continue;
      const auto src_id = stg_info->vertex_to_state_id.at(src);
      const auto dst_id = stg_info->vertex_to_state_id.at(dst);
      const auto increment_val = eppstg->CGetTransitionInfo(e)->get_epp_increment();
      present_to_next_to_increment[src_id][dst_id] = increment_val;
   }
   for(const auto& e : HLSMgr->RDiscr->fu_id_to_reset_edges.at(f_id))
   {
      vertex src = boost::source(e, *stg);
      const auto src_id = stg_info->vertex_to_state_id.at(src);
      if(stg->CGetStateInfo(src)->is_dummy)
      {
         InEdgeIterator in_e_it, in_e_end;
         boost::tie(in_e_it, in_e_end) = boost::in_edges(src, *astg);
         src = boost::source(*in_e_it, *astg);
      }
      const auto epp_edge_to_exit = eppstg->CGetEdge(src, fsm_exit_node);
      const auto increment_val = eppstg->CGetTransitionInfo(epp_edge_to_exit)->get_epp_increment();

      const vertex dst = boost::target(e, *stg);
      const auto dst_id = stg_info->vertex_to_state_id.at(dst);
      const auto epp_edge_from_entry = eppstg->CGetEdge(fsm_entry_node, dst);
      const auto reset_val = eppstg->CGetTransitionInfo(epp_edge_from_entry)->get_epp_increment();
      present_to_next_to_increment[src_id][dst_id] = increment_val;
      present_to_next_to_reset[src_id][dst_id] = reset_val;
   }

   if(present_to_next_to_increment.size())
   {
      result += "   case (" PRESENT_STATE_PORT_NAME ")\n";
      for(const auto& p2n2i : present_to_next_to_increment)
      {
         bool skip = true;
         for(const auto& n2i : p2n2i.second)
         {
            const auto increment = n2i.second;
            if(increment != 0)
            {
               skip = false;
               break;
            }
         }
         if(!skip)
         {
            const auto pres_state_id = p2n2i.first;
            std::string p_s_string;
            if(one_hot_encoding)
               p_s_string = STR(state_bitsize) + "'b" + encode_one_hot(max_value + 1, pres_state_id);
            else
               p_s_string = STR(state_bitsize) + "'d" + STR(pres_state_id);
            result += "   " + p_s_string +
                      ":\n"
                      "   begin\n"
                      "   case (" NEXT_STATE_PORT_NAME ")\n";
            for(const auto& n2i : p2n2i.second)
            {
               const auto next_state_id = n2i.first;
               std::string n_s_string;
               if(one_hot_encoding)
                  n_s_string = STR(state_bitsize) + "'b" + encode_one_hot(max_value + 1, next_state_id);
               else
                  n_s_string = STR(state_bitsize) + "'d" + STR(next_state_id);
               result += "   " + n_s_string +
                         ":\n"
                         "      begin\n";
               const auto increment = n2i.second;
               bool to_reset = present_to_next_to_reset.find(pres_state_id) != present_to_next_to_reset.end() and present_to_next_to_reset.at(pres_state_id).find(next_state_id) != present_to_next_to_reset.at(pres_state_id).end();
               result += "         epp_increment_val = " + STR(epp_trace_bitsize) + "'d" + STR(increment) + ";\n";
               if(to_reset)
               {
                  result += "         epp_to_reset = 1'b1;\n"
                            "         epp_reset_val = " +
                            STR(epp_trace_bitsize) + "'d" + STR(present_to_next_to_reset.at(pres_state_id).at(next_state_id)) + ";\n";
               }
               result += "      end\n";
            }
            result += "   default:\n"
                      "      begin\n"
                      "         epp_increment_val = " +
                      STR(epp_trace_bitsize) +
                      "'d0;\n"
                      "      end\n";
            result += "   endcase\n"
                      "   end\n";
         }
      }
      result += "   default:\n"
                "      begin\n"
                "         epp_increment_val = " +
                STR(epp_trace_bitsize) +
                "'d0;\n"
                "      end\n";
      result += "   endcase\n";
   }

   result += "end\n\n";

   std::string reset_type = HLSMgr->get_parameter()->getOption<std::string>(OPT_sync_reset);
   if(reset_type == "no" || reset_type == "sync")
      result += "always @(posedge " CLOCK_PORT_NAME ")\n";
   else if(!HLSMgr->get_parameter()->getOption<bool>(OPT_level_reset))
      result += "always @(posedge " CLOCK_PORT_NAME " or negedge " RESET_PORT_NAME ")\n";
   else
      result += "always @(posedge " CLOCK_PORT_NAME " or posedge " RESET_PORT_NAME ")\n";

   if(!HLSMgr->get_parameter()->getOption<bool>(OPT_level_reset))
      result += "if (" RESET_PORT_NAME " == 1'b0)\n";
   else
      result += "if (" RESET_PORT_NAME " == 1'b1)\n";
   result += "  begin\n"
             "   prev_epp_counter <= 0;\n"
             "   epp_counter <= 0;\n"
             "   epp_trace_offset <= 0;\n"
             "   prev_epp_trace_offset <= 0;\n"
             "   to_check_prev <= 0;\n"
             "   checker_state <= 0;\n"
             "end\n"
             "else\n"
             "begin\n"
             "   prev_epp_counter <= epp_counter;\n"
             "   epp_counter <= next_epp_counter;\n"
             "   epp_trace_offset <= next_epp_trace_offset;\n"
             "   prev_epp_trace_offset <= next_prev_epp_trace_offset;\n"
             "   to_check_prev <= next_to_check_prev;\n"
             "   checker_state <= next_checker_state;\n"
             "end\n\n";

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Added control flow checker..." + result);
   return result;
}
void ControlFlowChecker::add_clock_reset(structural_objectRef circuit)
{
   structural_managerRef& SM = HLS->control_flow_checker;
   /// define boolean type for the clock and reset signals
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding clock signal...");
   /// add clock port
   structural_objectRef clock_obj = SM->add_port(CLOCK_PORT_NAME, port_o::IN, circuit, port_type);
   GetPointer<port_o>(clock_obj)->set_is_clock(true);
   /// add entry in in_port_map between port id and port index
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Clock signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding reset signal...");
   /// add reset port
   SM->add_port(RESET_PORT_NAME, port_o::IN, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Reset signal added!");
}

void ControlFlowChecker::add_done_port(structural_objectRef circuit)
{
   structural_managerRef& SM = HLS->control_flow_checker;
   /// define boolean type for the done port
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding Done signal...");
   /// add done port
   SM->add_port(DONE_PORT_NAME, port_o::IN, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Done signal added!");
}

void ControlFlowChecker::add_start_port(structural_objectRef circuit)
{
   structural_managerRef& SM = HLS->control_flow_checker;
   /// define boolean type for the start port
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Start adding start signal...");
   /// add the start port
   SM->add_port(START_PORT_NAME, port_o::IN, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - Start signal added!");
}

void ControlFlowChecker::add_present_state(structural_objectRef circuit, unsigned int state_bitsize)
{
   structural_managerRef& SM = HLS->control_flow_checker;
   /// define boolean type for the start port
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", state_bitsize));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Adding present/next state signals...");
   /// add the present/next state
   SM->add_port(PRESENT_STATE_PORT_NAME, port_o::IN, circuit, port_type);
   SM->add_port(NEXT_STATE_PORT_NAME, port_o::IN, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - present/next state signals added!");
}

void ControlFlowChecker::add_notifiers(structural_objectRef circuit)
{
   structural_managerRef& SM = HLS->control_flow_checker;
   /// define boolean type for the start port
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  * Adding notifier signals...");
   /// add the notifiers
   structural_objectRef pm = SM->add_port(NOTIFIER_PORT_MISMATCH, port_o::OUT, circuit, structural_type_descriptorRef(new structural_type_descriptor("bool", 0)));
   GetPointer<port_o>(pm)->set_is_memory(true);
   structural_objectRef pmi = SM->add_port(NOTIFIER_PORT_MISMATCH_ID, port_o::OUT, circuit, structural_type_descriptorRef(new structural_type_descriptor("bool", 32)));
   GetPointer<port_o>(pmi)->set_is_memory(true);
   structural_objectRef pmo = SM->add_port(NOTIFIER_PORT_MISMATCH_OFFSET, port_o::OUT, circuit, structural_type_descriptorRef(new structural_type_descriptor("bool", 32)));
   GetPointer<port_o>(pmo)->set_is_memory(true);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  - notifier signals added!");
}

void ControlFlowChecker::add_common_ports(structural_objectRef circuit, unsigned int state_bitsize)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding clock and reset ports...");
   add_clock_reset(circuit);

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the start port...");
   add_start_port(circuit);

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the done port...");
   add_done_port(circuit);

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding the present_state...");
   add_present_state(circuit, state_bitsize);

   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Adding notifiers...");
   add_notifiers(circuit);
}

DesignFlowStep_Status ControlFlowChecker::InternalExec()
{
   if(HLSMgr->RDiscr->fu_id_control_flow_skip.find(funId) != HLSMgr->RDiscr->fu_id_control_flow_skip.end())
      return DesignFlowStep_Status::SUCCESS;
   HLS->control_flow_checker = structural_managerRef(new structural_manager(HLS->Param));
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const std::string f_name = FB->CGetBehavioralHelper()->get_function_name();
   const std::string cfc_module_name = "control_flow_checker_" + f_name;
   structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(cfc_module_name));
   HLS->control_flow_checker->set_top_info("ControlFlowChecker_i", module_type);
   structural_objectRef checker_circuit = HLS->control_flow_checker->get_circ();
   GetPointer<module>(checker_circuit)->set_copyright(GENERATED_COPYRIGHT);
   GetPointer<module>(checker_circuit)->set_authors("Checker automatically generated by bambu");
   GetPointer<module>(checker_circuit)->set_license(GENERATED_LICENSE);
   bool one_hot_encoding = is_one_hot(HLSMgr);

   const auto& stg = HLS->STG->CGetStg();
   const auto& stg_info = stg->CGetStateTransitionGraphInfo();
   unsigned max_value = 0;
   for(const auto& s : stg_info->state_id_to_vertex)
      max_value = std::max(max_value, s.first);
   const unsigned int state_bitsize = comp_state_bitsize(one_hot_encoding, HLSMgr, funId, max_value);

   size_t epp_trace_bitsize = HLSMgr->RDiscr->fu_id_to_epp_trace_bitsize.at(funId);
   GetPointer<module>(checker_circuit)->set_parameter("EPP_TRACE_BITSIZE", STR(epp_trace_bitsize));
   GetPointer<module>(checker_circuit)->set_parameter("MEMORY_INIT_file", "\"\"trace.mem\"\"");
   GetPointer<module>(checker_circuit)->set_parameter("EPP_TRACE_LENGTH", STR(1));
   GetPointer<module>(checker_circuit)->set_parameter("EPP_MISMATCH_ID", STR(STR(HLSMgr->RDiscr->epp_scope_id)));
   HLSMgr->RDiscr->epp_scope_id++;

   add_common_ports(checker_circuit, state_bitsize);

   HLS->control_flow_checker->add_NP_functionality(checker_circuit, NP_functionality::LIBRARY, cfc_module_name + " out_mismatch_id out_mismatch_trace_offset EPP_TRACE_BITSIZE MEMORY_INIT_file EPP_TRACE_LENGTH EPP_MISMATCH_ID");

   std::string verilog_cf_checker_description = create_control_flow_checker(epp_trace_bitsize, funId, HLS->STG, HLSMgr, one_hot_encoding, max_value, state_bitsize, debug_level);
   add_escape(verilog_cf_checker_description, "\\");
   HLS->control_flow_checker->add_NP_functionality(checker_circuit, NP_functionality::VERILOG_PROVIDED, verilog_cf_checker_description);
   return DesignFlowStep_Status::SUCCESS;
}
