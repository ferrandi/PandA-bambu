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
 *              Copyright (C) 2018-2026 Politecnico di Milano
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
 * @file dumpBambuIrSSA.cpp
 * @brief Plugin to dump functions and global variables starting from LLVM IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef NDEBUG
#define NDEBUG
#endif
#include "plugin_includes.hpp"

#include "HardekopfLin_AA.hpp"

#if PANDA_LLVM_CLANG_MAJOR > 5
#include "TreeHeightReduction.hpp"
#endif

#include <llvm/ADT/Twine.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/CFG.h>
#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/LazyValueInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/CodeGen/IntrinsicLowering.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/ConstantRange.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ModuleSlotTracker.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Pass.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Transforms/Utils/Local.h>

#if PANDA_LLVM_CLANG_MAJOR == 4
#include <llvm/Transforms/Utils/MemorySSA.h>
#else
#include <llvm/Analysis/MemorySSA.h>
#include <llvm/Support/KnownBits.h>
#include <llvm/Transforms/Utils/LowerMemIntrinsics.h>
#endif
#if PANDA_LLVM_CLANG_MAJOR < 11
#include <llvm/IR/CallSite.h>
#endif

#include <cxxabi.h>
#include <float.h>
#include <iomanip>

#define ANDERSEN_AA 1

#define DEBUG_TY_NODE "dump-bambu-ir"

static std::string create_file_name_string(const std::string& outdir_name, const std::string& original_filename)
{
   std::size_t found = original_filename.find_last_of("/\\");
   std::string dump_base_name;
   if(found == std::string::npos)
      dump_base_name = original_filename;
   else
      dump_base_name = original_filename.substr(found + 1);
   return outdir_name + "/" + dump_base_name + ".bambuir";
}

#define PEEL_THRESHOLD 16

namespace llvm
{
   char DumpBambuIR::buffer[LOCAL_BUFFER_LEN];

   static unsigned long long getAbiTypeAlignmentBytes(const llvm::DataLayout* DL, llvm::Type* ty)
   {
#if PANDA_LLVM_CLANG_MAJOR < 12
      return DL->getABITypeAlignment(ty);
#else
      return DL->getABITypeAlign(ty).value();
#endif
   }

   static unsigned long long commonAlignmentBytes(unsigned long long baseAlign, uint64_t offset)
   {
      if(offset == 0)
      {
         return baseAlign;
      }
      while(baseAlign > 1 && (offset & (baseAlign - 1)) != 0)
      {
         baseAlign >>= 1;
      }
      return baseAlign;
   }

   static unsigned long long getBasePointerAlignment(const llvm::Value* base, const llvm::DataLayout* DL)
   {
      if(const auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(base))
      {
#if PANDA_LLVM_CLANG_MAJOR < 16
         if(allocaInst->getAlignment())
         {
            return allocaInst->getAlignment();
         }
#else
         return allocaInst->getAlign().value();
#endif
         return getAbiTypeAlignmentBytes(DL, allocaInst->getAllocatedType());
      }
      if(const auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(base))
      {
#if PANDA_LLVM_CLANG_MAJOR < 16
         if(globalVar->getAlignment())
         {
            return globalVar->getAlignment();
         }
#else
         if(globalVar->getAlign())
         {
            return globalVar->getAlign()->value();
         }
#endif
         return getAbiTypeAlignmentBytes(DL, globalVar->getValueType());
      }
      if(const auto* arg = llvm::dyn_cast<llvm::Argument>(base))
      {
         if(arg->hasAttribute(llvm::Attribute::Alignment))
         {
#if PANDA_LLVM_CLANG_MAJOR < 10
            return arg->getParamAlignment();
#else
            if(const auto paramAlign = arg->getParamAlign())
            {
               return paramAlign->value();
            }
#endif
         }
      }
      if(const auto* ptrTy = llvm::dyn_cast<llvm::PointerType>(base->getType()))
      {
#if PANDA_LLVM_CLANG_MAJOR >= 19
         // Opaque pointers carry no pointee type, so we cannot infer the
         // base alignment from the pointer alone.  Return 0 ("unknown") to
         // let the caller keep the instruction's own alignment unmodified.
         (void)ptrTy;
         return 0;
#elif PANDA_LLVM_CLANG_MAJOR >= 16
         return ptrTy->isOpaque() ? 0 : getAbiTypeAlignmentBytes(DL, ptrTy->getPointerElementType());
#else
         return getAbiTypeAlignmentBytes(DL, ptrTy->getPointerElementType());
#endif
      }
      return 0;
   }

   static unsigned long long getEffectiveMemoryAccessAlignment(const llvm::Value* ptr, unsigned long long instAlign,
                                                               const llvm::DataLayout* DL)
   {
      auto effectiveAlign = instAlign;
      int64_t offset = 0;
      const auto* base = llvm::GetPointerBaseWithConstantOffset(ptr, offset, *DL);
      if(base)
      {
         const auto baseAlign = getBasePointerAlignment(base, DL);
         if(baseAlign)
         {
            const auto inferredAlign = commonAlignmentBytes(baseAlign, static_cast<uint64_t>(std::abs(offset)));
            if(inferredAlign < effectiveAlign)
            {
               effectiveAlign = inferredAlign;
            }
         }
      }
      return effectiveAlign;
   }

#define EXPRCODE(SYM, STRING, TYPE) SYM,
   enum class DumpBambuIR::ir_codes
   {
#include "IR.def"
   };
#undef EXPRCODE
/* Codes of IR nodes.  */
#define EXPRCODE(SYM, STRING, TYPE) STRING,
   const char* DumpBambuIR::ir_codesNames[] = {
#include "IR.def"
   };
#undef EXPRCODE
#define EXPRCODE(SYM, STRING, TYPE) TYPE,
   const DumpBambuIR::irClass DumpBambuIR::ir_codes2irClass[] = {
#include "IR.def"
   };
#undef EXPRCODE

#define EXPRCODE(SYM, STRING, TYPE)                                                                  \
   ((TYPE) == ir_unary                                                            ? IR_UNARY_RHS :   \
    (TYPE) == ir_binary                                                           ? IR_BINARY_RHS :  \
    (TYPE) == ir_ternary                                                          ? IR_TERNARY_RHS : \
    ((TYPE) == ir_constant || (TYPE) == ir_declaration || (TYPE) == ir_reference) ? IR_SINGLE_RHS :  \
    (IRC(SYM) == IRC(CONSTRUCTOR_NODE) || IRC(SYM) == IRC(SSA_NODE))              ? IR_SINGLE_RHS :  \
                                                                                    IR_INVALID_RHS),

   const DumpBambuIR::ir_rhs_class DumpBambuIR::ir_rhs_class_table[] = {
#include "IR.def"
   };
#undef EXPRCODE

   const char* DumpBambuIR::ValueTyNames[] = {
#define HANDLE_VALUE(Name) #Name,
#include "llvm/IR/Value.def"
#define HANDLE_INST(N, OPC, CLASS) #OPC,
#include "llvm/IR/Instruction.def"
   };

#define BUILTIN(N, C, T) #N,
   const std::set<std::string> DumpBambuIR::builtinsNames = {
#include "AdditionalBuiltins.def"
#include "clang/Basic/Builtins.def"
   };
#undef BUILTIN

   std::string DumpBambuIR::getName(const llvm::GlobalObject* fd)
   {
      const std::string name = fd->getName().data();
      const auto name_finite_pos = name.find("_finite");
      if(name_finite_pos != std::string::npos && (name_finite_pos + strlen("_finite")) == name.size())
      {
         if(name.size() > (2 + strlen("_finite")))
         {
            const auto name_nofinite = name.substr(2, name_finite_pos - 2);
            if(builtinsNames.find(std::string("__builtin_") + name_nofinite) != builtinsNames.end() ||
               builtinsNames.find(name_nofinite) != builtinsNames.end())
            {
               return name_nofinite;
            }
         }
      }
      return name;
   }

   static std::string getDemangled(const std::string& declname)
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

   static std::string getArgumentStringAttribute(const llvm::Function* fd, unsigned int arg_index,
                                                 const char* attribute_name)
   {
#if PANDA_LLVM_CLANG_MAJOR >= 5
      const auto arg_attr = fd->getAttributes().getParamAttr(arg_index, attribute_name);
#else
      const auto arg_attr = fd->getAttributes().getAttribute(arg_index + 1, attribute_name);
#endif
      if(!arg_attr.isStringAttribute())
      {
         return "";
      }
      return arg_attr.getValueAsString().str();
   }

   DumpBambuIR::DumpBambuIR(const std::string& _outdir_name, const std::string& _InFile, bool _onlyGlobals,
                            std::map<std::string, std::vector<std::string>>* _fun2params, bool early)
       :
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnull-dereference"
         GetTLI([](llvm::Function&) -> llvm::TargetLibraryInfo& { return *((llvm::TargetLibraryInfo*)nullptr); }),
         GetTTI([](llvm::Function&) -> llvm::TargetTransformInfo& { return *((llvm::TargetTransformInfo*)nullptr); }),
         GetDomTree([](llvm::Function&) -> llvm::DominatorTree& { return *((llvm::DominatorTree*)nullptr); }),
         GetLI([](llvm::Function&) -> llvm::LoopInfo& { return *((llvm::LoopInfo*)nullptr); }),
         GetMSSA([](llvm::Function&) -> MemorySSAAnalysisResult& { return *((MemorySSAAnalysisResult*)nullptr); }),
         GetLVI([](llvm::Function&) -> llvm::LazyValueInfo& { return *((llvm::LazyValueInfo*)nullptr); }),
         GetAC([](llvm::Function&) -> llvm::AssumptionCache& { return *((llvm::AssumptionCache*)nullptr); }),
#pragma clang diagnostic pop
         earlyAnalysis(early),
         outdir_name(_outdir_name),
         InFile(_InFile),
         filename(create_file_name_string(_outdir_name, _InFile)),
#if PANDA_LLVM_CLANG_MAJOR >= 7 && !defined(VVD)
         stream(create_file_name_string(_outdir_name, _InFile), EC, sys::fs::FA_Read | sys::fs::FA_Write),
#else
         stream(create_file_name_string(_outdir_name, _InFile), EC, llvm::sys::fs::F_RW),
#endif
         onlyGlobals(_onlyGlobals),
         fun2params(_fun2params),
         DL(nullptr),
         moduleContext(nullptr),
         last_used_index(0),
         column(0),
         PtoSets_AA(nullptr),
         SignedPointerTypeReference(0),
         last_memory_ssa_vers(std::numeric_limits<int>::max()),
         last_BB_index(2)
   {
      if(EC)
      {
         llvm::report_fatal_error("not able to open the output raw file");
      }
      DumpVersion(stream);
      assignCode(&SignedPointerTypeReference, IRC(SIGNEDPOINTERTYPE));
   }

   const void* DumpBambuIR::assignCodeAuto(const void* t)
   {
      assert(t);
      if(HAS_CODE(t))
         return t;

      const llvm::Value* llvm_obj = reinterpret_cast<const llvm::Value*>(t);
      auto vid = llvm_obj->getValueID();
      switch(vid)
      {
         case llvm::Value::GlobalVariableVal:
            return assignCode(t, IRC(VARIABLE_VAL_NODE));
         case llvm::Value::FunctionVal:
            return assignCode(t, IRC(FUNCTION_VAL_NODE));
         case llvm::Value::ConstantFPVal:
            return assignCode(t, IRC(CONSTANT_FP_VAL_NODE));
         case llvm::Value::ArgumentVal:
            return assignCode(t, IRC(ARGUMENT_VAL_NODE));
         case llvm::Value::ConstantIntVal:
         case llvm::Value::ConstantPointerNullVal:
            return assignCode(t, IRC(CONSTANT_INT_VAL_NODE));
         case llvm::Value::ConstantStructVal:
         case llvm::Value::ConstantAggregateZeroVal:
         case llvm::Value::ConstantDataArrayVal:
         case llvm::Value::ConstantArrayVal:
         case llvm::Value::ConstantDataVectorVal:
            return assignCode(t, IRC(CONSTRUCTOR_NODE));
         case llvm::Value::UndefValueVal:
         {
            auto type = llvm_obj->getType();
            if(type->isAggregateType() || type->isVectorTy())
               return assignCodeAuto(llvm::ConstantAggregateZero::get(type));
            else if(type->isPointerTy())
               return assignCodeAuto(llvm::ConstantPointerNull::get(cast<llvm::PointerType>(type)));
            else if(type->isIntegerTy())
               return assignCodeAuto(llvm::ConstantInt::get(type, 0, false));
            else if(type->isFloatingPointTy())
               return assignCodeAuto(llvm::ConstantFP::getNaN(type));
            else
            {
               llvm_obj->print(llvm::errs(), true);
               llvm::errs() << "\n";
               stream.close();
               report_fatal_error(
                   std::string("unexpected condition: " + std::string(ValueTyNames[llvm_obj->getValueID()])).c_str());
            }
         }
         case llvm::Value::ConstantExprVal:
         {
            auto type = assignCodeType(llvm_obj->getType());
            if(cast<llvm::ConstantExpr>(llvm_obj)->getOpcode() == llvm::Instruction::GetElementPtr)
               return LowerGetElementPtr(type, cast<llvm::ConstantExpr>(llvm_obj), nullptr);
            else if(cast<llvm::ConstantExpr>(llvm_obj)->getOpcode() == llvm::Instruction::IntToPtr ||
                    cast<llvm::ConstantExpr>(llvm_obj)->getOpcode() == llvm::Instruction::PtrToInt)
            {
               auto op = cast<llvm::ConstantExpr>(llvm_obj)->getOperand(0);
               return build1(IRC(NOP_NODE), type, getOperand(op, nullptr));
            }
            else if(cast<llvm::ConstantExpr>(llvm_obj)->getOpcode() == llvm::Instruction::BitCast)
            {
               auto op = cast<llvm::ConstantExpr>(llvm_obj)->getOperand(0);
               return build1(IRC(BITCAST_NODE), type, getOperand(op, nullptr));
            }
            else
            {
               llvm_obj->print(llvm::errs(), true);
               llvm::errs() << "\n";
               llvm::errs() << cast<llvm::ConstantExpr>(llvm_obj)->getOpcodeName() << "\n";
               stream.close();
               report_fatal_error(
                   std::string("unexpected condition: " + std::string(ValueTyNames[llvm_obj->getValueID()])).c_str());
            }
         }
         case llvm::Value::ConstantVectorVal:
            return assignCode(t, IRC(CONSTANT_VECTOR_VAL_NODE));
#define HANDLE_BINARY_INST(N, OPC, CLASS)                     \
   case llvm::Value::InstructionVal + llvm::Instruction::OPC: \
      return assignCode(t, IRC(IR_ASSIGN));
#include "llvm/IR/Instruction.def"
#if PANDA_LLVM_CLANG_MAJOR >= 10
         case llvm::Value::InstructionVal + llvm::Instruction::FNeg:
         case llvm::Value::InstructionVal + llvm::Instruction::Freeze:
#endif
         case llvm::Value::InstructionVal + llvm::Instruction::Store:
         case llvm::Value::InstructionVal + llvm::Instruction::Load:
         case llvm::Value::InstructionVal + llvm::Instruction::Select:
         case llvm::Value::InstructionVal + llvm::Instruction::FCmp:
         case llvm::Value::InstructionVal + llvm::Instruction::ICmp:
         case llvm::Value::InstructionVal + llvm::Instruction::BitCast:
            return assignCode(t, IRC(IR_ASSIGN));
         case llvm::Value::InstructionVal + llvm::Instruction::Ret:
            return assignCode(t, IRC(IR_RETURN));
         case llvm::Value::InstructionVal + llvm::Instruction::Br:
         {
            const llvm::BranchInst* br = reinterpret_cast<const llvm::BranchInst*>(t);
            if(br->isUnconditional())
               return assignCode(t, IRC(IR_GOTO));
            else
               return assignCode(t, IRC(IR_COND));
         }
         case llvm::Value::InstructionVal + llvm::Instruction::PHI:
            return assignCode(t, IRC(IR_PHI));
         case llvm::Value::InstructionVal + llvm::Instruction::Alloca:
            return assignCode(t, IRC(IR_ASSIGN_ALLOCA));
         case llvm::Value::InstructionVal + llvm::Instruction::Call:
         {
            const llvm::CallInst* ci = reinterpret_cast<const llvm::CallInst*>(t);
            if(ci->getCalledFunction() && ci->getCalledFunction()->isIntrinsic())
            {
               auto intID = ci->getCalledFunction()->getIntrinsicID();
               switch(intID)
               {
                  case llvm::Intrinsic::lifetime_start:
                  case llvm::Intrinsic::lifetime_end:
                  case llvm::Intrinsic::dbg_value:
                  case llvm::Intrinsic::stacksave:
                  case llvm::Intrinsic::stackrestore:
#ifdef VVD
                  case llvm::Intrinsic::directive_scope_entry:
                  case llvm::Intrinsic::directive_scope_exit:
#endif
                     return assignCode(t, IRC(IR_NOPMEM));
#if PANDA_LLVM_CLANG_MAJOR > 13
                  case llvm::Intrinsic::is_fpclass:
#endif
                  case llvm::Intrinsic::memcpy:
                  case llvm::Intrinsic::memset:
                  case llvm::Intrinsic::memmove:
                  case llvm::Intrinsic::trap:
                     return ci->use_empty() ? assignCode(t, IRC(IR_CALL)) : assignCode(t, IRC(IR_ASSIGN));
#if PANDA_LLVM_CLANG_MAJOR != 4
                  case llvm::Intrinsic::ssa_copy:
                     return assignCode(t, IRC(IR_SSACOPY));
#endif
                  case llvm::Intrinsic::copysign:
                  case llvm::Intrinsic::fabs:
                  case llvm::Intrinsic::sqrt:
                  case llvm::Intrinsic::rint:
                  case llvm::Intrinsic::fmuladd:
                  case llvm::Intrinsic::minnum:
                  case llvm::Intrinsic::maxnum:
#if PANDA_LLVM_CLANG_MAJOR > 7
                  case llvm::Intrinsic::sadd_sat:
                  case llvm::Intrinsic::uadd_sat:
                  case llvm::Intrinsic::ssub_sat:
                  case llvm::Intrinsic::usub_sat:
#endif
#if PANDA_LLVM_CLANG_MAJOR > 11
                  case llvm::Intrinsic::fshl:
                  case llvm::Intrinsic::fshr:
                  case llvm::Intrinsic::abs:
                  case llvm::Intrinsic::smax:
                  case llvm::Intrinsic::smin:
                  case llvm::Intrinsic::umax:
                  case llvm::Intrinsic::umin:
#endif
#if PANDA_LLVM_CLANG_MAJOR > 12
                  case llvm::Intrinsic::bitreverse:
#endif
                     return assignCode(t, IRC(IR_ASSIGN));
                  default:
                     llvm::errs() << "assignCodeAuto kind not supported: " << ValueTyNames[vid] << "\n";
                     ci->print(llvm::errs(), true);
                     stream.close();
                     report_fatal_error("Plugin Error");
               }
            }
#if PANDA_LLVM_CLANG_MAJOR >= 11
            auto calledFun = ci->getCalledOperand();
#else
            llvm::ImmutableCallSite CS(ci);
            auto calledFun = CS.getCalledValue();
#endif
            if(ci->getType()->isVoidTy() || ci->use_empty())
            {
               assert(ci->use_empty());
               return assignCode(t, IRC(IR_CALL));
            }
            else
               return assignCode(t, IRC(IR_ASSIGN));
         }
         case llvm::Value::InstructionVal + llvm::Instruction::GetElementPtr:
            return assignCode(t, IRC(GETELEMENTPTR));
         case llvm::Value::InstructionVal + llvm::Instruction::SExt:
         case llvm::Value::InstructionVal + llvm::Instruction::ZExt:
         case llvm::Value::InstructionVal + llvm::Instruction::Trunc:
         case llvm::Value::InstructionVal + llvm::Instruction::PtrToInt:
         case llvm::Value::InstructionVal + llvm::Instruction::IntToPtr:
         case llvm::Value::InstructionVal + llvm::Instruction::FPExt:
         case llvm::Value::InstructionVal + llvm::Instruction::FPToSI:
         case llvm::Value::InstructionVal + llvm::Instruction::FPToUI:
         case llvm::Value::InstructionVal + llvm::Instruction::FPTrunc:
         case llvm::Value::InstructionVal + llvm::Instruction::UIToFP:
         case llvm::Value::InstructionVal + llvm::Instruction::SIToFP:
         case llvm::Value::InstructionVal + llvm::Instruction::InsertValue:
         case llvm::Value::InstructionVal + llvm::Instruction::ExtractValue:
         case llvm::Value::InstructionVal + llvm::Instruction::InsertElement:
         case llvm::Value::InstructionVal + llvm::Instruction::ExtractElement:
         case llvm::Value::InstructionVal + llvm::Instruction::ShuffleVector:
            return assignCode(t, IRC(IR_ASSIGN));
         case llvm::Value::InstructionVal + llvm::Instruction::Switch:
            return assignCode(t, IRC(IR_IFELSEIF));
         case llvm::Value::InstructionVal + llvm::Instruction::Unreachable:
            return assignCode(t, IRC(IR_RETURN));
         default:
            llvm::errs() << "assignCodeAuto kind not supported: " << ValueTyNames[vid] << "\n";
            stream.close();
            report_fatal_error("Plugin Error");
      }
   }
   bool DumpBambuIR::IR_DECL_ASSEMBLER_NAME_SET_P(const void* t) const
   {
      if(IR_CODE(t) == IRC(MODULE_UNIT_NODE))
         return false;
      if(IR_CODE(t) == IRC(ALLOCAVARIABLE_VAL_NODE))
         return false;
      if(IR_CODE(t) == IRC(ORIGVARIABLE_VAL_NODE))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return IR_DECL_ASSEMBLER_NAME_SET_P(ov->orig);
      }
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      if(!getName(llvm_obj).empty())
      {
         std::string declname = getName(llvm_obj);
         int status;
         char* demangled_outbuffer = abi::__cxa_demangle(declname.c_str(), nullptr, nullptr, &status);
         if(status == 0)
         {
            free(demangled_outbuffer);
            return true;
         }
         else
            assert(demangled_outbuffer == nullptr);
      }
      return false;
   }

   const void* DumpBambuIR::IR_DECL_ASSEMBLER_NAME(const void* t)
   {
      assert(IR_CODE(t) != IRC(MODULE_UNIT_NODE));
      if(IR_CODE(t) == IRC(ORIGVARIABLE_VAL_NODE))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return IR_DECL_ASSEMBLER_NAME(ov->orig);
      }
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      if(!getName(llvm_obj).empty())
      {
         std::string declname = getName(llvm_obj);
         if(identifierTable.find(declname) == identifierTable.end())
            identifierTable.insert(declname);
         const void* dn = identifierTable.find(declname)->c_str();
         return assignCode(dn, IRC(IDENTIFIER_NODE));
      }
      else
         report_fatal_error("IR_DECL_ASSEMBLER_NAME: IR_DECL_ASSEMBLER_NAME_SET_P is not true");
   }

   static std::string getIntrinsicName(const llvm::Function* fd)
   {
      assert(fd->isIntrinsic());
      auto intID = fd->getIntrinsicID();
      switch(intID)
      {
         case llvm::Intrinsic::copysign:
            if(fd->getReturnType()->isFloatTy())
               return "copysignf";
            else if(fd->getReturnType()->isDoubleTy())
               return "copysign";
            else if(fd->getReturnType()->isFP128Ty())
               return "copysignl";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         case llvm::Intrinsic::fabs:
            if(fd->getReturnType()->isFloatTy())
               return "fabsf";
            else if(fd->getReturnType()->isDoubleTy())
               return "fabs";
            else if(fd->getReturnType()->isFP128Ty())
               return "fabsl";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         case llvm::Intrinsic::sqrt:
            if(fd->getReturnType()->isFloatTy())
               return "sqrtf";
            else if(fd->getReturnType()->isDoubleTy())
               return "sqrt";
            else if(fd->getReturnType()->isFP128Ty())
               return "sqrtl";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
#if PANDA_LLVM_CLANG_MAJOR > 13
         case llvm::Intrinsic::is_fpclass:
         {
            auto funType = cast<llvm::FunctionType>(fd->getValueType());
            if(funType->getParamType(0)->isFloatTy())
               return "_llvm_is_fpclass_f";
            else if(funType->getParamType(0)->isDoubleTy())
               return "_llvm_is_fpclass_d";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         }
#endif
         case llvm::Intrinsic::memcpy:
         {
            auto funType = cast<llvm::FunctionType>(fd->getValueType());
            if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 32)
               return "_llvm_memcpy_p0i8_p0i8_i32";
            else if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 64)
               return "_llvm_memcpy_p0i8_p0i8_i64";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         }
         case llvm::Intrinsic::memset:
         {
            auto funType = cast<llvm::FunctionType>(fd->getValueType());
            if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 32)
               return "_llvm_memset_p0i8_i32";
            else if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 64)
               return "_llvm_memset_p0i8_i64";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         }
         case llvm::Intrinsic::memmove:
         {
            auto funType = cast<llvm::FunctionType>(fd->getValueType());
            if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 32)
               return "_llvm_memmove_p0i8_p0i8_i32";
            else if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 64)
               return "_llvm_memmove_p0i8_p0i8_i64";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         }
         case llvm::Intrinsic::trap:
            return "__builtin_trap";
         case llvm::Intrinsic::dbg_value:
            return "__builtin_debug_value";
         case llvm::Intrinsic::vastart:
            return "__builtin_va_start";
         case llvm::Intrinsic::vaend:
            return "__builtin_va_end";
         case llvm::Intrinsic::vacopy:
            return "__builtin_va_copy";
#if PANDA_LLVM_CLANG_MAJOR > 7
         case llvm::Intrinsic::sadd_sat:
            return "__llvm_sadd_sat";
         case llvm::Intrinsic::uadd_sat:
            return "__llvm_uadd_sat";
         case llvm::Intrinsic::ssub_sat:
            return "__llvm_ssub_sat";
         case llvm::Intrinsic::usub_sat:
            return "__llvm_usub_sat";
#endif
#if PANDA_LLVM_CLANG_MAJOR > 11
         case llvm::Intrinsic::fshl:
            return "__llvm_fshl";
         case llvm::Intrinsic::fshr:
            return "__llvm_fshr";
         case llvm::Intrinsic::abs:
            return "__llvm_abs";
         case llvm::Intrinsic::smax:
         {
            return "__llvm_smax";
         }
         case llvm::Intrinsic::smin:
         {
            return "__llvm_smin";
         }
         case llvm::Intrinsic::umax:
         {
            return "__llvm_umax";
         }
         case llvm::Intrinsic::umin:
         {
            return "__llvm_umin";
         }
#endif
#if PANDA_LLVM_CLANG_MAJOR > 12
         case llvm::Intrinsic::bitreverse:
         {
            auto funType = cast<llvm::FunctionType>(fd->getValueType());
            if(funType->getParamType(0)->isIntegerTy())
            {
               auto bitsize = funType->getParamType(0)->getScalarSizeInBits();
               if(bitsize == 8)
               {
                  return "__llvm_bitreverse_i8";
               }
               else if(bitsize == 16)
               {
                  return "__llvm_bitreverse_i16";
               }
               else if(bitsize == 32)
               {
                  return "__llvm_bitreverse_i32";
               }
               else if(bitsize == 64)
               {
                  return "__llvm_bitreverse_i64";
               }
            }
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         }
#endif
         case llvm::Intrinsic::rint:
         {
            if(fd->getReturnType()->isFloatTy())
               return "rintf";
            else if(fd->getReturnType()->isDoubleTy())
               return "rint";
            else if(fd->getReturnType()->isFP128Ty())
               return "rintl";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         }
         case llvm::Intrinsic::fmuladd:
         {
            if(fd->getReturnType()->isFloatTy())
               return "__float32_muladd";
            else if(fd->getReturnType()->isDoubleTy())
               return "__float64_muladd";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         }
         case llvm::Intrinsic::minnum:
         {
            if(fd->getReturnType()->isFloatTy())
               return "fminf";
            else if(fd->getReturnType()->isDoubleTy())
               return "fmin";
            else if(fd->getReturnType()->isFP128Ty())
               return "fminl";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         }
         case llvm::Intrinsic::maxnum:
         {
            if(fd->getReturnType()->isFloatTy())
               return "fmaxf";
            else if(fd->getReturnType()->isDoubleTy())
               return "fmax";
            else if(fd->getReturnType()->isFP128Ty())
               return "fmaxl";
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
         }
         case llvm::Intrinsic::stacksave:
         {
            return "_llvm_stacksave_p0";
         }
         case llvm::Intrinsic::stackrestore:
         {
            return "_llvm_stackrestore_p0";
         }
