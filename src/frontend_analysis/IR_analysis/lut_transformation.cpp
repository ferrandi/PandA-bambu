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
 *              Copyright (c) 2004-2018 Politecnico di Milano
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
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_reindex.hpp"

std::string lut_transformation::DecToBin(unsigned long long int number)
{
   if(number == 0)
      return "0";
   if(number == 1)
      return "1";
   if(number % 2 == 0)
   {
      return DecToBin(number / 2) + "0";
   }
   else
   {
      return DecToBin(number / 2) + "1";
   }
}

unsigned long long int lut_transformation::BinToDec(const std::string& number)
{
   unsigned long long int i_bin = 0;
   try
   {
      size_t* endptr = nullptr;
      i_bin = std::stoull(number, endptr, 2);
   }
   catch(const std::invalid_argument& e)
   {
      std::string reasonWhy = e.what();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Invalid Argument" + reasonWhy);
   }
   catch(const std::out_of_range& e)
   {
      std::string reasonWhy = e.what();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Out Of Range" + reasonWhy);
   }
   return i_bin;
}

/**
 * Create a new concatenation
 * @param op0 first input
 * @param op1 second input
 * @param op0size size for shifting
 * @param bb_index index of bb where to append the concatenation
 */
tree_nodeRef lut_transformation::CreateConcat(tree_nodeRef op0, tree_nodeRef op1, std::pair<const unsigned int, blocRef> bb, tree_nodeRef stm_to_append)
{
   const auto type = tree_man->CreateDefaultUnsignedLongLongInt();
   unsigned int integer_cst1_id = TM->new_tree_node_id();
   tree_nodeRef const1 = tree_man->CreateIntegerCst(type, 1, integer_cst1_id);

   const std::string srcp_default("built-in:0:0");
   tree_nodeRef lshift_op = tree_man->create_binary_operation(type, op0, const1, srcp_default, lshift_expr_K);
   tree_nodeRef lshift_ga = CreateGimpleAssign(type, lshift_op, bb.first, srcp_default);
   bb.second->PushBefore(lshift_ga, stm_to_append);

   tree_nodeRef conc_op = tree_man->create_ternary_operation(type, GetPointer<const gimple_assign>(GET_CONST_NODE(lshift_ga))->op0, op1, const1, srcp_default, bit_ior_concat_expr_K);
   tree_nodeRef conc_ga = CreateGimpleAssign(type, conc_op, bb.first, srcp_default);
   bb.second->PushBefore(conc_ga, stm_to_append);
   return conc_ga;
}

/**
 * Create a multiconcatenation from a set of operand
 * @param set_of_nodes set of nodes to concatenate
 * @param bb_index index of bb where to append the concatenation
 */
tree_nodeRef lut_transformation::CreateMultiConcat(std::vector<tree_nodeRef> set_of_nodes, const std::pair<const unsigned int, blocRef>& bb, tree_nodeRef stm_to_append)
{
   // Create the first concat
   auto it = set_of_nodes.begin();

   const std::string srcp_default("built-in:0:0");
   const auto type = tree_man->CreateDefaultUnsignedLongLongInt();
   unsigned int integer_cst1_id = TM->new_tree_node_id();
   tree_nodeRef const1 = tree_man->CreateIntegerCst(type, 1, integer_cst1_id);
   tree_nodeRef mask_op = tree_man->create_binary_operation(type, *it, const1, srcp_default, bit_and_expr_K);
   tree_nodeRef mask_ga = CreateGimpleAssign(type, mask_op, bb.first, srcp_default);
   bb.second->PushBefore(mask_ga, stm_to_append);
   tree_nodeRef maskedOp = GetPointer<const gimple_assign>(GET_CONST_NODE((mask_ga)))->op0;
   tree_nodeRef concat = CreateConcat(maskedOp, *(it + 1), bb, stm_to_append);
   it = it + 2;
   while(it != set_of_nodes.end())
   {
      tree_nodeRef concat_ssa = (GetPointer<gimple_assign>(GET_NODE(concat)))->op0;
      concat = CreateConcat(concat_ssa, *(it), bb, stm_to_append);
      it = it + 1;
   }
   return concat;
}

/**
 * This function takes a binary number (as a string) a set of indexes and create a new int value
 * e.g binString = "0001" indexes = [0,3] return int(01) = 1
 * @param binString string of bit
 * @param indexesSet vector of indexes
 */
