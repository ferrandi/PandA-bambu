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
 * @file write_hls_summary.cpp
 * @brief Class to dump hls summary
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "write_hls_summary.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "call_graph_manager.hpp"

/// HLS include
#include "hls.hpp"
#include "hls_manager.hpp"

#include "dbgPrintHelper.hpp"
#include "memory.hpp"

#include <boost/filesystem/operations.hpp>

WriteHLSSummary::WriteHLSSummary(const ParameterConstRef _parameters, const HLS_managerRef _hls_mgr, const DesignFlowManagerConstRef _design_flow_manager) : HLS_step(_parameters, _hls_mgr, _design_flow_manager, HLSFlowStep_Type::WRITE_HLS_SUMMARY)
{
}

WriteHLSSummary::~WriteHLSSummary() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> WriteHLSSummary::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::ALL_FUNCTIONS));
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

DesignFlowStep_Status WriteHLSSummary::Exec()
{
   for(const auto top_function : HLSMgr->CGetCallGraphManager()->GetRootFunctions())
   {
      const hlsRef top_HLS = HLSMgr->get_HLS(top_function);
      top_HLS->PrintResources();
      if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
      {
         std::string out_file_name = "memory_allocation";
         unsigned int progressive = 0;
         std::string candidate_out_file_name;
         do
         {
            candidate_out_file_name = out_file_name + "_" + std::to_string(progressive++) + ".xml";
         } while(boost::filesystem::exists(candidate_out_file_name));
         out_file_name = candidate_out_file_name;
         HLSMgr->Rmem->xwrite(out_file_name);
      }
#if 0
      std::string out_file_name = "hls_summary";
      unsigned int progressive = 0;
      std::string candidate_out_file_name;
      do
      {
         candidate_out_file_name = out_file_name + "_" + std::to_string(progressive++) + ".xml";
      } while (boost::filesystem::exists(candidate_out_file_name));
      out_file_name = candidate_out_file_name;
      HLSMgr->xwrite(out_file_name);
#endif
   }
   return DesignFlowStep_Status::UNCHANGED;
}

bool WriteHLSSummary::HasToBeExecuted() const
{
   return true;
}
