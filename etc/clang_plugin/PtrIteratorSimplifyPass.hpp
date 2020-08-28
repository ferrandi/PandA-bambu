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
 *              Copyright (C) 2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
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

/*
 * Pointer iterator simplification pass
 *
 * @author Marco Siracusa
 *
 */

#ifndef SCALAR_REPLACEMENT_OF_AGGREGATES_PTRITERATORSIMPLIFICATIONPASS_HPP
#define SCALAR_REPLACEMENT_OF_AGGREGATES_PTRITERATORSIMPLIFICATIONPASS_HPP

#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

unsigned int total_expanded_phis = 0;
class PtrIteratorSimplifyPass : public llvm::LoopPass
{
   std::string get_val_string(llvm::Value* value)
   {
      std::string str;
      llvm::raw_string_ostream rso(str);
      value->print(rso);
      return rso.str();
   }

   std::string get_ty_string(llvm::Type* ty)
   {
      std::string str;
      llvm::raw_string_ostream rso(str);
      ty->print(rso);
      return rso.str();
   }

   bool get_reachables_and_externals(llvm::Use* use_rec, const std::set<llvm::BasicBlock*>& blocks, std::set<llvm::Use*>& reachables, std::set<llvm::Use*>& externals)
   {
      /// Process only if instruction not already in the reachables set
      if(reachables.insert(use_rec).second)
      {
         /// Go through the gepis and seach the phi operands for externals

         /// ASSUMPTION: GepOPs already converted in gepis beforehand
         if(llvm::GetElementPtrInst* user_as_gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(use_rec->getUser()))
         {
            /// Since LCSSA, we can go out the loop only through a phi node
            if(blocks.count(user_as_gepi->getParent()) == 0)
            {
               return false;
            }

            if(use_rec->getOperandNo() == user_as_gepi->getPointerOperandIndex())
            {
               if(user_as_gepi->getNumIndices() == 1)
               {
                  /// No big deal, just keep iterating
                  for(llvm::Use& gepi_use : user_as_gepi->uses())
                  {
                     get_reachables_and_externals(&gepi_use, blocks, reachables, externals);
                  }
               }
            }
         }
         else if(llvm::PHINode* user_as_phi = llvm::dyn_cast<llvm::PHINode>(use_rec->getUser()))
         {
            /// Go through the Operands and check if some external
            for(llvm::Use& phi_operand : user_as_phi->operands())
            {
               if(llvm::Instruction* operand_as_inst = llvm::dyn_cast<llvm::Instruction>(phi_operand.get()))
               {
                  if(blocks.count(operand_as_inst->getParent()) == 0)
                  {
                     externals.insert(&phi_operand);
                  }

                  if(!llvm::isa<llvm::PHINode>(operand_as_inst) and !llvm::isa<llvm::GEPOperator>(operand_as_inst))
                  {
                     return false;
                  }
               }
               else
               {
                  externals.insert(&phi_operand);

                  if(!llvm::isa<llvm::Argument>(phi_operand.get()) and !llvm::isa<llvm::GlobalValue>(phi_operand.get()) and !llvm::isa<llvm::PHINode>(phi_operand.get()))
                  {
                     return false;
                  }
               }
            }

            /// Keep iterating only if inside the loop
            /// This let us take care of LCSSA phis outside the loop
            if(blocks.count(user_as_phi->getParent()) > 0)
            {
               for(llvm::Use& phi_use : user_as_phi->uses())
               {
                  get_reachables_and_externals(&phi_use, blocks, reachables, externals);
               }
            }
         }
         else if(llvm::CmpInst* user_as_cmp = llvm::dyn_cast<llvm::CmpInst>(use_rec->getUser()))
         {
            /// Go through the Operands and check if some external
            for(llvm::Use& phi_operand : user_as_cmp->operands())
            {
               if(llvm::Instruction* operand_as_inst = llvm::dyn_cast<llvm::Instruction>(phi_operand.get()))
               {
                  if(blocks.count(operand_as_inst->getParent()) == 0)
                  {
                     externals.insert(&phi_operand);
                  }

                  if(!llvm::isa<llvm::PHINode>(operand_as_inst) and !llvm::isa<llvm::GEPOperator>(operand_as_inst))
                  {
                     return false;
                  }
               }
               else
               {
                  externals.insert(&phi_operand);

                  if(!llvm::isa<llvm::Argument>(phi_operand.get()) and !llvm::isa<llvm::GlobalValue>(phi_operand.get()) and !llvm::isa<llvm::PHINode>(phi_operand.get()))
                  {
                     return false;
                  }
               }
            }
         }
         else
         {
            /// Don't care, it's gonna be replaced by the transformation
         }
      }

      return true;
   }