unsigned long long int lut_transformation::GenerateIndexOfLutValue(const std::string& binString, const std::vector<std::size_t>& indexesSet)
{
   std::string result("");
   for(unsigned long it : indexesSet)
   {
      result += (binString.substr(it, 1));
   }
   return BinToDec(result);
}

/**
 * This function takes a binary number (as a string) a set of indexes and create a new int value
 * e.g binString = "0001" indexes = [0,3] return int(01) = 1
 * @param binString string of bit
 * @param indexesSet vector of indexes
 */
std::vector<tree_nodeRef> lut_transformation::CreateSetFromVector(std::vector<tree_nodeRef> firstSet, std::vector<tree_nodeRef> secondSet)
{
   std::vector<tree_nodeRef> result;
   result.insert(result.begin(), firstSet.begin(), firstSet.end());
   for(auto& it : secondSet)
   {
      bool found = false;
      for(auto& it1 : firstSet)
      {
         if(it == it1)
         {
            found = true;
            break;
         }
      }
      if(!found)
      {
         result.push_back(it);
      }
   }
   return result;
}

/**
 * Add zeroes to reach the string size
 * @param binString string of bit
 * @param sizeOfTheSet num of bit required
 */
std::string lut_transformation::AddZeroes(const std::string& _bitString, double sizeOfTheSet)
{
   std::string bitString = _bitString;
   while(bitString.size() < static_cast<std::size_t>(sizeOfTheSet))
   {
      bitString = "0" + bitString;
   }
   return bitString;
}

tree_nodeRef lut_transformation::CreateGimpleAssign(const tree_nodeRef type, const tree_nodeRef op, const unsigned int bb_index, const std::string& srcp_default)
{
   tree_nodeRef ssa_vd = tree_man->create_ssa_name(tree_nodeRef(), type);
   auto ret_value = tree_man->create_gimple_modify_stmt(ssa_vd, op, srcp_default, bb_index);
   return ret_value;
}

/**
 * Recursive function to get the first level of inputs of a node
 * The function returns a set of lut or primary inputs.
 * If the function finds a concat/shif/mask it just goes on searching for lut or primary inputs
 * @param noderef node to expand
 */
std::vector<tree_nodeRef> lut_transformation::GetInputs(const tree_nodeRef nodeReindex)
{
   std::vector<tree_nodeRef> allInputs;
   tree_nodeRef noderef = GET_NODE(nodeReindex);
   if(noderef->get_kind() == gimple_assign_K)
   {
      auto* gimpleNode = GetPointer<gimple_assign>(noderef);
      tree_nodeRef node = GET_NODE(gimpleNode->op1);
      enum kind kindOfNode = node->get_kind();
      if(kindOfNode == lut_expr_K)
      {
         // if it is a lut i just add it as input
         allInputs.push_back(gimpleNode->op0);
      }
      else if(kindOfNode == bit_ior_concat_expr_K)
      {
         // If it's a concat i have to call the function on the two operand
         auto* concat = GetPointer<bit_ior_concat_expr>(node);
         std::vector<tree_nodeRef> inputsOp0 = GetInputs(concat->op0);
         std::vector<tree_nodeRef> inputsOp1 = GetInputs(concat->op1);
         allInputs.insert(allInputs.end(), inputsOp0.begin(), inputsOp0.end());
         allInputs.insert(allInputs.end(), inputsOp1.begin(), inputsOp1.end());
      }
      else if(kindOfNode == lshift_expr_K)
      {
         auto* lshift = GetPointer<lshift_expr>(node);
         if(GET_NODE(lshift->op1)->get_kind() == integer_cst_K)
         {
            std::vector<tree_nodeRef> inputsOp0 = GetInputs(lshift->op0);
            allInputs.insert(allInputs.end(), inputsOp0.begin(), inputsOp0.end());
         }
         else
         {
            allInputs.push_back(gimpleNode->op0);
         }
      }
      else if(kindOfNode == bit_and_expr_K)
      {
         auto* mask = GetPointer<bit_and_expr>(node);
         if(GET_NODE(mask->op1)->get_kind() == integer_cst_K)
         {
            std::vector<tree_nodeRef> inputsOp0 = GetInputs(mask->op0);
            allInputs.insert(allInputs.end(), inputsOp0.begin(), inputsOp0.end());
         }
         else
         {
            allInputs.push_back(gimpleNode->op0);
         }
      }
      else
      {
         allInputs.push_back(gimpleNode->op0);
      }
   }
   else if(noderef->get_kind() == ssa_name_K)
   {
      tree_nodeRef temp_reindex = (GetPointer<const ssa_name>(noderef))->CGetDefStmt();
      tree_nodeRef temp_def0 = GET_NODE(temp_reindex);
      if(temp_def0->get_kind() == gimple_assign_K)
      {
         std::vector<tree_nodeRef> inputsOp0 = GetInputs(temp_reindex);
         allInputs.insert(allInputs.end(), inputsOp0.begin(), inputsOp0.end());
      }
      else
      {
         allInputs.push_back(nodeReindex);
      }
   }
   else
   {
      allInputs.push_back(nodeReindex);
   }
   return allInputs;
}

