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
 *              Copyright (C) 2018-2020 Politecnico di Milano
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
 * The implementation performs scalar replacement of aggregates.
 *
 * @author Marco Siracusa <marco.siracusa@mail.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include <llvm/Pass.h>

#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include <llvm/Analysis/InlineCost.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <stack>

#include <cxxabi.h>
#include <llvm/ADT/Statistic.h>

#define DEBUG_TYPE "csroa"

#include "CustomScalarReplacementOfAggregatesPass.hpp"

#define DEBUG_CSROA

static unsigned TotalAllocaBytes /*"Total amount of aggregate bytes by alloca instructions"*/;
static unsigned TotalGlobalBytes /*"Total amount of aggregate bytes by Global variables"*/;
static unsigned TotalOperandBytes /*"Total amount of aggregate bytes as function operands"*/;
static unsigned DisaggregatedAllocaBytes /*"Disaggregated amount of aggregate bytes by alloca instructions"*/;
static unsigned DisaggregatedGlobalBytes /*"Disaggregated amount of aggregate bytes by Global variables"*/;
static unsigned DisaggregatedOperandBytes /*"Disaggregated amount of aggregate bytes as function operands"*/;

#include "fpga_callbacks.hpp"

enum output_streams
{
   OUTS,
   ERRS,
   NULLS
};
class OutputHandler
{
 public:
   OutputHandler()
   {
   }

   llvm::raw_ostream* out;

   void set_output(output_streams os)
   {
      switch(os)
      {
         case OUTS:
            out = &llvm::outs();
            break;
         case ERRS:
            out = &llvm::errs();
            break;
         case NULLS:
            out = &llvm::nulls();
            break;
      }
   }

   template <class T>
   void operator<<(const T& obj)
   {
      *out << obj;
   }
};

OutputHandler output;

// static llvm::cl::opt<uint32_t> MaxNumScalarTypes("csroa-expanded-scalar-threshold", llvm::cl::Hidden, llvm::cl::init(128), llvm::cl::desc("Max amount of scalar types contained in a type for allowing disaggregation"));

// static llvm::cl::opt<uint32_t> MaxTypeByteSize("csroa-type-byte-size", llvm::cl::Hidden, llvm::cl::init(512), llvm::cl::desc("Max type size (in bytes) allowed for disaggregation"));

static llvm::cl::opt<uint32_t> CSROAInlineThreshold("csroa-inline-threshold", llvm::cl::Hidden, llvm::cl::init(200), llvm::cl::desc("number of maximum statements of the single called function after the inline is applied"));

#ifdef DEBUG_CSROA
static llvm::cl::opt<int32_t> CSROAMaxTransformations("csroa-max-transformations", llvm::cl::Hidden, llvm::cl::init(-1), llvm::cl::desc("number of maximum allocas expanded (default=-1, infinite number of transformations"));

std::set<llvm::Value*> recorded_expanded_aggregates;
#endif

// Add debugging checkpoints
#define ADD_CHECKPOINT(ID) llvm::errs() << "CHECKPOINT:" << ID << "\n";

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

std::string getDemangled(const std::string& declname)
{
   int status;
   char* demangled_outbuffer = abi::__cxa_demangle(declname.c_str(), nullptr, nullptr, &status);
   if(status == 0)
   {
      std::string res = declname;
      if(std::string(demangled_outbuffer).find_last_of('(') != std::string::npos)
      {
         res = demangled_outbuffer;
         auto parPos = res.find('(');
         assert(parPos != std::string::npos);
         res = res.substr(0, parPos);
      }
      free(demangled_outbuffer);
      return res;
   }
   assert(demangled_outbuffer == nullptr);
   return declname;
}

class Utilities
{
 public:
   static void print_cfg(const std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& op_expandability_map,
                         const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>>& op_dimensions_map)
   {
      std::vector<llvm::Instruction*> call_trace;

      llvm::dbgs() << "\n*********************************************";
      llvm::dbgs() << "\n******************** CFG ********************";
      llvm::dbgs() << "\n*********************************************\n";
      print_cfg_rec(compact_callgraph, nullptr, op_expandability_map, op_dimensions_map, call_trace);
      llvm::dbgs() << "\n*********************************************\n";
   }

 private:
   static void print_cfg_rec(const std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, llvm::Instruction* call_inst, const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& op_expandability_map,
                             const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>>& op_dimensions_map, std::vector<llvm::Instruction*>& call_trace)
   {
      for(unsigned long long d = 0; d < call_trace.size(); ++d)
      {
         llvm::dbgs() << "  ";
      }

      llvm::dbgs() << "  " << call_inst << "  ";
      if(call_inst)
      {
         llvm::dbgs() << get_val_string(call_inst) << "\n";
      }
      else
      {
         llvm::dbgs() << "KERNEL\n";
      }

      for(unsigned long long d = 0; d < call_trace.size(); ++d)
      {
         llvm::dbgs() << "  ";
      }

      llvm::dbgs() << "  Call trace:  ";
      for(llvm::Instruction* c : call_trace)
      {
         llvm::dbgs() << " " << c << "=" << (c ? llvm::CallSite(c).getCalledFunction()->getName() : "KERNEL") << "  ";
      }
      llvm::dbgs() << "\n";

      if(call_inst)
      {
         for(auto& op : (llvm::isa<llvm::CallInst>(call_inst) ? llvm::dyn_cast<llvm::CallInst>(call_inst)->arg_operands() : llvm::dyn_cast<llvm::InvokeInst>(call_inst)->arg_operands()))
         {
            for(unsigned long long d = 0; d < call_trace.size(); ++d)
            {
               llvm::dbgs() << "  ";
            }

            auto exp_it = op_expandability_map.find(std::make_pair(call_trace, &op));
            std::string exp_str = (exp_it != op_expandability_map.end() ? exp_it->second.get_string() : "-");

            auto dim_it = op_dimensions_map.find(std::make_pair(call_trace, &op));
            std::string dim_str = "-";
            if(dim_it != op_dimensions_map.end())
            {
               dim_str = "";
               for(unsigned long long d : dim_it->second)
               {
                  dim_str += std::to_string(d) + " ";
               }
            }
            llvm::dbgs() << "      Op" << op.getOperandNo() << ":  E( " << exp_str << " )  D( " << dim_str << " )\n";
         }
      }

      call_trace.push_back(call_inst);
      auto call_it = compact_callgraph.find(call_inst);

      if(call_it != compact_callgraph.end())
      {
         auto& call_vec = call_it->second;

         for(auto c : call_vec)
         {
            print_cfg_rec(compact_callgraph, c, op_expandability_map, op_dimensions_map, call_trace);
         }
      }

      call_trace.pop_back();
   }
};

bool is_relevant_function(llvm::Function* called_function)
{
   if(called_function)
   {
      if(!called_function->isIntrinsic())
      {
         if(called_function->size() > 0)
         {
            if(called_function->getBasicBlockList().size() != 0)
            {
               if(!called_function->isVarArg())
               {
                  bool is_wrapper = strncmp(called_function->getName().str().c_str(), std::string(wrapper_function_name).c_str(), std::string(wrapper_function_name).size()) == 0;
                  if(!is_wrapper)
                  {
                     return true;
                  }
               }
            }
         }
      }
   }

   return false;
}

bool is_relevant_call(llvm::Instruction* call_inst)
{
   if(call_inst != nullptr)
   {
      llvm::Function* called_function = llvm::CallSite(call_inst).getCalledFunction();

      if(called_function)
      {
         return is_relevant_function(called_function);
      }
   }

   return false;
}

bool is_allowed_intrinsic_call(llvm::Instruction* call_inst)
{
   if(call_inst != nullptr)
   {
      llvm::Function* called_function = llvm::CallSite(call_inst).getCalledFunction();

      if(called_function)
      {
         if(called_function->getIntrinsicID() == llvm::Intrinsic::lifetime_start)
         {
            return true;
         }
         if(called_function->getIntrinsicID() == llvm::Intrinsic::lifetime_end)
         {
            return true;
         }
      }
   }

   return false;
}

bool has_expandable_size(llvm::Value* aggregate, const llvm::DataLayout& DL, unsigned long long dimension, std::string& msg)
{
   if(aggregate->getType()->isPointerTy())
   {
      bool is_array_arg = dimension > 1;
      bool is_aggregate_type = aggregate->getType()->getPointerElementType()->isAggregateType();
      bool is_gvar_aggregate = llvm::isa<llvm::GlobalVariable>(aggregate) and llvm::dyn_cast<llvm::GlobalVariable>(aggregate)->getValueType()->isAggregateType();

      if(is_aggregate_type or is_gvar_aggregate or is_array_arg)
      {
         std::vector<llvm::Type*> contained_types;

         llvm::Type* ptr_ty = llvm::isa<llvm::GlobalVariable>(aggregate) ? llvm::dyn_cast<llvm::GlobalVariable>(aggregate)->getValueType() : aggregate->getType()->getPointerElementType();
         for(unsigned long long e_idx = 0; e_idx < dimension; e_idx++)
         {
            contained_types.push_back(ptr_ty);
         }

         unsigned long long non_aggregate_types = 0;

         for(auto c_idx = 0u; c_idx < contained_types.size(); c_idx++)
         {
            llvm::Type* el_ty = contained_types.at(c_idx);

            if(el_ty->isAggregateType())
            {
               if(el_ty->isStructTy())
               {
                  for(unsigned int e_idx = 0; e_idx < el_ty->getStructNumElements(); ++e_idx)
                  {
                     contained_types.push_back(el_ty->getStructElementType(e_idx));
                  }
               }
               else if(el_ty->isArrayTy())
               {
                  for(unsigned long long e_idx = 0; e_idx < el_ty->getArrayNumElements(); ++e_idx)
                  {
                     contained_types.push_back(el_ty->getArrayElementType());
                  }
               }
            }
            else
            {
               ++non_aggregate_types;
            }
         }

         unsigned long long allocated_size = DL.getTypeAllocSize(ptr_ty);

         allocated_size *= dimension;

         bool expandable_size = non_aggregate_types <= MaxNumScalarTypes and allocated_size <= MaxTypeByteSize;

         if(!expandable_size)
         {
            msg = "# aggregate types is " + std::to_string(non_aggregate_types) + " (allowed " + std::to_string(MaxNumScalarTypes) + ") and allocates size is " + std::to_string(allocated_size) + "(allowed " + std::to_string(MaxTypeByteSize) + ")";
         }
#ifdef DEBUG_CSROA
         else if(CSROAMaxTransformations != -1)
         {
            if(recorded_expanded_aggregates.find(aggregate) == recorded_expanded_aggregates.end() && recorded_expanded_aggregates.size() >= CSROAMaxTransformations)
            {
               msg = "# maximum limit of alloca expanded reached";
               return false;
            }
            else if(recorded_expanded_aggregates.find(aggregate) == recorded_expanded_aggregates.end())
               recorded_expanded_aggregates.insert(aggregate);
         }
#endif
         return expandable_size;
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }
}

void delete_functions_recursively(std::set<llvm::Function*>& fun_to_remove)
{
   do
   {
      bool some_deletion = false;
      for(auto f_it : fun_to_remove)
      {
         if(f_it->getNumUses() == 0)
         {
            f_it->eraseFromParent();
            fun_to_remove.erase(f_it);
            some_deletion = true;
            break;
         }
      }

      if(!some_deletion)
      {
         break;
      }
   } while(1);

   fun_to_remove.clear();
}

Expandability get_ptr_expandability(llvm::Use& ptr_use, llvm::Value* base_ptr, std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& operands_expandability_map, std::map<llvm::Use*, llvm::Value*>& point_to_set_map,
                                    bool constant_access, std::vector<llvm::Instruction*>& call_trace, const std::map<llvm::Function*, bool>& function_versioning_profitability)
{
   point_to_set_map.insert(std::make_pair(&ptr_use, base_ptr));

   if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(ptr_use.getUser()))
   {
      if(ptr_use.getOperandNo() == gep_op->getPointerOperandIndex())
      {
         llvm::GEPOperator* gepop_rec = gep_op;
         bool chain_has_all_constant_indices = true;
         while(gepop_rec != nullptr)
         {
            if(!gepop_rec->hasAllConstantIndices())
            {
               chain_has_all_constant_indices = false;
               break;
            }
            gepop_rec = llvm::dyn_cast<llvm::GEPOperator>(gepop_rec->getPointerOperand());
         }

         std::string str = "";
         Expandability expandability = compute_gepi_expandability_profit(gep_op, str);
         for(llvm::Use& use : gep_op->uses())
         {
            Expandability ptr_exp = get_ptr_expandability(use, base_ptr, operands_expandability_map, point_to_set_map, chain_has_all_constant_indices, call_trace, function_versioning_profitability);

            expandability.and_add(ptr_exp);

            if(!ptr_exp.expandability)
            {
               llvm::dbgs() << "    Use #" << use.getOperandNo() << " in " << get_val_string(use.getUser()) << " inhibits user expansion\n";
            }
         }

         return expandability;
      }
      else
      {
         return Expandability(false, 0.0, 0.0);
      }
   }
   else if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(ptr_use.getUser()))
   {
      if(ptr_use.getOperandNo() != load_inst->getPointerOperandIndex())
      {
         llvm::dbgs() << "    " << get_val_string(load_inst) << " inhibits user expansion\n";
      }

      std::string msg = "";
      Expandability load_expandability = compute_load_expandability_profit(load_inst, msg);
      load_expandability.expandability = ptr_use.getOperandNo() == load_inst->getPointerOperandIndex();
      return load_expandability;
   }
   else if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(ptr_use.getUser()))
   {
      if(ptr_use.getOperandNo() != store_inst->getPointerOperandIndex())
      {
         llvm::dbgs() << "    " << get_val_string(store_inst) << " inhibits user expansion\n";
      }

      std::string msg = "";
      Expandability store_expandability = compute_store_expandability_profit(store_inst, msg);
      store_expandability.expandability = ptr_use.getOperandNo() == store_inst->getPointerOperandIndex();
      return store_expandability;
   }
   else if(llvm::isa<llvm::CallInst>(ptr_use.getUser()) || llvm::isa<llvm::InvokeInst>(ptr_use.getUser()))
   {
      llvm::GEPOperator* gepop_rec = llvm::dyn_cast<llvm::GEPOperator>(ptr_use.get());
      bool chain_has_all_constant_indices = true;
      while(gepop_rec != nullptr)
      {
         if(!gepop_rec->hasAllConstantIndices())
         {
            chain_has_all_constant_indices = false;
            break;
         }
         gepop_rec = llvm::dyn_cast<llvm::GEPOperator>(gepop_rec->getPointerOperand());
      }

      auto call_inst = llvm::dyn_cast<llvm::Instruction>(ptr_use.getUser());
      if(false and !chain_has_all_constant_indices)
      {
         if(!operands_expandability_map.insert(std::make_pair(std::make_pair(call_trace, &ptr_use), Expandability(false, 0.0, 0.0))).second)
         {
            llvm::errs() << "ERR: Operand expandability inserted twice in map\n";
            exit(-1);
         }

         llvm::dbgs() << "    Use #" << ptr_use.getOperandNo() << " in " << get_val_string(ptr_use.getUser()) << " inhibits user expansion (non constant gepi chain in call)\n";

         return Expandability(false, 0.0, 0.0);
      }
      else if(is_relevant_call(call_inst))
      {
         llvm::Function* called_function = llvm::CallSite(call_inst).getCalledFunction();

         Expandability expandability(chain_has_all_constant_indices, 0.0, 0.0);

         auto profitability_it = function_versioning_profitability.find(called_function);

         if(profitability_it != function_versioning_profitability.end())
         {
            expandability.expandability = expandability.expandability and profitability_it->second;
         }

         if(!expandability.expandability)
         {
            llvm::dbgs() << "    Use #" << ptr_use.getOperandNo() << " in " << get_val_string(ptr_use.getUser()) << " inhibits user expansion\n";
         }

         llvm::Argument* ptr_arg = &*std::next(called_function->arg_begin(), ptr_use.getOperandNo());

         call_trace.push_back(call_inst);
         for(llvm::Use& use : ptr_arg->uses())
         {
            Expandability op_exp = get_ptr_expandability(use, base_ptr, operands_expandability_map, point_to_set_map, true, call_trace, function_versioning_profitability);
            expandability.and_add(op_exp);

            if(!op_exp.expandability)
            {
               llvm::dbgs() << "    Use #" << ptr_use.getOperandNo() << " in " << get_val_string(ptr_use.getUser()) << " inhibits user expansion\n";
            }
         }
         call_trace.pop_back();

         double profit = expandability.area_profit;
         bool got_casted = expandability.cast();
         if(got_casted)
         {
            llvm::dbgs() << "INFO: not profitable (" << profit << ") to expand use #" << ptr_use.getOperandNo() << " in " << get_val_string(ptr_use.getUser()) << "\n";
         }

         if(!operands_expandability_map.insert(std::make_pair(std::make_pair(call_trace, &ptr_use), expandability)).second)
         {
            llvm::errs() << "ERR: Operand expandability inserted twice in map\n";
            exit(-1);
         }

         return expandability;
      }
      else
      {
         if(false and !operands_expandability_map.insert(std::make_pair(std::make_pair(call_trace, &ptr_use), Expandability(false, 0.0, 0.0))).second)
         {
            llvm::errs() << "ERR: Operand expandability inserted twice in map\n";
            exit(-1);
         }
         return Expandability(false, 0.0, 0.0);
      }
   }
   else if(llvm::BitCastOperator* bitcast_op = llvm::dyn_cast<llvm::BitCastOperator>(ptr_use.getUser()))
   {
      for(llvm::Use& use : bitcast_op->uses())
      {
         get_ptr_expandability(use, base_ptr, operands_expandability_map, point_to_set_map, constant_access, call_trace, function_versioning_profitability);
         if(llvm::isa<llvm::CallInst>(use.getUser()) || llvm::isa<llvm::InvokeInst>(use.getUser()))
         {
            llvm::Instruction* call_inst = llvm::dyn_cast<llvm::Instruction>(use.getUser());
            if(!is_allowed_intrinsic_call(call_inst))
            {
               llvm::dbgs() << "    Use #" << ptr_use.getOperandNo() << " in " << get_val_string(ptr_use.getUser()) << " inhibits user expansion\n";
               return Expandability(false, 0.0, 0.0);
            }
         }
         else
         {
            llvm::dbgs() << "    Use #" << ptr_use.getOperandNo() << " in " << get_val_string(ptr_use.getUser()) << " inhibits user expansion\n";
            return Expandability(false, 0.0, 0.0);
         }
      }

      return Expandability(true, 0.0, 0.0);
   }
   else
   {
      llvm::dbgs() << "    Use #" << ptr_use.getOperandNo() << " in " << get_val_string(ptr_use.getUser()) << " inhibits user expansion\n";
      return Expandability(false, 0.0, 0.0);
   }
}

void compute_allocas_expandability_rec(llvm::Instruction* call_inst, llvm::Function* kernel_function, std::map<llvm::AllocaInst*, Expandability>& allocas_expandability_map, std::map<llvm::GlobalVariable*, Expandability>& globals_expandability_map,
                                       std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& operands_expandability_map, std::map<llvm::Use*, llvm::Value*>& point_to_set_map,
                                       std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, const llvm::DataLayout& DL, std::vector<llvm::Instruction*>& call_trace,
                                       const std::map<llvm::Function*, bool>& function_versioning_profitability)
{
   call_trace.push_back(call_inst);

   llvm::Function* called_function = nullptr;
   if(call_inst)
   {
      called_function = llvm::CallSite(call_inst).getCalledFunction();
   }
   else
   {
      called_function = kernel_function;
   }

   std::vector<llvm::Instruction*> call_inst_vec;

   for(llvm::BasicBlock& bb : *called_function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(&i))
         {
            if(alloca_inst->getType()->getPointerElementType()->isAggregateType())
            {
               std::string size_msg = "";
               Expandability expandability = compute_alloca_expandability_profit(alloca_inst, DL, size_msg);
               // #HOTPOINT

               if(!expandability.expandability)
               {
                  llvm::dbgs() << "WAR: " << get_val_string(alloca_inst) << " in " << alloca_inst->getFunction()->getName() << " cannot expand: " << size_msg << "\n";
               }

               for(llvm::Use& use : alloca_inst->uses())
               {
                  Expandability ptr_exp = get_ptr_expandability(use, alloca_inst, operands_expandability_map, point_to_set_map, true, call_trace, function_versioning_profitability);
                  expandability.and_add(ptr_exp);

                  if(!ptr_exp.expandability)
                  {
                     llvm::dbgs() << "WAR: " << get_val_string(alloca_inst) << " in " << alloca_inst->getFunction()->getName() << " cannot expand because of use #" << use.getOperandNo() << " in " << get_val_string(use.getUser()) << "\n";
                  }
               }

               double profit = expandability.area_profit;
               bool got_casted = expandability.cast();
               if(got_casted)
               {
                  llvm::dbgs() << "INFO: not profitable (" << profit << ") to expand " << get_val_string(alloca_inst) << "\n";
               }

               allocas_expandability_map.insert(std::make_pair(alloca_inst, expandability));
            }
            else
            {
               allocas_expandability_map.insert(std::make_pair(alloca_inst, Expandability(false, 0.0, 0.0)));
            }
         }
         else if(llvm::isa<llvm::CallInst>(i) || llvm::isa<llvm::InvokeInst>(i))
         {
            llvm::Instruction* inner_call_inst = llvm::dyn_cast<llvm::Instruction>(&i);
            if(is_relevant_call(inner_call_inst))
            {
               call_inst_vec.push_back(inner_call_inst);
            }
         }
      }
   }

   compact_callgraph.insert(std::make_pair(call_inst, call_inst_vec));

   for(llvm::Instruction* inner_call_inst : call_inst_vec)
   {
      compute_allocas_expandability_rec(inner_call_inst, nullptr, allocas_expandability_map, globals_expandability_map, operands_expandability_map, point_to_set_map, compact_callgraph, DL, call_trace, function_versioning_profitability);
   }

   call_trace.pop_back();
}

void compute_global_op_expandability_rec(llvm::Instruction* call_inst, const std::map<llvm::GlobalVariable*, Expandability>& globals_expandability_map,
                                         std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& operands_expandability_map, std::map<llvm::Use*, llvm::Value*>& point_to_set_map,
                                         std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, const llvm::DataLayout& DL, std::vector<llvm::Instruction*>& call_trace, const std::set<llvm::Use*>& globals_as_exp,
                                         const std::map<llvm::Function*, bool>& function_versioning_profitability)
{
   call_trace.push_back(call_inst);

   auto call_it = compact_callgraph.find(call_inst);

   if(call_it != compact_callgraph.end())
   {
      for(auto inner_call_inst : call_it->second)
      {
         for(llvm::Use& use : llvm::CallSite(inner_call_inst).args())
         {
            if(globals_as_exp.count(&use) > 0)
            {
               get_ptr_expandability(use, point_to_set_map.at(&use), operands_expandability_map, point_to_set_map, true, call_trace, function_versioning_profitability);
            }
         }

         compute_global_op_expandability_rec(inner_call_inst, globals_expandability_map, operands_expandability_map, point_to_set_map, compact_callgraph, DL, call_trace, globals_as_exp, function_versioning_profitability);
      }
   }

   call_trace.pop_back();
}

