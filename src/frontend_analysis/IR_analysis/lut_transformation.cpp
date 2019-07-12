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

#pragma region Macros declaration

#define IS_GIMPLE_ASSIGN(it) GET_NODE(*it)->get_kind() == gimple_assign_K
#define CHECK_BIN_EXPR_SIZE(binaryExpression) (static_cast<int>(tree_helper::Size(GET_NODE(binaryExpression->op0))) == 1 && static_cast<int>(tree_helper::Size(GET_NODE(binaryExpression->op1))) == 1)

#pragma endregion

#pragma region Types declaration

/**
 * `aig_network_ext` class provides operations derived from the one already existing in `mockturtle::aig_network`.
 */
class lut_transformation::aig_network_ext : public mockturtle::aig_network {
public:
    /**
     * Creates a 'greater' or equal operation.
     * 
     * @param a a `mockturtle::signal` representing the first operator of the `ge` operation
     * @param b a `mockturtle::signal` representing the second operator of the `ge` operation
     * 
     * @return a `mockturtle::signal` representing the operation `ge` between `a` and `b`
     */
    signal create_ge(signal const &a, signal const &b) {
        return !this->create_lt(a, b);
    }

    /**
     * Creates a 'greater' operation.
     * 
     * @param a a `mockturtle::signal` representing the first operator of the `gt` operation
     * @param b a `mockturtle::signal` representing the second operator of the `gt` operation
     * 
     * @return a `mockturtle::signal` representing the operation `gt` between `a` and `b`
     */
    signal create_gt(signal const &a, signal const &b) {
        return !this->create_le(a, b);
    }

    /**
     * Creates a 'equal' operation.
     * 
     * @param a a `mockturtle::signal` representing the first operator of the `gt` operation
     * @param b a `mockturtle::signal` representing the second operator of the `gt` operation
     * 
     * @return a `mockturtle::signal` representing the operation `eq` between `a` and `b`
     */
    signal create_eq(signal const &a, signal const &b) {
        return !this->create_xor(a, b);
    }

    /**
     * Creates a 'not equal' operation.
     * 
     * @param a a `mockturtle::signal` representing the first operator of the `ne` operation
     * @param b a `mockturtle::signal` representing the second operator of the `ne` operation
     * 
     * @return a `mockturtle::signal` representing the operation `ne` between `a` and `b`
     */
    signal create_ne(signal const &a, signal const &b) {
        return this->create_xor(a, b);
    }

    /**
     * Creates a 'and' operation.
     * Although the `create_and` operation already exists inside `mockturtle::aig_network` has different
     * inputs than all others operations (input signals are not constant).
     * 
     * @param a a `mockturtle::signal` representing the first operator of the `and` operation
     * @param b a `mockturtle::signal` representing the second operator of the `and` operation
     * 
     * @return a `mockturtle::signal` representing the operation `and` between `a` and `b`
     */
    signal create_and(signal const &a, signal const &b) {
        return mockturtle::aig_network::create_and(a, b);
    }
};

/**
 * Pointer that points to the function, of `aig_network_ext`, that represents a binary operation between two `mockturtle::aig_network::signal` 
 * and returns a `mockturtle::aig_network::signal`.
 */
// typedef mockturtle::aig_network::signal (lut_transformation::aig_network_ext::*aig_network_fn)(const mockturtle::aig_network::signal &, const mockturtle::aig_network::signal &);

#pragma endregion

tree_nodeRef lut_transformation::CreateGimpleAssign(const tree_nodeRef type, const tree_nodeRef op, const unsigned int bb_index, const std::string &srcp_default) {
   return tree_man->CreateGimpleAssign(type, tree_nodeRef(), tree_nodeRef(), op, bb_index, srcp_default);
}

/**
 * Checks if the provided `gimple_assign` is a primary output of lut network.
 * 
 * @param gimpleAssign the `gimple_assign` to check
 * @return whether the provided `gimple_assign` is a primary output
 */
bool lut_transformation::CheckIfPO(gimple_assign *gimpleAssign) {
    /// the index of the basic block holding the provided `gimpleAssign`
    const unsigned int currentBBIndex = gimpleAssign->bb_index;
    // the variables that uses the result of the provided `gimpleAssign`
    const std::vector<boost::shared_ptr<tree_node> > usedIn = gimpleAssign->use_set->variables;

    for (auto node : usedIn) {
        auto *childGimpleNode = GetPointer<gimple_node>(node);

        // the current operation is a primary output if it is used in
        // operation not belonging to the current basic block or if the operation
        // in which it is used does not belong to the K-operations set
        if (childGimpleNode->bb_index != currentBBIndex || node->get_kind() != gimple_assign_K) {
            return true;
        }
        else {
            auto *childGimpleAssign = GetPointer<gimple_assign>(node);
            enum kind code = GET_NODE(childGimpleAssign->op1)->get_kind();

            // it is a `PO` if code is not contained into `lutExpressibleOperations`
            return std::find(lutExpressibleOperations.begin(), lutExpressibleOperations.end(), code) == lutExpressibleOperations.end();
    }
    }

    return false;
}