#ifdef VVD
         case llvm::Intrinsic::directive_scope_entry:
         {
            return "directive_scope_entry";
         }
         case llvm::Intrinsic::directive_scope_exit:
         {
            return "directive_scope_exit";
         }
#endif
         default:
            fd->print(llvm::errs());
            report_fatal_error("Plugin Error");
      }
   }
   const void* DumpBambuIR::IR_DECL_NAME(const void* t)
   {
      if(IR_CODE(t) == IRC(MODULE_UNIT_NODE))
         return nullptr;
      if(IR_CODE(t) == IRC(ALLOCAVARIABLE_VAL_NODE))
      {
         const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
         const llvm::Instruction* ti = av->alloc_inst;
         if(MDNode* N = ti->getMetadata("annotation"))
         {
            std::string allocaname = std::string(cast<MDString>(N->getOperand(0))->getString());
            if(identifierTable.find(allocaname) == identifierTable.end())
               identifierTable.insert(allocaname);
            const void* an = identifierTable.find(allocaname)->c_str();
            return assignCode(an, IRC(IDENTIFIER_NODE));
         }
         else
            return nullptr;
      }
      if(IR_CODE(t) == IRC(ORIGVARIABLE_VAL_NODE))
         return nullptr;
      if(IR_CODE(t) == IRC(FIELD_VAL_NODE))
      {
         const field_val_node* ty = reinterpret_cast<const field_val_node*>(t);
         return ty->name;
      }
      const llvm::Value* llvm_obj = reinterpret_cast<const llvm::Value*>(t);
      if(llvm_obj->hasName() && IR_CODE(t) == IRC(FUNCTION_VAL_NODE))
      {
         const llvm::Function* fd = cast<const llvm::Function>(llvm_obj);
         std::string declname;
         if(fd->isIntrinsic())
            declname = getIntrinsicName(fd);
         else
         {
            declname = getName(fd);
            int status;
            char* demangled_outbuffer = abi::__cxa_demangle(declname.c_str(), nullptr, nullptr, &status);
            if(status == 0)
            {
               if(std::string(demangled_outbuffer).find(':') == std::string::npos &&
                  std::string(demangled_outbuffer).find('(') != std::string::npos)
               {
                  declname = demangled_outbuffer;
                  auto parPos = declname.find('(');
                  assert(parPos != std::string::npos);
                  declname = declname.substr(0, parPos);
               }
               free(demangled_outbuffer);
            }
            else
               assert(demangled_outbuffer == nullptr);
         }
         if(identifierTable.find(declname) == identifierTable.end())
            identifierTable.insert(declname);
         const void* dn = identifierTable.find(declname)->c_str();
         return assignCode(dn, IRC(IDENTIFIER_NODE));
      }
      else if(IR_CODE(t) == IRC(ARGUMENT_VAL_NODE))
      {
         const llvm::Argument* arg = reinterpret_cast<const llvm::Argument*>(t);
         std::string declname;
         if(argNameTable.find(arg) != argNameTable.end())
         {
            declname = argNameTable.find(arg)->second;
         }
         else
         {
            const llvm::Function* currentFunction = arg->getParent();
            llvm::ModuleSlotTracker MST(currentFunction->getParent());
            MST.incorporateFunction(*currentFunction);
            auto id = MST.getLocalSlot(arg);
            if(id >= 0)
            {
               snprintf(buffer, LOCAL_BUFFER_LEN, "P%d", id);
               declname = buffer;
            }
            else
            {
               assert(llvm2index.find(t) != llvm2index.end());
               snprintf(buffer, LOCAL_BUFFER_LEN, "Pd%d", llvm2index.find(t)->second);
               declname = buffer;
            }
         }
         if(identifierTable.find(declname) == identifierTable.end())
            identifierTable.insert(declname);
         const void* dn = identifierTable.find(declname)->c_str();
         return assignCode(dn, IRC(IDENTIFIER_NODE));
      }
      else if(llvm_obj->hasName())
      {
         std::string declname = std::string(llvm_obj->getName().data());
         if(identifierTable.find(declname) == identifierTable.end())
            identifierTable.insert(declname);
         const void* dn = identifierTable.find(declname)->c_str();
         return assignCode(dn, IRC(IDENTIFIER_NODE));
      }
      else
         return nullptr;
   }

   const char* DumpBambuIR::IDENTIFIER_POINTER(const void* t) const
   {
      const char* ii = reinterpret_cast<const char*>(t);
      return ii;
   }

   int DumpBambuIR::IDENTIFIER_LENGTH(const void* t) const
   {
      const char* ii = reinterpret_cast<const char*>(t);
      return static_cast<int>(std::strlen(ii));
   }

   const void* DumpBambuIR::IR_DECL_SOURCE_LOCATION(const void* t) const
   {
      if(IR_CODE(t) == IRC(MODULE_UNIT_NODE))
         return nullptr;
      else if(IR_CODE(t) == IRC(FIELD_VAL_NODE))
         return nullptr;
      else if(IR_CODE(t) == IRC(ALLOCAVARIABLE_VAL_NODE))
         return nullptr;
      else if(IR_CODE(t) == IRC(ORIGVARIABLE_VAL_NODE))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return IR_DECL_SOURCE_LOCATION(ov->orig);
      }
      else if(IR_CODE(t) == IRC(ARGUMENT_VAL_NODE))
         return nullptr;
      else
      {
         const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
         return llvm_obj->hasMetadata() ? llvm_obj->getMetadata(llvm::LLVMContext::MD_dbg) : nullptr;
      }
   }

   DumpBambuIR::expanded_location DumpBambuIR::expand_location(const void* i) const
   {
      const llvm::MDNode* llvm_obj = reinterpret_cast<const llvm::MDNode*>(i);
      expanded_location res;
      if(dyn_cast<llvm::DIExpression>(llvm_obj) || dyn_cast<llvm::DIEnumerator>(llvm_obj) ||
         dyn_cast<llvm::DITemplateParameter>(llvm_obj) || dyn_cast<llvm::DIEnumerator>(llvm_obj) ||
         dyn_cast<llvm::DISubrange>(llvm_obj) || dyn_cast<llvm::GenericDINode>(llvm_obj) ||
         dyn_cast<llvm::MDTuple>(llvm_obj))
      {
      }
      else if(auto* di = dyn_cast<llvm::DIGlobalVariableExpression>(llvm_obj))
      {
         res = expand_location(di->getVariable());
      }
      else if(auto* di = dyn_cast<llvm::DIVariable>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DISubprogram>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DILocation>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
         res.line = di->getLine();
         res.column = di->getColumn();
      }
      else if(auto* di = dyn_cast<llvm::DILexicalBlock>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
         res.line = di->getLine();
         res.column = di->getColumn();
      }
      else if(auto* di = dyn_cast<llvm::DIMacroFile>(llvm_obj))
      {
         res.filename = di->getFile()->getFilename().data();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DIMacro>(llvm_obj))
      {
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DIImportedEntity>(llvm_obj))
      {
         // res.filename = di->getFile()->getFilename().data();
         // res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DIObjCProperty>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DICompileUnit>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
      }
      else if(auto* di = dyn_cast<llvm::DIFile>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
      }
      else if(auto* di = dyn_cast<llvm::DICompositeType>(llvm_obj))
      {
         if(di->getFilename().empty())
            res = expand_location(di->getBaseType());
         else
         {
            res.filename = di->getFilename().data();
            res.file = res.filename.c_str();
            res.line = di->getLine();
         }
      }
      else if(auto* di = dyn_cast<llvm::DIDerivedType>(llvm_obj))
      {
         if(di->getFilename().empty())
            res = expand_location(di->getBaseType());
         else
         {
            res.filename = di->getFilename().data();
            res.file = res.filename.c_str();
            res.line = di->getLine();
         }
      }
      else if(auto* di = dyn_cast<llvm::DIType>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DINamespace>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
#if PANDA_LLVM_CLANG_MAJOR == 4
         res.line = di->getLine();
#endif
      }
      else if(auto* di = dyn_cast<llvm::DIModule>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
      }
      else if(auto* di = dyn_cast<llvm::DILexicalBlockFile>(llvm_obj))
      {
         res.filename = di->getFilename().data();
         res.file = res.filename.c_str();
      }
      else
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "expand_location: unexpected location %d", llvm_obj->getMetadataID());
         report_fatal_error(buffer);
      }
      return res;
   }

   bool DumpBambuIR::ir_has_location(const void* g) const
   {
      if(IR_CODE(g) == IRC(IR_NOP) || IR_CODE(g) == IRC(IR_PHI_VIRTUAL))
         return false;
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);

      if(inst->hasMetadata() && inst->getMetadata(llvm::LLVMContext::MD_dbg) != nullptr)
         return true;
      else
         return MetaDataMap.find(inst) != MetaDataMap.end();
   }

   const void* DumpBambuIR::ir_location(const void* g) const
   {
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      if(MetaDataMap.find(inst) != MetaDataMap.end())
         return MetaDataMap.find(inst)->second;
      else if(inst->hasMetadata() && inst->getMetadata(llvm::LLVMContext::MD_dbg) != nullptr)
         return inst->getMetadata(llvm::LLVMContext::MD_dbg);
      else
         return nullptr;
   }

   const DumpBambuIR::pt_info* DumpBambuIR::IR_SSA_NAME_PTR_INFO(const void* t) const
   {
      const ssa_node* ssa = reinterpret_cast<const ssa_node*>(t);
      return ssa->ptr_info.valid ? &ssa->ptr_info : nullptr;
   }

   void DumpBambuIR::dump_pt_solution(const DumpBambuIR::pt_solution* pt, const char* first_tag, const char* second_tag)
   {
      if(pt->anything)
         serialize_string_field(first_tag, "anything");
      if(pt->nonlocal)
         serialize_string_field(first_tag, "nonlocal");
      if(pt->escaped)
         serialize_string_field(first_tag, "escaped");
      if(pt->ipa_escaped)
         serialize_string_field(first_tag, "ipa_escaped");
      if(pt->null)
         serialize_string_field(first_tag, "null");

      for(auto var : pt->vars)
         serialize_child(second_tag, var);
   }

   int DumpBambuIR::getBB_index(const llvm::BasicBlock* BB)
   {
      assert(BB != nullptr);
      if(BB_index_map.find(BB) == BB_index_map.end())
      {
         BB_index_map[BB] = last_BB_index;
         int res = last_BB_index;
         ++last_BB_index;
         return res;
      }
      else
         return BB_index_map.find(BB)->second;
   }

   static unsigned getPredicateTEC(const llvm::Instruction* inst)
   {
      return cast<const llvm::CmpInst>(inst)->getPredicate();
   }

   static unsigned getPredicateTEC(const llvm::ConstantExpr* ce)
   {
#if PANDA_LLVM_CLANG_MAJOR < 19
      return ce->getPredicate();
#else
      // NOTE: Support for ICmpInst and FCmpInst was removed for ConstantExpr
      return llvm::ICmpInst::BAD_ICMP_PREDICATE;
#endif
   }

#if PANDA_LLVM_CLANG_MAJOR > 7
   static Intrinsic::ID getIntrinsicIDTEC(const llvm::Instruction* inst)
   {
      auto ci = llvm::dyn_cast<const llvm::CallInst>(inst);
      assert(ci);
      if(ci->getCalledFunction() && ci->getCalledFunction()->isIntrinsic())
      {
         return ci->getCalledFunction()->getIntrinsicID();
      }
      return (Intrinsic::ID)0;
   }
   static Intrinsic::ID getIntrinsicIDTEC(const llvm::ConstantExpr* ce)
   {
#if PANDA_LLVM_CLANG_MAJOR > 7
#if PANDA_LLVM_CLANG_MAJOR > 9
      auto ci = llvm::dyn_cast<llvm::CallInst>(ce->getAsInstruction());
#else
      auto ci = llvm::dyn_cast<llvm::CallInst>(const_cast<llvm::ConstantExpr*>(ce)->getAsInstruction());
#endif
      assert(ci);
      if(ci->getCalledFunction() && ci->getCalledFunction()->isIntrinsic())
      {
         return ci->getCalledFunction()->getIntrinsicID();
      }
#endif
      return (Intrinsic::ID)0;
   }