void compute_aggregates_expandability(llvm::Function* kernel_function, llvm::Module* module, std::map<llvm::AllocaInst*, Expandability>& allocas_expandability_map, std::map<llvm::GlobalVariable*, Expandability>& globals_expandability_map,
                                      std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& operands_expandability_map, std::map<llvm::Use*, llvm::Value*>& point_to_set_map,
                                      std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, const llvm::DataLayout& DL, const std::map<llvm::Function*, bool>& function_versioning_profitability)
{
   std::vector<llvm::Instruction*> call_trace;
   compute_allocas_expandability_rec(nullptr, kernel_function, allocas_expandability_map, globals_expandability_map, operands_expandability_map, point_to_set_map, compact_callgraph, DL, call_trace, function_versioning_profitability);

   std::set<llvm::Function*> function_worklist;
   for(auto cc_it : compact_callgraph)
   {
      for(auto call_inst : cc_it.second)
      {
         function_worklist.insert(llvm::CallSite(call_inst).getCalledFunction());
      }
   }
   function_worklist.insert(kernel_function);

   std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability> operands_expandability_map_by_globals;
   for(llvm::GlobalVariable& global_var : module->globals())
   {
      bool all_uses_in_worklist = true;

      for(llvm::Use& use : global_var.uses())
      {
         if(auto user_inst = llvm::dyn_cast<llvm::Instruction>(use.getUser()))
         {
            if(function_worklist.count(user_inst->getFunction()) == 0)
            {
               all_uses_in_worklist = false;
               break;
            }
         }
      }

      if(all_uses_in_worklist)
      {
         if(global_var.getType()->getPointerElementType()->isAggregateType())
         {
            call_trace.clear();
            call_trace.push_back(nullptr);

            std::string size_msg = "";

            Expandability expandability = compute_global_expandability_profit(&global_var, DL, size_msg);

            if(!expandability.expandability)
            {
               llvm::dbgs() << "WAR: " << get_val_string(&global_var) << " cannot expand: " << size_msg << "\n";
            }

            for(llvm::Use& use : global_var.uses())
            {
               Expandability ptr_exp = get_ptr_expandability(use, &global_var, operands_expandability_map_by_globals, point_to_set_map, true, call_trace, function_versioning_profitability);
               expandability.and_add(ptr_exp);

               if(!ptr_exp.expandability)
               {
                  llvm::dbgs() << "WAR: " << get_val_string(&global_var) << " cannot expand because of use #" << use.getOperandNo() << " in " << get_val_string(use.getUser()) << "\n";
               }
            }

            double profit = expandability.area_profit;
            bool got_casted = expandability.cast();
            if(got_casted)
            {
               llvm::dbgs() << "INFO: not profitable (" << profit << ") to expand " << global_var.getName() << "\n";
            }

            globals_expandability_map.insert(std::make_pair(&global_var, expandability));
            call_trace.pop_back();
         }
         else
         {
            globals_expandability_map.insert(std::make_pair(&global_var, Expandability(false, 0.0, 0.0)));
         }
      }
      else
      {
         globals_expandability_map.insert(std::make_pair(&global_var, Expandability(false, 0.0, 0.0)));
         llvm::dbgs() << "WAR: " << get_val_string(&global_var) << "  cannot expand because some uses out of worklist\n";
      }
   }

   std::vector<llvm::Instruction*> null_call_trace = std::vector<llvm::Instruction*>(1, nullptr);
   std::set<llvm::Use*> global_as_op;
   for(auto& op_it : operands_expandability_map_by_globals)
   {
      if(op_it.first.first == null_call_trace)
      {
         global_as_op.insert(op_it.first.second);
      }
   }

   compute_global_op_expandability_rec(nullptr, globals_expandability_map, operands_expandability_map, point_to_set_map, compact_callgraph, DL, call_trace, global_as_op, function_versioning_profitability);
}

void compute_callgraph_rec(llvm::Instruction* call_inst, llvm::Function* kernel_function, std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, std::vector<llvm::Instruction*>& call_trace)
{
   if(compact_callgraph.count(call_inst) > 0)
   {
      return;
   }
   call_trace.push_back(call_inst);

   llvm::Function* called_function = nullptr;
   if(call_inst)
   {
      called_function = llvm::CallSite(call_inst).getCalledFunction();
   }
   else
   {
      called_function = kernel_function;
   }

   std::vector<llvm::Instruction*> call_inst_vec;

   for(llvm::BasicBlock& bb : *called_function)
   {
      for(llvm::Instruction& i : bb)
      {
         if(llvm::isa<llvm::CallInst>(i) || llvm::isa<llvm::InvokeInst>(i))
         {
            auto inner_call_inst = llvm::dyn_cast<llvm::Instruction>(&i);
            if(is_relevant_call(inner_call_inst))
            {
               call_inst_vec.push_back(inner_call_inst);
            }
         }
      }
   }

   if(!compact_callgraph.insert(std::make_pair(call_inst, call_inst_vec)).second)
   {
      llvm::errs() << "ERR: call inst reinserted twice in callgraph\n";
      exit(-1);
   }

   for(llvm::Instruction* inner_call_inst : call_inst_vec)
   {
      compute_callgraph_rec(inner_call_inst, nullptr, compact_callgraph, call_trace);
   }

   call_trace.pop_back();
}

void compute_callgraph(llvm::Function* kernel_function, std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph)
{
   std::vector<llvm::Instruction*> call_trace;
   compute_callgraph_rec(nullptr, kernel_function, compact_callgraph, call_trace);
}

std::pair<double, std::vector<unsigned long long>> get_op_dims(llvm::Use* op_use, llvm::Instruction* parent_call_inst, const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>>& op_dims_map,
                                                               const std::vector<llvm::Instruction*>& call_trace)
{
   std::vector<llvm::Instruction*> parent_call_trace = std::vector<llvm::Instruction*>(call_trace.begin(), call_trace.end() - 1);

   llvm::Value* op = op_use->get();

   /*
    * Compute the dimension of the value (supposed arg operand) returning:
    *  - an empty vector in case of scalar or
    *  - a vector containing the operand dimensions in case of pointer
    */

   // If pointer returns empty vector, proceed otherwise
   if(op->getType()->isPointerTy())
   {
      // Discriminate whether the operand is a gepi, argument, alloca or global
      if(llvm::GEPOperator* gep_op_op = llvm::dyn_cast<llvm::GEPOperator>(op))
      {
         // In case the gepi has one index only, the argument
         if(gep_op_op->getNumIndices() == 1)
         {
            llvm::Value* ptr_op = gep_op_op->getPointerOperand();

            // Which is supposed to be an argument (err otherwise)
            if(llvm::Argument* arg_ptr_op = llvm::dyn_cast<llvm::Argument>(ptr_op))
            {
               llvm::Use& parent_op = parent_call_inst->getOperandUse(arg_ptr_op->getArgNo());
               auto op_dim_it = op_dims_map.find(std::make_pair(parent_call_trace, &parent_op));

               // Which is supposed to have known size (since already processed)
               if(op_dim_it != op_dims_map.end())
               {
                  std::vector<unsigned long long> arg_dims = op_dim_it->second;

                  // In case of known constant index shorten the array accordingly
                  unsigned long long idx = 0;
                  if(llvm::ConstantInt* c_idx = llvm::dyn_cast<llvm::ConstantInt>(gep_op_op->getOperand(1)))
                  {
                     idx = (unsigned long long)c_idx->getSExtValue();
                  }

                  // Check if the argument size is not empty (as it should)
                  if(!arg_dims.empty())
                  {
                     unsigned long long new_arg_dims_front = arg_dims.front() - idx;
                     if(new_arg_dims_front > 0)
                     {
                        arg_dims.front() = new_arg_dims_front;
                     }
                     else
                     {
                        llvm::dbgs() << "WAR: Dim out of range for " << get_val_string(gep_op_op) << "\n";
                        return std::make_pair(false, std::vector<unsigned long long>());
                        /// exit(-1);
                     }
                  }
                  else
                  {
                     llvm::errs() << "ERR: empty dims for " << get_val_string(gep_op_op) << "\n";
                     return std::make_pair(false, std::vector<unsigned long long>());
                     exit(-1);
                  }

                  return std::make_pair(true, arg_dims);
               }
               else
               {
                  llvm::errs() << "WAR: Argument dimension not found for " << get_val_string(arg_ptr_op) << " in " << get_val_string(op_use->getUser()) << "\n";
                  return std::make_pair(false, std::vector<unsigned long long>());
                  exit(-1);
               }
            }
            else
            {
               llvm::errs() << "WAR: Ptr op can be argument only for gepi " << get_val_string(gep_op_op) << "\n";
               return std::make_pair(false, std::vector<unsigned long long>());
               exit(-1);
            }
         }
         else if(gep_op_op->hasIndices()) // Otherwise, if gepi has indices
         {
            // Spot the last and last but one index
            llvm::Value* last_idx = gep_op_op->getOperand(gep_op_op->getNumOperands() - 1);

            // Get the last and last but one indexed type
            llvm::Type* last_idx_type = nullptr;
            llvm::Type* last_but_one_idx_type = nullptr;
            unsigned long long op_idx = 0;
            for(llvm::gep_type_iterator gt_it = llvm::gep_type_begin(gep_op_op), GTE = llvm::gep_type_end(gep_op_op); gt_it != GTE; ++gt_it)
            {
               if(op_idx == gep_op_op->getNumIndices() - 1)
               {
                  last_idx_type = gt_it.getIndexedType();
               }
               else if(op_idx == gep_op_op->getNumIndices() - 2)
               {
                  last_but_one_idx_type = gt_it.getIndexedType();
               }

               ++op_idx;
            }

            // Consider the last but one avoiding decay so
            llvm::Type* rec_ty = last_but_one_idx_type;
            std::vector<unsigned long long> gepi_dims;
            do
            {
               if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(rec_ty))
               {
                  gepi_dims.push_back(arr_ty->getArrayNumElements());
                  rec_ty = arr_ty->getArrayElementType();
               }
               else
               {
                  break;
               }
            } while(true);

            unsigned long long idx = 0;
            if(llvm::ConstantInt* c_idx = llvm::dyn_cast<llvm::ConstantInt>(last_idx))
            {
               idx = (unsigned long long)c_idx->getSExtValue();
            }

            if(!gepi_dims.empty())
            {
               unsigned long long new_arg_dims_front = gepi_dims.front() - idx;
               if(new_arg_dims_front > 0)
               {
                  gepi_dims.front() = new_arg_dims_front;
               }
               else
               {
                  llvm::errs() << "WAR: Dim out of range for " << get_val_string(gep_op_op) << "\n";
                  exit(-1);
               }
            }
            else
            {
               gepi_dims.push_back(1);
            }

            return std::make_pair(true, gepi_dims);
         }
         else
         {
            llvm::errs() << "WAR: Gep op with no indices " << get_val_string(gep_op_op) << "\n";
            return std::make_pair(false, std::vector<unsigned long long>());
            exit(-1);
         }
      }
      else if(llvm::Argument* arg_op = llvm::dyn_cast<llvm::Argument>(op))
      {
         llvm::Use& parent_op = parent_call_inst->getOperandUse(arg_op->getArgNo());
         auto op_dim_it = op_dims_map.find(std::make_pair(parent_call_trace, &parent_op));

         // Which is supposed to have known size (since already processed)
         if(op_dim_it != op_dims_map.end())
         {
            std::vector<unsigned long long> arg_dims = op_dim_it->second;
            return std::make_pair(true, arg_dims);
         }
         else
         {
            llvm::errs() << "WAR: Argument dimension not found for " << get_val_string(arg_op) << " at " << get_val_string(op_use->getUser()) << "\n";
            return std::make_pair(false, std::vector<unsigned long long>());
            exit(-1);
         }
      }
      else if(llvm::AllocaInst* alloca_inst_op = llvm::dyn_cast<llvm::AllocaInst>(op))
      {
         llvm::Type* rec_ty = alloca_inst_op->getType()->getPointerElementType();
         std::vector<unsigned long long> alloca_dims;
         do
         {
            if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(rec_ty))
            {
               alloca_dims.push_back(arr_ty->getArrayNumElements());
               rec_ty = arr_ty->getArrayElementType();
            }
            else
            {
               break;
            }
         } while(true);

         if(alloca_dims.empty())
         {
            alloca_dims.push_back(1);
         }
         return std::make_pair(true, alloca_dims);
      }
      else if(llvm::GlobalVariable* g_var_op = llvm::dyn_cast<llvm::GlobalVariable>(op))
      {
         llvm::Type* rec_ty = g_var_op->getType()->getPointerElementType();
         std::vector<unsigned long long> g_var_dims;
         do
         {
            if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(rec_ty))
            {
               g_var_dims.push_back(arr_ty->getArrayNumElements());
               rec_ty = arr_ty->getArrayElementType();
            }
            else
            {
               break;
            }
         } while(true);

         if(g_var_dims.empty())
         {
            g_var_dims.push_back(1);
         }
         return std::make_pair(true, g_var_dims);
      }
      else
      {
         llvm::errs() << "WAR: unknown element for " << get_val_string(op_use->get()) << " in " << get_val_string(op_use->getUser()) << "\n";
         return std::make_pair(false, std::vector<unsigned long long>());
         // return std::vector<unsigned long long>();
         exit(-1);
      }
   }
   else
   {
      return std::make_pair(false, std::vector<unsigned long long>());
   }
}

void compute_op_exp_and_dims_rec(llvm::Instruction* call_inst, llvm::Instruction* parent_call_inst, const std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, const std::map<llvm::Use*, llvm::Value*>& point_to_set_map,
                                 std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& operands_expandability_map, const std::map<llvm::AllocaInst*, Expandability>& allocas_expandability_map,
                                 const std::map<llvm::GlobalVariable*, Expandability>& globals_expandability_map, std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>>& operands_dimensions_map,
                                 const llvm::DataLayout& DL, std::vector<llvm::Instruction*>& call_trace, const std::map<llvm::Function*, bool>& function_versioning_profitability)
{
   if(call_inst)
   {
      if(llvm::Function* called_function = llvm::CallSite(call_inst).getCalledFunction())
      {
         auto nOperands = llvm::isa<llvm::CallInst>(call_inst) ? llvm::dyn_cast<llvm::CallInst>(call_inst)->getNumArgOperands() : llvm::dyn_cast<llvm::InvokeInst>(call_inst)->getNumArgOperands();
         for(auto idx = 0u; idx < nOperands; ++idx)
         {
            llvm::Use& op_use = call_inst->getOperandUse(idx);

            std::pair<std::vector<llvm::Instruction*>, llvm::Use*> call_key = std::make_pair(call_trace, &op_use);

            std::vector<unsigned long long> dims = std::vector<unsigned long long>();
            auto exp_it = operands_expandability_map.find(call_key);
            if(exp_it != operands_expandability_map.end())
            {
               if(exp_it->second.expandability)
               {
                  bool found = true;
                  std::tie(found, dims) = get_op_dims(&op_use, parent_call_inst, operands_dimensions_map, call_trace);

                  if(found)
                  {
                     if(dims.size() == 1 and dims.front() == 1)
                     {
                        dims.clear();
                     }

                     std::string size_msg = "";
                     unsigned long long first_dim = (dims.empty() ? 0 : dims.front());

                     Expandability expandability = compute_operand_expandability_profit(&op_use, DL, first_dim, size_msg);

                     operands_expandability_map[call_key] = expandability;
                     if(!expandability.expandability)
                     {
                        llvm::dbgs() << "WAR: Use #" << op_use.getOperandNo() << " in " << get_val_string(op_use.getUser()) << "  cannot expand because of its size (" << size_msg << ")\n";
                     }
                  }
                  else
                  {
                     Expandability expandability = Expandability(false, 0.0, 0.0);

                     operands_expandability_map[call_key] = expandability;
                     if(!expandability.expandability)
                     {
                        llvm::dbgs() << "WAR: Use #" << op_use.getOperandNo() << " in " << get_val_string(op_use.getUser()) << "  cannot expand because not computable dimensions\n";
                     }
                  }
               }
            }
            else
            {
               operands_expandability_map.insert(std::make_pair(call_key, Expandability(false, 0.0, 0.0)));
               ;
            }

            operands_dimensions_map.insert(std::make_pair(call_key, dims));
         }
      }
   }

   auto call_it = compact_callgraph.find(call_inst);

   if(call_it != compact_callgraph.end())
   {
      call_trace.push_back(call_inst);
      for(llvm::Instruction* inner_call_inst : call_it->second)
      {
         compute_op_exp_and_dims_rec(inner_call_inst, call_inst, compact_callgraph, point_to_set_map, operands_expandability_map, allocas_expandability_map, globals_expandability_map, operands_dimensions_map, DL, call_trace,
                                     function_versioning_profitability);
      }
      call_trace.pop_back();
   }
}

void compute_op_exp_and_dims(const std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, const std::map<llvm::Use*, llvm::Value*>& point_to_set_map,
                             std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& operands_expandability_map, const std::map<llvm::AllocaInst*, Expandability>& allocas_expandability_map,
                             const std::map<llvm::GlobalVariable*, Expandability>& globals_expandability_map, std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>>& operands_dimensions_map,
                             const llvm::DataLayout& DL, const std::map<llvm::Function*, bool>& function_versioning_profitability)
{
   std::vector<llvm::Instruction*> call_trace;
   compute_op_exp_and_dims_rec(nullptr, nullptr, compact_callgraph, point_to_set_map, operands_expandability_map, allocas_expandability_map, globals_expandability_map, operands_dimensions_map, DL, call_trace, function_versioning_profitability);
}

template <class I>
I* get_related_inst(I* inst, llvm::Function* new_function)
{
   auto bb_iter = inst->getParent()->getIterator();
   auto inst_iter = inst->getIterator();

   signed long long bb_dist = std::distance(inst->getFunction()->begin(), bb_iter);
   signed long long inst_dist = std::distance(inst->getParent()->begin(), inst_iter);

   auto new_bb_iter = std::next(new_function->begin(), bb_dist);
   auto new_inst_iter = std::next(new_bb_iter->begin(), inst_dist);

   if(I* new_inst = llvm::dyn_cast<I>(&*new_inst_iter))
   {
      return new_inst;
   }
   else
   {
      llvm::errs() << "ERR Wrong related instruction type\n";
      exit(-1);
   }
}
/*
void initialize_callsites(llvm::Instruction* call_inst,
                          std::vector<llvm::Instruction*>& call_trace,
                          const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, bool>& operands_expandability_map,
                          const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>>& operands_dimensions_map,
                          const std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph,
                          std::map<llvm::Instruction*, std::map<std::vector<llvm::Instruction*>, std::vector<std::pair<bool, std::vector<unsigned long long>>>>>& op_exp_and_dim_by_callsite)
{
   if(call_inst)
   {
      auto nOperands = llvm::isa<llvm::CallInst>(call_inst) ? llvm::dyn_cast<llvm::CallInst>(call_inst)->getNumArgOperands() : llvm::dyn_cast<llvm::InvokeInst>(call_inst)->getNumArgOperands();
      std::vector<std::pair<bool, std::vector<unsigned long long>>> init_vec = std::vector<std::pair<bool, std::vector<unsigned long long>>>(nOperands, std::make_pair(false, std::vector<unsigned long long>()));

      op_exp_and_dim_by_callsite[call_inst][call_trace] = init_vec;
   }

   auto call_it = compact_callgraph.find(call_inst);

   if(call_it != compact_callgraph.end())
   {
      call_trace.push_back(call_inst);
      for(llvm::Instruction* inner_call_inst : call_it->second)
      {
         initialize_callsites(inner_call_inst, call_trace, operands_expandability_map, operands_dimensions_map, compact_callgraph, op_exp_and_dim_by_callsite);
      }
      call_trace.pop_back();
   }
}
*/
void update_calls_rec(llvm::Instruction* call_inst, llvm::Function* parent_versioned_function, std::vector<llvm::Instruction*>& call_trace, const std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph,
                      const std::map<llvm::Instruction*, std::map<std::vector<llvm::Instruction*>, std::vector<std::pair<bool, std::vector<unsigned long long>>>>>& op_exp_and_dim_by_callsite,
                      const std::map<std::tuple<llvm::Function*, std::vector<bool>, std::vector<std::vector<unsigned long long>>>, llvm::Function*>& versioned_functions_map,
                      std::set<std::tuple<llvm::Function*, std::vector<bool>, std::vector<std::vector<unsigned long long>>>>& processed_versioned_functions, const std::map<llvm::Function*, bool>& function_versioning_profitability)
{
   llvm::Function* next_parent = nullptr;
   if(call_inst)
   {
      if(llvm::Function* called_function = llvm::CallSite(call_inst).getCalledFunction())
      {
         bool is_versioning_profitable = function_versioning_profitability.at(called_function);

         const std::vector<std::pair<bool, std::vector<unsigned long long>>>& op_exp_and_dim_vec = op_exp_and_dim_by_callsite.at(call_inst).at(call_trace);

         std::vector<bool> exp_vec;
         std::vector<std::vector<unsigned long long>> dim_vec;

         for(const auto& pair : op_exp_and_dim_vec)
         {
            // exp_vec.push_back(is_versioning_profitable and pair.first);
            // dim_vec.push_back(pair.second);
            if(is_versioning_profitable)
            {
               exp_vec.push_back(pair.first);
               dim_vec.push_back(pair.second);
            }
            else
            {
               exp_vec.push_back(false);
               dim_vec.push_back(std::vector<unsigned long long>());
            }
         }

         std::tuple<llvm::Function*, std::vector<bool>, std::vector<std::vector<unsigned long long>>> vfun_key = std::make_tuple(called_function, exp_vec, dim_vec);

         llvm::Function* versioned_function = versioned_functions_map.at(vfun_key);

         llvm::Instruction* versioned_parent_call_inst = get_related_inst<llvm::Instruction>(call_inst, parent_versioned_function);
         if(llvm::isa<llvm::CallInst>(versioned_parent_call_inst))
            llvm::dyn_cast<llvm::CallInst>(versioned_parent_call_inst)->setCalledFunction(versioned_function);
         else
            llvm::dyn_cast<llvm::InvokeInst>(versioned_parent_call_inst)->setCalledFunction(versioned_function);
         if(!processed_versioned_functions.insert(vfun_key).second)
         {
            return;
         }

         next_parent = versioned_function;
      }
   }

   if(next_parent == nullptr)
   {
      next_parent = parent_versioned_function;
   }

   auto call_it = compact_callgraph.find(call_inst);

   if(call_it != compact_callgraph.end())
   {
      call_trace.push_back(call_inst);
      for(llvm::Instruction* inner_call_inst : call_it->second)
      {
         update_calls_rec(inner_call_inst, next_parent, call_trace, compact_callgraph, op_exp_and_dim_by_callsite, versioned_functions_map, processed_versioned_functions, function_versioning_profitability);
      }
      call_trace.pop_back();
   }
}

