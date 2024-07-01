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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file c_backend.cpp
 * @brief Simple class used to drive the backend in order to be able to print c source code
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "c_backend.hpp"

#include "Parameter.hpp"
#include "application_frontend_flow_step.hpp"
#include "application_manager.hpp"
#include "c_backend_information.hpp"
#include "c_backend_step_factory.hpp"
#include "c_writer.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "frontend_flow_step.hpp"
#include "frontend_flow_step_factory.hpp"
#include "hls_flow_step_factory.hpp"
#include "hls_function_step.hpp"
#include "hls_manager.hpp"
#include "indented_output_stream.hpp"
#include "string_manipulation.hpp"
#include "tree_manager.hpp"
#include "utility.hpp"

CBackend::CBackend(const CBackendInformationConstRef _c_backend_information,
                   const DesignFlowManagerConstRef _design_flow_manager, const application_managerConstRef _AppM,
                   const ParameterConstRef _parameters)
    : DesignFlowStep(ComputeSignature(_c_backend_information), _design_flow_manager, _parameters),
      writer(CWriter::CreateCWriter(_c_backend_information, RefcountCast<const HLS_manager>(_AppM),
                                    IndentedOutputStreamRef(new IndentedOutputStream()))),
      AppM(_AppM),
      c_backend_info(_c_backend_information)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

DesignFlowStepFactoryConstRef CBackend::CGetDesignFlowStepFactory() const
{
   return design_flow_manager.lock()->CGetDesignFlowStepFactory(DesignFlowStep::C_BACKEND);
}

std::string CBackend::GetName() const
{
   return "CBackend::" + c_backend_info->GetName();
}

DesignFlowStep::signature_t CBackend::ComputeSignature(const CBackendInformationConstRef c_backend_info)
{
   return DesignFlowStep::ComputeSignature(C_BACKEND, 0, c_backend_info->GetSignatureContext());
}

void CBackend::ComputeRelationships(DesignFlowStepSet& relationships,
                                    const DesignFlowStep::RelationshipType relationship_type)
{
   const auto DFMgr = design_flow_manager.lock();
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         switch(c_backend_info->type)
         {
            case(CBackendInformation::CB_SEQUENTIAL):
            case(CBackendInformation::CB_MDPI_WRAPPER):
            {
               CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
                   frontend_relationships;
               frontend_relationships.insert(std::make_pair(BAMBU_FRONTEND_FLOW, FrontendFlowStep::WHOLE_APPLICATION));
               FrontendFlowStep::CreateSteps(DFMgr, frontend_relationships, AppM, relationships);
               break;
            }
            case(CBackendInformation::CB_BBP):
            {
               CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
                   frontend_relationships;
               frontend_relationships.insert(
                   std::make_pair(BASIC_BLOCKS_CFG_COMPUTATION, FrontendFlowStep::ALL_FUNCTIONS));
               frontend_relationships.insert(
                   std::make_pair(DEAD_CODE_ELIMINATION_IPA, FrontendFlowStep::WHOLE_APPLICATION));
               frontend_relationships.insert(std::make_pair(LOOP_COMPUTATION, FrontendFlowStep::ALL_FUNCTIONS));
               frontend_relationships.insert(std::make_pair(NI_SSA_LIVENESS, FrontendFlowStep::ALL_FUNCTIONS));
               frontend_relationships.insert(
                   std::make_pair(OPERATIONS_CFG_COMPUTATION, FrontendFlowStep::ALL_FUNCTIONS));
               frontend_relationships.insert(std::make_pair(VAR_ANALYSIS, FrontendFlowStep::ALL_FUNCTIONS));
               FrontendFlowStep::CreateSteps(DFMgr, frontend_relationships, AppM, relationships);
               break;
            }
#if HAVE_HLS_BUILT
            case(CBackendInformation::CB_DISCREPANCY_ANALYSIS):
            case(CBackendInformation::CB_HLS):
            {
               // The first time this step is added, we add the dependence
               // from the complete call graph computation. Ideally we would
               // need also the dependence from the HLS steps,
               // but at this point we don't know the top function yet.
               // The trick is that the dependencies will be recomputed again
               // before this is executed. At that time the top
               // function will be ready. The dependencies from HLS steps are
               // added after the check on the call graph for this reason.
               const auto frontend_step_factory = GetPointer<const FrontendFlowStepFactory>(
                   DFMgr->CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND));
               const auto call_graph_computation_step =
                   DFMgr->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(COMPLETE_CALL_GRAPH));
               const auto cg_design_flow_step =
                   call_graph_computation_step != DesignFlowGraph::null_vertex() ?
                       DFMgr->CGetDesignFlowGraph()->CGetNodeInfo(call_graph_computation_step)->design_flow_step :
                       frontend_step_factory->CreateApplicationFrontendFlowStep(COMPLETE_CALL_GRAPH);
               relationships.insert(cg_design_flow_step);

               // Root function cannot be computed at the beginning so if the
               // call graph is not ready yet we exit. The relationships will
               // be computed again after the call graph computation.
               const auto CGM = AppM->CGetCallGraphManager();
               if(boost::num_vertices(*(CGM->CGetCallGraph())))
               {
                  const auto hls_step_factory =
                      GetPointer<const HLSFlowStepFactory>(DFMgr->CGetDesignFlowStepFactory(DesignFlowStep::HLS));
                  relationships.insert(hls_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::TEST_VECTOR_PARSER,
                                                                           HLSFlowStepSpecializationConstRef()));
               }
               break;
            }