#endif

   template <class InstructionOrConstantExpr>
   DumpBambuIR::ir_codes DumpBambuIR::ir_node_code(InstructionOrConstantExpr* inst)
   {
      auto opcode = inst->getOpcode();
      switch(opcode)
      {
#if PANDA_LLVM_CLANG_MAJOR >= 10
         case llvm::Instruction::FNeg:
            return IRC(NEG_NODE);
         case llvm::Instruction::Freeze:
            return IRC(SSA_NODE);
#endif
         case llvm::Instruction::Add:
            return IRC(ADD_NODE);
         case llvm::Instruction::FAdd:
            return IRC(ADD_NODE);
         case llvm::Instruction::Sub:
            return IRC(SUB_NODE);
         case llvm::Instruction::FSub:
            return IRC(SUB_NODE);
         case llvm::Instruction::Mul:
            return IRC(MUL_NODE);
         case llvm::Instruction::FMul:
            return IRC(MUL_NODE);
         case llvm::Instruction::UDiv:
            return IRC(IDIV_NODE);
         case llvm::Instruction::SDiv:
            return IRC(IDIV_NODE);
         case llvm::Instruction::FDiv:
            return IRC(FDIV_NODE);
         case llvm::Instruction::URem:
            return IRC(IREM_NODE);
         case llvm::Instruction::SRem:
            return IRC(IREM_NODE);
         case llvm::Instruction::FRem:
            return IRC(FREM_NODE);
         // Logical operators (integer operands)
         case llvm::Instruction::Shl: // Shift left  (logical)
            return IRC(SHL_NODE);
         case llvm::Instruction::LShr: // Shift right (logical)
            return IRC(SHR_NODE);
         case llvm::Instruction::AShr: // Shift right (arithmetic)
            return IRC(SHR_NODE);
         case llvm::Instruction::And:
            return IRC(AND_NODE);
         case llvm::Instruction::Or:
            return IRC(IOR_NODE);
         case llvm::Instruction::Xor:
            return IRC(XOR_NODE);

         case llvm::Instruction::Store:
            return IRC(SSA_NODE);
         case llvm::Instruction::Load:
            return IRC(MEM_ACCESS_NODE);

         case llvm::Instruction::BitCast:
            return IRC(BITCAST_NODE);
         case llvm::Instruction::Call:
         {
#if PANDA_LLVM_CLANG_MAJOR > 7
            auto CallID = getIntrinsicIDTEC(inst);
            if(CallID == llvm::Intrinsic::sadd_sat || CallID == llvm::Intrinsic::uadd_sat)
               return IRC(ADD_SAT_NODE);
            else if(CallID == llvm::Intrinsic::ssub_sat || CallID == llvm::Intrinsic::usub_sat)
               return IRC(SUB_SAT_NODE);
#if PANDA_LLVM_CLANG_MAJOR > 11
            else if(CallID == llvm::Intrinsic::fshl)
               return IRC(FSHL_NODE);
            else if(CallID == llvm::Intrinsic::fshr)
               return IRC(FSHR_NODE);
            else if(CallID == llvm::Intrinsic::abs)
               return IRC(ABS_NODE);
            else if(CallID == llvm::Intrinsic::fabs)
               return IRC(ABS_NODE);
            else if(CallID == llvm::Intrinsic::smax)
               return IRC(MAX_NODE);
            else if(CallID == llvm::Intrinsic::smin)
               return IRC(MIN_NODE);
            else if(CallID == llvm::Intrinsic::umax)
               return IRC(MAX_NODE);
            else if(CallID == llvm::Intrinsic::umin)
               return IRC(MIN_NODE);
#endif
            else
#endif
               return IRC(CALL_NODE);
         }
         case llvm::Instruction::SExt:
         case llvm::Instruction::ZExt:
         case llvm::Instruction::Trunc:
         case llvm::Instruction::PtrToInt:
         case llvm::Instruction::IntToPtr:
         case llvm::Instruction::FPExt:
         case llvm::Instruction::FPTrunc:
            return IRC(NOP_NODE);

         case llvm::Instruction::FPToSI:
         case llvm::Instruction::FPToUI:
            return IRC(FPTOI_NODE);

         case llvm::Instruction::UIToFP:
         case llvm::Instruction::SIToFP:
            return IRC(ITOFP_NODE);

         case llvm::Instruction::FCmp:
         case llvm::Instruction::ICmp:
         {
            auto predicate = getPredicateTEC(inst);
            switch(predicate)
            {
               case llvm::ICmpInst::FCMP_OEQ:
                  return IRC(FCMP_OEQ);
               case llvm::ICmpInst::ICMP_EQ:
                  return IRC(EQ_NODE);
               case llvm::ICmpInst::FCMP_ONE:
                  return IRC(FCMP_ONE);
               case llvm::ICmpInst::ICMP_NE:
                  return IRC(NE_NODE);
               case llvm::ICmpInst::FCMP_OGT:
               case llvm::ICmpInst::ICMP_UGT:
               case llvm::ICmpInst::ICMP_SGT:
                  return IRC(GT_NODE);
               case llvm::ICmpInst::FCMP_OGE:
               case llvm::ICmpInst::ICMP_UGE:
               case llvm::ICmpInst::ICMP_SGE:
                  return IRC(GE_NODE);
               case llvm::ICmpInst::FCMP_OLT:
               case llvm::ICmpInst::ICMP_ULT:
               case llvm::ICmpInst::ICMP_SLT:
                  return IRC(LT_NODE);
               case llvm::ICmpInst::FCMP_OLE:
               case llvm::ICmpInst::ICMP_ULE:
               case llvm::ICmpInst::ICMP_SLE:
                  return IRC(LE_NODE);
               case llvm::ICmpInst::FCMP_UEQ:
                  return IRC(FCMP_UEQ);
               case llvm::ICmpInst::FCMP_UNE:
                  return IRC(FCMP_UNE);
               case llvm::ICmpInst::FCMP_UGT:
                  return IRC(UNGT_NODE);
               case llvm::ICmpInst::FCMP_UGE:
                  return IRC(UNGE_NODE);
               case llvm::ICmpInst::FCMP_ULT:
                  return IRC(UNLT_NODE);
               case llvm::ICmpInst::FCMP_ULE:
                  return IRC(UNLE_NODE);
               case llvm::ICmpInst::FCMP_ORD:
                  return IRC(FCMP_ORD);
               case llvm::ICmpInst::FCMP_UNO:
                  return IRC(FCMP_UNO);
               default:
                  llvm::errs() << "ir_expr_code::ICmpInst kind not supported: " << predicate << "\n";
                  stream.close();
                  report_fatal_error("Plugin Error");
            }
         }

         case llvm::Instruction::Select:
         {
            return IRC(SELECT_NODE);
         }
         case llvm::Instruction::Br:
         {
            assert(cast<const llvm::BranchInst>(inst)->isConditional());
            return IRC(SSA_NODE);
         }
         case llvm::Instruction::InsertValue:
         {
            return IRC(INSERTVALUE);
         }
         case llvm::Instruction::ExtractValue:
         {
            return IRC(EXTRACTVALUE);
         }
         case llvm::Instruction::InsertElement:
         {
            return IRC(INSERTELEMENT);
         }
         case llvm::Instruction::ExtractElement:
         {
            return IRC(EXTRACTELEMENT);
         }
         case llvm::Instruction::ShuffleVector:
         {
            return IRC(SHUFFLEVECTOR);
         }
         default:
            llvm::errs() << "ir_expr_code kind not supported: " << ValueTyNames[llvm::Value::InstructionVal + opcode]
                         << "\n";
            stream.close();
            report_fatal_error("Plugin Error");
      }
   }
   DumpBambuIR::ir_codes DumpBambuIR::ir_expr_code(const void* g)
   {
      ir_codes code = ir_code(g);
      if(code == IRC(IR_ASSIGN) || code == IRC(IR_COND))
      {
         const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
         assert(inst);
         return ir_node_code(inst);
      }
      else
      {
         assert(code == IRC(IR_CALL));
         return IRC(CALL_NODE);
      }
   }

   const void* DumpBambuIR::ir_assign_lhs(const void* g)
   {
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      auto currentFunction = inst->getFunction();
      if(isa<llvm::StoreInst>(inst))
      {
         const llvm::StoreInst& store = cast<const llvm::StoreInst>(*inst);
         auto val_zero = llvm::APInt(32, 0);
         if(uicTable.find(val_zero) == uicTable.end())
         {
            uicTable[val_zero] = assignCodeAuto(llvm::ConstantInt::get(inst->getContext(), val_zero));
         }
         const void* zero = uicTable.find(val_zero)->second;
         auto addr = getOperand(store.getPointerOperand(), currentFunction);
         auto ty = store.getValueOperand()->getType();
         auto isVecType = ty->isVectorTy();
         auto type = assignCodeType(ty);
         auto written_obj_size = ty->isSized() ? DL->getTypeAllocSizeInBits(ty) : 8ULL;
         const auto funName = getName(store.getFunction());
         const auto demangled = getDemangled(funName);
         bool is_a_top_parameter = isa<llvm::Argument>(store.getPointerOperand()) &&
                                   (llvm::find(TopFunctionNames, funName) != TopFunctionNames.end() ||
                                    llvm::find(TopFunctionNames, demangled) != TopFunctionNames.end());
         // Refine the instruction alignment with the base object alignment and
         // any constant byte offset accumulated along the pointer derivation.
         // This keeps typed accesses on naturally aligned parameters aligned,
         // while still catching explicit byte-offset or cast-based misalignment.
#if PANDA_LLVM_CLANG_MAJOR < 16
         unsigned long long effectiveStoreAlign = store.getAlignment();
#else
         unsigned long long effectiveStoreAlign = store.getAlign().value();
#endif
         effectiveStoreAlign = getEffectiveMemoryAccessAlignment(store.getPointerOperand(), effectiveStoreAlign, DL);
         if(effectiveStoreAlign && written_obj_size > (8 * effectiveStoreAlign) && !is_a_top_parameter && !isVecType)
         {
            return build1(IRC(UNALIGNED_MEM_ACCESS_NODE), type, addr);
         }
         else
            return build1(IRC(MEM_ACCESS_NODE), type, addr);
      }
      return getSSA(inst, g, currentFunction, false);
   }

   const void* DumpBambuIR::ir_assign_rhs_alloca(const void* g)
   {
      const llvm::AllocaInst* inst = reinterpret_cast<const llvm::AllocaInst*>(g);
      auto type = assignCodeType(inst->getType());
      const void* allocaVar;
      if(index2alloca_var.find(g) == index2alloca_var.end())
      {
         auto& av = index2alloca_var[g];
         av.alloc_inst = inst;
         std::set<const llvm::User*> visited;
         visited.insert(inst);
         const auto& TLI = GetTLI(*const_cast<llvm::Function*>(inst->getFunction()));
         av.addr = temporary_addr_check(inst, visited, TLI);
         allocaVar = assignCode(&av, IRC(ALLOCAVARIABLE_VAL_NODE));
      }
      else
         allocaVar = &index2alloca_var.find(g)->second;
      return build1(IRC(ADDR_NODE), type, allocaVar);
   }

   const void* DumpBambuIR::IR_DECL_ABSTRACT_ORIGIN(const void* t)
   {
      const void* origVar;
      if(index2orig_var.find(t) == index2orig_var.end())
      {
         auto& ov = index2orig_var[t];
         ov.orig = t;
         origVar = assignCode(&ov, IRC(ORIGVARIABLE_VAL_NODE));
      }
      else
         origVar = &index2orig_var.find(t)->second;
      return origVar;
   }

   const void* DumpBambuIR::ir_assign_rhs_getelementptr(const void* g)
   {
      llvm::Instruction* inst = const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(g));
      llvm::Function* currentFunction = inst->getFunction();
      return LowerGetElementPtr(inst->getType(), inst, currentFunction);
   }
   template <class insert_or_extract>
   void accumulateConstantOffset(const llvm::DataLayout* DL, llvm::APInt& Offset, insert_or_extract* ioe)
   {
      auto currTy = ioe->getAggregateOperand()->getType();
      LLVM_DEBUG({
         llvm::dbgs() << "Instruction   : " << *ioe << "\n"
                      << "Aggregate type: " << *currTy << "\n";
      });
      for(auto idx : ioe->indices())
      {
         LLVM_DEBUG(llvm::dbgs() << " Access index: " << idx << " -> " << *currTy << " -> ");
         if(llvm::isa<StructType>(currTy))
         {
            StructType* STy = llvm::cast<llvm::StructType>(currTy);
            currTy = STy->getStructElementType(idx);
            auto SL = DL->getStructLayout(STy);
            Offset += SL->getElementOffsetInBits(idx);
         }
         else if(llvm::isa<llvm::ArrayType>(currTy))
         {
            currTy = llvm::cast<llvm::ArrayType>(currTy)->getElementType();
            Offset += idx * DL->getTypeAllocSizeInBits(currTy);
         }
         else
         {
            report_fatal_error("type not supported");
         }
         LLVM_DEBUG(llvm::dbgs() << Offset << "\n");
      }
   }
   const void* DumpBambuIR::ir_assign_rhs_insertvalue(const void* g)
   {
      auto inst = const_cast<llvm::InsertValueInst*>(reinterpret_cast<const llvm::InsertValueInst*>(g));
      auto ty = inst->getType();
      auto type = assignCodeType(ty);
      auto currentFunction = inst->getFunction();
      auto op0Value = inst->getAggregateOperand();
      auto op0 = getOperand(op0Value, currentFunction);
      auto op1 = getOperand(inst->getInsertedValueOperand(), currentFunction);
      unsigned SizeInBits;
      if(op0Value->getType()->isPtrOrPtrVectorTy())
      {
         SizeInBits = DL->getPointerTypeSizeInBits(op0Value->getType());
      }
      else
      {
         SizeInBits = 64;
      }
      llvm::APInt Offset(SizeInBits, 0);
      accumulateConstantOffset(DL, Offset, inst);
      auto offset_node = assignCodeAuto(llvm::ConstantInt::get(inst->getContext(), Offset));
      return build3(IRC(INSERTVALUE), type, op0, op1, offset_node);
   }

   const void* DumpBambuIR::ir_assign_rhs_extractvalue(const void* g)
   {
      auto inst = const_cast<llvm::ExtractValueInst*>(reinterpret_cast<const llvm::ExtractValueInst*>(g));
      auto ty = inst->getType();
      auto type = assignCodeType(ty);
      auto currentFunction = inst->getFunction();
      auto op0 = getOperand(inst->getAggregateOperand(), currentFunction);
      auto op0Value = inst->getAggregateOperand();
      unsigned SizeInBits;
      if(op0Value->getType()->isPtrOrPtrVectorTy())
      {
         SizeInBits = DL->getPointerTypeSizeInBits(op0Value->getType());
      }
      else
      {
         SizeInBits = 64;
      }
      llvm::APInt Offset(SizeInBits, 0);
      accumulateConstantOffset(DL, Offset, inst);
      auto offset_node = assignCodeAuto(llvm::ConstantInt::get(inst->getContext(), Offset));
      return build2(IRC(EXTRACTVALUE), type, op0, offset_node);
   }

   bool DumpBambuIR::temporary_addr_check(const llvm::User* inst, std::set<const llvm::User*>& visited,
                                          const llvm::TargetLibraryInfo& TLI)
   {
      for(auto U : inst->users())
      {
         if(visited.find(U) == visited.end())
         {
            visited.insert(U);
            if(isa<llvm::LoadInst>(U))
            {
               /// do nothing
            }
            else if(isa<llvm::StoreInst>(U))
            {
               if(U->getOperand(0) == inst)
               {
                  return false;
               }
            }
            else if(isa<llvm::PHINode>(U) || isa<llvm::GetElementPtrInst>(U) || isa<llvm::BitCastInst>(U) ||
                    isa<llvm::PtrToIntInst>(U) || isa<llvm::IntToPtrInst>(U) ||
                    cast<llvm::Instruction>(U)->getOpcode() == llvm::Instruction::Sub ||
                    cast<llvm::Instruction>(U)->getOpcode() == llvm::Instruction::Add ||
                    cast<llvm::Instruction>(U)->getOpcode() == llvm::Instruction::AShr ||
                    cast<llvm::Instruction>(U)->getOpcode() == llvm::Instruction::And ||
                    cast<llvm::Instruction>(U)->getOpcode() == llvm::Instruction::Or)
            {
               if(isa<llvm::PHINode>(U) && (!PtoSets_AA || !is_PTS(PtoSets_AA->PE(U), TLI)))
               {
                  return false;
               }
               auto res = temporary_addr_check(U, visited, TLI);
               if(!res)
               {
                  return res;
               }
            }
            else if(isa<llvm::CmpInst>(U))
               ;
            else if(isa<llvm::SelectInst>(U))
            {
               auto si = cast<llvm::SelectInst>(U);
               if(si->getOperand(0) == inst)
               {
                  /// do nothing
               }
               else
               {
                  if(!PtoSets_AA || !is_PTS(PtoSets_AA->PE(U), TLI))
                  {
                     return false;
                  }
                  auto res = temporary_addr_check(U, visited, TLI);
                  if(!res)
                  {
                     return res;
                  }
               }
            }
            else if(isa<llvm::CallInst>(U))
            {
               if(cast<llvm::CallInst>(U)->getCalledFunction() &&
                  (getName(cast<llvm::CallInst>(U)->getCalledFunction()).find("_write_bambu_internal") !=
                       std::string::npos ||
                   getName(cast<llvm::CallInst>(U)->getCalledFunction()).find("_read_bambu_internal") !=
                       std::string::npos))
               {
                  /// do nothing
               }
               else
               {
                  return false;
               }
            }
            else if(isa<llvm::ReturnInst>(U))
            {
               return false;
            }
            else
            {
               return false;
            }
         }
      }
      return true;
   }

   void DumpBambuIR::add_alloca_pt_solution(const void* lhs, const void* rhs)
   {
      ssa_node* lhs_ssa = const_cast<ssa_node*>(reinterpret_cast<const ssa_node*>(lhs));
      const void* vd = IR_OPERAND(rhs, 0);
      lhs_ssa->ptr_info.valid = true;
      lhs_ssa->ptr_info.pt.vars.insert(vd);
   }

   const void* DumpBambuIR::getNop(const llvm::Value* operand, const void* parent)
   {
      if(index2ir_nop.find(operand) == index2ir_nop.end())
      {
         auto& gn = index2ir_nop[operand];
         assignCode(&gn, IRC(IR_NOP));
         gn.parent = assignCodeAuto(parent);
      }
      return &index2ir_nop.find(operand)->second;
   }

   template <class InstructionOrConstantExpr>
   bool DumpBambuIR::isSignedInstruction(const InstructionOrConstantExpr* inst) const
   {
      auto opcode = inst->getOpcode();
      switch(opcode)
      {
         case llvm::Instruction::Select:
         {
            auto op0 = inst->getOperand(1);
            auto op1 = inst->getOperand(2);
            if(dyn_cast<llvm::ConstantInt>(op0) && dyn_cast<InstructionOrConstantExpr>(op1))
               return isSignedResult(dyn_cast<InstructionOrConstantExpr>(op1));
            else if(dyn_cast<llvm::ConstantInt>(op1) && dyn_cast<InstructionOrConstantExpr>(op0))
               return isSignedResult(dyn_cast<InstructionOrConstantExpr>(op0));
            else if(dyn_cast<InstructionOrConstantExpr>(op0) && dyn_cast<InstructionOrConstantExpr>(op1))
               return isSignedResult(dyn_cast<InstructionOrConstantExpr>(op0)) &&
                      isSignedResult(dyn_cast<InstructionOrConstantExpr>(op1));
            else
               return false;
         }
         case llvm::Instruction::Add:
         case llvm::Instruction::Sub:
         case llvm::Instruction::Mul:
         {
            auto op0 = inst->getOperand(0);
            auto op1 = inst->getOperand(1);
            if(dyn_cast<llvm::ConstantInt>(op0) && dyn_cast<InstructionOrConstantExpr>(op1))
               return isSignedResult(dyn_cast<InstructionOrConstantExpr>(op1));
            else if(dyn_cast<llvm::ConstantInt>(op1) && dyn_cast<InstructionOrConstantExpr>(op0))
               return isSignedResult(dyn_cast<InstructionOrConstantExpr>(op0));
            else if(dyn_cast<InstructionOrConstantExpr>(op0) && dyn_cast<InstructionOrConstantExpr>(op1))
               return isSignedResult(dyn_cast<InstructionOrConstantExpr>(op0)) &&
                      isSignedResult(dyn_cast<InstructionOrConstantExpr>(op1));
            else
               return false;
         }
#if PANDA_LLVM_CLANG_MAJOR > 7
         case llvm::Instruction::Call:
         {
            auto CallID = getIntrinsicIDTEC(inst);
            return (CallID == llvm::Intrinsic::sadd_sat || CallID == llvm::Intrinsic::ssub_sat
#if PANDA_LLVM_CLANG_MAJOR > 11
                    || CallID == llvm::Intrinsic::smax || CallID == llvm::Intrinsic::smin ||
                    CallID == llvm::Intrinsic::abs
#endif
            );
         }
#endif
         default:
            return false;
      }
   }

   template <class InstructionOrConstantExpr>
   bool DumpBambuIR::isSignedResult(const InstructionOrConstantExpr* inst) const
   {
      auto opcode = inst->getOpcode();
      switch(opcode)
      {
         case llvm::Instruction::SDiv:
         case llvm::Instruction::SRem:
         case llvm::Instruction::AShr:
         case llvm::Instruction::FPToSI:
         case llvm::Instruction::SExt:
            return true;
         default:
            return isSignedInstruction(inst);
      }
   }
   const llvm::Type* DumpBambuIR::getCondSignedResult(const llvm::Value* operand, const llvm::Type* type) const
   {
      if(isa<llvm::Instruction>(operand))
      {
         const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(operand);
         assert(inst);
         if(isSignedResult(inst))
            return AddSignedTag(type);
         else
            return type;
      }
      else
         return type;
   }

   template <class InstructionOrConstantExpr>
   bool DumpBambuIR::isSignedOperand(const InstructionOrConstantExpr* inst, unsigned index) const
   {
      if(!inst->getOperand(index)->getType()->isIntegerTy())
         return false;
      auto opcode = inst->getOpcode();
      switch(opcode)
      {
         case llvm::Instruction::AShr:
            return index == 0;
         case llvm::Instruction::SDiv:
         case llvm::Instruction::SRem:
         case llvm::Instruction::SExt:
         case llvm::Instruction::SIToFP:
            return true;
         case llvm::Instruction::ICmp:
         {
            auto predicate = getPredicateTEC(inst);
            switch(predicate)
            {
               case llvm::ICmpInst::ICMP_SGT:
               case llvm::ICmpInst::ICMP_SGE:
               case llvm::ICmpInst::ICMP_SLT:
               case llvm::ICmpInst::ICMP_SLE:
                  return true;
               default:
                  return false;
            }
         }
         case llvm::Instruction::Select:
         {
            if(index == 0)
            {
               return false;
            }
            else
            {
               return isSignedInstruction(inst);
            }
         }
         case llvm::Instruction::Add:
         case llvm::Instruction::Sub:
         case llvm::Instruction::Mul:
         {
            return isSignedInstruction(inst);
         }
         case llvm::Instruction::Call:
         {
            return isSignedInstruction(inst);
         }
         default:
            return false;
      }
   }

   template <class InstructionOrConstantExpr>
   bool DumpBambuIR::isUnsignedOperand(const InstructionOrConstantExpr* inst, unsigned index) const
   {
      if(!inst->getOperand(index)->getType()->isIntegerTy())
         return false;
      auto opcode = inst->getOpcode();
      switch(opcode)
      {
         case llvm::Instruction::AShr:
            return index != 0;
         case llvm::Instruction::Shl:
         case llvm::Instruction::LShr:
         case llvm::Instruction::UDiv:
         case llvm::Instruction::URem:
         case llvm::Instruction::ZExt:
         case llvm::Instruction::Trunc:
         case llvm::Instruction::UIToFP:
         case llvm::Instruction::IntToPtr:
         case llvm::Instruction::PtrToInt:
         case llvm::Instruction::And:
         case llvm::Instruction::Or:
         case llvm::Instruction::Xor:
         case llvm::Instruction::PHI:
         case llvm::Instruction::Ret:
         case llvm::Instruction::Store:
         case llvm::Instruction::Switch:
            return true;
         case llvm::Instruction::ICmp:
         {
            auto predicate = getPredicateTEC(inst);
            switch(predicate)
            {
               case llvm::ICmpInst::ICMP_UGT:
               case llvm::ICmpInst::ICMP_UGE:
               case llvm::ICmpInst::ICMP_ULT:
               case llvm::ICmpInst::ICMP_ULE:
               case llvm::ICmpInst::ICMP_EQ:
               case llvm::ICmpInst::ICMP_NE:
                  return true;
               default:
                  return false;
            }
         }
         case llvm::Instruction::Select:
         {
            if(index == 0)
            {
               return true;
            }
            else
            {
               return !isSignedInstruction(inst);
            }
         }
         case llvm::Instruction::Add:
         case llvm::Instruction::Sub:
         case llvm::Instruction::Mul:
         {
            return !isSignedInstruction(inst);
         }
         case llvm::Instruction::Call:
         {
            return !isSignedInstruction(inst);
         }
         default:
            return false;
      }
   }

   const void* DumpBambuIR::getSSA(const llvm::Value* operand, const void* def_stmt,
                                   const llvm::Function* currentFunction, bool isDefault)
   {
      auto isVirtual = operand->getValueID() == llvm::Value::MemoryDefVal ||
                       operand->getValueID() == llvm::Value::MemoryUseVal ||
                       operand->getValueID() == llvm::Value::MemoryPhiVal;
      auto key = std::make_pair(def_stmt, isVirtual);
      if(index2ssa_name.find(key) == index2ssa_name.end())
      {
         int ssa_vers;
         auto& sn = index2ssa_name[key];
         assignCode(&sn, IRC(SSA_NODE));
         if(isVirtual)
         {
            if(memoryaccess2ssaindex.find(def_stmt) == memoryaccess2ssaindex.end())
            {
               ssa_vers = last_memory_ssa_vers;
               --last_memory_ssa_vers;
               memoryaccess2ssaindex[def_stmt] = ssa_vers;
            }
            else
               ssa_vers = memoryaccess2ssaindex.find(def_stmt)->second;
            sn.type = assignCodeType(llvm::Type::getVoidTy(currentFunction->getContext()));
            sn.isVirtual = true;
         }
         else
         {
            assert(currentFunction != nullptr);
            assert(currentFunction->getParent());
            llvm::ModuleSlotTracker MST(currentFunction->getParent());
            MST.incorporateFunction(*currentFunction);
            ssa_vers = MST.getLocalSlot(operand);
            if(ssa_vers < 0)
            {
               if(memoryaccess2ssaindex.find(operand) == memoryaccess2ssaindex.end())
               {
                  ssa_vers = last_memory_ssa_vers;
                  --last_memory_ssa_vers;
                  memoryaccess2ssaindex[operand] = ssa_vers;
               }
               else
                  ssa_vers = memoryaccess2ssaindex.find(operand)->second;
            }
            assert(ssa_vers >= 0);
            sn.type = assignCodeType(getCondSignedResult(operand, operand->getType()));

            if(operand->getType()->isPointerTy() && PtoSets_AA)
            {
               auto varId = PtoSets_AA->PE(operand);
               const auto& TLI = GetTLI(*const_cast<llvm::Function*>(currentFunction));
               if(is_PTS(varId, TLI, true))
               {
                  const std::vector<u32>* pts = PtoSets_AA->pointsToSet(varId);
                  for(auto var : *pts)
                  {
                     if(PtoSets_AA->is_any(var))
                     {
                        sn.ptr_info.pt.anything = true;
                     }
                     else
                     {
                        auto val = PtoSets_AA->getValue(var);
                        assert(val);
                        assert(!dyn_cast<llvm::Argument>(val));
                        sn.ptr_info.valid = true;
                        auto vid = val->getValueID();
                        if(vid == llvm::Value::InstructionVal + llvm::Instruction::Alloca)
                           sn.ptr_info.pt.vars.insert(IR_OPERAND(ir_assign_rhs_alloca(val), 0));
                        else if(vid == llvm::Value::GlobalVariableVal)
                           sn.ptr_info.pt.vars.insert(assignCodeAuto(val));
                        else if(vid == llvm::Value::FunctionVal)
                        {
                           /// pointers to function are managed as point to anything objects
                           sn.ptr_info.pt.anything = true;
                           sn.ptr_info.valid = false;
                           //                           sn.ptr_info.pt.vars.push_back(assignCodeAuto(val));
                        }
                        else if(llvm::dyn_cast<const llvm::CallInst>(val))
                        {
                           /// malloc like functions are managed as point to anything objects
                           sn.ptr_info.pt.anything = true;
                           sn.ptr_info.valid = false;
                           //                           sn.ptr_info.pt.vars.push_back(assignCodeAuto(val));
                        }
                        else
                        {
                           val->print(llvm::errs());
                           stream.close();
                           report_fatal_error(
                               ("unexpected pointer to variable " + std::string(ValueTyNames[val->getValueID()]))
                                   .c_str());
                        }
                     }
                  }
               }
            }
         }
         sn.vers = ssa_vers;
         assert(HAS_CODE(def_stmt));
         sn.def_stmts = def_stmt;
         sn.isDefault = isDefault;
      }
      return &index2ssa_name.find(key)->second;
   }

   bool DumpBambuIR::is_PTS(unsigned int varId, const llvm::TargetLibraryInfo& TLI, bool with_all)
   {
      if(varId != NOVAR_ID && (with_all || PtoSets_AA->is_single(varId)) && !PtoSets_AA->has_malloc_obj(varId, &TLI))
      {
         if(with_all)
            return true;
         const std::vector<u32>* pts = PtoSets_AA->pointsToSet(varId);
         for(auto var : *pts)
         {
            if(PtoSets_AA->is_any(var))
               return false;
            auto val = PtoSets_AA->getValue(var);
            assert(val);
            if(dyn_cast<llvm::Argument>(val) || dyn_cast<llvm::Function>(val) || dyn_cast<llvm::CallInst>(val))
               return false;
         }
         return true;
      }
      else
         return false;
   }

   bool DumpBambuIR::is_virtual_ssa(const void* t) const
   {
      const ssa_node* ssa = reinterpret_cast<const ssa_node*>(t);
      return ssa->isVirtual;
   }

   bool DumpBambuIR::IR_SSA_NAME_IS_DEFAULT_DEF(const void* t) const
   {
      const ssa_node* ssa = reinterpret_cast<const ssa_node*>(t);
      return ssa->isDefault;
   }

   const void* DumpBambuIR::LowerGetElementPtrOffset(const llvm::GEPOperator* gep_op,
                                                     const llvm::Function* currentFunction, const void*& base_node,
                                                     bool& isZero)
   {
      if(gep_op->hasAllConstantIndices())
      {
         llvm::APInt OffsetAI(DL->getPointerTypeSizeInBits(gep_op->getType()), 0);
#ifndef NDEBUG
         auto resVal =
#endif
             gep_op->accumulateConstantOffset(*DL, OffsetAI);
         assert(resVal);
         isZero = !OffsetAI;
         return assignCodeAuto(llvm::ConstantInt::get(gep_op->getContext(), OffsetAI));
      }
      else
      {
         llvm::APInt ConstantIndexOffset(DL->getPointerTypeSizeInBits(gep_op->getType()), 0);
         for(llvm::gep_type_iterator GTI = llvm::gep_type_begin(gep_op), GTE = llvm::gep_type_end(gep_op); GTI != GTE;
             ++GTI)
         {
            llvm::ConstantInt* OpC = dyn_cast<llvm::ConstantInt>(GTI.getOperand());
            if(!OpC)
            {
               if(GTI.getStructTypeOrNull())
               {
                  report_fatal_error("unexpected condition: struct LowerGetElementPtrOffset");
                  // continue;
               }
               // For array or vector indices, scale the index by the size of the type.
               auto index = getOperand(GTI.getOperand(), currentFunction);
               auto index_type = IR_TYPE_NODE(index);
               bool isSignedIndexType = CheckSignedTag(index_type);
               auto array_elmt_size =
                   llvm::APInt(ConstantIndexOffset.getBitWidth(), DL->getTypeAllocSize(GTI.getIndexedType()));
               auto array_elmt_sizeCI = llvm::ConstantInt::get(gep_op->getContext(), array_elmt_size);
               auto array_elmt_sizeCI_type = array_elmt_sizeCI->getType();
               auto array_elmt_size_node = assignCodeAuto(array_elmt_sizeCI);
               if(isSignedIndexType)
               {
                  if(index2integer_cst_signed.find(array_elmt_size_node) == index2integer_cst_signed.end())
                  {
                     auto& ics_obj = index2integer_cst_signed[array_elmt_size_node];
                     auto type_operand = IR_TYPE_NODE(array_elmt_size_node);
                     ics_obj.ic = array_elmt_size_node;
                     ics_obj.type = AddSignedTag(type_operand);
                     array_elmt_size_node = assignCode(&ics_obj, IRC(CONSTANT_INT_VAL_NODE_SIGNED));
                  }
                  else
                     array_elmt_size_node = &index2integer_cst_signed.find(array_elmt_size_node)->second;
               }
               auto index_times_size = build2(
                   IRC(MUL_NODE), isSignedIndexType ? AddSignedTag(array_elmt_sizeCI_type) : array_elmt_sizeCI_type,
                   index, array_elmt_size_node);
               if(isSignedIndexType)
                  index_times_size = build1(IRC(NOP_NODE), array_elmt_sizeCI_type, index_times_size);
               auto accu = build2(IRC(GEP_NODE), IR_TYPE_NODE(base_node), base_node, index_times_size);
               base_node = accu;
               continue;
            }
            if(OpC->isZero())
               continue;

            // Handle a struct index, which adds its field offset to the pointer.
            if(llvm::StructType* STy = GTI.getStructTypeOrNull())
            {
               unsigned ElementIdx = OpC->getZExtValue();
               const llvm::StructLayout* SL = DL->getStructLayout(STy);
               ConstantIndexOffset += llvm::APInt(ConstantIndexOffset.getBitWidth(), SL->getElementOffset(ElementIdx));
               continue;
            }

            // For array or vector indices, scale the index by the size of the type.
            llvm::APInt Index = OpC->getValue().sextOrTrunc(ConstantIndexOffset.getBitWidth());
            ConstantIndexOffset +=
                Index * llvm::APInt(ConstantIndexOffset.getBitWidth(), DL->getTypeAllocSize(GTI.getIndexedType()));
         }
         isZero = !ConstantIndexOffset;
         return assignCodeAuto(llvm::ConstantInt::get(gep_op->getContext(), ConstantIndexOffset));
      }
   }

   const void* DumpBambuIR::LowerGetElementPtr(const void* type, const llvm::User* gep,
                                               const llvm::Function* currentFunction)
   {
      assert(IR_CODE(type) == IRC(POINTER_TY_NODE));
      auto base_node = getOperand(gep->getOperand(0), currentFunction);
      const llvm::GEPOperator* gep_op = dyn_cast<llvm::GEPOperator>(gep);
      assert(gep_op);
      bool isZero = false;
      auto offset_node = LowerGetElementPtrOffset(gep_op, currentFunction, base_node, isZero);
      if(isZero)
         return base_node;
      else
         return build2(IRC(GEP_NODE), type, base_node, offset_node);
   }

   const void* DumpBambuIR::getOperand(const llvm::Value* operand, const llvm::Function* currentFunction)
   {
      if(isa<llvm::ConstantInt>(operand) || isa<llvm::ConstantFP>(operand) || isa<llvm::ConstantPointerNull>(operand))
         return assignCodeAuto(operand);
      else if(isa<llvm::ConstantAggregateZero>(operand))
         return assignCodeAuto(operand);
      else if(isa<llvm::Instruction>(operand))
      {
         return getSSA(operand, operand, currentFunction, false);
      }
      else if(isa<llvm::Argument>(operand))
      {
         auto def_stmt = getNop(operand, dyn_cast<llvm::Argument>(operand)->getParent());
         auto ssa = getSSA(operand, def_stmt, currentFunction, false);
         index2ssa_name.find(std::make_pair(def_stmt, false))->second.var = assignCodeAuto(operand);
         return ssa;
      }
      else if(isa<llvm::GlobalVariable>(operand))
      {
         auto type = assignCodeType(operand->getType());
         return build1(IRC(ADDR_NODE), type, assignCodeAuto(operand));
      }
      else if(isa<llvm::Function>(operand))
      {
         auto type = assignCodeType(operand->getType());
         return build1(IRC(ADDR_NODE), type, assignCodeAuto(operand));
      }
      else if(isa<llvm::ConstantExpr>(operand))
      {
         auto ce = cast<llvm::ConstantExpr>(operand);
         auto resType = assignCodeType(ce->getType());
         if(ce->getOpcode() == llvm::Instruction::GetElementPtr)
            return LowerGetElementPtr(assignCodeType(operand->getType()), ce, currentFunction);
         else
         {
            auto tec = ir_node_code(ce);
            if(get_ir_rhs_class(tec) == IR_TERNARY_RHS)
            {
               return build3(tec, resType, getSignedOperandIndex(ce, 0, currentFunction),
                             getSignedOperandIndex(ce, 1, currentFunction),
                             getSignedOperandIndex(ce, 2, currentFunction));
            }
            else if(get_ir_rhs_class(tec) == IR_BINARY_RHS)
            {
               return build2(tec, resType, getSignedOperandIndex(ce, 0, currentFunction),
                             getSignedOperandIndex(ce, 1, currentFunction));
            }
            else if(get_ir_rhs_class(tec) == IR_UNARY_RHS)
            {
               assert(ce->getOpcode() != llvm::Instruction::SExt);
               return build1(tec, resType, getSignedOperandIndex(ce, 0, currentFunction));
            }
            else if(get_ir_rhs_class(tec) == IR_SINGLE_RHS)
            {
               auto ltype = resType;
               auto rhs = getSignedOperandIndex(ce, 0, currentFunction);
               auto rtype = IR_TYPE_NODE(rhs);
               if(ltype == rtype)
                  return rhs;
               else
                  return build1(IRC(BITCAST_NODE), ltype, rhs);
            }
            else
               report_fatal_error("unexpected condition");
         }
      }
      else if(isa<llvm::UndefValue>(operand))
      {
         auto type = operand->getType();
         if(type->isAggregateType() || type->isVectorTy())
            return assignCodeAuto(llvm::ConstantAggregateZero::get(type));
         else if(type->isPointerTy())
            return assignCodeAuto(llvm::ConstantPointerNull::get(cast<llvm::PointerType>(type)));
         else if(type->isIntegerTy())
            return assignCodeAuto(llvm::ConstantInt::get(type, 0, false));
         else if(type->isFloatingPointTy())
            return assignCodeAuto(llvm::ConstantFP::getNaN(type));
         else
         {
            operand->print(llvm::errs(), true);
            llvm::errs() << "\n";
            stream.close();
            report_fatal_error(
                (std::string("Unsupported LLVM statement: ") + std::string(ValueTyNames[operand->getValueID()]))
                    .c_str());
         }
      }
      else if(isa<llvm::ConstantDataArray>(operand) || isa<llvm::ConstantDataVector>(operand) ||
              isa<llvm::ConstantStruct>(operand) || isa<llvm::ConstantArray>(operand) ||
              isa<llvm::ConstantVector>(operand))
         return assignCodeAuto(operand);
      else
      {
         operand->print(llvm::errs(), true);
         llvm::errs() << "\n";
         stream.close();
         report_fatal_error(
             (std::string("Unsupported LLVM statement: ") + std::string(ValueTyNames[operand->getValueID()])).c_str());
      }
   }

   template <class InstructionOrConstantExpr>
   const void* DumpBambuIR::getSignedOperand(const InstructionOrConstantExpr* inst, const void* op, unsigned index)
   {
      auto isSignedOp = isSignedOperand(inst, index);
      if(isSignedOp)
      {
         auto type_operand = IR_TYPE_NODE(op);
         auto ir_code_op = IR_CODE(type_operand);
         if(ir_code_op == IRC(POINTER_TY_NODE))
         {
            return build1(IRC(NOP_NODE), &SignedPointerTypeReference, op);
         }
         else if(IR_CODE(op) == IRC(CONSTANT_INT_VAL_NODE))
         {
            const void* ics;
            if(index2integer_cst_signed.find(op) == index2integer_cst_signed.end())
            {
               auto& ics_obj = index2integer_cst_signed[op];
               ics_obj.ic = op;
               ics_obj.type = AddSignedTag(type_operand);
               ics = assignCode(&ics_obj, IRC(CONSTANT_INT_VAL_NODE_SIGNED));
            }
            else
            {
               ics = &index2integer_cst_signed.find(op)->second;
            }
            return ics;
         }
         else if(!CheckSignedTag(IR_TYPE_NODE(op)))
         {
            return build1(IRC(NOP_NODE), AddSignedTag(type_operand), op);
         }
         else
         {
            return op;
         }
      }
      else if(isUnsignedOperand(inst, index) && (CheckSignedTag(IR_TYPE_NODE(op))))
         return build1(IRC(NOP_NODE), NormalizeSignedTag(IR_TYPE_NODE(op)), op);
      else
         return op;
   }
   template <class InstructionOrConstantExpr>
   const void* DumpBambuIR::getSignedOperandIndex(const InstructionOrConstantExpr* inst, unsigned index,
                                                  const llvm::Function* currentFunction)
   {
      auto op = getOperand(inst->getOperand(index), currentFunction);
      return getSignedOperand(inst, op, index);
   }

   const void* DumpBambuIR::ir_assign_rhsIndex(const void* g, unsigned index)
   {
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      auto currentFunction = inst->getFunction();
      if(isa<llvm::LoadInst>(inst))
      {
         assert(index == 0);
         const llvm::LoadInst& load = cast<const llvm::LoadInst>(*inst);
         auto val_zero = llvm::APInt(32, 0);
         if(uicTable.find(val_zero) == uicTable.end())
         {
            uicTable[val_zero] = assignCodeAuto(llvm::ConstantInt::get(inst->getContext(), val_zero));
         }
         const void* zero = uicTable.find(val_zero)->second;
         auto addr = getOperand(load.getPointerOperand(), currentFunction);
         auto ty = load.getType();
         auto isVecType = ty->isVectorTy();
         auto type = assignCodeType(ty);
         auto read_obj_size = ty->isSized() ? DL->getTypeAllocSizeInBits(ty) : 8ULL;
         const auto funName = getName(load.getFunction());
         const auto demangled = getDemangled(funName);
         bool is_a_top_parameter = isa<llvm::Argument>(load.getPointerOperand()) &&
                                   (llvm::find(TopFunctionNames, funName) != TopFunctionNames.end() ||
                                    llvm::find(TopFunctionNames, demangled) != TopFunctionNames.end());

         // Refine the instruction alignment with the base object alignment and
         // any constant byte offset accumulated along the pointer derivation.
         // This keeps typed accesses on naturally aligned parameters aligned,
         // while still catching explicit byte-offset or cast-based misalignment.
#if PANDA_LLVM_CLANG_MAJOR < 16
         unsigned long long effectiveLoadAlign = load.getAlignment();
#else
         unsigned long long effectiveLoadAlign = load.getAlign().value();
#endif
         effectiveLoadAlign = getEffectiveMemoryAccessAlignment(load.getPointerOperand(), effectiveLoadAlign, DL);
         if(effectiveLoadAlign && read_obj_size > (8 * effectiveLoadAlign) && !is_a_top_parameter && !isVecType)
            return build1(IRC(UNALIGNED_MEM_ACCESS_NODE), type, addr);
         else
            return build1(IRC(MEM_ACCESS_NODE), type, addr);
      }
      else if(isa<llvm::SExtInst>(inst) && cast<const llvm::SExtInst>(*inst).getType()->isIntegerTy() &&
              cast<const llvm::SExtInst>(*inst).getOperand(0)->getType()->isIntegerTy() &&
              cast<const llvm::SExtInst>(*inst).getOperand(0)->getType()->getIntegerBitWidth() == 1)
      {
         assert(index == 0);
         const llvm::SExtInst& sext = cast<const llvm::SExtInst>(*inst);
         auto MSB_pos = llvm::APInt(32, sext.getType()->getIntegerBitWidth() - 1);
         assert(MSB_pos.ugt(0));
         if(uicTable.find(MSB_pos) == uicTable.end())
         {
            uicTable[MSB_pos] = assignCodeAuto(llvm::ConstantInt::get(inst->getContext(), MSB_pos));
         }
         const void* MSB_posNode = uicTable.find(MSB_pos)->second;
         auto type = AddSignedTag(assignCodeType(sext.getType()));
         auto casted = build1(IRC(NOP_NODE), type, getOperand(inst->getOperand(index), currentFunction));
         auto shiftedLeft = build2(IRC(SHL_NODE), type, casted, MSB_posNode);
         return build2(IRC(SHR_NODE), type, shiftedLeft, MSB_posNode);
      }
      else if(isa<llvm::TruncInst>(inst) && cast<const llvm::TruncInst>(*inst).getType()->isIntegerTy())
      {
         assert(index == 0);
         const llvm::TruncInst& tI = cast<const llvm::TruncInst>(*inst);
         auto bw = tI.getType()->getIntegerBitWidth();
         if(bw != 8 && bw != 16 && bw != 32 && bw != 64)
         {
            auto mask = (llvm::APInt(bw + 1, 1) << bw) - 1;
            if(uicTable.find(mask) == uicTable.end())
            {
               uicTable[mask] = assignCodeAuto(llvm::ConstantInt::get(inst->getContext(), mask));
            }
            const void* maskNode = uicTable.find(mask)->second;
            auto type = assignCodeType(tI.getType());
            return build2(IRC(AND_NODE), type, getOperand(inst->getOperand(index), currentFunction), maskNode);
         }
      }
      else if(isa<llvm::ShuffleVectorInst>(inst) && index == 2)
      {
         const auto& svi = cast<const llvm::ShuffleVectorInst>(*inst);
#if PANDA_LLVM_CLANG_MAJOR >= 11
         auto op = getOperand(svi.getShuffleMaskForBitcode(), currentFunction);
#else
         auto op = getOperand(svi.getMask(), currentFunction);
#endif
         return op;
      }
      return getSignedOperandIndex(inst, index, currentFunction);
   }

   const void* DumpBambuIR::ir_phi_virtual_result(const void* g) const
   {
      const ir_phi_virtual* gpv = reinterpret_cast<const ir_phi_virtual*>(g);
      assert(HAS_CODE(gpv->res));
      return gpv->res;
   }
   unsigned int DumpBambuIR::ir_phi_num_args(const void* g) const
   {
      const llvm::PHINode* phi = reinterpret_cast<const llvm::PHINode*>(g);
      return phi->getNumIncomingValues();
   }

   unsigned int DumpBambuIR::ir_phi_virtual_num_args(const void* g) const
   {
      const ir_phi_virtual* gpv = reinterpret_cast<const ir_phi_virtual*>(g);
      return gpv->def_edfe_pairs.size();
   }

   const void* DumpBambuIR::ir_phi_arg_def(const void* g, unsigned int index)
   {
      const llvm::PHINode* phi = reinterpret_cast<const llvm::PHINode*>(g);
      return getSignedOperand(phi, getOperand(phi->getIncomingValue(index), phi->getFunction()), 0);
   }

   const void* DumpBambuIR::ir_phi_virtual_arg_def(const void* g, unsigned int index)
   {
      const ir_phi_virtual* gpv = reinterpret_cast<const ir_phi_virtual*>(g);
      assert(index < gpv->def_edfe_pairs.size());
      return gpv->def_edfe_pairs.at(index).first;
   }

   int DumpBambuIR::ir_phi_arg_edgeBBindex(const void* g, unsigned int index)
   {
      const llvm::PHINode* phi = reinterpret_cast<const llvm::PHINode*>(g);
      auto BB = phi->getIncomingBlock(index);
      return getBB_index(BB);
   }

   int DumpBambuIR::ir_phi_virtual_arg_edgeBBindex(const void* g, unsigned int index)
   {
      const ir_phi_virtual* gpv = reinterpret_cast<const ir_phi_virtual*>(g);
      assert(index < gpv->def_edfe_pairs.size());
      return gpv->def_edfe_pairs.at(index).second;
   }

   const void* DumpBambuIR::ir_call_fn(const void* g)
   {
      const llvm::CallInst* ci = reinterpret_cast<const llvm::CallInst*>(g);
#if PANDA_LLVM_CLANG_MAJOR >= 11
      auto calledFun = ci->getCalledOperand();
#else
      llvm::ImmutableCallSite CS(ci);
      auto calledFun = CS.getCalledValue();
#endif
      if(isa<llvm::Function>(calledFun))
      {
         auto type = assignCodeType(calledFun->getType());
         return build1(IRC(ADDR_NODE), type, assignCodeAuto(calledFun));
      }
      else if(isa<llvm::ConstantExpr>(calledFun))
      {
         auto type = assignCodeType(calledFun->getType());
         auto ce = cast<llvm::ConstantExpr>(calledFun);
         if(ce->getOpcode() == llvm::Instruction::BitCast)
         {
            auto op = ce->getOperand(0);
            if(isa<llvm::Function>(op))
               return build1(IRC(ADDR_NODE), type, assignCodeAuto(op));
         }
         return getOperand(calledFun, ci->getFunction());
      }
      else
      {
         return getOperand(calledFun, ci->getFunction());
      }
   }

   unsigned int DumpBambuIR::ir_call_num_args(const void* g)
   {
      const llvm::CallInst* ci = reinterpret_cast<const llvm::CallInst*>(g);
#if PANDA_LLVM_CLANG_MAJOR < 14
      return ci->getNumArgOperands();
#else
      return ci->arg_size();
#endif
   }

   const void* DumpBambuIR::ir_call_arg(const void* g, unsigned int arg_index)
   {
      const llvm::CallInst* ci = reinterpret_cast<const llvm::CallInst*>(g);
      return getOperand(ci->getArgOperand(arg_index), ci->getFunction());
   }

   const void* DumpBambuIR::ir_return_retval(const void* g)
   {
      const auto* ri = reinterpret_cast<const llvm::Instruction*>(g);
      if(isa<llvm::ReturnInst>(ri) && cast<llvm::ReturnInst>(ri)->getReturnValue())
      {
         auto op = getOperand(cast<llvm::ReturnInst>(ri)->getReturnValue(), ri->getFunction());
         return getSignedOperand(ri, op, 0);
      }
      else if(isa<llvm::UnreachableInst>(ri))
      {
         auto ui = cast<llvm::UnreachableInst>(ri);
         auto fu = ui->getFunction();
         if(fu->getReturnType()->isVoidTy())
            return nullptr;
         else
         {
            auto retType = fu->getReturnType();
            return assignCodeAuto(llvm::UndefValue::get(retType));
         }
      }
      else
         return nullptr;
   }

   const std::vector<std::pair<const void*, unsigned int>> DumpBambuIR::ir_ifelseif_pairs(const void* g)
   {
      std::vector<std::pair<const void*, unsigned int>> res;
      const auto si = reinterpret_cast<const llvm::SwitchInst*>(g);
      auto var = getSignedOperand(si, getOperand(si->getCondition(), si->getFunction()), 0);
      std::map<unsigned int, std::set<const void*>> case_labels;
      for(auto case_expr : si->cases())
      {
         const void* guard0 = assignCodeAuto(case_expr.getCaseValue());
         auto BB = case_expr.getCaseSuccessor();
         auto olabel = getBB_index(BB);
         case_labels[olabel].insert(guard0);
      }
      for(auto label_guards : case_labels)
      {
         auto dest = label_guards.first;
         auto guards = label_guards.second;
         auto btype = llvm::Type::getInt1Ty(si->getFunction()->getContext());
         const void* cond = nullptr;
         for(auto g : guards)
         {
            const auto eq = build2(IRC(EQ_NODE), btype, var, g);
            cond = cond ? build2(IRC(IOR_NODE), btype, cond, eq) : eq;
         }
         res.push_back(std::make_pair(cond, dest));
      }
      res.push_back(std::make_pair(nullptr, getBB_index(si->getDefaultDest())));

      return res;
   }

   const std::vector<std::pair<const void*, unsigned int>> DumpBambuIR::ir_ifelse_pairs(const void* g)
   {
      std::vector<std::pair<const void*, unsigned int>> res;
      const auto bi = reinterpret_cast<const llvm::BranchInst*>(g);
      res.push_back(std::make_pair(ir_cond_op(g), getBB_index(bi->getSuccessor(0))));
      res.push_back(std::make_pair(nullptr, getBB_index(bi->getSuccessor(1))));
      return res;
   }

   const void* DumpBambuIR::build_custom_function_call_node(const void* g)
   {
      assert(index2call_node.find(g) == index2call_node.end());
      auto& ce = index2call_node[g];
      auto res = assignCode(&ce, IRC(CALL_NODE));
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);

      ce.type = assignCodeType(inst->getType());
      ce.fn = ir_call_fn(g);
      unsigned int arg_index;
      for(arg_index = 0; arg_index < ir_call_num_args(g); arg_index++)
         ce.args.push_back(ir_call_arg(g, arg_index));
      return res;
   }

   const void* DumpBambuIR::call_node_fn(const void* t)
   {
      const call_node* ce = reinterpret_cast<const call_node*>(t);
      assert(HAS_CODE(ce->fn));
      return ce->fn;
   }

   unsigned int DumpBambuIR::call_node_num_args(const void* t)
   {
      const call_node* ce = reinterpret_cast<const call_node*>(t);
      return ce->args.size();
   }

   const void* DumpBambuIR::call_node_arg(const void* t, unsigned int arg_index)
   {
      const call_node* ce = reinterpret_cast<const call_node*>(t);
      assert(HAS_CODE(ce->args.at(arg_index)));
      return ce->args.at(arg_index);
   }

   const void* DumpBambuIR::getVirtualPhi(llvm::MemoryPhi* mp, const llvm::MemorySSA& MSSA)
   {
      llvm::BasicBlock* BB = mp->getBlock();
      if(index2ir_phi_virtual.find(BB) == index2ir_phi_virtual.end())
      {
         auto& gpv = index2ir_phi_virtual[BB];
         assignCode(&gpv, IRC(IR_PHI_VIRTUAL));
         const llvm::Function* currentFunction = BB->getParent();
         gpv.parent = assignCodeAuto(currentFunction);
         gpv.bb_index = getBB_index(BB);
         gpv.res = getSSA(mp, &gpv, currentFunction, false);
         for(auto index = 0u; index < mp->getNumIncomingValues(); ++index)
         {
            auto val = mp->getIncomingValue(index);
            assert(val->getValueID() == llvm::Value::MemoryDefVal || val->getValueID() == llvm::Value::MemoryPhiVal);
            if(val->getValueID() == llvm::Value::MemoryDefVal)
            {
               bool isDefault = false;
               const void* def_stmt = getVirtualDefStatement(val, isDefault, MSSA, currentFunction);
               gpv.def_edfe_pairs.push_back(std::make_pair(getSSA(val, def_stmt, currentFunction, isDefault),
                                                           getBB_index(mp->getIncomingBlock(index))));
            }
            else
            {
               assert(val->getValueID() == llvm::Value::MemoryPhiVal);
               gpv.def_edfe_pairs.push_back(
                   std::make_pair(ir_phi_virtual_result(getVirtualPhi(dyn_cast<llvm::MemoryPhi>(val), MSSA)),
                                  getBB_index(mp->getIncomingBlock(index))));
            }
         }
      }
      return &index2ir_phi_virtual.find(BB)->second;
   }

   const void* DumpBambuIR::build3(DumpBambuIR::ir_codes tc, const void* type, const void* op1, const void* op2,
                                   const void* op3)
   {
      index2ir_node.push_front(ir_node());
      auto& te = index2ir_node.front();
      assignCode(&te, tc);
      te.tc = tc;
      te.type = assignCodeType(reinterpret_cast<const llvm::Type*>(type));
      assert(op1 != nullptr || op2 != nullptr || op3 != nullptr);
      assert(op1 == nullptr || HAS_CODE(op1));
      assert(op2 == nullptr || HAS_CODE(op2));
      assert(op3 == nullptr || HAS_CODE(op3));
      te.op1 = op1;
      te.op2 = op2;
      te.op3 = op3;
      return &te;
   }

   const void* DumpBambuIR::getParentDecl(const void* t)
   {
      if(IR_CODE(t) == IRC(MODULE_UNIT_NODE))
         return nullptr;
      else if(IR_CODE(t) == IRC(ALLOCAVARIABLE_VAL_NODE))
      {
         const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
         return assignCodeAuto(av->alloc_inst->getFunction());
      }
      else if(IR_CODE(t) == IRC(ORIGVARIABLE_VAL_NODE))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return getParentDecl(ov->orig);
      }
      else if(IR_CODE(t) == IRC(FIELD_VAL_NODE))
      {
         const field_val_node* ty = reinterpret_cast<const field_val_node*>(t);
         assert(HAS_CODE(ty->parent));
         return ty->parent;
      }
      else if(IR_CODE(t) == IRC(ARGUMENT_VAL_NODE))
      {
         const llvm::Argument* llvm_obj = reinterpret_cast<const llvm::Argument*>(t);
         return assignCodeAuto(llvm_obj->getParent());
      }
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      const llvm::Module* parent = llvm_obj->getParent();
      assert(parent);
      return assignCode(parent, IRC(MODULE_UNIT_NODE));
   }

   bool DumpBambuIR::IR_DECL_C_BIT_FIELD(const void* t) const
   {
      return false;
   }

   bool DumpBambuIR::IR_DECL_EXTERNAL(const void* t) const
   {
      if(IR_CODE(t) == IRC(ORIGVARIABLE_VAL_NODE))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         if(IR_CODE(ov->orig) == IRC(VARIABLE_VAL_NODE))
            return IR_DECL_EXTERNAL(ov->orig);
         else
            return false;
      }
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      if(isa<llvm::GlobalVariable>(llvm_obj))
         return llvm_obj->hasExternalLinkage() && !cast<llvm::GlobalVariable>(llvm_obj)->hasInitializer();
      else if(isa<llvm::Function>(llvm_obj))
         return cast<llvm::Function>(llvm_obj)->empty();
      else
         report_fatal_error("unexpected case");
   }

   bool DumpBambuIR::IR_PUBLIC(const void* t) const
   {
      if(IR_CODE(t) == IRC(ORIGVARIABLE_VAL_NODE))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         if(IR_CODE(ov->orig) == IRC(VARIABLE_VAL_NODE))
            return IR_PUBLIC(ov->orig);
         else
            return false;
      }
      const llvm::GlobalValue* llvm_obj = reinterpret_cast<const llvm::GlobalValue*>(t);
      return llvm_obj->hasDefaultVisibility() && !llvm_obj->hasInternalLinkage() &&
             llvm_obj->getLinkage() != llvm::GlobalValue::PrivateLinkage;
   }

   bool DumpBambuIR::IR_STATIC(const void* t) const
   {
      if(IR_CODE(t) == IRC(ORIGVARIABLE_VAL_NODE))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         if(IR_CODE(ov->orig) == IRC(VARIABLE_VAL_NODE))
            return IR_STATIC(ov->orig);
         else
            return false;
      }
      const llvm::GlobalValue* llvm_obj = reinterpret_cast<const llvm::GlobalValue*>(t);
      auto lt = llvm_obj->getLinkage();
      return lt == llvm::GlobalValue::InternalLinkage || lt == llvm::GlobalValue::PrivateLinkage;
   }

   bool DumpBambuIR::is_builtin_fn(const void* t) const
   {
      assert(IR_CODE(t) == IRC(FUNCTION_VAL_NODE));
      const llvm::Function* fd = reinterpret_cast<const llvm::Function*>(t);
      if(fd->hasName())
      {
         std::string declname;
         if(fd->isIntrinsic())
            declname = getIntrinsicName(fd);
         else
            declname = std::string(getName(fd));
         if(builtinsNames.find(std::string("__builtin_") + declname) != builtinsNames.end() ||
            builtinsNames.find(declname) != builtinsNames.end())
            return true;
      }
      return false;
   }

   const void* DumpBambuIR::IR_DECL_INITIAL(const void* t)
   {
      if(IR_CODE(t) == IRC(FIELD_VAL_NODE))
         return nullptr;
      if(IR_CODE(t) == IRC(ALLOCAVARIABLE_VAL_NODE))
         return nullptr;
      if(IR_CODE(t) == IRC(ORIGVARIABLE_VAL_NODE))
         return nullptr;
      const llvm::Value* llvm_obj = reinterpret_cast<const llvm::Value*>(t);
      if(isa<llvm::GlobalVariable>(llvm_obj) && cast<llvm::GlobalVariable>(llvm_obj)->hasInitializer())
      {
         auto init_value = cast<llvm::GlobalVariable>(llvm_obj)->getInitializer();
         return getOperand(init_value, nullptr);
      }
      else
         return nullptr;
   }

   unsigned int DumpBambuIR::IR_DECL_BITSIZEALLOC(const void* t)
   {
      if(IR_CODE(t) == IRC(ALLOCAVARIABLE_VAL_NODE))
      {
         const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
         auto arraySize = av->alloc_inst->getArraySize();
         if(isa<llvm::ConstantInt>(arraySize) && cast<llvm::ConstantInt>(arraySize)->getZExtValue() == 1)
            return IR_TYPE_BITSIZEALLOC(IR_TYPE_NODE(t));
         else
         {
            llvm::errs() << "Dynamic size alloca instruction: ";
            av->alloc_inst->print(llvm::errs());
            llvm::errs() << "\n  Size value: ";
            arraySize->print(llvm::errs());
            llvm::errs() << "\n";
            stream.close();
            report_fatal_error("Unsupported dynamic memory allocation");
         }
      }
      else if(IR_CODE(t) == IRC(ORIGVARIABLE_VAL_NODE))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return IR_DECL_BITSIZEALLOC(ov->orig);
      }
      else
         return IR_TYPE_BITSIZEALLOC(IR_TYPE_NODE(t));
   }

   int DumpBambuIR::IR_DECL_ALIGN(const void* t)
   {
      if(IR_CODE(t) == IRC(VARIABLE_VAL_NODE))
      {
         const llvm::GlobalVariable* llvm_obj = reinterpret_cast<const llvm::GlobalVariable*>(t);
#if PANDA_LLVM_CLANG_MAJOR < 16
         return std::max(8u, 8 * llvm_obj->getAlignment());
#else
         return std::max(8u, 8 * static_cast<unsigned>(llvm_obj->getAlign()->value()));
#endif
      }
      else if(IR_CODE(t) == IRC(ALLOCAVARIABLE_VAL_NODE))
      {
         const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
#if PANDA_LLVM_CLANG_MAJOR < 16
         auto algn = av->alloc_inst->getAlignment();
#else
         auto algn = static_cast<unsigned>(av->alloc_inst->getAlign().value());
#endif
         if(algn == 0)
         {
            auto arraySize = av->alloc_inst->getArraySize();
            if(isa<llvm::ConstantInt>(arraySize) && cast<llvm::ConstantInt>(arraySize)->getZExtValue() == 1)
            {
               auto allocType = av->alloc_inst->getAllocatedType();
               auto typeSize = allocType->isSized() ? DL->getTypeAllocSizeInBits(allocType) : 8ULL;
               algn = typeSize / 8ULL;
            }
         }
         return std::max(8u, 8 * algn);
      }
      else
         return IR_TYPE_ALIGN(IR_TYPE_NODE(t));
   }

   bool DumpBambuIR::IR_DECL_PACKED(const void* t) const
   {
      assert(IR_CODE(t) == IRC(FIELD_VAL_NODE));
      const field_val_node* ty = reinterpret_cast<const field_val_node*>(t);
      const llvm::StructType* st = reinterpret_cast<const llvm::StructType*>(ty->parent);
      return st->isPacked();
   }

   const std::string DumpBambuIR::FIELD_VAL_NODE_OFFSET(const void* t)
   {
      assert(IR_CODE(t) == IRC(FIELD_VAL_NODE));
      const field_val_node* ty = reinterpret_cast<const field_val_node*>(t);
      return ty->offset;
   }

   bool DumpBambuIR::IR_READONLY(const void* t) const
   {
      ir_codes code = IR_CODE(t);
      if(code == IRC(ORIGVARIABLE_VAL_NODE))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return IR_READONLY(ov->orig);
      }
      else if(code == IRC(VARIABLE_VAL_NODE) || code == IRC(ARGUMENT_VAL_NODE))
      {
         const auto llvm_val = reinterpret_cast<const llvm::Value*>(t);
         if(code == IRC(VARIABLE_VAL_NODE))
         {
            const auto llvm_glb = llvm::dyn_cast<llvm::GlobalVariable>(llvm_val);
            if(llvm_glb)
            {
               return llvm_glb->isConstant();
            }
         }
         else if(code == IRC(ARGUMENT_VAL_NODE))
         {
            const auto llvm_arg = llvm::dyn_cast<llvm::Argument>(llvm_val);
            if(llvm_arg)
            {
               return llvm_arg->onlyReadsMemory();
            }
         }
      }
      return false;
   }
   bool DumpBambuIR::IR_ADDRESSABLE(const void* t) const
   {
      assert(IR_CODE(t) == IRC(ALLOCAVARIABLE_VAL_NODE));
      const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
      return !av->addr;
   }
   const void* DumpBambuIR::IR_OPERAND(const void* t, unsigned index)
   {
      const ir_node* te = reinterpret_cast<const ir_node*>(t);
      if(index == 0)
         return te->op1;
      else if(index == 1)
         return te->op2;
      else if(index == 2)
         return te->op3;
      else
         report_fatal_error("unexpected condition");
   }

   std::string DumpBambuIR::IR_INT_CST(const void* t)
   {
      const llvm::ConstantData* cd;
      bool isSigned = IR_CODE(t) == IRC(CONSTANT_INT_VAL_NODE_SIGNED);
      if(isSigned)
         cd = reinterpret_cast<const llvm::ConstantData*>(reinterpret_cast<const integer_cst_signed*>(t)->ic);
      else
         cd = reinterpret_cast<const llvm::ConstantData*>(t);
      if(isa<llvm::ConstantPointerNull>(cd))
         return "0";
      const llvm::ConstantInt* llvm_obj = cast<const llvm::ConstantInt>(cd);
      const llvm::APInt& val = llvm_obj->getValue();
      if(isSigned || CheckSignedTag(IR_TYPE_NODE(t)))
      {
         SmallString<40> S;
         val.toStringSigned(S);
         return S.c_str();
      }
      else
      {
         SmallString<40> S;
         val.toStringUnsigned(S);
         return S.c_str();
      }
   }

   const void* DumpBambuIR::assignCodeType(const llvm::Type* ty)
   {
      if(reinterpret_cast<const unsigned int*>(ty) == &SignedPointerTypeReference)
         return ty;
      auto typeId = NormalizeSignedTag(ty)->getTypeID();
      switch(typeId)
      {
         case llvm::Type::VoidTyID:
            return assignCode(ty, IRC(VOID_TY_NODE));
         case llvm::Type::HalfTyID:
         case llvm::Type::FloatTyID:
         case llvm::Type::DoubleTyID:
         case llvm::Type::X86_FP80TyID:
         case llvm::Type::FP128TyID:
         case llvm::Type::PPC_FP128TyID:
            return assignCode(ty, IRC(REAL_TY_NODE));
         case llvm::Type::LabelTyID:
            llvm::errs() << "assignCodeType kind not supported: LabelTyID\n";
            stream.close();
            report_fatal_error("Plugin Error");
         case llvm::Type::MetadataTyID:
            return assignCode(ty, IRC(METADATA_TY_ID));
         case llvm::Type::X86_MMXTyID:
            llvm::errs() << "assignCodeType kind not supported: X86_MMXTyID\n";
            stream.close();
            report_fatal_error("Plugin Error");
         case llvm::Type::TokenTyID:
            llvm::errs() << "assignCodeType kind not supported: TokenTyID\n";
            stream.close();
            report_fatal_error("Plugin Error");
         case llvm::Type::IntegerTyID:
            return assignCode(ty, IRC(INTEGER_TY_NODE));
         case llvm::Type::FunctionTyID:
            return assignCode(ty, IRC(FUNCTION_TY_NODE));
         case llvm::Type::StructTyID:
            return assignCode(ty, IRC(RECORD_TY_NODE));
         case llvm::Type::ArrayTyID:
            return assignCode(ty, IRC(ARRAY_TY_NODE));
         case llvm::Type::PointerTyID:
            return assignCode(ty, IRC(POINTER_TY_NODE));
#if PANDA_LLVM_CLANG_MAJOR >= 11
         case llvm::Type::FixedVectorTyID:
         case llvm::Type::ScalableVectorTyID:
#else
         case llvm::Type::VectorTyID:
#endif
            return assignCode(ty, IRC(VECTOR_TY_NODE));
         default:
         {
            llvm::errs() << "type id not managed\n";
            stream.close();
            report_fatal_error("Plugin error");
         }
      }
   }

   const void* DumpBambuIR::IR_TYPE_NODE(const void* t)
   {
      ir_codes code = IR_CODE(t);
      if(code == IRC(MODULE_UNIT_NODE))
         return nullptr;
      else if(code == IRC(ALLOCAVARIABLE_VAL_NODE))
      {
         const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
         auto allocType = av->alloc_inst->getAllocatedType();
         return assignCodeType(allocType);
      }
      else if(code == IRC(ORIGVARIABLE_VAL_NODE))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return IR_TYPE_NODE(ov->orig);
      }
      else if(code == IRC(FIELD_VAL_NODE))
      {
         const field_val_node* ty = reinterpret_cast<const field_val_node*>(t);
         assert(HAS_CODE(ty->type));
         return ty->type;
      }
      else if(code == IRC(IR_NOP))
      {
         report_fatal_error("unexpected");
         // const ir_nop* gn = reinterpret_cast<const ir_nop*>(t);
         // return IR_TYPE_NODE(gn->argument_val_node);
      }
      else if(code == IRC(SSA_NODE))
      {
         const ssa_node* ssa = reinterpret_cast<const ssa_node*>(t);
         assert(HAS_CODE(ssa->type));
         return ssa->type;
      }
      else if(code == IRC(CALL_NODE))
      {
         const call_node* ce = reinterpret_cast<const call_node*>(t);
         assert(HAS_CODE(ce->type));
         return ce->type;
      }
      else if(code == IRC(CONSTANT_INT_VAL_NODE_SIGNED))
      {
         const integer_cst_signed* ics = reinterpret_cast<const integer_cst_signed*>(t);
         assert(HAS_CODE(ics->type));
         return ics->type;
      }
      auto code_class = IR_CODE_CLASS(code);
      if(code_class == ir_type)
      {
         bool isSigned = CheckSignedTag(t);
         t = NormalizeSignedTag(t);
         const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
         auto typeId = ty->getTypeID();
         switch(typeId)
         {
            case llvm::Type::VoidTyID:
            case llvm::Type::HalfTyID:
            case llvm::Type::FloatTyID:
            case llvm::Type::DoubleTyID:
            case llvm::Type::X86_FP80TyID:
            case llvm::Type::FP128TyID:
            case llvm::Type::PPC_FP128TyID:
            case llvm::Type::LabelTyID:
            case llvm::Type::MetadataTyID:
            case llvm::Type::X86_MMXTyID:
            case llvm::Type::TokenTyID:
            case llvm::Type::IntegerTyID:
            case llvm::Type::StructTyID:
               llvm::errs() << "IR_TYPE_NODE kind not supported: type of type: " << GET_IR_CODE_NAME(IR_CODE(t)) << ":"
                            << typeId << " ptr " << (unsigned long long)t << "\n";
               stream.close();
               report_fatal_error("Plugin Error");

            case llvm::Type::FunctionTyID:
               return assignCodeType(cast<llvm::FunctionType>(ty)->getReturnType());
            case llvm::Type::ArrayTyID:
               return assignCodeType(cast<llvm::ArrayType>(ty)->getElementType());
            case llvm::Type::PointerTyID:
#if PANDA_LLVM_CLANG_MAJOR < 16
               return assignCodeType(cast<llvm::PointerType>(ty)->getElementType());
#else
               return assignCodeType(ty->isOpaquePointerTy() ? llvm::Type::getVoidTy(*moduleContext) :
                                                               ty->getNonOpaquePointerElementType());
#endif
#if PANDA_LLVM_CLANG_MAJOR >= 11
            case llvm::Type::FixedVectorTyID:
            case llvm::Type::ScalableVectorTyID:
#else
            case llvm::Type::VectorTyID:
#endif
            {
               const auto eltType = cast<const llvm::VectorType>(ty)->getElementType();
               return assignCodeType(isSigned ? AddSignedTag(eltType) : eltType);
            }
            default:
            {
               llvm::errs() << "type id not managed\n";
               stream.close();
               report_fatal_error("Plugin error");
            }
         }
      }
      else if(IS_NODE_CODE_CLASS(code_class))
      {
         const ir_node* te = reinterpret_cast<const ir_node*>(t);
         assert(te->type);
         assert(HAS_CODE(te->type));
         return te->type;
      }
      else
      {
         const llvm::Value* llvm_obj = reinterpret_cast<const llvm::Value*>(t);
         if(isa<llvm::GlobalValue>(llvm_obj))
         {
            return assignCodeType(cast<llvm::GlobalValue>(llvm_obj)->getValueType());
         }
         return assignCodeType(llvm_obj->getType());
      }
   }

   bool DumpBambuIR::IR_POINTER_TYPE_P(const void* t) const
   {
      return (IR_CODE(t) == IRC(POINTER_TY_NODE));
   }

   bool DumpBambuIR::IR_TYPE_UNSIGNED(const void* t) const
   {
      ir_codes code = IR_CODE(t);
      if(code == IRC(SIGNEDPOINTERTYPE))
         return false;
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      assert(NormalizeSignedTag(ty)->isIntegerTy());
      return !CheckSignedTag(ty);
   }

   int DumpBambuIR::IR_TYPE_BITSIZE(const void* t) const
   {
      if(IR_CODE(t) == IRC(SIGNEDPOINTERTYPE))
         return 32;
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      ty = NormalizeSignedTag(ty);
      auto typeId = ty->getTypeID();
      switch(typeId)
      {
         case llvm::Type::IntegerTyID:
         {
            llvm::Type* casted_ty = const_cast<llvm::Type*>(ty);
            return DL->getTypeSizeInBits(casted_ty);
         }
         case llvm::Type::HalfTyID:
         case llvm::Type::FloatTyID:
         case llvm::Type::DoubleTyID:
         case llvm::Type::X86_FP80TyID:
         case llvm::Type::FP128TyID:
         case llvm::Type::PPC_FP128TyID:
            return ty->getPrimitiveSizeInBits();

         case llvm::Type::VoidTyID:
         case llvm::Type::LabelTyID:
         case llvm::Type::MetadataTyID:
         case llvm::Type::X86_MMXTyID:
         case llvm::Type::TokenTyID:
         case llvm::Type::StructTyID:
         case llvm::Type::FunctionTyID:
         case llvm::Type::ArrayTyID:
         case llvm::Type::PointerTyID:
#if PANDA_LLVM_CLANG_MAJOR >= 11
         case llvm::Type::FixedVectorTyID:
         case llvm::Type::ScalableVectorTyID:
#else
         case llvm::Type::VectorTyID:
#endif
         default:
            llvm::errs() << "IR_TYPE_BITSIZE kind not supported\n";
            report_fatal_error("Plugin Error");
      }
   }

   const void* DumpBambuIR::getIntegerCST(bool isSigned, llvm::LLVMContext& context, const APInt& val, const void* t)
   {
      auto nodeVal = assignCodeAuto(llvm::ConstantInt::get(context, val));
      if(isSigned)
      {
         const void* ics;
         if(index2integer_cst_signed.find(nodeVal) == index2integer_cst_signed.end())
         {
            auto& ics_obj = index2integer_cst_signed[nodeVal];
            ics_obj.ic = nodeVal;
            ics_obj.type = t;
            ics = assignCode(&ics_obj, IRC(CONSTANT_INT_VAL_NODE_SIGNED));
         }
         else
         {
            ics = &index2integer_cst_signed.find(nodeVal)->second;
         }
         return ics;
      }
      if(uicTable.find(val) == uicTable.end())
      {
         uicTable[val] = nodeVal;
      }
      return uicTable.find(val)->second;
   }

   const void* DumpBambuIR::IR_TYPE_VALUES(const void*)
   {
      report_fatal_error("unexpected call to IR_TYPE_VALUES");
   }

   const void* DumpBambuIR::IR_TYPE_NAME(const void* t)
   {
      if(IR_CODE(t) == IRC(SIGNEDPOINTERTYPE))
         return nullptr;
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      ty = NormalizeSignedTag(ty);
      if(ty->isStructTy())
      {
         auto st = cast<llvm::StructType>(ty);
         if(st->hasName())
         {
            std::string declname = st->getName().data();
            if(declname.find("struct.") == 0)
            {
               declname = declname.substr(sizeof("struct.") - 1U);
            }
            else if(declname.find("class.") == 0)
            {
               declname = declname.substr(sizeof("class.") - 1U);
            }
            if(identifierTable.find(declname) == identifierTable.end())
               identifierTable.insert(declname);
            const void* dn = identifierTable.find(declname)->c_str();
            return assignCode(dn, IRC(IDENTIFIER_NODE));
         }
      }
      return nullptr; /// TBF
   }

   unsigned int DumpBambuIR::IR_TYPE_BITSIZEALLOC(const void* t)
   {
      const llvm::Type* Cty = reinterpret_cast<const llvm::Type*>(t);
      llvm::Type* ty = const_cast<llvm::Type*>(NormalizeSignedTag(Cty));
      if(IR_CODE(t) == IRC(SIGNEDPOINTERTYPE))
      {
         return DL->getPointerSizeInBits();
      }
      else if(ty->isFunctionTy())
      {
         return 8u;
      }
      else if(ty->isVoidTy())
      {
         return 0u;
      }
      else
      {
         return ty->isSized() ? DL->getTypeAllocSizeInBits(ty) : 8ULL;
      }
   }

   bool DumpBambuIR::IR_TYPE_PACKED(const void* t) const
   {
      if(IR_CODE(t) == IRC(SIGNEDPOINTERTYPE))
         return false;
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      ty = NormalizeSignedTag(ty);
      if(ty->isStructTy())
         return cast<llvm::StructType>(ty)->isPacked();
      else
         return false;
   }

   int DumpBambuIR::IR_TYPE_ALIGN(const void* t) const
   {
      if(IR_CODE(t) == IRC(SIGNEDPOINTERTYPE) || IR_CODE(t) == IRC(FUNCTION_TY_NODE) || IR_CODE(t) == IRC(VOID_TY_NODE))
         return 8;
      const llvm::Type* Cty = reinterpret_cast<const llvm::Type*>(t);
      llvm::Type* ty = const_cast<llvm::Type*>(NormalizeSignedTag(Cty));
      if(!ty->isSized())
         return 8;
#if PANDA_LLVM_CLANG_MAJOR < 12
      return std::max(8u, 8 * DL->getABITypeAlignment(ty));
#else
      return std::max(8ull, 8ull * DL->getABITypeAlign(ty).value());
#endif
   }

   const std::vector<const void*> DumpBambuIR::IR_TYPE_ARG_NODES(const void* t)
   {
      std::vector<const void*> res;
      assert(CheckSignedTag(reinterpret_cast<const llvm::Type*>(t)) == 0);
      assert(isa<llvm::FunctionType>(reinterpret_cast<const llvm::Type*>(t)));
      const llvm::FunctionType* llvm_obj = reinterpret_cast<const llvm::FunctionType*>(t);
      if(llvm_obj->params().empty())
         return res;
      for(const auto& par : llvm_obj->params())
      {
         assignCodeType(par);
         res.push_back(par);
      }
      return res;
   }

   uint64_t DumpBambuIR::NELEMENTS(const void* t)
   {
      assert(IR_CODE(t) == IRC(ARRAY_TY_NODE));
      const llvm::ArrayType* at = reinterpret_cast<const llvm::ArrayType*>(t);
      return at->getNumElements();
   }

   bool DumpBambuIR::stdarg_p(const void* t) const
   {
      assert(CheckSignedTag(reinterpret_cast<const llvm::Type*>(t)) == 0);
      assert(isa<llvm::FunctionType>(reinterpret_cast<const llvm::Type*>(t)));
      const llvm::FunctionType* llvm_obj = reinterpret_cast<const llvm::FunctionType*>(t);
      return llvm_obj->isVarArg();
   }

   llvm::ArrayRef<llvm::Type*> DumpBambuIR::IR_TYPE_FIELDS(const void* t)
   {
      assert(CheckSignedTag(reinterpret_cast<const llvm::Type*>(t)) == 0);
      assert(isa<llvm::StructType>(reinterpret_cast<const llvm::Type*>(t)));
      return reinterpret_cast<const llvm::StructType*>(t)->elements();
   }

   const void* DumpBambuIR::GET_FIELD_VAL_NODE(const void* t, unsigned int pos, const void* parent)
   {
      const llvm::StructType* scty = reinterpret_cast<const llvm::StructType*>(parent);
      if(index2field_val_node.find(std::make_pair(parent, pos)) == index2field_val_node.end())
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "fd%d", pos);
         std::string fdName = buffer;
         if(identifierTable.find(fdName) == identifierTable.end())
         {
            identifierTable.insert(fdName);
         }
         index2field_val_node[std::make_pair(parent, pos)].name =
             assignCode(identifierTable.find(fdName)->c_str(), IRC(IDENTIFIER_NODE));
         assert(CheckSignedTag(reinterpret_cast<const llvm::Type*>(t)) == 0);
         index2field_val_node[std::make_pair(parent, pos)].type = assignCodeType(scty->getElementType(pos));
         index2field_val_node[std::make_pair(parent, pos)].parent = assignCodeAuto(parent);
         index2field_val_node[std::make_pair(parent, pos)].size = IR_TYPE_BITSIZEALLOC(t);
         index2field_val_node[std::make_pair(parent, pos)].algn = IR_TYPE_ALIGN(t);
         auto offset =
             llvm::APInt(64, DL->getStructLayout(const_cast<llvm::StructType*>(scty))->getElementOffsetInBits(pos));
         SmallString<40> S;
         offset.toStringUnsigned(S);
         index2field_val_node[std::make_pair(parent, pos)].offset = S.c_str();
      }
      return assignCode(&index2field_val_node.find(std::make_pair(parent, pos))->second, IRC(FIELD_VAL_NODE));
   }

   const std::list<const void*> DumpBambuIR::IR_DECL_ARGUMENTS(const void* t)
   {
      const llvm::Function* fd = reinterpret_cast<const llvm::Function*>(t);
      const std::vector<std::string>* tracked_names = nullptr;
      if(fd->hasName() && fun2params)
      {
         const auto fun_parms_it = fun2params->find(getName(fd));
         if(fun_parms_it != fun2params->end())
         {
            tracked_names = &fun_parms_it->second;
         }
      }

      std::list<const void*> res;
      unsigned int par_index = 0;
      for(const auto& par : fd->args())
      {
         res.push_back(assignCodeAuto(&par));
         auto arg_name = getArgumentStringAttribute(fd, par_index, "bambu.orig_name");
         if(arg_name.empty() && tracked_names && par_index < tracked_names->size() &&
            tracked_names->size() == fd->arg_size())
         {
            arg_name = (*tracked_names)[par_index];
         }
         if(arg_name.empty())
         {
            if(par.hasName())
            {
               arg_name = par.getName().str();
            }
         }
         if(arg_name.empty())
         {
            arg_name = "P" + std::to_string(par_index);
         }
         argNameTable[&par] = arg_name;
         ++par_index;
      }
      return res;
   }

   const void* DumpBambuIR::getStatement_list(const void* t)
   {
      if(index2statement_list.find(t) == index2statement_list.end())
      {
         const llvm::Function* fd = reinterpret_cast<const llvm::Function*>(t);
         index2statement_list[t].F = fd;
      }
      return assignCode(&index2statement_list.at(t), IRC(STATEMENT_LIST_NODE));
   }

   const void* DumpBambuIR::getParentStmt(const void* g)
   {
      if(IR_CODE(g) == IRC(IR_NOP))
      {
         const ir_nop* gn = reinterpret_cast<const ir_nop*>(g);
         assert(HAS_CODE(gn));
         return gn->parent;
      }
      else if(IR_CODE(g) == IRC(IR_PHI_VIRTUAL))
      {
         const ir_phi_virtual* gpv = reinterpret_cast<const ir_phi_virtual*>(g);
         assert(HAS_CODE(gpv->parent));
         return gpv->parent;
      }
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      return assignCodeAuto(inst->getFunction());
   }

   int DumpBambuIR::get_bb_index(const void* g)
   {
      if(IR_CODE(g) == IRC(IR_NOP))
         return 0;
      if(IR_CODE(g) == IRC(IR_PHI_VIRTUAL))
      {
         const ir_phi_virtual* gpv = reinterpret_cast<const ir_phi_virtual*>(g);
         return gpv->bb_index;
      }
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      const llvm::BasicBlock* BB = inst->getParent();
      return getBB_index(BB);
   }

   bool DumpBambuIR::ir_has_mem_ops(const void* g)
   {
      if(IR_CODE(g) == IRC(IR_NOP))
         return false;
      if(IR_CODE(g) == IRC(IR_PHI_VIRTUAL))
         return false;
      llvm::Instruction* inst = const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(g));
      llvm::Function* currentFunction = inst->getFunction();
      llvm::MemorySSA& MSSA = GetMSSA(*currentFunction).getMSSA();
      return MSSA.getMemoryAccess(inst);
   }

   const void* DumpBambuIR::getVirtualDefStatement(llvm::MemoryAccess* defAccess, bool& isDefault,
                                                   const llvm::MemorySSA& MSSA, const llvm::Function* currentFunction)
   {
      if(MSSA.isLiveOnEntryDef(defAccess))
      {
         isDefault = true;
         assert(defAccess->getValueID() == llvm::Value::MemoryDefVal);
         return getNop(currentFunction,
                       currentFunction); /// LiveOnEntry is identified by the function and has scope the function again
      }
      else
      {
         if(defAccess->getValueID() == llvm::Value::MemoryDefVal)
         {
            return assignCodeAuto(dyn_cast<llvm::MemoryUseOrDef>(defAccess)->getMemoryInst());
         }
         else
         {
            assert(defAccess->getValueID() == llvm::Value::MemoryPhiVal);
            auto def_stmt = getVirtualPhi(dyn_cast<llvm::MemoryPhi>(defAccess), MSSA);
            assert(HAS_CODE(def_stmt));
            return def_stmt;
         }
      }
   }
   /// This does one-way checks to see if Use could theoretically be hoisted above
   /// MayClobber. This will not check the other way around.
   ///
   /// This assumes that, for the purposes of MemorySSA, Use comes directly after
   /// MayClobber, with no potentially clobbering operations in between them.
   /// (Where potentially clobbering ops are memory barriers, aliased stores, etc.)
   //    static bool areLoadsReorderable(const llvm::LoadInst* Use, const llvm::LoadInst* MayClobber)
   //    {
   //       bool VolatileUse = Use->isVolatile();
   //       bool VolatileClobber = MayClobber->isVolatile();
   //       // Volatile operations may never be reordered with other volatile operations.
   //       if(VolatileUse && VolatileClobber)
   //          return false;
   //       // Otherwise, volatile doesn't matter here. From the language reference:
   //       // 'optimizers may change the order of volatile operations relative to
   //       // non-volatile operations.'"
   //
   //       // If a load is seq_cst, it cannot be moved above other loads. If its ordering
   //       // is weaker, it can be moved above other loads. We just need to be sure that
   //       // MayClobber isn't an acquire load, because loads can't be moved above
   //       // acquire loads.
   //       //
   //       // Note that this explicitly *does* allow the free reordering of monotonic (or
   //       // weaker) loads of the same address.
   //       bool SeqCstUse = Use->getOrdering() == llvm::AtomicOrdering::SequentiallyConsistent;
   //       bool MayClobberIsAcquire = isAtLeastOrStrongerThan(MayClobber->getOrdering(),
   //       llvm::AtomicOrdering::Acquire); return !(SeqCstUse || MayClobberIsAcquire);
   //    }

   void DumpBambuIR::serialize_vops(const void* g)
   {
      assert(IR_CODE(g) != IRC(IR_PHI_VIRTUAL));
      llvm::Instruction* inst = const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(g));
      llvm::Function* currentFunction = inst->getFunction();
      auto& MSSA = GetMSSA(*currentFunction).getMSSA();
