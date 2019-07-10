/*
 *
 *                         _/_/_/     _/_/    _/     _/ _/_/_/     _/_/
 *                        _/    _/ _/     _/ _/_/  _/ _/    _/ _/     _/
 *                      _/_/_/  _/_/_/_/ _/  _/_/ _/    _/ _/_/_/_/
 *                     _/        _/     _/ _/     _/ _/    _/ _/     _/
 *                    _/        _/     _/ _/     _/ _/_/_/  _/     _/
 *
 *                 ***********************************************
 *                                        PandA Project
 *                            URL: http://panda.dei.polimi.it
 *                              Politecnico di Milano - DEIB
 *                                System Architectures Group
 *                 ***********************************************
 *                  Copyright (C) 2004-2019 Politecnico di Milano
 *
 *    This file is part of the PandA framework.
 *
 *    The PandA framework is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/**
 * @file lut_transformation.hpp
 * @brief recognize lut expressions.
 * @author Di Simone Jacopo
 * @author Cappello Paolo
 * @author Inajjar Ilyas
 * @author Angelo Gallarello
 * @author Stefano Longari
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#include "lut_transformation.hpp"

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "function_behavior.hpp"

/// constants include
#include "allocation_constants.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/technology includes
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"

/// HLS includes
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// STD include
#include <fstream>

#if HAVE_BAMBU_BUILT
/// technology include
#include "technology_manager.hpp"

/// technology/physical_library/modes include
#include "time_model.hpp"
#endif

/// tree includes
#include "behavioral_helper.hpp"

/// technology/physical_library include
#include "technology_node.hpp"

/// utility include
#include "math_function.hpp"

/// tree includes
#include "dbgPrintHelper.hpp"        // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_reindex.hpp"

#include <mockturtle/mockturtle.hpp>

class aig_network_ext : public mockturtle::aig_network {
public:
    signal create_ge(signal const &a, signal const &b) {
        return !this->create_lt(a, b);
    }

    signal create_gt(signal const &a, signal const &b) {
        return !this->create_le(a, b);
    }

    signal create_eq(signal const &a, signal const &b) {
        return !this->create_xor(a, b);
    }

    signal create_ne(signal const &a, signal const &b) {
        return this->create_xor(a, b);
    }
};

typedef mockturtle::aig_network::signal (aig_network_ext::*aig_network_fn)(const mockturtle::aig_network::signal &, const mockturtle::aig_network::signal &);

tree_nodeRef lut_transformation::CreateGimpleAssign(const tree_nodeRef type, const tree_nodeRef op, const unsigned int bb_index, const std::string &srcp_default) {
   return tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), op, bb_index, srcp_default);
}

lut_transformation::lut_transformation(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
     : FunctionFrontendFlowStep(_AppM, _function_id, LUT_TRANSFORMATION, _design_flow_manager, Param), max_lut_size(NUM_CST_allocation_default_max_lut_size) {
    debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

bool lut_transformation::CheckIfPO(const unsigned int currentBBIndex, const std::vector<boost::shared_ptr<tree_node> > usedIn) {
    std::vector<unsigned int> usedInBBIndexes = std::transform(usedIn.begin(), usedIn.end(), [](const tree_nodeRef node) {
        auto *gimpleNode = GetPointer<gimple_node>(node);
        return gimpleNode->bb_index;
    });

    for (unsigned int index : usedInBBIndexes) {
        if (index != currentBBIndex) {
            return true;
        }
    }

    return false;
}

const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> lut_transformation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const {
    std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
    switch(relationship_type) {
        case(DEPENDENCE_RELATIONSHIP):
         if(not parameters->getOption<int>(OPT_gcc_openmp_simd))
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BIT_VALUE_OPT, SAME_FUNCTION));
            break;
        case(INVALIDATION_RELATIONSHIP):
            if (GetStatus() == DesignFlowStep_Status::SUCCESS) {
                relationships.insert(
                    std::pair<FrontendFlowStepType, 
                    FunctionRelationship>(DEAD_CODE_ELIMINATION, SAME_FUNCTION)
                );
            }
            break;
        case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTI_WAY_IF, SAME_FUNCTION));
            break;
        default:
            THROW_UNREACHABLE("");
    }
    return relationships;
}

lut_transformation::~lut_transformation() = default;

DesignFlowStep_Status lut_transformation::InternalExec() {
    tree_nodeRef temp = TM->get_tree_node_const(function_id);
    auto* fd = GetPointer<function_decl>(temp);
    THROW_ASSERT(fd && fd->body, "Node is not a function or it has not a body");
    auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
    THROW_ASSERT(sl, "Body is not a statement list");

    bool modified = false;

    for (std::pair<unsigned int, blocRef> block : sl->list_of_bloc) {
        aig_network_ext aig;
        std::map<mockturtle::aig_network::signal, tree_nodeRef> signalToNodeRef;
        std::map<tree_nodeRef, mockturtle::aig_network::signal> nodeRefToSignal;

        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
        const auto &statements = block.second->CGetStmtList();
        /// end of statements list
        auto statementsEnd = statements.end();
        /// size of statements list
        size_t statementsCount = statements.size();
        /// start of statements list
        auto statementsIterator = statements.begin();

        while (statementsIterator != statementsEnd) {
            #ifndef NDEBUG
            if(not AppM->ApplyNewTransformation()) {
                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reached max cfg transformations");
                statementsIterator++;
                continue;
            }
            #endif

            if (!GET_NODE(*statementsIterator)->get_kind() != gimple_assign_K) {
                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(*statementsIterator)->ToString());
                statementsIterator++;
            }

            auto *gimpleAssign = GetPointer<gimple_assign>(GET_NODE(*statementsIterator));
            const std::string srcp_default = gimpleAssign->include_name + ":" + STR(gimpleAssign->line_number) + ":" + STR(gimpleAssign->column_number);
            enum kind code1 = GET_NODE(gimpleAssign->op1)->get_kind();
            
            auto *binaryExpression = GetPointer<binary_expr>(GET_NODE(gimpleAssign->op1));

            THROW_ASSERT(binaryExpression->op0 && binaryExpression->op1, "expected two parameters");
            int data_size0 = static_cast<int>(tree_helper::Size(GET_NODE(binaryExpression->op0)));
            int data_size1 = static_cast<int>(tree_helper::Size(GET_NODE(binaryExpression->op1)));

            /* 
                && !tree_helper::is_constant(TM, binaryExpression->op0->index) 
                && !tree_helper::is_constant(TM, binaryExpression->op1->index)
            */
            if (!(data_size0 == 1
                && data_size1 == 1)) {
                continue;
            } 

            mockturtle::aig_network::signal res;
            mockturtle::aig_network::signal op1;
            mockturtle::aig_network::signal op2;
            aig_network_fn nodeCreateFn;

            if (nodeRefToSignal.find(binaryExpression->op0) != nodeRefToSignal.end()) {
                op1 = nodeRefToSignal[binaryExpression->op0];
            }
            else {
                op1 = aig.create_pi();

                nodeRefToSignal[binaryExpression->op0] = op1;
                signalToNodeRef[op1] = binaryExpression->op0;
            }

            if (nodeRefToSignal.find(binaryExpression->op1) != nodeRefToSignal.end()) {
                op2 = nodeRefToSignal[binaryExpression->op1];
            }
            else {
                op2 = aig.create_pi();

                nodeRefToSignal[binaryExpression->op1] = op2;
                signalToNodeRef[op2] = binaryExpression->op1;
            }

            switch (code1) {
                case bit_and_expr_K:
                case truth_and_expr_K:
                    nodeCreateFn = &aig_network_ext::create_and;
                    break;
                case bit_ior_expr_K:
                case truth_or_expr_K:
                    nodeCreateFn = &aig_network_ext::create_or;
                    break;
                case bit_xor_expr_K:
                case truth_xor_expr_K:
                    nodeCreateFn = &aig_network_ext::create_xor;
                    break;
                case eq_expr_K:
                    nodeCreateFn = &aig_network_ext::create_eq;
                    break;
                case ge_expr_K:
                    nodeCreateFn = &aig_network_ext::create_ge;
                    break;
                case lut_expr_K:
                    break; // TODO: capire come sono descritte e come inserirle dentro a mockturtle
                case gt_expr_K:
                    nodeCreateFn = &aig_network_ext::create_gt;
                    break;
                case le_expr_K:
                    nodeCreateFn = &aig_network_ext::create_le;
                    break;
                case lt_expr_K:
                    nodeCreateFn = &aig_network_ext::create_lt;
                    break;
                case ne_expr_K:
                    nodeCreateFn = &aig_network_ext::create_ne;
                    break;
            }

            res = (aig.*nodeCreateFn)(op1, op2);
            nodeRefToSignal[gimpleAssign->op0] = res;
            signalToNodeRef[res] = gimpleAssign->op0;

            if (this->CheckIfPO(block.first, gimpleAssign->use_set->variables)) {
                aig.create_po(res);
            }

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(*statementsIterator)->ToString());
            statementsIterator++;
        }

        mockturtle::mapping_view<mockturtle::aig_network, true> mapped_aig{aig};

        mockturtle::lut_mapping_params ps;
        ps.cut_enumeration_ps.cut_size = max_lut_size; // parameter
        mockturtle::lut_mapping<mockturtle::mapping_view<mockturtle::aig_network, true>, true>(mapped_aig, ps);
        auto lut = *mockturtle::collapse_mapped_network<mockturtle::klut_network>(mapped_aig);

        // dalla network a lut_expr_K
        // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added to LUT list : " + STR(lut_ga));
        // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Modified statement " + GET_NODE(lut_ga)->ToString());

        modified = statementsCount != statements.size();
        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examining BB" + STR(block.first));
    }

    if (modified) {
        function_behavior->UpdateBBVersion();
        return DesignFlowStep_Status::SUCCESS;
    }

    return DesignFlowStep_Status::UNCHANGED;
}