void initialize_callsites(llvm::Instruction* call_inst, std::vector<llvm::Instruction*>& call_trace, const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& operands_expandability_map,
                          const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>>& operands_dimensions_map, const std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph,
                          std::map<llvm::Instruction*, std::map<std::vector<llvm::Instruction*>, std::vector<std::pair<bool, std::vector<unsigned long long>>>>>& op_exp_and_dim_by_callsite)
{
   if(call_inst)
   {
      auto nOperands = llvm::isa<llvm::CallInst>(call_inst) ? llvm::dyn_cast<llvm::CallInst>(call_inst)->getNumArgOperands() : llvm::dyn_cast<llvm::InvokeInst>(call_inst)->getNumArgOperands();
      std::vector<std::pair<bool, std::vector<unsigned long long>>> init_vec = std::vector<std::pair<bool, std::vector<unsigned long long>>>(nOperands, std::make_pair(false, std::vector<unsigned long long>()));

      op_exp_and_dim_by_callsite[call_inst][call_trace] = init_vec;
   }

   auto call_it = compact_callgraph.find(call_inst);

   if(call_it != compact_callgraph.end())
   {
      call_trace.push_back(call_inst);
      for(llvm::Instruction* inner_call_inst : call_it->second)
      {
         initialize_callsites(inner_call_inst, call_trace, operands_expandability_map, operands_dimensions_map, compact_callgraph, op_exp_and_dim_by_callsite);
      }
      call_trace.pop_back();
   }
}

void compute_function_versioning_profit(const std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, llvm::Function* kernel_function, const std::map<llvm::Use*, llvm::Value*>& point_to_set_map,
                                        std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& operands_expandability_map, const std::map<llvm::AllocaInst*, Expandability>& allocas_expandability_map,
                                        const std::map<llvm::GlobalVariable*, Expandability>& globals_expandability_map, std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>>& operands_dimensions_map,
                                        const llvm::DataLayout& DL, std::map<llvm::Function*, bool>& versioning_cost_map, bool force_no_versioning)
{
   std::set<llvm::Use*> forbidden_expansions;
   std::vector<llvm::Instruction*> call_trace;

   std::map<llvm::Instruction*, std::map<std::vector<llvm::Instruction*>, std::vector<std::pair<bool, std::vector<unsigned long long>>>>> op_exp_and_dim_by_callsite;

   {
      std::vector<llvm::Instruction*> call_trace;
      initialize_callsites(nullptr, call_trace, operands_expandability_map, operands_dimensions_map, compact_callgraph, op_exp_and_dim_by_callsite);
   }

   if(operands_expandability_map.size() != operands_dimensions_map.size())
   {
      llvm::errs() << "ERR Exp and dim maps with different sizes (" << operands_expandability_map.size() << " vs. " << operands_dimensions_map.size() << ")\n";
      llvm::errs() << "IN DIMS BUT MISSING IN EXP:\n";
      for(auto dims_it : operands_dimensions_map)
      {
         const std::pair<std::vector<llvm::Instruction*>, llvm::Use*>& dim_key = dims_it.first;
         if(operands_expandability_map.count(dim_key) == 0)
         {
            llvm::errs() << "  Use:  " << get_val_string(dim_key.second->get()) << "\n";
            llvm::errs() << "  User: " << get_val_string(dim_key.second->getUser()) << "\n";
            llvm::errs() << "  Call trace:  ";
            for(llvm::Instruction* c : dim_key.first)
            {
               llvm::errs() << " " << c << "=" << (c ? llvm::CallSite(c).getCalledFunction()->getName() : "KERNEL") << "  ";
            }
            llvm::errs() << "\n\n";
         }
      }
      llvm::errs() << "IN EXP BUT MISSING IN DIMS:\n";
      for(auto exp_it : operands_expandability_map)
      {
         const std::pair<std::vector<llvm::Instruction*>, llvm::Use*>& exp_key = exp_it.first;
         if(operands_dimensions_map.count(exp_key) == 0)
         {
            llvm::errs() << "  Use:  " << get_val_string(exp_key.second->get()) << "\n";
            llvm::errs() << "  User: " << get_val_string(exp_key.second->getUser()) << "\n";
            llvm::errs() << "  Call trace:  ";
            for(llvm::Instruction* c : exp_key.first)
            {
               llvm::errs() << " " << c << "=" << (c ? llvm::CallSite(c).getCalledFunction()->getName() : "KERNEL") << "  ";
            }
            llvm::errs() << "\n\n";
         }
      }
      exit(-1);
   }

   auto op_exp_it = operands_expandability_map.begin();
   auto op_dim_it = operands_dimensions_map.begin();
   auto op_exp_it_end = operands_expandability_map.end();
   auto op_dim_it_end = operands_dimensions_map.end();

   for(; op_exp_it != op_exp_it_end or op_dim_it != op_dim_it_end; ++op_exp_it, ++op_dim_it)
   {
      if(op_exp_it->first != op_dim_it->first)
      {
         llvm::errs() << "ERR wrong iter\n";
         exit(-1);
      }

      const std::pair<std::vector<llvm::Instruction*>, llvm::Use*>& op_key = op_exp_it->first;
      const std::vector<llvm::Instruction*>& call_trace = op_key.first;
      auto call_user = llvm::dyn_cast<llvm::Instruction>(op_key.second->getUser());
      unsigned long long operand_no = op_key.second->getOperandNo();
      bool expandability = op_exp_it->second.expandability;
      const std::vector<unsigned long long>& dimension = op_dim_it->second;

      std::vector<std::pair<bool, std::vector<unsigned long long>>>& call_exp_and_dim_vec = op_exp_and_dim_by_callsite.at(call_user).at(call_trace);

      call_exp_and_dim_vec.at(operand_no).first = expandability;
      call_exp_and_dim_vec.at(operand_no).second = dimension;
   }

   std::set<llvm::Function*> function_worklist;
   for(auto cc_it : compact_callgraph)
   {
      for(auto call_inst : cc_it.second)
      {
         function_worklist.insert(llvm::CallSite(call_inst).getCalledFunction());
      }
   }

   for(llvm::Function* function : function_worklist)
   {
      std::set<std::pair<std::vector<bool>, std::vector<std::vector<unsigned long long>>>> versions_set;

      for(llvm::User* user : function->users())
      {
         if(llvm::isa<llvm::CallInst>(user) || llvm::isa<llvm::InvokeInst>(user))
         {
            auto call_inst = llvm::dyn_cast<llvm::Instruction>(user);
            if(function_worklist.count(call_inst->getFunction()) > 0 or call_inst->getFunction() == kernel_function)
            {
               const std::map<std::vector<llvm::Instruction*>, std::vector<std::pair<bool, std::vector<unsigned long long>>>>& callsite_map = op_exp_and_dim_by_callsite.at(call_inst);

               for(const auto& callsite_map_it : callsite_map)
               {
                  const std::vector<std::pair<bool, std::vector<unsigned long long>>>& exp_dim_vec = callsite_map_it.second;

                  std::vector<bool> ops_exp;
                  std::vector<std::vector<unsigned long long>> ops_dim;

                  for(const std::pair<bool, std::vector<unsigned long long>>& pair : exp_dim_vec)
                  {
                     ops_exp.push_back(pair.first);
                     ops_dim.push_back(pair.second);
                  }

                  std::pair<std::vector<bool>, std::vector<std::vector<unsigned long long>>> vfun_key = std::make_pair(ops_exp, ops_dim);

                  versions_set.insert(vfun_key);
               }
            }
         }
         else
         {
            user->print(llvm::errs());
            llvm::errs() << "\nWarning: user not a Call\n";
         }
      }

      Expandability versioning_revenue = Expandability(true, 0.0, 0.0);
      for(const auto& op_it : operands_expandability_map)
      {
         const llvm::Use* use = op_it.first.second;
         const Expandability& profit = op_it.second;

         llvm::Function* called_function = llvm::CallSite(use->getUser()).getCalledFunction();
         if(called_function == function)
         {
            versioning_revenue.and_add(profit);
         }
      }

      unsigned long num_versions = versions_set.size() - 1;
      Expandability versioning_cost = compute_function_versioning_cost(function);
      versioning_cost *= num_versions;
      Expandability versioning_profit = versioning_revenue;
      versioning_profit -= versioning_cost;
      versioning_profit.expandability = true;
      llvm::dbgs() << "INFO: Estimated profit of " << versioning_profit.get_string() << " for versioning function " << function->getName() << " #" << num_versions << " times\n";

      versioning_profit.cast();
      bool is_expandable = (versioning_profit.expandability and !force_no_versioning) or num_versions <= 0;

      if(!is_expandable)
      {
         llvm::dbgs() << "INFO: Not versioning function " << function->getName() << " since not profitable\n";
      }

      versioning_cost_map[function] = versioning_profit.expandability;
   }
}

void perform_function_versioning(std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, llvm::Function* kernel_function, const std::map<llvm::Use*, llvm::Value*>& point_to_set_map,
                                 const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& operands_expandability_map, const std::map<llvm::AllocaInst*, Expandability>& allocas_expandability_map,
                                 const std::map<llvm::GlobalVariable*, Expandability>& globals_expandability_map, const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>>& operands_dimensions_map,
                                 std::map<llvm::Argument*, bool>& arg_exp_map, std::map<llvm::Argument*, std::vector<unsigned long long>>& arg_dims_map, std::set<llvm::Function*>& function_worklist_to_ret,
                                 const std::map<llvm::Function*, bool>& function_versioning_profitability)
{
   std::map<llvm::Instruction*, std::map<std::vector<llvm::Instruction*>, std::vector<std::pair<bool, std::vector<unsigned long long>>>>> op_exp_and_dim_by_callsite;

   {
      std::vector<llvm::Instruction*> call_trace;
      initialize_callsites(nullptr, call_trace, operands_expandability_map, operands_dimensions_map, compact_callgraph, op_exp_and_dim_by_callsite);
   }

   if(operands_expandability_map.size() != operands_dimensions_map.size())
   {
      llvm::errs() << "ERR Exp and dim maps with different sizes (" << operands_expandability_map.size() << " vs. " << operands_dimensions_map.size() << ")\n";
      llvm::errs() << "IN DIMS BUT MISSING IN EXP:\n";
      for(auto dims_it : operands_dimensions_map)
      {
         const std::pair<std::vector<llvm::Instruction*>, llvm::Use*>& dim_key = dims_it.first;
         if(operands_expandability_map.count(dim_key) == 0)
         {
            llvm::errs() << "  Use:  " << get_val_string(dim_key.second->get()) << "\n";
            llvm::errs() << "  User: " << get_val_string(dim_key.second->getUser()) << "\n";
            llvm::errs() << "  Call trace:  ";
            for(llvm::Instruction* c : dim_key.first)
            {
               llvm::errs() << " " << c << "=" << (c ? llvm::CallSite(c).getCalledFunction()->getName() : "KERNEL") << "  ";
            }
            llvm::errs() << "\n\n";
         }
      }
      llvm::errs() << "IN EXP BUT MISSING IN DIMS:\n";
      for(auto exp_it : operands_expandability_map)
      {
         const std::pair<std::vector<llvm::Instruction*>, llvm::Use*>& exp_key = exp_it.first;
         if(operands_dimensions_map.count(exp_key) == 0)
         {
            llvm::errs() << "  Use:  " << get_val_string(exp_key.second->get()) << "\n";
            llvm::errs() << "  User: " << get_val_string(exp_key.second->getUser()) << "\n";
            llvm::errs() << "  Call trace:  ";
            for(llvm::Instruction* c : exp_key.first)
            {
               llvm::errs() << " " << c << "=" << (c ? llvm::CallSite(c).getCalledFunction()->getName() : "KERNEL") << "  ";
            }
            llvm::errs() << "\n\n";
         }
      }
      exit(-1);
   }

   auto op_exp_it = operands_expandability_map.begin();
   auto op_dim_it = operands_dimensions_map.begin();
   auto op_exp_it_end = operands_expandability_map.end();
   auto op_dim_it_end = operands_dimensions_map.end();

   for(; op_exp_it != op_exp_it_end or op_dim_it != op_dim_it_end; ++op_exp_it, ++op_dim_it)
   {
      if(op_exp_it->first != op_dim_it->first)
      {
         llvm::errs() << "ERR wrong iter\n";
         exit(-1);
      }

      const std::pair<std::vector<llvm::Instruction*>, llvm::Use*>& op_key = op_exp_it->first;
      const std::vector<llvm::Instruction*>& call_trace = op_key.first;
      auto call_user = llvm::dyn_cast<llvm::Instruction>(op_key.second->getUser());
      unsigned long long operand_no = op_key.second->getOperandNo();
      bool expandability = op_exp_it->second.expandability;
      const std::vector<unsigned long long>& dimension = op_dim_it->second;

      std::vector<std::pair<bool, std::vector<unsigned long long>>>& call_exp_and_dim_vec = op_exp_and_dim_by_callsite.at(call_user).at(call_trace);

      call_exp_and_dim_vec.at(operand_no).first = expandability;
      call_exp_and_dim_vec.at(operand_no).second = dimension;
   }

   std::set<llvm::Function*> function_worklist;
   for(auto cc_it : compact_callgraph)
   {
      for(auto call_inst : cc_it.second)
      {
         function_worklist.insert(llvm::CallSite(call_inst).getCalledFunction());
      }
   }

   std::map<std::tuple<llvm::Function*, std::vector<bool>, std::vector<std::vector<unsigned long long>>>, llvm::Function*> versioned_function_map;

   for(llvm::Function* function : function_worklist)
   {
      bool versioning_is_profitable = function_versioning_profitability.at(function);
      std::map<std::tuple<llvm::Function*, std::vector<bool>, std::vector<std::vector<unsigned long long>>>, llvm::Function*> local_versioned_function_map;

      for(llvm::User* user : function->users())
      {
         if(llvm::isa<llvm::CallInst>(user) || llvm::isa<llvm::InvokeInst>(user))
         {
            auto call_inst = llvm::dyn_cast<llvm::Instruction>(user);
            if(function_worklist.count(call_inst->getFunction()) > 0 or call_inst->getFunction() == kernel_function)
            {
               const std::map<std::vector<llvm::Instruction*>, std::vector<std::pair<bool, std::vector<unsigned long long>>>>& callsite_map = op_exp_and_dim_by_callsite.at(call_inst);

               for(const auto& callsite_map_it : callsite_map)
               {
                  const std::vector<std::pair<bool, std::vector<unsigned long long>>>& exp_dim_vec = callsite_map_it.second;

                  std::vector<bool> ops_exp;
                  std::vector<std::vector<unsigned long long>> ops_dim;

                  for(const std::pair<bool, std::vector<unsigned long long>>& pair : exp_dim_vec)
                  {
                     if(versioning_is_profitable)
                     {
                        ops_exp.push_back(pair.first);
                        ops_dim.push_back(pair.second);
                     }
                     else
                     {
                        ops_exp.push_back(false);
                        ops_dim.push_back(std::vector<unsigned long long>());
                     }
                  }

                  std::tuple<llvm::Function*, std::vector<bool>, std::vector<std::vector<unsigned long long>>> vfun_key = std::make_tuple(function, ops_exp, ops_dim);

                  local_versioned_function_map.insert(std::make_pair(vfun_key, nullptr));
               }
            }
         }
         else
         {
            user->print(llvm::errs());
            llvm::dbgs() << "\nWarning: user not a Call\n";
         }
      }

      unsigned long long version_id = 0;
      for(auto ver_it : local_versioned_function_map)
      {
         const std::tuple<llvm::Function*, std::vector<bool>, std::vector<std::vector<unsigned long long>>>& vfun_key = ver_it.first;

         // Clone the function
         llvm::ValueToValueMapTy VMap;
         llvm::ClonedCodeInfo* code_info = new llvm::ClonedCodeInfo();
         llvm::Function* cloned_function = (local_versioned_function_map.size() == 1 ? function : llvm::CloneFunction(function, VMap, code_info));
         if(function->hasFnAttribute(llvm::Attribute::NoInline))
            cloned_function->addFnAttr(llvm::Attribute::NoInline);
         if(function->hasFnAttribute(llvm::Attribute::AlwaysInline))
            cloned_function->addFnAttr(llvm::Attribute::AlwaysInline);
         if(function->hasFnAttribute(llvm::Attribute::InlineHint))
            cloned_function->addFnAttr(llvm::Attribute::InlineHint);

         if(local_versioned_function_map.size() != 1)
         {
            std::string new_name = function->getName().str() + ".v" + std::to_string(version_id);
            cloned_function->setName(new_name);
         }

         if(!versioned_function_map.insert(std::make_pair(vfun_key, cloned_function)).second)
         {
            llvm::errs() << "ERR: Twin versioned function\n";
            exit(-1);
         }

         llvm::dbgs() << "INFO: Function " << function->getName() << " [";
         function->getFunctionType()->print(llvm::errs());
         llvm::dbgs() << "] (" << function << " ver #" << version_id << ") versioned as " << cloned_function->getName() << " with arguments \n";
         for(auto& arg : cloned_function->args())
         {
            llvm::Function* function;
            std::vector<bool> ops_exp_vec;
            std::vector<std::vector<unsigned long long>> ops_dim_vec;
            std::tie(function, ops_exp_vec, ops_dim_vec) = vfun_key;

            arg_exp_map.insert(std::make_pair(&arg, ops_exp_vec.at(arg.getArgNo())));
            arg_dims_map.insert(std::make_pair(&arg, ops_dim_vec.at(arg.getArgNo())));

            llvm::dbgs() << "   Arg" << arg.getArgNo() << ":  E( " << ops_exp_vec.at(arg.getArgNo()) << " )  D( ";
            for(unsigned long long d : ops_dim_vec.at(arg.getArgNo()))
            {
               llvm::dbgs() << d << " ";
            }
            llvm::dbgs() << ")\n";
         }

         ++version_id;
      }
   }

   std::set<llvm::Function*> versioned_function_worklist;
   std::set<llvm::Function*> fun_to_del;

   for(const auto& ver_it : versioned_function_map)
   {
      if(function_versioning_profitability.at(std::get<0>(ver_it.first)))
      {
         fun_to_del.insert(std::get<0>(ver_it.first));
      }
      versioned_function_worklist.insert(ver_it.second);
   }

   {
      std::vector<llvm::Instruction*> call_trace;
      std::set<std::tuple<llvm::Function*, std::vector<bool>, std::vector<std::vector<unsigned long long>>>> processed_versioned_functions;
      update_calls_rec(nullptr, kernel_function, call_trace, compact_callgraph, op_exp_and_dim_by_callsite, versioned_function_map, processed_versioned_functions, function_versioning_profitability);
   }

   function_worklist_to_ret.clear();
   function_worklist_to_ret = versioned_function_worklist;
   delete_functions_recursively(fun_to_del);
   compact_callgraph.clear();
}