#if PANDA_LLVM_CLANG_MAJOR > 14
      MSSA.ensureOptimizedUses();
#endif
      const llvm::MemoryUseOrDef* ma = MSSA.getMemoryAccess(inst);
      if(ma->getValueID() == llvm::Value::MemoryUseVal || ma->getValueID() == llvm::Value::MemoryDefVal)
      {
         bool isDefault = false;
         llvm::MemoryAccess* defAccess = ma->getDefiningAccess();
         const void* def_stmt = getVirtualDefStatement(defAccess, isDefault, MSSA, currentFunction);
         const void* vuse = getSSA(ma, def_stmt, currentFunction, isDefault);
         serialize_child("memuse", vuse);
      }
      if(ma->getValueID() == llvm::Value::MemoryDefVal)
      {
         const void* vdef = getSSA(ma, g, currentFunction, false);
         serialize_child("memdef", vdef);
      }
      std::set<llvm::MemoryAccess*> visited;
      auto startingMA = MSSA.getMemoryAccess(inst);
      visited.insert(startingMA);
      /// check for true dependencies by exploiting LLVM alias analysis infrastructure
      if(ma->getValueID() == llvm::Value::MemoryDefVal)
      {
         const void* vdef = getSSA(ma, g, currentFunction, false);
         serialize_child("vdef", vdef);
      }
      auto isMemDefVal = startingMA->getValueID() == llvm::Value::MemoryDefVal;
      auto isSimpleDefUse = isa<llvm::CallInst>(inst) || isa<llvm::InvokeInst>(inst) || isa<llvm::FenceInst>(inst);
      if(isSimpleDefUse)
      {
         serialize_ir_aliased_reaching_defs(startingMA, MSSA, visited, inst->getFunction(), isMemDefVal, nullptr);
      }
      else
      {
         const auto Loc = llvm::MemoryLocation::get(inst);
         serialize_ir_aliased_reaching_defs(startingMA, MSSA, visited, inst->getFunction(), isMemDefVal, &Loc);
      }
   }

   void DumpBambuIR::serialize_ir_aliased_reaching_defs(llvm::MemoryAccess* MA, llvm::MemorySSA& MSSA,
                                                        std::set<llvm::MemoryAccess*>& visited,
                                                        const llvm::Function* currentFunction, bool isMemDefVal,
                                                        const llvm::MemoryLocation* Loc)
   {
      if(MSSA.isLiveOnEntryDef(MA))
      {
         return;
      }
      llvm::MemoryAccess* defMA = nullptr;
      if(MA->getValueID() != llvm::Value::MemoryPhiVal)
      {
         defMA = dyn_cast<llvm::MemoryUseOrDef>(MA)->getDefiningAccess();
         if(Loc && !MSSA.isLiveOnEntryDef(defMA))
         {
            defMA = MSSA.getWalker()->getClobberingMemoryAccess(defMA, *Loc);
         }
      }
      else
      {
         defMA = MA;
      }
      if(visited.find(defMA) != visited.end())
      {
         return;
      }
      visited.insert(defMA);

      auto manageMemoryDefVal = [&](llvm::MemoryAccess* defMemAcc) {
         bool isDefault = false;
         auto def_stmt = getVirtualDefStatement(defMemAcc, isDefault, MSSA, currentFunction);
         auto ssaV = getSSA(MA, def_stmt, currentFunction, isDefault);
         if(isMemDefVal)
         {
            serialize_child("vover", ssaV);
         }
         else
         {
            serialize_child("vuse", ssaV);
         }
         serialize_ir_aliased_reaching_defs(defMemAcc, MSSA, visited, currentFunction, isMemDefVal, Loc);
      };

      if(defMA->getValueID() == llvm::Value::MemoryDefVal)
      {
         manageMemoryDefVal(defMA);
      }
      else
      {
         assert(defMA->getValueID() == llvm::Value::MemoryPhiVal);
         auto mp = dyn_cast<llvm::MemoryPhi>(defMA);
         for(auto index = 0u; index < mp->getNumIncomingValues(); ++index)
         {
            auto val = mp->getIncomingValue(index);
            if(visited.find(val) == visited.end())
            {
               assert(val->getValueID() == llvm::Value::MemoryDefVal || val->getValueID() == llvm::Value::MemoryPhiVal);
               if(MSSA.isLiveOnEntryDef(val))
               {
               }
               else if(val->getValueID() == llvm::Value::MemoryDefVal)
               {
                  if(Loc && !MSSA.isLiveOnEntryDef(val))
                  {
                     val = MSSA.getWalker()->getClobberingMemoryAccess(val, *Loc);
                  }
                  manageMemoryDefVal(val);
               }
               else
               {
                  serialize_ir_aliased_reaching_defs(val, MSSA, visited, currentFunction, isMemDefVal, Loc);
               }
            }
         }
      }
   }

   const void* DumpBambuIR::IR_SSA_NAME_VAR(const void* t) const
   {
      const ssa_node* ssa = reinterpret_cast<const ssa_node*>(t);
      return ssa->var;
   }

   int DumpBambuIR::IR_SSA_NAME_VERSION(const void* t) const
   {
      const ssa_node* ssa = reinterpret_cast<const ssa_node*>(t);
      return ssa->vers;
   }

   const void* DumpBambuIR::IR_SSA_NAME_DEF_STMT(const void* t) const
   {
      const ssa_node* ssa = reinterpret_cast<const ssa_node*>(t);
      return ssa->def_stmts;
   }

   const void* DumpBambuIR::getMinValue(const void* t)
   {
      const ssa_node* ssa = reinterpret_cast<const ssa_node*>(t);
      if(ssa->var || ssa->isVirtual)
         return nullptr;
      llvm::Instruction* inst =
          const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(ssa->def_stmts));
      if(inst->getType()->isIntegerTy())
      {
         llvm::BasicBlock* BB = inst->getParent();
         llvm::Function* currentFunction = inst->getFunction();
         llvm::LazyValueInfo& LVI = GetLVI(*currentFunction);
         llvm::ConstantRange range =
#if PANDA_LLVM_CLANG_MAJOR < 12
             LVI.getConstantRange(inst, BB, inst);
#else
             LVI.getConstantRange(inst, inst, true);
#endif
         auto isSigned = CheckSignedTag(IR_TYPE_NODE(t));
         if(!range.isFullSet())
         {
#ifdef DEBUG_RA
            if(isSigned)
               llvm::errs() << "Range: <" << range.getSignedMin() << "," << range.getSignedMax() << "> ";
            else
               llvm::errs() << "Range: <" << range.getUnsignedMin().getZExtValue() << ","
                            << range.getUnsignedMax().getZExtValue() << "> ";
            inst->print(llvm::errs());
            llvm::errs() << "\n";
#endif
            auto val = isSigned ? range.getSignedMin() : range.getUnsignedMin();
            return getIntegerCST(isSigned, inst->getContext(), val, IR_TYPE_NODE(t));
         }
         else
            return nullptr;
      }
      else
         return nullptr;
   }

   const void* DumpBambuIR::getMaxValue(const void* t)
   {
      const ssa_node* ssa = reinterpret_cast<const ssa_node*>(t);
      if(ssa->var || ssa->isVirtual)
         return nullptr;
      llvm::Instruction* inst =
          const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(ssa->def_stmts));
      if(inst->getType()->isIntegerTy())
      {
         llvm::BasicBlock* BB = inst->getParent();
         llvm::Function* currentFunction = inst->getFunction();
         auto& LVI = GetLVI(*currentFunction);
         auto isSigned = CheckSignedTag(IR_TYPE_NODE(t));

         llvm::ConstantRange range =
#if PANDA_LLVM_CLANG_MAJOR < 12
             LVI.getConstantRange(inst, BB, inst);
#else
             LVI.getConstantRange(inst, inst, true);
#endif
         if(!range.isFullSet())
         {
            auto val = isSigned ? range.getSignedMax() : range.getUnsignedMax();
            return getIntegerCST(isSigned, inst->getContext(), val, IR_TYPE_NODE(t));
         }
         else
         {
            return nullptr;
         }
      }
      else
         return nullptr;
   }

   const std::list<std::pair<const void*, const void*>> DumpBambuIR::CONSTRUCTOR_ELTS(const void* t)
   {
      std::list<std::pair<const void*, const void*>> res;
      const llvm::Value* llvm_obj = reinterpret_cast<const llvm::Value*>(t);
      auto vid = llvm_obj->getValueID();
      switch(vid)
      {
         case llvm::Value::ConstantAggregateZeroVal:
         {
            const llvm::ConstantAggregateZero* val = reinterpret_cast<const llvm::ConstantAggregateZero*>(t);
            auto type = val->getType();
            if(dyn_cast<llvm::ArrayType>(type) || dyn_cast<llvm::VectorType>(type))
            {
#if PANDA_LLVM_CLANG_MAJOR >= 13
               for(unsigned index = 0; index < val->getElementCount().getFixedValue(); ++index)
#else
               for(unsigned index = 0; index < val->getNumElements(); ++index)
#endif
               {
                  auto indexAPInt = llvm::APInt(32, index);
                  if(uicTable.find(indexAPInt) == uicTable.end())
                  {
                     uicTable[indexAPInt] = assignCodeAuto(llvm::ConstantInt::get(llvm_obj->getContext(), indexAPInt));
                  }
                  const void* idx = uicTable.find(indexAPInt)->second;
                  const void* valu = getOperand(val->getSequentialElement(), nullptr);
                  res.push_back(std::make_pair(idx, valu));
               }
            }
            else
            {
               const void* ty = IR_TYPE_NODE(t);
#if PANDA_LLVM_CLANG_MAJOR >= 13
               for(unsigned index = 0; index < val->getElementCount().getFixedValue(); ++index)
#else
               for(unsigned index = 0; index < val->getNumElements(); ++index)
#endif
               {
                  auto op = val->getStructElement(index);
                  const void* valu = getOperand(op, nullptr);
                  const void* idx = GET_FIELD_VAL_NODE(IR_TYPE_NODE(assignCodeAuto(op)), index, ty);
                  res.push_back(std::make_pair(idx, valu));
               }
            }
            return res;
         }
         case llvm::Value::ConstantStructVal:
         {
            const llvm::ConstantStruct* val = reinterpret_cast<const llvm::ConstantStruct*>(t);
            const void* ty = IR_TYPE_NODE(t);
            for(unsigned index = 0; index < val->getNumOperands(); ++index)
            {
               auto op = val->getOperand(index);
               const void* valu = getOperand(op, nullptr);
               const void* idx = GET_FIELD_VAL_NODE(IR_TYPE_NODE(assignCodeAuto(op)), index, ty);
               res.push_back(std::make_pair(idx, valu));
            }
            return res;
         }
         case llvm::Value::ConstantDataArrayVal:
         case llvm::Value::ConstantDataVectorVal:
         {
            const llvm::ConstantDataSequential* val = reinterpret_cast<const llvm::ConstantDataSequential*>(t);
            for(unsigned index = 0; index < val->getNumElements(); ++index)
            {
               auto indexAPInt = llvm::APInt(64, index);
               if(uicTable.find(indexAPInt) == uicTable.end())
               {
                  uicTable[indexAPInt] = assignCodeAuto(llvm::ConstantInt::get(llvm_obj->getContext(), indexAPInt));
               }
               const void* idx = uicTable.find(indexAPInt)->second;
               const void* valu = getOperand(val->getElementAsConstant(index), nullptr);
               res.push_back(std::make_pair(idx, valu));
            }
            return res;
         }
         case llvm::Value::ConstantArrayVal:
         {
            const llvm::ConstantArray* val = reinterpret_cast<const llvm::ConstantArray*>(t);
            for(unsigned index = 0; index < val->getNumOperands(); ++index)
            {
               auto indexAPInt = llvm::APInt(64, index);
               if(uicTable.find(indexAPInt) == uicTable.end())
               {
                  uicTable[indexAPInt] = assignCodeAuto(llvm::ConstantInt::get(llvm_obj->getContext(), indexAPInt));
               }
               const void* idx = uicTable.find(indexAPInt)->second;
               auto elmnt = val->getOperand(index);
               const void* valu = getOperand(elmnt, nullptr);
               res.push_back(std::make_pair(idx, valu));
            }
            return res;
         }

         default:
            llvm::errs() << "CONSTRUCTOR_ELTS kind not supported: " << ValueTyNames[vid] << "\n";
            stream.close();
            report_fatal_error("Plugin Error");
      }
   }

   void DumpBambuIR::serialize_new_line()
   {
      snprintf(buffer, LOCAL_BUFFER_LEN, "\n%*s", SOL_COLUMN, "");
      stream << buffer;
      column = SOL_COLUMN;
   }

   void DumpBambuIR::serialize_maybe_newline()
   {
      int extra;

      /* See if we need a new line. */
      if(column > EOL_COLUMN)
         serialize_new_line();
      /* See if we need any padding.  */
      else if((extra = (column - SOL_COLUMN) % COLUMN_ALIGNMENT) != 0)
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "%*s", COLUMN_ALIGNMENT - extra, "");
         stream << buffer;
         column += COLUMN_ALIGNMENT - extra;
      }
   }

   void DumpBambuIR::serialize_pointer(const char* field, const void* ptr)
   {
      serialize_maybe_newline();
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: %-8llx ", field, (unsigned long long)ptr);
      stream << buffer;
      column += 15;
   }

   void DumpBambuIR::DumpVersion(llvm::raw_fd_ostream& stream)
   {
      auto node_count_str = std::to_string(last_used_index);
      node_count_str = std::string(10 - node_count_str.size(), ' ') + node_count_str;
      stream.seek(0);
      stream << "COMPILER_VERSION: \"Clang " PANDA_LLVM_CLANG_VERSION_STR "\"\nPLUGIN_VERSION: \"" PANDA_PLUGIN_VERSION
                "\"\nNODE_COUNT: "
             << node_count_str << "\n";
   }

   void DumpBambuIR::serialize_int(const char* field, int i)
   {
      serialize_maybe_newline();
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: %-7d ", field, i);
      stream << buffer;
      column += 14;
   }

   /* Serialize wide integer i using FIELD to identify it.  */
   void DumpBambuIR::serialize_int_cst(const char* field, const std::string& i)
   {
      serialize_maybe_newline();
      snprintf(buffer, LOCAL_BUFFER_LEN,
               "%-4s: "
               "%s",
               field, i.c_str());
      stream << buffer;
      column += 21;
   }

   static void real_to_hexadecimal(char* buffer, unsigned size_buff, const llvm::APFloat& val)
   {
      llvm::APInt API = val.bitcastToAPInt();
      llvm::APInt APIAbs = API;
      APIAbs.clearBit(API.getBitWidth() - 1);
      auto sem = &val.getSemantics();
      unsigned nbitsExp = 0;
      unsigned nbitsMan = 0;
      if(sem == &llvm::APFloat::IEEEhalf())
      {
         nbitsExp = 5;
         nbitsMan = 10;
      }
      else if(sem == &llvm::APFloat::IEEEsingle())
      {
         nbitsExp = 8;
         nbitsMan = 23;
      }
      else if(sem == &llvm::APFloat::IEEEdouble())
      {
         nbitsExp = 11;
         nbitsMan = 52;
      }
      else if(sem == &llvm::APFloat::x87DoubleExtended())
      {
         nbitsExp = 16;
         nbitsMan = 63;
      }
      else if(sem == &llvm::APFloat::PPCDoubleDouble())
      {
         report_fatal_error("PPCDoubleDouble format not supported in real_to_hexadecimal");
      }
      else if(sem == &llvm::APFloat::IEEEquad())
      {
         nbitsExp = 15;
         nbitsMan = 112;
      }
      else
         report_fatal_error("unexpected floating point format in real_to_hexadecimal");

      unsigned ExpBiased = API.lshr(nbitsMan).getZExtValue() & ((1U << nbitsExp) - 1);
      const auto zeroVal =
#if PANDA_LLVM_CLANG_MAJOR < 17
          llvm::APInt::getNullValue(API.getBitWidth());
#else
          llvm::APInt::getZero(API.getBitWidth());
#endif
      int ExpUnbiased = (zeroVal == APIAbs) ? 0 : (ExpBiased - ((1U << (nbitsExp - 1)) - 2));

      snprintf(buffer, size_buff, "p%+d", ExpUnbiased);

      const auto nOnes =
#if PANDA_LLVM_CLANG_MAJOR < 17
          llvm::APInt::getAllOnesValue(nbitsMan);
#else
          llvm::APInt::getAllOnes(nbitsMan);
#endif
      llvm::APInt Mantissa = API & nOnes.zext(API.getBitWidth());
      if(ExpBiased != 0)
      {
         Mantissa.setBit(nbitsMan);
      }

      size_t digits = size_buff - strlen(buffer) - (val.isNegative() ? 1 : 0) - 4 - 1;
      assert(digits <= size_buff);
      char* current = buffer;
      if(val.isNegative())
         *current++ = '-';
      *current++ = '0';
      *current++ = 'x';
      *current++ = '0';
      *current++ = '.';
      for(int index1 = ((nbitsMan + 1) / 4) - !((nbitsMan + 1) % 4); index1 >= 0 && digits > 0; --index1)
      {
         *current++ = "0123456789abcdef"[(Mantissa.lshr(index1 * 4).getLoBits(4).getZExtValue())];
         --digits;
      }
      sprintf(current, "p%+d", ExpUnbiased);
   }

   /* Serialize real r using FIELD to identify it.  */
   void DumpBambuIR::serialize_real(const void* t)
   {
      serialize_maybe_newline();
      /* Code copied from print_node.  */
      /*if(IR_OVERFLOW(t))
      {
         stream << "overflow ";
         column += 8;
      }*/
      assert(reinterpret_cast<const llvm::ConstantFP*>(t)->getValueID() == llvm::Value::ConstantFPVal);
      const llvm::APFloat& d = reinterpret_cast<const llvm::ConstantFP*>(t)->getValueAPF();
      if(d.isInfinity())
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "valr: %-7s ", "\"Inf\"");
         stream << buffer;
         if(d.isNegative())
            snprintf(buffer, LOCAL_BUFFER_LEN, "valx: %-7s ", "\"-Inf\"");
         else
            snprintf(buffer, LOCAL_BUFFER_LEN, "valx: %-7s ", "\"Inf\"");
         stream << buffer;
      }
      else if(d.isNaN())
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "valr: %-7s ", "\"Nan\"");
         stream << buffer;
         if(d.isNegative())
            snprintf(buffer, LOCAL_BUFFER_LEN, "valx: %-7s ", "\"-Nan\"");
         else
            snprintf(buffer, LOCAL_BUFFER_LEN, "valx: %-7s ", "\"Nan\"");
         stream << buffer;
      }
      else
      {
         bool isDouble = &d.getSemantics() == &llvm::APFloat::IEEEdouble();
         snprintf(buffer, LOCAL_BUFFER_LEN, "%.*g", (isDouble ? __DBL_DECIMAL_DIG__ : __FLT_DECIMAL_DIG__),
                  (isDouble ? d.convertToDouble() : d.convertToFloat()));
         std::string literalReal = std::string(buffer);
         if(literalReal.find('.') == std::string::npos && literalReal.find('e') == std::string::npos)
            literalReal = literalReal + ".";
         if(!isDouble && literalReal.find('e') == std::string::npos)
            literalReal = literalReal + "f";
         stream << "valr: \"" << literalReal << "\" ";

         stream << "valx: \"";
         real_to_hexadecimal(buffer, LOCAL_BUFFER_LEN, d);
         stream << std::string(buffer);
         stream << "\"";
      }
      column += 21;
   }

   int DumpBambuIR::serialize_with_double_quote(const char* input, int length)
   {
      int new_length;
      stream << "\"";
      new_length = serialize_with_escape(input, length);
      stream << "\"";
      return new_length + 2;
   }

   /* Add a backslash before an escape sequence to serialize the string
      with the escape sequence */
   int DumpBambuIR::serialize_with_escape(const char* input, int length)
   {
      int i;
      int k = 0;
      for(i = 0; i < length; i++)
      {
         switch(input[i])
         {
            case '\n':
            {
               /* new line*/
               stream << "\\";
               stream << "n";
               k += 2;
               break;
            }
            case '\t':
            {
               /* horizontal tab */
               stream << "\\";
               stream << "t";
               k += 2;
               break;
            }
            case '\v':
            {
               /* vertical tab */
               stream << "\\";
               stream << "v";
               k += 2;
               break;
            }
            case '\b':
            {
               /* backspace */
               stream << "\\";
               stream << "b";
               k += 2;
               break;
            }
            case '\r':
            {
               /* carriage return */
               stream << "\\";
               stream << "r";
               k += 2;
               break;
            }
            case '\f':
            {
               /* jump page */
               stream << "\\";
               stream << "f";
               k += 2;
               break;
            }
            case '\a':
            {
               /* alarm */
               stream << "\\";
               stream << "a";
               k += 2;
               break;
            }
            case '\\':
            {
               /* backslash */
               stream << "\\";
               stream << "\\";
               k += 2;
               break;
            }
            case '\"':
            {
               /* double quote */
               stream << "\\";
               stream << "\"";
               k += 2;
               break;
            }
            case '\'':
            {
               /* quote */
               stream << "\\";
               stream << "\'";
               k += 2;
               break;
            }
            case '\0':
            {
               /* null */
               stream << "\\";
               stream << "0";
               k += 2;
               break;
            }
            default:
            {
               stream << input[i];
               k++;
            }
         }
      }
      return k;
   }

   /* Serialize the string S.  */
   void DumpBambuIR::serialize_string(const char* string)
   {
      serialize_maybe_newline();
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-13s ", string);
      stream << buffer;
      if(std::strlen(string) > 13)
         column += strlen(string) + 1;
      else
         column += 14;
   }

   /* Serialize the string field S.  */
   void DumpBambuIR::serialize_string_field(const char* field, const char* str)
   {
      int length;
      serialize_maybe_newline();
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: ", field);
      stream << buffer;
      length = std::strlen(str);
      length = serialize_with_double_quote(str, length);
      if(length > 7)
         column += 6 + length + 1;
      else
         column += 14;
   }

   void DumpBambuIR::queue_and_serialize_index(const char* field, const void* t)
   {
      LLVM_DEBUG(llvm::dbgs() << "field:" << field << "\n");
      unsigned int index;
      if(t == nullptr)
      {
         return;
      }
      if(llvm2index.find(t) != llvm2index.end())
      {
         index = llvm2index.find(t)->second;
         assert(index <= last_used_index);
      }
      else
      {
         /* If we haven't, add it to the queue.  */
         index = queue(t);
      }
      serialize_maybe_newline();
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: ", field);
      stream << buffer;
      column += 6;
      serialize_index(index);
   }

   void DumpBambuIR::serialize_index(unsigned int index)
   {
      snprintf(buffer, LOCAL_BUFFER_LEN, "@%-6u ", index);
      stream << buffer;
      column += 8;
   }

   void DumpBambuIR::queue_and_serialize_type(const void* t)
   {
      queue_and_serialize_index("type", IR_TYPE_NODE(t));
   }

   static void computeLoopLabels(std::map<const llvm::Loop*, unsigned>& loopLabes, llvm::Loop* curLoop,
                                 unsigned int& label)
   {
      loopLabes[curLoop] = label;
      label++;
      for(auto it = curLoop->begin(); it != curLoop->end(); ++it)
      {
         computeLoopLabels(loopLabes, *it, label);
      }
   }

   void DumpBambuIR::dequeue_and_serialize_ir(const void* g)
   {
      assert(llvm2index.find(g) != llvm2index.end());
      unsigned int index = llvm2index.find(g)->second;

      auto code = IR_CODE(g);
      const char* code_name = GET_IR_CODE_NAME(code);
#ifndef NDEBUG
      if(code != IRC(IR_NOP) && code != IRC(IR_PHI_VIRTUAL))
      {
         llvm::Instruction* inst = const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(g));
         llvm::Function* currentFunction = inst->getFunction();
         LLVM_DEBUG(llvm::dbgs() << "@" << code_name << " @" << index << "\n");
         auto& MSSA = GetMSSA(*currentFunction).getMSSA();
         if(MSSA.getMemoryAccess(inst))
         {
            LLVM_DEBUG(llvm::dbgs() << *inst << " | " << *MSSA.getMemoryAccess(inst) << "\n");
         }
         else
         {
            LLVM_DEBUG(llvm::dbgs() << *inst << "\n");
         }
      }
      else
      {
         LLVM_DEBUG(llvm::dbgs() << "@" << code_name << "\n");
      }