// DesignFlowStep_Status lut_transformation::InternalExec()
// {
//     bool modified = false;
//     tree_nodeRef temp = TM->get_tree_node_const(function_id);
//     auto* fd = GetPointer<function_decl>(temp);
//     THROW_ASSERT(fd && fd->body, "Node is not a function or it has not a body");
//     auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
//     THROW_ASSERT(sl, "Body is not a statement list");
//     /// iteration over basic blocks
//     for(auto block : sl->list_of_bloc)
//     {
//         std::list<tree_nodeRef> lutList;

//         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
//         const auto& list_of_stmt = block.second->CGetStmtList();
//         /// end of statements list
//         auto it_los_end = list_of_stmt.end();
//         /// size of statements list
//         size_t n_stmts = list_of_stmt.size();
//         /// start of statements list
//         auto it_los = list_of_stmt.begin();
//         /// iteration over statements
//         while(it_los != it_los_end)
//         {
// #ifndef NDEBUG
//             if(not AppM->ApplyNewTransformation())
//             {
//                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reached max cfg transformations");
//                 it_los++;
//                 continue;
//             }
// #endif
//             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(*it_los)->ToString());
            
//             if (GET_NODE(*it_los)->get_kind() == gimple_assign_K)
//             {
//                 auto* ga = GetPointer<gimple_assign>(GET_NODE(*it_los));
//                 const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
//                 // second operand : the right part of the assignment
//                 enum kind code1 = GET_NODE(ga->op1)->get_kind();
//                 long long int lut_number = 0;