bool check_function_versioning(const std::map<llvm::Instruction*, std::vector<llvm::Instruction*>>& compact_callgraph, llvm::Function* kernel_function,
                               const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability>& operands_expandability_map,
                               const std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>>& operands_dimensions_map, std::map<llvm::Argument*, bool>& arg_exp_map,
                               std::map<llvm::Argument*, std::vector<unsigned long long>>& arg_dims_map, const std::map<llvm::Function*, bool>& function_versioning_profitability)
{
   std::map<llvm::Instruction*, std::map<std::vector<llvm::Instruction*>, std::vector<std::pair<bool, std::vector<unsigned long long>>>>> op_exp_and_dim_by_callsite;

   {
      std::vector<llvm::Instruction*> call_trace;
      initialize_callsites(nullptr, call_trace, operands_expandability_map, operands_dimensions_map, compact_callgraph, op_exp_and_dim_by_callsite);
   }

   if(operands_expandability_map.size() != operands_dimensions_map.size())
   {
      llvm::errs() << "ERR Exp and dim maps with different sizes (" << operands_expandability_map.size() << " vs. " << operands_dimensions_map.size() << ")\n";
      llvm::errs() << "IN DIMS BUT MISSING IN EXP:\n";
      for(auto dims_it : operands_dimensions_map)
      {
         const std::pair<std::vector<llvm::Instruction*>, llvm::Use*>& dim_key = dims_it.first;
         if(operands_expandability_map.count(dim_key) == 0)
         {
            llvm::errs() << "  Use:  " << get_val_string(dim_key.second->get()) << "\n";
            llvm::errs() << "  User: " << get_val_string(dim_key.second->getUser()) << "\n";
            llvm::errs() << "  Call trace:  ";
            for(llvm::Instruction* c : dim_key.first)
            {
               llvm::errs() << " " << c << "=" << (c ? llvm::CallSite(c).getCalledFunction()->getName() : "KERNEL") << "  ";
            }
            llvm::errs() << "\n\n";
         }
      }
      llvm::errs() << "IN EXP BUT MISSING IN DIMS:\n";
      for(auto exp_it : operands_expandability_map)
      {
         const std::pair<std::vector<llvm::Instruction*>, llvm::Use*>& exp_key = exp_it.first;
         if(operands_dimensions_map.count(exp_key) == 0)
         {
            llvm::errs() << "  Use:  " << get_val_string(exp_key.second->get()) << "\n";
            llvm::errs() << "  User: " << get_val_string(exp_key.second->getUser()) << "\n";
            llvm::errs() << "  Call trace:  ";
            for(llvm::Instruction* c : exp_key.first)
            {
               llvm::errs() << " " << c << "=" << (c ? llvm::CallSite(c).getCalledFunction()->getName() : "KERNEL") << "  ";
            }
            llvm::errs() << "\n\n";
         }
      }
      exit(-1);
   }

   auto op_exp_it = operands_expandability_map.begin();
   auto op_dim_it = operands_dimensions_map.begin();
   auto op_exp_it_end = operands_expandability_map.end();
   auto op_dim_it_end = operands_dimensions_map.end();

   for(; op_exp_it != op_exp_it_end or op_dim_it != op_dim_it_end; ++op_exp_it, ++op_dim_it)
   {
      if(op_exp_it->first != op_dim_it->first)
      {
         llvm::errs() << "ERR: wrong iter\n";
         exit(-1);
      }

      const std::pair<std::vector<llvm::Instruction*>, llvm::Use*>& op_key = op_exp_it->first;
      const std::vector<llvm::Instruction*>& call_trace = op_key.first;
      llvm::Instruction* call_user = llvm::dyn_cast<llvm::Instruction>(op_key.second->getUser());
      unsigned long long operand_no = op_key.second->getOperandNo();
      bool expandability = op_exp_it->second.expandability;
      const std::vector<unsigned long long>& dimension = op_dim_it->second;

      std::vector<std::pair<bool, std::vector<unsigned long long>>>& call_exp_and_dim_vec = op_exp_and_dim_by_callsite.at(call_user).at(call_trace);

      call_exp_and_dim_vec.at(operand_no).first = expandability;
      call_exp_and_dim_vec.at(operand_no).second = dimension;
   }

   std::set<llvm::Function*> function_worklist;
   for(auto cc_it : compact_callgraph)
   {
      for(auto call_inst : cc_it.second)
      {
         function_worklist.insert(llvm::CallSite(call_inst).getCalledFunction());
      }
   }

   std::map<llvm::Function*, std::set<std::vector<std::pair<bool, std::vector<unsigned long long>>>>> check_args_by_fun;

   for(llvm::Function* function : function_worklist)
   {
      for(llvm::User* user : function->users())
      {
         if(llvm::isa<llvm::CallInst>(user) || llvm::isa<llvm::InvokeInst>(user))
         {
            auto call_inst = llvm::dyn_cast<llvm::Instruction>(user);
            if(function_worklist.count(call_inst->getFunction()) > 0 or call_inst->getFunction() == kernel_function)
            {
               const std::map<std::vector<llvm::Instruction*>, std::vector<std::pair<bool, std::vector<unsigned long long>>>>& callsite_map = op_exp_and_dim_by_callsite.at(call_inst);

               for(const auto& callsite_map_it : callsite_map)
               {
                  const std::vector<std::pair<bool, std::vector<unsigned long long>>>& exp_dim_vec = callsite_map_it.second;

                  check_args_by_fun[llvm::CallSite(call_inst).getCalledFunction()].insert(exp_dim_vec);
               }
            }
         }
         else
         {
            user->print(llvm::errs());
            llvm::errs() << "Warning user not a call\n";
            // exit(-1);
         }
      }
   }

   bool ret = true;

   for(const auto& check_it : check_args_by_fun)
   {
      llvm::Function* function = check_it.first;
      if(!function_versioning_profitability.at(function))
      {
         const std::vector<std::pair<bool, std::vector<unsigned long long>>>& op_vec = *check_it.second.begin();

         for(llvm::Argument& arg : function->args())
         {
            arg_exp_map.insert(std::make_pair(&arg, false));
            arg_dims_map.insert(std::make_pair(&arg, std::vector<unsigned long long>()));
         }
      }
      else if(check_it.second.size() != 1)
      {
         ret = false;

         llvm::dbgs() << "WAR: Function " << function->getName() << " not properly versioned\n";
         for(llvm::User* user : function->users())
         {
            if(llvm::isa<llvm::CallInst>(user) || llvm::isa<llvm::InvokeInst>(user))
            {
               auto call_inst = llvm::dyn_cast<llvm::Instruction>(user);
               if(function_worklist.count(call_inst->getFunction()) > 0 or call_inst->getFunction() == kernel_function)
               {
                  const std::map<std::vector<llvm::Instruction*>, std::vector<std::pair<bool, std::vector<unsigned long long>>>>& callsite_map = op_exp_and_dim_by_callsite.at(call_inst);

                  for(const auto& callsite_map_it : callsite_map)
                  {
                     const std::vector<std::pair<bool, std::vector<unsigned long long>>>& exp_dim_vec = callsite_map_it.second;

                     llvm::dbgs() << "   Call: " << call_inst << "  " << get_val_string(call_inst) << "\n";
                     llvm::dbgs() << "      Call trace:  ";
                     for(llvm::Instruction* c : callsite_map_it.first)
                     {
                        llvm::dbgs() << " " << c << "=" << (c ? llvm::CallSite(c).getCalledFunction()->getName() : "KERNEL") << "  ";
                     }
                     llvm::dbgs() << "\n";
                     unsigned long long idx = 0;
                     for(const auto& exp_it : exp_dim_vec)
                     {
                        llvm::dbgs() << "      Op" << idx++ << ":  E( " << exp_it.first << " )  D( ";
                        for(unsigned long long d : exp_it.second)
                        {
                           llvm::dbgs() << d << " ";
                        }
                        llvm::dbgs() << ")\n";
                     }
                  }
               }
            }
            else
            {
               llvm::errs() << "ERR user not a call\n";
               exit(-1);
            }
         }
      }
      else
      {
         const std::vector<std::pair<bool, std::vector<unsigned long long>>>& op_vec = *check_it.second.begin();

         for(llvm::Argument& arg : function->args())
         {
            arg_exp_map.insert(std::make_pair(&arg, op_vec.at(arg.getArgNo()).first));
            arg_dims_map.insert(std::make_pair(&arg, op_vec.at(arg.getArgNo()).second));
         }
      }
   }

   if(!ret)
   {
      arg_exp_map.clear();
      arg_dims_map.clear();
   }

   return ret;
}

void propagate_constant_arguments(const std::set<llvm::Function*>& function_worklist)
{
   std::map<llvm::Argument*, llvm::Value*> args_to_propagate;
   for(llvm::Function* function : function_worklist)
   {
      for(llvm::Argument& arg : function->args())
      {
         if(arg.getType()->isIntegerTy())
         {
            bool can_propagate = true;
            llvm::Value* op_arg = nullptr;

            for(llvm::User* user : function->users())
            {
               if(!can_propagate)
                  break;
               if(llvm::isa<llvm::CallInst>(user->stripPointerCasts()) || llvm::isa<llvm::InvokeInst>(user->stripPointerCasts()))
               {
                  auto call_inst = llvm::dyn_cast<llvm::Instruction>(user->stripPointerCasts());
                  llvm::Value* op = call_inst->getOperand(arg.getArgNo());
                  if(!llvm::isa<llvm::ConstantInt>(op))
                  {
                     can_propagate = false;
                     break;
                  }
                  if(op_arg == nullptr)
                  {
                     op_arg = op;
                  }
                  else
                  {
                     if(op_arg != op)
                     {
                        can_propagate = false;
                        break;
                     }
                     else
                     {
                        // Nothing to do
                     }
                  }
               }
               else
               {
                  can_propagate = false;
                  break;
               }
            }

            if(can_propagate and op_arg != nullptr)
            {
               args_to_propagate.insert(std::make_pair(&arg, op_arg));
            }
         }
      }
   }

   for(std::pair<llvm::Argument*, llvm::Value*> arg_pair : args_to_propagate)
   {
      arg_pair.first->replaceAllUsesWith(arg_pair.second);
   }
}

void expand_alloca(llvm::AllocaInst* alloca_inst, const llvm::DataLayout& DL, std::map<llvm::AllocaInst*, std::vector<llvm::AllocaInst*>>& exp_allocas_map)
{
   if(llvm::StructType* str_ty = llvm::dyn_cast<llvm::StructType>(alloca_inst->getAllocatedType()))
   {
      for(unsigned idx = 0; idx < str_ty->getStructNumElements(); ++idx)
      {
         llvm::Type* element = str_ty->getStructElementType(idx);

         std::string new_alloca_name = alloca_inst->getName().str() + "." + std::to_string(idx);
         llvm::AllocaInst* new_alloca_inst = new llvm::AllocaInst(element,
#if __clang_major__ > 4
                                                                  DL.getAllocaAddrSpace(),
#endif
                                                                  new_alloca_name, alloca_inst);

         // new_alloca_inst->setAlignment(DL.getTypeSizeInBits(new_alloca_inst->getAllocatedType())/8);
         exp_allocas_map[alloca_inst].push_back(new_alloca_inst);

         expand_alloca(new_alloca_inst, DL, exp_allocas_map);
      }
   }
   else if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(alloca_inst->getAllocatedType()))
   {
      for(unsigned idx = 0; idx < arr_ty->getArrayNumElements(); ++idx)
      {
         llvm::Type* element_ty = arr_ty->getArrayElementType();

         std::string new_alloca_name = alloca_inst->getName().str() + "." + std::to_string(idx);
         llvm::AllocaInst* new_alloca_inst = new llvm::AllocaInst(element_ty,
#if __clang_major__ > 4
                                                                  DL.getAllocaAddrSpace(),
#endif
                                                                  new_alloca_name, alloca_inst);

         // new_alloca_inst->setAlignment(DL.getTypeSizeInBits(new_alloca_inst->getAllocatedType())/8);
         exp_allocas_map[alloca_inst].push_back(new_alloca_inst);

         expand_alloca(new_alloca_inst, DL, exp_allocas_map);
      }
   }
}

void expand_allocas(const std::set<llvm::Function*>& function_worklist, const llvm::DataLayout& DL, std::map<llvm::AllocaInst*, std::vector<llvm::AllocaInst*>>& exp_allocas_map, const std::map<llvm::AllocaInst*, Expandability>& allocas_expandability_map)
{
   for(llvm::Function* function : function_worklist)
   {
      // Go through all the instructions looking for allocas
      for(auto& bb : *function)
      {
         // Global alloca set so to avoid loops in analysis
         std::vector<llvm::AllocaInst*> alloca_vec = std::vector<llvm::AllocaInst*>();

         for(auto& i : bb)
         {
            if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(&i))
            {
               if(alloca_inst->getAllocatedType()->isAggregateType())
               {
                  auto objType = alloca_inst->getAllocatedType();
                  TotalAllocaBytes += DL.getTypeAllocSize(objType);
               }

               if(allocas_expandability_map.at(alloca_inst).expandability)
               {
                  alloca_vec.push_back(alloca_inst);

                  llvm::dbgs() << "INFO: expanding alloca " << get_val_string(alloca_inst) << "\n";
                  DisaggregatedAllocaBytes += DL.getTypeAllocSize(alloca_inst->getAllocatedType());
               }
               else
               {
                  if(alloca_inst->getAllocatedType()->isAggregateType())
                  {
                     llvm::dbgs() << "INFO: not expanding alloca " << get_val_string(alloca_inst) << "\n";
                  }
               }
            }
         }

         for(llvm::AllocaInst* alloca_inst : alloca_vec)
         {
            if(alloca_inst->getAllocatedType()->isAggregateType())
            {
               expand_alloca(alloca_inst, DL, exp_allocas_map);
            }
         }
      }
   }
}

void expand_globals(llvm::Module* module, const llvm::DataLayout& DL, std::map<llvm::GlobalVariable*, std::vector<llvm::GlobalVariable*>>& exp_globals_map, const std::map<llvm::GlobalVariable*, Expandability>& globals_expandability_map)
{
   // Global vector containing globals to be expanded yet
   std::vector<llvm::GlobalVariable*> globals_to_exp = std::vector<llvm::GlobalVariable*>();

   for(llvm::GlobalVariable& g_var : module->globals())
   {
      if(g_var.getValueType()->isAggregateType())
      {
         TotalGlobalBytes += DL.getTypeAllocSize(g_var.getValueType());
      }

      if(globals_expandability_map.at(&g_var).expandability)
      {
         globals_to_exp.push_back(&g_var);

         llvm::dbgs() << "INFO: expanding global named " << g_var.getName() << "\n";
         DisaggregatedGlobalBytes += DL.getTypeAllocSize(g_var.getValueType());
      }
      else
      {
         if(g_var.getValueType()->isAggregateType())
         {
            llvm::dbgs() << "INFO: not expanding global named " << g_var.getName() << "\n";
         }
      }
   }

   for(unsigned long long g_idx = 0; g_idx < globals_to_exp.size(); g_idx++)
   {
      llvm::GlobalVariable* g_var = globals_to_exp.at(g_idx);

      if(llvm::StructType* str_ty = llvm::dyn_cast<llvm::StructType>(g_var->getType()->getPointerElementType()))
      {
         for(unsigned e_idx = 0; e_idx < str_ty->getStructNumElements(); ++e_idx)
         {
            llvm::Type* element = str_ty->getStructElementType(e_idx);

            std::string new_global_name = g_var->getName().str() + "." + std::to_string(e_idx);

            llvm::Constant* initializer = nullptr;
            if(llvm::Constant* c_el = llvm::dyn_cast<llvm::Constant>(g_var->getInitializer()->getAggregateElement(e_idx)))
            {
               initializer = c_el;
            }
            llvm::GlobalVariable* new_g_var = new llvm::GlobalVariable(*g_var->getParent(), element, g_var->isConstant(), g_var->getLinkage(), initializer, new_global_name, g_var);

            new_g_var->copyAttributesFrom(g_var);
            // new_g_var->setAlignment(DL.getTypeSizeInBits(new_g_var->getType()->getPointerElementType())/8);

            exp_globals_map[g_var].push_back(new_g_var);
            globals_to_exp.insert(globals_to_exp.begin() + g_idx + 1, new_g_var);
         }
      }
      else if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(g_var->getType()->getPointerElementType()))
      {
         for(unsigned e_idx = 0; e_idx < arr_ty->getArrayNumElements(); ++e_idx)
         {
            llvm::Type* element = arr_ty->getArrayElementType();

            std::string new_global_name = g_var->getName().str() + "." + std::to_string(e_idx);

            llvm::Constant* initializer = nullptr;
            if(llvm::Constant* c_el = llvm::dyn_cast<llvm::Constant>(g_var->getInitializer()->getAggregateElement(e_idx)))
            {
               initializer = c_el;
            }
            llvm::GlobalVariable* new_g_var = new llvm::GlobalVariable(*g_var->getParent(), element, g_var->isConstant(), g_var->getLinkage(), initializer, new_global_name, g_var);
            new_g_var->copyAttributesFrom(g_var);
            // new_g_var->setAlignment(DL.getTypeSizeInBits(new_g_var->getType()->getPointerElementType())/8);

            exp_globals_map[g_var].push_back(new_g_var);
            globals_to_exp.insert(globals_to_exp.begin() + g_idx + 1, new_g_var);
         }
      }
   }
}

void expand_signatures_and_call_sites(std::set<llvm::Function*>& function_worklist, std::map<llvm::Function*, llvm::Function*>& exp_fun_map, std::map<llvm::Argument*, bool>& arg_expandability_map,
                                      std::map<llvm::Argument*, std::vector<unsigned long long>>& arg_dimensions_map, std::map<llvm::Argument*, std::vector<llvm::Argument*>>& expanded_arguments_map, std::set<llvm::Instruction*>& inst_to_remove,
                                      std::set<llvm::Function*>& fun_to_remove)
{
   struct ArgObj
   {
      unsigned long index = {0};
      llvm::Type* type = {nullptr};
      std::string arg_name;
      bool expandable;
      std::vector<unsigned long long> size;
   };

   class ExpArgs
   {
    public:
      static void exp_arg(llvm::Type* arg_type, unsigned long long arg_dim_decay, ArgObj* arg, std::map<ArgObj*, std::vector<ArgObj*>>& exp_args_map_ref, std::map<unsigned long, ArgObj>& newMockFunctionArgs,
                          std::vector<llvm::ConstantPointerNull*>& exp_ops)
      {
         if(llvm::PointerType* ptr_type = llvm::dyn_cast<llvm::PointerType>(arg_type))
         {
            if(arg_dim_decay > 0)
            {
               arg->size = std::vector<unsigned long long>(1, arg_dim_decay);
               arg->expandable = true;

               if(ptr_type->getElementType()->isArrayTy())
               {
                  std::vector<unsigned long long> array_dims = compute_array_dims(ptr_type->getElementType());
                  arg->size.insert(arg->size.end(), array_dims.begin(), array_dims.end());
               }

               for(int e_idx = 0; e_idx < arg_dim_decay; ++e_idx)
               {
                  llvm::PointerType* new_arg_type = ptr_type;
                  if(new_arg_type->getPointerElementType()->isArrayTy())
                  {
                     new_arg_type = llvm::PointerType::getUnqual(new_arg_type->getPointerElementType()->getArrayElementType());
                  }
                  std::string exp_arg_name = arg->arg_name + "." + std::to_string(e_idx);
                  unsigned long long argNo = newMockFunctionArgs.size();
                  ArgObj* new_arg = &newMockFunctionArgs[argNo];
                  new_arg->index = argNo;
                  new_arg->arg_name = exp_arg_name;
                  new_arg->type = new_arg_type;

                  exp_args_map_ref[arg].push_back(new_arg);

                  llvm::ConstantPointerNull* null_ptr = llvm::ConstantPointerNull::get(new_arg_type);
                  exp_ops.push_back(null_ptr);

                  exp_arg(ptr_type, 0, new_arg, exp_args_map_ref, newMockFunctionArgs, exp_ops);
               }
            }
            else if(llvm::StructType* str_ty = llvm::dyn_cast<llvm::StructType>(ptr_type->getElementType()))
            {
               arg->size = std::vector<unsigned long long>();
               arg->expandable = true;

               for(unsigned long long e_idx = 0; e_idx != str_ty->getStructNumElements(); ++e_idx)
               {
                  llvm::Type* str_el_type = str_ty->getStructElementType(e_idx);
                  llvm::Type* new_arg_type = llvm::PointerType::getUnqual((str_el_type->isArrayTy() ? str_el_type->getArrayElementType() : str_el_type));

                  std::string new_arg_name = arg->arg_name + "." + std::to_string(e_idx);
                  unsigned long long argNo = newMockFunctionArgs.size();
                  ArgObj* new_arg = &newMockFunctionArgs[argNo];
                  new_arg->index = argNo;
                  new_arg->type = new_arg_type;
                  new_arg->arg_name = new_arg_name;

                  exp_args_map_ref[arg].push_back(new_arg);

                  llvm::ConstantPointerNull* null_ptr = llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(new_arg_type));
                  exp_ops.push_back(null_ptr);

                  llvm::Type* rec_type = llvm::PointerType::getUnqual(str_el_type);
                  exp_arg(rec_type, 0, new_arg, exp_args_map_ref, newMockFunctionArgs, exp_ops);
               }
            }
            else if(llvm::ArrayType* arr_type = llvm::dyn_cast<llvm::ArrayType>(ptr_type->getElementType()))
            {
               arg->size = compute_array_dims(arr_type);
               arg->expandable = true;

               for(unsigned long long e_idx = 0; e_idx != arr_type->getArrayNumElements(); ++e_idx)
               {
                  llvm::Type* new_arg_type = llvm::PointerType::getUnqual((arr_type->getArrayElementType()->isArrayTy() ? arr_type->getArrayElementType()->getArrayElementType() : arr_type->getArrayElementType()));
                  std::string new_arg_name = arg->arg_name + "." + std::to_string(e_idx);
                  unsigned long long argNo = newMockFunctionArgs.size();
                  ArgObj* new_arg = &newMockFunctionArgs[argNo];
                  new_arg->index = argNo;
                  new_arg->type = new_arg_type;
                  new_arg->arg_name = new_arg_name;
                  new_arg->expandable = arg->expandable;

                  exp_args_map_ref[arg].push_back(new_arg);

                  llvm::ConstantPointerNull* null_ptr = llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(new_arg_type));
                  exp_ops.push_back(null_ptr);

                  llvm::Type* rec_type = llvm::PointerType::getUnqual(arr_type->getArrayElementType());
                  exp_arg(rec_type, 0, new_arg, exp_args_map_ref, newMockFunctionArgs, exp_ops);
               }
            }
            else
            {
               arg->expandable = false;
               arg->size.clear();
            }
         }
      }

    private:
      static std::vector<unsigned long long> compute_array_dims(llvm::Type* array_type)
      {
         if(array_type->isArrayTy())
         {
            llvm::Type* rec_ty = array_type;
            std::vector<unsigned long long> tmp_sizes;
            do
            {
               if(llvm::ArrayType* arr_ty = llvm::dyn_cast<llvm::ArrayType>(rec_ty))
               {
                  tmp_sizes.push_back(arr_ty->getArrayNumElements());
                  rec_ty = arr_ty->getArrayElementType();
               }
               else if(rec_ty->isStructTy())
               {
                  // tmp_sizes.push_back(1);
                  break;
               }
               else
               {
                  break;
               }
            } while(true);
            return tmp_sizes;
         }
         else
         {
            return std::vector<unsigned long long>();
         }
      }
   };

   for(llvm::Function* called_function : function_worklist)
   {
      std::vector<llvm::Type*> new_arg_ty_vec = std::vector<llvm::Type*>();
      std::map<unsigned long long, llvm::Argument*> idxs_of_expanded_args;
      std::map<ArgObj*, std::vector<ArgObj*>> mock_exp_args_map;
      std::map<llvm::Argument*, std::vector<llvm::ConstantPointerNull*>> exp_ops_map;

      llvm::Function::arg_iterator arg_it_b = called_function->arg_begin();
      llvm::Function::arg_iterator arg_it_e = called_function->arg_end();

      std::map<unsigned long, ArgObj> newMockFunctionArgs;

      // Go through all the function arguments
      for(auto arg_it = arg_it_b; arg_it != arg_it_e; arg_it++)
      {
         llvm::Argument* arg = &*arg_it;

         bool arg_expandability = arg_expandability_map.at(arg);
         const std::vector<unsigned long long>& arg_dimensions = arg_dimensions_map.at(arg);

         llvm::Type* new_arg_ty = arg->getType();
         std::string new_arg_name = arg->getName();

         // Create the new argument and append it to the mock function
         unsigned long long argNo = newMockFunctionArgs.size();
         ArgObj* new_arg = &newMockFunctionArgs[argNo];
         new_arg->index = argNo;
         new_arg->type = new_arg_ty;
         new_arg->arg_name = new_arg_name;

         idxs_of_expanded_args[new_arg->index] = arg;

         std::vector<llvm::ConstantPointerNull*> exp_ops;
         if(arg_expandability)
         {
            ExpArgs::exp_arg(new_arg_ty, (arg_dimensions.empty() ? 0 : arg_dimensions.front()), new_arg, mock_exp_args_map, newMockFunctionArgs, exp_ops);
         }
         exp_ops_map.insert(std::make_pair(arg, exp_ops));

         if(arg->getType()->isPointerTy())
         {
            unsigned long long arg_size = arg->getParent()->getParent()->getDataLayout().getTypeAllocSize(arg->getType()->getPointerElementType());
            if(!arg_dimensions_map.at(arg).empty())
            {
               arg_size *= arg_dimensions_map.at(arg).front();
            }
            TotalOperandBytes += arg_size;
            if(arg_expandability)
            {
               DisaggregatedOperandBytes += arg_size;
            }
         }
      }

      for(auto& a : newMockFunctionArgs)
      {
         new_arg_ty_vec.push_back(a.second.type);
      }

      // Keep the same return type
      llvm::Type* new_return_type = called_function->getFunctionType()->getReturnType();

      llvm::FunctionType* new_function_type = llvm::FunctionType::get(new_return_type, new_arg_ty_vec, false);
      llvm::GlobalValue::LinkageTypes linkage = called_function->getLinkage();
      std::string new_function_name = called_function->getName().str() + ".C";

      // Create function prototype
      llvm::Function* expanded_function = llvm::Function::Create(new_function_type, linkage, new_function_name, called_function->getParent());
      if(called_function->hasFnAttribute(llvm::Attribute::NoInline))
         expanded_function->addFnAttr(llvm::Attribute::NoInline);
      if(called_function->hasFnAttribute(llvm::Attribute::AlwaysInline))
         expanded_function->addFnAttr(llvm::Attribute::AlwaysInline);
      if(called_function->hasFnAttribute(llvm::Attribute::InlineHint))
         expanded_function->addFnAttr(llvm::Attribute::InlineHint);

      llvm::ValueToValueMapTy VMap;

      std::map<ArgObj*, llvm::Argument*> mock_to_new_arg_map;
      llvm::Function::arg_iterator nf_arg_it_b = expanded_function->arg_begin();
      llvm::Function::arg_iterator nf_arg_it_e = expanded_function->arg_end();
      llvm::Function::arg_iterator nf_arg_it = nf_arg_it_b;
      unsigned long mock_arg_index = 0;
      for(; nf_arg_it != nf_arg_it_e; nf_arg_it++, ++mock_arg_index)
      {
         llvm::Argument* nf_arg = &*nf_arg_it;
         ArgObj* mf_arg = &newMockFunctionArgs.at(mock_arg_index);

         mock_to_new_arg_map[mf_arg] = nf_arg;

         auto idx_it = idxs_of_expanded_args.find(nf_arg->getArgNo());
         if(idx_it != idxs_of_expanded_args.end())
         {
            llvm::Argument* arg = idx_it->second;
            VMap[arg] = nf_arg;
         }

         nf_arg->setName(mf_arg->arg_name);

         // Assign the size to the new argument if the mock argument had it
         arg_expandability_map.insert(std::make_pair(nf_arg, mf_arg->expandable));
         arg_dimensions_map.insert(std::make_pair(nf_arg, mf_arg->size));
      }

      for(auto& ma1 : mock_exp_args_map)
      {
         auto mf1_arg = ma1.first;
         auto nf1_arg = mock_to_new_arg_map[mf1_arg];

         for(auto& ma2 : ma1.second)
         {
            auto mf2_arg = ma2;
            auto nf2_arg = mock_to_new_arg_map[mf2_arg];

            expanded_arguments_map[nf1_arg].push_back(nf2_arg);
         }
      }

      // Clone the function
      llvm::SmallVector<llvm::ReturnInst*, 8> returns;
      llvm::CloneFunctionInto(expanded_function, called_function, VMap, true, returns);
      fun_to_remove.insert(called_function);

      // Track the function mapping (old->new)
      exp_fun_map[called_function] = expanded_function;

      llvm::dbgs() << "INFO: Function " << called_function->getName() << " [";
      called_function->getFunctionType()->print(llvm::errs());
      llvm::dbgs() << "] expanded as " << expanded_function->getName() << " with arguments \n";
      for(llvm::Argument& arg : expanded_function->args())
      {
         llvm::dbgs() << "   Arg" << arg.getArgNo() << ":  E( " << arg_expandability_map.at(&arg) << " )  D( ";
         for(unsigned long long d : arg_dimensions_map.at(&arg))
         {
            llvm::dbgs() << d << " ";
         }
         llvm::dbgs() << ")    " << get_val_string(&arg) << "\n";
      }

      std::set<llvm::Instruction*> calls_to_remove;
      for(llvm::Use& use : called_function->uses())
      {
         llvm::User* user = use.getUser();

         llvm::CallInst* user_call_inst = llvm::dyn_cast<llvm::CallInst>(user);
         llvm::InvokeInst* user_invoke_inst = llvm::dyn_cast<llvm::InvokeInst>(user);

         if(user_call_inst != nullptr || user_invoke_inst != nullptr)
         {
            llvm::Instruction* call_inst = llvm::dyn_cast<llvm::Instruction>(user);

            // Recursively populate the operand vector, expanding with null pointers
            std::vector<llvm::Value*> new_call_ops = std::vector<llvm::Value*>();

            for(auto& op : (user_call_inst != nullptr ? user_call_inst->arg_operands() : user_invoke_inst->arg_operands()))
            {
               llvm::Value* operand = op.get();

               llvm::Argument* arg = &*std::next(called_function->arg_begin(), op.getOperandNo());

               new_call_ops.push_back(operand);

               const std::vector<llvm::ConstantPointerNull*>& null_ptr_exp_vec = exp_ops_map.at(arg);

               new_call_ops.insert(new_call_ops.end(), null_ptr_exp_vec.begin(), null_ptr_exp_vec.end());
            }

            // Build the new call site
            std::string new_call_name = call_inst->getName().str() + ".C";
            auto new_call_inst = llvm::CallInst::Create(expanded_function, new_call_ops, (call_inst->hasName() ? new_call_name : ""), call_inst);

            // Replace the old one
            new_call_inst->takeName(call_inst);
            call_inst->replaceAllUsesWith(new_call_inst);

            // Put all the pointer operands of the old call site to null
            auto nOperands = llvm::isa<llvm::CallInst>(call_inst) ? llvm::dyn_cast<llvm::CallInst>(call_inst)->getNumArgOperands() : llvm::dyn_cast<llvm::InvokeInst>(call_inst)->getNumArgOperands();
            for(unsigned short idx = 0; idx < nOperands; ++idx)
            {
               if(call_inst->getOperand(idx)->getType()->isPointerTy())
               {
                  if(llvm::isa<llvm::CallInst>(user))
                     llvm::dyn_cast<llvm::CallInst>(call_inst)->setArgOperand(idx, llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(call_inst->getOperand(idx)->getType())));
                  else
                     llvm::dyn_cast<llvm::InvokeInst>(call_inst)->setArgOperand(idx, llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(call_inst->getOperand(idx)->getType())));
               }
            }

            calls_to_remove.insert(call_inst);
         }
      }

      for(llvm::Instruction* inst : calls_to_remove)
      {
         inst->eraseFromParent();
      }
   }

   std::set<llvm::Function*> support_function_worklist = function_worklist;
   function_worklist.clear();

   for(llvm::Function* function : support_function_worklist)
   {
      auto exp_it = exp_fun_map.find(function);

      if(exp_it != exp_fun_map.end())
      {
         function_worklist.insert(exp_it->second);
      }
      else
      {
         function_worklist.insert(function);
      }
   }
}