/**
 * The function returns  mergedSet.indexOf(values[i]) for every values in value
 * @param mergedSet  set of merged node
 * @param values nodes to look for
 */
std::vector<std::size_t> lut_transformation::CreateLutIndexSet(std::vector<tree_nodeRef> mergedSet, std::vector<tree_nodeRef> values)
{
   std::vector<std::size_t> result;
   for(auto& value : values)
   {
      result.push_back(static_cast<std::size_t>(std::distance(mergedSet.begin(), std::find(mergedSet.begin(), mergedSet.end(), value))));
   }
   return result;
}

/**
 * This function find the index of a node in a vector
 * @param mergedSet vector to search in
 * @param node node to search
 */
std::vector<std::size_t> lut_transformation::FindIndex(std::vector<tree_nodeRef> mergedSet, tree_nodeRef node)
{
   std::vector<std::size_t> result;
   for(auto it = mergedSet.begin(); it != mergedSet.end(); ++it)
   {
      std::size_t position = static_cast<std::size_t>(std::distance(mergedSet.begin(), it));
      if(node == *it)
      {
         result.push_back(position);
      }
   }
   return result;
}

/**
 * This function create the final string to use to find the final value in the current lut table
 * It basically concatenates the inputs following the right order
 * @param binaryString binary string where to find the inputs
 * @param unmergedSet original set not merged
 * @param mergedSet merged set
 * @param mergingValue value of the lut table we are mergin with the input combination of binaryString
 */