//                 if (code1 == bit_and_expr_K || code1 == truth_and_expr_K)
//                 {
//                     lut_number = 8;
//                     // in1 = aig.create_pi();
//                     // in2 = aig.create_pi();
//                     // aig.create_and(in1, in2);
//                 }
//                 else if (code1 == bit_ior_expr_K || code1 == truth_or_expr_K)
//                 {
//                     lut_number = 14;
//                     // in1 = aig.create_pi();
//                     // in2 = aig.create_pi();
//                     // aig.create_or(in1, in2);
//                 }
//                 else if (code1 == bit_xor_expr_K || code1 == truth_xor_expr_K)
//                 {
//                     lut_number = 6; 
//                     // in1 = aig.create_pi();
//                     // in2 = aig.create_pi();
//                     // aig.create_xor(in1, in2);
//                 }

//                 if (lut_number != 0)
//                 {
//                     auto* be = GetPointer<binary_expr>(GET_NODE(ga->op1));
//                     THROW_ASSERT(be->op0 && be->op1, "expected two parameters");
//                     int data_size0 = static_cast<int>(tree_helper::Size(GET_NODE(be->op0)));
//                     int data_size1 = static_cast<int>(tree_helper::Size(GET_NODE(be->op1)));
//                     if (data_size0 == 1 and data_size1 == 1 and not tree_helper::is_constant(TM, be->op0->index) and not tree_helper::is_constant(TM, be->op1->index))
//                     {
//                         const auto type = tree_man->CreateDefaultUnsignedLongLongInt();

//                         tree_nodeRef convert_op = tree_man->create_unary_operation(type, be->op0, srcp_default, convert_expr_K);
//                         tree_nodeRef convert_ga = CreateGimpleAssign(type, convert_op, block.first, srcp_default);
//                         block.second->PushBefore(convert_ga, *it_los);

//                         unsigned int integer_cst1_id = TM->new_tree_node_id();
//                         tree_nodeRef const1 = tree_man->CreateIntegerCst(type, 1, integer_cst1_id);