void update_allocas_expandability_map(std::map<llvm::AllocaInst*, Expandability>& allocas_expandability_map, const std::map<llvm::Function*, llvm::Function*>& exp_fun_map)
{
   std::map<llvm::AllocaInst*, Expandability> support_allocas_expandability_map = allocas_expandability_map;
   allocas_expandability_map.clear();

   for(auto alloca_it : support_allocas_expandability_map)
   {
      llvm::AllocaInst* alloca_inst = alloca_it.first;
      Expandability expandability = alloca_it.second;

      auto bb_iter = alloca_inst->getParent()->getIterator();
      auto alloca_iter = alloca_inst->getIterator();

      signed long long bb_dist = std::distance(alloca_inst->getFunction()->begin(), bb_iter);
      signed long long alloca_dist = std::distance(alloca_inst->getParent()->begin(), alloca_iter);

      llvm::Function* expanded_function = alloca_inst->getFunction();
      auto exp_it = exp_fun_map.find(alloca_inst->getFunction());
      if(exp_it != exp_fun_map.end())
      {
         expanded_function = exp_it->second;
      }

      auto new_bb_iter = std::next(expanded_function->begin(), bb_dist);
      auto new_alloca_iter = std::next(new_bb_iter->begin(), alloca_dist);

      if(llvm::AllocaInst* new_alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(&*new_alloca_iter))
      {
         allocas_expandability_map.insert(std::make_pair(new_alloca_inst, expandability));
      }
      else
      {
         llvm::errs() << "ERR\n";
         exit(-1);
      }
   }
}

bool compute_base_and_idxs(llvm::Use* ptr_use, llvm::Value*& base_address, std::vector<std::pair<llvm::Type*, llvm::Value*>>& idx_chain, std::vector<llvm::Instruction*>& inst_chain)
{
   if(llvm::BitCastOperator* bitcast_op = llvm::dyn_cast<llvm::BitCastOperator>(ptr_use->get()))
   {
      if(llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(ptr_use->getUser()))
      {
         inst_chain.push_back(inst);
      }
      // Recursively go through the gepi chain up to the base address
      compute_base_and_idxs(&bitcast_op->getOperandUse(0), base_address, idx_chain, inst_chain);
      return false;
   }
   else if(llvm::GEPOperator* gep_op = llvm::dyn_cast<llvm::GEPOperator>(ptr_use->get()))
   {
      llvm::Instruction* containing_inst = nullptr;

      if(llvm::GetElementPtrInst* gep_inst = llvm::dyn_cast<llvm::GetElementPtrInst>(ptr_use->get()))
      {
         containing_inst = gep_inst;
         inst_chain.push_back(gep_inst);
      }
      else
      {
         if(llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(ptr_use->getUser()))
         {
            containing_inst = inst;
         }
         else
         {
            llvm::errs() << "ERR: User not an inst " << get_val_string(ptr_use->get()) << " " << get_val_string(ptr_use->getUser()) << "\n";
            exit(-1);
         }
      }

      // Recursively go through the gepi chain up to the base address
      bool ret = compute_base_and_idxs(&gep_op->getOperandUse(gep_op->getPointerOperandIndex()), base_address, idx_chain, inst_chain);

      if(ret)
      {
         llvm::gep_type_iterator gti_begin = llvm::gep_type_begin(gep_op);
         llvm::gep_type_iterator gti_end = llvm::gep_type_end(gep_op);

         if(idx_chain.empty())
         {
            llvm::errs() << "ERR: Empty chain";
            exit(-1);
         }
         llvm::Value*& last_chain_idx = idx_chain.back().second;
         llvm::Value* first_gepi_idx = gti_begin.getOperand();

         llvm::ConstantInt* last_chain_idx_c = llvm::dyn_cast<llvm::ConstantInt>(last_chain_idx);
         llvm::ConstantInt* first_gepi_idx_c = llvm::dyn_cast<llvm::ConstantInt>(first_gepi_idx);

         if(last_chain_idx_c != nullptr and first_gepi_idx_c != nullptr)
         {
            unsigned long long idx_sum = last_chain_idx_c->getSExtValue() + first_gepi_idx_c->getSExtValue();
            last_chain_idx = llvm::ConstantInt::get(first_gepi_idx_c->getType(), idx_sum);
         }
         else
         {
            llvm::Instruction* user_inst = llvm::dyn_cast<llvm::Instruction>(ptr_use->getUser());
            if(last_chain_idx->getType()->isIntegerTy() and first_gepi_idx->getType()->isIntegerTy())
            {
               if(last_chain_idx->getType()->getIntegerBitWidth() != first_gepi_idx->getType()->getIntegerBitWidth())
               {
                  if(last_chain_idx->getType()->getIntegerBitWidth() < first_gepi_idx->getType()->getIntegerBitWidth())
                  {
                     std::string sext_name = gep_op->getName().str() + ".sext";
                     last_chain_idx = llvm::SExtInst::Create(llvm::Instruction::CastOps::SExt, last_chain_idx, first_gepi_idx->getType(), sext_name, user_inst);
                  }
                  else
                  {
                     std::string trunc_name = gep_op->getName().str() + ".trunc";
                     last_chain_idx = llvm::TruncInst::Create(llvm::Instruction::CastOps::Trunc, last_chain_idx, first_gepi_idx->getType(), trunc_name, user_inst);
                  }
               }
            }
            else
            {
               llvm::errs() << "ERR: not integer idx\n";
               exit(-1);
            }
            std::string add_name = gep_op->getName().str() + ".add";
            last_chain_idx = llvm::BinaryOperator::Create(llvm::Instruction::BinaryOps::Add, last_chain_idx, first_gepi_idx, add_name, user_inst);
         }

         ++gti_begin;

         for(llvm::gep_type_iterator gti = gti_begin; gti != gti_end; ++gti)
         {
            idx_chain.push_back(std::make_pair(gti.getIndexedType(), gti.getOperand()));
         }
      }

      return ret;
   }
   else if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(ptr_use->get()))
   {
      // The alloca becomes the base address
      base_address = alloca_inst;
      llvm::ConstantInt* zero_ci = llvm::ConstantInt::get(llvm::Type::getInt32Ty(alloca_inst->getContext()), 0);
      idx_chain.push_back(std::make_pair(alloca_inst->getAllocatedType(), zero_ci));

      return true;
   }
   else if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(ptr_use->get()))
   {
      // The global variable becomes the base address
      base_address = g_var;
      llvm::ConstantInt* zero_ci = llvm::ConstantInt::get(llvm::Type::getInt32Ty(g_var->getContext()), 0);
      idx_chain.push_back(std::make_pair(g_var->getType()->getPointerElementType(), zero_ci));

      return true;
   }
   else if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(ptr_use->get()))
   {
      // The argument becomes the base address
      base_address = arg;
      llvm::ConstantInt* zero_ci = llvm::ConstantInt::get(llvm::Type::getInt32Ty(arg->getContext()), 0);
      idx_chain.push_back(std::make_pair(arg->getType()->getPointerElementType(), zero_ci));
      idx_chain.push_back(std::make_pair(arg->getType()->getPointerElementType(), zero_ci));

      return true;
   }
   else
   {
      return false;
   }

   return false;
}

template <class I>
llvm::Value* get_element_at_idx(I* base_address, const std::map<I*, std::vector<I*>>& map, std::vector<std::pair<llvm::Type*, llvm::Value*>> idx_chain)
{
   if(idx_chain.empty())
   {
      return base_address;
   }
   else
   {
      llvm::Type* idx_type = idx_chain.front().first;
      llvm::Value* idx_value = idx_chain.front().second;

      auto exp_it = map.find(base_address);
      if(exp_it != map.end())
      {
         const std::vector<I*>& subelements = exp_it->second;
         signed long long idx = llvm::dyn_cast<llvm::ConstantInt>(idx_value)->getSExtValue();

         I* subelement = subelements.at(idx);

         idx_chain.erase(idx_chain.begin());
         return get_element_at_idx(subelement, map, idx_chain);
      }
      else
      {
         llvm::errs() << "ERR: cannot find expansion for idx #" << llvm::dyn_cast<llvm::ConstantInt>(idx_value)->getSExtValue() << "\n";
         llvm::errs() << "Base: " << get_val_string(base_address) << "\n";
         llvm::errs() << "Idxs: ";
         for(auto idx : idx_chain)
         {
            llvm::errs() << " " << llvm::dyn_cast<llvm::ConstantInt>(idx.second)->getSExtValue();
         }
         exit(-1);
      }
   }

   return nullptr;
}

static void gen_gepi_map(llvm::Value* gepi_base, llvm::Argument* arg, llvm::Use* use, std::map<llvm::Value*, std::vector<llvm::Value*>>& gepi_map, const std::map<llvm::Argument*, std::vector<llvm::Argument*>>& arg_map,
                         const std::map<llvm::Argument*, std::vector<unsigned long long>>& arg_size_map, std::string gepi_name)
{
   auto arg_it = arg_map.find(arg);

   if(arg_it != arg_map.end())
   {
      std::vector<llvm::Type*> type_vec;

      auto size_it = arg_size_map.find(llvm::dyn_cast<llvm::Argument>(gepi_base));
      bool already_decay = false;
      if(size_it != arg_size_map.end())
      {
         if(!size_it->second.empty())
         {
            if(size_it->second.front() > 1)
            {
               for(unsigned long long idx = 0; idx < size_it->second.front(); idx++)
               {
                  already_decay = true;
                  type_vec.push_back(gepi_base->getType()->getPointerElementType());
               }
            }
         }
      }

      if(!already_decay)
      {
         llvm::Type* ptd_ty;
         if(gepi_base->getType()->isPointerTy())
         {
            ptd_ty = gepi_base->getType()->getPointerElementType();
         }
         else
         {
            ptd_ty = gepi_base->getType();
         }

         if(ptd_ty->isAggregateType())
         {
            if(ptd_ty->isArrayTy())
            {
               for(unsigned long long idx = 0; idx < ptd_ty->getArrayNumElements(); idx++)
               {
                  type_vec.push_back(ptd_ty->getArrayElementType());
               }
            }
            else if(ptd_ty->isStructTy())
            {
               for(unsigned long long idx = 0; idx < ptd_ty->getStructNumElements(); idx++)
               {
                  type_vec.push_back(ptd_ty->getStructElementType(idx)); // TODO check it out
               }
            }
            else if(ptd_ty->isVectorTy())
            {
               for(unsigned long long idx = 0; idx < ptd_ty->getVectorNumElements(); idx++)
               {
                  type_vec.push_back(ptd_ty->getVectorElementType());
               }
            }
            else
            {
               llvm::errs() << "ERR: Unknown aggregate\n";
               exit(-1);
            }
         }
      }

      unsigned long long idx = 0;
      for(llvm::Type* ty : type_vec)
      {
         std::vector<llvm::Value*> gepi_ops = std::vector<llvm::Value*>();

         llvm::Type* gepi_type = llvm::cast<llvm::PointerType>(gepi_base->getType()->getScalarType())->getElementType();

         if(!already_decay)
         {
            llvm::Type* op1_ty = llvm::IntegerType::get(gepi_base->getContext(), 32);
            llvm::Constant* op1 = llvm::ConstantInt::get(op1_ty, 0, false);
            gepi_ops.push_back(op1);
         }

         llvm::Type* op2_ty = llvm::IntegerType::get(gepi_base->getContext(), 32);
         llvm::Constant* op2 = llvm::ConstantInt::get(op2_ty, idx, false);
         gepi_ops.push_back(op2);

         std::string new_gepi_name = gepi_name + "." + std::to_string(idx);

         llvm::GetElementPtrInst* new_gepi = llvm::GetElementPtrInst::CreateInBounds(gepi_type, gepi_base, gepi_ops, new_gepi_name, llvm::dyn_cast<llvm::Instruction>(use->getUser()));
         gepi_map[gepi_base].push_back(new_gepi);
         gen_gepi_map(new_gepi, arg, use, gepi_map, arg_map, arg_size_map, new_gepi_name);

         ++idx;
      }
   }
   else
   {
      // llvm::errs() << "ERR: arg not found\n";
      // arg->dump();
      // exit(-1);
   }
}

llvm::Value* get_expanded_value_from_expandable_base(const std::map<llvm::Argument*, std::vector<llvm::Argument*>>& exp_args_map, const std::map<llvm::AllocaInst*, std::vector<llvm::AllocaInst*>>& exp_allocas_map,
                                                     const std::map<llvm::GlobalVariable*, std::vector<llvm::GlobalVariable*>>& exp_globals_map, llvm::Value* base_address, const std::vector<std::pair<llvm::Type*, llvm::Value*>>& idx_chain)
{
   if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(base_address))
   {
      return get_element_at_idx(alloca_inst, exp_allocas_map, idx_chain);
   }
   else if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(base_address))
   {
      return get_element_at_idx(arg, exp_args_map, idx_chain);
   }
   else if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(base_address))
   {
      return get_element_at_idx(g_var, exp_globals_map, idx_chain);
   }
   else
   {
      llvm::errs() << "ERR: Neither alloca, argument, nor global as base address " << get_val_string(base_address) << "\n";
      exit(-1);
   }
}

template <class T>
void expand_types(T* ptr, const std::map<T*, std::vector<T*>>& exp_map_ref, std::vector<std::pair<llvm::Type*, std::vector<unsigned long long>>>& exp_types_ref, std::vector<unsigned long long> idx_chain)
{
   auto exp_it = exp_map_ref.find(ptr);

   // Recursively iterate if the element has been expanded, push the type otherwise
   if(exp_it != exp_map_ref.end() and !exp_it->second.empty())
   {
      const std::vector<T*>& exp_vec_ref = exp_it->second;

      idx_chain.push_back(0);
      for(T* exp_el : exp_vec_ref)
      {
         expand_types(exp_el, exp_map_ref, exp_types_ref, idx_chain);
         idx_chain.back()++;
      }
   }
   else
   {
      exp_types_ref.push_back(std::make_pair(ptr->getType()->getPointerElementType(), idx_chain));
   }
}

// unsigned long long expansions = 0;

