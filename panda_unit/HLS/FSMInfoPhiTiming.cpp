// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

/**
 * @file FSMInfoPhiTiming.cpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "HLS/fsm/FSMInfo.hpp"
#include "HLS/function_allocation/functions.hpp"
#include "HLS/hls_device.hpp"
#include "HLS/hls_manager.hpp"
#include "HLS/memory/memory.hpp"
#include "HLS/memory/memory_allocation.hpp"
#include "HLS/scheduling/schedule.hpp"
#include "HLS/simulation/SimulationInformation.hpp"

#include "IR/behavioral_helper.hpp"
#include "IR/ir_manager.hpp"
#include "IR/ir_node.hpp"
#include "Parameter.hpp"
#include "algorithms/dominance/SemiNCADominance.hpp"
#include "behavior/application_manager.hpp"
#include "behavior/basic_blocks_graph_constructor.hpp"
#include "behavior/call_graph_manager.hpp"
#include "behavior/function_behavior.hpp"
#include "behavior/op_graph.hpp"
#include "behavior/operations_graph_constructor.hpp"
#include "hls.hpp"
#include "token_interface.hpp"
#include "wrapper/BackendWrapper.hpp"

#include <memory>

#include <boost/test/unit_test.hpp>

namespace
{
   class TestParameters : public Parameter
   {
    public:
      TestParameters() : Parameter("libbambu_unit", 0, nullptr)
      {
         SetCommonDefaults();
         setOption(OPT_debug_level, 0);
         setOption(OPT_output_level, 0);
         setOption(OPT_addr_bus_bitsize, 32U);
         setOption(OPT_clock_period, 10.0);
         setOption(OPT_clock_period_resource_fraction, 1.0);
         setOption(OPT_channels_type, MemoryAllocation_ChannelsType::MEM_ACC_11);
         setOption(OPT_memory_allocation_policy, MemoryAllocation_Policy::NONE);
         setOption(OPT_input_file, std::string("fsm_phi_timing.cpp"));
      }

      int Exec() override
      {
         return 0;
      }
      void CheckParameters() override
      {
      }

    protected:
      void SetDefaults() override
      {
      }
      void PrintProgramName(std::ostream&) const override
      {
      }
      void PrintHelp(std::ostream&) const override
      {
      }
   };

   struct PhiTimingFixture
   {
      ParameterRef mutable_parameters{new TestParameters()};
      ParameterConstRef parameters{mutable_parameters};
      HLS_managerRef manager{new HLS_manager(parameters, HLS_device::factory(mutable_parameters))};
      unsigned int function_id{};
      FunctionBehaviorRef behavior;
      std::unique_ptr<OpGraph> graph;
      OpGraph::vertex_descriptor producer;
      OpGraph::vertex_descriptor phi;
      ScheduleRef schedule;

      PhiTimingFixture()
      {
         auto ir = manager->get_ir_manager();
         const auto name = ir->create_ir_node(identifier_node_K, {{TOK(TOK_STRG), "fsm_phi_timing_test"}});
         function_id = ir->new_ir_node_id();
         auto function = std::make_shared<function_val_node>(function_id);
         function->name = name;
         ir->AddIRNode(function);

         auto helper = BehavioralHelperRef(new BehavioralHelper(manager, function_id, parameters));
         behavior = FunctionBehaviorRef(new FunctionBehavior(manager, helper, parameters));
         manager->GetCallGraphManager().AddFunction(function_id, behavior);
         auto hls = HLS_manager::create_HLS(manager, function_id);

         OpNodeInfo producer_info;
         producer_info.bb_index = 3;
         producer = const_cast<OpGraphsCollection&>(behavior->GetOpGraphsCollection()).AddVertex(producer_info);
         OpNodeInfo phi_info;
         phi_info.bb_index = 4;
         phi = const_cast<OpGraphsCollection&>(behavior->GetOpGraphsCollection()).AddVertex(phi_info);
         const_cast<OpGraphsCollection&>(behavior->GetOpGraphsCollection()).GetGraphInfo().SSA2Def[17] = producer;
         graph = std::make_unique<OpGraph>(behavior->GetOpGraph(FunctionBehavior::FFLSAODG));

         schedule = ScheduleRef(new Schedule(manager, function_id, parameters));
         (void)hls;
      }

      unsigned evaluate(bool dummy, unsigned bb_src = 3, unsigned bb_src_state = 3)
      {
         FSMInfo fsm;
         const auto state = fsm.createState({}, {}, {}, 3, {}, {}, 0, 7, false, true, *graph);
         fsm.getState(state).isDummy = dummy;
         fsm.BB2MaxStep[3] = 7;
         return fsm.GetStepPhiIn(*graph, phi, 17, bb_src, bb_src_state, state, schedule);
      }
   };
} // namespace

BOOST_AUTO_TEST_CASE(fsm_phi_timing_accounts_for_dummy_source_state)
{
   PhiTimingFixture fixture;
   BOOST_CHECK_EQUAL(fixture.evaluate(false), 7U);
   BOOST_CHECK_EQUAL(fixture.evaluate(true), 8U);
   BOOST_CHECK_EQUAL(fixture.evaluate(false, 3, 4), 8U);
}
