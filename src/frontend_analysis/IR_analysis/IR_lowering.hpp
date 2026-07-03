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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file IR_lowering.hpp
 * @brief Decompose some complex statements into set of simple operations.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef IR_LOWERING_HPP
#define IR_LOWERING_HPP
#include "custom_set.hpp"
#include "design_flow_step.hpp"
#include "frontend_flow_step.hpp"
#include "function_frontend_flow_step.hpp"
#include "refcount.hpp"

#include <list>
#include <string>
#include <utility>

REF_FORWARD_DECL(bloc);
class constant_int_val_node;
enum kind : int;
REF_FORWARD_DECL(IR_lowering);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_manipulation);
REF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(HLS_device);
class statement_list_node;
struct assign_stmt;

/**
 * Compute the control flow graph for the operations.
 */
class IR_lowering : public FunctionFrontendFlowStep
{
 private:
   /// The IR manager
   ir_managerRef TM;

   /// The IR manipulation
   ir_manipulationRef ir_man;

   std::string constdiv_lowering_mode;
   double constdiv_dsp_scale_k;
   std::string constmultdiv_score;
   std::string constmultdiv_decision_metric;
   std::string constmultdiv_lut_cost_model;
   double constmultdiv_w_latency;
   double constmultdiv_w_area;
   bool constdiv_composite_enable;
   double constdiv_composite_margin;
   unsigned constdiv_composite_max_pairs;
   bool constmul_enable;
   bool constmul_balance_tree;
   unsigned constmul_balance_tree_min_terms;
   unsigned constmul_max_terms;
   unsigned constmul_max_depth;
   bool constmul_try_factor_forms;
   bool constmul_enable_small_factor_chains;
   double constmul_dsp_scale_k;
   bool constmul_kcm_enable;
   unsigned constmul_kcm_alpha;
   std::string constmul_kcm_sum_strategy;
   bool constmul_kcm_merge_table_add;
   std::string constmul_kcm_cost_model;
   bool constdivmul_params_initialized;

   void initConstDivMulLoweringParams();
   double getConstMultDivScore(double latency, double area) const;
   std::string getEffectiveConstMultDivDecisionMetric() const;

   /**
    * Expand signed modulus of OP0 by a power of two D in mode MODE.
    */
   ir_nodeRef expand_smod_pow2(const ir_nodeRef& op0, unsigned long long int d, const ir_nodeRef& stmt,
                               const blocRef& block, const ir_nodeRef& type, const std::string& loc_info_default);

   /**
    * Expand signed division of OP0 by a power of two D in mode MODE.
    * This routine is only called for positive values of D.
    */
   ir_nodeRef expand_sdiv_pow2(const ir_nodeRef& op0, unsigned long long int d, const ir_nodeRef& stmt,
                               const blocRef& block, const ir_nodeRef& type, const std::string& loc_info_default);

   ir_nodeRef decomposeMultiplicationByConstant(const ir_nodeRef& op0, const constant_int_val_node* ic_node,
                                                const ir_nodeRef& old_target, const ir_nodeRef& stmt,
                                                const blocRef& block, const ir_nodeRef& type_expr,
                                                const std::string& loc_info_default);

   ir_nodeRef expand_mult_highpart(const ir_nodeRef& op0, unsigned long long int ml, const ir_nodeRef& type_expr,
                                   int data_bitsize, const std::list<ir_nodeRef>::const_iterator it_los,
                                   const blocRef& block, const std::string& loc_info_default);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * @brief check if the max transformation limit has been reached
    * @param stmt is the current statement
    * @return true in case all the next transformations have to be skipped
    */
   bool reached_max_transformation_limit(const ir_nodeRef& stmt);

   void decomposeDivisionByConstant(const std::pair<unsigned int, blocRef>& block,
                                    std::list<ir_nodeRef>::const_iterator& it_los, assign_stmt* ga,
                                    const ir_nodeRef& op1, enum kind code1, bool& restart_analysis,
                                    const std::string& loc_info_default, const std::string& step_name);

