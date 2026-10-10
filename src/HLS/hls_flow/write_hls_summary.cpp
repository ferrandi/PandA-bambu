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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file write_hls_summary.cpp
 * @brief Class to dump hls summary
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "write_hls_summary.hpp"

#include "Parameter.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "fileIO.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "memory.hpp"

#include <filesystem>

WriteHLSSummary::WriteHLSSummary(const ParameterConstRef _parameters, const HLS_managerRef _hls_mgr,
                                 const DesignFlowManager& _design_flow_manager)
    : HLS_step(_parameters, _hls_mgr, _design_flow_manager, HLSFlowStep_Type::WRITE_HLS_SUMMARY)
{
}

HLS_step::HLSRelationships
WriteHLSSummary::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   HLSRelationships ret;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(),
                                    HLSFlowStep_Relationship::ALL_FUNCTIONS));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

DesignFlowStep_Status WriteHLSSummary::Exec()
{
   for(const auto top_function : HLSMgr->CGetCallGraphManager().GetRootFunctions())
   {
      const hlsRef top_HLS = HLSMgr->get_HLS(top_function);
      top_HLS->PrintResources();
      if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
      {
         const auto out_file_name = parameters->getOption<std::string>(OPT_output_directory) + "/memory_allocation_";
         unsigned int progressive = 0;
         std::string candidate_out_file_name;
         do
         {
            candidate_out_file_name = out_file_name + std::to_string(progressive++) + ".xml";
         } while(std::filesystem::exists(candidate_out_file_name));
         HLSMgr->Rmem->xwrite(candidate_out_file_name);
      }
   }
   return DesignFlowStep_Status::UNCHANGED;
}

bool WriteHLSSummary::HasToBeExecuted() const
{
   return true;
}