void process_single_pointer(llvm::Use* ptr_u, llvm::BasicBlock*& new_bb, std::set<llvm::Instruction*>& inst_to_remove, const std::map<llvm::Argument*, std::vector<llvm::Argument*>>& exp_args_map,
                            const std::map<llvm::AllocaInst*, std::vector<llvm::AllocaInst*>>& exp_allocas_map, const std::map<llvm::GlobalVariable*, std::vector<llvm::GlobalVariable*>>& exp_globals_map,
                            const std::map<llvm::Argument*, std::vector<unsigned long long>>& arg_dimensions_map, const llvm::DataLayout& DL, bool should_be_constant)
{
   llvm::Value* base_address = nullptr;

   if(llvm::isa<llvm::ConstantPointerNull>(ptr_u->get()))
   {
      return;
   }

   if(llvm::Instruction* user_inst = llvm::dyn_cast<llvm::Instruction>(ptr_u->getUser()))
   {
      std::vector<std::pair<llvm::Type*, llvm::Value*>> idx_chain;
      std::vector<llvm::Instruction*> inst_chain;

      bool has_expandable_base = compute_base_and_idxs(ptr_u, base_address, idx_chain, inst_chain);

      if(base_address != nullptr)
      {
         if(llvm::Argument* arg_base = llvm::dyn_cast<llvm::Argument>(base_address))
         {
            has_expandable_base = exp_args_map.count(arg_base) > 0;
         }
         else if(llvm::AllocaInst* alloca_base = llvm::dyn_cast<llvm::AllocaInst>(base_address))
         {
            has_expandable_base = exp_allocas_map.count(alloca_base) > 0;
         }
         else if(llvm::GlobalVariable* global_base = llvm::dyn_cast<llvm::GlobalVariable>(base_address))
         {
            has_expandable_base = exp_globals_map.count(global_base) > 0;
         }
      }

      if(has_expandable_base)
      {
         idx_chain.erase(idx_chain.begin());

         if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(base_address))
         {
            auto size_it = arg_dimensions_map.find(arg);

            if(size_it != arg_dimensions_map.end() and !size_it->second.empty() and size_it->second.front() > 0)
            {
               // Nothing to do
            }
            else
            {
               idx_chain.erase(idx_chain.begin());
            }
         }
         /*
               if(llvm::isa<llvm::CallInst>(ptr_u->getUser()) || llvm::isa<llvm::InvokeInst>(ptr_u->getUser()))
               {
                  llvm::Instruction* call_inst = llvm::dyn_cast<llvm::Instruction>(ptr_u->getUser());

                  llvm::Argument* arg = &*std::next(llvm::CallSite(call_inst).getCalledFunction()->arg_begin(), ptr_u->getOperandNo());

                  auto size_it = arg_dimensions_map.find(arg);
                  if(size_it != arg_dimensions_map.end() and !size_it->second.empty() and size_it->second.front() > 0)
                  {
                     // Nothing to do
                  }
                  else
                  {
                     if(llvm::ConstantInt* last_idx_c = llvm::dyn_cast<llvm::ConstantInt>(idx_chain.back().second))
                     {
                        if(last_idx_c->getSExtValue() != 0)
                        {
                           llvm::errs() << "ERR: Wrong decay\n";
                           exit(-1);
                        }
                     }
                     else
                     {
                        llvm::errs() << "ERR: Wrong decay\n";
                        exit(-1);
                     }
                     idx_chain.pop_back();
                  }
               }
      */
         llvm::dbgs() << "\nINFO: In function " << user_inst->getFunction()->getName() << " expanding " << get_val_string(ptr_u->getUser()) << "\n";
         llvm::dbgs() << "   Base address: " << get_val_string(base_address) << "\n";
         if(idx_chain.empty())
         {
            llvm::dbgs() << "   Empty idx chain\n";
         }
         else
         {
            llvm::dbgs() << "   Idx chain:\n";
            for(auto const& idx : idx_chain)
            {
               llvm::dbgs() << "     Idx type: " << get_ty_string(idx.first) << "\n";
               llvm::dbgs() << "     Val: " << get_val_string(idx.second) << "\n";
            }
         }
         if(inst_chain.empty())
         {
            llvm::dbgs() << "   Empty inst chain\n";
         }
         else
         {
            llvm::dbgs() << "   Inst chain:\n";
            for(auto const& idx : inst_chain)
            {
               llvm::dbgs() << "   " << get_val_string(idx) << "\n";
            }
         }

         for(llvm::Instruction* i : inst_chain)
         {
            if(has_expandable_base)
            {
               inst_to_remove.insert(i);
            }
         }

         bool is_constant = true;

         // Check whether the offset is constant
         for(const std::pair<llvm::Type*, llvm::Value*>& idx : idx_chain)
         {
            if(!llvm::isa<llvm::ConstantInt>(idx.second))
            {
               is_constant = false;
               break;
            }
         }

         if(is_constant)
         {
            llvm::Value* exp_val = get_expanded_value_from_expandable_base(exp_args_map, exp_allocas_map, exp_globals_map, base_address, idx_chain);
            llvm::dbgs() << "   expanded as " << get_val_string(exp_val) << "\n";
            ptr_u->set(exp_val);
         }
         else
         {
            if(should_be_constant)
            {
               llvm::errs() << "ERR Constant access needed in op # " << ptr_u->getOperandNo() << " in " << get_val_string(ptr_u->getUser()) << "\n";
               exit(-1);
            }

            std::vector<std::pair<llvm::Type*, std::vector<unsigned long long>>> expanded_types;
            if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(base_address))
            {
               std::vector<unsigned long long> gepi_chain = std::vector<unsigned long long>();
               expand_types(alloca_inst, exp_allocas_map, expanded_types, gepi_chain);
            }
            else if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(base_address))
            {
               std::vector<unsigned long long> gepi_chain = std::vector<unsigned long long>();
               expand_types(arg, exp_args_map, expanded_types, gepi_chain);
            }
            else if(llvm::GlobalVariable* g_var = llvm::dyn_cast<llvm::GlobalVariable>(base_address))
            {
               std::vector<unsigned long long> gepi_chain = std::vector<unsigned long long>();
               expand_types(g_var, exp_globals_map, expanded_types, gepi_chain);
            }
            else
            {
               llvm::errs() << "ERR: Unknown base " << get_val_string(base_address);
               exit(-1);
            }

            class CountSublements
            {
             public:
               static unsigned long long count(llvm::Type* ty)
               {
                  unsigned long long counter = 0;
                  if(ty->isArrayTy())
                  {
                     counter += ty->getArrayNumElements() * count(ty->getArrayElementType());
                  }
                  else if(ty->isStructTy())
                  {
                     for(unsigned long long idx = 0; idx < ty->getStructNumElements(); ++idx)
                     {
                        counter += count(ty->getStructElementType(idx));
                     }
                  }
                  else
                  {
                     ++counter;
                  }

                  return counter;
               }
            };

            llvm::APInt zero_ai = llvm::APInt((unsigned int)32, 0, false);
            llvm::ConstantInt* zero_c = llvm::ConstantInt::get(base_address->getContext(), zero_ai);
            llvm::Value* idx_sum = zero_c;

            llvm::Type* base_type_rec = base_address->getType()->getPointerElementType();
            if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(base_address))
            {
               auto size_it = arg_dimensions_map.find(arg);

               if(size_it != arg_dimensions_map.end() and !size_it->second.empty() and size_it->second.front() > 1)
               {
                  base_type_rec = llvm::ArrayType::get(base_type_rec, size_it->second.front());
               }
            }

            std::string idx_name = "";
            for(const std::pair<llvm::Type*, llvm::Value*>& idx_pair : idx_chain)
            {
               llvm::Value* idx_val = idx_pair.second;

               if(base_type_rec->isStructTy())
               {
                  if(llvm::ConstantInt* idx_c = llvm::dyn_cast<llvm::ConstantInt>(idx_val))
                  {
                     idx_name += "." + std::to_string(idx_c->getSExtValue());

                     unsigned long long offset = 0;
                     for(unsigned long long i = 0; i < base_type_rec->getStructNumElements() and i < idx_c->getSExtValue(); ++i)
                     {
                        offset += CountSublements::count(base_type_rec->getStructElementType(i));
                     }

                     llvm::APInt offset_ap = llvm::APInt((unsigned int)32, offset, false);
                     llvm::ConstantInt* offset_c = llvm::ConstantInt::get(base_address->getContext(), offset_ap);
                     std::string add_name = ptr_u->getUser()->getName().str() + ".add" + idx_name;
                     idx_sum = llvm::BinaryOperator::Create(llvm::Instruction::BinaryOps::Add, idx_sum, offset_c, add_name, user_inst);

                     base_type_rec = base_type_rec->getStructElementType(idx_c->getSExtValue());
                  }
                  else
                  {
                     llvm::Type* ty = (base_type_rec->getStructNumElements() > 0 ? base_type_rec->getStructElementType(0) : nullptr);

                     for(unsigned long long i = 0; i < base_type_rec->getStructNumElements(); ++i)
                     {
                        if(ty != base_type_rec->getStructElementType(i))
                        {
                           ty = nullptr;
                        }
                     }

                     if(ty)
                     {
                        idx_name += ".x";
                        std::string mul_name = ptr_u->getUser()->getName().str() + ".mul" + idx_name;
                        std::string add_name = mul_name + ".add";
                        llvm::APInt sub_count_ap = llvm::APInt(32, CountSublements::count(ty), false);
                        llvm::ConstantInt* sub_count_ci = llvm::ConstantInt::get(base_address->getContext(), sub_count_ap);

                        if(idx_val->getType()->isIntegerTy(64))
                        {
                           llvm::Type* int32_ty = llvm::Type::getInt32Ty(user_inst->getContext());
                           std::string sext_name = mul_name + ".sext";
                           idx_val = llvm::CastInst::Create(llvm::Instruction::CastOps::Trunc, idx_val, int32_ty, sext_name, user_inst);
                        }
                        llvm::BinaryOperator* mul = llvm::BinaryOperator::Create(llvm::Instruction::BinaryOps::Mul, sub_count_ci, idx_val, mul_name, user_inst);
                        idx_sum = llvm::BinaryOperator::Create(llvm::Instruction::BinaryOps::Add, idx_sum, mul, add_name, user_inst);

                        base_type_rec = ty;
                     }
                     else
                     {
                        ptr_u->get()->dump();
                        ptr_u->getUser()->dump();
                        llvm::errs() << "ERR: Non constant idx on struct\n";
                        exit(-1);
                     }
                  }
               }
               else if(base_type_rec->isArrayTy())
               {
                  if(llvm::ConstantInt* idx_c = llvm::dyn_cast<llvm::ConstantInt>(idx_val))
                  {
                     idx_name += "." + std::to_string(idx_c->getSExtValue());
                  }
                  else
                  {
                     idx_name += ".x";
                  }

                  std::string mul_name = ptr_u->getUser()->getName().str() + ".mul" + idx_name;
                  std::string add_name = mul_name + ".add";
                  llvm::APInt sub_count_ap = llvm::APInt(32, CountSublements::count(base_type_rec->getArrayElementType()), false);
                  llvm::ConstantInt* sub_count_ci = llvm::ConstantInt::get(base_address->getContext(), sub_count_ap);

                  if(idx_val->getType()->isIntegerTy(64))
                  {
                     llvm::Type* int32_ty = llvm::Type::getInt32Ty(user_inst->getContext());
                     std::string sext_name = mul_name + ".sext";
                     idx_val = llvm::CastInst::Create(llvm::Instruction::CastOps::Trunc, idx_val, int32_ty, sext_name, user_inst);
                  }
                  llvm::BinaryOperator* mul = llvm::BinaryOperator::Create(llvm::Instruction::BinaryOps::Mul, sub_count_ci, idx_val, mul_name, user_inst);
                  idx_sum = llvm::BinaryOperator::Create(llvm::Instruction::BinaryOps::Add, idx_sum, mul, add_name, user_inst);

                  base_type_rec = base_type_rec->getArrayElementType();
               }
            }

            llvm::Function* wrapper_function = nullptr;
            llvm::Instruction* wrapper_call = nullptr;
            llvm::ReturnInst* ret = nullptr;
            bool isStore = false;

            // unsigned long long type_count = 0;
            llvm::Type* return_type = nullptr;
            llvm::Type* ptr_type = nullptr;
            std::vector<llvm::Type*> param_tys;
            llvm::Type* offset_ty = idx_sum->getType();
            std::vector<llvm::Value*> args;
            if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(user_inst))
            {
               param_tys.push_back(store_inst->getValueOperand()->getType());
               args.push_back(store_inst->getValueOperand());
            }
            param_tys.push_back(offset_ty);
            args.push_back(idx_sum);

            if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(user_inst))
            {
               return_type = load_inst->getPointerOperand()->getType()->getPointerElementType();
               ptr_type = load_inst->getPointerOperand()->getType();
            }
            else if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(user_inst))
            {
               return_type = llvm::Type::getVoidTy(store_inst->getContext());
               ptr_type = store_inst->getPointerOperand()->getType();
               isStore = true;
            }

            for(const std::pair<llvm::Type*, std::vector<unsigned long long>>& exp_ty_pair : expanded_types)
            {
               // llvm::Type *bitcast_type = nullptr;
               llvm::Type* ty1 = exp_ty_pair.first;
               llvm::Type* ty2 = ptr_u->get()->getType()->getPointerElementType();
               if(ty1 == ty2)
               {
                  //++type_count;
                  param_tys.push_back(ptr_type);
                  args.push_back(llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(ptr_type)));
               }
            }

            llvm::FunctionType* function_ty = llvm::FunctionType::get(return_type, param_tys, false);
            wrapper_function = llvm::Function::Create(function_ty, llvm::GlobalValue::LinkageTypes::InternalLinkage, wrapper_function_name, user_inst->getModule());
            wrapper_function->addFnAttr(llvm::Attribute::NoInline);
            wrapper_function->addFnAttr(llvm::Attribute::OptimizeNone);

            llvm::BasicBlock* bb = llvm::BasicBlock::Create(user_inst->getContext(), "", wrapper_function);
            if(return_type->isVoidTy())
            {
               ret = llvm::ReturnInst::Create(user_inst->getContext(), bb);
            }
            else
            {
               ret = llvm::ReturnInst::Create(user_inst->getContext(), llvm::UndefValue::get(return_type), bb);
            }
            llvm::Instruction* new_inst = user_inst->clone();
            new_inst->insertBefore(ret);

            wrapper_call = llvm::CallInst::Create(wrapper_function, args, (return_type->isVoidTy() ? "" : "wrapper_call"), user_inst);

            if(!return_type->isVoidTy())
            {
               user_inst->replaceAllUsesWith(wrapper_call);
            }
            inst_to_remove.insert(user_inst);

            user_inst = new_inst;

            // Keep track on where to split
            llvm::Instruction* split_before = user_inst;

            unsigned long long bytes_acc = 0;

            llvm::PHINode* last_phi_set = nullptr;

            // const llvm::DataLayout* DL = &user_inst->getModule()->getDataLayout();

            llvm::Function::arg_iterator arg_it = wrapper_function->arg_begin();
            llvm::Instruction::op_iterator op_it = wrapper_call->op_begin();
            llvm::Function::arg_iterator arg_offset_it = wrapper_function->arg_begin();
            llvm::Function::arg_iterator arg_value_it = wrapper_function->arg_begin();
            if(wrapper_function != nullptr)
            {
               if(isStore)
               {
                  arg_it->setName("value");
                  arg_value_it = arg_it;
                  ++arg_it;
                  ++op_it;
               }
               arg_it->setName("offset");
               arg_offset_it = arg_it;
               ++arg_it;
               ++op_it;
            }
            // Create the if-then-else chain
            unsigned long long type_idx = 0;
            for(const std::pair<llvm::Type*, std::vector<unsigned long long>>& exp_ty_pair : expanded_types)
            {
               llvm::Type* exp_ty = exp_ty_pair.first;
               std::vector<unsigned long long> gepi_chain = exp_ty_pair.second;

               llvm::Value* curr_idx = &*arg_offset_it;

               llvm::APInt type_idx_ai = llvm::APInt(32, type_idx++, false);
               llvm::Type* ty1 = exp_ty;
               llvm::Type* ty2 = ptr_u->get()->getType()->getPointerElementType();
               if(ty1 != ty2)
               {
                  continue;
               }

               if(gepi_chain.size() == idx_chain.size())
               {
                  bool different_chain = false;
                  for(unsigned long long i = 0; i < gepi_chain.size(); ++i)
                  {
                     if(llvm::ConstantInt* idx_c = llvm::dyn_cast<llvm::ConstantInt>(idx_chain.at(i).second))
                     {
                        if((unsigned long long)idx_c->getSExtValue() != gepi_chain.at(i))
                        {
                           different_chain = true;
                           break;
                        }
                     }
                  }
                  if(different_chain)
                  {
                     continue;
                  }
               }

               llvm::ConstantInt* type_idx_ci = llvm::ConstantInt::get(base_address->getContext(), type_idx_ai);
               std::string cmp_name = ptr_u->getUser()->getName().str() + ".cmp." + std::to_string(0);

               llvm::CmpInst* cond = llvm::CmpInst::Create(llvm::CmpInst::OtherOps::ICmp, llvm::CmpInst::Predicate::ICMP_EQ, curr_idx, type_idx_ci, cmp_name, split_before);

#if __clang_major__ > 7
               llvm::Instruction* then_term;
               llvm::Instruction* else_term;
#else
               llvm::TerminatorInst* then_term;
               llvm::TerminatorInst* else_term;
#endif
               llvm::SplitBlockAndInsertIfThenElse(cond, split_before, &then_term, &else_term);
               then_term->getParent()->setName("csroa.if.then");
               else_term->getParent()->setName("csroa.if.else");
               split_before->getParent()->setName("csroa.if.end");

               if(new_bb == nullptr)
               {
                  new_bb = split_before->getParent();
               }

               llvm::Instruction* new_inst = user_inst->clone();
               new_inst->insertBefore(then_term);

               if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(ptr_u->getUser()))
               {
                  llvm::PHINode* phi_node = llvm::PHINode::Create(ptr_u->get()->getType()->getPointerElementType(), expanded_types.size(), ptr_u->getUser()->getName().str() + ".phi", split_before);

                  if(ret != nullptr)
                  {
                     if(!ret->getReturnValue()->getType()->isVoidTy())
                     {
                        if(llvm::isa<llvm::UndefValue>(ret->getReturnValue()))
                        {
                           ret->setOperand(0, phi_node);
                        }
                     }
                  }

                  phi_node->addIncoming(new_inst, then_term->getParent());
                  phi_node->addIncoming(llvm::UndefValue::get(new_inst->getType()), else_term->getParent());

                  if(last_phi_set)
                  {
                     last_phi_set->setIncomingValue(1, phi_node);
                  }
                  else
                  {
                     user_inst->replaceAllUsesWith(phi_node);
                  }

                  last_phi_set = phi_node;
               }

               std::vector<std::pair<llvm::Type*, llvm::Value*>> el_idx_chain;
               double idx_rec = type_idx_ai.getSExtValue();
               for(unsigned long long gepi_idx : gepi_chain)
               {
                  llvm::APInt gepi_idx_ap = llvm::APInt(32, gepi_idx);
                  llvm::ConstantInt* gepi_idx_ci = llvm::ConstantInt::get(user_inst->getContext(), gepi_idx_ap);
                  el_idx_chain.push_back(std::make_pair(nullptr, gepi_idx_ci));
               }
               llvm::Value* exp_val = get_expanded_value_from_expandable_base(exp_args_map, exp_allocas_map, exp_globals_map, base_address, el_idx_chain);

               std::string arg_name = "arg" + std::to_string(arg_it->getArgNo());
               arg_it->setName(arg_name);
               wrapper_call->setOperand(op_it->getOperandNo(), exp_val);
               if(isStore)
                  new_inst->setOperand(0, &*arg_value_it);

               new_inst->setOperand(ptr_u->getOperandNo(), &*arg_it);
               ++arg_it;
               ++op_it;

               split_before = else_term;
            }

            llvm::dbgs() << "   expanded as " << get_val_string(wrapper_call) << "\n";

            // TODO Fix it
            user_inst->replaceAllUsesWith(llvm::UndefValue::get(user_inst->getType()));
            user_inst->eraseFromParent();
         }
      }
   }
   else
   {
      llvm::errs() << "ERR: User not inst\n";
      exit(-1);
   }
}

void process_arg_pointer(llvm::Use* ptr_u, llvm::BasicBlock*& new_bb, std::set<llvm::Instruction*>& inst_to_remove, const std::map<llvm::Argument*, std::vector<llvm::Argument*>>& exp_args_map,
                         const std::map<llvm::AllocaInst*, std::vector<llvm::AllocaInst*>>& exp_allocas_map, const std::map<llvm::GlobalVariable*, std::vector<llvm::GlobalVariable*>>& exp_globals_map,
                         const std::map<llvm::Argument*, std::vector<unsigned long long>>& arg_dimensions_map, bool should_be_constant)
{
   llvm::Value* base_address = nullptr;

   if(llvm::isa<llvm::ConstantPointerNull>(ptr_u->get()))
   {
      return;
   }

   std::vector<std::pair<llvm::Type*, llvm::Value*>> idx_chain;
   std::vector<llvm::Instruction*> inst_chain;

   bool has_expandable_base = compute_base_and_idxs(ptr_u, base_address, idx_chain, inst_chain);

   if(base_address != nullptr)
   {
      if(llvm::Argument* arg_base = llvm::dyn_cast<llvm::Argument>(base_address))
      {
         has_expandable_base = exp_args_map.count(arg_base) > 0;
      }
      else if(llvm::AllocaInst* alloca_base = llvm::dyn_cast<llvm::AllocaInst>(base_address))
      {
         has_expandable_base = exp_allocas_map.count(alloca_base) > 0;
      }
      else if(llvm::GlobalVariable* global_base = llvm::dyn_cast<llvm::GlobalVariable>(base_address))
      {
         has_expandable_base = exp_globals_map.count(global_base) > 0;
      }
   }

   if(has_expandable_base)
   {
      for(llvm::Instruction* i : inst_chain)
      {
         inst_to_remove.insert(i);
      }
   }

   bool is_constant = true;

   // Check whether the offset is constant
   for(const std::pair<llvm::Type*, llvm::Value*>& idx : idx_chain)
   {
      if(!llvm::isa<llvm::ConstantInt>(idx.second))
      {
         is_constant = false;
         break;
      }
   }

   if(llvm::isa<llvm::CallInst>(ptr_u->getUser()) || llvm::isa<llvm::InvokeInst>(ptr_u->getUser()))
   {
      llvm::Instruction* call_inst = llvm::dyn_cast<llvm::Instruction>(ptr_u->getUser());

      llvm::Argument* arg_u = &*std::next(llvm::CallSite(call_inst).getCalledFunction()->arg_begin(), ptr_u->getOperandNo());

      if(is_constant)
      {
         llvm::dbgs() << "\nINFO: In function " << llvm::dyn_cast<llvm::Instruction>(ptr_u->getUser())->getFunction()->getName() << ", expanding operand #" << ptr_u->getOperandNo() << " of " << get_val_string(ptr_u->getUser()) << "\n";
         llvm::dbgs() << "   Base address: " << get_val_string(base_address) << "\n";
         if(idx_chain.empty())
         {
            llvm::dbgs() << "   Empty idx chain\n";
         }
         else
         {
            llvm::dbgs() << "   Idx chain:\n";
            for(auto const& idx : idx_chain)
            {
               llvm::dbgs() << "     Val: " << get_val_string(idx.second) << "(" << get_ty_string(idx.first) << ")"
                            << "\n";
            }
         }
         if(inst_chain.empty())
         {
            llvm::dbgs() << "   Empty inst chain\n";
         }
         else
         {
            llvm::dbgs() << "   Inst chain:\n";
            for(auto const& idx : inst_chain)
            {
               llvm::dbgs() << "   " << get_val_string(idx) << "\n";
            }
         }

         idx_chain.erase(idx_chain.begin());
         if(llvm::Argument* arg = llvm::dyn_cast<llvm::Argument>(base_address))
         {
            auto size_it = arg_dimensions_map.find(arg);

            if(size_it != arg_dimensions_map.end() and !size_it->second.empty() and size_it->second.front() > 0)
            {
               // Nothing to do
            }
            else
            {
               idx_chain.erase(idx_chain.begin());
            }
         }

         if(idx_chain.empty())
         {
            llvm::ConstantInt* zero_c = llvm::ConstantInt::get(base_address->getContext(), llvm::APInt(32, (unsigned long long)0));
            idx_chain.push_back(std::make_pair(arg_u->getType()->getPointerElementType(), zero_c));
         }
         else
         {
            // idx_chain.erase(idx_chain.begin());

            if(idx_chain.empty())
            {
               llvm::ConstantInt* zero_c = llvm::ConstantInt::get(base_address->getContext(), llvm::APInt(32, (unsigned long long)0));
               idx_chain.push_back(std::make_pair(arg_u->getType()->getPointerElementType(), zero_c));
            }
            else
            {
               if(llvm::isa<llvm::CallInst>(ptr_u->getUser()) || llvm::isa<llvm::InvokeInst>(ptr_u->getUser()))
               {
                  llvm::Instruction* call_inst = llvm::dyn_cast<llvm::Instruction>(ptr_u->getUser());

                  llvm::Argument* arg = &*std::next(llvm::CallSite(call_inst).getCalledFunction()->arg_begin(), ptr_u->getOperandNo());

                  auto size_it = arg_dimensions_map.find(arg);
                  if(size_it != arg_dimensions_map.end() and !size_it->second.empty() and size_it->second.front() > 0)
                  {
                     // Nothing to do
                  }
                  else
                  {
                     if(llvm::ConstantInt* last_idx_c = llvm::dyn_cast<llvm::ConstantInt>(idx_chain.back().second))
                     {
                        if(last_idx_c->getSExtValue() != 0)
                        {
                           llvm::errs() << "ERR: Wrong decay\n";
                           exit(-1);
                        }
                     }
                     else
                     {
                        llvm::errs() << "ERR: Wrong decay\n";
                        exit(-1);
                     }
                     if(idx_chain.size() > 1)
                     {
                        idx_chain.pop_back();
                     }
                  }
               }
            }
         }

         // llvm::Value *exp_base = get_expanded_value_from_expandable_base(exp_args_map, exp_allocas_map, exp_globals_map, base_address, idx_chain);
         // llvm::errs() << "Exp arg: "; exp_base->dump();

         auto exp_arg_it = exp_args_map.find(arg_u);

         if(exp_arg_it != exp_args_map.end())
         {
            unsigned short bitwidth = 32; // idx_chain.back().second->getType()->getIntegerBitWidth();
            signed long long exp_arg_u_idx = llvm::dyn_cast<llvm::ConstantInt>(idx_chain.back().second)->getSExtValue();
            for(llvm::Argument* exp_arg_u : exp_arg_it->second)
            {
               std::vector<std::pair<llvm::Type*, llvm::Value*>> exp_idx_chain = idx_chain;

               llvm::Value* exp_val = nullptr;
               if(has_expandable_base)
               {
                  llvm::ConstantInt* c_idx = llvm::ConstantInt::get(base_address->getContext(), llvm::APInt(bitwidth, (unsigned long long)exp_arg_u_idx));
                  exp_idx_chain.back() = std::make_pair(exp_arg_u->getType()->getPointerElementType(), c_idx);
                  exp_val = get_expanded_value_from_expandable_base(exp_args_map, exp_allocas_map, exp_globals_map, base_address, exp_idx_chain);
               }
               else
               {
                  exp_idx_chain.pop_back();

                  llvm::ConstantInt* c_idx = llvm::ConstantInt::get(base_address->getContext(), llvm::APInt(bitwidth, (unsigned long long)exp_arg_u_idx));
                  exp_idx_chain.push_back(std::make_pair(exp_arg_u->getType()->getPointerElementType(), c_idx));

                  std::string gepi_name = base_address->getName().str() + ".gepi";
                  std::vector<llvm::Value*> gepi_idxs;

                  gepi_idxs.push_back(llvm::ConstantInt::get(base_address->getContext(), llvm::APInt(32, (unsigned long long)0)));

                  for(const std::pair<llvm::Type*, llvm::Value*>& idx : exp_idx_chain)
                  {
                     gepi_name += "." + std::to_string(llvm::dyn_cast<llvm::ConstantInt>(idx.second)->getSExtValue());
                     gepi_idxs.push_back(idx.second);
                  }

                  exp_val = llvm::GetElementPtrInst::CreateInBounds(nullptr, base_address, gepi_idxs, gepi_name, call_inst);
               }

               // Take care of array decay now
               if(exp_val->getType()->getPointerElementType()->isArrayTy())
               {
                  if(exp_val->getType()->getPointerElementType()->getArrayElementType() == exp_arg_u->getType()->getPointerElementType())
                  {
                     if(!llvm::isa<llvm::Argument>(exp_val))
                     {
                        std::vector<llvm::Value*> gepi_ops = std::vector<llvm::Value*>();
                        llvm::Type* op1_ty = llvm::IntegerType::get(exp_arg_u->getContext(), 32);
                        llvm::Constant* op1 = llvm::ConstantInt::get(op1_ty, 0, false);
                        gepi_ops.push_back(op1);
                        llvm::Type* op2_ty = llvm::IntegerType::get(exp_arg_u->getContext(), 32);
                        llvm::Constant* op2 = llvm::ConstantInt::get(op2_ty, 0, false);
                        gepi_ops.push_back(op2);

                        std::string gepi_name = exp_val->getName().str() + ".decay";
                        llvm::Type* gepi_type = exp_val->getType()->getPointerElementType();

                        llvm::GetElementPtrInst* decay_gep_inst = llvm::GetElementPtrInst::CreateInBounds(gepi_type, exp_val, gepi_ops, gepi_name, call_inst);

                        exp_val = decay_gep_inst;
                     }
                  }
                  else
                  {
                     // llvm::errs() << "ERR: Malformed decay!\n";
                     // call_inst->dump();
                     // exp_arg_u->dump();
                     // exp_val->dump();
                     // exit(-1);
                     // TODO Review this
                  }
               }

               call_inst->setOperand(exp_arg_u->getArgNo(), exp_val);
               llvm::dbgs() << "   expanded as " << get_val_string(exp_val) << "\n";

               // arg_offset += accessed_size;
               exp_arg_u_idx++;
            }
         }
      }
      else
      {
         llvm::errs() << "ERR: Non constant access in function call operand " << get_val_string(ptr_u->get()) << " in " << get_val_string(call_inst) << "\n";
         exit(-1);
      }
   }
}