#endif
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         switch(c_backend_info->type)
         {
            case(CBackendInformation::CB_SEQUENTIAL):
            case(CBackendInformation::CB_MDPI_WRAPPER):
            case(CBackendInformation::CB_BBP):
            {
               CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
                   frontend_relationships;
               frontend_relationships.insert(
                   std::make_pair(FrontendFlowStepType::MULTIPLE_ENTRY_IF_REDUCTION, FrontendFlowStep::ALL_FUNCTIONS));
               FrontendFlowStep::CreateSteps(DFMgr, frontend_relationships, AppM, relationships);
               if(c_backend_info->type != CBackendInformation::CB_BBP)
               {
                  // The first time this step is added, we add the dependence
                  // from the complete call graph computation. Ideally we would
                  // need also the dependence from the HLS steps,
                  // but at this point we don't know the top function yet.
                  // The trick is that the dependencies will be recomputed again
                  // before this is executed. At that time the top
                  // function will be ready. The dependencies from HLS steps are
                  // added after the check on the call graph for this reason.
                  const auto frontend_step_factory = GetPointer<const FrontendFlowStepFactory>(
                      DFMgr->CGetDesignFlowStepFactory(DesignFlowStep::FRONTEND));
                  const auto call_graph_computation_step =
                      DFMgr->GetDesignFlowStep(ApplicationFrontendFlowStep::ComputeSignature(COMPLETE_CALL_GRAPH));
                  const auto cg_design_flow_step =
                      call_graph_computation_step != DesignFlowGraph::null_vertex() ?
                          DFMgr->CGetDesignFlowGraph()->CGetNodeInfo(call_graph_computation_step)->design_flow_step :
                          frontend_step_factory->CreateApplicationFrontendFlowStep(COMPLETE_CALL_GRAPH);
                  relationships.insert(cg_design_flow_step);

                  // Root function cannot be computed at the beginning so if the
                  // call graph is not ready yet we exit. The relationships will
                  // be computed again after the call graph computation.
                  const auto CG = AppM->CGetCallGraphManager()->CGetCallGraph();
                  if(boost::num_vertices(*CG))
                  {
                     const auto top_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
                     for(const auto& top_symbol : top_symbols)
                     {
                        const auto top_fnode = AppM->get_tree_manager()->GetFunction(top_symbol);
                        const auto hls_step_factory =
                            GetPointer<const HLSFlowStepFactory>(DFMgr->CGetDesignFlowStepFactory(DesignFlowStep::HLS));
                        const auto hls_top_function = DFMgr->GetDesignFlowStep(
                            HLSFunctionStep::ComputeSignature(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW,
                                                              HLSFlowStepSpecializationConstRef(), top_fnode->index));
                        const auto hls_top_function_step =
                            hls_top_function != DesignFlowGraph::null_vertex() ?
                                DFMgr->CGetDesignFlowGraph()->CGetNodeInfo(hls_top_function)->design_flow_step :
                                hls_step_factory->CreateHLSFlowStep(HLSFlowStep_Type::HLS_SYNTHESIS_FLOW,
                                                                    top_fnode->index);
                        relationships.insert(hls_top_function_step);
                     }
                  }
               }
               break;
            }
#if HAVE_HLS_BUILT
            case(CBackendInformation::CB_DISCREPANCY_ANALYSIS):
            case(CBackendInformation::CB_HLS):
            {
               break;
            }
#endif
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
}

bool CBackend::HasToBeExecuted() const
{
   return true;
}

void CBackend::Initialize()
{
   if(c_backend_info->src_filename.has_parent_path())
   {
      std::filesystem::create_directories(c_backend_info->src_filename.parent_path());
   }
   std::filesystem::remove(c_backend_info->src_filename);
}

DesignFlowStep_Status CBackend::Exec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->writing " + c_backend_info->src_filename.string());
   writer->WriteFile(c_backend_info->src_filename);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--written " + c_backend_info->src_filename.string());
   return DesignFlowStep_Status::SUCCESS;
}