lut_transformation::aig_network_fn lut_transformation::GetNodeCreationFunction(enum kind code) {
    switch (code) {
        case bit_and_expr_K:
        case truth_and_expr_K:
            return &aig_network_ext::create_and;
        case bit_ior_expr_K:
        case truth_or_expr_K:
            return &aig_network_ext::create_or;
        case bit_xor_expr_K:
        case truth_xor_expr_K:
            return &aig_network_ext::create_xor;
        case eq_expr_K:
            return &aig_network_ext::create_eq;
        case ge_expr_K:
            return &aig_network_ext::create_ge;
        case lut_expr_K:
            break; // TODO: capire come sono descritte e come inserirle dentro a mockturtle
        case gt_expr_K:
            return &aig_network_ext::create_gt;
        case le_expr_K:
            return &aig_network_ext::create_le;
        case lt_expr_K:
            return &aig_network_ext::create_lt;
        case ne_expr_K:
            return &aig_network_ext::create_ne;
        default:
            return nullptr;
    }
}

bool lut_transformation::ProcessBasicBlock(std::pair<unsigned int, blocRef> block) {
        aig_network_ext aig;
    std::map<tree_nodeRef, mockturtle::aig_network::signal> nodeRefToSignal;
        std::map<mockturtle::aig_network::signal, tree_nodeRef> signalToNodeRef;
    std::map<mockturtle::aig_network::signal, std::pair<tree_nodeRef, std::list<tree_nodeRef>::iterator> > signalToOutputNode;

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
            if(!AppM->ApplyNewTransformation()) {
                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reached max cfg transformations");
                statementsIterator++;
                continue;
            }
#endif

        if (!IS_GIMPLE_ASSIGN(statementsIterator)) {
                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(*statementsIterator)->ToString());
                statementsIterator++;
            }

            auto *gimpleAssign = GetPointer<gimple_assign>(GET_NODE(*statementsIterator));
            const std::string srcp_default = gimpleAssign->include_name + ":" + STR(gimpleAssign->line_number) + ":" + STR(gimpleAssign->column_number);
            enum kind code1 = GET_NODE(gimpleAssign->op1)->get_kind();
            
            auto *binaryExpression = GetPointer<binary_expr>(GET_NODE(gimpleAssign->op1));

            THROW_ASSERT(binaryExpression->op0 && binaryExpression->op1, "expected two parameters");

        // the operands must be booleans
        if (!CHECK_BIN_EXPR_SIZE(binaryExpression)) {
            statementsIterator++;
                continue;
            } 

            mockturtle::aig_network::signal res;
            mockturtle::aig_network::signal op1;
            mockturtle::aig_network::signal op2;
        lut_transformation::aig_network_fn nodeCreateFn;

        // if the first operand has already been processed then the previous signal is used
            if (nodeRefToSignal.find(binaryExpression->op0) != nodeRefToSignal.end()) {
                op1 = nodeRefToSignal[binaryExpression->op0];
            }
        else { // otherwise the operand is a primary input
                op1 = aig.create_pi();

                nodeRefToSignal[binaryExpression->op0] = op1;
                signalToNodeRef[op1] = binaryExpression->op0;
            }

        // if the second operand has already been processed then the previous signal is used
            if (nodeRefToSignal.find(binaryExpression->op1) != nodeRefToSignal.end()) {
                op2 = nodeRefToSignal[binaryExpression->op1];
            }
        else { // otherwise the operand is a primary input
                op2 = aig.create_pi();

                nodeRefToSignal[binaryExpression->op1] = op2;
                signalToNodeRef[op2] = binaryExpression->op1;
            }

        nodeCreateFn = this->GetNodeCreationFunction(code1);

        if (nodeCreateFn == nullptr) {
            statementsIterator++;
            continue;
            }

            res = (aig.*nodeCreateFn)(op1, op2);
            nodeRefToSignal[gimpleAssign->op0] = res;
            signalToNodeRef[res] = gimpleAssign->op0;
        signalToOutputNode[res] = std::make_pair<tree_nodeRef, std::list<tree_nodeRef>::iterator>(gimpleAssign->op0, statementsIterator);

        if (this->CheckIfPO(gimpleAssign)) {
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

    mockturtle::write_bench(lut, std::cout);

    lut.foreach_node([&](auto const &node) {
        
    });

        // dalla network a lut_expr_K
        // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added to LUT list : " + STR(lut_ga));
        // INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Modified statement " + GET_NODE(lut_ga)->ToString());

        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examining BB" + STR(block.first));

    return statementsCount != statements.size();
}

#pragma region Life cycle
lut_transformation::~lut_transformation() = default;

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

lut_transformation::lut_transformation(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
     : FunctionFrontendFlowStep(_AppM, _function_id, LUT_TRANSFORMATION, _design_flow_manager, Param), max_lut_size(NUM_CST_allocation_default_max_lut_size) {
    debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

DesignFlowStep_Status lut_transformation::InternalExec() {
    tree_nodeRef temp = TM->get_tree_node_const(function_id);
    auto* fd = GetPointer<function_decl>(temp);
    THROW_ASSERT(fd && fd->body, "Node is not a function or it has not a body");
    auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
    THROW_ASSERT(sl, "Body is not a statement list");

    bool modified = false;

    for (std::pair<unsigned int, blocRef> block : sl->list_of_bloc) {
        modified |= this->ProcessBasicBlock(block);
    }

    if (modified) {
        function_behavior->UpdateBBVersion();
        return DesignFlowStep_Status::SUCCESS;
    }

    return DesignFlowStep_Status::UNCHANGED;
}

const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> lut_transformation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const {
    std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
    switch(relationship_type) {
        case(DEPENDENCE_RELATIONSHIP):
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
            relationships.insert(
                std::pair<FrontendFlowStepType, 
                FunctionRelationship>(MULTI_WAY_IF, SAME_FUNCTION)
            );
            break;
        default:
            THROW_UNREACHABLE("");
    }
    return relationships;
}
#pragma endregion