std::string lut_transformation::CreateFinalString(const std::string& binaryString, const std::vector<tree_nodeRef>& unmergedSet, std::vector<tree_nodeRef>& mergedSet, const std::string& mergingValue)
{
   std::string finalString;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Computing value of final LUT");
   for(const auto& it : unmergedSet)
   {
      std::vector<std::size_t> indexList = FindIndex(mergedSet, it);
      if(indexList.size() != 0)
      {
         finalString += binaryString.substr(indexList.at(0), 1);
      }
      else
      {
         finalString += mergingValue;
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--" + finalString);
   return finalString;
}

/**
 * This function can be used to find all the LUT in a list of statements
 * @param list_of_stmt list of statement where to search the lut
 */
std::vector<tree_nodeRef> lut_transformation::GetLutList(const std::vector<tree_nodeRef> list_of_stmt)
{
   std::vector<tree_nodeRef> lutList;
   auto it_los_end = list_of_stmt.end();
   auto it_los = list_of_stmt.begin();
   // iteration over statements
   while(it_los != it_los_end)
   {
      if(GET_NODE(*it_los)->get_kind() == gimple_assign_K)
      {
         auto* ga = GetPointer<gimple_assign>(GET_NODE(*it_los));
         if(ga->op1->get_kind() == lut_expr_K)
         {
            lutList.push_back(*it_los);
         }
      }
      it_los++;
   }
   return lutList;
}

/**
 * Main function that takes a list of lut and call the algorithm to merge it
 * @param gimpleLutList list of gimple where the right part is a lut_expr
 * @param bb_index index of the bb to add the lut
 */
void lut_transformation::MergeLut(const std::list<tree_nodeRef>& gimpleLutList, const std::pair<const unsigned int, blocRef>& bb)
{
   for(const auto& stmt : gimpleLutList)
   {
      auto* consideredLutGa = GetPointer<gimple_assign>(GET_NODE(stmt));
      THROW_ASSERT(consideredLutGa, STR(stmt));
      auto* consideredLut = GetPointer<lut_expr>(GET_NODE(consideredLutGa->op1));
      THROW_ASSERT(consideredLut, STR(consideredLutGa->op1));
      std::vector<tree_nodeRef> expansionSet = GetInputs(consideredLut->op0);
      auto nodeToExpand = expansionSet.begin();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Starting analysis of: " + STR(stmt));
#ifndef NDEBUG
      for(auto& i3 : expansionSet)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Initial LUT support : " + STR(i3));
      }
#endif

      while(nodeToExpand != expansionSet.end())
      {
#ifndef NDEBUG
         if(not AppM->ApplyNewTransformation())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reached max cfg transformations");
            break;
         }
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering node " + STR(*nodeToExpand));
         std::vector<tree_nodeRef> inputsOfToMerge;
         lut_expr* lutToExpand = nullptr;
         // Get the lut to expand and its inputs
         THROW_ASSERT(*nodeToExpand, "");
         if((GET_NODE(*nodeToExpand))->get_kind() == ssa_name_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering input " + STR(*nodeToExpand));
            tree_nodeRef temp_def0 = GET_NODE((GetPointer<const ssa_name>(GET_NODE(*nodeToExpand)))->CGetDefStmt());
            if(temp_def0->get_kind() == gimple_assign_K)
            {
               auto* gaToExpand = GetPointer<gimple_assign>(temp_def0);
               if(GET_NODE(gaToExpand->op1)->get_kind() == lut_expr_K)
               {
                  lutToExpand = GetPointer<lut_expr>(GET_NODE(gaToExpand->op1));
                  inputsOfToMerge = GetInputs(lutToExpand->op0);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Merging with : " + STR(temp_def0));
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered input " + STR(*nodeToExpand));
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipping " + STR(*nodeToExpand));
         }
         // If there are no more input, we stop the merge
         // And go on with the next expansion
         if(inputsOfToMerge.size() == 0)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipping becasue no more inputs");
            ++nodeToExpand;
            continue;
         }
#ifndef NDEBUG
         for(auto& i3 : inputsOfToMerge)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Merging LUT support : " + STR(i3));
         }
#endif
         // Create a "merged set" using the input of the considered lut + the input of the mergin one
         // And remove the merging itself
         std::vector<tree_nodeRef> unmergedSet;
         unmergedSet.insert(unmergedSet.begin(), expansionSet.begin(), expansionSet.end());
         auto temp_expansionSet = expansionSet;
         /// We remove nodeToExpand from expansionSet to check the number of actual inputs;
         /// if the number is ok, then we actually remove, otherwise we keep
         temp_expansionSet.erase(std::find(temp_expansionSet.begin(), temp_expansionSet.end(), *nodeToExpand));
         THROW_ASSERT(expansionSet.size() != temp_expansionSet.size(), "");
         std::vector<tree_nodeRef> mergedSet = CreateSetFromVector(temp_expansionSet, inputsOfToMerge);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Size of support set is " + STR(mergedSet.size()));
         if(mergedSet.size() > max_lut_size)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipping becasue too many inputs");
            ++nodeToExpand;
            continue;
         }
         expansionSet = temp_expansionSet;
         // Now create the index sets
         std::vector<std::size_t> firstOpIndexes = CreateLutIndexSet(mergedSet, inputsOfToMerge);

         // Get the lut values
         auto* considered_lut_cost = GetPointer<integer_cst>(GET_NODE(consideredLut->op1));
         THROW_ASSERT(considered_lut_cost, STR(consideredLut->op1));
         auto lutNumber = static_cast<long long unsigned int>(tree_helper::get_integer_cst_value(considered_lut_cost));

         std::string currentLutValueInBit = AddZeroes(DecToBin(lutNumber), pow(2, static_cast<double>(unmergedSet.size())));
         std::reverse(currentLutValueInBit.begin(), currentLutValueInBit.end());

         std::string firstOpValueInBit;

         auto* expand_lut_cost = GetPointer<integer_cst>(GET_NODE(lutToExpand->op1));
         THROW_ASSERT(expand_lut_cost, STR(lutToExpand->op1));
         auto expandlutNumber = static_cast<long long unsigned int>(tree_helper::get_integer_cst_value(expand_lut_cost));
         firstOpValueInBit = AddZeroes(DecToBin(expandlutNumber), pow(2, static_cast<double>(inputsOfToMerge.size())));
         std::reverse(firstOpValueInBit.begin(), firstOpValueInBit.end());

         // Build the new lut
         std::string newLutValue("");
         for(std::size_t i = 0; i < static_cast<std::size_t>(pow(2, static_cast<double>(mergedSet.size()))); i++)
         {
            std::string binaryNumber = AddZeroes(DecToBin(i), static_cast<double>(mergedSet.size()));
            auto indexForFirst = GenerateIndexOfLutValue(binaryNumber, firstOpIndexes);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Index in first LUT is " + STR(indexForFirst));
            std::string firstTempValue = firstOpValueInBit.substr(static_cast<size_t>(indexForFirst), 1);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Outcome of first LUT is " + firstOpValueInBit + "[" + STR(indexForFirst) + "] = " + STR(firstTempValue));
            std::string finalString = CreateFinalString(binaryNumber, unmergedSet, mergedSet, firstTempValue);
            auto indexForCurrent = BinToDec(finalString);
            newLutValue += currentLutValueInBit.substr(static_cast<size_t>(indexForCurrent), 1);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current LUT value " + newLutValue);
         }
         // create new int lut number
         std::reverse(newLutValue.begin(), newLutValue.end());
         auto newLutNumber = BinToDec(newLutValue);
         // create new concat
#ifndef NDEBUG
         for(auto& i3 : mergedSet)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Merged LUT support : " + STR(i3));
         }
#endif
         // create new lut and push it
         tree_nodeRef multiConcat = CreateMultiConcat(mergedSet, bb, GET_NODE(stmt));
         unsigned int lut_id = TM->new_tree_node_id();
         const auto type = tree_man->CreateDefaultUnsignedLongLongInt();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LUT constant is " + STR(newLutNumber));
         tree_nodeRef lut_constant = tree_man->CreateIntegerCst(type, static_cast<long long int>(newLutNumber), lut_id);
         TM->ReplaceTreeNode(stmt, consideredLut->op1, lut_constant);
         tree_nodeRef concat_ssa = (GetPointer<gimple_assign>(GET_NODE(multiConcat)))->op0;
         GetPointer<ssa_name>(GET_NODE(concat_ssa))->bit_values = std::string(mergedSet.size(), 'U');
         TM->ReplaceTreeNode(stmt, consideredLut->op0, concat_ssa);
#ifndef NDEBUG
         AppM->RegisterTransformation(GetName(), stmt);
#endif

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--New stmt is " + STR(stmt));
         expansionSet = mergedSet;
         // restart expansion set from start
         nodeToExpand = expansionSet.begin();
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended analysis");
   }
}