void expand_ptrs(const std::set<llvm::Function*> function_worklist, const std::map<llvm::Argument*, std::vector<llvm::Argument*>>& exp_args_map, const std::map<llvm::AllocaInst*, std::vector<llvm::AllocaInst*>>& exp_allocas_map,
                 const std::map<llvm::GlobalVariable*, std::vector<llvm::GlobalVariable*>>& exp_globals_map, const std::map<llvm::Argument*, bool>& arg_expandability_map, const std::map<llvm::Argument*, std::vector<unsigned long long>>& arg_dimensions_map,
                 std::set<llvm::Instruction*>& inst_to_remove, const llvm::DataLayout& DL)
{
   for(llvm::Function* f : function_worklist)
   {
      std::vector<llvm::BasicBlock*> bbs;

      for(llvm::BasicBlock& bb : *f)
      {
         bbs.push_back(&bb);
      }

      // for (auto bb_it = bbs.begin(); bb_it != bbs.end(); ++bb_it) {
      for(unsigned long long bb_idx = 0; bb_idx < bbs.size(); ++bb_idx)
      {
         llvm::BasicBlock* bb = bbs.at(bb_idx);

         for(llvm::Instruction& i : *bb)
         {
            if(inst_to_remove.count(&i) != 0)
            {
               continue;
            }

            llvm::BasicBlock* new_bb = nullptr;
            if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(&i))
            {
               process_single_pointer(&load_inst->getOperandUse(0), new_bb, inst_to_remove, exp_args_map, exp_allocas_map, exp_globals_map, arg_dimensions_map, DL, false);
            }
            else if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(&i))
            {
               process_single_pointer(&store_inst->getOperandUse(1), new_bb, inst_to_remove, exp_args_map, exp_allocas_map, exp_globals_map, arg_dimensions_map, DL, false);
            }
            else if(llvm::isa<llvm::CallInst>(&i) || llvm::isa<llvm::InvokeInst>(&i))
            {
               auto call_inst = llvm::dyn_cast<llvm::Instruction>(&i);
               if(is_relevant_call(call_inst))
               {
                  llvm::Function* called_function = llvm::CallSite(call_inst).getCalledFunction();

                  auto nOperands = llvm::isa<llvm::CallInst>(call_inst) ? llvm::dyn_cast<llvm::CallInst>(call_inst)->getNumArgOperands() : llvm::dyn_cast<llvm::InvokeInst>(call_inst)->getNumArgOperands();
                  for(unsigned op_i = 0u; op_i < nOperands; op_i++)
                  {
                     auto op_u = &(call_inst->getOperandUse(op_i));
                     llvm::Argument* arg = &*std::next(called_function->arg_begin(), op_i);

                     if(arg_expandability_map.at(arg) and exp_args_map.count(arg) > 0)
                     {
                        if(op_u->get()->getType()->isPointerTy())
                        {
                           process_arg_pointer(op_u, new_bb, inst_to_remove, exp_args_map, exp_allocas_map, exp_globals_map, arg_dimensions_map, true);
                        }
                     }
                     else
                     {
                        if(op_u->get()->getType()->isPointerTy())
                        {
                           process_single_pointer(op_u, new_bb, inst_to_remove, exp_args_map, exp_allocas_map, exp_globals_map, arg_dimensions_map, DL, true);
                        }
                     }
                  }
               }
            }

            if(new_bb != nullptr and false)
            {
               bbs.insert(bbs.begin() + bb_idx + 1, new_bb);
               break;
            }
         }
      }
   }
}

void cleanup(llvm::Module& module, const std::map<llvm::Function*, llvm::Function*>& exp_fun_map, const std::set<llvm::Function*>& function_worklist, const std::set<llvm::Instruction*>& inst_to_remove,
             const std::map<llvm::Argument*, std::vector<llvm::Argument*>>& exp_args_map, const std::map<llvm::GlobalVariable*, std::vector<llvm::GlobalVariable*>>& exp_globals_map,
             const std::map<llvm::AllocaInst*, std::vector<llvm::AllocaInst*>>& exp_allocas_map)
{
   // Map specifying the expanded arguments for each function
   // std::map<llvm::Function*, std::set<unsigned long long>> exp_idx_args_map;

   for(auto& i : inst_to_remove)
   {
      i->replaceAllUsesWith(llvm::UndefValue::get(i->getType()));
      i->eraseFromParent();
   }

   class CheckArg
   {
    private:
      // Recursively check whether the argument is recursively used by calls only
      static bool usedByArgsOnly(llvm::Argument* arg, std::set<llvm::Argument*> inspected_args)
      {
         if(inspected_args.count(arg) == 0)
         {
            inspected_args.insert(arg);

            for(auto& use : arg->uses())
            {
               if(llvm::isa<llvm::CallInst>(use.getUser()) || llvm::isa<llvm::InvokeInst>(use.getUser()))
               {
                  auto call_inst = llvm::dyn_cast<llvm::Instruction>(use.getUser());
                  llvm::Function::arg_iterator arg_it = llvm::CallSite(call_inst).getCalledFunction()->arg_begin();

                  for(unsigned long long i = 0; i < use.getOperandNo(); i++)
                  {
                     arg_it++;
                  }

                  if(!usedByArgsOnly(&*arg_it, inspected_args))
                  {
                     return false;
                  }
               }
               else
               {
                  return false;
               }
            }
         }

         return true;
      }

      // Recursively check whether the argument is recursively loaded only
      static bool loadedOnly(llvm::Argument* arg, std::set<llvm::Argument*> inspected_args)
      {
         if(inspected_args.count(arg) == 0)
         {
            inspected_args.insert(arg);

            if(llvm::PointerType* ptr_ty = llvm::dyn_cast<llvm::PointerType>(arg->getType()))
            {
               if(!ptr_ty->getElementType()->isAggregateType())
               {
                  for(auto& use : arg->uses())
                  {
                     if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(use.getUser()))
                     {
                        // nothing to do
                     }
                     else if(llvm::isa<llvm::CallInst>(use.getUser()) || llvm::isa<llvm::InvokeInst>(use.getUser()))
                     {
                        auto call_inst = llvm::dyn_cast<llvm::Instruction>(use.getUser());
                        bool is_wrapper = strncmp(llvm::CallSite(call_inst).getCalledFunction()->getName().str().c_str(), std::string(wrapper_function_name).c_str(), std::string(wrapper_function_name).size()) == 0;
                        if(is_wrapper)
                        {
                           return false;
                        }

                        llvm::Function::arg_iterator arg_it = llvm::CallSite(call_inst).getCalledFunction()->arg_begin();

                        for(unsigned long long i = 0; i < use.getOperandNo(); i++)
                        {
                           arg_it++;
                        }

                        if(!loadedOnly(&*arg_it, inspected_args))
                        {
                           return false;
                        }
                     }
                     else
                     {
                        return false;
                     }
                  }
               }
               else
               {
                  return false;
               }
            }
            else
            {
               return false;
            }
         }

         return true;
      }

    public:
      // Check whether the argument is used by one store only (and no calls)
      static bool storedOnce(llvm::Argument* arg)
      {
         if(llvm::PointerType* ptr_ty = llvm::dyn_cast<llvm::PointerType>(arg->getType()))
         {
            if(!ptr_ty->getElementType()->isAggregateType())
            {
               if(arg->getNumUses() == 1)
               {
                  llvm::Use& first_use = *arg->use_begin();

                  if(llvm::StoreInst* store_inst = llvm::dyn_cast<llvm::StoreInst>(first_use.getUser()))
                  {
                     return store_inst->getPointerOperand() == arg;
                  }
                  else
                  {
                     return false;
                  }
               }
               else
               {
                  return false;
               }
            }
            else
            {
               return false;
            }
         }
         else
         {
            return false;
         }

         return true;
      }

      static bool usedByArgsOnly_wrapper(llvm::Argument* arg)
      {
         return usedByArgsOnly(arg, std::set<llvm::Argument*>());
      }

      static bool loadedOnly_wrapper(llvm::Argument* arg)
      {
         return loadedOnly(arg, std::set<llvm::Argument*>());
      }
   };

   struct sate_cmp
   {
      bool operator()(const std::pair<llvm::Instruction*, unsigned long long>& lhs, const std::pair<llvm::Instruction*, unsigned long long>& rhs) const
      {
         if(lhs.first < rhs.first)
         {
            return true;
         }
         else if(lhs.first == rhs.first)
         {
            return lhs.second < rhs.second;
         }
         else
         {
            return false;
         }
      }
   };

   std::map<std::pair<llvm::Instruction*, unsigned long long>, llvm::Argument*, sate_cmp> arg_to_arg;

   std::set<llvm::Function*> erase_in_fixed;
   std::set<llvm::Function*> new_functions_set;
   for(llvm::Function* function : function_worklist)
   {
      // Create the new function type based on the recomputed parameters.
      std::vector<llvm::Type*> arg_tys;

      llvm::Argument* arg_stored_once = nullptr;
      if(function->getReturnType()->isVoidTy())
      {
         unsigned long long num_args_stored_once = 0;

         for(llvm::Argument& a : function->args())
         {
            llvm::Argument* arg = &a;

            if(exp_args_map.count(arg) == 0)
            {
               if(CheckArg::storedOnce(arg))
               {
                  ++num_args_stored_once;
                  arg_stored_once = arg;
               }
            }
         }

         if(num_args_stored_once > 1)
         {
            // arg_stored_once = nullptr;
         }
      }

      if(function->getBasicBlockList().size() != 1)
      {
         arg_stored_once = nullptr;
      }

      for(llvm::Argument& a : function->args())
      {
         if(exp_args_map.count(&a) == 0)
         {
            // If the argument is stored once
            if(arg_stored_once == &a)
            {
               for(unsigned short attr_idx = 0; attr_idx < llvm::Attribute::EndAttrKinds; attr_idx++)
               {
                  if(a.hasAttribute((llvm::Attribute::AttrKind)attr_idx))
                  {
                     // llvm::errs() << "Function: " << function->getName() << "\n  ";
                     // llvm::errs() << " " << attr_idx;
                     a.removeAttr((llvm::Attribute::AttrKind)attr_idx);
                  }
               }
               // TODO definitely improve this feature
            }
            else
            {
               // If it is not only used as argument recursively
               if(!CheckArg::usedByArgsOnly_wrapper(&a))
               {
                  // Check whether recursively loaded only
                  if(CheckArg::loadedOnly_wrapper(&a))
                  {
                     arg_tys.push_back(a.getType()->getPointerElementType());
                  }
                  else
                  {
                     arg_tys.push_back(a.getType());
                  }
               }
            }
         }
      }

      if(arg_stored_once != nullptr)
      {
         // Get the store instruction
         llvm::StoreInst* single_store_inst = llvm::dyn_cast<llvm::StoreInst>(arg_stored_once->use_begin()->getUser());

         for(llvm::BasicBlock& bb : *function)
         {
            if(llvm::ReturnInst* ret_inst = llvm::dyn_cast<llvm::ReturnInst>(bb.getTerminator()))
            {
               llvm::ReturnInst* new_ret_inst = llvm::ReturnInst::Create(ret_inst->getContext(), single_store_inst->getValueOperand(), &bb);
               ret_inst->eraseFromParent();
            }
         }

         single_store_inst->eraseFromParent();
      }

      llvm::Type* return_type = nullptr;
      if(arg_stored_once == nullptr)
      {
         return_type = function->getReturnType();
      }
      else
      {
         return_type = arg_stored_once->getType()->getPointerElementType();
      }
      llvm::FunctionType* new_fun_ty = llvm::FunctionType::get(return_type, arg_tys, function->isVarArg());

      std::string new_fun_name = function->getName().str() + ".c";
      llvm::Function* new_function = llvm::Function::Create(new_fun_ty, function->getLinkage(), new_fun_name, function->getParent());
      if(function->hasFnAttribute(llvm::Attribute::NoInline))
         new_function->addFnAttr(llvm::Attribute::NoInline);
      if(function->hasFnAttribute(llvm::Attribute::AlwaysInline))
         new_function->addFnAttr(llvm::Attribute::AlwaysInline);
      if(function->hasFnAttribute(llvm::Attribute::InlineHint))
         new_function->addFnAttr(llvm::Attribute::InlineHint);
      new_function->setComdat(function->getComdat());
      new_function->setCallingConv(function->getCallingConv());

      new_function->getBasicBlockList().splice(new_function->begin(), function->getBasicBlockList());

      new_functions_set.insert(new_function);

      std::set<llvm::Argument*> unused_args;
      std::set<llvm::Argument*> scalar_args;

      std::set<llvm::LoadInst*> loads_to_rem;
      unsigned int i = 0;
      for(llvm::Function::arg_iterator fun_arg_it = function->arg_begin(), new_fun_arg_it = new_function->arg_begin(); fun_arg_it != function->arg_end(); ++fun_arg_it, ++i)
      {
         if(exp_args_map.count(&*fun_arg_it) == 0)
         {
            // If the argument is stored once
            if(&*fun_arg_it == arg_stored_once)
            {
               unused_args.insert(&*fun_arg_it);
            }
            else
            {
               // If it is not only used as argument recursively
               if(!CheckArg::usedByArgsOnly_wrapper(&*fun_arg_it))
               {
                  // Check whether recursively loaded only
                  if(CheckArg::loadedOnly_wrapper(&*fun_arg_it))
                  {
                     for(auto& use : fun_arg_it->uses())
                     {
                        if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(use.getUser()))
                        {
                           load_inst->replaceAllUsesWith(&*new_fun_arg_it);
                           // load_inst->eraseFromParent();
                           loads_to_rem.insert(load_inst);
                        }
                        else if(llvm::isa<llvm::CallInst>(use.getUser()) || llvm::isa<llvm::InvokeInst>(use.getUser()))
                        {
                           llvm::Instruction* call_inst = llvm::dyn_cast<llvm::Instruction>(use.getUser());
                           arg_to_arg.insert(std::make_pair(std::make_pair(call_inst, use.getOperandNo()), &*new_fun_arg_it));
                        }
                        else
                        {
                           llvm::errs() << "ERR: User not load nor call\n";
                           exit(-1);
                        }
                     }

                     scalar_args.insert(&*fun_arg_it);
                     fun_arg_it->replaceAllUsesWith(llvm::UndefValue::get(fun_arg_it->getType()));

                     new_fun_arg_it->removeAttr(llvm::Attribute::ReadOnly);
                     new_fun_arg_it->removeAttr(llvm::Attribute::NoCapture);
                  }
                  else
                  {
                     fun_arg_it->replaceAllUsesWith(&*new_fun_arg_it);
                  }
                  new_fun_arg_it->takeName(&*fun_arg_it);
                  ++new_fun_arg_it;
               }
               else
               {
                  unused_args.insert(&*fun_arg_it);
                  fun_arg_it->replaceAllUsesWith(llvm::Constant::getNullValue(fun_arg_it->getType()));
               }
            }
         }
         else
         {
            fun_arg_it->replaceAllUsesWith(llvm::Constant::getNullValue(fun_arg_it->getType()));
         }
      }

      for(auto r_it = loads_to_rem.rbegin(); r_it != loads_to_rem.rend(); ++r_it)
      {
         llvm::LoadInst* load_inst = *r_it;
         load_inst->eraseFromParent();
      }

      std::vector<llvm::User*> fixed_users;

      for(auto use_it = function->use_begin(); use_it != function->use_end(); use_it++)
      {
         llvm::User* user = use_it->getUser();
         fixed_users.push_back(user);
      }

      for(auto& user : fixed_users)
      {
         if(llvm::isa<llvm::CallInst>(user) || llvm::isa<llvm::InvokeInst>(user))
         {
            auto call_inst = llvm::dyn_cast<llvm::Instruction>(user);
            std::vector<llvm::Value*> call_ops = std::vector<llvm::Value*>();
            for(auto& op : (llvm::isa<llvm::CallInst>(user) ? llvm::dyn_cast<llvm::CallInst>(call_inst)->arg_operands() : llvm::dyn_cast<llvm::InvokeInst>(call_inst)->arg_operands()))
            {
               llvm::Value* operand = op.get();

               llvm::Argument* arg = &*std::next(llvm::CallSite(call_inst).getCalledFunction()->arg_begin(), op.getOperandNo());

               if(exp_args_map.count(arg) == 0)
               {
                  if(unused_args.count(arg) == 0)
                  {
                     if(scalar_args.count(arg) == 0)
                     {
                        call_ops.push_back(operand);
                     }
                     else
                     {
                        auto to_exp_it = arg_to_arg.find(std::make_pair(call_inst, arg->getArgNo()));

                        if(to_exp_it == arg_to_arg.end())
                        {
                           std::string new_load_name = call_inst->getName().str() + ".load." + std::to_string(i);
                           llvm::LoadInst* load_inst = new llvm::LoadInst(operand, new_load_name, call_inst);

                           call_ops.push_back(load_inst);
                        }
                        else
                        {
                           call_ops.push_back(to_exp_it->second);
                        }
                     }
                  }
               }
            }

            std::string new_call_name = call_inst->getName().str() + ".c";
            llvm::CallInst* new_call_inst = llvm::CallInst::Create(new_function, call_ops, (llvm::isa<llvm::CallInst>(user) && llvm::dyn_cast<llvm::CallInst>(call_inst)->hasName() ? new_call_name : ""), call_inst);

            if(llvm::isa<llvm::CallInst>(user))
            {
               new_call_inst->setTailCall(llvm::dyn_cast<llvm::CallInst>(call_inst)->isTailCall());
               new_call_inst->setTailCallKind(llvm::dyn_cast<llvm::CallInst>(call_inst)->getTailCallKind());
            }
            new_call_inst->setCallingConv(new_function->getCallingConv());

            if(arg_stored_once != nullptr)
            {
               llvm::Value* arg_op = call_inst->getOperand(arg_stored_once->getArgNo());

               llvm::StoreInst* new_store = new llvm::StoreInst(new_call_inst, arg_op);
               new_store->insertAfter(new_call_inst);
            }
            else
            {
               call_inst->replaceAllUsesWith(new_call_inst);
            }

            call_inst->eraseFromParent();
         }
         else
         {
            llvm::errs() << "ERR: Function use should be call\n";
            exit(-1);
         }
      }

      erase_in_fixed.insert(function);
   }

   std::vector<llvm::AllocaInst*> allocas_to_del;

   for(llvm::Function* new_function : new_functions_set)
   {
      for(llvm::BasicBlock& bb : *new_function)
      {
         for(llvm::Instruction& i : bb)
         {
            if(llvm::AllocaInst* alloca_inst = llvm::dyn_cast<llvm::AllocaInst>(&i))
            {
               if(alloca_inst->getNumUses() == 0)
               {
                  allocas_to_del.push_back(alloca_inst);
               }
            }
            else
            {
               break;
            }
         }
      }
   }

   for(llvm::AllocaInst* alloca_to_del : allocas_to_del)
   {
      alloca_to_del->eraseFromParent();
   }

   for(auto r_it = erase_in_fixed.rbegin(); r_it != erase_in_fixed.rend(); ++r_it)
   {
      llvm::Function* f = *r_it;
      f->eraseFromParent();
   }

   std::vector<llvm::GlobalVariable*> globals_to_del;

   std::set<llvm::LoadInst*> loads_to_del;
   for(llvm::GlobalVariable& g_var : module.globals())
   {
      if(!g_var.getType()->getPointerElementType()->isAggregateType())
      {
         if(g_var.getInitializer() != nullptr)
         {
            bool only_load_inst_users = true;

            for(auto& u : g_var.uses())
            {
               if(llvm::LoadInst* load_isnt = llvm::dyn_cast<llvm::LoadInst>(u.getUser()))
               {
                  if(load_isnt->getPointerOperandIndex() != u.getOperandNo())
                  {
                     only_load_inst_users = false;
                     break;
                  }
               }
               else
               {
                  only_load_inst_users = false;
                  break;
               }
            }

            if(only_load_inst_users)
            {
               for(auto& u : g_var.uses())
               {
                  if(llvm::LoadInst* load_inst = llvm::dyn_cast<llvm::LoadInst>(u.getUser()))
                  {
                     load_inst->replaceAllUsesWith(g_var.getInitializer());
                     loads_to_del.insert(load_inst);
                  }
                  else
                  {
                     llvm::errs() << "ERR: Non load use\n";
                     exit(-1);
                  }
               }
            }
         }
      }

      if(g_var.getNumUses() == 0)
      {
         globals_to_del.push_back(&g_var);
      }
   }

   for(llvm::LoadInst* load_inst : loads_to_del)
   {
      load_inst->eraseFromParent();
   }

   for(llvm::GlobalVariable* g_var : globals_to_del)
   {
      llvm::dbgs() << "INFO: Erasing " << g_var->getValueName()->first() << " from module\n";
      g_var->eraseFromParent();
   }

} // end cleanup