   llvm::Value* operate_on_index(llvm::BinaryOperator::BinaryOps bin_op, llvm::Value* lhs_idx, llvm::Value* rhs_idx, llvm::Instruction* insert_point, llvm::Type* idx_ty)
   {
      if(llvm::ConstantInt* constant_lhs = llvm::dyn_cast<llvm::ConstantInt>(lhs_idx))
      {
         if(llvm::ConstantInt* constant_rhs = llvm::dyn_cast<llvm::ConstantInt>(rhs_idx))
         {
            unsigned long value = constant_lhs->getSExtValue() + constant_rhs->getSExtValue();
            return llvm::ConstantInt::get(idx_ty, value);
         }
         else
         {
            if(constant_lhs->getSExtValue() == 0)
            {
               return rhs_idx;
            }
            else
            {
               llvm::BinaryOperator* add_op = llvm::BinaryOperator::Create(bin_op, lhs_idx, rhs_idx, "offset", insert_point);
               return add_op;
            }
         }
      }
      else
      {
         if(llvm::ConstantInt* constant_rhs = llvm::dyn_cast<llvm::ConstantInt>(rhs_idx))
         {
            if(constant_rhs->getSExtValue() == 0)
            {
               return lhs_idx;
            }
            else
            {
               llvm::BinaryOperator* add_op = llvm::BinaryOperator::Create(bin_op, lhs_idx, rhs_idx, "offset", insert_point);
               return add_op;
            }
         }
         else
         {
            llvm::BinaryOperator* add_op = llvm::BinaryOperator::Create(llvm::BinaryOperator::BinaryOps::Add, rhs_idx, lhs_idx, "offset", insert_point);
            return add_op;
         }
      }

      return nullptr;
   }

 public:
   static char ID; // Pass ID, replacement for typeid
   PtrIteratorSimplifyPass() : LoopPass(ID)
   {
   }