lut_transformation::lut_transformation(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, LUT_TRANSFORMATION, _design_flow_manager, Param), max_lut_size(NUM_CST_allocation_default_max_lut_size)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> lut_transformation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(BIT_VALUE_OPT, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
            relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(MULTI_WAY_IF, SAME_FUNCTION));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

lut_transformation::~lut_transformation() = default;

DesignFlowStep_Status lut_transformation::InternalExec()
{
   bool modified = false;
   tree_nodeRef temp = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(temp);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it has not a body");
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement list");
   /// iteration over basic blocks
   for(auto block : sl->list_of_bloc)
   {
      std::list<tree_nodeRef> lutList;

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
      const auto& list_of_stmt = block.second->CGetStmtList();
      /// end of statements list
      auto it_los_end = list_of_stmt.end();
      /// size of statements list
      size_t n_stmts = list_of_stmt.size();
      /// start of statements list
      auto it_los = list_of_stmt.begin();
      /// iteration over statements
      while(it_los != it_los_end)
      {
#ifndef NDEBUG
         if(not AppM->ApplyNewTransformation())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reached max cfg transformations");
            it_los++;
            continue;
         }
#endif
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(*it_los)->ToString());
         if(GET_NODE(*it_los)->get_kind() == gimple_assign_K)
         {
            auto* ga = GetPointer<gimple_assign>(GET_NODE(*it_los));
            const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
            // second operand : the right part of the assignment
            enum kind code1 = GET_NODE(ga->op1)->get_kind();
            long long int lut_number = 0;

            if(code1 == bit_and_expr_K || code1 == truth_and_expr_K)
            {
               lut_number = 8;
            }
            else if(code1 == bit_ior_expr_K || code1 == truth_or_expr_K)
            {
               lut_number = 14;
            }
            else if(code1 == bit_xor_expr_K || code1 == truth_xor_expr_K)
            {
               lut_number = 6;
            }

            if(lut_number != 0)
            {
               auto* be = GetPointer<binary_expr>(GET_NODE(ga->op1));
               THROW_ASSERT(be->op0 && be->op1, "expected two parameters");
               int data_size0 = static_cast<int>(tree_helper::Size(GET_NODE(be->op0)));
               int data_size1 = static_cast<int>(tree_helper::Size(GET_NODE(be->op1)));
               if(data_size0 == 1 and data_size1 == 1 and not tree_helper::is_constant(TM, be->op0->index) and not tree_helper::is_constant(TM, be->op1->index))
               {
                  const auto type = tree_man->CreateDefaultUnsignedLongLongInt();

                  tree_nodeRef convert_op = tree_man->create_unary_operation(type, be->op0, srcp_default, convert_expr_K);
                  tree_nodeRef convert_ga = CreateGimpleAssign(type, convert_op, block.first, srcp_default);
                  block.second->PushBefore(convert_ga, *it_los);

                  unsigned int integer_cst1_id = TM->new_tree_node_id();
                  tree_nodeRef const1 = tree_man->CreateIntegerCst(type, 1, integer_cst1_id);

                  tree_nodeRef mask_op = tree_man->create_binary_operation(type, GetPointer<const gimple_assign>(GET_CONST_NODE(convert_ga))->op0, const1, srcp_default, bit_and_expr_K);
                  tree_nodeRef mask_ga = CreateGimpleAssign(type, mask_op, block.first, srcp_default);
                  block.second->PushBefore(mask_ga, *it_los);

                  tree_nodeRef lshift_op = tree_man->create_binary_operation(type, GetPointer<const gimple_assign>(GET_CONST_NODE((mask_ga)))->op0, const1, srcp_default, lshift_expr_K);
                  tree_nodeRef lshift_ga = CreateGimpleAssign(type, lshift_op, block.first, srcp_default);
                  block.second->PushBefore(lshift_ga, *it_los);

                  tree_nodeRef conc_op = tree_man->create_ternary_operation(type, GetPointer<const gimple_assign>(GET_CONST_NODE(lshift_ga))->op0, be->op1, const1, srcp_default, bit_ior_concat_expr_K);
                  tree_nodeRef conc_ga = CreateGimpleAssign(type, conc_op, block.first, srcp_default);
                  GetPointer<gimple_assign>(GET_NODE(conc_ga))->temporary_address = true;
                  tree_nodeRef result_conc = GetPointer<gimple_assign>(GET_NODE(conc_ga))->op0;
                  GetPointer<ssa_name>(GET_NODE(result_conc))->bit_values = "UU";
                  // insert concatenation node
                  block.second->PushBefore(conc_ga, *it_los);
                  // create lut node
                  // transform constant to tree_nodeRef
                  unsigned int integer_cst2_id = TM->new_tree_node_id();
                  tree_nodeRef and_op_cst = tree_man->CreateIntegerCst(type, lut_number, integer_cst2_id);
                  const auto type1 = tree_man->CreateDefaultUnsignedLongLongInt();
                  tree_nodeRef new_op1 = tree_man->create_binary_operation(be->type, result_conc, and_op_cst, srcp_default, lut_expr_K);
                  tree_nodeRef lut_ga = CreateGimpleAssign(be->type, new_op1, block.first, srcp_default);
                  GetPointer<gimple_assign>(GET_NODE(lut_ga))->op0 = ga->op0;
                  // insert lut node
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replacing " + STR(*it_los) + " with " + STR(lut_ga));
                  block.second->Replace(*it_los, lut_ga, true);
#ifndef NDEBUG
                  AppM->RegisterTransformation(GetName(), lut_ga);
#endif
                  lutList.push_back(lut_ga);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added to LUT list : " + STR(lut_ga));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Modified statement " + GET_NODE(lut_ga)->ToString());
                  it_los = list_of_stmt.begin();
                  it_los_end = list_of_stmt.end();
                  continue;
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(*it_los)->ToString());
         it_los++;
      }
      // Merge all the LUT in the block
      MergeLut(lutList, block);

      if(n_stmts != list_of_stmt.size())
         modified = true;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examining BB" + STR(block.first));
   }
   if(modified)
      function_behavior->UpdateBBVersion();
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void lut_transformation::Initialize()
{
   TM = AppM->get_tree_manager();
   tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
   THROW_ASSERT(GetPointer<const HLS_manager>(AppM)->get_HLS_target(), "");
   const auto hls_target = GetPointer<const HLS_manager>(AppM)->get_HLS_target();
   THROW_ASSERT(hls_target->get_target_device()->has_parameter("max_lut_size"), "");
   max_lut_size = hls_target->get_target_device()->get_parameter<size_t>("max_lut_size");
}

void lut_transformation::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto* technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const DesignFlowStepRef technology_design_flow_step =
             technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   FunctionFrontendFlowStep::ComputeRelationships(relationship, relationship_type);
}