void inline_wrappers(llvm::Function* kernel_function, std::set<llvm::Function*>& inner_functions, llvm::ModulePass* modulePass)
{
   std::set<llvm::Function*> fun_to_remove;

   for(llvm::Function* f : inner_functions)
   {
      bool doOpt = false;
      unsigned nStmtsCaller = 0;
      for(const auto& bbCaller : *f)
      {
         nStmtsCaller += bbCaller.size();
      }
   process_function:
      for(auto& bb : *f)
      {
         for(auto& inst : bb)
         {
            if(llvm::isa<llvm::CallInst>(inst) || llvm::isa<llvm::InvokeInst>(inst))
            {
               auto call_inst = llvm::dyn_cast<llvm::Instruction>(&inst);
               auto cf = llvm::CallSite(call_inst).getCalledFunction();
               if(cf == nullptr)
                  continue;
               if(cf->isIntrinsic())
                  continue;
               unsigned nStmtsCallee = 0;
               for(const auto& bbCallee : *cf)
               {
                  nStmtsCallee += bbCallee.size();
               }
               unsigned nusers = 0;
               for(auto it : cf->users())
               {
                  ++nusers;
               }
               // llvm::errs()<<cf->getName().str() << " number of users: "<< nusers << " Caller: " << nStmtsCaller << " Callee: " << nStmtsCallee << "\n";
               auto inlineCost = nStmtsCaller + nusers * nStmtsCallee;
               if(cf->doesNotAccessMemory())
               {
                  llvm::dbgs() << " doesNotAccessMemory\n";
                  call_inst->print(llvm::errs());
                  llvm::dbgs() << "\n";
               }
               bool is_wrapper = strncmp(cf->getName().str().c_str(), std::string(wrapper_function_name).c_str(), std::string(wrapper_function_name).size()) == 0;
               if(is_wrapper || (is_relevant_call(call_inst) and (nusers < 5 && inlineCost < CSROAInlineThreshold)))
               {
                  llvm::dbgs() << cf->getName().str() << " Inlined!\n";
                  cf->removeFnAttr(llvm::Attribute::NoInline);
                  cf->removeFnAttr(llvm::Attribute::OptimizeNone);
                  llvm::InlineFunctionInfo IFI = llvm::InlineFunctionInfo();
                  if((llvm::isa<llvm::CallInst>(inst) && !llvm::InlineFunction(llvm::dyn_cast<llvm::CallInst>(call_inst), IFI)) || (llvm::isa<llvm::InvokeInst>(inst) && !llvm::InlineFunction(llvm::dyn_cast<llvm::InvokeInst>(call_inst), IFI)))
                  {
                     llvm::errs() << "ERR: Cannot inline wrapper " << cf->getName() << "\n";
                     exit(-1);
                  }

                  fun_to_remove.insert(cf);
                  doOpt = true;

                  goto process_function;
               }
            }
         }
      }
      if(doOpt)
      {
         llvm::TargetLibraryInfo& TLI = modulePass->getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();
         const llvm::TargetTransformInfo& TTI = modulePass->getAnalysis<llvm::TargetTransformInfoWrapperPass>().getTTI(*f);
         for(llvm::Function::iterator BBIt = f->begin(); BBIt != f->end();)
            llvm::SimplifyInstructionsInBlock(&*BBIt++, &TLI);
         for(llvm::Function::iterator BBIt = f->begin(); BBIt != f->end();)
#if __clang_major__ >= 6
            llvm::simplifyCFG(&*BBIt++, TTI, 1);
#else
            llvm::SimplifyCFG(&*BBIt++, TTI, 1);
#endif
         llvm::removeUnreachableBlocks(*f);

         /// whenever it is possible the following promotes alloca to reg
         {
            auto& DT = modulePass->getAnalysis<llvm::DominatorTreeWrapperPass>(*f).getDomTree();
            auto& AC = modulePass->getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(*f);
            std::vector<llvm::AllocaInst*> Allocas;
            llvm::BasicBlock& BB = f->getEntryBlock(); // Get the entry node for the function

            while(1)
            {
               Allocas.clear();

               // Find allocas that are safe to promote, by looking at all instructions in
               // the entry node
               for(llvm::BasicBlock::iterator I = BB.begin(), E = --BB.end(); I != E; ++I)
                  if(llvm::AllocaInst* AI = llvm::dyn_cast<llvm::AllocaInst>(I)) // Is it an alloca?
                     if(llvm::isAllocaPromotable(AI))
                        Allocas.push_back(AI);

               if(Allocas.empty())
                  break;
#if __clang_major__ != 4
               llvm::PromoteMemToReg(Allocas, DT, &AC);
#else
               llvm::PromoteMemToReg(Allocas, DT, nullptr, &AC);
#endif
            }
         }
      }
   }

   for(llvm::Function* function : fun_to_remove)
   {
      inner_functions.erase(function);
   }

   delete_functions_recursively(fun_to_remove);

   // for(auto f_it = fun_to_remove.rbegin(); f_it != fun_to_remove.rend(); ++f_it)
   //{
   //   llvm::Function* f = *f_it;
   //   f->eraseFromParent();
   //}
}

llvm::Function* get_top_function(llvm::Module& module, std::string top_name)
{
   // check if the translation unit has the top function name
   for(auto& fun : module.getFunctionList())
   {
      if(!fun.isIntrinsic() && !fun.isDeclaration())
      {
         auto funName = fun.getName();
         auto demangled = getDemangled(funName);
         if(!fun.hasInternalLinkage() && (funName == top_name || demangled == top_name))
         {
            return &fun;
         }
      }
   }

   return nullptr;
}

bool CustomScalarReplacementOfAggregatesPass::runOnModule(llvm::Module& module)
{
   if(skipModule(module))
      return false;
   TotalAllocaBytes = TotalGlobalBytes = TotalOperandBytes = DisaggregatedAllocaBytes = DisaggregatedGlobalBytes = DisaggregatedOperandBytes = 0;
   llvm::Function* kernel_function = get_top_function(module, kernel_name);
#ifdef DEBUG_CSROA
   recorded_expanded_aggregates.clear();
#endif

   if(kernel_function == nullptr)
   {
      llvm::errs() << "ERR: Kernel function (" << kernel_name << ") not found! Possible values:\n";
      for(llvm::Function& function : module)
      {
         llvm::errs() << "  " << function.getName() << "\n";
      }

      exit(-1);
   }
   if(sroa_phase == SROA_functionVersioning)
   {
      llvm::dbgs() << "\n ***********************************************";
      llvm::dbgs() << "\n ********** BEGIN FUNCTION VERSIONING **********";
      llvm::dbgs() << "\n *********************************************** \n";
      auto t_begin = std::chrono::high_resolution_clock::now();
      const llvm::DataLayout DL = module.getDataLayout();

      std::map<llvm::Instruction*, std::vector<llvm::Instruction*>> compact_callgraph;

      std::map<llvm::AllocaInst*, Expandability> allocas_expandability_map;
      std::map<llvm::GlobalVariable*, Expandability> globals_expandability_map;
      std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability> operands_expandability_map;
      std::map<llvm::Use*, llvm::Value*> point_to_set_map;
      std::map<llvm::Function*, bool> empty_function_versioning_profitability;

      compute_aggregates_expandability(kernel_function, &module, allocas_expandability_map, globals_expandability_map, operands_expandability_map, point_to_set_map, compact_callgraph, DL, empty_function_versioning_profitability);

      std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>> operands_dimensions_map;
      compute_op_exp_and_dims(compact_callgraph, point_to_set_map, operands_expandability_map, allocas_expandability_map, globals_expandability_map, operands_dimensions_map, DL, empty_function_versioning_profitability);

      std::map<llvm::Argument*, bool> arguments_expandability_map;
      std::map<llvm::Argument*, std::vector<unsigned long long>> arguments_dimensions_map;

      std::set<llvm::Function*> function_worklist;
      std::map<llvm::Function*, bool> function_versioning_profitability;

      compute_function_versioning_profit(compact_callgraph, kernel_function, point_to_set_map, operands_expandability_map, allocas_expandability_map, globals_expandability_map, operands_dimensions_map, DL, function_versioning_profitability, false);

      perform_function_versioning(compact_callgraph, kernel_function, point_to_set_map, operands_expandability_map, allocas_expandability_map, globals_expandability_map, operands_dimensions_map, arguments_expandability_map, arguments_dimensions_map,
                                  function_worklist, function_versioning_profitability);

      propagate_constant_arguments(function_worklist);

      assert(!llvm::verifyModule(module, &llvm::errs()));

#ifdef DEBUG_CSROA
      if(CSROAMaxTransformations != -1)
         llvm::dbgs() << "Number of alloca expanded " << recorded_expanded_aggregates.size() << "\n";
#endif

      auto t_end = std::chrono::high_resolution_clock::now();
      double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_begin).count();
      llvm::dbgs() << "\nINFO: Function versioning took " << duration * 1e-9 << " seconds to complete";
      llvm::dbgs() << "\n *********************************************";
      llvm::dbgs() << "\n ********** END FUNCTION VERSIONING **********";
      llvm::dbgs() << "\n ********************************************* \n";

      return true;
   }

   if(sroa_phase == SROA_disaggregation)
   {
      llvm::dbgs() << "\n ******************************************";
      llvm::dbgs() << "\n ********** BEGIN DISAGGREGATION **********";
      llvm::dbgs() << "\n ****************************************** \n";
      auto t_begin = std::chrono::high_resolution_clock::now();
      const llvm::DataLayout DL = module.getDataLayout();

      std::map<llvm::Instruction*, std::vector<llvm::Instruction*>> compact_callgraph;

      std::map<llvm::AllocaInst*, Expandability> allocas_expandability_map;
      std::map<llvm::GlobalVariable*, Expandability> globals_expandability_map;
      std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, Expandability> operands_expandability_map;
      std::map<llvm::Use*, llvm::Value*> point_to_set_map;
      std::map<llvm::Function*, bool> empty_function_versioning_profitability;
      compute_aggregates_expandability(kernel_function, &module, allocas_expandability_map, globals_expandability_map, operands_expandability_map, point_to_set_map, compact_callgraph, DL, empty_function_versioning_profitability);

      std::map<std::pair<std::vector<llvm::Instruction*>, llvm::Use*>, std::vector<unsigned long long>> operands_dimensions_map;
      compute_op_exp_and_dims(compact_callgraph, point_to_set_map, operands_expandability_map, allocas_expandability_map, globals_expandability_map, operands_dimensions_map, DL, empty_function_versioning_profitability);

      std::map<llvm::Argument*, bool> arguments_expandability_map;
      std::map<llvm::Argument*, std::vector<unsigned long long>> arguments_dimensions_map;

      std::set<llvm::Function*> function_worklist;
      std::map<llvm::Function*, bool> function_versioning_profitability;

      compute_function_versioning_profit(compact_callgraph, kernel_function, point_to_set_map, operands_expandability_map, allocas_expandability_map, globals_expandability_map, operands_dimensions_map, DL, function_versioning_profitability, true);

      allocas_expandability_map.clear();
      globals_expandability_map.clear();
      operands_expandability_map.clear();
      point_to_set_map.clear();
      compact_callgraph.clear();
      operands_dimensions_map.clear();

      compute_aggregates_expandability(kernel_function, &module, allocas_expandability_map, globals_expandability_map, operands_expandability_map, point_to_set_map, compact_callgraph, DL, function_versioning_profitability);
      compute_op_exp_and_dims(compact_callgraph, point_to_set_map, operands_expandability_map, allocas_expandability_map, globals_expandability_map, operands_dimensions_map, DL, function_versioning_profitability);

      if(!check_function_versioning(compact_callgraph, kernel_function, operands_expandability_map, operands_dimensions_map, arguments_expandability_map, arguments_dimensions_map, function_versioning_profitability))
      {
         llvm::errs() << "ERR: Wrong versioning in disaggregation\n";
         exit(-1);
      }
      /*
               arguments_expandability_map.clear(); // useless
               arguments_dimensions_map.clear();    // useless

               std::map<llvm::Function*, bool> function_versioning_profitability; // TODO it's gonna be empty and it's gonna fail while performing versioning just below here
               std::set<llvm::Function*> function_worklist;
               perform_function_versioning(compact_callgraph, kernel_function, point_to_set_map, operands_expandability_map2, allocas_expandability_map2, globals_expandability_map2, operands_dimensions_map, arguments_expandability_map,
         arguments_dimensions_map, function_worklist, function_versioning_profitability);

               allocas_expandability_map.clear();
               globals_expandability_map.clear();
               operands_expandability_map.clear();
               point_to_set_map.clear();
               compact_callgraph.clear();
               operands_dimensions_map.clear();

               compute_aggregates_expandability(kernel_function, &module, allocas_expandability_map, globals_expandability_map, operands_expandability_map, point_to_set_map, compact_callgraph, DL);

               compute_op_exp_and_dims(compact_callgraph, point_to_set_map, operands_expandability_map, allocas_expandability_map, globals_expandability_map, operands_dimensions_map, DL);

               arguments_expandability_map.clear();
               arguments_dimensions_map.clear();

               allocas_expandability_map2.clear();
               globals_expandability_map2.clear();
               operands_expandability_map2.clear();
               for (auto op_it : allocas_expandability_map) {
                  allocas_expandability_map2[op_it.first] = op_it.second.expandability;
               }
               for (auto op_it : globals_expandability_map) {
                  globals_expandability_map2[op_it.first] = op_it.second.expandability;
               }
               for (auto op_it : operands_expandability_map) {
                  operands_expandability_map2[op_it.first] = op_it.second.expandability;
               }

               if(!check_function_versioning(compact_callgraph, kernel_function, operands_expandability_map2, operands_dimensions_map, arguments_expandability_map, arguments_dimensions_map, function_versioning_profitability))
               {
                  llvm::errs() << "ERR: Wrong versioning!\n";
                  exit(-1);
               }
            }
      */
      // std::set<llvm::Function*> function_worklist;
      function_worklist.clear();
      for(auto cc_it : compact_callgraph)
      {
         for(auto call_inst : cc_it.second)
         {
            function_worklist.insert(llvm::CallSite(call_inst).getCalledFunction());
         }
      }

      std::set<llvm::Instruction*> inst_to_remove;
      std::set<llvm::Function*> fun_to_remove;

      // Map linking any function with its modified version
      std::map<llvm::Function*, llvm::Function*> exp_fun_map;

      // Map specifying how arguments have been expanded
      std::map<llvm::Argument*, std::vector<llvm::Argument*>> arguments_expansion_map;

      expand_signatures_and_call_sites(function_worklist, exp_fun_map, arguments_expandability_map, arguments_dimensions_map, arguments_expansion_map, inst_to_remove, fun_to_remove);

      update_allocas_expandability_map(allocas_expandability_map, exp_fun_map);

      // Map specifying how allocas have been expanded
      std::map<llvm::AllocaInst*, std::vector<llvm::AllocaInst*>> allocas_expansion_map;
      function_worklist.insert(kernel_function);

      expand_allocas(function_worklist, DL, allocas_expansion_map, allocas_expandability_map);
      function_worklist.erase(kernel_function);

      // Map specifying how global variables have been expanded
      std::map<llvm::GlobalVariable*, std::vector<llvm::GlobalVariable*>> globals_expansion_map;
      expand_globals(&module, DL, globals_expansion_map, globals_expandability_map);

      delete_functions_recursively(fun_to_remove);

      function_worklist.insert(kernel_function);

      expand_ptrs(function_worklist, arguments_expansion_map, allocas_expansion_map, globals_expansion_map, arguments_expandability_map, arguments_dimensions_map, inst_to_remove, DL);

      function_worklist.erase(kernel_function);

      cleanup(module, exp_fun_map, function_worklist, inst_to_remove, arguments_expansion_map, globals_expansion_map, allocas_expansion_map);

      assert(!llvm::verifyModule(module, &llvm::errs()));

#ifdef DEBUG_CSROA
      if(CSROAMaxTransformations != -1)
         llvm::dbgs() << "Number of alloca expanded " << recorded_expanded_aggregates.size() << "\n";
#endif

      llvm::dbgs() << "Total amount of aggregate bytes by alloca instructions " << TotalAllocaBytes << "\n";
      llvm::dbgs() << "Total amount of aggregate bytes by Global variables " << TotalGlobalBytes << "\n";
      llvm::dbgs() << "Total amount of aggregate bytes as function operands " << TotalOperandBytes << "\n";
      llvm::dbgs() << "Disaggregated amount of aggregate bytes by alloca instructions " << DisaggregatedAllocaBytes << "\n";
      llvm::dbgs() << "Disaggregated amount of aggregate bytes by Global variables " << DisaggregatedGlobalBytes << "\n";
      llvm::dbgs() << "Disaggregated amount of aggregate bytes as function operands " << DisaggregatedOperandBytes << "\n";

      auto t_end = std::chrono::high_resolution_clock::now();
      double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_begin).count();
      llvm::dbgs() << "\nINFO: Disaggregation took " << duration * 1e-9 << " seconds to complete";
      llvm::dbgs() << "\n ****************************************";
      llvm::dbgs() << "\n ********** END DISAGGREGATION **********";
      llvm::dbgs() << "\n **************************************** \n";

      return true;
   }

   if(sroa_phase == SROA_wrapperInlining)
   {
      llvm::dbgs() << "\n ********************************************";
      llvm::dbgs() << "\n ********** BEGIN WRAPPER INLINING **********";
      llvm::dbgs() << "\n ******************************************** \n";
      auto t_begin = std::chrono::high_resolution_clock::now();

      std::map<llvm::Instruction*, std::vector<llvm::Instruction*>> compact_callgraph;

      compute_callgraph(kernel_function, compact_callgraph);

      std::set<llvm::Function*> function_worklist;
      for(auto cc_it : compact_callgraph)
      {
         for(auto call_inst : cc_it.second)
         {
            function_worklist.insert(llvm::CallSite(call_inst).getCalledFunction());
         }
      }

      function_worklist.insert(kernel_function);

      inline_wrappers(kernel_function, function_worklist, this);

      function_worklist.erase(kernel_function);

      std::set<llvm::Instruction*> inst_to_remove;
      std::map<llvm::Argument*, std::vector<llvm::Argument*>> exp_args_map;
      std::map<llvm::AllocaInst*, std::vector<llvm::AllocaInst*>> exp_allocas_map;
      std::map<llvm::GlobalVariable*, std::vector<llvm::GlobalVariable*>> exp_globals_map;
      std::map<llvm::Function*, llvm::Function*> exp_fun_map;
      cleanup(module, exp_fun_map, function_worklist, inst_to_remove, exp_args_map, exp_globals_map, exp_allocas_map);

      assert(!llvm::verifyModule(module, &llvm::errs()));

#ifdef DEBUG_CSROA
      if(CSROAMaxTransformations != -1)
         llvm::dbgs() << "Number of alloca expanded " << recorded_expanded_aggregates.size() << "\n";
#endif

      auto t_end = std::chrono::high_resolution_clock::now();
      double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(t_end - t_begin).count();
      llvm::dbgs() << "\nINFO: Wrapper inlining took " << duration * 1e-9 << " seconds to complete";
      llvm::dbgs() << "\n ******************************************";
      llvm::dbgs() << "\n ********** END WRAPPER INLINING **********";
      llvm::dbgs() << "\n ****************************************** \n";

      return true;
   }

   return false;
}
/*
CustomScalarReplacementOfAggregatesPass* createSROACodeSimplificationPass(std::string kernel_name)
{
   return new CustomScalarReplacementOfAggregatesPass(kernel_name, SROA_codeSimplification);
}
*/
CustomScalarReplacementOfAggregatesPass* createSROAFunctionVersioningPass(std::string kernel_name)
{
   return new CustomScalarReplacementOfAggregatesPass(kernel_name, SROA_functionVersioning);
}

CustomScalarReplacementOfAggregatesPass* createSROADisaggregationPass(std::string kernel_name)
{
   return new CustomScalarReplacementOfAggregatesPass(kernel_name, SROA_disaggregation);
}

CustomScalarReplacementOfAggregatesPass* createSROAWrapperInliningPass(std::string kernel_name)
{
   return new CustomScalarReplacementOfAggregatesPass(kernel_name, SROA_wrapperInlining);
}