   bool runOnLoop(llvm::Loop* L, llvm::LPPassManager&) override
   {
      if(skipLoop(L))
         return false;

      llvm::Type* idx_ty = llvm::Type::getInt32Ty(L->getHeader()->getContext());

      unsigned long expanded_phis = 0;
      std::set<llvm::BasicBlock*> blocks(L->block_begin(), L->block_end());

      /// Go through all the header instruction looking for candidate phi nodes (type is pointer)
      std::vector<llvm::PHINode*> phi_candidates;

      for(llvm::Instruction& i : *L->getHeader())
      {
         if(llvm::PHINode* phi_node = llvm::dyn_cast<llvm::PHINode>(&i))
         {
            if(phi_node->getType()->isPointerTy())
            {
               /// phi_candidates.push_back(phi_node);

               bool has_arg_operand = false;
               for(auto& op : phi_node->incoming_values())
               {
                  if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(&op))
                  {
                     for(llvm::User* user : arg->getParent()->users())
                     {
                        if(llvm::CallInst* call_inst = llvm::dyn_cast<llvm::CallInst>(user))
                        {
                           llvm::Value* op = call_inst->getOperandUse(arg->getArgNo());
                           if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(op))
                           {
                              if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(gep_op->getPointerOperand()))
                              {
                                 if(arg->getType()->getPointerElementType()->isIntegerTy(8))
                                 {
                                    has_arg_operand = true;
                                 }
                              }
                           }
                        }
                     }
                  }
               }

               if(!has_arg_operand)
               {
                  phi_candidates.push_back(phi_node);
               }
            }
         }
      }

      /// For each phi candidate, compute the reachable instructions (gepis and phis)
      /// For each of those instructions, track the operands coming from outside the loop
      for(llvm::PHINode* candidate_phi : phi_candidates)
      {
         std::set<llvm::Use*> reachable_uses;
         std::set<llvm::Use*> external_uses;
         bool feasible = true;

         for(llvm::Use& phi_candidate_use : candidate_phi->uses())
         {
            feasible = get_reachables_and_externals(&phi_candidate_use, blocks, reachable_uses, external_uses) and feasible;
         }

         /// Feasible only if phi operands are other phis, gepis, args, or globals
         if(feasible)
         {
            /// Try to reconduct all of the external addresses to a single base address (and relative offsets)

            /// Store a vector of set of pointers
            /// Each pointer will add the base address to the set if it's a gepi, nothing otherwise
            std::vector<std::pair<llvm::Value*, std::set<llvm::Value*>>> vec_ptr_addr_set;

            for(llvm::Use* external_use : external_uses)
            {
               std::pair<llvm::Value*, std::set<llvm::Value*>> vec_pair = std::make_pair(external_use->get(), std::set<llvm::Value*>());
               vec_ptr_addr_set.push_back(vec_pair);
               vec_ptr_addr_set.back().second.insert(external_use->get());
            }

            llvm::Value* common_external = nullptr; /// The common external to be checked (stays null if not found)
            bool iterate = true;
            while(iterate and !vec_ptr_addr_set.empty())
            {
               iterate = true;

               unsigned long gepi_updates = 0;

               /// Try intersecting all of the sets and update the gepi pointers
               std::set<llvm::Value*> intersection = vec_ptr_addr_set.front().second;
               for(std::pair<llvm::Value*, std::set<llvm::Value*>>& ptr_addr_set_pair : vec_ptr_addr_set)
               {
                  std::set<llvm::Value*> erase_set;
                  for(llvm::Value* val : intersection)
                  {
                     if(ptr_addr_set_pair.second.count(val) == 0)
                     {
                        erase_set.insert(val);
                     }
                  }

                  for(llvm::Value* val : erase_set)
                  {
                     intersection.erase(val);
                  }

                  /// Add the base pointer to the set if value is a gepi
                  if(llvm::GetElementPtrInst* gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(ptr_addr_set_pair.first))
                  {
                     ptr_addr_set_pair.first = gepi->getPointerOperand();
                     ptr_addr_set_pair.second.insert(gepi->getPointerOperand());
                     gepi_updates++;
                  }
               }

               /// Control the loop iterations
               switch(intersection.size())
               {
                  case 0:
                     iterate = true;
                     break;
                  case 1:
                  {
                     iterate = false;
                     common_external = *intersection.begin();
                  }
                  break;
                  default:
                     iterate = false;
               }

               iterate = iterate and gepi_updates > 0;
            }

            if(common_external)
            {
               expanded_phis++;
               total_expanded_phis++;
               if(total_expanded_phis >= 6)
                  return expanded_phis > 0;
               std::set<llvm::Instruction*> inst_to_remove;
               std::set<llvm::GetElementPtrInst*> inserted_gepis;

               /// Compute the offset of each external wrt the common one
               std::map<llvm::Use*, llvm::Value*> external_offset_map;

               for(llvm::Use* external_use : external_uses)
               {
                  auto insert_it = external_offset_map.insert(std::make_pair(external_use, llvm::ConstantInt::get(llvm::Type::getInt32Ty(L->getHeader()->getContext()), 0)));

                  llvm::Value*& external_offset_ref = insert_it.first->second;

                  llvm::Use* use_rec = external_use;
                  while(llvm::GetElementPtrInst* use_as_gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(use_rec->get()))
                  {
                     if(use_as_gepi == common_external)
                     {
                        break;
                     }

                     llvm::Use& single_idx = *use_as_gepi->idx_begin();
                     use_rec = &use_as_gepi->getOperandUse(use_as_gepi->getPointerOperandIndex());

                     external_offset_ref = operate_on_index(llvm::BinaryOperator::BinaryOps::Add, external_offset_ref, single_idx, use_as_gepi, idx_ty);
                  }
               }

               llvm::dbgs() << "INFO: Canonicalizing " << get_val_string(candidate_phi) << "\n";
               llvm::dbgs() << "      With reachable uses: \n";
               for(llvm::Use* reachable_use : reachable_uses)
               {
                  llvm::dbgs() << "          #" << reachable_use->getOperandNo() << " in " << get_val_string(reachable_use->getUser()) << "\n";
               }
               llvm::dbgs() << "      With external uses: \n";
               for(llvm::Use* external_use : external_uses)
               {
                  llvm::dbgs() << "          #" << external_use->getOperandNo() << " in " << get_val_string(external_use->getUser()) << " (with offset " << get_val_string(external_offset_map.at(external_use)) << ")\n";
               }
               llvm::dbgs() << "      With common external: \n";
               llvm::dbgs() << "          " << get_val_string(common_external) << "\n";

               /// Transform all phi nodes and then the gepi uses
               std::map<llvm::PHINode*, llvm::PHINode*> phi_map; /// Phi map containing the transformated nodes
               std::set<llvm::Use*> uses_to_set;
               for(llvm::Use* reachable_use : reachable_uses)
               {
                  if(llvm::PHINode* user_as_phi = llvm::dyn_cast<llvm::PHINode>(reachable_use->getUser()))
                  {
                     phi_map[user_as_phi] = nullptr;
                  }
               }

               /// Transform all the phi nodes
               for(std::pair<llvm::PHINode* const, llvm::PHINode*>& phi_pair : phi_map)
               {
                  llvm::PHINode* phi_iter = phi_pair.first;
                  llvm::PHINode* phi_index = llvm::PHINode::Create(idx_ty, phi_iter->getNumIncomingValues(), phi_iter->getName().str() + ".idx", phi_iter);
                  phi_pair.second = phi_index;

                  for(llvm::Use& phi_incoming : phi_iter->incoming_values())
                  {
                     llvm::BasicBlock* incoming_bb = phi_iter->getIncomingBlock(phi_incoming);
                     auto ext_it = external_offset_map.find(&phi_incoming);

                     if(ext_it != external_offset_map.end())
                     {
                        phi_index->addIncoming(ext_it->second, incoming_bb);
                     }
                     else
                     {
                        phi_index->addIncoming(llvm::UndefValue::get(idx_ty), incoming_bb);
                        uses_to_set.insert(&phi_incoming);
                     }
                  }
                  llvm::dbgs() << "      Transforming PHI " << get_val_string(phi_iter) << "\n";
                  llvm::dbgs() << "            in new PHI " << get_val_string(phi_index) << "\n";
               }

               /// Transform all the gepis out of the phi nodes
               std::vector<llvm::Use*> uses_to_process;
               for(std::pair<llvm::PHINode* const, llvm::PHINode*>& phi_pair : phi_map)
               {
                  llvm::PHINode* phi_iter = phi_pair.first;
                  llvm::PHINode* phi_index = phi_pair.second;

                  std::vector<llvm::Value*> idxs;
                  idxs.push_back(phi_index);
                  llvm::GetElementPtrInst* first_gepi = llvm::GetElementPtrInst::CreateInBounds(common_external, idxs, phi_iter->getName().str() + ".firstgepi", &*phi_iter->getParent()->getFirstInsertionPt());

                  llvm::dbgs() << "      Assigning first gepi " << get_val_string(first_gepi) << " to phi " << get_val_string(phi_index) << "\n";

                  phi_iter->replaceAllUsesWith(first_gepi);
                  for(llvm::Use& gepi_use : first_gepi->uses())
                  {
                     uses_to_process.push_back(&gepi_use);
                  }
               }

               for(unsigned long i = 0; i < uses_to_process.size(); i++)
               {
                  llvm::Use* use_to_process = uses_to_process.at(i);

                  llvm::dbgs() << "      Transforming use #" << use_to_process->getOperandNo() << " of " << get_val_string(use_to_process->getUser()) << "\n";
                  if(llvm::GetElementPtrInst* use_as_gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(use_to_process->get()))
                  {
                     if(llvm::PHINode* user_as_phi = llvm::dyn_cast<llvm::PHINode>(use_to_process->getUser()))
                     {
                        auto phi_it = phi_map.find(user_as_phi);
                        if(phi_it != phi_map.end())
                        {
                           llvm::PHINode* phi_index = phi_map.at(user_as_phi);
                           int incoming_value_idx = user_as_phi->getBasicBlockIndex(user_as_phi->getIncomingBlock(*use_to_process));
                           phi_index->setIncomingValue(incoming_value_idx, use_as_gepi->getOperand(1));
                           llvm::dbgs() << "            as " << get_val_string(phi_index) << "\n";
                        }
                        else
                        {
                           llvm::dbgs() << "            as " << get_val_string(user_as_phi) << "\n";
                        }
                        uses_to_set.erase(use_to_process);
                     }
                     else if(llvm::GetElementPtrInst* user_as_gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(use_to_process->getUser()))
                     {
                        llvm::Value* new_idx = operate_on_index(llvm::BinaryOperator::BinaryOps::Add, use_as_gepi->getOperand(1), user_as_gepi->getOperand(1), user_as_gepi, idx_ty);

                        std::vector<llvm::Value*> idxs;
                        idxs.push_back(new_idx);
                        llvm::GetElementPtrInst* new_gepi = llvm::GetElementPtrInst::CreateInBounds(common_external, idxs, user_as_gepi->getName().str() + ".gepi", user_as_gepi);

                        llvm::dbgs() << "            as " << get_val_string(new_gepi) << "\n";

                        user_as_gepi->replaceAllUsesWith(new_gepi);
                        for(llvm::Use& gepi_use : new_gepi->uses())
                        {
                           unsigned long operand_no = gepi_use.getOperandNo();
                           uses_to_set.erase(&gepi_use);
                           uses_to_process.push_back(&gepi_use);
                        }
                        inst_to_remove.insert(user_as_gepi);
                        inserted_gepis.insert(new_gepi);
                     }
                     else if(llvm::CmpInst* user_as_cmp = llvm::dyn_cast<llvm::CmpInst>(use_to_process->getUser()))
                     {
                        unsigned int other_idx = (use_to_process->getOperandNo() == 0 ? 1 : 0);
                        llvm::Use& other_use = user_as_cmp->getOperandUse(other_idx);

                        llvm::Use* use_rec = &other_use;
                        llvm::Value* offset = llvm::ConstantInt::get(idx_ty, 0);
                        if(other_use == common_external)
                        {
                        }
                        else
                        {
                           while(llvm::GetElementPtrInst* use_as_gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(use_rec->get()))
                           {
                              if(use_as_gepi == common_external)
                              {
                                 break;
                              }
                              llvm::Use& single_idx = *use_as_gepi->idx_begin();
                              use_rec = &use_as_gepi->getOperandUse(use_as_gepi->getPointerOperandIndex());
                              offset = operate_on_index(llvm::BinaryOperator::BinaryOps::Add, offset, single_idx, use_as_gepi, idx_ty);
                           }
                        }

                        llvm::CmpInst* new_cmp = llvm::CmpInst::Create(llvm::CmpInst::ICmp, user_as_cmp->getPredicate(), use_as_gepi->getOperand(1), offset, user_as_cmp->getName() + ".idx", user_as_cmp);

                        llvm::dbgs() << "            as " << get_val_string(new_cmp) << "\n";

                        user_as_cmp->replaceAllUsesWith(new_cmp);
                        inst_to_remove.insert(user_as_cmp);
                     }
                  }
                  else
                  {
                     llvm::errs() << "ERR: Use should be a gepi\n";
                     exit(-1);
                  }
               }

               for(std::pair<llvm::PHINode*, llvm::PHINode*> phi_pair : phi_map)
               {
                  phi_pair.first->eraseFromParent();
               }

               unsigned long num_deletion = 0;
               std::set<llvm::Instruction*> remove_set = inst_to_remove;
               do
               {
                  num_deletion = 0;
                  std::set<llvm::Instruction*> removed_set;

                  for(llvm::Instruction* inst_to_erase : remove_set)
                  {
                     if(!inst_to_erase->hasNUsesOrMore(1))
                     {
                        removed_set.insert(inst_to_erase);
                     }
                  }

                  for(llvm::Instruction* inst_to_erase : removed_set)
                  {
                     remove_set.erase(inst_to_erase);
                     inst_to_erase->eraseFromParent();
                  }

                  num_deletion = removed_set.size();
               } while(num_deletion > 0);

               for(llvm::GetElementPtrInst* gepi : inserted_gepis)
               {
                  if(gepi->getNumUses() == 0)
                  {
                     gepi->eraseFromParent();
                  }
               }

               assert("All uses replaced" && uses_to_set.empty());
            }
            else
            {
               llvm::dbgs() << "INFO: Cannot canonicalize (no common)" << get_val_string(candidate_phi) << "\n";
            }
         }
         else
         {
            llvm::dbgs() << "INFO: Cannot canonicalize (unfeasible)" << get_val_string(candidate_phi) << "\n";
         }
      }

      return expanded_phis > 0;
   }

   void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
   {
      AU.addRequired<llvm::TargetTransformInfoWrapperPass>();
      llvm::getLoopAnalysisUsage(AU);
   }
};

char PtrIteratorSimplifyPass::ID = 0;

llvm::Pass* createPtrIteratorSimplifyPass()
{
   return new PtrIteratorSimplifyPass();
}

#endif // SCALAR_REPLACEMENT_OF_AGGREGATES_PTRITERATORSIMPLIFICATIONPASS_HPP