   bool handleTrivialDivByConstant(assign_stmt* ga, const ir_nodeRef& op0, const ir_nodeRef& typeExpr, long long extOp1,
                                   bool unsignedp, bool remFlag, const std::string& locInfoDefault,
                                   bool& restartAnalysis);

   void assignRemainderFromQuotient(const std::pair<unsigned int, blocRef>& block,
                                    std::list<ir_nodeRef>::const_iterator& itLos, assign_stmt* ga,
                                    const ir_nodeRef& op0, const ir_nodeRef& op1, const ir_nodeRef& typeExpr,
                                    const ir_nodeRef& quotientExpr, const std::string& locInfoDefault,
                                    const std::string& stepName, bool restrictWidth, unsigned int dataBitsize);

   void lowerUnsignedTruncDivModByConstant(const std::pair<unsigned int, blocRef>& block,
                                           std::list<ir_nodeRef>::const_iterator& itLos, assign_stmt* ga,
                                           const ir_nodeRef& op0, const ir_nodeRef& op1, const ir_nodeRef& typeExpr,
                                           long long extOp1, bool remFlag, const std::string& locInfoDefault,
                                           const std::string& stepName, bool& restartAnalysis);

   void lowerSignedTruncDivModByConstant(const std::pair<unsigned int, blocRef>& block,
                                         std::list<ir_nodeRef>::const_iterator& itLos, assign_stmt* ga,
                                         const ir_nodeRef& op0, const ir_nodeRef& op1, const ir_nodeRef& typeExpr,
                                         long long extOp1, bool remFlag, const std::string& locInfoDefault,
                                         const std::string& stepName, bool& restartAnalysis);

   void normalizePhiNodes(const statement_list_node* stmtList);

   std::string buildLocInfoDefault(const ir_nodeRef& stmt) const;

   ir_nodeRef createAndInsertAssign(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos,
                                    const ir_nodeRef& type, const ir_nodeRef& min, const ir_nodeRef& max,
                                    const ir_nodeRef& expr, const std::string& locInfoDefault, bool setTempAddress,
                                    bool& restartAnalysis);

   void extractExpr(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos, ir_nodeRef& op,
                    bool setTempAddress, const std::string& locInfoDefault, bool& restartAnalysis);

   void extractUnaryExpr(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos, ir_nodeRef& op,
                         bool duplicate, bool setTempAddress, const std::string& locInfoDefault, bool& restartAnalysis);

   void typeCastExpr(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos, ir_nodeRef& op,
                     const ir_nodeRef& type, const std::string& locInfoDefault, bool& restartAnalysis);

   void normalizeOperands(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos, assign_stmt* ga,
                          enum kind code0, enum kind code1, const std::string& locInfoDefault, bool& restartAnalysis);

   bool handleMemRef(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos, assign_stmt* ga,
                     enum kind code0, enum kind code1, const std::string& locInfoDefault, bool& restartAnalysis);

   bool lowerArithExpr(const std::pair<unsigned int, blocRef>& block, std::list<ir_nodeRef>::const_iterator& itLos,
                       assign_stmt* ga, enum kind code1, const std::string& locInfoDefault, bool& restartAnalysis);

   bool handleMemRefOutput(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos, assign_stmt* ga,
                           enum kind code0, const std::string& locInfoDefault, bool& restartAnalysis);

 public:
   IR_lowering(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id,
               const DesignFlowManager& design_flow_manager);

   void getAddSubMultCosts(const HLS_deviceRef& HLS_D, unsigned data_bitsize, double& add_delay, double& sub_delay,
                           double& mult_delay, double& shift_delay, double& add_area, double& mult_area,
                           double& mult_lut_area, double& mult_dsp_count, double& shift_area, bool require_fus) const;
   bool getLutCost(const HLS_deviceRef& HLS_D, unsigned in_bits, unsigned max_lut_size, double& lut_delay,
                   double& lut_area) const;

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;

   void ComputeRelationships(DesignFlowStepSet& relationship,
                             const DesignFlowStep::RelationshipType relationship_type) override;
};
#endif