//                         tree_nodeRef mask_op = tree_man->create_binary_operation(type, GetPointer<const gimple_assign>(GET_CONST_NODE(convert_ga))->op0, const1, srcp_default, bit_and_expr_K);
//                         tree_nodeRef mask_ga = CreateGimpleAssign(type, mask_op, block.first, srcp_default);
//                         block.second->PushBefore(mask_ga, *it_los);

//                         tree_nodeRef lshift_op = tree_man->create_binary_operation(type, GetPointer<const gimple_assign>(GET_CONST_NODE((mask_ga)))->op0, const1, srcp_default, lshift_expr_K);
//                         tree_nodeRef lshift_ga = CreateGimpleAssign(type, lshift_op, block.first, srcp_default);
//                         block.second->PushBefore(lshift_ga, *it_los);

//                         tree_nodeRef conc_op = tree_man->create_ternary_operation(type, GetPointer<const gimple_assign>(GET_CONST_NODE(lshift_ga))->op0, be->op1, const1, srcp_default, bit_ior_concat_expr_K);
//                         tree_nodeRef conc_ga = CreateGimpleAssign(type, conc_op, block.first, srcp_default);
//                         GetPointer<gimple_assign>(GET_NODE(conc_ga))->temporary_address = true;
//                         tree_nodeRef result_conc = GetPointer<gimple_assign>(GET_NODE(conc_ga))->op0;
//                         GetPointer<ssa_name>(GET_NODE(result_conc))->bit_values = "UU";
//                         // insert concatenation node
//                         block.second->PushBefore(conc_ga, *it_los);
//                         // create lut node
//                         // transform constant to tree_nodeRef
//                         unsigned int integer_cst2_id = TM->new_tree_node_id();
//                         tree_nodeRef and_op_cst = tree_man->CreateIntegerCst(type, lut_number, integer_cst2_id);
//                         const auto type1 = tree_man->CreateDefaultUnsignedLongLongInt();
//                         tree_nodeRef new_op1 = tree_man->create_binary_operation(be->type, result_conc, and_op_cst, srcp_default, lut_expr_K);
//                         tree_nodeRef lut_ga = CreateGimpleAssign(be->type, new_op1, block.first, srcp_default);
//                         GetPointer<gimple_assign>(GET_NODE(lut_ga))->op0 = ga->op0;
//                         // insert lut node
//                         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replacing " + STR(*it_los) + " with " + STR(lut_ga));
//                         block.second->Replace(*it_los, lut_ga, true);
// #ifndef NDEBUG
//                         AppM->RegisterTransformation(GetName(), lut_ga);
// #endif
//                         lutList.push_back(lut_ga);
//                         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added to LUT list : " + STR(lut_ga));
//                         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Modified statement " + GET_NODE(lut_ga)->ToString());
//                         it_los = list_of_stmt.begin();
//                         it_los_end = list_of_stmt.end();
//                         continue;
//                     }
//                 }
//             }
//             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(*it_los)->ToString());
//             it_los++;
//         }
//         // Merge all the LUT in the block
//         MergeLut(lutList, block);

//         if(n_stmts != list_of_stmt.size())
//             modified = true;
//         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examining BB" + STR(block.first));
//     }
//     if(modified)
//         function_behavior->UpdateBBVersion();
//     return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
// }

void lut_transformation::Initialize() {
    TM = AppM->get_tree_manager();
    tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
    THROW_ASSERT(GetPointer<const HLS_manager>(AppM)->get_HLS_target(), "");
    const auto hls_target = GetPointer<const HLS_manager>(AppM)->get_HLS_target();
    THROW_ASSERT(hls_target->get_target_device()->has_parameter("max_lut_size"), "");
    max_lut_size = hls_target->get_target_device()->get_parameter<size_t>("max_lut_size");
}

void lut_transformation::ComputeRelationships(DesignFlowStepSet &relationship, const DesignFlowStep::RelationshipType relationship_type) {
    switch(relationship_type) {
        case(PRECEDENCE_RELATIONSHIP):
            break;
        case DEPENDENCE_RELATIONSHIP: {
            const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
            const auto *technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
            const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
            const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
            const DesignFlowStepRef technology_design_flow_step = technology_flow_step ? 
                design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : 
                technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
            relationship.insert(technology_design_flow_step);

            break;
        }
        case INVALIDATION_RELATIONSHIP:
            break;
        default:
            THROW_UNREACHABLE("");
    }

    FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}