#endif
      /* Print the node index.  */
      serialize_index(index);

      snprintf(buffer, LOCAL_BUFFER_LEN, "%-16s ", code_name);
      stream << buffer;
      column = 25;
      serialize_child("parent", getParentStmt(g));
      serialize_int("bb_index", get_bb_index(g));

      if(ir_has_mem_ops(g))
         serialize_vops(g);

      if(ir_has_location(g))
      {
         expanded_location xloc = expand_location(ir_location(g));
         serialize_maybe_newline();
         snprintf(buffer, LOCAL_BUFFER_LEN, "loc_info: \"%s\":%-d:%-6d ", xloc.file, xloc.line, xloc.column);
         if(xloc.file && xloc.file[0])
         {
            stream << buffer;
            column += 12 + strlen(xloc.file) + 8;
         }
      }
      switch(ir_code(g))
      {
         case IRC(IR_ASSIGN_ALLOCA):
         {
            auto lhs = ir_assign_lhs(g);
            auto rhs = ir_assign_rhs_alloca(g);
            add_alloca_pt_solution(lhs, rhs);
            serialize_child("op", lhs);
            serialize_child("op", rhs);
            if(index2alloca_var.find(g)->second.addr)
               serialize_string("addr");
            break;
         }
         case IRC(GETELEMENTPTR):
         {
            auto lhs = ir_assign_lhs(g);
            auto rhs = ir_assign_rhs_getelementptr(g);
            serialize_child("op", lhs);
            serialize_child("op", rhs);
            std::set<const llvm::User*> visited;
            auto currInst = reinterpret_cast<const llvm::User*>(g);
            visited.insert(currInst);
            auto castInst = reinterpret_cast<const llvm::Instruction*>(g);
            const llvm::TargetLibraryInfo& TLI = GetTLI(*const_cast<llvm::Function*>(castInst->getFunction()));
            if(temporary_addr_check(currInst, visited, TLI))
               serialize_string("addr");
            break;
         }
         case IRC(IR_SSACOPY):
         {
            auto lhs = ir_assign_lhs(g);
            auto rhs = ir_assign_rhs1(g);
            serialize_child("op", lhs);
            serialize_child("op", rhs);
            break;
         }
         case IRC(IR_ASSIGN):
         {
            if(get_ir_rhs_class(ir_expr_code(g)) == IR_TERNARY_RHS)
            {
               auto lhs = ir_assign_lhs(g);
               auto tc = ir_assign_rhs_code(g);
               serialize_child("op", lhs);
               auto op0 = ir_assign_rhs1(g);
               auto op1 = ir_assign_rhs2(g);
               auto op2 = ir_assign_rhs3(g);
               serialize_child("op", build3(tc, IR_TYPE_NODE(lhs), op0, op1, op2));
            }
            else if(get_ir_rhs_class(ir_expr_code(g)) == IR_BINARY_RHS)
            {
               auto lhs = ir_assign_lhs(g);
               auto tc = ir_assign_rhs_code(g);
               serialize_child("op", lhs);
               serialize_child("op", build2(tc, IR_TYPE_NODE(lhs), ir_assign_rhs1(g), ir_assign_rhs2(g)));
            }
            else if(get_ir_rhs_class(ir_expr_code(g)) == IR_UNARY_RHS)
            {
               auto lhs = ir_assign_lhs(g);
               auto tc = ir_assign_rhs_code(g);
               serialize_child("op", lhs);
               auto rhs = build1(tc, IR_TYPE_NODE(lhs), ir_assign_rhs1(g));
               serialize_child("op", rhs);
            }
            else if(get_ir_rhs_class(ir_expr_code(g)) == IR_SINGLE_RHS)
            {
               auto lhs = ir_assign_lhs(g);
               serialize_child("op", lhs);
               auto ltype = IR_TYPE_NODE(lhs);
               auto rhs = ir_assign_rhs1(g);
               auto rtype = IR_TYPE_NODE(rhs);
               if(ltype == rtype)
                  serialize_child("op", rhs);
               else
                  serialize_child("op", build1(IRC(BITCAST_NODE), ltype, rhs));
            }
            else if(ir_expr_code(g) == IRC(CALL_NODE))
            {
               serialize_child("op", ir_assign_lhs(g));
               serialize_child("op", build_custom_function_call_node(g));
            }
            else if(ir_expr_code(g) == IRC(UNLT_NODE) || ir_expr_code(g) == IRC(UNGE_NODE) ||
                    ir_expr_code(g) == IRC(UNGT_NODE) || ir_expr_code(g) == IRC(UNLE_NODE))
            {
               auto gcode = ir_expr_code(g);
               const auto inv_compare = [&]() {
                  if(gcode == IRC(UNLT_NODE))
                  {
                     return IRC(GE_NODE);
                  }
                  if(gcode == IRC(UNGE_NODE))
                  {
                     return IRC(LT_NODE);
                  }
                  if(gcode == IRC(UNGT_NODE))
                  {
                     return IRC(LE_NODE);
                  }
                  if(gcode == IRC(UNLE_NODE))
                  {
                     return IRC(GT_NODE);
                  }
                  llvm::errs() << "not expected gcode\n";
                  stream.close();
                  report_fatal_error("Plugin Error");
               }();
               const llvm::FCmpInst* cmpInst = reinterpret_cast<const llvm::FCmpInst*>(g);
               auto lhs = ir_assign_lhs(g);
               serialize_child("op", lhs);
               auto btype = IR_TYPE_NODE(lhs);
               auto op1 = ir_assign_rhs1(g);
               auto op2 = ir_assign_rhs2(g);
               auto in_expr = build2(inv_compare, btype, op1, op2);
               auto rhs = build1(IRC(NOT_NODE), btype, in_expr);
               serialize_child("op", rhs);
            }
            else if(ir_expr_code(g) == IRC(FCMP_OEQ) || ir_expr_code(g) == IRC(FCMP_ONE) ||
                    ir_expr_code(g) == IRC(FCMP_ORD) || ir_expr_code(g) == IRC(FCMP_UEQ) ||
                    ir_expr_code(g) == IRC(FCMP_UNE) || ir_expr_code(g) == IRC(FCMP_UNO))
            {
               const llvm::FCmpInst* cmpInst = reinterpret_cast<const llvm::FCmpInst*>(g);

               auto noNAN = cmpInst->getFastMathFlags().noNaNs();
               auto lhs = ir_assign_lhs(g);
               serialize_child("op", lhs);
               auto btype = IR_TYPE_NODE(lhs);
               auto llvm_op1 = cmpInst->getOperand(0);
               auto llvm_op2 = cmpInst->getOperand(1);
               bool isOp1Const = isa<llvm::ConstantFP>(llvm_op1);
               bool isOp2Const = isa<llvm::ConstantFP>(llvm_op2);
               auto bitsize1 = llvm_op1->getType()->getPrimitiveSizeInBits();
               assert(llvm_op2->getType()->getPrimitiveSizeInBits() == bitsize1);
               auto intObjType = llvm::Type::getIntNTy(*moduleContext, static_cast<unsigned>(bitsize1));
               auto op1 = ir_assign_rhs1(g);
               auto op2 = ir_assign_rhs2(g);
               auto vcType = assignCodeType(intObjType);
               auto vc_op1 = isOp1Const ?
                                 assignCodeAuto(llvm::ConstantInt::get(
                                     intObjType, cast<llvm::ConstantFP>(llvm_op1)->getValueAPF().bitcastToAPInt())) :
                                 build1(IRC(BITCAST_NODE), vcType, op1);
               auto vc_op2 = isOp2Const ?
                                 assignCodeAuto(llvm::ConstantInt::get(
                                     intObjType, cast<llvm::ConstantFP>(llvm_op2)->getValueAPF().bitcastToAPInt())) :
                                 build1(IRC(BITCAST_NODE), vcType, op2);
               auto constOne = assignCodeAuto(llvm::ConstantInt::get(intObjType, 1, false));
               auto lshift_op1 = build2(IRC(SHL_NODE), vcType, vc_op1, constOne);
               auto abs_op1 = build2(IRC(SHR_NODE), vcType, lshift_op1, constOne);
               auto lshift_op2 = build2(IRC(SHL_NODE), vcType, vc_op2, constOne);
               auto abs_op2 = build2(IRC(SHR_NODE), vcType, lshift_op2, constOne);
               const void* constNAN = nullptr;
               if(bitsize1 == 32)
                  constNAN = assignCodeAuto(llvm::ConstantInt::get(intObjType, 0x7f800000, false));
               else if(bitsize1 == 64)
                  constNAN = assignCodeAuto(llvm::ConstantInt::get(intObjType, 0x7FF0000000000000, false));
               else
               {
                  llvm::errs() << bitsize1 << "\n";
                  report_fatal_error("unsupported floating point bitsize");
               }
               auto isNAN_op1 = build2(IRC(GT_NODE), btype, abs_op1, constNAN);
               auto isNAN_op2 = build2(IRC(GT_NODE), btype, abs_op2, constNAN);
               const void* rhs = nullptr;
               auto gcode = ir_expr_code(g);
               if(gcode == IRC(FCMP_ORD) || gcode == IRC(FCMP_OEQ) || gcode == IRC(FCMP_ONE))
               {
                  auto isNotNAN_op1 = build1(IRC(NOT_NODE), btype, isNAN_op1);
                  auto isNotNAN_op2 = build1(IRC(NOT_NODE), btype, isNAN_op2);
                  auto ordered = build2(IRC(AND_NODE), btype, isNotNAN_op1, isNotNAN_op2);
                  if(gcode == IRC(FCMP_ORD))
                  {
                     assert(!noNAN);
                     rhs = ordered;
                  }
                  else if(gcode == IRC(FCMP_OEQ))
                  {
                     auto eq = build2(IRC(EQ_NODE), btype, op1, op2);
                     rhs = noNAN ? eq : build2(IRC(AND_NODE), btype, ordered, eq);
                  }
                  else if(gcode == IRC(FCMP_ONE))
                  {
                     auto neq = build2(IRC(NE_NODE), btype, op1, op2);
                     rhs = noNAN ? neq : build2(IRC(AND_NODE), btype, ordered, neq);
                  }
                  else
                     report_fatal_error("unexpected case");
               }
               else
               {
                  auto unordered = build2(IRC(IOR_NODE), btype, isNAN_op1, isNAN_op2);
                  if(gcode == IRC(FCMP_UNO))
                  {
                     assert(!noNAN);
                     rhs = unordered;
                  }
                  else if(gcode == IRC(FCMP_UEQ))
                  {
                     auto eq = build2(IRC(EQ_NODE), btype, op1, op2);
                     rhs = noNAN ? eq : build2(IRC(IOR_NODE), btype, unordered, eq);
                  }
                  else if(gcode == IRC(FCMP_UNE))
                  {
                     auto neq = build2(IRC(NE_NODE), btype, op1, op2);
                     rhs = noNAN ? neq : build2(IRC(IOR_NODE), btype, unordered, neq);
                  }
                  else
                     report_fatal_error("unexpected case");
               }
               serialize_child("op", rhs);
            }
            else if(ir_expr_code(g) == IRC(INSERTVALUE))
            {
               serialize_child("op", ir_assign_lhs(g));
               auto rhs = ir_assign_rhs_insertvalue(g);
               serialize_child("op", rhs);
            }
            else if(ir_expr_code(g) == IRC(EXTRACTVALUE))
            {
               serialize_child("op", ir_assign_lhs(g));
               auto rhs = ir_assign_rhs_extractvalue(g);
               serialize_child("op", rhs);
            }
            else
               report_fatal_error("unexpected condition");
            break;
         }
         case IRC(IR_COND):
         {
            for(auto def_dest : ir_ifelse_pairs(g))
            {
               if(def_dest.first)
               {
                  serialize_child("op", def_dest.first);
               }
               serialize_int("bloc", def_dest.second);
            }
            break;
         }
         case IRC(IR_GOTO):
            report_fatal_error("unexpected IR_GOTO"); /// serialize_child ("op", ir_goto_dest (g));
            break;

         case IRC(IR_NOPMEM):
            break;

         case IRC(IR_NOP):
            break;

         case IRC(IR_RETURN):
            serialize_child("op", ir_return_retval(g));
            break;

         case IRC(IR_IFELSEIF):
         {
            for(auto def_dest : ir_ifelseif_pairs(g))
            {
               if(def_dest.first)
               {
                  serialize_child("op", def_dest.first);
               }
               serialize_int("bloc", def_dest.second);
            }
            break;
         }
         case IRC(IR_PHI):
         {
            serialize_child("res", ir_phi_result(g));
            std::set<int> bb_visited;
            for(auto i = 0u; i < ir_phi_num_args(g); i++)
            {
               auto bbIndex = ir_phi_arg_edgeBBindex(g, i);
               if(bb_visited.find(bbIndex) == bb_visited.end())
               {
                  bb_visited.insert(bbIndex);
                  serialize_child("def", ir_phi_arg_def(g, i));
                  serialize_int("edge", bbIndex);
               }
            }
            break;
         }
         case IRC(IR_PHI_VIRTUAL):
         {
            serialize_child("res", ir_phi_virtual_result(g));
            std::set<int> bb_visited;
            for(auto i = 0u; i < ir_phi_virtual_num_args(g); i++)
            {
               auto bbIndex = ir_phi_virtual_arg_edgeBBindex(g, i);
               if(bb_visited.find(bbIndex) == bb_visited.end())
               {
                  bb_visited.insert(bbIndex);
                  serialize_child("def", ir_phi_virtual_arg_def(g, i));
                  serialize_int("edge", bbIndex);
               }
            }
            serialize_string("virtual");
            break;
         }
         case IRC(IR_CALL):
         {
            serialize_child("fn", ir_call_fn(g));
            unsigned int arg_index;
            for(arg_index = 0; arg_index < ir_call_num_args(g); arg_index++)
            {
               serialize_child("arg", ir_call_arg(g, arg_index));
            }
            break;
         }
         default:
            report_fatal_error("unexpected statement");
      }

      /* Terminate the line.  */
      stream << "\n";
   }

   void DumpBambuIR::dequeue_and_serialize_statement(const void* t)
   {
      assert(llvm2index.find(t) != llvm2index.end());
      unsigned int index = llvm2index.find(t)->second;

      const char* code_name = GET_IR_CODE_NAME(IR_CODE(t));

      /* Print the node index.  */
      serialize_index(index);

      snprintf(buffer, LOCAL_BUFFER_LEN, "%-16s ", code_name);
      stream << buffer;
      column = 25;

      /* In case of basic blocks the function print:
                     + first a list of all statements
                     +  then for each basic block it prints:
                        - the number of the basic block
                        - the predecessor basic block
                        - the successor basic block
                        - list of statement
                     + otherwise it prints only the list of statements */
      const auto sl = reinterpret_cast<const statement_list_node*>(t);
      llvm::Function* currentFunction = const_cast<llvm::Function*>(sl->F);
      auto& LI = GetLI(*currentFunction);
      std::map<const llvm::Loop*, unsigned> loopLabes;
      if(!LI.empty())
      {
         unsigned int label = 1;
         for(auto it = LI.begin(); it != LI.end(); ++it)
            computeLoopLabels(loopLabes, *it, label);
      }
      auto& MSSA = GetMSSA(*currentFunction).getMSSA();

      for(const auto& BB : *currentFunction)
      {
         const char* field;
         serialize_new_line();
         serialize_int("bloc", getBB_index(&BB));
         LLVM_DEBUG(llvm::dbgs() << "BB" << getBB_index(&BB) << "\n");
         if(MSSA.getMemoryAccess(&BB))
         {
            LLVM_DEBUG(llvm::dbgs() << "|!!!!!!!!!!!!!!!!!! " << *MSSA.getMemoryAccess(&BB) << "\n");
         }
         if(!LI.empty() && LI.getLoopFor(&BB) && LI.getLoopFor(&BB)->getHeader() == &BB &&
            LI.getLoopFor(&BB)->isAnnotatedParallel())
         {
            /// to be checked
            // serialize_string("hpl");
         }
         if(LI.empty() || !LI.getLoopFor(&BB))
         {
            serialize_int("loop_id", 0);
         }
         else
         {
            serialize_int("loop_id", loopLabes.find(LI.getLoopFor(&BB))->second);
         }
         if(llvm::pred_begin(&BB) == llvm::pred_end(&BB))
         {
            serialize_maybe_newline();
            field = "pred: ENTRY";
            snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s ", field);
            stream << buffer;
            column += 14;
         }
         else
         {
            std::set<int> pred_visited;
            for(const auto pred : llvm::predecessors(&BB))
            {
               auto bbIndex = getBB_index(pred);
               if(pred_visited.find(bbIndex) == pred_visited.end())
               {
                  pred_visited.insert(bbIndex);
                  serialize_int("pred", bbIndex);
               }
            }
         }
         if(llvm::succ_begin(&BB) == llvm::succ_end(&BB) || isa<llvm::UnreachableInst>(BB.getTerminator()))
         {
            serialize_maybe_newline();
            field = "succ: EXIT";
            snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s ", field);
            stream << buffer;
            column += 14;
         }
         else
         {
            std::set<int> succ_visited;
            for(const auto succ : llvm::successors(&BB))
            {
               auto bbIndex = getBB_index(succ);
               if(succ_visited.find(bbIndex) == succ_visited.end())
               {
                  succ_visited.insert(bbIndex);
                  serialize_int("succ", bbIndex);
               }
            }
         }
         if(MSSA.getMemoryAccess(&BB))
         {
            /// add virtual phi
            serialize_ir_child("phi", getVirtualPhi(MSSA.getMemoryAccess(&BB), MSSA));
         }
         for(const auto& inst : BB)
         {
            if(isa<llvm::PHINode>(inst))
               serialize_ir_child("phi", assignCodeAuto(&inst));
            else
            {
               if(isa<llvm::BranchInst>(inst) && cast<llvm::BranchInst>(inst).isUnconditional() &&
                  isa<llvm::BasicBlock>(*cast<llvm::BranchInst>(inst).getOperand(0)))
                  ; /// goto to basic blocks can be skipped
               else
               {
                  serialize_ir_child("stmt", assignCodeAuto(&inst));
               }
            }
         }
      }
      /* Terminate the line.  */
      stream << "\n";
   }

   std::string DumpBambuIR::getHeaderForBuiltin(const void* t)
   {
      assert(IR_CODE(t) == IRC(FUNCTION_VAL_NODE));
      assert(is_builtin_fn(t));
      const llvm::Function* fd = reinterpret_cast<const llvm::Function*>(t);
      if(fd->hasName())
      {
         std::string declname;
         if(fd->isIntrinsic())
            declname = getIntrinsicName(fd);
         else
            declname = std::string(getName(fd));
         if(declname == "acos" or declname == "acosh" or declname == "asin" or declname == "asinh" or
            declname == "atan" or declname == "atanh" or declname == "atan2" or declname == "cbrt" or
            declname == "ceil" or declname == "copysign" or declname == "cos" or declname == "cosh" or
            declname == "erf" or declname == "erfc" or declname == "exp" or declname == "exp2" or declname == "expm1" or
            declname == "fabs" or declname == "fdim" or declname == "floor" or declname == "fma" or
            declname == "fmax" or declname == "fmin" or declname == "fmod" or declname == "frexp" or
            declname == "hypot" or declname == "ilogb" or declname == "ldexp" or declname == "lgamma" or
            declname == "llrint" or declname == "llround" or declname == "log" or declname == "log10" or
            declname == "log1p" or declname == "log2" or declname == "logb" or declname == "lrint" or
            declname == "lround" or declname == "modf" or declname == "nan" or declname == "nearbyint" or
            declname == "nextafter" or declname == "nexttoward" or declname == "pow" or declname == "remainder" or
            declname == "remquo" or declname == "rint" or declname == "round" or declname == "scalbln" or
            declname == "scalbn" or declname == "sin" or declname == "sinh" or declname == "sincos" or
            declname == "sqrt" or declname == "tan" or declname == "tanh" or declname == "tgamma" or
            declname == "trunc" or declname == "isinf" or declname == "isinf_sign" or declname == "isnan" or
            declname == "isnormal" or declname == "isfinite" or declname == "huge_val")
            return "/usr/include/math.h";
         else if(declname == "cabs" or declname == "cacos" or declname == "cacosh" or declname == "carg" or
                 declname == "casin" or declname == "casinh" or declname == "catan" or declname == "catanh" or
                 declname == "ccos" or declname == "ccosh" or declname == "cexp" or declname == "cimag" or
                 declname == "clog" or declname == "conj" or declname == "cpow" or declname == "cproj" or
                 declname == "creal" or declname == "csin" or declname == "csinh" or declname == "csqrt" or
                 declname == "ctan" or declname == "ctanh")
            return "/usr/include/complex.h";
         else if(declname == "memchr" or declname == "memcmp" or declname == "memcpy" or declname == "memmove" or
                 declname == "memset" or declname == "strcat" or declname == "strchr" or declname == "strcmp" or
                 declname == "strcpy" or declname == "strcspn" or declname == "strlen" or declname == "strncat" or
                 declname == "strncmp" or declname == "strncpy" or declname == "strpbrk" or declname == "strrchr" or
                 declname == "strspn" or declname == "strstr")
            return "/usr/include/string.h";
         else if(declname == "fprintf" or declname == "putc" or declname == "fputc" or declname == "fputs" or
                 declname == "fscanf" or declname == "fwrite" or declname == "printf" or declname == "putchar" or
                 declname == "puts" or declname == "scanf" or declname == "snprintf" or declname == "sprintf" or
                 declname == "sscanf" or declname == "vfprintf" or declname == "vfscanf" or declname == "vprintf" or
                 declname == "vscanf" or declname == "vsnprintf" or declname == "vsprintf" or declname == "vsscanf")
            return "/usr/include/stdio.h";
         else if(declname == "isalnum" or declname == "isalpha" or declname == "isblank" or declname == "iscntrl" or
                 declname == "isdigit" or declname == "isgraph" or declname == "islower" or declname == "isprint" or
                 declname == "ispunct" or declname == "isspace" or declname == "isupper" or declname == "isxdigit" or
                 declname == "tolower" or declname == "toupper")
            return "/usr/include/ctype.h";
         else if(declname == "iswalnum" or declname == "iswalpha" or declname == "iswblank" or declname == "iswcntrl" or
                 declname == "iswdigit" or declname == "iswgraph" or declname == "iswlower" or declname == "iswprint" or
                 declname == "iswpunct" or declname == "iswspace" or declname == "iswupper" or
                 declname == "iswxdigit" or declname == "towlower" or declname == "towupper")
            return "/usr/include/wctype.h";
         else if(declname == "abort" or declname == "abs" or declname == "calloc" or declname == "exit" or
                 declname == "free" or declname == "labs" or declname == "llabs" or declname == "malloc" or
                 declname == "realloc" or declname == "_exit2" or declname == "aligned_alloc")
            return "/usr/include/stdlib.h";
         else if(declname == "imaxabs")
            return "/usr/include/inttypes.h";
         else if(declname == "strftime")
            return "/usr/include/time.h";
      }
      return "";
   }

   void DumpBambuIR::dequeue_and_serialize()
   {
      assert(!Queue.empty());
      const void* t = Queue.front();
      assert(t);
      Queue.pop_front();

      if(setOfStmts.find(t) != setOfStmts.end())
      {
         dequeue_and_serialize_ir(t);
         return;
      }
      else if(setOfStatementsList.find(t) != setOfStatementsList.end())
      {
         dequeue_and_serialize_statement(t);
         return;
      }

      assert(llvm2index.find(t) != llvm2index.end());
      unsigned int index = llvm2index.find(t)->second;

      /* Print the node index.  */
      serialize_index(index);

      ir_codes code = IR_CODE(t);
      const char* code_name = GET_IR_CODE_NAME(code);
      LLVM_DEBUG(llvm::dbgs() << "|" << code_name << "\n");
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-16s ", code_name);
      stream << buffer;
      column = 25;

      auto code_class = IR_CODE_CLASS(code);

      if(IS_NODE_CODE_CLASS(code_class))
      {
         queue_and_serialize_type(t);
         if(IR_EXPR_HAS_LOCATION(t))
         {
            serialize_maybe_newline();
            snprintf(buffer, LOCAL_BUFFER_LEN, "loc_info: \"%s\":%-u:%-6u ", IR_EXPR_FILENAME(t), IR_EXPR_LINENO(t),
                     IR_EXPR_COLUMNNO(t));
            column += 12 + strlen(IR_EXPR_FILENAME(t)) + 8;
         }
         switch(code_class)
         {
            case ir_unary:
            {
               serialize_child("op", IR_OPERAND(t, 0));
               break;
            }

            case ir_binary:
            {
               serialize_child("op", IR_OPERAND(t, 0));
               serialize_child("op", IR_OPERAND(t, 1));
               break;
            }

            case ir_ternary:
            {
               serialize_child("op", IR_OPERAND(t, 0));
               serialize_child("op", IR_OPERAND(t, 1));
               serialize_child("op", IR_OPERAND(t, 2));
               break;
            }

            case ir_reference:
            case ir_other_expression:
            {
               /* These nodes are handled explicitly below.  */
               break;
            }

            default:
               report_fatal_error("Unexpected case");
         }
      }
      else if(IR_DECL_P(t))
      {
         expanded_location xloc;
         if(IR_DECL_NAME(t))
            serialize_child("name", IR_DECL_NAME(t));
         if(IR_DECL_ASSEMBLER_NAME_SET_P(t) && IR_DECL_ASSEMBLER_NAME(t) != IR_DECL_NAME(t))
            serialize_child("mngl", IR_DECL_ASSEMBLER_NAME(t));

         /* And types.  */
         queue_and_serialize_type(t);
         serialize_child("parent", getParentDecl(t));

         if(!IR_DECL_SOURCE_LOCATION(t))
         {
            serialize_maybe_newline();
            /// with clang/llvm there is no type definition
            snprintf(buffer, LOCAL_BUFFER_LEN, "loc_info: \"");
            stream << buffer;
            if(code == IRC(FUNCTION_VAL_NODE) && is_builtin_fn(t) &&
               reinterpret_cast<const llvm::Function*>(t)->empty())
            {
               auto headerFile = getHeaderForBuiltin(t);
               if(headerFile != "")
                  stream << headerFile;
               else
                  stream << "<built-in>";
            }
            else
               stream << InFile;
            snprintf(buffer, LOCAL_BUFFER_LEN, "\":0:0 ");
            stream << buffer;
            column += 12 + InFile.size() + 8;
         }
         else
         {
            /* And a source position.  */
            xloc = expand_location(IR_DECL_SOURCE_LOCATION(t));
            if(xloc.file)
            {
               serialize_maybe_newline();
               snprintf(buffer, LOCAL_BUFFER_LEN, "loc_info: \"%s\":%-d:%-6d ", xloc.file, xloc.line, xloc.column);
               stream << buffer;
               column += 12 + strlen(xloc.file) + 8;
            }
         }
      }
      else if(code_class == ir_type)
      {
         serialize_int("bitsizealloc", IR_TYPE_BITSIZEALLOC(t));

         /* All types have alignments.  */
         serialize_int("algn", IR_TYPE_ALIGN(t));
      }
      else if(code_class == ir_constant)
         queue_and_serialize_type(t);

      switch(code)
      {
         case IRC(IDENTIFIER_NODE):
            serialize_string_field("strg", IDENTIFIER_POINTER(t));
            break;
         case IRC(SIGNEDPOINTERTYPE):
         case IRC(INTEGER_TY_NODE):
            serialize_int("bitsize", IR_TYPE_BITSIZE(t));
            if(IR_TYPE_UNSIGNED(t))
               serialize_string("unsigned");
            break;
         case IRC(REAL_TY_NODE):
            serialize_int("bitsize", IR_TYPE_BITSIZE(t));
            break;
         case IRC(POINTER_TY_NODE):
            serialize_child("ptd", IR_TYPE_NODE(t));
            break;

         case IRC(FUNCTION_TY_NODE):
         {
            serialize_child("retn", IR_TYPE_NODE(t));
            auto args = IR_TYPE_ARG_NODES(t);
            for(auto arg : args)
            {
               serialize_child("arg", arg);
            }
            if(!args.empty() && stdarg_p(t)) // ISO C requires a named parameter before '...'
               serialize_string("varargs");
            break;
         }
         case IRC(ARRAY_TY_NODE):
         {
            serialize_child("elts", IR_TYPE_NODE(t));
            auto nel = llvm::APInt(64, NELEMENTS(t));
            SmallString<40> S;
            nel.toStringUnsigned(S);
            serialize_int_cst("nelements", S.c_str());
            break;
         }
         case IRC(VECTOR_TY_NODE):
            serialize_child("elts", IR_TYPE_NODE(t));
            break;

         case IRC(RECORD_TY_NODE):
         {
            serialize_child("name", IR_TYPE_NAME(t));
            if(IR_TYPE_PACKED(t))
            {
               serialize_string("packed");
            }
            if(!IR_TYPE_FIELDS(t).empty())
            {
               unsigned int counter = 0;
               for(auto op : IR_TYPE_FIELDS(t))
               {
                  assignCodeType(op);
                  serialize_child("flds", GET_FIELD_VAL_NODE(op, counter, t));
                  ++counter;
               }
            }
            break;
         }
         case IRC(SSA_NODE):
         {
            queue_and_serialize_type(t);
            serialize_child("var", IR_SSA_NAME_VAR(t));
            auto vers = IR_SSA_NAME_VERSION(t);
            //            llvm::errs() << "vers: " << vers << "\n";
            serialize_int("vers", vers);
            if(IR_POINTER_TYPE_P(IR_TYPE_NODE(t)) && IR_SSA_NAME_PTR_INFO(t))
               dump_pt_solution(&(IR_SSA_NAME_PTR_INFO(t)->pt), "use", "use_vars");
            serialize_ir_child("def_stmt", IR_SSA_NAME_DEF_STMT(t));
            if(is_virtual_ssa(t))
               serialize_string("virtual");
            if(IR_SSA_NAME_IS_DEFAULT_DEF(t))
               serialize_string("default");
            serialize_child("min", getMinValue(t));
            serialize_child("max", getMaxValue(t));

            break;
         }
         case IRC(ALLOCAVARIABLE_VAL_NODE):
         case IRC(ORIGVARIABLE_VAL_NODE):
         case IRC(VARIABLE_VAL_NODE):
         case IRC(ARGUMENT_VAL_NODE):
         case IRC(FIELD_VAL_NODE):
         {
            if(code == IRC(FIELD_VAL_NODE) && IR_DECL_C_BIT_FIELD(t))
               serialize_string("bitfield");
            if(code == IRC(VARIABLE_VAL_NODE) || code == IRC(ORIGVARIABLE_VAL_NODE))
            {
               if(IR_DECL_EXTERNAL(t))
                  serialize_string("extern");
               else if(!IR_PUBLIC(t) && IR_STATIC(t))
                  serialize_string("static");
            }
            if(IR_DECL_INITIAL(t))
               serialize_child("init", IR_DECL_INITIAL(t));
            serialize_int("bitsizealloc", IR_DECL_BITSIZEALLOC(t));
            serialize_int("algn", IR_DECL_ALIGN(t));
            if(code == IRC(FIELD_VAL_NODE) && IR_DECL_PACKED(t))
            {
               serialize_string("packed");
            }

            if(code == IRC(FIELD_VAL_NODE))
            {
               serialize_int_cst("offset", FIELD_VAL_NODE_OFFSET(t));
            }
            if(IR_READONLY(t))
               serialize_string("readonly");
            if(code == IRC(ALLOCAVARIABLE_VAL_NODE) && PtoSets_AA)
            {
               if(!IR_ADDRESSABLE(t))
                  serialize_string("addr_not_taken");
            }
            break;
         }
         case IRC(FUNCTION_VAL_NODE):
         {
            for(const auto arg : IR_DECL_ARGUMENTS(t))
            {
               serialize_child("arg", arg);
            }
            if(is_builtin_fn(t))
               serialize_string("builtin");
            if(!IR_PUBLIC(t))
               serialize_string("static");
            if(!IR_DECL_EXTERNAL(t))
               serialize_statement_child("body", getStatement_list(t));

            break;
         }
         case IRC(CONSTANT_INT_VAL_NODE_SIGNED):
         case IRC(CONSTANT_INT_VAL_NODE):
         {
            serialize_int_cst("value", IR_INT_CST(t));
            break;
         }

         case IRC(CONSTANT_FP_VAL_NODE):
         {
            serialize_real(t);
            break;
         }

         case IRC(CONSTANT_VECTOR_VAL_NODE):
         {
            auto val = reinterpret_cast<const llvm::ConstantVector*>(t);
            for(unsigned indexCV = 0; indexCV < val->getNumOperands(); ++indexCV)
            {
               auto op = val->getOperand(indexCV);
               const void* valu = getOperand(op, nullptr);
               serialize_child("valu", valu);
            }
            break;
         }

         case IRC(MEM_ACCESS_NODE):
         {
            serialize_child("op", IR_OPERAND(t, 0));
            break;
         }
         case IRC(UNALIGNED_MEM_ACCESS_NODE):
         {
            serialize_child("op", IR_OPERAND(t, 0));
            break;
         }
         case IRC(CALL_NODE):
         {
            serialize_child("fn", call_node_fn(t));
            unsigned int arg_index;
            for(arg_index = 0; arg_index < call_node_num_args(t); arg_index++)
            {
               serialize_child("arg", call_node_arg(t, arg_index));
            }
            break;
         }
         case IRC(CONSTRUCTOR_NODE):
         {
            queue_and_serialize_type(t);
            for(const auto& el : CONSTRUCTOR_ELTS(t))
            {
               serialize_child("idx", el.first);
               serialize_child("valu", el.second);
            }
            break;
         }
         case IRC(INSERTVALUE):
         {
            serialize_child("op", IR_OPERAND(t, 0));
            serialize_child("op", IR_OPERAND(t, 1));
            serialize_child("op", IR_OPERAND(t, 2));
            break;
         }
         case IRC(EXTRACTVALUE):
         {
            serialize_child("op", IR_OPERAND(t, 0));
            serialize_child("op", IR_OPERAND(t, 1));
            break;
         }
         default:
            /* There are no additional fields to print.  */
            break;
      }
      stream << "\n";
   }

   void DumpBambuIR::SerializeFunctionHeader(const void* obj)
   {
      assert(IR_CODE(obj) == IRC(FUNCTION_VAL_NODE));
      const llvm::Function* fd = reinterpret_cast<const llvm::Function*>(obj);
      stream << "\n;; Function " << getName(fd) << "(" << getName(fd) << ")\n\n";
      stream << ";; " << getName(fd) << "(";
      stream << ")\n";
   }

   unsigned int DumpBambuIR::queue(const void* obj)
   {
      unsigned int index = ++last_used_index;
      assert(llvm2index.find(obj) == llvm2index.end());
      llvm2index[obj] = index;
      assert(HAS_CODE(obj));
      Queue.push_back(obj);
      return index;
   }

   void DumpBambuIR::SerializeGlobalIRNode(const void* obj)
   {
      if(IR_CODE(obj) == IRC(FUNCTION_VAL_NODE))
      {
         SerializeFunctionHeader(obj);
      }
      if(llvm2index.find(obj) == llvm2index.end())
      {
         queue(obj);
      }
      while(!Queue.empty())
      {
         dequeue_and_serialize();
      }
   }

   static bool skipIntrinsic(llvm::Intrinsic::ID id)
   {
      switch(id)
      {
         case llvm::Intrinsic::lifetime_start:
         case llvm::Intrinsic::lifetime_end:
         case llvm::Intrinsic::copysign:
         case llvm::Intrinsic::fabs:
         case llvm::Intrinsic::fmuladd:
         case llvm::Intrinsic::maxnum:
#if PANDA_LLVM_CLANG_MAJOR > 13
         case llvm::Intrinsic::is_fpclass:
#endif
         case llvm::Intrinsic::memcpy:
         case llvm::Intrinsic::memmove:
         case llvm::Intrinsic::memset:
         case llvm::Intrinsic::minnum:
         case llvm::Intrinsic::rint:
         case llvm::Intrinsic::sqrt:
         case llvm::Intrinsic::stackrestore:
         case llvm::Intrinsic::stacksave:
         case llvm::Intrinsic::trap:
#if PANDA_LLVM_CLANG_MAJOR > 7
         case llvm::Intrinsic::sadd_sat:
         case llvm::Intrinsic::ssub_sat:
         case llvm::Intrinsic::uadd_sat:
         case llvm::Intrinsic::usub_sat:
#endif
#if PANDA_LLVM_CLANG_MAJOR > 11
         case llvm::Intrinsic::abs:
         case llvm::Intrinsic::fshl:
         case llvm::Intrinsic::fshr:
         case llvm::Intrinsic::smax:
         case llvm::Intrinsic::smin:
         case llvm::Intrinsic::umax:
         case llvm::Intrinsic::umin:
#endif
#if PANDA_LLVM_CLANG_MAJOR > 12
         case llvm::Intrinsic::bitreverse:
#endif
#ifdef VVD
         case llvm::Intrinsic::directive_scope_entry:
         case llvm::Intrinsic::directive_scope_exit:
#endif
            return true;

         default:
            return false;
      }
   }
   static bool noLoweringIntrinsic(llvm::Intrinsic::ID id)
   {
      switch(id)
      {
         case llvm::Intrinsic::dbg_value:
         case llvm::Intrinsic::stackrestore:
         case llvm::Intrinsic::stacksave:
         case llvm::Intrinsic::vacopy:
         case llvm::Intrinsic::vaend:
         case llvm::Intrinsic::vastart:
            return true;
         default:
            return false;
      }
   }

   void DumpBambuIR::buildMetaDataMap(const llvm::Module& M)
   {
      for(auto& fun : M.getFunctionList())
      {
         if(!fun.isIntrinsic() && !fun.isDeclaration())
         {
            for(const auto& BB : fun)
            {
               for(const auto& inst : BB)
               {
                  if(const llvm::DbgValueInst* dbgInstrCall = dyn_cast<llvm::DbgValueInst>(&inst))
                  {
                     auto val = dbgInstrCall->getValue();
                     if(val && !isa<llvm::Constant>(val) && MetaDataMap.find(val) == MetaDataMap.end())
                     {
                        MetaDataMap[val] = dbgInstrCall->getRawVariable();
                        //                        auto DIExpr = dbgInstrCall->getExpression();
                        //                        if(DIExpr)
                        // {
                        //                           llvm::errs() << "Inst: ";
                        //                           inst.print(llvm::errs());
                        //                           llvm::errs() << "\n";
                        //                           llvm::errs() << "Value: ";
                        //                           val->print(llvm::errs());
                        //                           llvm::errs() << "\n";
                        //                           llvm::errs() << "Metadata: ";
                        //                           dbgInstrCall->getRawVariable()->print(llvm::errs());
                        //                           llvm::errs() <<"\n";
                        // }
                     }
                  }
               }
            }
         }
      }
   }

   static bool isSimpleEnoughPointerToCommitLocal(llvm::Constant* C, llvm::Type* cType, const llvm::DataLayout& DL)
   {
      // Conservatively, avoid aggregate types. This is because we don't
      // want to worry about them partially overlapping other stores.
#if PANDA_LLVM_CLANG_MAJOR < 16
      if(!cast<llvm::PointerType>(C->getType())->getElementType()->isSingleValueType())
#else
      if(C->getType()->isOpaquePointerTy() || !C->getType()->getNonOpaquePointerElementType()->isSingleValueType())
#endif
      {
         return false;
      }

      if(llvm::GlobalVariable* GV = dyn_cast<llvm::GlobalVariable>(C))
         // Do not allow weak/*_odr/linkonce linkage or external globals.
         return GV->hasUniqueInitializer();

      if(llvm::ConstantExpr* CE = dyn_cast<llvm::ConstantExpr>(C))
      {
         // Handle a constantexpr gep.
         if(CE->getOpcode() == llvm::Instruction::GetElementPtr && isa<llvm::GlobalVariable>(CE->getOperand(0)) &&
            cast<llvm::GEPOperator>(CE)->isInBounds())
         {
            llvm::GlobalVariable* GV = cast<llvm::GlobalVariable>(CE->getOperand(0));
            // Do not allow weak/*_odr/linkonce/dllimport/dllexport linkage or
            // external globals.
            if(!GV->hasUniqueInitializer())
               return false;

            // The first index must be zero.
            llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(*std::next(CE->op_begin()));
            if(!CI || !CI->isZero())
               return false;

               // The remaining indices must be compile-time known integers within the
               // notional bounds of the corresponding static array types.
#if PANDA_LLVM_CLANG_MAJOR > 15
            Constant* StrippedC = cast<Constant>(CE->stripInBoundsConstantOffsets());
            if(StrippedC == C)
#else
            if(!CE->isGEPWithNoNotionalOverIndexing())
#endif
               return false;

#if PANDA_LLVM_CLANG_MAJOR > 18
            return ConstantFoldLoadFromUniformValue(GV->getInitializer(), cType, DL);
#elif PANDA_LLVM_CLANG_MAJOR > 15
            return ConstantFoldLoadFromUniformValue(GV->getInitializer(), cType);
#elif PANDA_LLVM_CLANG_MAJOR >= 13
            return ConstantFoldLoadThroughGEPConstantExpr(GV->getInitializer(), CE, cType, DL);
#else
            return ConstantFoldLoadThroughGEPConstantExpr(GV->getInitializer(), CE);
#endif

            // A constantexpr bitcast from a pointer to another pointer is a no-op,
            // and we know how to evaluate it by moving the bitcast from the pointer
            // operand to the value operand.
         }
         else if(CE->getOpcode() == llvm::Instruction::BitCast && isa<llvm::GlobalVariable>(CE->getOperand(0)))
         {
            // Do not allow weak/*_odr/linkonce/dllimport/dllexport linkage or
            // external globals.
            return cast<llvm::GlobalVariable>(CE->getOperand(0))->hasUniqueInitializer();
         }
      }

      return false;
   }

   /// This is a customized/local version of EvaluateStoreIntoLocal taken from lib/Analysis/ConstantFolding.cpp
   static llvm::Constant* ConstantFoldLoadThroughBitcastLocal(llvm::Constant* C, llvm::Type* DestTy,
                                                              const llvm::DataLayout& DL)
   {
      do
      {
         llvm::Type* SrcTy = C->getType();

         // If the type sizes are the same and a cast is legal, just directly
         // cast the constant.
         if(DL.getTypeSizeInBits(DestTy) == DL.getTypeSizeInBits(SrcTy))
         {
            llvm::Instruction::CastOps Cast = llvm::Instruction::BitCast;
            // If we are going from a pointer to int or vice versa, we spell the cast
            // differently.
            if(SrcTy->isIntegerTy() && DestTy->isPointerTy())
               Cast = llvm::Instruction::IntToPtr;
            else if(SrcTy->isPointerTy() && DestTy->isIntegerTy())
               Cast = llvm::Instruction::PtrToInt;

            if(llvm::CastInst::castIsValid(Cast, C, DestTy))
               return llvm::ConstantExpr::getCast(Cast, C, DestTy);
         }

         // If this isn't an aggregate type, there is nothing we can do to drill down
         // and find a bitcastable constant.
         if(!SrcTy->isAggregateType())
            return nullptr;

         // We're simulating a load through a pointer that was bitcast to point to
         // a different type, so we can try to walk down through the initial
         // elements of an aggregate to see if some part of th e aggregate is
         // castable to implement the "load" semantic model.
         C = C->getAggregateElement(0u);
      } while(C);

      return nullptr;
   }

   /// This is a customized/local version of EvaluateStoreIntoLocal taken from lib/Transforms/IPO/GlobalOpt.cpp
   static llvm::Constant* EvaluateStoreIntoLocal(llvm::Constant* Init, llvm::Constant* Val, llvm::ConstantExpr* Addr,
                                                 unsigned OpNo)
   {
      // Base case of the recursion.
      if(OpNo == Addr->getNumOperands())
      {
         assert(Val->getType() == Init->getType() && "Type mismatch!");
         return Val;
      }

      llvm::SmallVector<llvm::Constant*, 32> Elts;
      if(llvm::StructType* STy = dyn_cast<llvm::StructType>(Init->getType()))
      {
         // Break up the constant into its elements.
         for(unsigned i = 0, e = STy->getNumElements(); i != e; ++i)
            Elts.push_back(Init->getAggregateElement(i));

         // Replace the element that we are supposed to.
         llvm::ConstantInt* CU = cast<llvm::ConstantInt>(Addr->getOperand(OpNo));
         unsigned Idx = CU->getZExtValue();
         assert(Idx < STy->getNumElements() && "Struct index out of range!");
         Elts[Idx] = EvaluateStoreIntoLocal(Elts[Idx], Val, Addr, OpNo + 1);

         // Return the modified struct.
         return llvm::ConstantStruct::get(STy, Elts);
      }

      llvm::ConstantInt* CI = cast<llvm::ConstantInt>(Addr->getOperand(OpNo));
      auto initType = Init->getType();
      uint64_t NumElts = 0;
      if(dyn_cast<llvm::ArrayType>(initType))
      {
         NumElts = dyn_cast<llvm::ArrayType>(initType)->getNumElements();
      }
      else if(dyn_cast<llvm::VectorType>(initType))
      {
#if PANDA_LLVM_CLANG_MAJOR >= 12
         NumElts = dyn_cast<llvm::VectorType>(initType)->getElementCount().getFixedValue();
#else
         NumElts = dyn_cast<llvm::VectorType>(initType)->getNumElements();
#endif
      }
      else
      {
         report_fatal_error("unexpected case");
      }

      // Break up the array into elements.
      for(uint64_t i = 0, e = NumElts; i != e; ++i)
         Elts.push_back(Init->getAggregateElement(i));

      assert(CI->getZExtValue() < NumElts);
      Elts[CI->getZExtValue()] = EvaluateStoreIntoLocal(Elts[CI->getZExtValue()], Val, Addr, OpNo + 1);

      if(Init->getType()->isArrayTy())
         return llvm::ConstantArray::get(cast<llvm::ArrayType>(initType), Elts);
      return llvm::ConstantVector::get(Elts);
   }

   /// This is a customized/local version of CommitValueTo taken from lib/Transforms/IPO/GlobalOpt.cpp
   static void CommitValueToLocal(llvm::Constant* Val, llvm::Constant* Addr)
   {
      if(llvm::GlobalVariable* GV = dyn_cast<llvm::GlobalVariable>(Addr))
      {
         assert(GV->hasInitializer());
         GV->setInitializer(Val);
         return;
      }

      llvm::ConstantExpr* CE = cast<llvm::ConstantExpr>(Addr);
      llvm::GlobalVariable* GV = cast<llvm::GlobalVariable>(CE->getOperand(0));
      GV->setInitializer(EvaluateStoreIntoLocal(GV->getInitializer(), Val, CE, 2));
   }

   /// This is a customized/local version of BatchCommitValueTo taken from lib/Transforms/IPO/GlobalOpt.cpp
   static void BatchCommitValueToLocal(const llvm::DenseMap<llvm::Constant*, llvm::Constant*>& Mem)
   {
      llvm::SmallVector<std::pair<llvm::GlobalVariable*, llvm::Constant*>, 32> GVs;
      llvm::SmallVector<std::pair<llvm::ConstantExpr*, llvm::Constant*>, 32> ComplexCEs;
      llvm::SmallVector<std::pair<llvm::ConstantExpr*, llvm::Constant*>, 32> SimpleCEs;
      SimpleCEs.reserve(Mem.size());

      for(const auto& I : Mem)
      {
         if(auto* GV = dyn_cast<llvm::GlobalVariable>(I.first))
         {
            GVs.push_back(std::make_pair(GV, I.second));
         }
         else
         {
            llvm::ConstantExpr* GEP = cast<llvm::ConstantExpr>(I.first);
            // We don't handle the deeply recursive case using the batch method.
            if(GEP->getNumOperands() > 3)
               ComplexCEs.push_back(std::make_pair(GEP, I.second));
            else
               SimpleCEs.push_back(std::make_pair(GEP, I.second));
         }
      }

      // The algorithm below doesn't handle cases like nested structs, so use the
      // slower fully general method if we have to.
      for(auto ComplexCE : ComplexCEs)
         CommitValueToLocal(ComplexCE.second, ComplexCE.first);

      for(auto GVPair : GVs)
      {
         assert(GVPair.first->hasInitializer());
         GVPair.first->setInitializer(GVPair.second);
      }

      if(SimpleCEs.empty())
         return;

      // We cache a single global's initializer elements in the case where the
      // subsequent address/val pair uses the same one. This avoids throwing away and
      // rebuilding the constant struct/vector/array just because one element is
      // modified at a time.
      llvm::SmallVector<llvm::Constant*, 32> Elts;
      Elts.reserve(SimpleCEs.size());
      llvm::GlobalVariable* CurrentGV = nullptr;

      auto commitAndSetupCache = [&](llvm::GlobalVariable* GV, bool Update) {
         llvm::Constant* Init = GV->getInitializer();
         llvm::Type* Ty = Init->getType();
         if(Update)
         {
            if(CurrentGV)
            {
               assert(CurrentGV && "Expected a GV to commit to!");
               llvm::Type* CurrentInitTy = CurrentGV->getInitializer()->getType();
               // We have a valid cache that needs to be committed.
               if(llvm::StructType* STy = dyn_cast<llvm::StructType>(CurrentInitTy))
                  CurrentGV->setInitializer(llvm::ConstantStruct::get(STy, Elts));
               else if(llvm::ArrayType* ArrTy = dyn_cast<llvm::ArrayType>(CurrentInitTy))
                  CurrentGV->setInitializer(llvm::ConstantArray::get(ArrTy, Elts));
               else
                  CurrentGV->setInitializer(llvm::ConstantVector::get(Elts));
            }
            if(CurrentGV == GV)
               return;
            // Need to clear and set up cache for new initializer.
            CurrentGV = GV;
            Elts.clear();
            unsigned NumElts = 0;
            if(auto* STy = dyn_cast<llvm::StructType>(Ty))
            {
               NumElts = STy->getNumElements();
            }
            else if(auto* ATy = dyn_cast<llvm::ArrayType>(Ty))
            {
               NumElts = ATy->getNumElements();
            }
            else if(auto* VTy = dyn_cast<llvm::VectorType>(Ty))
            {
#if PANDA_LLVM_CLANG_MAJOR >= 12
               NumElts = VTy->getElementCount().getFixedValue();
#else
               NumElts = VTy->getNumElements();
#endif
            }
            else
            {
               report_fatal_error("unexpected case");
            }
            for(unsigned i = 0, e = NumElts; i != e; ++i)
            {
               Elts.push_back(Init->getAggregateElement(i));
            }
         }
      };

      for(auto CEPair : SimpleCEs)
      {
         llvm::ConstantExpr* GEP = CEPair.first;
         llvm::Constant* Val = CEPair.second;

         llvm::GlobalVariable* GV = cast<llvm::GlobalVariable>(GEP->getOperand(0));
         commitAndSetupCache(GV, GV != CurrentGV);
         llvm::ConstantInt* CI = cast<llvm::ConstantInt>(GEP->getOperand(2));
         Elts[CI->getZExtValue()] = Val;
      }
      // The last initializer in the list needs to be committed, others
      // will be committed on a new initializer being processed.
      commitAndSetupCache(CurrentGV, true);
   }

   static bool removableStore(llvm::StoreInst* SI, llvm::GlobalVariable* GV, llvm::TargetLibraryInfo& TLI,
                              const llvm::DataLayout& DL, llvm::Constant*& Ptr, llvm::Constant*& Val, bool firstPart)
   {
      if(!SI->isSimple())
      {
         LLVM_DEBUG(llvm::dbgs() << "Store is not simple! Can not evaluate.\n");
         return false; // no volatile/atomic accesses.
      }

      if(!dyn_cast<llvm::Constant>(SI->getOperand(1)))
      {
         LLVM_DEBUG(llvm::dbgs() << "Ptr is not constant.\n");
         return false;
      }
      Ptr = dyn_cast<llvm::Constant>(SI->getOperand(1));
      if(auto* FoldedPtr = llvm::ConstantFoldConstant(Ptr, DL, &TLI))
      {
         LLVM_DEBUG(llvm::dbgs() << "Folding constant ptr expression: " << *Ptr);
         Ptr = FoldedPtr;
         LLVM_DEBUG(llvm::dbgs() << "; To: " << *Ptr << "\n");
      }
      if(!isSimpleEnoughPointerToCommitLocal(Ptr, SI->getValueOperand()->getType(), DL))
      {
         // If this is too complex for us to commit, reject it.
         LLVM_DEBUG(llvm::dbgs() << "Pointer is too complex for us to evaluate store.");
         return false;
      }
      if(!dyn_cast<llvm::Constant>(SI->getOperand(0)))
      {
         LLVM_DEBUG(llvm::dbgs() << "Value stored is not constant.\n");
         return false;
      }
      Val = dyn_cast<llvm::Constant>(SI->getOperand(0));
      if(llvm::ConstantExpr* CE = dyn_cast<llvm::ConstantExpr>(Ptr))
      {
         if(CE->getOpcode() == llvm::Instruction::BitCast)
         {
            LLVM_DEBUG(llvm::dbgs() << "Attempting to resolve bitcast on constant ptr.\n");
            // If we're evaluating a store through a bitcast, then we need
            // to pull the bitcast off the pointer type and push it onto the
            // stored value.
            Ptr = CE->getOperand(0);

#if PANDA_LLVM_CLANG_MAJOR < 16
            llvm::Type* NewTy = cast<llvm::PointerType>(Ptr->getType())->getElementType();
#else
            if(Ptr->getType()->isOpaquePointerTy())
            {
               LLVM_DEBUG(llvm::dbgs() << "Opaque BitCast.\n");
               return false;
            }
            llvm::Type* NewTy = Ptr->getType()->getNonOpaquePointerElementType();
#endif

            // In order to push the bitcast onto the stored value, a bitcast
            // from NewTy to Val's type must be legal.  If it's not, we can try
            // introspecting NewTy to find a legal conversion.
            llvm::Constant* NewVal;
            while(!(NewVal = ConstantFoldLoadThroughBitcastLocal(Val, NewTy, DL)))
            {
               // If NewTy is a struct, we can convert the pointer to the struct
               // into a pointer to its first member.
               // FIXME: This could be extended to support arrays as well.
               if(llvm::StructType* STy = dyn_cast<llvm::StructType>(NewTy))
               {
                  NewTy = STy->getTypeAtIndex(0U);

                  llvm::IntegerType* IdxTy = llvm::IntegerType::get(NewTy->getContext(), 32);
                  llvm::Constant* IdxZero = llvm::ConstantInt::get(IdxTy, 0, false);
                  llvm::Constant* const IdxList[] = {IdxZero, IdxZero};

                  Ptr = llvm::ConstantExpr::getGetElementPtr(nullptr, Ptr, IdxList);
                  if(auto* FoldedPtr = llvm::ConstantFoldConstant(Ptr, DL, &TLI))
                     Ptr = FoldedPtr;

                  // If we can't improve the situation by introspecting NewTy,
                  // we have to give up.
               }
               else
               {
                  LLVM_DEBUG(llvm::dbgs() << "Failed to bitcast constant ptr, can not evaluate.\n");
                  return false;
               }
            }
            Val = NewVal;
            LLVM_DEBUG(llvm::dbgs() << "Evaluated bitcast: " << *Val << "\n");
         }
      }
      if(firstPart)
         return true;
      if(llvm::GlobalVariable* GVi = dyn_cast<llvm::GlobalVariable>(Ptr))
      {
         if(GV == GVi)
            return true;
      }
      else
      {
         llvm::ConstantExpr* CEj = cast<llvm::ConstantExpr>(Ptr);
         llvm::GlobalVariable* GVj = cast<llvm::GlobalVariable>(CEj->getOperand(0));
         if(GV == GVj)
            return true;
      }
      return false;
   }

   static llvm::Constant* getInitializerLocal(llvm::Constant* C)
   {
      auto* GV = dyn_cast<llvm::GlobalVariable>(C);
      return GV && GV->hasDefinitiveInitializer() ? GV->getInitializer() : nullptr;
   }

   llvm::Constant* ComputeLoadResultLocal(llvm::Constant* P, Type* pType,
                                          llvm::DenseMap<llvm::Constant*, llvm::Constant*>& MutatedMemory,
                                          const llvm::DataLayout& DL)
   {
      // If this memory location has been recently stored, use the stored value: it
      // is the most up-to-date.
      auto I = MutatedMemory.find(P);
      if(I != MutatedMemory.end())
         return I->second;

      // Access it.
      if(llvm::GlobalVariable* GV = dyn_cast<llvm::GlobalVariable>(P))
      {
         if(GV->hasDefinitiveInitializer())
            return GV->getInitializer();
         return nullptr;
      }

      if(llvm::ConstantExpr* CE = dyn_cast<llvm::ConstantExpr>(P))
      {
         switch(CE->getOpcode())
         {
            // Handle a constantexpr getelementptr.
            case llvm::Instruction::GetElementPtr:
               if(auto* I = getInitializerLocal(CE->getOperand(0)))
               {
#if PANDA_LLVM_CLANG_MAJOR > 18
                  return ConstantFoldLoadFromUniformValue(I, pType, DL);
#elif PANDA_LLVM_CLANG_MAJOR > 15
                  return ConstantFoldLoadFromUniformValue(I, pType);
#elif PANDA_LLVM_CLANG_MAJOR >= 13
                  return llvm::ConstantFoldLoadThroughGEPConstantExpr(I, CE, pType, DL);
#else
                  return llvm::ConstantFoldLoadThroughGEPConstantExpr(I, CE);
#endif
               }
               break;
               // Handle a constantexpr bitcast.
            case llvm::Instruction::BitCast:
               llvm::Constant* Val = dyn_cast<llvm::Constant>(CE->getOperand(0));
               auto MM = MutatedMemory.find(Val);
               auto* I = (MM != MutatedMemory.end()) ? MM->second : getInitializerLocal(CE->getOperand(0));
               if(I)
                  return ConstantFoldLoadThroughBitcastLocal(I, pType, DL);
               break;
         }
      }

      return nullptr; // don't know how to evaluate.
   }

   static void updateLoads(llvm::GlobalVariable* GV, llvm::DenseMap<llvm::Constant*, llvm::Constant*>& MutatedMemory,
                           llvm::TargetLibraryInfo& TLI, const llvm::DataLayout& DL,
                           std::list<llvm::Instruction*>& deadList)
   {
      for(auto user : GV->users())
      {
         for(auto u : user->users())
         {
            if(llvm::LoadInst* LI = dyn_cast<llvm::LoadInst>(u))
            {
               if(LI->isSimple())
               {
                  if(llvm::Constant* Ptr = dyn_cast<llvm::Constant>(LI->getOperand(0)))
                  {
                     if(auto* FoldedPtr = llvm::ConstantFoldConstant(Ptr, DL, &TLI))
                     {
                        Ptr = FoldedPtr;
                     }
                     auto InstResult = ComputeLoadResultLocal(Ptr, LI->getType(), MutatedMemory, DL);
                     if(InstResult)
                     {
                        LI->replaceAllUsesWith(InstResult);
                        if(llvm::isInstructionTriviallyDead(LI, &TLI))
                           deadList.push_back(LI);
                     }
                  }
               }
            }
         }
      }
   }
   bool DumpBambuIR::RebuildConstants(llvm::Module& M)
   {
      llvm::SmallPtrSet<llvm::GlobalVariable*, 8> Invariants;
      llvm::SmallPtrSet<llvm::Instruction*, 8> Stores;
      auto res = false;
      auto currFuncIterator = M.getFunctionList().begin();
      while(currFuncIterator != M.getFunctionList().end())
      {
         auto& F = *currFuncIterator;
         auto fname = std::string(getName(&F));
         llvm::TargetLibraryInfo& TLI = GetTLI(F);
         std::list<llvm::Instruction*> deadList;
         for(llvm::Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI)
         {
            for(llvm::BasicBlock::iterator II = BI->begin(), IE = BI->end(); II != IE; ++II)
            {
               if(isa<llvm::IntrinsicInst>(*II))
               {
                  llvm::IntrinsicInst* IntInst = dyn_cast<llvm::IntrinsicInst>(&(*II));
                  if(IntInst && IntInst->getIntrinsicID() == llvm::Intrinsic::invariant_start)
                  {
                     llvm::ConstantInt* Size = cast<llvm::ConstantInt>(IntInst->getArgOperand(0));
                     auto invOp = IntInst->getArgOperand(1)->stripPointerCasts();
                     if(llvm::GlobalVariable* GV = dyn_cast<llvm::GlobalVariable>(invOp))
                     {
                        llvm::Type* ElemTy = GV->getValueType();
                        if(!Size->isMinusOne() && Size->getValue().getLimitedValue() >= DL->getTypeStoreSize(ElemTy))
                        {
                           /// Check the pattern first
                           bool allRemovable = BI->begin() != II;
                           for(llvm::BasicBlock::iterator CurInst = BI->begin(); CurInst != II; ++CurInst)
                           {
                              llvm::Constant* Val;
                              llvm::Constant* Ptr;
                              llvm::StoreInst* SI = dyn_cast<llvm::StoreInst>(CurInst);
                              allRemovable = SI && removableStore(SI, GV, TLI, *DL, Ptr, Val, false);
                              if(!allRemovable)
                                 break;
                           }
                           if(allRemovable)
                           {
                              llvm::GlobalVariable* GVi = nullptr;
                              llvm::DenseMap<llvm::Constant*, llvm::Constant*> MutatedMemory;
                              LLVM_DEBUG(llvm::dbgs() << "Found a global var that is an invariant: " << *GV << "\n");
                              for(llvm::BasicBlock::iterator CurInst = BI->begin(); CurInst != II;)
                              {
                                 llvm::Constant* Val = nullptr;
                                 llvm::Constant* Ptr = nullptr;
#ifndef NDEBUG
                                 auto resRS =
#endif
                                     removableStore(dyn_cast<llvm::StoreInst>(CurInst), GV, TLI, *DL, Ptr, Val, false);
                                 assert(dyn_cast<llvm::StoreInst>(CurInst) && resRS);
                                 MutatedMemory[Ptr] = Val;

                                 auto me = CurInst;
                                 bool atBegin(BI->begin() == me);
                                 if(!atBegin)
                                    --me;
                                 CurInst->eraseFromParent();
                                 if(atBegin)
                                    CurInst = BI->begin();
                                 else
                                 {
                                    CurInst = me;
                                    ++CurInst;
                                 }
                              }
                              /// check the next statement
                              llvm::BasicBlock::iterator GuardInst = II;
                              ++GuardInst;
                              if(GuardInst != IE)
                              {
                                 llvm::Constant* Val = nullptr;
                                 llvm::Constant* Ptr = nullptr;
                                 llvm::StoreInst* SI = dyn_cast<llvm::StoreInst>(GuardInst);
                                 auto Removable = SI && removableStore(SI, GV, TLI, *DL, Ptr, Val, true);
                                 if(Removable)
                                 {
                                    GVi = dyn_cast<llvm::GlobalVariable>(Ptr);
                                    if(!GVi)
                                    {
                                       llvm::ConstantExpr* CEj = cast<llvm::ConstantExpr>(Ptr);
                                       GVi = cast<llvm::GlobalVariable>(CEj->getOperand(0));
                                    }
                                    if(GVi)
                                    {
                                       if(!GVi->getName().empty())
                                       {
                                          std::string declname = std::string(GVi->getName());
                                          int status;
                                          char* demangled_outbuffer =
                                              abi::__cxa_demangle(declname.c_str(), nullptr, nullptr, &status);
                                          if(status == 0)
                                          {
                                             declname = demangled_outbuffer;
                                             if(declname.find("guard variable for") != std::string::npos)
                                             {
                                                std::string declname1 = std::string(GV->getName());
                                                char* demangled_outbuffer1 =
                                                    abi::__cxa_demangle(declname1.c_str(), nullptr, nullptr, &status);
                                                if(status == 0)
                                                {
                                                   declname1 = demangled_outbuffer1;
                                                   if(declname.find(declname1) != std::string::npos)
                                                   {
                                                      MutatedMemory[Ptr] = Val;
                                                      GuardInst->eraseFromParent();
                                                   }
                                                   else
                                                      GVi = nullptr;
                                                   free(demangled_outbuffer1);
                                                }
                                                else
                                                {
                                                   assert(demangled_outbuffer1 == nullptr);
                                                   GVi = nullptr;
                                                }
                                             }
                                             else
                                                GVi = nullptr;
                                             free(demangled_outbuffer);
                                          }
                                          else
                                          {
                                             GVi = nullptr;
                                             assert(demangled_outbuffer == nullptr);
                                          }
                                       }
                                    }
                                 }
                              }
                              BatchCommitValueToLocal(MutatedMemory);
                              GV->setConstant(true);
                              updateLoads(GV, MutatedMemory, TLI, *DL, deadList);
                              if(GVi)
                              {
                                 GVi->setConstant(true);
                                 updateLoads(GVi, MutatedMemory, TLI, *DL, deadList);
                              }
                              res = true;
                           }
                        }
                        else
                        {
                           LLVM_DEBUG(llvm::dbgs() << "Found a global var, but can not treat it as an invariant.\n");
                        }
                     }
                  }
               }
            }
         }
         for(auto I : deadList)
            if(llvm::isInstructionTriviallyDead(I, &TLI))
               I->eraseFromParent();
         if(!deadList.empty())
         {
            const llvm::TargetTransformInfo& TTI = GetTTI(F);
            for(llvm::Function::iterator BBIt = F.begin(); BBIt != F.end();)
               llvm::SimplifyInstructionsInBlock(&*BBIt++, &TLI);
            for(llvm::Function::iterator BBIt = F.begin(); BBIt != F.end();)
#if PANDA_LLVM_CLANG_MAJOR >= 12
               llvm::simplifyCFG(&*BBIt++, TTI);
#elif PANDA_LLVM_CLANG_MAJOR >= 6
               llvm::simplifyCFG(&*BBIt++, TTI, 1);
#else
               llvm::SimplifyCFG(&*BBIt++, TTI, 1);
#endif
            llvm::removeUnreachableBlocks(F);
         }
         ++currFuncIterator;
      }
      return res;
   }

   /// Intrinsic lowering
   bool DumpBambuIR::lowerIntrinsics(llvm::Module& M)
   {
      auto res = false;
      llvm::IntrinsicLowering* IL = new llvm::IntrinsicLowering(*DL);
      auto currFuncIterator = M.getFunctionList().begin();
      while(currFuncIterator != M.getFunctionList().end())
      {
         auto& F = *currFuncIterator;
         auto fname = std::string(getName(&F));
         for(auto& BB : *currFuncIterator)
         {
            auto curInstIterator = BB.begin();
            while(curInstIterator != BB.end())
            {
               if(isa<llvm::CallInst>(*curInstIterator))
               {
                  auto& ci = cast<llvm::CallInst>(*curInstIterator);
                  llvm::Function* Callee = ci.getCalledFunction();
                  LLVM_DEBUG({
                     llvm::dbgs() << "Intrinsic call:";
                     ci.print(llvm::dbgs());
                     llvm::dbgs() << "\n";
                     if(Callee)
                     {
                        llvm::dbgs() << " callee(" << llvm::Intrinsic::getName(Callee->getIntrinsicID()) << "):\n";
                        Callee->print(llvm::dbgs());
                        llvm::dbgs() << "\n";
                     }
                  });
                  if(Callee && Callee->isIntrinsic() && !skipIntrinsic(Callee->getIntrinsicID()))
                  {
                     res = true;
                     auto me = curInstIterator;
                     bool atBegin(BB.begin() == me);
                     if(!atBegin)
                        --me;
                     if(noLoweringIntrinsic(Callee->getIntrinsicID()))
                     {
                        assert(ci.use_empty() && "Lowering should have eliminated any uses of the intrinsic call!");
                        ci.eraseFromParent();
                     }
#if PANDA_LLVM_CLANG_MAJOR >= 8 || defined(_WIN32)
                     else if(Callee->getIntrinsicID() == llvm::Intrinsic::is_constant)
                     {
                        auto C = llvm::ConstantInt::get(llvm::Type::getInt1Ty(ci.getContext()), 0, false);
                        ci.replaceAllUsesWith(C);
                        ci.eraseFromParent();
                     }
#endif
                     else
                        IL->LowerIntrinsicCall(&ci);
                     if(atBegin)
                        curInstIterator = BB.begin();
                     else
                     {
                        curInstIterator = me;
                        ++curInstIterator;
                     }
                  }
                  else
                     ++curInstIterator;
               }
               else
                  ++curInstIterator;
            }
         }
         ++currFuncIterator;
      }
      delete IL;
      return res;
   }

   void DumpBambuIR::computeMAEntryDefs(
       const llvm::Function* F,
       std::map<const llvm::Function*, std::map<const void*, std::set<const llvm::Instruction*>>>&
           CurrentListofMAEntryDef)
   {
      auto& MSSA = GetMSSA(*const_cast<llvm::Function*>(F)).getMSSA();
      for(const auto& BB : *F)
      {
         for(const auto& inst : BB)
         {
            const llvm::MemoryUseOrDef* ma = MSSA.getMemoryAccess(&inst);
            if(ma && ma->getValueID() == llvm::Value::MemoryDefVal)
            {
               auto defMA = MSSA.getWalker()->getClobberingMemoryAccess(MSSA.getMemoryAccess(&inst));
               if(MSSA.isLiveOnEntryDef(defMA))
               {
                  CurrentListofMAEntryDef[F][nullptr].insert(&inst);
               }
               else if(defMA->getValueID() == llvm::Value::MemoryPhiVal)
               {
                  CurrentListofMAEntryDef[F]
                                         [ir_phi_virtual_result(getVirtualPhi(dyn_cast<llvm::MemoryPhi>(defMA), MSSA))]
                                             .insert(&inst);
               }
               else
               {
                  assert(defMA->getValueID() == llvm::Value::MemoryDefVal);
                  CurrentListofMAEntryDef[F][dyn_cast<llvm::MemoryUseOrDef>(defMA)->getMemoryInst()].insert(&inst);
               }
            }
         }
      }
   }

   bool DumpBambuIR::exec(llvm::Module& M, const std::vector<std::string>& _TopFunctionNames,
                          llvm::function_ref<llvm::TargetLibraryInfo&(llvm::Function&)> _GetTLI,
                          llvm::function_ref<llvm::TargetTransformInfo&(llvm::Function&)> _GetTTI,
                          llvm::function_ref<llvm::DominatorTree&(llvm::Function&)> _GetDomTree,
                          llvm::function_ref<llvm::LoopInfo&(llvm::Function&)> _GetLI,
                          llvm::function_ref<MemorySSAAnalysisResult&(llvm::Function&)> _GetMSSA,
                          llvm::function_ref<llvm::LazyValueInfo&(llvm::Function&)> _GetLVI,
                          llvm::function_ref<llvm::AssumptionCache&(llvm::Function&)> _GetAC,
#if PANDA_LLVM_CLANG_MAJOR > 5
                          llvm::function_ref<llvm::OptimizationRemarkEmitter&(llvm::Function&)> GetORE,
#endif
                          const std::string& costTable)
   {
      DL = &M.getDataLayout();
      GetTLI = _GetTLI;
      GetTTI = _GetTTI;
      GetDomTree = _GetDomTree;
      GetLI = _GetLI;
      GetMSSA = _GetMSSA;
      GetLVI = _GetLVI;
      GetAC = _GetAC;
      moduleContext = &M.getContext();
      TopFunctionNames = _TopFunctionNames;
      bool res = false;
#if PANDA_LLVM_CLANG_MAJOR > 5
      if(!costTable.empty())
      {
         TreeHeightReduction THR;
         res |= THR.runOnModule(M, GetLI, GetORE, costTable);
      }
#endif

      if(!earlyAnalysis)
      {
         LLVM_DEBUG(llvm::dbgs() << "Building metadata\n");
         buildMetaDataMap(M);
         LLVM_DEBUG(llvm::dbgs() << "Metadata built\n");

         LLVM_DEBUG(llvm::dbgs() << "Rebuilding Constants\n");
         res |= RebuildConstants(M);

         LLVM_DEBUG(llvm::dbgs() << "Lowering Intrinsics\n");
         res |= lowerIntrinsics(M);
         LLVM_DEBUG(llvm::dbgs() << "done\n");
         if(!onlyGlobals)
         {
            if(TopFunctionNames.size())
            {
               const auto TopFunctionName = TopFunctionNames.front();
               LLVM_DEBUG(llvm::dbgs() << "Performing alias analysis on first top function: " << TopFunctionName
                                       << "\n");
#if ANDERSEN_AA
               PtoSets_AA = new Andersen_AA(TopFunctionName);
#else
               PtoSets_AA = new Staged_Flow_Sensitive_AA(TopFunctionName);
#endif
               PtoSets_AA->computePointToSet(M);
               LLVM_DEBUG(llvm::dbgs() << "Performed alias analysis\n");
            }
         }
         LLVM_DEBUG(llvm::dbgs() << "done\n");
      }

      if(!earlyAnalysis)
      {
         if(!onlyGlobals)
         {
            for(const auto& fun : M.getFunctionList())
            {
               if(!fun.isDeclaration() && !fun.isIntrinsic())
               {
                  computeMAEntryDefs(&fun, CurrentListofMAEntryDef);
               }
            }
         }

         for(const auto& globalVar : M.globals())
         {
            LLVM_DEBUG(llvm::dbgs() << "Found global name: " << globalVar.getName() << "|"
                                    << ValueTyNames[globalVar.getValueID()] << "\n");
            SerializeGlobalIRNode(assignCodeAuto(&globalVar));
         }
         if(!onlyGlobals)
         {
            for(const auto& fun : M.getFunctionList())
            {
               if(fun.isIntrinsic())
               {
                  LLVM_DEBUG(llvm::dbgs() << "Function intrinsic skipped: " << getName(&fun) << "|"
                                          << ValueTyNames[fun.getValueID()] << "\n");
               }
               else
               {
                  LLVM_DEBUG(llvm::dbgs()
                             << "Found function: " << getName(&fun) << "|" << ValueTyNames[fun.getValueID()] << "\n");
                  SerializeGlobalIRNode(assignCodeAuto(&fun));
               }
            }
            CurrentListofMAEntryDef.clear();
         }
      }

      if(PtoSets_AA)
      {
         delete PtoSets_AA;
         PtoSets_AA = nullptr;
      }

      DumpVersion(stream);

      // M.print(llvm::errs(), nullptr);
      return res;
   }
} // namespace llvm
