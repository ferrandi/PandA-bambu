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
/**
 * @file plugin_dumpGimpleSSA.cpp
 * @brief Plugin to dump functions and global variables in gimple raw format starting from LLVM IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "plugin_includes.hpp"

#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Pass.h"
#include "llvm/Support/ErrorHandling.h"

#if __clang_major__ == 4
#include "llvm/Transforms/Utils/MemorySSA.h"
#else
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Transforms/Utils/LowerMemIntrinsics.h"
#endif
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/Utils/Local.h"

#if HAVE_LIBBDD
#include "HardekopfLin_AA.hpp"
#endif

#include "llvm/RangeAnalysis.hpp"
#include "llvm/eSSA.hpp"

#include "CustomScalarReplacementOfAggregatesPass.hpp"

#include <cxxabi.h>
#include <iomanip>

#include <float.h>

#define PRINT_DBG_MSG 0

static std::string create_file_name_string(const std::string& outdir_name, const std::string& original_filename)
{
   std::size_t found = original_filename.find_last_of("/\\");
   std::string dump_base_name;
   if(found == std::string::npos)
      dump_base_name = original_filename;
   else
      dump_base_name = original_filename.substr(found + 1);
   return outdir_name + "/" + dump_base_name + ".gimplePSSA";
}

#define PEEL_THRESHOLD 16

namespace llvm
{
   char DumpGimpleRaw::buffer[LOCAL_BUFFER_LEN];

#define DEFTREECODE(SYM, STRING, TYPE, NARGS) SYM,
#define DEFGSCODE(SYM, NAME, GSSCODE) SYM,
   enum class DumpGimpleRaw::tree_codes
   {
#include "gcc/c-common.def"
#include "gcc/cp-tree.def"
#include "gcc/gimple.def"
#include "gcc/tree.def"
      DEFGSCODE(GIMPLE_PHI_VIRTUAL, "gimple_phi", GSS_PHI) DEFGSCODE(GIMPLE_ASSIGN_ALLOCA, "gimple_assign", GSS_WITH_MEM_OPS) DEFGSCODE(GIMPLE_NOPMEM, "gimple_nop", GSS_BASE) DEFGSCODE(GETELEMENTPTR, "gimple_assign", GSS_WITH_OPS)
          DEFGSCODE(GIMPLE_SSACOPY, "gimple_assign", GSS_WITH_OPS) DEFTREECODE(ALLOCAVAR_DECL, "var_decl", tcc_declaration, 0) DEFTREECODE(ORIGVAR_DECL, "var_decl", tcc_declaration, 0) DEFTREECODE(INTEGER_CST_SIGNED, "integer_cst", tcc_constant, 0)
              DEFTREECODE(SIGNEDPOINTERTYPE, "integer_type", tcc_type, 0) DEFTREECODE(MISALIGNED_INDIRECT_REF, "misaligned_indirect_ref", tcc_reference, 2) DEFTREECODE(FCMP_OEQ, "truth_andif_expr", tcc_expression, 2)
                  DEFTREECODE(FCMP_ONE, "truth_andif_expr", tcc_expression, 2) DEFTREECODE(FCMP_ORD, "truth_andif_expr", tcc_expression, 2) DEFTREECODE(FCMP_UEQ, "truth_orif_expr", tcc_expression, 2)
                      DEFTREECODE(FCMP_UNE, "truth_orif_expr", tcc_expression, 2) DEFTREECODE(FCMP_UNO, "truth_orif_expr", tcc_expression, 2)
   };
#undef DEFTREECODE
#undef DEFGSCODE
/* Codes of tree nodes.  */
#define DEFTREECODE(SYM, STRING, TYPE, NARGS) STRING,
#define DEFGSCODE(SYM, NAME, GSSCODE) NAME,
   const char* DumpGimpleRaw::tree_codesNames[] = {
#include "gcc/c-common.def"
#include "gcc/cp-tree.def"
#include "gcc/gimple.def"
#include "gcc/tree.def"
       DEFGSCODE(GIMPLE_PHI_VIRTUAL, "gimple_phi", GSS_PHI) DEFGSCODE(GIMPLE_ASSIGN_ALLOCA, "gimple_assign", GSS_WITH_MEM_OPS) DEFGSCODE(GIMPLE_NOPMEM, "gimple_nop", GSS_BASE) DEFGSCODE(GETELEMENTPTR, "gimple_assign", GSS_WITH_OPS)
           DEFGSCODE(GIMPLE_SSACOPY, "gimple_assign", GSS_WITH_OPS) DEFTREECODE(ALLOCAVAR_DECL, "var_decl", tcc_declaration, 0) DEFTREECODE(ORIGVAR_DECL, "var_decl", tcc_declaration, 0) DEFTREECODE(INTEGER_CST_SIGNED, "integer_cst", tcc_constant, 0)
               DEFTREECODE(SIGNEDPOINTERTYPE, "integer_type", tcc_type, 0) DEFTREECODE(MISALIGNED_INDIRECT_REF, "misaligned_indirect_ref", tcc_reference, 2) DEFTREECODE(FCMP_OEQ, "truth_andif_expr", tcc_expression, 2)
                   DEFTREECODE(FCMP_ONE, "truth_andif_expr", tcc_expression, 2) DEFTREECODE(FCMP_ORD, "truth_andif_expr", tcc_expression, 2) DEFTREECODE(FCMP_UEQ, "truth_orif_expr", tcc_expression, 2)
                       DEFTREECODE(FCMP_UNE, "truth_orif_expr", tcc_expression, 2) DEFTREECODE(FCMP_UNO, "truth_orif_expr", tcc_expression, 2)};
#undef DEFTREECODE
#undef DEFGSCODE
#define DEFTREECODE(SYM, STRING, TYPE, NARGS) TYPE,
#define DEFGSCODE(SYM, NAME, GSSCODE) tcc_statement,
   const DumpGimpleRaw::tree_codes_class DumpGimpleRaw::tree_codes2tree_codes_class[] = {
#include "gcc/c-common.def"
#include "gcc/cp-tree.def"
#include "gcc/gimple.def"
#include "gcc/tree.def"
       DEFGSCODE(GIMPLE_PHI_VIRTUAL, "gimple_phi", GSS_PHI) DEFGSCODE(GIMPLE_ASSIGN_ALLOCA, "gimple_assign", GSS_WITH_MEM_OPS) DEFGSCODE(GIMPLE_NOPMEM, "gimple_nop", GSS_BASE) DEFGSCODE(GETELEMENTPTR, "gimple_assign", GSS_WITH_OPS)
           DEFGSCODE(GIMPLE_SSACOPY, "gimple_assign", GSS_WITH_OPS) DEFTREECODE(ALLOCAVAR_DECL, "var_decl", tcc_declaration, 0) DEFTREECODE(ORIGVAR_DECL, "var_decl", tcc_declaration, 0) DEFTREECODE(INTEGER_CST_SIGNED, "integer_cst", tcc_constant, 0)
               DEFTREECODE(SIGNEDPOINTERTYPE, "integer_type", tcc_type, 0) DEFTREECODE(MISALIGNED_INDIRECT_REF, "misaligned_indirect_ref", tcc_reference, 2) DEFTREECODE(FCMP_OEQ, "truth_andif_expr", tcc_expression, 2)
                   DEFTREECODE(FCMP_ONE, "truth_andif_expr", tcc_expression, 2) DEFTREECODE(FCMP_ORD, "truth_andif_expr", tcc_expression, 2) DEFTREECODE(FCMP_UEQ, "truth_orif_expr", tcc_expression, 2)
                       DEFTREECODE(FCMP_UNE, "truth_orif_expr", tcc_expression, 2) DEFTREECODE(FCMP_UNO, "truth_orif_expr", tcc_expression, 2)};
#undef DEFTREECODE
#undef DEFGSCODE
#define DEFTREECODE(SYM, STRING, TYPE, NARGS) NARGS,
#define DEFGSCODE(SYM, NAME, GSSCODE) 0,
   const unsigned int DumpGimpleRaw::tree_codes2nargs[] = {
#include "gcc/c-common.def"
#include "gcc/cp-tree.def"
#include "gcc/gimple.def"
#include "gcc/tree.def"
       DEFGSCODE(GIMPLE_PHI_VIRTUAL, "gimple_phi", GSS_PHI) DEFGSCODE(GIMPLE_ASSIGN_ALLOCA, "gimple_assign", GSS_WITH_MEM_OPS) DEFGSCODE(GIMPLE_NOPMEM, "gimple_nop", GSS_BASE) DEFGSCODE(GETELEMENTPTR, "gimple_assign", GSS_WITH_OPS)
           DEFGSCODE(GIMPLE_SSACOPY, "gimple_assign", GSS_WITH_OPS) DEFTREECODE(ALLOCAVAR_DECL, "var_decl", tcc_declaration, 0) DEFTREECODE(ORIGVAR_DECL, "var_decl", tcc_declaration, 0) DEFTREECODE(INTEGER_CST_SIGNED, "integer_cst", tcc_constant, 0)
               DEFTREECODE(SIGNEDPOINTERTYPE, "integer_type", tcc_type, 0) DEFTREECODE(MISALIGNED_INDIRECT_REF, "misaligned_indirect_ref", tcc_reference, 2) DEFTREECODE(FCMP_OEQ, "truth_andif_expr", tcc_expression, 2)
                   DEFTREECODE(FCMP_ONE, "truth_andif_expr", tcc_expression, 2) DEFTREECODE(FCMP_ORD, "truth_andif_expr", tcc_expression, 2) DEFTREECODE(FCMP_UEQ, "truth_orif_expr", tcc_expression, 2)
                       DEFTREECODE(FCMP_UNE, "truth_orif_expr", tcc_expression, 2) DEFTREECODE(FCMP_UNO, "truth_orif_expr", tcc_expression, 2)};
#undef DEFTREECODE
#undef DEFGSCODE

#define DEFTREECODE(SYM, STRING, TYPE, NARGS)                                                                                                                                                                                        \
   ((TYPE) == tcc_unary ? GIMPLE_UNARY_RHS :                                                                                                                                                                                         \
                          ((TYPE) == tcc_binary || (TYPE) == tcc_comparison) ?                                                                                                                                                       \
                          GIMPLE_BINARY_RHS :                                                                                                                                                                                        \
                          ((TYPE) == tcc_constant || (TYPE) == tcc_declaration || (TYPE) == tcc_reference) ?                                                                                                                         \
                          GIMPLE_SINGLE_RHS :                                                                                                                                                                                        \
                          (GT(SYM) == GT(TRUTH_AND_EXPR) || GT(SYM) == GT(TRUTH_OR_EXPR) || GT(SYM) == GT(TRUTH_XOR_EXPR)) ?                                                                                                         \
                          GIMPLE_BINARY_RHS :                                                                                                                                                                                        \
                          GT(SYM) == GT(TRUTH_NOT_EXPR) ?                                                                                                                                                                            \
                          GIMPLE_UNARY_RHS :                                                                                                                                                                                         \
                          (GT(SYM) == GT(COND_EXPR) || GT(SYM) == GT(WIDEN_MULT_PLUS_EXPR) || GT(SYM) == GT(WIDEN_MULT_MINUS_EXPR) || GT(SYM) == GT(DOT_PROD_EXPR) || GT(SYM) == GT(SAD_EXPR) || GT(SYM) == GT(REALIGN_LOAD_EXPR) || \
                           GT(SYM) == GT(VEC_COND_EXPR) || GT(SYM) == GT(VEC_PERM_EXPR) || GT(SYM) == GT(BIT_INSERT_EXPR) || GT(SYM) == GT(FMA_EXPR)) ?                                                                              \
                          GIMPLE_TERNARY_RHS :                                                                                                                                                                                       \
                          (GT(SYM) == GT(CONSTRUCTOR) || GT(SYM) == GT(OBJ_TYPE_REF) || GT(SYM) == GT(ASSERT_EXPR) || GT(SYM) == GT(ADDR_EXPR) || GT(SYM) == GT(WITH_SIZE_EXPR) || GT(SYM) == GT(SSA_NAME)) ? GIMPLE_SINGLE_RHS : GIMPLE_INVALID_RHS),
#define END_OF_BASE_TREE_CODES GIMPLE_INVALID_RHS,
#define DEFGSCODE(SYM, NAME, GSSCODE) GIMPLE_INVALID_RHS,
   const DumpGimpleRaw::gimple_rhs_class DumpGimpleRaw::gimple_rhs_class_table[] = {
#include "gcc/c-common.def"
#include "gcc/cp-tree.def"
#include "gcc/gimple.def"
#include "gcc/tree.def"
       DEFGSCODE(GIMPLE_PHI_VIRTUAL, "gimple_phi", GSS_PHI) DEFGSCODE(GIMPLE_ASSIGN_ALLOCA, "gimple_assign", GSS_WITH_MEM_OPS) DEFGSCODE(GIMPLE_NOPMEM, "gimple_nop", GSS_BASE) DEFGSCODE(GETELEMENTPTR, "gimple_assign", GSS_WITH_OPS)
           DEFGSCODE(GIMPLE_SSACOPY, "gimple_assign", GSS_WITH_OPS) DEFTREECODE(ALLOCAVAR_DECL, "var_decl", tcc_declaration, 0) DEFTREECODE(ORIGVAR_DECL, "var_decl", tcc_declaration, 0) DEFTREECODE(INTEGER_CST_SIGNED, "integer_cst", tcc_constant, 0)
               DEFTREECODE(SIGNEDPOINTERTYPE, "integer_type", tcc_type, 0) DEFTREECODE(MISALIGNED_INDIRECT_REF, "misaligned_indirect_ref", tcc_reference, 2) DEFTREECODE(FCMP_OEQ, "truth_andif_expr", tcc_expression, 2)
                   DEFTREECODE(FCMP_ONE, "truth_andif_expr", tcc_expression, 2) DEFTREECODE(FCMP_ORD, "truth_andif_expr", tcc_expression, 2) DEFTREECODE(FCMP_UEQ, "truth_orif_expr", tcc_expression, 2)
                       DEFTREECODE(FCMP_UNE, "truth_orif_expr", tcc_expression, 2) DEFTREECODE(FCMP_UNO, "truth_orif_expr", tcc_expression, 2)};
#undef DEFTREECODE
#undef END_OF_BASE_TREE_CODE

   const char* DumpGimpleRaw::ValueTyNames[] = {
#define HANDLE_VALUE(Name) #Name,
#include "llvm/IR/Value.def"
#define HANDLE_INST(N, OPC, CLASS) #OPC,
#include "llvm/IR/Instruction.def"
   };

#define DEF_BUILTIN(X, N, C, T, LT, B, F, NA, AT, IM, COND) N,
   const std::set<std::string> DumpGimpleRaw::builtinsNames = {
#include "gcc/builtins.def"
   };
#undef DEF_BUILTIN

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

   DumpGimpleRaw::DumpGimpleRaw(const std::string& _outdir_name, const std::string& _InFile, bool _onlyGlobals, std::map<std::string, std::vector<std::string>>* _fun2params, bool early)
       : earlyAnalysis(early),
         outdir_name(_outdir_name),
         InFile(_InFile),
         filename(create_file_name_string(_outdir_name, _InFile)),
#if __clang_major__ >= 7
         stream(create_file_name_string(_outdir_name, _InFile), EC, sys::fs::FA_Read | sys::fs::FA_Write),
#else
         stream(create_file_name_string(_outdir_name, _InFile), EC, llvm::sys::fs::F_RW),
#endif
         onlyGlobals(_onlyGlobals),
         fun2params(_fun2params),
         DL(nullptr),
         modulePass(nullptr),
         last_used_index(0),
         column(0),
         PtoSets_AA(nullptr),
         SignedPointerTypeReference(0),
         last_memory_ssa_vers(std::numeric_limits<int>::max()),
         last_BB_index(2),
         RA(nullptr)
   {
      if(EC)
      {
         llvm::report_fatal_error("not able to open the output raw file");
      }
      DumpVersion(stream);
      assignCode(&SignedPointerTypeReference, GT(SIGNEDPOINTERTYPE));
   }

   std::string DumpGimpleRaw::getTypeName(const void* t) const
   {
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      bool is_signed = CheckSignedTag(ty);
      ty = NormalizeSignedTag(ty);
      if(ty->isSingleValueType())
      {
         if(ty->isVoidTy())
            return "void";
         else if(ty->isHalfTy())
            return "_Float16";
         else if(ty->isFloatTy())
            return "float";
         else if(ty->isDoubleTy())
            return "double";
         else if(ty->isX86_FP80Ty())
            return "long double";
         else if(ty->isFP128Ty())
            return "long double";
         else if(ty->isIntegerTy())
         {
            if(ty->getScalarSizeInBits() == 1)
               return "_Bool";
            else if(ty->getScalarSizeInBits() > 1 && ty->getScalarSizeInBits() <= 8)
               return is_signed ? "signed char" : "unsigned char";
            else if(ty->getScalarSizeInBits() > 8 && ty->getScalarSizeInBits() <= 16)
               return is_signed ? "short int" : "unsigned short int";
            else if(ty->getScalarSizeInBits() > 16 && ty->getScalarSizeInBits() <= 32)
               return is_signed ? "int" : "unsigned int";
            else if(ty->getScalarSizeInBits() > 32 && ty->getScalarSizeInBits() <= 64)
               return is_signed ? "long long int" : "unsigned long long int";
            else
               llvm_unreachable("not expected integer bitwidth size");
         }
      }
      llvm_unreachable("not managed");
   }

   const void* DumpGimpleRaw::assignCodeAuto(const void* t)
   {
      assert(t);
      if(HAS_CODE(t))
         return t;

      const llvm::Value* llvm_obj = reinterpret_cast<const llvm::Value*>(t);
      auto vid = llvm_obj->getValueID();
      switch(vid)
      {
         case llvm::Value::GlobalVariableVal:
            return assignCode(t, GT(VAR_DECL));
         case llvm::Value::FunctionVal:
            return assignCode(t, GT(FUNCTION_DECL));
         case llvm::Value::ConstantIntVal:
            return assignCode(t, GT(INTEGER_CST));
         case llvm::Value::ConstantFPVal:
            return assignCode(t, GT(REAL_CST));
         case llvm::Value::ArgumentVal:
            return assignCode(t, GT(PARM_DECL));
         case llvm::Value::ConstantPointerNullVal:
            return assignCode(t, GT(INTEGER_CST));
         case llvm::Value::ConstantStructVal:
            return assignCode(t, GT(CONSTRUCTOR));
         case llvm::Value::ConstantAggregateZeroVal:
            return assignCode(t, GT(CONSTRUCTOR));
         case llvm::Value::ConstantDataArrayVal:
            return assignCode(t, GT(CONSTRUCTOR));
         case llvm::Value::ConstantArrayVal:
            return assignCode(t, GT(CONSTRUCTOR));
         case llvm::Value::ConstantDataVectorVal:
            return assignCode(t, GT(CONSTRUCTOR));
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
               llvm_unreachable((std::string("unexpected condition: ") + std::string(ValueTyNames[llvm_obj->getValueID()])).c_str());
            }
         }
         case llvm::Value::ConstantExprVal:
         {
            auto type = assignCodeType(llvm_obj->getType());
            if(cast<llvm::ConstantExpr>(llvm_obj)->getOpcode() == llvm::Instruction::GetElementPtr)
               return LowerGetElementPtr(type, cast<llvm::ConstantExpr>(llvm_obj), nullptr);
            else if(cast<llvm::ConstantExpr>(llvm_obj)->getOpcode() == llvm::Instruction::IntToPtr || cast<llvm::ConstantExpr>(llvm_obj)->getOpcode() == llvm::Instruction::PtrToInt)
            {
               auto op = cast<llvm::ConstantExpr>(llvm_obj)->getOperand(0);
               return build1(GT(NOP_EXPR), type, getOperand(op, nullptr));
            }
            else if(cast<llvm::ConstantExpr>(llvm_obj)->getOpcode() == llvm::Instruction::BitCast)
            {
               auto op = cast<llvm::ConstantExpr>(llvm_obj)->getOperand(0);
               return build1(GT(VIEW_CONVERT_EXPR), type, getOperand(op, nullptr));
            }
            else
            {
               llvm_obj->print(llvm::errs(), true);
               llvm::errs() << "\n";
               llvm::errs() << cast<llvm::ConstantExpr>(llvm_obj)->getOpcodeName() << "\n";
               stream.close();
               llvm_unreachable((std::string("unexpected condition: ") + std::string(ValueTyNames[llvm_obj->getValueID()])).c_str());
            }
         }

#define HANDLE_BINARY_INST(N, OPC, CLASS)                     \
   case llvm::Value::InstructionVal + llvm::Instruction::OPC: \
      return assignCode(t, GT(GIMPLE_ASSIGN));
#include "llvm/IR/Instruction.def"
         case llvm::Value::InstructionVal + llvm::Instruction::Store:
         case llvm::Value::InstructionVal + llvm::Instruction::Load:
         case llvm::Value::InstructionVal + llvm::Instruction::Select:
         case llvm::Value::InstructionVal + llvm::Instruction::FCmp:
         case llvm::Value::InstructionVal + llvm::Instruction::ICmp:
         case llvm::Value::InstructionVal + llvm::Instruction::BitCast:
            return assignCode(t, GT(GIMPLE_ASSIGN));
         case llvm::Value::InstructionVal + llvm::Instruction::Ret:
            return assignCode(t, GT(GIMPLE_RETURN));
         case llvm::Value::InstructionVal + llvm::Instruction::Br:
         {
            const llvm::BranchInst* br = reinterpret_cast<const llvm::BranchInst*>(t);
            if(br->isUnconditional())
               return assignCode(t, GT(GIMPLE_GOTO));
            else
               return assignCode(t, GT(GIMPLE_COND));
         }
         case llvm::Value::InstructionVal + llvm::Instruction::PHI:
            return assignCode(t, GT(GIMPLE_PHI));
         case llvm::Value::InstructionVal + llvm::Instruction::Alloca:
            return assignCode(t, GT(GIMPLE_ASSIGN_ALLOCA));
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
                     return assignCode(t, GT(GIMPLE_NOPMEM));
                  case llvm::Intrinsic::memcpy:
                  case llvm::Intrinsic::memset:
                  case llvm::Intrinsic::memmove:
                  case llvm::Intrinsic::trap:
                     return ci->use_empty() ? assignCode(t, GT(GIMPLE_CALL)) : assignCode(t, GT(GIMPLE_ASSIGN));
                  case llvm::Intrinsic::fabs:
                     return assignCode(t, GT(GIMPLE_ASSIGN));
                  case llvm::Intrinsic::rint:
                     return assignCode(t, GT(GIMPLE_ASSIGN));
                  case llvm::Intrinsic::fmuladd:
                     return assignCode(t, GT(GIMPLE_ASSIGN));
#if __clang_major__ != 4
                  case llvm::Intrinsic::ssa_copy:
                     return assignCode(t, GT(GIMPLE_SSACOPY));
#endif
                  case llvm::Intrinsic::minnum:
                  case llvm::Intrinsic::maxnum:
                     return assignCode(t, GT(GIMPLE_ASSIGN));
                  default:
                     llvm::errs() << "assignCodeAuto kind not supported: " << ValueTyNames[vid] << "\n";
                     ci->print(llvm::errs(), true);
                     stream.close();
                     llvm_unreachable("Plugin Error");
               }
            }
            auto calledFun = ci->getCalledValue();
            if(isa<llvm::InlineAsm>(calledFun))
               return assignCode(t, GT(GIMPLE_ASM));
            if(ci->getType()->isVoidTy() || ci->use_empty())
            {
               assert(ci->use_empty());
               return assignCode(t, GT(GIMPLE_CALL));
            }
            else
               return assignCode(t, GT(GIMPLE_ASSIGN));
         }
         case llvm::Value::InstructionVal + llvm::Instruction::GetElementPtr:
            return assignCode(t, GT(GETELEMENTPTR));
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
            return assignCode(t, GT(GIMPLE_ASSIGN));
         case llvm::Value::InstructionVal + llvm::Instruction::Switch:
            return assignCode(t, GT(GIMPLE_SWITCH));
         case llvm::Value::InstructionVal + llvm::Instruction::Unreachable:
            return assignCode(t, GT(GIMPLE_RETURN));
         default:
            llvm::errs() << "assignCodeAuto kind not supported: " << ValueTyNames[vid] << "\n";
            stream.close();
            llvm_unreachable("Plugin Error");
      }
   }
   bool DumpGimpleRaw::DECL_ASSEMBLER_NAME_SET_P(const void* t) const
   {
      if(TREE_CODE(t) == GT(TRANSLATION_UNIT_DECL))
         return false;
      if(TREE_CODE(t) == GT(ALLOCAVAR_DECL))
         return false;
      if(TREE_CODE(t) == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return DECL_ASSEMBLER_NAME_SET_P(ov->orig);
      }
      if(TREE_CODE(t) == GT(LABEL_DECL))
         return false;
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      if(!llvm_obj->getName().empty())
      {
         std::string declname = std::string(llvm_obj->getName());
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

   const void* DumpGimpleRaw::DECL_ASSEMBLER_NAME(const void* t)
   {
      assert(TREE_CODE(t) != GT(TRANSLATION_UNIT_DECL));
      if(TREE_CODE(t) == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return DECL_ASSEMBLER_NAME(ov->orig);
      }
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      if(!llvm_obj->getName().empty())
      {
         std::string declname = std::string(llvm_obj->getName());
         if(identifierTable.find(declname) == identifierTable.end())
            identifierTable.insert(declname);
         const void* dn = identifierTable.find(declname)->c_str();
         return assignCode(dn, GT(IDENTIFIER_NODE));
      }
      else
         llvm_unreachable("DECL_ASSEMBLER_NAME: DECL_ASSEMBLER_NAME_SET_P is not true");
   }

   static std::string getIntrinsicName(const llvm::Function* fd)
   {
      assert(fd->isIntrinsic());
      auto intID = fd->getIntrinsicID();
      switch(intID)
      {
         case llvm::Intrinsic::fabs:
            if(fd->getReturnType()->isFloatTy())
               return "fabsf";
            else if(fd->getReturnType()->isDoubleTy())
               return "fabs";
            else if(fd->getReturnType()->isFP128Ty())
               return "fabsl";
            fd->print(llvm::errs());
            llvm_unreachable("Plugin Error");
         case llvm::Intrinsic::memcpy:
         {
            auto funType = cast<llvm::FunctionType>(fd->getValueType());
            if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 32)
               return "_llvm_memcpy_p0i8_p0i8_i32";
            else if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 64)
               return "_llvm_memcpy_p0i8_p0i8_i64";
            fd->print(llvm::errs());
            llvm_unreachable("Plugin Error");
         }

         case llvm::Intrinsic::memset:
         {
            auto funType = cast<llvm::FunctionType>(fd->getValueType());
            if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 32)
               return "_llvm_memset_p0i8_i32";
            else if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 64)
               return "_llvm_memset_p0i8_i64";
            fd->print(llvm::errs());
            llvm_unreachable("Plugin Error");
         }
         case llvm::Intrinsic::memmove:
         {
            auto funType = cast<llvm::FunctionType>(fd->getValueType());
            if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 32)
               return "_llvm_memmove_p0i8_p0i8_i32";
            else if(funType->getParamType(2)->isIntegerTy() && funType->getParamType(2)->getScalarSizeInBits() == 64)
               return "_llvm_memmove_p0i8_p0i8_i64";
            fd->print(llvm::errs());
            llvm_unreachable("Plugin Error");
         }
         case llvm::Intrinsic::trap:
            return "__builtin_trap";
         case llvm::Intrinsic::dbg_value:
            return "__builtin_debug_value";
         case llvm::Intrinsic::rint:
         {
            if(fd->getReturnType()->isFloatTy())
               return "rintf";
            else if(fd->getReturnType()->isDoubleTy())
               return "rint";
            else if(fd->getReturnType()->isFP128Ty())
               return "rintl";
            fd->print(llvm::errs());
            llvm_unreachable("Plugin Error");
         }
         case llvm::Intrinsic::fmuladd:
         {
            if(fd->getReturnType()->isFloatTy())
               return "__float32_muladdif";
            else if(fd->getReturnType()->isDoubleTy())
               return "__float64_muladdif";
            fd->print(llvm::errs());
            llvm_unreachable("Plugin Error");
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
            llvm_unreachable("Plugin Error");
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
            llvm_unreachable("Plugin Error");
         }
         default:
            fd->print(llvm::errs());
            llvm_unreachable("Plugin Error");
      }
   }
   const void* DumpGimpleRaw::DECL_NAME(const void* t)
   {
      if(TREE_CODE(t) == GT(TRANSLATION_UNIT_DECL))
         return nullptr;
      if(TREE_CODE(t) == GT(ALLOCAVAR_DECL))
         return nullptr;
      if(TREE_CODE(t) == GT(ORIGVAR_DECL))
         return nullptr;
      if(TREE_CODE(t) == GT(LABEL_DECL))
         return nullptr;
      if(TREE_CODE(t) == GT(FIELD_DECL))
      {
         const field_decl* ty = reinterpret_cast<const field_decl*>(t);
         return ty->name;
      }
      const llvm::Value* llvm_obj = reinterpret_cast<const llvm::Value*>(t);
      if(llvm_obj->hasName() && TREE_CODE(t) == GT(FUNCTION_DECL))
      {
         const llvm::Function* fd = cast<const llvm::Function>(llvm_obj);
         std::string declname;
         if(fd->isIntrinsic())
            declname = getIntrinsicName(fd);
         else
         {
            declname = std::string(llvm_obj->getName());
            int status;
            char* demangled_outbuffer = abi::__cxa_demangle(declname.c_str(), nullptr, nullptr, &status);
            if(status == 0)
            {
               if(std::string(demangled_outbuffer).find(':') == std::string::npos && std::string(demangled_outbuffer).find('(') != std::string::npos)
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
         return assignCode(dn, GT(IDENTIFIER_NODE));
      }
      else if(TREE_CODE(t) == GT(PARM_DECL))
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
         return assignCode(dn, GT(IDENTIFIER_NODE));
      }
      else if(llvm_obj->hasName())
      {
         std::string declname = std::string(llvm_obj->getName());
         if(identifierTable.find(declname) == identifierTable.end())
            identifierTable.insert(declname);
         const void* dn = identifierTable.find(declname)->c_str();
         return assignCode(dn, GT(IDENTIFIER_NODE));
      }
      else
         return nullptr;
   }

   const char* DumpGimpleRaw::IDENTIFIER_POINTER(const void* t) const
   {
      const char* ii = reinterpret_cast<const char*>(t);
      return ii;
   }

   int DumpGimpleRaw::IDENTIFIER_LENGTH(const void* t) const
   {
      const char* ii = reinterpret_cast<const char*>(t);
      return static_cast<int>(std::strlen(ii));
   }

   const void* DumpGimpleRaw::TREE_PURPOSE(const void* t) const
   {
      const tree_list* tl = reinterpret_cast<const tree_list*>(t);
      return tl->purp;
   }

   const void* DumpGimpleRaw::TREE_VALUE(const void* t) const
   {
      const tree_list* tl = reinterpret_cast<const tree_list*>(t);
      return tl->valu;
   }

   const void* DumpGimpleRaw::TREE_CHAIN(const void* t) const
   {
      const tree_list* tl = reinterpret_cast<const tree_list*>(t);
      if(tl->chan)
      {
         assert(index2tree_list.find(tl->chan) != index2tree_list.end());
         return &index2tree_list.find(tl->chan)->second;
      }
      else
         return nullptr;
   }

   int DumpGimpleRaw::TREE_VEC_LENGTH(const void* t) const
   {
      const tree_vec* tv = reinterpret_cast<const tree_vec*>(t);
      return static_cast<int>(tv->data.size());
   }

   const void* DumpGimpleRaw::TREE_VEC_ELT(const void* t, int i) const
   {
      const tree_vec* tv = reinterpret_cast<const tree_vec*>(t);
      return tv->data.at(static_cast<size_t>(i));
   }

   const void* DumpGimpleRaw::DECL_SOURCE_LOCATION(const void* t) const
   {
      if(TREE_CODE(t) == GT(TRANSLATION_UNIT_DECL))
         return nullptr;
      else if(TREE_CODE(t) == GT(FIELD_DECL))
         return nullptr;
      else if(TREE_CODE(t) == GT(LABEL_DECL))
         return nullptr;
      else if(TREE_CODE(t) == GT(ALLOCAVAR_DECL))
         return nullptr;
      else if(TREE_CODE(t) == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return DECL_SOURCE_LOCATION(ov->orig);
      }
      else if(TREE_CODE(t) == GT(PARM_DECL))
         return nullptr;
      else
      {
         const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
         return llvm_obj->hasMetadata() ? llvm_obj->getMetadata(llvm::LLVMContext::MD_dbg) : nullptr;
      }
   }

   DumpGimpleRaw::expanded_location DumpGimpleRaw::expand_location(const void* i) const
   {
      const llvm::MDNode* llvm_obj = reinterpret_cast<const llvm::MDNode*>(i);
      expanded_location res;
      if(dyn_cast<llvm::DIExpression>(llvm_obj) || dyn_cast<llvm::DIEnumerator>(llvm_obj) || dyn_cast<llvm::DITemplateParameter>(llvm_obj) || dyn_cast<llvm::DIEnumerator>(llvm_obj) || dyn_cast<llvm::DISubrange>(llvm_obj) ||
         dyn_cast<llvm::GenericDINode>(llvm_obj) || dyn_cast<llvm::MDTuple>(llvm_obj))
      {
      }
      else if(auto* di = dyn_cast<llvm::DIGlobalVariableExpression>(llvm_obj))
      {
         res = expand_location(di->getVariable());
      }
      else if(auto* di = dyn_cast<llvm::DIVariable>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DISubprogram>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DILocation>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
         res.column = di->getColumn();
      }
      else if(auto* di = dyn_cast<llvm::DILexicalBlock>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
         res.column = di->getColumn();
      }
      else if(auto* di = dyn_cast<llvm::DIMacroFile>(llvm_obj))
      {
         res.filename = di->getFile()->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DIMacro>(llvm_obj))
      {
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DIImportedEntity>(llvm_obj))
      {
         // res.filename = di->getFile()->getFilename();
         // res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DIObjCProperty>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DICompileUnit>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
      }
      else if(auto* di = dyn_cast<llvm::DIFile>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
      }
      else if(auto* di = dyn_cast<llvm::DICompositeType>(llvm_obj))
      {
         if(di->getFilename().empty())
            res = expand_location(di->getBaseType());
         else
         {
            res.filename = di->getFilename();
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
            res.filename = di->getFilename();
            res.file = res.filename.c_str();
            res.line = di->getLine();
         }
      }
      else if(auto* di = dyn_cast<llvm::DIType>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto* di = dyn_cast<llvm::DINamespace>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
#if __clang_major__ == 4
         res.line = di->getLine();
#endif
      }
      else if(auto* di = dyn_cast<llvm::DIModule>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
      }
      else if(auto* di = dyn_cast<llvm::DILexicalBlockFile>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
      }
      else
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "expand_location: unexpected location %d", llvm_obj->getMetadataID());
         llvm_unreachable(buffer);
      }
      return res;
   }

   bool DumpGimpleRaw::gimple_has_location(const void* g) const
   {
      if(TREE_CODE(g) == GT(GIMPLE_NOP) || TREE_CODE(g) == GT(GIMPLE_PHI_VIRTUAL) || TREE_CODE(g) == GT(GIMPLE_LABEL))
         return false;
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);

      if(inst->hasMetadata() && inst->getMetadata(llvm::LLVMContext::MD_dbg) != nullptr)
         return true;
      else
         return MetaDataMap.find(inst) != MetaDataMap.end();
   }

   const void* DumpGimpleRaw::gimple_location(const void* g) const
   {
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      if(MetaDataMap.find(inst) != MetaDataMap.end())
         return MetaDataMap.find(inst)->second;
      else if(inst->hasMetadata() && inst->getMetadata(llvm::LLVMContext::MD_dbg) != nullptr)
         return inst->getMetadata(llvm::LLVMContext::MD_dbg);
      else
         return nullptr;
   }

   const DumpGimpleRaw::pt_info* DumpGimpleRaw::SSA_NAME_PTR_INFO(const void* t) const
   {
      const ssa_name* ssa = reinterpret_cast<const ssa_name*>(t);
      return ssa->ptr_info.valid ? &ssa->ptr_info : nullptr;
   }

   void DumpGimpleRaw::dump_pt_solution(const DumpGimpleRaw::pt_solution* pt, const char* first_tag, const char* second_tag)
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

   int DumpGimpleRaw::getBB_index(const llvm::BasicBlock* BB)
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
      return ce->getPredicate();
   }

   template <class InstructionOrConstantExpr>
   DumpGimpleRaw::tree_codes DumpGimpleRaw::tree_expr_code(InstructionOrConstantExpr* inst)
   {
      auto opcode = inst->getOpcode();
      switch(opcode)
      {
         case llvm::Instruction::Add:
            return GT(PLUS_EXPR);
         case llvm::Instruction::FAdd:
            return GT(PLUS_EXPR);
         case llvm::Instruction::Sub:
            return GT(MINUS_EXPR);
         case llvm::Instruction::FSub:
            return GT(MINUS_EXPR);
         case llvm::Instruction::Mul:
            return GT(MULT_EXPR);
         case llvm::Instruction::FMul:
            return GT(MULT_EXPR);
         case llvm::Instruction::UDiv:
            return GT(TRUNC_DIV_EXPR);
         case llvm::Instruction::SDiv:
            return GT(TRUNC_DIV_EXPR);
         case llvm::Instruction::FDiv:
            return GT(RDIV_EXPR);
         case llvm::Instruction::URem:
            return GT(TRUNC_MOD_EXPR);
         case llvm::Instruction::SRem:
            return GT(TRUNC_MOD_EXPR);
         case llvm::Instruction::FRem:
            llvm_unreachable("floating point remainder not foreseen yet");
         // Logical operators (integer operands)
         case llvm::Instruction::Shl: // Shift left  (logical)
            return GT(LSHIFT_EXPR);
         case llvm::Instruction::LShr: // Shift right (logical)
            return GT(RSHIFT_EXPR);
         case llvm::Instruction::AShr: // Shift right (arithmetic)
            return GT(RSHIFT_EXPR);
         case llvm::Instruction::And:
            return GT(BIT_AND_EXPR);
         case llvm::Instruction::Or:
            return GT(BIT_IOR_EXPR);
         case llvm::Instruction::Xor:
            return GT(BIT_XOR_EXPR);

         case llvm::Instruction::Store:
            return GT(SSA_NAME);
         case llvm::Instruction::Load:
            return GT(MEM_REF);

         case llvm::Instruction::BitCast:
            return GT(VIEW_CONVERT_EXPR);
         case llvm::Instruction::Call:
            return GT(CALL_EXPR);
         case llvm::Instruction::SExt:
         case llvm::Instruction::ZExt:
         case llvm::Instruction::Trunc:
         case llvm::Instruction::PtrToInt:
         case llvm::Instruction::IntToPtr:
         case llvm::Instruction::FPExt:
         case llvm::Instruction::FPTrunc:
            return GT(NOP_EXPR);

         case llvm::Instruction::FPToSI:
         case llvm::Instruction::FPToUI:
            return GT(FIX_TRUNC_EXPR);

         case llvm::Instruction::UIToFP:
         case llvm::Instruction::SIToFP:
            return GT(FLOAT_EXPR);

         case llvm::Instruction::FCmp:
         case llvm::Instruction::ICmp:
         {
            auto predicate = getPredicateTEC(inst);
            switch(predicate)
            {
               case llvm::ICmpInst::FCMP_OEQ:
                  return GT(FCMP_OEQ);
               case llvm::ICmpInst::ICMP_EQ:
                  return GT(EQ_EXPR);
               case llvm::ICmpInst::FCMP_ONE:
                  return GT(FCMP_ONE);
               case llvm::ICmpInst::ICMP_NE:
                  return GT(NE_EXPR);
               case llvm::ICmpInst::FCMP_OGT:
               case llvm::ICmpInst::ICMP_UGT:
               case llvm::ICmpInst::ICMP_SGT:
                  return GT(GT_EXPR);
               case llvm::ICmpInst::FCMP_OGE:
               case llvm::ICmpInst::ICMP_UGE:
               case llvm::ICmpInst::ICMP_SGE:
                  return GT(GE_EXPR);
               case llvm::ICmpInst::FCMP_OLT:
               case llvm::ICmpInst::ICMP_ULT:
               case llvm::ICmpInst::ICMP_SLT:
                  return GT(LT_EXPR);
               case llvm::ICmpInst::FCMP_OLE:
               case llvm::ICmpInst::ICMP_ULE:
               case llvm::ICmpInst::ICMP_SLE:
                  return GT(LE_EXPR);
               case llvm::ICmpInst::FCMP_UEQ:
                  return GT(FCMP_UEQ);
               case llvm::ICmpInst::FCMP_UNE:
                  return GT(FCMP_UNE);
               case llvm::ICmpInst::FCMP_UGT:
                  return GT(UNGT_EXPR);
               case llvm::ICmpInst::FCMP_UGE:
                  return GT(UNGE_EXPR);
               case llvm::ICmpInst::FCMP_ULT:
                  return GT(UNLT_EXPR);
               case llvm::ICmpInst::FCMP_ULE:
                  return GT(UNLE_EXPR);
               case llvm::ICmpInst::FCMP_ORD:
                  return GT(FCMP_ORD);
               case llvm::ICmpInst::FCMP_UNO:
                  return GT(FCMP_UNO);
               default:
                  llvm::errs() << "gimple_expr_code::ICmpInst kind not supported: " << predicate << "\n";
                  stream.close();
                  llvm_unreachable("Plugin Error");
            }
         }

         case llvm::Instruction::Select:
            return GT(COND_EXPR);
         case llvm::Instruction::Br:
         {
            assert(cast<const llvm::BranchInst>(inst)->isConditional());
            return GT(SSA_NAME);
         }
         default:
            llvm::errs() << "gimple_expr_code kind not supported: " << ValueTyNames[llvm::Value::InstructionVal + opcode] << "\n";
            stream.close();
            llvm_unreachable("Plugin Error");
      }
   }
   DumpGimpleRaw::tree_codes DumpGimpleRaw::gimple_expr_code(const void* g)
   {
      tree_codes code = gimple_code(g);
      if(code == GT(GIMPLE_ASSIGN) || code == GT(GIMPLE_COND))
      {
         const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
         assert(inst);
         return tree_expr_code(inst);
      }
      else
      {
         assert(code == GT(GIMPLE_CALL));
         return GT(CALL_EXPR);
      }
   }

   const void* DumpGimpleRaw::gimple_assign_lhs(const void* g)
   {
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      auto currentFunction = inst->getFunction();
      if(isa<llvm::StoreInst>(inst))
      {
         const llvm::StoreInst& store = cast<const llvm::StoreInst>(*inst);
         if(uicTable.find(0) == uicTable.end())
            uicTable[0] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt32Ty(inst->getContext()), 0, false));
         const void* zero = uicTable.find(0)->second;
         auto addr = getOperand(store.getPointerOperand(), currentFunction);
         auto ty = store.getValueOperand()->getType();
         auto type = assignCodeType(ty);
         auto written_obj_size = ty->isSized() ? DL->getTypeAllocSizeInBits(ty) : 8ULL;
         auto funName = store.getFunction()->getName();
         auto demangled = getDemangled(funName);
         bool is_a_top_parameter = isa<llvm::Argument>(store.getPointerOperand()) && (funName == TopFunctionName || demangled == TopFunctionName);
         if(store.getAlignment() && written_obj_size > (8 * store.getAlignment()) && !is_a_top_parameter)
            return build1(GT(MISALIGNED_INDIRECT_REF), type, addr);
         else
            return build2(GT(MEM_REF), type, addr, zero);
      }
      return getSSA(inst, g, currentFunction, false);
   }

   const void* DumpGimpleRaw::gimple_assign_rhs_alloca(const void* g)
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
         const llvm::TargetLibraryInfo& TLI = modulePass->getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();
         av.addr = temporary_addr_check(inst, visited, TLI);
         allocaVar = assignCode(&av, GT(ALLOCAVAR_DECL));
      }
      else
         allocaVar = &index2alloca_var.find(g)->second;
      return build1(GT(ADDR_EXPR), type, allocaVar);
   }

   const void* DumpGimpleRaw::DECL_ABSTRACT_ORIGIN(const void* t)
   {
      const void* origVar;
      if(index2orig_var.find(t) == index2orig_var.end())
      {
         auto& ov = index2orig_var[t];
         ov.orig = t;
         origVar = assignCode(&ov, GT(ORIGVAR_DECL));
      }
      else
         origVar = &index2orig_var.find(t)->second;
      return origVar;
   }

   const void* DumpGimpleRaw::gimple_assign_rhs_getelementptr(const void* g)
   {
      llvm::Instruction* inst = const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(g));
      llvm::Function* currentFunction = inst->getFunction();
      return LowerGetElementPtr(inst->getType(), inst, currentFunction);
   }

   bool DumpGimpleRaw::temporary_addr_check(const llvm::User* inst, std::set<const llvm::User*>& visited, const llvm::TargetLibraryInfo& TLI)
   {
      for(auto U : inst->users())
      {
         if(visited.find(U) == visited.end())
         {
            visited.insert(U);
            if(isa<llvm::LoadInst>(U))
               ;
            else if(isa<llvm::StoreInst>(U))
            {
               if(U->getOperand(0) == inst)
                  return false;
            }
            else if(isa<llvm::PHINode>(U) || isa<llvm::GetElementPtrInst>(U) || isa<llvm::BitCastInst>(U) || isa<llvm::PtrToIntInst>(U) || isa<llvm::IntToPtrInst>(U) || cast<llvm::Instruction>(U)->getOpcode() == llvm::Instruction::Sub ||
                    cast<llvm::Instruction>(U)->getOpcode() == llvm::Instruction::Add || cast<llvm::Instruction>(U)->getOpcode() == llvm::Instruction::AShr || cast<llvm::Instruction>(U)->getOpcode() == llvm::Instruction::And ||
                    cast<llvm::Instruction>(U)->getOpcode() == llvm::Instruction::Or)
            {
               if(isa<llvm::PHINode>(U)
#if HAVE_LIBBDD
                  && (!PtoSets_AA || !is_PTS(PtoSets_AA->PE(U), TLI))
#endif
               )
                  return false;
               auto res = temporary_addr_check(U, visited, TLI);
               if(!res)
                  return res;
            }
            else if(isa<llvm::CmpInst>(U))
               ;
            else if(isa<llvm::SelectInst>(U))
            {
               auto si = cast<llvm::SelectInst>(U);
               if(si->getOperand(0) == inst)
                  ;
               else
               {
#if HAVE_LIBBDD
                  if(!PtoSets_AA || !is_PTS(PtoSets_AA->PE(U), TLI))
#endif
                     return false;
                  auto res = temporary_addr_check(U, visited, TLI);
                  if(!res)
                     return res;
               }
            }
            else if(isa<llvm::CallInst>(U))
               return false;
            else if(isa<llvm::ReturnInst>(U))
               return false;
            else
               return false;
         }
      }
      return true;
   }

   void DumpGimpleRaw::add_alloca_pt_solution(const void* lhs, const void* rhs)
   {
      ssa_name* lhs_ssa = const_cast<ssa_name*>(reinterpret_cast<const ssa_name*>(lhs));
      const void* vd = TREE_OPERAND(rhs, 0);
      lhs_ssa->ptr_info.valid = true;
      lhs_ssa->ptr_info.pt.vars.insert(vd);
   }

   const void* DumpGimpleRaw::getGimpleNop(const llvm::Value* operand, const void* scpe)
   {
      if(index2gimple_nop.find(operand) == index2gimple_nop.end())
      {
         auto& gn = index2gimple_nop[operand];
         assignCode(&gn, GT(GIMPLE_NOP));
         gn.scpe = assignCodeAuto(scpe);
      }
      return &index2gimple_nop.find(operand)->second;
   }

   template <class InstructionOrConstantExpr>
   bool DumpGimpleRaw::isSignedInstruction(const InstructionOrConstantExpr* inst) const
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
               return isSignedResult(dyn_cast<InstructionOrConstantExpr>(op0)) && isSignedResult(dyn_cast<InstructionOrConstantExpr>(op1));
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
               return isSignedResult(dyn_cast<InstructionOrConstantExpr>(op0)) && isSignedResult(dyn_cast<InstructionOrConstantExpr>(op1));
            else
               return false;
         }
         default:
            return false;
      }
   }

   template <class InstructionOrConstantExpr>
   bool DumpGimpleRaw::isSignedResult(const InstructionOrConstantExpr* inst) const
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
   const llvm::Type* DumpGimpleRaw::getCondSignedResult(const llvm::Value* operand, const llvm::Type* type) const
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
   bool DumpGimpleRaw::isSignedOperand(const InstructionOrConstantExpr* inst, unsigned index) const
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
         default:
            return false;
      }
   }

   template <class InstructionOrConstantExpr>
   bool DumpGimpleRaw::isUnsignedOperand(const InstructionOrConstantExpr* inst, unsigned index) const
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
         default:
            return false;
      }
   }

   const void* DumpGimpleRaw::getSSA(const llvm::Value* operand, const void* def_stmt, const llvm::Function* currentFunction, bool isDefault)
   {
      auto isVirtual = operand->getValueID() == llvm::Value::MemoryDefVal || operand->getValueID() == llvm::Value::MemoryUseVal || operand->getValueID() == llvm::Value::MemoryPhiVal;
      auto key = std::make_pair(def_stmt, isVirtual);
      if(index2ssa_name.find(key) == index2ssa_name.end())
      {
         int ssa_vers;
         auto& sn = index2ssa_name[key];
         assignCode(&sn, GT(SSA_NAME));
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
#if HAVE_LIBBDD
            if(operand->getType()->isPointerTy() && PtoSets_AA)
            {
               auto varId = PtoSets_AA->PE(operand);
               const llvm::TargetLibraryInfo& TLI = modulePass->getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();
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
                           sn.ptr_info.pt.vars.insert(TREE_OPERAND(gimple_assign_rhs_alloca(val), 0));
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
                           llvm_unreachable(("unexpected pointer to variable " + std::string(ValueTyNames[val->getValueID()])).c_str());
                        }
                     }
                  }
               }
            }
#endif
         }
         sn.vers = ssa_vers;
         assert(HAS_CODE(def_stmt));
         sn.def_stmts = def_stmt;
         sn.isDefault = isDefault;
      }
      return &index2ssa_name.find(key)->second;
   }

   bool DumpGimpleRaw::is_PTS(unsigned int varId, const llvm::TargetLibraryInfo& TLI, bool with_all)
   {
#if HAVE_LIBBDD
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
#endif
         return false;
   }

   bool DumpGimpleRaw::is_virtual_ssa(const void* t) const
   {
      const ssa_name* ssa = reinterpret_cast<const ssa_name*>(t);
      return ssa->isVirtual;
   }

   bool DumpGimpleRaw::SSA_NAME_IS_DEFAULT_DEF(const void* t) const
   {
      const ssa_name* ssa = reinterpret_cast<const ssa_name*>(t);
      return ssa->isDefault;
   }

   const void* DumpGimpleRaw::LowerGetElementPtrOffset(const llvm::GEPOperator* gep_op, const llvm::Function* currentFunction, const void*& base_node, bool& isZero)
   {
      if(gep_op->hasAllConstantIndices())
      {
         llvm::APInt OffsetAI(DL->getPointerTypeSizeInBits(gep_op->getType()), 0);
         assert(gep_op->accumulateConstantOffset(*DL, OffsetAI));
         isZero = !OffsetAI;
         return assignCodeAuto(llvm::ConstantInt::get(gep_op->getContext(), OffsetAI));
      }
      else
      {
         llvm::APInt ConstantIndexOffset(DL->getPointerTypeSizeInBits(gep_op->getType()), 0);
         for(llvm::gep_type_iterator GTI = llvm::gep_type_begin(gep_op), GTE = llvm::gep_type_end(gep_op); GTI != GTE; ++GTI)
         {
            llvm::ConstantInt* OpC = dyn_cast<llvm::ConstantInt>(GTI.getOperand());
            if(!OpC)
            {
               if(GTI.getStructTypeOrNull())
               {
                  llvm_unreachable("unexpected condition: struct LowerGetElementPtrOffset");
                  // continue;
               }
               // For array or vector indices, scale the index by the size of the type.
               auto index = getOperand(GTI.getOperand(), currentFunction);
               auto index_type = TREE_TYPE(index);
               bool isSignedIndexType = CheckSignedTag(index_type);
               auto array_elmt_size = llvm::APInt(ConstantIndexOffset.getBitWidth(), DL->getTypeAllocSize(GTI.getIndexedType()));
               auto array_elmt_sizeCI = llvm::ConstantInt::get(gep_op->getContext(), array_elmt_size);
               auto array_elmt_sizeCI_type = array_elmt_sizeCI->getType();
               auto array_elmt_size_node = assignCodeAuto(array_elmt_sizeCI);
               if(isSignedIndexType)
               {
                  if(index2integer_cst_signed.find(array_elmt_size_node) == index2integer_cst_signed.end())
                  {
                     auto& ics_obj = index2integer_cst_signed[array_elmt_size_node];
                     auto type_operand = TREE_TYPE(array_elmt_size_node);
                     ics_obj.ic = array_elmt_size_node;
                     ics_obj.type = AddSignedTag(type_operand);
                     array_elmt_size_node = assignCode(&ics_obj, GT(INTEGER_CST_SIGNED));
                  }
                  else
                     array_elmt_size_node = &index2integer_cst_signed.find(array_elmt_size_node)->second;
               }
               auto index_times_size = build2(GT(MULT_EXPR), isSignedIndexType ? AddSignedTag(array_elmt_sizeCI_type) : array_elmt_sizeCI_type, index, array_elmt_size_node);
               if(isSignedIndexType)
                  index_times_size = build1(GT(NOP_EXPR), array_elmt_sizeCI_type, index_times_size);
               auto accu = build2(GT(POINTER_PLUS_EXPR), TREE_TYPE(base_node), base_node, index_times_size);
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
            ConstantIndexOffset += Index * llvm::APInt(ConstantIndexOffset.getBitWidth(), DL->getTypeAllocSize(GTI.getIndexedType()));
         }
         isZero = !ConstantIndexOffset;
         return assignCodeAuto(llvm::ConstantInt::get(gep_op->getContext(), ConstantIndexOffset));
      }
   }

   const void* DumpGimpleRaw::LowerGetElementPtr(const void* type, const llvm::User* gep, const llvm::Function* currentFunction)
   {
      assert(TREE_CODE(type) == GT(POINTER_TYPE));
      // auto mem_ref_type = TREE_TYPE(type);
      auto base_node = getOperand(gep->getOperand(0), currentFunction);
      const llvm::GEPOperator* gep_op = dyn_cast<llvm::GEPOperator>(gep);
      assert(gep_op);
      bool isZero = false;
      auto offset_node = LowerGetElementPtrOffset(gep_op, currentFunction, base_node, isZero);
      //      if(gep_op->isInBounds())
      //      {
      //         auto mem_ref_node = build2(GT(MEM_REF), mem_ref_type, base_node, offset_node);
      //         return build1(GT(ADDR_EXPR), type, mem_ref_node);
      //      }
      //      else
      if(isZero)
         return base_node;
      else
         return build2(GT(POINTER_PLUS_EXPR), type, base_node, offset_node);
   }

   const void* DumpGimpleRaw::getOperand(const llvm::Value* operand, const llvm::Function* currentFunction)
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
         auto def_stmt = getGimpleNop(operand, dyn_cast<llvm::Argument>(operand)->getParent());
         auto ssa = getSSA(operand, def_stmt, currentFunction, false);
         index2ssa_name.find(std::make_pair(def_stmt, false))->second.var = assignCodeAuto(operand);
         return ssa;
      }
      else if(isa<llvm::GlobalVariable>(operand))
      {
         auto type = assignCodeType(operand->getType());
         return build1(GT(ADDR_EXPR), type, assignCodeAuto(operand));
      }
      else if(isa<llvm::Function>(operand))
      {
         auto type = assignCodeType(operand->getType());
         return build1(GT(ADDR_EXPR), type, assignCodeAuto(operand));
      }
      else if(isa<llvm::ConstantExpr>(operand))
      {
         auto ce = cast<llvm::ConstantExpr>(operand);
         auto resType = assignCodeType(ce->getType());
         if(ce->getOpcode() == llvm::Instruction::GetElementPtr)
            return LowerGetElementPtr(assignCodeType(operand->getType()), ce, currentFunction);
         else
         {
            auto tec = tree_expr_code(ce);
            if(get_gimple_rhs_class(tec) == GIMPLE_TERNARY_RHS)
            {
               return build3(tec, resType, getSignedOperandIndex(ce, 0, currentFunction), getSignedOperandIndex(ce, 1, currentFunction), getSignedOperandIndex(ce, 2, currentFunction));
            }
            else if(get_gimple_rhs_class(tec) == GIMPLE_BINARY_RHS)
            {
               return build2(tec, resType, getSignedOperandIndex(ce, 0, currentFunction), getSignedOperandIndex(ce, 1, currentFunction));
            }
            else if(get_gimple_rhs_class(tec) == GIMPLE_UNARY_RHS)
            {
               assert(ce->getOpcode() != llvm::Instruction::SExt);
               return build1(tec, resType, getSignedOperandIndex(ce, 0, currentFunction));
            }
            else if(get_gimple_rhs_class(tec) == GIMPLE_SINGLE_RHS)
            {
               auto ltype = resType;
               auto rhs = getSignedOperandIndex(ce, 0, currentFunction);
               auto rtype = TREE_TYPE(rhs);
               if(ltype == rtype)
                  return rhs;
               else
                  return build1(GT(VIEW_CONVERT_EXPR), ltype, rhs);
            }
            else
               llvm_unreachable("unexpected condition");
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
            llvm_unreachable((std::string("unexpected condition: ") + std::string(ValueTyNames[operand->getValueID()])).c_str());
         }
      }
      else if(isa<llvm::ConstantDataArray>(operand) || isa<llvm::ConstantDataVector>(operand) || isa<llvm::ConstantStruct>(operand) || isa<llvm::ConstantArray>(operand))
         return assignCodeAuto(operand);
      else
      {
         operand->print(llvm::errs(), true);
         llvm::errs() << "\n";
         stream.close();
         llvm_unreachable((std::string("unexpected condition: ") + std::string(ValueTyNames[operand->getValueID()])).c_str());
      }
   }

   template <class InstructionOrConstantExpr>
   const void* DumpGimpleRaw::getSignedOperand(const InstructionOrConstantExpr* inst, const void* op, unsigned index)
   {
      auto isSignedOp = isSignedOperand(inst, index);
      if(isSignedOp)
      {
         auto type_operand = TREE_TYPE(op);
         auto tree_code_op = TREE_CODE(type_operand);
         if(tree_code_op == GT(POINTER_TYPE))
         {
            return build1(GT(NOP_EXPR), &SignedPointerTypeReference, op);
         }
         else if(TREE_CODE(op) == GT(INTEGER_CST))
         {
            const void* ics;
            if(index2integer_cst_signed.find(op) == index2integer_cst_signed.end())
            {
               auto& ics_obj = index2integer_cst_signed[op];
               ics_obj.ic = op;
               ics_obj.type = AddSignedTag(type_operand);
               ics = assignCode(&ics_obj, GT(INTEGER_CST_SIGNED));
            }
            else
               ics = &index2integer_cst_signed.find(op)->second;
            return ics;
         }
         else if(!CheckSignedTag(TREE_TYPE(op)))
            return build1(GT(NOP_EXPR), AddSignedTag(type_operand), op);
         else
            return op;
      }
      else if(isUnsignedOperand(inst, index) && (CheckSignedTag(TREE_TYPE(op))))
         return build1(GT(NOP_EXPR), NormalizeSignedTag(TREE_TYPE(op)), op);
      else
         return op;
   }
   template <class InstructionOrConstantExpr>
   const void* DumpGimpleRaw::getSignedOperandIndex(const InstructionOrConstantExpr* inst, unsigned index, const llvm::Function* currentFunction)
   {
      auto op = getOperand(inst->getOperand(index), currentFunction);
      return getSignedOperand(inst, op, index);
   }

   const void* DumpGimpleRaw::gimple_assign_rhsIndex(const void* g, unsigned index)
   {
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      auto currentFunction = inst->getFunction();
      if(isa<llvm::LoadInst>(inst))
      {
         assert(index == 0);
         const llvm::LoadInst& load = cast<const llvm::LoadInst>(*inst);
         if(uicTable.find(0) == uicTable.end())
            uicTable[0] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt32Ty(inst->getContext()), 0, false));
         const void* zero = uicTable.find(0)->second;
         auto addr = getOperand(load.getPointerOperand(), currentFunction);
         auto ty = load.getType();
         auto type = assignCodeType(ty);
         auto read_obj_size = ty->isSized() ? DL->getTypeAllocSizeInBits(ty) : 8ULL;
         auto funName = load.getFunction()->getName();
         auto demangled = getDemangled(funName);
         bool is_a_top_parameter = isa<llvm::Argument>(load.getPointerOperand()) && (funName == TopFunctionName || demangled == TopFunctionName);

         if(load.getAlignment() && read_obj_size > (8 * load.getAlignment()) && !is_a_top_parameter)
            return build1(GT(MISALIGNED_INDIRECT_REF), type, addr);
         else
            return build2(GT(MEM_REF), type, addr, zero);
      }
      else if(isa<llvm::SExtInst>(inst) && cast<const llvm::SExtInst>(*inst).getType()->isIntegerTy() && cast<const llvm::SExtInst>(*inst).getOperand(0)->getType()->isIntegerTy() &&
              cast<const llvm::SExtInst>(*inst).getOperand(0)->getType()->getIntegerBitWidth() == 1)
      {
         assert(index == 0);
         const llvm::SExtInst& sext = cast<const llvm::SExtInst>(*inst);
         auto MSB_pos = sext.getType()->getIntegerBitWidth() - 1;
         assert(MSB_pos > 0);
         if(uicTable.find(MSB_pos) == uicTable.end())
            uicTable[MSB_pos] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt32Ty(inst->getContext()), MSB_pos, false));
         const void* MSB_posNode = uicTable.find(MSB_pos)->second;
         auto type = AddSignedTag(assignCodeType(sext.getType()));
         auto casted = build1(GT(NOP_EXPR), type, getOperand(inst->getOperand(index), currentFunction));
         auto shiftedLeft = build2(GT(LSHIFT_EXPR), type, casted, MSB_posNode);
         return build2(GT(RSHIFT_EXPR), type, shiftedLeft, MSB_posNode);
      }
      else if(isa<llvm::TruncInst>(inst) && cast<const llvm::TruncInst>(*inst).getType()->isIntegerTy())
      {
         assert(index == 0);
         const llvm::TruncInst& tI = cast<const llvm::TruncInst>(*inst);
         auto bw = tI.getType()->getIntegerBitWidth();
         if(bw != 8 && bw != 16 && bw != 32 && bw != 64)
         {
            auto mask = (1ULL << bw) - 1;
            if(uicTable.find(mask) == uicTable.end())
               uicTable[mask] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt64Ty(inst->getContext()), mask, false));
            const void* maskNode = uicTable.find(mask)->second;
            auto type = assignCodeType(tI.getType());
            return build2(GT(BIT_AND_EXPR), type, getOperand(inst->getOperand(index), currentFunction), maskNode);
         }
      }
      return getSignedOperandIndex(inst, index, currentFunction);
   }

   const void* DumpGimpleRaw::boolean_type_node(const void* g)
   {
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      return assignCodeType(inst->getType());
   }

   const char* DumpGimpleRaw::gimple_asm_string(const void* g)
   {
      const llvm::CallInst* ci = reinterpret_cast<const llvm::CallInst*>(g);
      auto calledFun = ci->getCalledValue();
      assert(isa<llvm::InlineAsm>(calledFun));
      auto ia = cast<llvm::InlineAsm>(calledFun);
      return ia->getAsmString().c_str();
   }

   const void* DumpGimpleRaw::gimple_label_label(const void* g)
   {
      const gimple_label* gl = reinterpret_cast<const gimple_label*>(g);
      return gl->op;
   }

   const void* DumpGimpleRaw::gimple_phi_virtual_result(const void* g) const
   {
      const gimple_phi_virtual* gpv = reinterpret_cast<const gimple_phi_virtual*>(g);
      assert(HAS_CODE(gpv->res));
      return gpv->res;
   }
   unsigned int DumpGimpleRaw::gimple_phi_num_args(const void* g) const
   {
      const llvm::PHINode* phi = reinterpret_cast<const llvm::PHINode*>(g);
      return phi->getNumIncomingValues();
   }

   unsigned int DumpGimpleRaw::gimple_phi_virtual_num_args(const void* g) const
   {
      const gimple_phi_virtual* gpv = reinterpret_cast<const gimple_phi_virtual*>(g);
      return gpv->def_edfe_pairs.size();
   }

   const void* DumpGimpleRaw::gimple_phi_arg_def(const void* g, unsigned int index)
   {
      const llvm::PHINode* phi = reinterpret_cast<const llvm::PHINode*>(g);
      return getSignedOperand(phi, getOperand(phi->getIncomingValue(index), phi->getFunction()), 0);
   }

   const void* DumpGimpleRaw::gimple_phi_virtual_arg_def(const void* g, unsigned int index)
   {
      const gimple_phi_virtual* gpv = reinterpret_cast<const gimple_phi_virtual*>(g);
      assert(index < gpv->def_edfe_pairs.size());
      return gpv->def_edfe_pairs.at(index).first;
   }

   int DumpGimpleRaw::gimple_phi_arg_edgeBBindex(const void* g, unsigned int index)
   {
      const llvm::PHINode* phi = reinterpret_cast<const llvm::PHINode*>(g);
      auto BB = phi->getIncomingBlock(index);
      return getBB_index(BB);
   }

   int DumpGimpleRaw::gimple_phi_virtual_arg_edgeBBindex(const void* g, unsigned int index)
   {
      const gimple_phi_virtual* gpv = reinterpret_cast<const gimple_phi_virtual*>(g);
      assert(index < gpv->def_edfe_pairs.size());
      return gpv->def_edfe_pairs.at(index).second;
   }

   const void* DumpGimpleRaw::gimple_call_fn(const void* g)
   {
      const llvm::CallInst* ci = reinterpret_cast<const llvm::CallInst*>(g);
      auto calledFun = ci->getCalledValue();
      if(isa<llvm::Function>(calledFun))
      {
         auto type = assignCodeType(calledFun->getType());
         return build1(GT(ADDR_EXPR), type, assignCodeAuto(calledFun));
      }
      else if(isa<llvm::ConstantExpr>(calledFun))
      {
         auto type = assignCodeType(calledFun->getType());
         auto ce = cast<llvm::ConstantExpr>(calledFun);
         if(ce->getOpcode() == llvm::Instruction::BitCast)
         {
            auto op = ce->getOperand(0);
            if(isa<llvm::Function>(op))
               return build1(GT(ADDR_EXPR), type, assignCodeAuto(op));
         }
         return getOperand(calledFun, ci->getFunction());
      }
      else
      {
         return getOperand(calledFun, ci->getFunction());
      }
   }

   unsigned int DumpGimpleRaw::gimple_call_num_args(const void* g)
   {
      const llvm::CallInst* ci = reinterpret_cast<const llvm::CallInst*>(g);
      return ci->getNumArgOperands();
   }

   const void* DumpGimpleRaw::gimple_call_arg(const void* g, unsigned int arg_index)
   {
      const llvm::CallInst* ci = reinterpret_cast<const llvm::CallInst*>(g);
      return getOperand(ci->getArgOperand(arg_index), ci->getFunction());
   }

   const void* DumpGimpleRaw::gimple_return_retval(const void* g)
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

   const void* DumpGimpleRaw::gimple_switch_index(const void* g)
   {
      const llvm::SwitchInst* si = reinterpret_cast<const llvm::SwitchInst*>(g);
      return getSignedOperand(si, getOperand(si->getCondition(), si->getFunction()), 0);
   }

   const void* DumpGimpleRaw::gimple_switch_vec(const void* g)
   {
      if(memoization_tree_vec.find(g) != memoization_tree_vec.end())
         return memoization_tree_vec.find(g)->second;
      const llvm::SwitchInst* si = reinterpret_cast<const llvm::SwitchInst*>(g);
      tree_vec& tree_vec_element = index2tree_vec[last_used_index + 1];
      auto res = &tree_vec_element;
      assignCode(res, GT(TREE_VEC));
      queue(res);
      memoization_tree_list[g] = res;
      for(auto case_expr : si->cases())
      {
         const void* op1 = assignCodeAuto(case_expr.getCaseValue());
         const void* op2 = nullptr;
         auto BB = case_expr.getCaseSuccessor();
         assert(index2label_decl.find(BB) != index2label_decl.end());
         const void* op3 = &index2label_decl.find(BB)->second;
         tree_vec_element.data.push_back(build3(GT(CASE_LABEL_EXPR), llvm::Type::getVoidTy(si->getFunction()->getContext()), op1, op2, op3));
      }
      assert(index2label_decl.find(si->getDefaultDest()) != index2label_decl.end());
      tree_vec_element.data.push_back(build3(GT(CASE_LABEL_EXPR), llvm::Type::getVoidTy(si->getFunction()->getContext()), nullptr, nullptr, &index2label_decl.find(si->getDefaultDest())->second));
      return res;
   }

   const void* DumpGimpleRaw::build_custom_function_call_expr(const void* g)
   {
      assert(index2call_expr.find(g) == index2call_expr.end());
      auto& ce = index2call_expr[g];
      auto res = assignCode(&ce, GT(CALL_EXPR));
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);

      ce.type = assignCodeType(inst->getType());
      ce.fn = gimple_call_fn(g);
      unsigned int arg_index;
      for(arg_index = 0; arg_index < gimple_call_num_args(g); arg_index++)
         ce.args.push_back(gimple_call_arg(g, arg_index));
      return res;
   }

   const void* DumpGimpleRaw::call_expr_fn(const void* t)
   {
      const call_expr* ce = reinterpret_cast<const call_expr*>(t);
      assert(HAS_CODE(ce->fn));
      return ce->fn;
   }

   unsigned int DumpGimpleRaw::call_expr_num_args(const void* t)
   {
      const call_expr* ce = reinterpret_cast<const call_expr*>(t);
      return ce->args.size();
   }

   const void* DumpGimpleRaw::call_expr_arg(const void* t, unsigned int arg_index)
   {
      const call_expr* ce = reinterpret_cast<const call_expr*>(t);
      assert(HAS_CODE(ce->args.at(arg_index)));
      return ce->args.at(arg_index);
   }

   const void* DumpGimpleRaw::getVirtualGimplePhi(llvm::MemoryPhi* mp, const llvm::MemorySSA& MSSA)
   {
      llvm::BasicBlock* BB = mp->getBlock();
      if(index2gimple_phi_virtual.find(BB) == index2gimple_phi_virtual.end())
      {
         auto& gpv = index2gimple_phi_virtual[BB];
         assignCode(&gpv, GT(GIMPLE_PHI_VIRTUAL));
         const llvm::Function* currentFunction = BB->getParent();
         gpv.scpe = assignCodeAuto(currentFunction);
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
               gpv.def_edfe_pairs.push_back(std::make_pair(getSSA(val, def_stmt, currentFunction, isDefault), getBB_index(mp->getIncomingBlock(index))));
            }
            else
            {
               assert(val->getValueID() == llvm::Value::MemoryPhiVal);
               gpv.def_edfe_pairs.push_back(std::make_pair(gimple_phi_virtual_result(getVirtualGimplePhi(dyn_cast<llvm::MemoryPhi>(val), MSSA)), getBB_index(mp->getIncomingBlock(index))));
            }
         }
      }
      return &index2gimple_phi_virtual.find(BB)->second;
   }

   const void* DumpGimpleRaw::build3(DumpGimpleRaw::tree_codes tc, const void* type, const void* op1, const void* op2, const void* op3)
   {
      index2tree_expr.push_front(tree_expr());
      auto& te = index2tree_expr.front();
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

   const void* DumpGimpleRaw::DECL_CONTEXT(const void* t)
   {
      if(TREE_CODE(t) == GT(TRANSLATION_UNIT_DECL))
         return nullptr;
      else if(TREE_CODE(t) == GT(ALLOCAVAR_DECL))
      {
         const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
         return assignCodeAuto(av->alloc_inst->getFunction());
      }
      else if(TREE_CODE(t) == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return DECL_CONTEXT(ov->orig);
      }
      else if(TREE_CODE(t) == GT(FIELD_DECL))
      {
         const field_decl* ty = reinterpret_cast<const field_decl*>(t);
         assert(HAS_CODE(ty->scpe));
         return ty->scpe;
      }
      else if(TREE_CODE(t) == GT(LABEL_DECL))
      {
         const label_decl* ty = reinterpret_cast<const label_decl*>(t);
         assert(HAS_CODE(ty->scpe));
         return ty->scpe;
      }
      else if(TREE_CODE(t) == GT(PARM_DECL))
      {
         const llvm::Argument* llvm_obj = reinterpret_cast<const llvm::Argument*>(t);
         return assignCodeAuto(llvm_obj->getParent());
      }
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      const llvm::Module* scpe = llvm_obj->getParent();
      assert(scpe);
      return assignCode(scpe, GT(TRANSLATION_UNIT_DECL));
   }

   int DumpGimpleRaw::DECL_UID(const void* t) const
   {
      assert(llvm2index.find(t) != llvm2index.end());
      return llvm2index.find(t)->second;
   }

   bool DumpGimpleRaw::DECL_C_BIT_FIELD(const void* t) const
   {
      return false;
   }

   bool DumpGimpleRaw::DECL_EXTERNAL(const void* t) const
   {
      if(TREE_CODE(t) == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         if(TREE_CODE(ov->orig) == GT(VAR_DECL))
            return DECL_EXTERNAL(ov->orig);
         else
            return false;
      }
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      if(isa<llvm::GlobalVariable>(llvm_obj))
         return llvm_obj->hasExternalLinkage() && !cast<llvm::GlobalVariable>(llvm_obj)->hasInitializer();
      else if(isa<llvm::Function>(llvm_obj))
         return cast<llvm::Function>(llvm_obj)->getBasicBlockList().empty();
      else
         llvm_unreachable("unexpected case");
   }

   bool DumpGimpleRaw::TREE_PUBLIC(const void* t) const
   {
      if(TREE_CODE(t) == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         if(TREE_CODE(ov->orig) == GT(VAR_DECL))
            return TREE_PUBLIC(ov->orig);
         else
            return false;
      }
      const llvm::GlobalValue* llvm_obj = reinterpret_cast<const llvm::GlobalValue*>(t);
      return llvm_obj->hasDefaultVisibility() && !llvm_obj->hasInternalLinkage() && llvm_obj->getLinkage() != llvm::GlobalValue::PrivateLinkage;
   }

   bool DumpGimpleRaw::TREE_STATIC(const void* t) const
   {
      if(TREE_CODE(t) == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         if(TREE_CODE(ov->orig) == GT(VAR_DECL))
            return TREE_STATIC(ov->orig);
         else
            return false;
      }
      const llvm::GlobalValue* llvm_obj = reinterpret_cast<const llvm::GlobalValue*>(t);
      auto lt = llvm_obj->getLinkage();
      return lt == llvm::GlobalValue::InternalLinkage || lt == llvm::GlobalValue::PrivateLinkage;
   }

   bool DumpGimpleRaw::is_builtin_fn(const void* t) const
   {
      assert(TREE_CODE(t) == GT(FUNCTION_DECL));
      const llvm::Function* fd = reinterpret_cast<const llvm::Function*>(t);
      if(fd->hasName())
      {
         std::string declname;
         if(fd->isIntrinsic())
            declname = getIntrinsicName(fd);
         else
            declname = std::string(fd->getName());
         if(builtinsNames.find(std::string("__builtin_") + declname) != builtinsNames.end() || builtinsNames.find(declname) != builtinsNames.end())
            return true;
      }
      return false;
   }

   const void* DumpGimpleRaw::DECL_ARG_TYPE(const void* t)
   {
      return TREE_TYPE(t);
   }

   const void* DumpGimpleRaw::DECL_INITIAL(const void* t)
   {
      if(TREE_CODE(t) == GT(FIELD_DECL))
         return nullptr;
      if(TREE_CODE(t) == GT(ALLOCAVAR_DECL))
         return nullptr;
      if(TREE_CODE(t) == GT(ORIGVAR_DECL))
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

   const void* DumpGimpleRaw::DECL_SIZE(const void* t)
   {
      if(TREE_CODE(t) == GT(ALLOCAVAR_DECL))
      {
         const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
         auto arraySize = av->alloc_inst->getArraySize();
         if(isa<llvm::ConstantInt>(arraySize) && cast<llvm::ConstantInt>(arraySize)->getZExtValue() == 1)
            return TYPE_SIZE(TREE_TYPE(t));
         else
         {
            llvm::errs() << "Size type " << ValueTyNames[arraySize->getValueID()] << "\n";
            stream.close();
            llvm_unreachable("Plugin error");
         }
      }
      else if(TREE_CODE(t) == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return DECL_SIZE(ov->orig);
      }
      else
         return TYPE_SIZE(TREE_TYPE(t));
   }

   int DumpGimpleRaw::DECL_ALIGN(const void* t)
   {
      if(TREE_CODE(t) == GT(VAR_DECL))
      {
         const llvm::GlobalVariable* llvm_obj = reinterpret_cast<const llvm::GlobalVariable*>(t);
         return std::max(8u, 8 * llvm_obj->getAlignment());
      }
      else if(TREE_CODE(t) == GT(ALLOCAVAR_DECL))
      {
         const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
         auto algn = av->alloc_inst->getAlignment();
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
         return TYPE_ALIGN(TREE_TYPE(t));
   }

   bool DumpGimpleRaw::DECL_PACKED(const void* t) const
   {
      assert(TREE_CODE(t) == GT(FIELD_DECL));
      const field_decl* ty = reinterpret_cast<const field_decl*>(t);
      const llvm::StructType* st = reinterpret_cast<const llvm::StructType*>(ty->scpe);
      return st->isPacked();
   }

   bool DumpGimpleRaw::DECL_FIELD_OFFSET(const void* t) const
   {
      assert(TREE_CODE(t) == GT(FIELD_DECL));
      const field_decl* ty = reinterpret_cast<const field_decl*>(t);
      return ty->bpos != nullptr;
   }

   const void* DumpGimpleRaw::bit_position(const void* t)
   {
      assert(TREE_CODE(t) == GT(FIELD_DECL));
      const field_decl* ty = reinterpret_cast<const field_decl*>(t);
      return ty->bpos;
   }

   int DumpGimpleRaw::TREE_USED(const void* t) const
   {
      assert(TREE_CODE(t) == GT(ALLOCAVAR_DECL) || TREE_CODE(t) == GT(VAR_DECL) || TREE_CODE(t) == GT(PARM_DECL) || TREE_CODE(t) == GT(ORIGVAR_DECL));
      if(TREE_CODE(t) == GT(PARM_DECL))
         return 1;
      else if(TREE_CODE(t) == GT(ALLOCAVAR_DECL))
         return 1;
      else if(TREE_CODE(t) == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return TREE_USED(ov->orig);
      }
      const llvm::Constant* llvm_obj = reinterpret_cast<const llvm::Constant*>(t);
      if(llvm_obj->isConstantUsed())
         return 1;
      else
         return 0;
   }

   bool DumpGimpleRaw::DECL_REGISTER(const void*) const
   {
      return false;
   }

   bool DumpGimpleRaw::TREE_READONLY(const void* t) const
   {
      if(TREE_CODE(t) == GT(FIELD_DECL))
         return false;
      else if(TREE_CODE(t) == GT(ALLOCAVAR_DECL))
         return false;
      else if(TREE_CODE(t) == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return TREE_READONLY(ov->orig);
      }
      else if(TREE_CODE(t) == GT(RESULT_DECL))
         return false;
      const llvm::GlobalVariable* llvm_obj = reinterpret_cast<const llvm::GlobalVariable*>(t);
      return llvm_obj->isConstant();
   }
   bool DumpGimpleRaw::TREE_ADDRESSABLE(const void* t) const
   {
      assert(TREE_CODE(t) == GT(ALLOCAVAR_DECL));
      const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
      return !av->addr;
   }
   const void* DumpGimpleRaw::TREE_OPERAND(const void* t, unsigned index)
   {
      const tree_expr* te = reinterpret_cast<const tree_expr*>(t);
      if(index == 0)
         return te->op1;
      else if(index == 1)
         return te->op2;
      else if(index == 2)
         return te->op3;
      else
         llvm_unreachable("unexpected condition");
   }

   int64_t DumpGimpleRaw::TREE_INT_CST_LOW(const void* t)
   {
      const llvm::ConstantData* cd;
      bool isSigned = TREE_CODE(t) == GT(INTEGER_CST_SIGNED);
      if(isSigned)
         cd = reinterpret_cast<const llvm::ConstantData*>(reinterpret_cast<const integer_cst_signed*>(t)->ic);
      else
         cd = reinterpret_cast<const llvm::ConstantData*>(t);
      if(isa<llvm::ConstantPointerNull>(cd))
         return 0;
      const llvm::ConstantInt* llvm_obj = cast<const llvm::ConstantInt>(cd);
      const llvm::APInt& val = llvm_obj->getValue();
      assert(val.getNumWords() == 1);
      if(isSigned || CheckSignedTag(TREE_TYPE(t)))
         return val.getSExtValue();
      else
         return static_cast<int64_t>(val.getZExtValue());
   }

   const void* DumpGimpleRaw::assignCodeType(const llvm::Type* ty)
   {
      if(reinterpret_cast<const unsigned int*>(ty) == &SignedPointerTypeReference)
         return ty;
      auto typeId = NormalizeSignedTag(ty)->getTypeID();
      switch(typeId)
      {
         case llvm::Type::VoidTyID:
            return assignCode(ty, GT(VOID_TYPE));
         case llvm::Type::HalfTyID:
         case llvm::Type::FloatTyID:
         case llvm::Type::DoubleTyID:
         case llvm::Type::X86_FP80TyID:
         case llvm::Type::FP128TyID:
         case llvm::Type::PPC_FP128TyID:
            return assignCode(ty, GT(REAL_TYPE));
         case llvm::Type::LabelTyID:
            llvm::errs() << "assignCodeType kind not supported: LabelTyID\n";
            stream.close();
            llvm_unreachable("Plugin Error");
         case llvm::Type::MetadataTyID:
            return assignCode(ty, GT(ANNOTATE_EXPR));
         case llvm::Type::X86_MMXTyID:
            llvm::errs() << "assignCodeType kind not supported: X86_MMXTyID\n";
            stream.close();
            llvm_unreachable("Plugin Error");
         case llvm::Type::TokenTyID:
            llvm::errs() << "assignCodeType kind not supported: TokenTyID\n";
            stream.close();
            llvm_unreachable("Plugin Error");
         case llvm::Type::IntegerTyID:
            return assignCode(ty, GT(INTEGER_TYPE));
         case llvm::Type::FunctionTyID:
            return assignCode(ty, GT(FUNCTION_TYPE));
         case llvm::Type::StructTyID:
            return assignCode(ty, GT(RECORD_TYPE));
         case llvm::Type::ArrayTyID:
            return assignCode(ty, GT(ARRAY_TYPE));
         case llvm::Type::PointerTyID:
            return assignCode(ty, GT(POINTER_TYPE));
         case llvm::Type::VectorTyID:
            return assignCode(ty, GT(VECTOR_TYPE));
         default:
         {
            llvm::errs() << "type id not managed\n";
            stream.close();
            llvm_unreachable("Plugin error");
         }
      }
   }

   const void* DumpGimpleRaw::TREE_TYPE(const void* t)
   {
      tree_codes code = TREE_CODE(t);
      if(code == GT(TRANSLATION_UNIT_DECL))
         return nullptr;
      else if(code == GT(ALLOCAVAR_DECL))
      {
         const alloca_var* av = reinterpret_cast<const alloca_var*>(t);
         auto allocType = av->alloc_inst->getAllocatedType();
         return assignCodeType(allocType);
      }
      else if(code == GT(ORIGVAR_DECL))
      {
         const orig_var* ov = reinterpret_cast<const orig_var*>(t);
         return TREE_TYPE(ov->orig);
      }
      else if(code == GT(FIELD_DECL))
      {
         const field_decl* ty = reinterpret_cast<const field_decl*>(t);
         assert(HAS_CODE(ty->type));
         return ty->type;
      }
      else if(code == GT(LABEL_DECL))
      {
         const label_decl* ty = reinterpret_cast<const label_decl*>(t);
         assert(HAS_CODE(ty->type));
         return ty->type;
      }
      else if(code == GT(GIMPLE_NOP))
      {
         llvm_unreachable("unexpected");
         // const gimple_nop* gn = reinterpret_cast<const gimple_nop*>(t);
         // return TREE_TYPE(gn->parm_decl);
      }
      else if(code == GT(SSA_NAME))
      {
         const ssa_name* ssa = reinterpret_cast<const ssa_name*>(t);
         assert(HAS_CODE(ssa->type));
         return ssa->type;
      }
      else if(code == GT(CALL_EXPR))
      {
         const call_expr* ce = reinterpret_cast<const call_expr*>(t);
         assert(HAS_CODE(ce->type));
         return ce->type;
      }
      else if(code == GT(INTEGER_CST_SIGNED))
      {
         const integer_cst_signed* ics = reinterpret_cast<const integer_cst_signed*>(t);
         assert(HAS_CODE(ics->type));
         return ics->type;
      }
      tree_codes_class code_class = TREE_CODE_CLASS(code);
      if(code_class == tcc_type)
      {
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
               llvm::errs() << "TREE_TYPE kind not supported: type of type: " << GET_TREE_CODE_NAME(TREE_CODE(t)) << ":" << typeId << "\n";
               stream.close();
               llvm_unreachable("Plugin Error");

            case llvm::Type::FunctionTyID:
               return assignCodeType(cast<llvm::FunctionType>(ty)->getReturnType());
            case llvm::Type::ArrayTyID:
               return assignCodeType(cast<llvm::ArrayType>(ty)->getElementType());
            case llvm::Type::PointerTyID:
               return assignCodeType(cast<llvm::PointerType>(ty)->getElementType());
            case llvm::Type::VectorTyID:
               return assignCodeType(cast<llvm::VectorType>(ty)->getElementType());
            default:
            {
               llvm::errs() << "type id not managed\n";
               stream.close();
               llvm_unreachable("Plugin error");
            }
         }
      }
      else if(IS_EXPR_CODE_CLASS(code_class))
      {
         const tree_expr* te = reinterpret_cast<const tree_expr*>(t);
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

   bool DumpGimpleRaw::POINTER_TYPE_P(const void* t) const
   {
      return (TREE_CODE(t) == GT(POINTER_TYPE) || TREE_CODE(t) == GT(REFERENCE_TYPE));
   }

   bool DumpGimpleRaw::TYPE_UNSIGNED(const void* t) const
   {
      tree_codes code = TREE_CODE(t);
      if(code == GT(SIGNEDPOINTERTYPE))
         return false;
      if(code == GT(COMPLEX_TYPE))
         llvm_unreachable("unexpected call to TYPE_UNSIGNED");
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      assert(NormalizeSignedTag(ty)->isIntegerTy());
      return !CheckSignedTag(ty);
   }

   bool DumpGimpleRaw::COMPLEX_FLOAT_TYPE_P(const void*) const
   {
      llvm_unreachable("unexpected call to COMPLEX_FLOAT_TYPE_P");
   }

   bool DumpGimpleRaw::TYPE_SATURATING(const void*) const
   {
      llvm_unreachable("unexpected call to COMPLEX_FLOAT_TYPE_P");
   }

   int DumpGimpleRaw::TYPE_PRECISION(const void* t) const
   {
      if(TREE_CODE(t) == GT(SIGNEDPOINTERTYPE))
         return 32;
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      ty = NormalizeSignedTag(ty);
      auto typeId = ty->getTypeID();
      switch(typeId)
      {
         case llvm::Type::IntegerTyID:
         {
            llvm::Type* casted_ty = const_cast<llvm::Type*>(ty);
            return DL->getTypeAllocSizeInBits(casted_ty);
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
         case llvm::Type::VectorTyID:
         default:
            llvm::errs() << "TYPE_PRECISION kind not supported\n";
            llvm_unreachable("Plugin Error");
      }
   }

   const void* DumpGimpleRaw::TYPE_MIN_VALUE(const void* t)
   {
      const llvm::Type* Cty = reinterpret_cast<const llvm::Type*>(t);
      bool isSigned = TREE_CODE(t) == GT(SIGNEDPOINTERTYPE) || CheckSignedTag(Cty);
      llvm::Type* ty = const_cast<llvm::Type*>(NormalizeSignedTag(Cty));
      auto obj_size = TREE_CODE(t) == GT(SIGNEDPOINTERTYPE) ? 32 : DL->getTypeAllocSizeInBits(ty);
      uint64_t val;
      if(isSigned)
      {
         val = -(1ULL << (obj_size - 1));
      }
      else
         val = 0;
      auto context = TREE_CODE(t) == GT(SIGNEDPOINTERTYPE) ? moduleContext : &ty->getContext();
      if(uicTable.find(val) == uicTable.end())
         uicTable[val] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), val, false));
      return uicTable.find(val)->second;
   }

   const void* DumpGimpleRaw::TYPE_MAX_VALUE(const void* t)
   {
      const llvm::Type* Cty = reinterpret_cast<const llvm::Type*>(t);
      bool isSigned = TREE_CODE(t) == GT(SIGNEDPOINTERTYPE) || CheckSignedTag(Cty);
      llvm::Type* ty = const_cast<llvm::Type*>(NormalizeSignedTag(Cty));
      auto obj_size = TREE_CODE(t) == GT(SIGNEDPOINTERTYPE) ? 32 : DL->getTypeAllocSizeInBits(ty);
      auto maxvalue = llvm::APInt::getMaxValue(std::max((isSigned ? obj_size - 1ULL : obj_size), 1ULL)).getZExtValue();
      if(maxValueITtable.find(t) != maxValueITtable.end())
         maxvalue = maxValueITtable.find(t)->second;
      auto context = TREE_CODE(t) == GT(SIGNEDPOINTERTYPE) ? moduleContext : &ty->getContext();
      if(uicTable.find(maxvalue) == uicTable.end())
         uicTable[maxvalue] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), maxvalue, false));
      return uicTable.find(maxvalue)->second;
   }

   const void* DumpGimpleRaw::TYPE_VALUES(const void*)
   {
      llvm_unreachable("unexpected call to TYPE_VALUES");
   }

   const void* DumpGimpleRaw::TYPE_NAME(const void* t)
   {
      if(TREE_CODE(t) == GT(SIGNEDPOINTERTYPE))
         return nullptr;
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      ty = NormalizeSignedTag(ty);
      if(ty->isStructTy())
      {
         auto st = cast<llvm::StructType>(ty);
         if(st->hasName())
         {
            std::string declname = st->getName();
            if(identifierTable.find(declname) == identifierTable.end())
               identifierTable.insert(declname);
            const void* dn = identifierTable.find(declname)->c_str();
            return assignCode(dn, GT(IDENTIFIER_NODE));
         }
      }
      return nullptr; /// TBF
   }

   const void* DumpGimpleRaw::TYPE_SIZE(const void* t)
   {
      const llvm::Type* Cty = reinterpret_cast<const llvm::Type*>(t);
      llvm::Type* ty = const_cast<llvm::Type*>(NormalizeSignedTag(Cty));
      if(TREE_CODE(t) == GT(SIGNEDPOINTERTYPE))
      {
         auto obj_size = 32u;
         if(uicTable.find(obj_size) == uicTable.end())
            uicTable[obj_size] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt32Ty(ty->getContext()), obj_size, false));
         return uicTable.find(obj_size)->second;
      }
      else if(ty->isFunctionTy())
      {
         auto obj_size = 8u;
         if(uicTable.find(obj_size) == uicTable.end())
            uicTable[obj_size] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt32Ty(ty->getContext()), obj_size, false));
         return uicTable.find(obj_size)->second;
      }
      else if(ty->isVoidTy())
         return nullptr;
      else
      {
         auto obj_size = ty->isSized() ? DL->getTypeAllocSizeInBits(ty) : 8ULL;
         if(uicTable.find(obj_size) == uicTable.end())
            uicTable[obj_size] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt32Ty(ty->getContext()), obj_size, false));
         return uicTable.find(obj_size)->second;
      }
   }

   const void* DumpGimpleRaw::TYPE_CONTEXT(const void*)
   {
      return nullptr; /// TBF
   }

   bool DumpGimpleRaw::TYPE_PACKED(const void* t) const
   {
      if(TREE_CODE(t) == GT(SIGNEDPOINTERTYPE))
         return false;
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      ty = NormalizeSignedTag(ty);
      if(ty->isStructTy())
         return cast<llvm::StructType>(ty)->isPacked();
      else
         return false;
   }

   int DumpGimpleRaw::TYPE_ALIGN(const void* t) const
   {
      if(TREE_CODE(t) == GT(SIGNEDPOINTERTYPE) || TREE_CODE(t) == GT(FUNCTION_TYPE) || TREE_CODE(t) == GT(VOID_TYPE))
         return 8;
      const llvm::Type* Cty = reinterpret_cast<const llvm::Type*>(t);
      llvm::Type* ty = const_cast<llvm::Type*>(NormalizeSignedTag(Cty));
      return std::max(8u, 8 * DL->getABITypeAlignment(ty));
   }

   const void* DumpGimpleRaw::TYPE_ARG_TYPES(const void* t)
   {
      assert(CheckSignedTag(reinterpret_cast<const llvm::Type*>(t)) == 0);
      assert(isa<llvm::FunctionType>(reinterpret_cast<const llvm::Type*>(t)));
      const llvm::FunctionType* llvm_obj = reinterpret_cast<const llvm::FunctionType*>(t);
      if(llvm_obj->params().empty())
         return nullptr;
      if(memoization_tree_list.find(t) != memoization_tree_list.end())
         return memoization_tree_list.find(t)->second;
      bool is_first_element = true;
      void* res = nullptr;
      void* cur = nullptr;
      for(const auto& par : llvm_obj->params())
      {
         tree_list& tree_element = index2tree_list[last_used_index + 1];
         assignCode(&tree_element, GT(TREE_LIST));
         unsigned int index = queue(&tree_element);
         assignCodeType(par);
         tree_element.valu = par;
         if(is_first_element)
         {
            is_first_element = false;
            res = &tree_element;
         }
         else
            reinterpret_cast<tree_list*>(cur)->chan = index;
         cur = &tree_element;
      }
      memoization_tree_list[t] = res;
      return res;
   }

   const void* DumpGimpleRaw::TYPE_DOMAIN(const void* t)
   {
      assert(TREE_CODE(t) == GT(ARRAY_TYPE));
      const llvm::ArrayType* at = reinterpret_cast<const llvm::ArrayType*>(t);
      uint64_t nelements = at->getNumElements();
      /// create the context
      if(ArraysContexts.find(at) == ArraysContexts.end())
         ArraysContexts[at] = new llvm::LLVMContext;
      llvm::Type* IT = llvm::Type::getInt32Ty(*ArraysContexts.find(at)->second);
      assignCodeType(IT);
      maxValueITtable[IT] = nelements - 1;
      return IT;
   }

   bool DumpGimpleRaw::stdarg_p(const void* t) const
   {
      assert(CheckSignedTag(reinterpret_cast<const llvm::Type*>(t)) == 0);
      assert(isa<llvm::FunctionType>(reinterpret_cast<const llvm::Type*>(t)));
      const llvm::FunctionType* llvm_obj = reinterpret_cast<const llvm::FunctionType*>(t);
      return llvm_obj->isVarArg();
   }

   llvm::ArrayRef<llvm::Type*> DumpGimpleRaw::TYPE_FIELDS(const void* t)
   {
      assert(CheckSignedTag(reinterpret_cast<const llvm::Type*>(t)) == 0);
      assert(isa<llvm::StructType>(reinterpret_cast<const llvm::Type*>(t)));
      return reinterpret_cast<const llvm::StructType*>(t)->elements();
   }

   const void* DumpGimpleRaw::GET_FIELD_DECL(const void* t, unsigned int pos, const void* scpe)
   {
      const llvm::StructType* scty = reinterpret_cast<const llvm::StructType*>(scpe);
      if(index2field_decl.find(std::make_pair(scpe, pos)) == index2field_decl.end())
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "fd%d", pos);
         std::string fdName = buffer;
         if(identifierTable.find(fdName) == identifierTable.end())
            identifierTable.insert(fdName);
         index2field_decl[std::make_pair(scpe, pos)].name = assignCode(identifierTable.find(fdName)->c_str(), GT(IDENTIFIER_NODE));
         assert(CheckSignedTag(reinterpret_cast<const llvm::Type*>(t)) == 0);
         index2field_decl[std::make_pair(scpe, pos)].type = assignCodeType(scty->getElementType(pos));
         index2field_decl[std::make_pair(scpe, pos)].scpe = assignCodeAuto(scpe);
         index2field_decl[std::make_pair(scpe, pos)].size = TYPE_SIZE(t);
         index2field_decl[std::make_pair(scpe, pos)].algn = TYPE_ALIGN(t);
         uint64_t offset = DL->getStructLayout(const_cast<llvm::StructType*>(scty))->getElementOffsetInBits(pos);
         if(uicTable.find(offset) == uicTable.end())
            uicTable[offset] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt32Ty(scty->getContext()), offset, false));
         index2field_decl[std::make_pair(scpe, pos)].bpos = uicTable.find(offset)->second;
      }
      return assignCode(&index2field_decl.find(std::make_pair(scpe, pos))->second, GT(FIELD_DECL));
   }

   const void* DumpGimpleRaw::GET_METHOD_TYPE(const llvm::Type*, unsigned int, const void*)
   {
      llvm_unreachable("unexpected condition");
   }

   const void* DumpGimpleRaw::TYPE_METHOD_BASETYPE(const void*)
   {
      llvm_unreachable("unexpected condition");
   }

   const std::list<const void*> DumpGimpleRaw::DECL_ARGUMENTS(const void* t)
   {
      const llvm::Function* fd = reinterpret_cast<const llvm::Function*>(t);
      bool nameAreKnown = false;
      if(fd->hasName() && fun2params->find(fd->getName()) != fun2params->end() && fun2params->find(fd->getName())->second.size() == fd->arg_size())
      {
         nameAreKnown = true;
      }

      std::list<const void*> res;
      unsigned int par_index = 0;
      for(const auto& par : fd->args())
      {
         res.push_back(assignCodeAuto(&par));
         if(nameAreKnown)
            argNameTable[&par] = fun2params->find(fd->getName())->second[par_index];
         ++par_index;
      }
      return res;
   }

   const void* DumpGimpleRaw::getStatement_list(const void* t)
   {
      const llvm::Function* fd = reinterpret_cast<const llvm::Function*>(t);
      return assignCode(&fd->getBasicBlockList(), GT(STATEMENT_LIST));
   }

   const void* DumpGimpleRaw::getGimpleScpe(const void* g)
   {
      if(TREE_CODE(g) == GT(GIMPLE_NOP))
      {
         const gimple_nop* gn = reinterpret_cast<const gimple_nop*>(g);
         assert(HAS_CODE(gn));
         return gn->scpe;
      }
      else if(TREE_CODE(g) == GT(GIMPLE_PHI_VIRTUAL))
      {
         const gimple_phi_virtual* gpv = reinterpret_cast<const gimple_phi_virtual*>(g);
         assert(HAS_CODE(gpv->scpe));
         return gpv->scpe;
      }
      else if(TREE_CODE(g) == GT(GIMPLE_LABEL))
      {
         const gimple_label* gl = reinterpret_cast<const gimple_label*>(g);
         assert(HAS_CODE(gl->scpe));
         return gl->scpe;
      }
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      return assignCodeAuto(inst->getFunction());
   }

   int DumpGimpleRaw::getGimple_bb_index(const void* g)
   {
      if(TREE_CODE(g) == GT(GIMPLE_NOP))
         return 0;
      if(TREE_CODE(g) == GT(GIMPLE_PHI_VIRTUAL))
      {
         const gimple_phi_virtual* gpv = reinterpret_cast<const gimple_phi_virtual*>(g);
         return gpv->bb_index;
      }
      if(TREE_CODE(g) == GT(GIMPLE_LABEL))
      {
         const gimple_label* gl = reinterpret_cast<const gimple_label*>(g);
         return gl->bb_index;
      }
      const llvm::Instruction* inst = reinterpret_cast<const llvm::Instruction*>(g);
      const llvm::BasicBlock* BB = inst->getParent();
      return getBB_index(BB);
   }

   bool DumpGimpleRaw::gimple_has_mem_ops(const void* g) const
   {
      if(TREE_CODE(g) == GT(GIMPLE_NOP))
         return false;
      if(TREE_CODE(g) == GT(GIMPLE_PHI_VIRTUAL))
         return false;
      if(TREE_CODE(g) == GT(GIMPLE_LABEL))
         return false;
      llvm::Instruction* inst = const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(g));
      llvm::Function* currentFunction = inst->getFunction();
      auto& MSSA = modulePass->getAnalysis<llvm::MemorySSAWrapperPass>(*currentFunction).getMSSA();
      return MSSA.getMemoryAccess(inst);
   }

   const void* DumpGimpleRaw::getVirtualDefStatement(llvm::MemoryAccess* defAccess, bool& isDefault, const llvm::MemorySSA& MSSA, const llvm::Function* currentFunction)
   {
      if(MSSA.isLiveOnEntryDef(defAccess))
      {
         isDefault = true;
         assert(defAccess->getValueID() == llvm::Value::MemoryDefVal);
         return getGimpleNop(currentFunction, currentFunction); /// LiveOnEntry is identified by the function and has scope the function again
      }
      else
      {
         if(defAccess->getValueID() == llvm::Value::MemoryDefVal)
            return assignCodeAuto(dyn_cast<llvm::MemoryUseOrDef>(defAccess)->getMemoryInst());
         else
         {
            assert(defAccess->getValueID() == llvm::Value::MemoryPhiVal);
            auto def_stmt = getVirtualGimplePhi(dyn_cast<llvm::MemoryPhi>(defAccess), MSSA);
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
   //       bool MayClobberIsAcquire = isAtLeastOrStrongerThan(MayClobber->getOrdering(), llvm::AtomicOrdering::Acquire);
   //       return !(SeqCstUse || MayClobberIsAcquire);
   //    }

   void DumpGimpleRaw::serialize_vops(const void* g)
   {
      assert(TREE_CODE(g) != GT(GIMPLE_PHI_VIRTUAL));
      llvm::Instruction* inst = const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(g));
      llvm::Function* currentFunction = inst->getFunction();
      auto& MSSA = modulePass->getAnalysis<llvm::MemorySSAWrapperPass>(*currentFunction).getMSSA();
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
      /// check for true dependencies by exploiting LLVM alias analysis infrastructure
      if(ma->getValueID() == llvm::Value::MemoryUseVal)
      {
         /// Serialize gimple pairs because of use after def chain
         std::set<llvm::MemoryAccess*> visited;
         auto startingMA = MSSA.getMemoryAccess(inst);
         if(llvm::ImmutableCallSite(inst) || isa<llvm::FenceInst>(inst))
            serialize_gimple_aliased_reaching_defs(startingMA, MSSA, visited, inst->getFunction(), nullptr, "vuse");
         else
         {
            const auto Loc = llvm::MemoryLocation::get(inst);
            serialize_gimple_aliased_reaching_defs(startingMA, MSSA, visited, inst->getFunction(), &Loc, "vuse");
         }
      }
      else if(ma->getValueID() == llvm::Value::MemoryDefVal)
      {
         const void* vdef = getSSA(ma, g, currentFunction, false);
         serialize_child("vdef", vdef);
         std::set<llvm::MemoryAccess*> visited;
         auto startingMA = MSSA.getMemoryAccess(inst);
         if(llvm::ImmutableCallSite(inst) || isa<llvm::FenceInst>(inst))
            serialize_gimple_aliased_reaching_defs(startingMA, MSSA, visited, inst->getFunction(), nullptr, "vover");
         else
         {
            const auto Loc = llvm::MemoryLocation::get(inst);
            serialize_gimple_aliased_reaching_defs(startingMA, MSSA, visited, inst->getFunction(), &Loc, "vover");
         }
      }
   }

   void DumpGimpleRaw::serialize_gimple_aliased_reaching_defs(llvm::MemoryAccess* MA, llvm::MemorySSA& MSSA, std::set<llvm::MemoryAccess*>& visited, const llvm::Function* currentFunction, const llvm::MemoryLocation* OrigLoc, const char* tag)
   {
      llvm::MemoryAccess* defMA = nullptr;
      if(MA->getValueID() != llvm::Value::MemoryPhiVal)
      {
         auto immDefAcc = dyn_cast<llvm::MemoryUseOrDef>(MA)->getDefiningAccess();
         if(MSSA.isLiveOnEntryDef(immDefAcc))
            defMA = immDefAcc;
         else if(OrigLoc)
         {
            defMA = MSSA.getWalker()->getClobberingMemoryAccess(immDefAcc, *OrigLoc);
         }
         else
         {
            defMA = immDefAcc;
         }
      }
      else
         defMA = MA;
      if(visited.find(defMA) != visited.end())
         return;
      visited.insert(defMA);

      if(defMA->getValueID() == llvm::Value::MemoryDefVal)
      {
         bool isDefault = false;
         const void* def_stmt = getVirtualDefStatement(defMA, isDefault, MSSA, currentFunction);
         serialize_child(tag, getSSA(MA, def_stmt, currentFunction, isDefault));
         if(!MSSA.isLiveOnEntryDef(defMA))
            serialize_gimple_aliased_reaching_defs(defMA, MSSA, visited, currentFunction, OrigLoc, tag);
      }
      else
      {
         assert(defMA->getValueID() == llvm::Value::MemoryPhiVal);
         auto mp = dyn_cast<llvm::MemoryPhi>(defMA);
         for(auto index = 0u; index < mp->getNumIncomingValues(); ++index)
         {
            auto val = mp->getIncomingValue(index);
            assert(val->getValueID() == llvm::Value::MemoryDefVal || val->getValueID() == llvm::Value::MemoryPhiVal);
            if(MSSA.isLiveOnEntryDef(val))
            {
               if(visited.find(val) == visited.end())
               {
                  bool isDefault = false;
                  const void* def_stmt = getVirtualDefStatement(val, isDefault, MSSA, currentFunction);
                  serialize_child(tag, getSSA(val, def_stmt, currentFunction, isDefault));
                  visited.insert(val);
               }
            }
            else if(val->getValueID() == llvm::Value::MemoryDefVal)
            {
               if(OrigLoc)
               {
                  val = MSSA.getWalker()->getClobberingMemoryAccess(val, *OrigLoc);
               }
               if(visited.find(val) == visited.end())
               {
                  if(val->getValueID() == llvm::Value::MemoryPhiVal)
                     serialize_gimple_aliased_reaching_defs(val, MSSA, visited, currentFunction, OrigLoc, tag);
                  else
                  {
                     bool isDefault = false;
                     const void* def_stmt = getVirtualDefStatement(val, isDefault, MSSA, currentFunction);
                     serialize_child(tag, getSSA(val, def_stmt, currentFunction, isDefault));
                     visited.insert(val);
                     if(!MSSA.isLiveOnEntryDef(val))
                        serialize_gimple_aliased_reaching_defs(val, MSSA, visited, currentFunction, OrigLoc, tag);
                  }
               }
            }
            else
               serialize_gimple_aliased_reaching_defs(val, MSSA, visited, currentFunction, OrigLoc, tag);
         }
      }
   }

   const void* DumpGimpleRaw::SSA_NAME_VAR(const void* t) const
   {
      const ssa_name* ssa = reinterpret_cast<const ssa_name*>(t);
      return ssa->var;
   }

   int DumpGimpleRaw::SSA_NAME_VERSION(const void* t) const
   {
      const ssa_name* ssa = reinterpret_cast<const ssa_name*>(t);
      return ssa->vers;
   }

   const void* DumpGimpleRaw::SSA_NAME_DEF_STMT(const void* t) const
   {
      const ssa_name* ssa = reinterpret_cast<const ssa_name*>(t);
      return ssa->def_stmts;
   }

   const void* DumpGimpleRaw::getMinValue(const void* t)
   {
      const ssa_name* ssa = reinterpret_cast<const ssa_name*>(t);
      if(ssa->var || ssa->isVirtual)
         return nullptr;
      llvm::Instruction* inst = const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(ssa->def_stmts));
      if(inst->getType()->isIntegerTy())
      {
         if(RA)
         {
            auto varRange = RA->getRange(inst);
            if(!varRange.isMaxRange())
            {
               auto isSigned = CheckSignedTag(TREE_TYPE(t));
#ifdef DEBUG_RA
               if(isSigned)
                  llvm::errs() << "Range: <" << varRange.getSignedMin() << "," << varRange.getSignedMax() << "> ";
               else
                  llvm::errs() << "Range: <" << varRange.getUnsignedMin().getZExtValue() << "," << varRange.getUnsignedMax().getZExtValue() << "> ";
               inst->print(llvm::errs());
               llvm::errs() << "\n";
               assert(isSigned ? (varRange.getSignedMin().sextOrTrunc(64).getSExtValue() <= varRange.getSignedMax().sextOrTrunc(64).getSExtValue()) :
                                 (varRange.getUnsignedMin().zextOrTrunc(64).getZExtValue() <= varRange.getUnsignedMax().zextOrTrunc(64).getZExtValue()));
#endif
               return assignCodeAuto(llvm::ConstantInt::get(inst->getContext(), (isSigned ? varRange.getSignedMin().sextOrTrunc(64) : varRange.getUnsignedMin().zextOrTrunc(64))));
            }
            else
               return nullptr;
         }
         else
         {
            llvm::BasicBlock* BB = inst->getParent();
            llvm::Function* currentFunction = inst->getFunction();
            assert(modulePass);
            llvm::LazyValueInfo& LVI = modulePass->getAnalysis<llvm::LazyValueInfoWrapperPass>(*currentFunction).getLVI();
            unsigned long long int zeroMask = 0;
#if __clang_major__ != 4
            llvm::KnownBits KnownOneZero;
            auto AC = modulePass->getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(*currentFunction);
            auto& DT = modulePass->getAnalysis<llvm::DominatorTreeWrapperPass>(*currentFunction).getDomTree();
            KnownOneZero = llvm::computeKnownBits(inst, *DL, 0, &AC, inst, &DT);
            zeroMask = KnownOneZero.Zero.getZExtValue();
#else
            //         llvm::APInt KnownZero;
            //         llvm::APInt KnownOne;
            //         auto AC = modulePass->getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(*currentFunction);
            //         const auto& DT = modulePass->getAnalysis<llvm::DominatorTreeWrapperPass>(*currentFunction).getDomTree();
            //         llvm::computeKnownBits(inst,KnownZero, KnownOne, *DL, 0, &AC, inst, &DT);
            //         zeroMask = KnownZero.getZExtValue();
#endif
            auto ty = inst->getType();
            auto obj_size = ty->isSized() ? DL->getTypeAllocSizeInBits(ty) : 8ULL;
            auto active_size = ty->isSized() ? DL->getTypeSizeInBits(ty) : 8ULL;
            auto isSigned = CheckSignedTag(TREE_TYPE(t));
            auto i = active_size;
            for(; i > 1; --i)
            {
               if((zeroMask & (1ULL << (i - 1))) == 0)
                  break;
            }
            if(i != active_size)
            {
               if(isSigned)
                  ++i;
               if(i < active_size)
                  active_size = i;
            }
            llvm::ConstantRange range = LVI.getConstantRange(inst, BB, inst);
            //            if(!range.isFullSet() && !isSigned)
            //            {
            //               if(range.getUnsignedMin().getZExtValue() > range.getUnsignedMax().getZExtValue())
            //                  isSigned = true;
            //            }

            if(obj_size != active_size)
            {
               uint64_t val;
               if(isSigned)
               {
                  val = -(1ULL << (active_size - 1));
               }
               else
                  val = 0;
               if(!range.isFullSet())
               {
                  if(isSigned)
                  {
                     if(range.getSignedMin().getSExtValue() > (int64_t)val)
                        val = range.getSignedMin().getSExtValue();
                  }
                  else
                  {
                     if(range.getUnsignedMin().getZExtValue() > val)
                        val = range.getUnsignedMin().getZExtValue();
                  }
               }

#ifdef DEBUG_RA
               if(isSigned)
                  llvm::errs() << "Range: " << (int64_t)val << ",";
               else
                  llvm::errs() << "Range: " << val << ",";
#endif
               auto context = TREE_CODE(t) == GT(SIGNEDPOINTERTYPE) ? moduleContext : &ty->getContext();
               if(uicTable.find(val) == uicTable.end())
                  uicTable[val] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), val, false));
               return uicTable.find(val)->second;
            }
            else
            {
               if(!range.isFullSet())
               {
                  //               llvm::errs() << "Range:\n";
                  //               range.print(llvm::errs());
                  //               llvm::errs() << range.getBitWidth() << "\n";
                  //               auto low = range.getLower();
                  //               llvm::errs() << "Min: " << (low.isMinValue()?"Tmin":"Fmin") << " " << (low.isMaxValue()?"Tmax":"Fmax") << "\n";
                  //               llvm::errs() << "Min: " << (low.isMinSignedValue()?"Tmin_signed":"Fmin_signed") << " " << (low.isMaxSignedValue()?"Tmax_signed":"Fmax_signed") << "\n";
                  //               llvm::errs() << "Min: " << (range.getUnsignedMin()).getZExtValue() << "\n";
                  //               llvm::errs() << "Min: " << (range.getSignedMin()) << "\n";
                  //               low.print(llvm::errs(), isSigned);
                  //               llvm::errs() << (isSigned?"T":"F") << "\n";
                  //               auto upper = range.getUpper()-1;
                  //               llvm::errs() << "Max: " << (upper.isMinValue()?"Tmin":"Fmin") << " " << (upper.isMaxValue()?"Tmax":"Fmax") << "\n";
                  //               llvm::errs() << "Max: " << (upper.isMinSignedValue()?"Tmin_signed":"Fmin_signed") << " " << (upper.isMaxSignedValue()?"Tmax_signed":"Fmax_signed") << "\n";
                  //               llvm::errs() << "Max: " << (range.getUnsignedMax()).getZExtValue() << "\n";
                  //               llvm::errs() << "Max: " << (range.getSignedMax()) << "\n";
                  //               upper.print(llvm::errs(), isSigned);
                  //               llvm::errs() << (isSigned?"T":"F") << "\n";
#ifdef DEBUG_RA
                  if(isSigned)
                     llvm::errs() << "Range: <" << range.getSignedMin() << "," << range.getSignedMax() << "> ";
                  else
                     llvm::errs() << "Range: <" << range.getUnsignedMin().getZExtValue() << "," << range.getUnsignedMax().getZExtValue() << "> ";
                  inst->print(llvm::errs());
                  llvm::errs() << "\n";
#endif
                  return assignCodeAuto(llvm::ConstantInt::get(inst->getContext(), (isSigned ? range.getSignedMin().sextOrTrunc(64) : range.getUnsignedMin().zextOrTrunc(64))));
               }
               else
                  return nullptr;
            }
         }
      }
      else
         return nullptr;
   }
   const void* DumpGimpleRaw::getMaxValue(const void* t)
   {
      const ssa_name* ssa = reinterpret_cast<const ssa_name*>(t);
      if(ssa->var || ssa->isVirtual)
         return nullptr;
      llvm::Instruction* inst = const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(ssa->def_stmts));
      if(inst->getType()->isIntegerTy())
      {
         if(RA)
         {
            auto varRange = RA->getRange(inst);
            if(!varRange.isMaxRange())
            {
               auto isSigned = CheckSignedTag(TREE_TYPE(t));
               return assignCodeAuto(llvm::ConstantInt::get(inst->getContext(), (isSigned ? varRange.getSignedMax().sextOrTrunc(64) : varRange.getUnsignedMax().zextOrTrunc(64))));
            }
            else
               return nullptr;
         }
         else
         {
            llvm::BasicBlock* BB = inst->getParent();
            llvm::Function* currentFunction = inst->getFunction();
            assert(modulePass);
            llvm::LazyValueInfo& LVI = modulePass->getAnalysis<llvm::LazyValueInfoWrapperPass>(*currentFunction).getLVI();
            unsigned long long int zeroMask = 0;
#if __clang_major__ != 4
            llvm::KnownBits KnownOneZero;
            auto AC = modulePass->getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(*currentFunction);
            const auto& DT = modulePass->getAnalysis<llvm::DominatorTreeWrapperPass>(*currentFunction).getDomTree();
            KnownOneZero = llvm::computeKnownBits(inst, *DL, 0, &AC, inst, &DT);
            zeroMask = KnownOneZero.Zero.getZExtValue();
#else
            //         llvm::APInt KnownZero;
            //         llvm::APInt KnownOne;
            //         auto AC = modulePass->getAnalysis<llvm::AssumptionCacheTracker>().getAssumptionCache(*currentFunction);
            //         const auto& DT = modulePass->getAnalysis<llvm::DominatorTreeWrapperPass>(*currentFunction).getDomTree();
            //         llvm::computeKnownBits(inst,KnownZero, KnownOne, *DL, 0, &AC, inst, &DT);
            //         zeroMask = KnownZero.getZExtValue();
#endif
            auto ty = inst->getType();
            auto obj_size = ty->isSized() ? DL->getTypeAllocSizeInBits(ty) : 8ULL;
            auto active_size = ty->isSized() ? DL->getTypeSizeInBits(ty) : 8ULL;
            auto isSigned = CheckSignedTag(TREE_TYPE(t));
            auto i = active_size;
            for(; i > 1; --i)
            {
               if((zeroMask & (1ULL << (i - 1))) == 0)
                  break;
            }
            if(i != active_size)
            {
               if(isSigned)
                  ++i;
               if(i < active_size)
                  active_size = i;
            }
            llvm::ConstantRange range = LVI.getConstantRange(inst, BB, inst);
            //            if(!range.isFullSet() && !isSigned)
            //            {
            //               if(range.getUnsignedMin().getZExtValue() > range.getUnsignedMax().getZExtValue())
            //                  isSigned = true;
            //            }
            if(obj_size != active_size)
            {
               uint64_t val;
               assert(active_size < 64);
               if(isSigned)
               {
                  val = (1ULL << (active_size - 1)) - 1;
               }
               else
                  val = (1ULL << (active_size)) - 1;
               if(!range.isFullSet())
               {
                  if(isSigned)
                  {
                     if(range.getSignedMax().getSExtValue() < (int64_t)val)
                        val = range.getSignedMin().getSExtValue();
                  }
                  else
                  {
                     if(range.getUnsignedMax().getZExtValue() < val)
                        val = range.getUnsignedMax().getZExtValue();
                  }
               }
               else
               {
                  assert(range.getBitWidth() >= active_size);
               }

#ifdef DEBUG_RA
               if(isSigned)
                  llvm::errs() << (int64_t)val << ") ";
               else
                  llvm::errs() << val << ") ";
               inst->print(llvm::errs());
               llvm::errs() << "\n";
#endif
               // auto maxvalue = llvm::APInt::getMaxValue(std::max((isSigned?active_size-1ULL:active_size),1ULL)).getZExtValue();
               auto maxvalue = val;
               auto context = TREE_CODE(t) == GT(SIGNEDPOINTERTYPE) ? moduleContext : &ty->getContext();
               if(uicTable.find(maxvalue) == uicTable.end())
                  uicTable[maxvalue] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), maxvalue, false));
               return uicTable.find(maxvalue)->second;
            }
            else
            {
               if(!range.isFullSet())
               {
                  auto isSigned = CheckSignedTag(TREE_TYPE(t));
                  return assignCodeAuto(llvm::ConstantInt::get(inst->getContext(), (isSigned ? range.getSignedMax().sextOrTrunc(64) : range.getUnsignedMax().zextOrTrunc(64))));
               }
               else
               {
                  assert(range.getBitWidth() == active_size);
                  return nullptr;
               }
            }
         }
      }
      else
         return nullptr;
   }

   const std::list<std::pair<const void*, const void*>> DumpGimpleRaw::CONSTRUCTOR_ELTS(const void* t)
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
               for(unsigned index = 0; index < val->getNumElements(); ++index)
               {
                  if(uicTable.find(index) == uicTable.end())
                     uicTable[index] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_obj->getContext()), index, false));
                  const void* idx = uicTable.find(index)->second;
                  const void* valu = getOperand(val->getSequentialElement(), nullptr);
                  res.push_back(std::make_pair(idx, valu));
               }
            }
            else
            {
               const void* ty = TREE_TYPE(t);
               for(unsigned index = 0; index < val->getNumElements(); ++index)
               {
                  auto op = val->getStructElement(index);
                  const void* valu = getOperand(op, nullptr);
                  const void* idx = GET_FIELD_DECL(TREE_TYPE(assignCodeAuto(op)), index, ty);
                  res.push_back(std::make_pair(idx, valu));
               }
            }
            return res;
         }
         case llvm::Value::ConstantStructVal:
         {
            const llvm::ConstantStruct* val = reinterpret_cast<const llvm::ConstantStruct*>(t);
            const void* ty = TREE_TYPE(t);
            for(unsigned index = 0; index < val->getNumOperands(); ++index)
            {
               auto op = val->getOperand(index);
               const void* valu = getOperand(op, nullptr);
               const void* idx = GET_FIELD_DECL(TREE_TYPE(assignCodeAuto(op)), index, ty);
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
               if(uicTable.find(index) == uicTable.end())
                  uicTable[index] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_obj->getContext()), index, false));
               const void* idx = uicTable.find(index)->second;
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
               if(uicTable.find(index) == uicTable.end())
                  uicTable[index] = assignCodeAuto(llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_obj->getContext()), index, false));
               const void* idx = uicTable.find(index)->second;
               auto elmnt = val->getOperand(index);
               const void* valu = getOperand(elmnt, nullptr);
               res.push_back(std::make_pair(idx, valu));
            }
            return res;
         }

         default:
            llvm::errs() << "CONSTRUCTOR_ELTS kind not supported: " << ValueTyNames[vid] << "\n";
            stream.close();
            llvm_unreachable("Plugin Error");
      }
   }

   const void* DumpGimpleRaw::CASE_LOW(const void* t)
   {
      assert(TREE_CODE(t) == GT(CASE_LABEL_EXPR));
      const tree_expr* te = reinterpret_cast<const tree_expr*>(t);
      return te->op1;
   }
   const void* DumpGimpleRaw::CASE_HIGH(const void* t)
   {
      assert(TREE_CODE(t) == GT(CASE_LABEL_EXPR));
      const tree_expr* te = reinterpret_cast<const tree_expr*>(t);
      return te->op2;
   }
   const void* DumpGimpleRaw::CASE_LABEL(const void* t)
   {
      assert(TREE_CODE(t) == GT(CASE_LABEL_EXPR));
      const tree_expr* te = reinterpret_cast<const tree_expr*>(t);
      return te->op3;
   }

   void DumpGimpleRaw::serialize_new_line()
   {
      snprintf(buffer, LOCAL_BUFFER_LEN, "\n%*s", SOL_COLUMN, "");
      stream << buffer;
      column = SOL_COLUMN;
   }

   void DumpGimpleRaw::serialize_maybe_newline()
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

   void DumpGimpleRaw::serialize_pointer(const char* field, const void* ptr)
   {
      serialize_maybe_newline();
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: %-8llx ", field, (unsigned long long)ptr);
      stream << buffer;
      column += 15;
   }

   void DumpGimpleRaw::DumpVersion(llvm::raw_fd_ostream& stream)
   {
      const char* panda_plugin_version = (const char*)PANDA_PLUGIN_VERSION;
      int version = __GNUC__, minor = __GNUC_MINOR__, patchlevel = __GNUC_PATCHLEVEL__;
      stream << "GCC_VERSION: \"" << version << "." << minor << "." << patchlevel << "\"\n";
      stream << "PLUGIN_VERSION: \"" << panda_plugin_version << "\"\n";
   }

   void DumpGimpleRaw::serialize_int(const char* field, int i)
   {
      serialize_maybe_newline();
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: %-7d ", field, i);
      stream << buffer;
      column += 14;
   }

   /* Serialize wide integer i using FIELD to identify it.  */
   void DumpGimpleRaw::serialize_wide_int(const char* field, int64_t i)
   {
      serialize_maybe_newline();
      snprintf(buffer, LOCAL_BUFFER_LEN,
               "%-4s: "
               "%" PRId64,
               field, i);
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
         llvm_unreachable("PPCDoubleDouble format not supported in real_to_hexadecimal");
      }
      else if(sem == &llvm::APFloat::IEEEquad())
      {
         nbitsExp = 15;
         nbitsMan = 112;
      }
      else
         llvm_unreachable("unexpected floating point format in real_to_hexadecimal");

      unsigned ExpBiased = API.lshr(nbitsMan).getZExtValue() & ((1U << nbitsExp) - 1);
      int ExpUnbiased = (llvm::APInt::getNullValue(API.getBitWidth()) == APIAbs) ? 0 : (ExpBiased - ((1U << (nbitsExp - 1)) - 2));

      snprintf(buffer, size_buff, "p%+d", ExpUnbiased);

      llvm::APInt Mantissa = API & llvm::APInt::getAllOnesValue(nbitsMan).zext(API.getBitWidth());
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
   void DumpGimpleRaw::serialize_real(const void* t)
   {
      serialize_maybe_newline();
      /* Code copied from print_node.  */
      /*if(TREE_OVERFLOW(t))
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
         snprintf(buffer, LOCAL_BUFFER_LEN, "valx: %-7s ", "\"Nan\"");
         stream << buffer;
      }
      else
      {
         bool isDouble = &d.getSemantics() == &llvm::APFloat::IEEEdouble();
         snprintf(buffer, LOCAL_BUFFER_LEN, "%.*g", (isDouble ? __DBL_DECIMAL_DIG__ : __FLT_DECIMAL_DIG__), (isDouble ? d.convertToDouble() : d.convertToFloat()));
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

   int DumpGimpleRaw::serialize_with_double_quote(const char* input, int length)
   {
      int new_length;
      stream << "\"";
      new_length = serialize_with_escape(input, length);
      stream << "\"";
      return new_length + 2;
   }

   /* Add a backslash before an escape sequence to serialize the string
      with the escape sequence */
   int DumpGimpleRaw::serialize_with_escape(const char* input, int length)
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
   void DumpGimpleRaw::serialize_string(const char* string)
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
   void DumpGimpleRaw::serialize_string_field(const char* field, const char* str)
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

   void DumpGimpleRaw::serialize_string_cst(const char* field, const char* str, int length, unsigned int precision)
   {
      int new_length;
      serialize_maybe_newline();
      if(precision == 8)
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: ", field);
         stream << buffer;
         new_length = serialize_with_double_quote(str, length - 1);
         if(new_length > 7)
            column += 6 + new_length + 1;
         else
            column += 14;
         serialize_int("lngt", length);
      }
      else
      {
         const unsigned int* string = (const unsigned int*)str;
         unsigned int i, lngt = length / 4 - 1;
         snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: \"", field);
         stream << buffer;
         for(i = 0; i < lngt; i++)
         {
            snprintf(buffer, LOCAL_BUFFER_LEN, "\\x%x", string[i]);
            stream << buffer;
         }
         stream << "\" ";
         column += 7 + lngt;
         serialize_int("lngt", lngt + 1);
      }
   }

   void DumpGimpleRaw::queue_and_serialize_index(const char* field, const void* t)
   {
#if PRINT_DBG_MSG
      llvm::errs() << "field:" << field << "\n";
#endif
      unsigned int index;
      if(t == nullptr)
         return;
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

   void DumpGimpleRaw::serialize_index(unsigned int index)
   {
      snprintf(buffer, LOCAL_BUFFER_LEN, "@%-6u ", index);
      stream << buffer;
      column += 8;
   }

   void DumpGimpleRaw::queue_and_serialize_type(const void* t)
   {
      queue_and_serialize_index("type", TREE_TYPE(t));
   }

   static void computeLoopLabels(std::map<const llvm::Loop*, unsigned>& loopLabes, llvm::Loop* curLoop, unsigned int& label)
   {
      loopLabes[curLoop] = label;
      label++;
      for(auto it = curLoop->begin(); it != curLoop->end(); ++it)
      {
         computeLoopLabels(loopLabes, *it, label);
      }
   }

   const void* DumpGimpleRaw::createGimpleLabelStmt(const llvm::BasicBlock* BB)
   {
      if(index2gimple_label.find(BB) == index2gimple_label.end())
      {
         auto& gl = index2gimple_label[BB];
         assignCode(&gl, GT(GIMPLE_LABEL));
         const llvm::Function* currentFunction = BB->getParent();
         gl.scpe = assignCodeAuto(currentFunction);
         gl.bb_index = getBB_index(BB);
         if(index2label_decl.find(BB) == index2label_decl.end())
         {
            auto& ld = index2label_decl[BB];
            assignCode(&ld, GT(LABEL_DECL));
            ld.type = assignCodeType(llvm::Type::getVoidTy(currentFunction->getContext()));
            ld.scpe = assignCodeAuto(currentFunction);
         }
         gl.op = &index2label_decl.find(BB)->second;
      }
      return &index2gimple_label.find(BB)->second;
   }

   void DumpGimpleRaw::dequeue_and_serialize_gimple(const void* g)
   {
      assert(llvm2index.find(g) != llvm2index.end());
      unsigned int index = llvm2index.find(g)->second;

      auto code = TREE_CODE(g);
      const char* code_name = GET_TREE_CODE_NAME(code);
#if PRINT_DBG_MSG
      if(code != GT(GIMPLE_NOP) && code != GT(GIMPLE_PHI_VIRTUAL) && code != GT(GIMPLE_LABEL))
      {
         llvm::Instruction* inst = const_cast<llvm::Instruction*>(reinterpret_cast<const llvm::Instruction*>(g));
         llvm::Function* currentFunction = inst->getFunction();
         llvm::errs() << "@" << code_name << " @" << index << "\n";
         inst->print(llvm::errs());
         llvm::errs() << "\n";
         auto& MSSA = modulePass->getAnalysis<llvm::MemorySSAWrapperPass>(*currentFunction).getMSSA();
         if(MSSA.getMemoryAccess(inst))
            llvm::errs() << "| " << *MSSA.getMemoryAccess(inst) << "\n";
      }
      else
         llvm::errs() << "@" << code_name << "\n";
#endif
      /* Print the node index.  */
      serialize_index(index);

      snprintf(buffer, LOCAL_BUFFER_LEN, "%-16s ", code_name);
      stream << buffer;
      column = 25;
      serialize_child("scpe", getGimpleScpe(g));
      serialize_int("bb_index", getGimple_bb_index(g));

      if(gimple_has_mem_ops(g))
         serialize_vops(g);

      if(gimple_has_location(g))
      {
         expanded_location xloc = expand_location(gimple_location(g));
         serialize_maybe_newline();
         snprintf(buffer, LOCAL_BUFFER_LEN, "srcp: \"%s\":%-d:%-6d ", xloc.file, xloc.line, xloc.column);
         if(xloc.file && xloc.file[0])
         {
            stream << buffer;
            column += 12 + strlen(xloc.file) + 8;
         }
      }
      serialize_int("time_weight", code == GT(GIMPLE_NOP) ? 0 : 1);
      serialize_int("size_weight", code == GT(GIMPLE_NOP) ? 0 : 1);
      switch(gimple_code(g))
      {
         case GT(GIMPLE_ASM):
         {
            serialize_string_field("str", gimple_asm_string(g));
            llvm_unreachable("gimple asm unsupported");
            break;
         }
         case GT(GIMPLE_ASSIGN_ALLOCA):
         {
            auto lhs = gimple_assign_lhs(g);
            auto rhs = gimple_assign_rhs_alloca(g);
            add_alloca_pt_solution(lhs, rhs);
            serialize_child("op", lhs);
            serialize_child("op", rhs);
            if(index2alloca_var.find(g)->second.addr)
               serialize_string("addr");
            break;
         }
         case GT(GETELEMENTPTR):
         {
            auto lhs = gimple_assign_lhs(g);
            auto rhs = gimple_assign_rhs_getelementptr(g);
            serialize_child("op", lhs);
            serialize_child("op", rhs);
            std::set<const llvm::User*> visited;
            auto currInst = reinterpret_cast<const llvm::User*>(g);
            visited.insert(currInst);
            const llvm::TargetLibraryInfo& TLI = modulePass->getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();
            if(temporary_addr_check(currInst, visited, TLI))
               serialize_string("addr");
            break;
         }
         case GT(GIMPLE_SSACOPY):
         {
            auto lhs = gimple_assign_lhs(g);
            auto rhs = gimple_assign_rhs1(g);
            serialize_child("op", lhs);
            serialize_child("op", rhs);
            break;
         }
         case GT(GIMPLE_ASSIGN):
         {
            if(get_gimple_rhs_class(gimple_expr_code(g)) == GIMPLE_TERNARY_RHS)
            {
               auto lhs = gimple_assign_lhs(g);
               auto tc = gimple_assign_rhs_code(g);
               serialize_child("op", lhs);
               serialize_child("op", build3(tc, TREE_TYPE(lhs), gimple_assign_rhs1(g), gimple_assign_rhs2(g), gimple_assign_rhs3(g)));
            }
            else if(get_gimple_rhs_class(gimple_expr_code(g)) == GIMPLE_BINARY_RHS)
            {
               auto lhs = gimple_assign_lhs(g);
               auto tc = gimple_assign_rhs_code(g);
               serialize_child("op", lhs);
               serialize_child("op", build2(tc, TREE_TYPE(lhs), gimple_assign_rhs1(g), gimple_assign_rhs2(g)));
            }
            else if(get_gimple_rhs_class(gimple_expr_code(g)) == GIMPLE_UNARY_RHS)
            {
               auto lhs = gimple_assign_lhs(g);
               auto tc = gimple_assign_rhs_code(g);
               serialize_child("op", lhs);
               auto rhs = build1(tc, TREE_TYPE(lhs), gimple_assign_rhs1(g));
               serialize_child("op", rhs);
            }
            else if(get_gimple_rhs_class(gimple_expr_code(g)) == GIMPLE_SINGLE_RHS)
            {
               auto lhs = gimple_assign_lhs(g);
               serialize_child("op", lhs);
               auto ltype = TREE_TYPE(lhs);
               auto rhs = gimple_assign_rhs1(g);
               auto rtype = TREE_TYPE(rhs);
               if(ltype == rtype)
                  serialize_child("op", rhs);
               else
                  serialize_child("op", build1(GT(VIEW_CONVERT_EXPR), ltype, rhs));
            }
            else if(gimple_expr_code(g) == GT(CALL_EXPR))
            {
               serialize_child("op", gimple_assign_lhs(g));
               serialize_child("op", build_custom_function_call_expr(g));
            }
            else if(gimple_expr_code(g) == GT(FCMP_OEQ) || gimple_expr_code(g) == GT(FCMP_ONE) || gimple_expr_code(g) == GT(FCMP_ORD) || gimple_expr_code(g) == GT(FCMP_UEQ) || gimple_expr_code(g) == GT(FCMP_UNE) || gimple_expr_code(g) == GT(FCMP_UNO))
            {
               const llvm::FCmpInst* cmpInst = reinterpret_cast<const llvm::FCmpInst*>(g);

               auto noNAN = cmpInst->getFastMathFlags().noNaNs();
               auto lhs = gimple_assign_lhs(g);
               serialize_child("op", lhs);
               auto btype = TREE_TYPE(lhs);
               auto llvm_op1 = cmpInst->getOperand(0);
               auto llvm_op2 = cmpInst->getOperand(1);
               bool isOp1Const = isa<llvm::ConstantFP>(llvm_op1);
               bool isOp2Const = isa<llvm::ConstantFP>(llvm_op2);
               auto prec1 = llvm_op1->getType()->getPrimitiveSizeInBits();
               assert(llvm_op2->getType()->getPrimitiveSizeInBits() == prec1);
               auto intObjType = llvm::Type::getIntNTy(*moduleContext, static_cast<unsigned>(prec1));
               auto op1 = gimple_assign_rhs1(g);
               auto op2 = gimple_assign_rhs2(g);
               auto vcType = assignCodeType(intObjType);
               auto vc_op1 = isOp1Const ? assignCodeAuto(llvm::ConstantInt::get(intObjType, cast<llvm::ConstantFP>(llvm_op1)->getValueAPF().bitcastToAPInt())) : build1(GT(VIEW_CONVERT_EXPR), vcType, op1);
               auto vc_op2 = isOp2Const ? assignCodeAuto(llvm::ConstantInt::get(intObjType, cast<llvm::ConstantFP>(llvm_op2)->getValueAPF().bitcastToAPInt())) : build1(GT(VIEW_CONVERT_EXPR), vcType, op2);
               auto constOne = assignCodeAuto(llvm::ConstantInt::get(intObjType, 1, false));
               auto lshift_op1 = build2(GT(LSHIFT_EXPR), vcType, vc_op1, constOne);
               auto abs_op1 = build2(GT(RSHIFT_EXPR), vcType, lshift_op1, constOne);
               auto lshift_op2 = build2(GT(LSHIFT_EXPR), vcType, vc_op2, constOne);
               auto abs_op2 = build2(GT(RSHIFT_EXPR), vcType, lshift_op2, constOne);
               const void* constNAN;
               if(prec1 == 32)
                  constNAN = assignCodeAuto(llvm::ConstantInt::get(intObjType, 0x7f800000, false));
               else if(prec1 == 64)
                  constNAN = assignCodeAuto(llvm::ConstantInt::get(intObjType, 0x7FF0000000000000, false));
               else
               {
                  llvm::errs() << prec1 << "\n";
                  llvm_unreachable("unsupported floating point precision");
               }
               auto isNAN_op1 = build2(GT(GT_EXPR), btype, abs_op1, constNAN);
               auto isNAN_op2 = build2(GT(GT_EXPR), btype, abs_op2, constNAN);
               const void* rhs;
               auto gcode = gimple_expr_code(g);
               if(gcode == GT(FCMP_ORD) || gcode == GT(FCMP_OEQ) || gcode == GT(FCMP_ONE))
               {
                  auto isNotNAN_op1 = build1(GT(TRUTH_NOT_EXPR), btype, isNAN_op1);
                  auto isNotNAN_op2 = build1(GT(TRUTH_NOT_EXPR), btype, isNAN_op2);
                  auto ordered = build2(GT(TRUTH_ANDIF_EXPR), btype, isNotNAN_op1, isNotNAN_op2);
                  if(gcode == GT(FCMP_ORD))
                  {
                     assert(!noNAN);
                     rhs = ordered;
                  }
                  else if(gcode == GT(FCMP_OEQ))
                  {
                     auto eq = build2(GT(EQ_EXPR), btype, op1, op2);
                     rhs = noNAN ? eq : build2(GT(TRUTH_ANDIF_EXPR), btype, ordered, eq);
                  }
                  else if(gcode == GT(FCMP_ONE))
                  {
                     auto neq = build2(GT(NE_EXPR), btype, op1, op2);
                     rhs = noNAN ? neq : build2(GT(TRUTH_ANDIF_EXPR), btype, ordered, neq);
                  }
                  else
                     llvm_unreachable("unexpected case");
               }
               else
               {
                  auto unordered = build2(GT(TRUTH_ORIF_EXPR), btype, isNAN_op1, isNAN_op2);
                  if(gcode == GT(FCMP_UNO))
                  {
                     assert(!noNAN);
                     rhs = unordered;
                  }
                  else if(gcode == GT(FCMP_UEQ))
                  {
                     auto eq = build2(GT(EQ_EXPR), btype, op1, op2);
                     rhs = noNAN ? eq : build2(GT(TRUTH_ORIF_EXPR), btype, unordered, eq);
                  }
                  else if(gcode == GT(FCMP_UNE))
                  {
                     auto neq = build2(GT(NE_EXPR), btype, op1, op2);
                     rhs = noNAN ? neq : build2(GT(TRUTH_ORIF_EXPR), btype, unordered, neq);
                  }
                  else
                     llvm_unreachable("unexpected case");
               }
               serialize_child("op", rhs);
            }
            else
               llvm_unreachable("unexpected condition");
            break;
         }
         case GT(GIMPLE_COND):
            serialize_child("op", gimple_cond_op(g));
            break;

         case GT(GIMPLE_LABEL):
            serialize_child("op", gimple_label_label(g));
            break;

         case GT(GIMPLE_GOTO):
            llvm_unreachable("unexpected GIMPLE_GOTO"); /// serialize_child ("op", gimple_goto_dest (g));
            break;

         case GT(GIMPLE_NOPMEM):
            break;

         case GT(GIMPLE_NOP):
            break;

         case GT(GIMPLE_RETURN):
            serialize_child("op", gimple_return_retval(g));
            break;

         case GT(GIMPLE_SWITCH):
            serialize_child("op", gimple_switch_index(g));
            serialize_child("op", gimple_switch_vec(g));
            // llvm_unreachable("not yet supported");
            break;

         case GT(GIMPLE_PHI):
         {
            serialize_child("res", gimple_phi_result(g));
            std::set<int> bb_visited;
            for(auto i = 0u; i < gimple_phi_num_args(g); i++)
            {
               auto bbIndex = gimple_phi_arg_edgeBBindex(g, i);
               if(bb_visited.find(bbIndex) == bb_visited.end())
               {
                  bb_visited.insert(bbIndex);
                  serialize_child("def", gimple_phi_arg_def(g, i));
                  serialize_int("edge", bbIndex);
               }
            }
            break;
         }
         case GT(GIMPLE_PHI_VIRTUAL):
         {
            serialize_child("res", gimple_phi_virtual_result(g));
            std::set<int> bb_visited;
            for(auto i = 0u; i < gimple_phi_virtual_num_args(g); i++)
            {
               auto bbIndex = gimple_phi_virtual_arg_edgeBBindex(g, i);
               if(bb_visited.find(bbIndex) == bb_visited.end())
               {
                  bb_visited.insert(bbIndex);
                  serialize_child("def", gimple_phi_virtual_arg_def(g, i));
                  serialize_int("edge", bbIndex);
               }
            }
            serialize_string("virtual");
            break;
         }
         case GT(GIMPLE_CALL):
         {
            serialize_child("fn", gimple_call_fn(g));
            unsigned int arg_index;
            for(arg_index = 0; arg_index < gimple_call_num_args(g); arg_index++)
            {
               serialize_child("arg", gimple_call_arg(g, arg_index));
            }
            break;
         }
         default:
            llvm_unreachable("unexpected gimple statement");
      }

      /* Terminate the line.  */
      stream << "\n";
   }

   void DumpGimpleRaw::dequeue_and_serialize_statement(const void* t)
   {
      assert(llvm2index.find(t) != llvm2index.end());
      unsigned int index = llvm2index.find(t)->second;

      const char* code_name = GET_TREE_CODE_NAME(TREE_CODE(t));

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
      const llvm::Function::BasicBlockListType& bblist = *reinterpret_cast<const llvm::Function::BasicBlockListType*>(t);
      llvm::Function* currentFunction = const_cast<llvm::Function*>(bblist.front().getParent());
      assert(modulePass);
      auto& LI = modulePass->getAnalysis<llvm::LoopInfoWrapperPass>(*currentFunction).getLoopInfo();
      std::map<const llvm::Loop*, unsigned> loopLabes;
      if(!LI.empty())
      {
         unsigned int label = 1;
         for(auto it = LI.begin(); it != LI.end(); ++it)
            computeLoopLabels(loopLabes, *it, label);
      }
      std::set<const llvm::BasicBlock*> BB_with_gimple_label;
      for(const auto& BB : bblist)
      {
         if(isa<llvm::SwitchInst>(BB.getTerminator()))
         {
            const llvm::SwitchInst* si = cast<llvm::SwitchInst>(BB.getTerminator());
            const llvm::BasicBlock* dest = si->getDefaultDest();
            BB_with_gimple_label.insert(dest);
            createGimpleLabelStmt(dest);
            for(auto case_expr : si->cases())
            {
               dest = case_expr.getCaseSuccessor();
               createGimpleLabelStmt(dest);
               BB_with_gimple_label.insert(dest);
            }
         }
      }
      auto& MSSA = modulePass->getAnalysis<llvm::MemorySSAWrapperPass>(*currentFunction).getMSSA();

      for(const auto& BB : bblist)
      {
         const char* field;
         serialize_new_line();
         serialize_int("bloc", getBB_index(&BB));
#if PRINT_DBG_MSG
         llvm::errs() << "BB" << getBB_index(&BB) << "\n";
         if(MSSA.getMemoryAccess(&BB))
            llvm::errs() << "|!!!!!!!!!!!!!!!!!! " << *MSSA.getMemoryAccess(&BB) << "\n";
#endif
         if(!LI.empty() && LI.getLoopFor(&BB) && LI.getLoopFor(&BB)->getHeader() == &BB && LI.getLoopFor(&BB)->isAnnotatedParallel())
            serialize_string("hpl");
         if(LI.empty() || !LI.getLoopFor(&BB))
            serialize_int("loop_id", 0);
         else
            serialize_int("loop_id", loopLabes.find(LI.getLoopFor(&BB))->second);
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
         if(isa<llvm::BranchInst>(BB.getTerminator()))
         {
            const llvm::BranchInst* bi = cast<llvm::BranchInst>(BB.getTerminator());
            if(bi->getNumOperands() == 3)
            {
               serialize_int("true_edge", getBB_index(bi->getSuccessor(0)));
               serialize_int("false_edge", getBB_index(bi->getSuccessor(1)));
            }
         }
         if(MSSA.getMemoryAccess(&BB))
         {
            /// add virtual phi
            serialize_gimple_child("phi", getVirtualGimplePhi(MSSA.getMemoryAccess(&BB), MSSA));
         }
         bool firstStmt = BB_with_gimple_label.find(&BB) != BB_with_gimple_label.end();
         for(const auto& inst : BB.getInstList())
         {
            if(isa<llvm::PHINode>(inst))
               serialize_gimple_child("phi", assignCodeAuto(&inst));
            else
            {
               if(isa<llvm::BranchInst>(inst) && cast<llvm::BranchInst>(inst).isUnconditional() && isa<llvm::BasicBlock>(*cast<llvm::BranchInst>(inst).getOperand(0)))
                  ; /// goto to basic blocks can be skipped
               else
               {
                  if(firstStmt)
                     serialize_gimple_child("stmt", createGimpleLabelStmt(&BB));
                  firstStmt = false;
                  serialize_gimple_child("stmt", assignCodeAuto(&inst));
               }
            }
         }
         if(firstStmt)
            serialize_gimple_child("stmt", createGimpleLabelStmt(&BB));
      }
      /* Terminate the line.  */
      stream << "\n";
   }

   std::string DumpGimpleRaw::getHeaderForBuiltin(const void* t)
   {
      assert(TREE_CODE(t) == GT(FUNCTION_DECL));
      assert(is_builtin_fn(t));
      const llvm::Function* fd = reinterpret_cast<const llvm::Function*>(t);
      if(fd->hasName())
      {
         std::string declname;
         if(fd->isIntrinsic())
            declname = getIntrinsicName(fd);
         else
            declname = std::string(fd->getName());
         if(declname == "acos" or declname == "acosh" or declname == "asin" or declname == "asinh" or declname == "atan" or declname == "atanh" or declname == "atan2" or declname == "cbrt" or declname == "ceil" or declname == "copysign" or
            declname == "cos" or declname == "cosh" or declname == "erf" or declname == "erfc" or declname == "exp" or declname == "exp2" or declname == "expm1" or declname == "fabs" or declname == "fdim" or declname == "floor" or declname == "fma" or
            declname == "fmax" or declname == "fmin" or declname == "fmod" or declname == "frexp" or declname == "hypot" or declname == "ilogb" or declname == "ldexp" or declname == "lgamma" or declname == "llrint" or declname == "llround" or
            declname == "log" or declname == "log10" or declname == "log1p" or declname == "log2" or declname == "logb" or declname == "lrint" or declname == "lround" or declname == "modf" or declname == "nan" or declname == "nearbyint" or
            declname == "nextafter" or declname == "nexttoward" or declname == "pow" or declname == "remainder" or declname == "remquo" or declname == "rint" or declname == "round" or declname == "scalbln" or declname == "scalbn" or declname == "sin" or
            declname == "sinh" or declname == "sincos" or declname == "sqrt" or declname == "tan" or declname == "tanh" or declname == "tgamma" or declname == "trunc" or declname == "isinf" or declname == "isnan")
            return "/usr/include/math.h";
         else if(declname == "cabs" or declname == "cacos" or declname == "cacosh" or declname == "carg" or declname == "casin" or declname == "casinh" or declname == "catan" or declname == "catanh" or declname == "ccos" or declname == "ccosh" or
                 declname == "cexp" or declname == "cimag" or declname == "clog" or declname == "conj" or declname == "cpow" or declname == "cproj" or declname == "creal" or declname == "csin" or declname == "csinh" or declname == "csqrt" or
                 declname == "ctan" or declname == "ctanh")
            return "/usr/include/complex.h";
         else if(declname == "memchr" or declname == "memcmp" or declname == "memcpy" or declname == "memmove" or declname == "memset" or declname == "strcat" or declname == "strchr" or declname == "strcmp" or declname == "strcpy" or
                 declname == "strcspn" or declname == "strlen" or declname == "strncat" or declname == "strncmp" or declname == "strncpy" or declname == "strpbrk" or declname == "strrchr" or declname == "strspn" or declname == "strstr")
            return "/usr/include/string.h";
         else if(declname == "fprintf" or declname == "putc" or declname == "fputc" or declname == "fputs" or declname == "fscanf" or declname == "fwrite" or declname == "printf" or declname == "putchar" or declname == "puts" or declname == "scanf" or
                 declname == "snprintf" or declname == "sprintf" or declname == "sscanf" or declname == "vfprintf" or declname == "vfscanf" or declname == "vprintf" or declname == "vscanf" or declname == "vsnprintf" or declname == "vsprintf" or
                 declname == "vsscanf")
            return "/usr/include/stdio.h";
         else if(declname == "isalnum" or declname == "isalpha" or declname == "isblank" or declname == "iscntrl" or declname == "isdigit" or declname == "isgraph" or declname == "islower" or declname == "isprint" or declname == "ispunct" or
                 declname == "isspace" or declname == "isupper" or declname == "isxdigit" or declname == "tolower" or declname == "toupper")
            return "/usr/include/ctype.h";
         else if(declname == "iswalnum" or declname == "iswalpha" or declname == "iswblank" or declname == "iswcntrl" or declname == "iswdigit" or declname == "iswgraph" or declname == "iswlower" or declname == "iswprint" or declname == "iswpunct" or
                 declname == "iswspace" or declname == "iswupper" or declname == "iswxdigit" or declname == "towlower" or declname == "towupper")
            return "/usr/include/wctype.h";
         else if(declname == "abort" or declname == "abs" or declname == "calloc" or declname == "exit" or declname == "free" or declname == "labs" or declname == "llabs" or declname == "malloc" or declname == "realloc" or declname == "_exit2" or
                 declname == "aligned_alloc")
            return "/usr/include/stdlib.h";
         else if(declname == "imaxabs")
            return "/usr/include/inttypes.h";
         else if(declname == "strftime")
            return "/usr/include/time.h";
      }
      return "";
   }

   void DumpGimpleRaw::dequeue_and_serialize()
   {
      assert(!Queue.empty());
      const void* t = Queue.front();
      assert(t);
      Queue.pop_front();

      if(setOfGimples.find(t) != setOfGimples.end())
      {
         dequeue_and_serialize_gimple(t);
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

      const char* code_name = GET_TREE_CODE_NAME(TREE_CODE(t));
#if PRINT_DBG_MSG
      llvm::errs() << "|" << code_name << "\n";
#endif
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-16s ", code_name);
      stream << buffer;
      column = 25;

      tree_codes code = TREE_CODE(t);
      tree_codes_class code_class = TREE_CODE_CLASS(code);

      if(IS_EXPR_CODE_CLASS(code_class))
      {
         queue_and_serialize_type(t);
         if(EXPR_HAS_LOCATION(t))
         {
            serialize_maybe_newline();
            snprintf(buffer, LOCAL_BUFFER_LEN, "srcp: \"%s\":%-d:%-6d ", EXPR_FILENAME(t), EXPR_LINENO(t), EXPR_COLUMNNO(t));
            column += 12 + strlen(EXPR_FILENAME(t)) + 8;
         }
         switch(code_class)
         {
            case tcc_unary:
               serialize_child("op", TREE_OPERAND(t, 0));
               break;

            case tcc_binary:
            case tcc_comparison:
               serialize_child("op", TREE_OPERAND(t, 0));
               serialize_child("op", TREE_OPERAND(t, 1));
               break;

            case tcc_expression:
            case tcc_reference:
            case tcc_statement:
            case tcc_vl_exp:
               /* These nodes are handled explicitly below.  */
               break;

            default:
               llvm_unreachable("Unexpected case");
         }
      }
      else if(DECL_P(t))
      {
         expanded_location xloc;
         if(DECL_NAME(t))
            serialize_child("name", DECL_NAME(t));
         if(DECL_ASSEMBLER_NAME_SET_P(t) && DECL_ASSEMBLER_NAME(t) != DECL_NAME(t))
            serialize_child("mngl", DECL_ASSEMBLER_NAME(t));

         if((code == GT(VAR_DECL) || code == GT(ALLOCAVAR_DECL)) && DECL_ALIGN(t) != TYPE_ALIGN(TREE_TYPE(t)))
            serialize_child("orig", DECL_ABSTRACT_ORIGIN(t));
         /* And types.  */
         queue_and_serialize_type(t);
         serialize_child("scpe", DECL_CONTEXT(t));

         if(!DECL_SOURCE_LOCATION(t))
         {
            serialize_maybe_newline();
            /// with clang/llvm there is no type definition
            snprintf(buffer, LOCAL_BUFFER_LEN, "srcp: \"");
            stream << buffer;
            if(code == GT(FUNCTION_DECL) && is_builtin_fn(t) && reinterpret_cast<const llvm::Function*>(t)->getBasicBlockList().empty())
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
            xloc = expand_location(DECL_SOURCE_LOCATION(t));
            if(xloc.file)
            {
               serialize_maybe_newline();
               snprintf(buffer, LOCAL_BUFFER_LEN, "srcp: \"%s\":%-d:%-6d ", xloc.file, xloc.line, xloc.column);
               stream << buffer;
               column += 12 + strlen(xloc.file) + 8;
            }
         }

         if(code == GT(LABEL_DECL))
            serialize_string("artificial");
         serialize_int("uid", DECL_UID(t));
      }
      else if(code_class == tcc_type)
      {
         /* All types have associated declarations.  */
         serialize_child("name", TYPE_NAME(t));
         /* And sizes.  */
         serialize_child("size", TYPE_SIZE(t));

         /* All types have context.  */            /*NB TBC*/
         serialize_child("scpe", TYPE_CONTEXT(t)); /*NB TBC*/

         /* All types have alignments.  */
         serialize_int("algn", TYPE_ALIGN(t));

         if(TYPE_PACKED(t))
         {
            serialize_string("packed");
         }
      }
      else if(code_class == tcc_constant)
         queue_and_serialize_type(t);

      switch(code)
      {
         case GT(IDENTIFIER_NODE):
            serialize_string_field("strg", IDENTIFIER_POINTER(t));
            serialize_int("lngt", IDENTIFIER_LENGTH(t));
            break;

         case GT(TREE_LIST):
            serialize_child("purp", TREE_PURPOSE(t));
            serialize_child("valu", TREE_VALUE(t));
            serialize_child("chan", TREE_CHAIN(t));
            break;
         case GT(TREE_VEC):
         {
            serialize_int("lngt", TREE_VEC_LENGTH(t));
            for(auto i = 0; i < TREE_VEC_LENGTH(t); ++i)
            {
               serialize_child("op", TREE_VEC_ELT(t, i));
            }
            break;
         }

         case GT(SIGNEDPOINTERTYPE):
         case GT(INTEGER_TYPE):
         case GT(ENUMERAL_TYPE):
            serialize_int("prec", TYPE_PRECISION(t));
            if(TYPE_UNSIGNED(t))
               serialize_string("unsigned");
            serialize_child("min", TYPE_MIN_VALUE(t));
            serialize_child("max", TYPE_MAX_VALUE(t));

            if(code == GT(ENUMERAL_TYPE))
               serialize_child("csts", TYPE_VALUES(t));
            break;

         case GT(COMPLEX_TYPE):
            if(TYPE_UNSIGNED(t))
               serialize_string("unsigned");
            if(COMPLEX_FLOAT_TYPE_P(t))
               serialize_string("real");
            break;

         case GT(REAL_TYPE):
            serialize_int("prec", TYPE_PRECISION(t));
            break;

         case GT(FIXED_POINT_TYPE):
            serialize_int("prec", TYPE_PRECISION(t));
            serialize_string_field("sign", TYPE_UNSIGNED(t) ? "unsigned" : "signed");
            serialize_string_field("saturating", TYPE_SATURATING(t) ? "saturating" : "non-saturating");
            break;

         case GT(POINTER_TYPE):
            serialize_child("ptd", TREE_TYPE(t));
            break;

         case GT(REFERENCE_TYPE):
            serialize_child("refd", TREE_TYPE(t));
            break;

         case GT(METHOD_TYPE):
            serialize_child("retn", TREE_TYPE(t));
            serialize_child("prms", TYPE_ARG_TYPES(t));
            serialize_child("clas", TYPE_METHOD_BASETYPE(t));
            break;

         case GT(FUNCTION_TYPE):
         {
            serialize_child("retn", TREE_TYPE(t));
            auto args = TYPE_ARG_TYPES(t);
            serialize_child("prms", args);
            if(args && stdarg_p(t)) // ISO C requires a named parameter before '...'
               serialize_string("varargs");
            break;
         }
         case GT(ARRAY_TYPE):
            serialize_child("elts", TREE_TYPE(t));
            serialize_child("domn", TYPE_DOMAIN(t));
            break;

         case GT(VECTOR_TYPE):
            serialize_child("elts", TREE_TYPE(t));
            break;

         case GT(RECORD_TYPE):
         case GT(UNION_TYPE):
         {
            if(TREE_CODE(t) == GT(RECORD_TYPE))
               serialize_string("struct");
            else
               serialize_string("union");
            if(!TYPE_FIELDS(t).empty())
            {
               unsigned int counter = 0;
               for(auto op : TYPE_FIELDS(t))
               {
                  assignCodeType(op);
                  if(op->isFunctionTy())
                     serialize_child("fncs", GET_METHOD_TYPE(op, counter, t));
                  else
                     serialize_child("flds", GET_FIELD_DECL(op, counter, t));
                  ++counter;
               }
            }
            break;
         }

         case GT(SSA_NAME):
         {
            queue_and_serialize_type(t);
            serialize_child("var", SSA_NAME_VAR(t));
            auto vers = SSA_NAME_VERSION(t);
            //            llvm::errs() << "vers: " << vers << "\n";
            serialize_int("vers", vers);
            if(POINTER_TYPE_P(TREE_TYPE(t)) && SSA_NAME_PTR_INFO(t))
               dump_pt_solution(&(SSA_NAME_PTR_INFO(t)->pt), "use", "use_vars");
            //            if (TREE_THIS_VOLATILE (t))
            //              serialize_string("volatile");
            //            else
            serialize_gimple_child("def_stmt", SSA_NAME_DEF_STMT(t));
            if(is_virtual_ssa(t))
               serialize_string("virtual");
            if(SSA_NAME_IS_DEFAULT_DEF(t))
               serialize_string("default");
            serialize_child("min", getMinValue(t));
            serialize_child("max", getMaxValue(t));

            break;
         }
         case GT(ALLOCAVAR_DECL):
         case GT(ORIGVAR_DECL):
         case GT(VAR_DECL):
         case GT(PARM_DECL):
         case GT(FIELD_DECL):
         case GT(RESULT_DECL):
            if(TREE_CODE(t) == GT(FIELD_DECL) && DECL_C_BIT_FIELD(t))
               serialize_string("bitfield");
            if(TREE_CODE(t) == GT(VAR_DECL) || TREE_CODE(t) == GT(ORIGVAR_DECL))
            {
               if(DECL_EXTERNAL(t))
                  serialize_string("extern");
               else if(!TREE_PUBLIC(t) && TREE_STATIC(t))
                  serialize_string("static");
            }
            if(TREE_CODE(t) == GT(PARM_DECL))
               serialize_child("argt", DECL_ARG_TYPE(t));
            else if(DECL_INITIAL(t))
               serialize_child("init", DECL_INITIAL(t));
            serialize_child("size", DECL_SIZE(t));
            serialize_int("algn", DECL_ALIGN(t));
            if(TREE_CODE(t) == GT(FIELD_DECL) && DECL_PACKED(t))
            {
               serialize_string("packed");
            }

            if(TREE_CODE(t) == GT(FIELD_DECL))
            {
               if(DECL_FIELD_OFFSET(t))
                  serialize_child("bpos", bit_position(t));
            }
            else if(TREE_CODE(t) == GT(ALLOCAVAR_DECL) || TREE_CODE(t) == GT(ORIGVAR_DECL) || TREE_CODE(t) == GT(VAR_DECL) || TREE_CODE(t) == GT(PARM_DECL))
            {
               serialize_int("used", TREE_USED(t));
               if(DECL_REGISTER(t))
                  serialize_string("register");
            }
            if(TREE_READONLY(t) && TREE_CODE(t) != GT(RESULT_DECL) && TREE_CODE(t) != GT(FIELD_DECL))
               serialize_string("readonly");
#if HAVE_LIBBDD
            if(TREE_CODE(t) == GT(ALLOCAVAR_DECL) && PtoSets_AA)
            {
               if(TREE_ADDRESSABLE(t))
                  ; // serialize_string("addr_taken");
               else
                  serialize_string("addr_not_taken");
            }
#endif
            break;
         case GT(FUNCTION_DECL):
         {
            for(const auto arg : DECL_ARGUMENTS(t))
            {
               serialize_child("arg", arg);
            }
            if(DECL_EXTERNAL(t))
               serialize_string("undefined");
            if(is_builtin_fn(t))
               serialize_string("builtin");
            if(!TREE_PUBLIC(t))
               serialize_string("static");
            if(!DECL_EXTERNAL(t))
               serialize_statement_child("body", getStatement_list(t));

            break;
         }
         case GT(INTEGER_CST_SIGNED):
         case GT(INTEGER_CST):
            serialize_wide_int("value", TREE_INT_CST_LOW(t));
            break;

         case GT(STRING_CST):
            llvm_unreachable("Unexpected. Strings should be converted in standard arrays");
            //           if (TREE_TYPE (t))
            //             serialize_string_cst("strg" , TREE_STRING_POINTER (t), TREE_STRING_LENGTH (t), TYPE_ALIGN(TREE_TYPE (t)));
            //           else
            //             serialize_string_cst("strg" , TREE_STRING_POINTER (t), TREE_STRING_LENGTH (t), 8);
            break;

         case GT(REAL_CST):
            serialize_real(t);
            break;

            //         case GT(COMPLEX_CST):
            //           serialize_child ("real", TREE_REALPART (t));
            //           serialize_child ("imag", TREE_IMAGPART (t));
            //           break;

            //         case GT(FIXED_CST):
            //           serialize_fixed ("valu", TREE_FIXED_CST_PTR (t));
            //           break;

         case GT(TRUTH_NOT_EXPR):
         case GT(ADDR_EXPR):
         case GT(VIEW_CONVERT_EXPR):
            /* These nodes are unary, but do not have code class `1'.  */
            serialize_child("op", TREE_OPERAND(t, 0));
            break;

         case GT(MEM_REF):
            serialize_child("op", TREE_OPERAND(t, 0));
            serialize_child("op", TREE_OPERAND(t, 1));
            break;

         case GT(MISALIGNED_INDIRECT_REF):
            serialize_child("op", TREE_OPERAND(t, 0));
            break;
         case GT(COND_EXPR):
            serialize_child("op", TREE_OPERAND(t, 0));
            serialize_child("op", TREE_OPERAND(t, 1));
            serialize_child("op", TREE_OPERAND(t, 2));
            break;

         case GT(CALL_EXPR):
         {
            serialize_child("fn", call_expr_fn(t));
            unsigned int arg_index;
            for(arg_index = 0; arg_index < call_expr_num_args(t); arg_index++)
            {
               serialize_child("arg", call_expr_arg(t, arg_index));
            }
            break;
         }
         case GT(CONSTRUCTOR):
         {
            queue_and_serialize_type(t);
            for(const auto& el : CONSTRUCTOR_ELTS(t))
            {
               serialize_child("idx", el.first);
               serialize_child("valu", el.second);
            }
            break;
         }
         case GT(CASE_LABEL_EXPR):
         {
            if(CASE_LOW(t) && CASE_HIGH(t))
            {
               serialize_child("op", CASE_LOW(t));
               serialize_child("op", CASE_HIGH(t));
               serialize_child("goto", CASE_LABEL(t));
            }
            else if(CASE_LOW(t))
            {
               serialize_child("op", CASE_LOW(t));
               serialize_child("goto", CASE_LABEL(t));
            }
            else
            {
               serialize_string("default");
               serialize_child("goto", CASE_LABEL(t));
            }
            break;
         }
         case GT(TRUTH_ANDIF_EXPR):
         case GT(TRUTH_ORIF_EXPR):
            serialize_child("op", TREE_OPERAND(t, 0));
            serialize_child("op", TREE_OPERAND(t, 1));
            break;

         default:
            /* There are no additional fields to print.  */
            break;
      }
      stream << "\n";
   }

   void DumpGimpleRaw::SerializeGimpleFunctionHeader(const void* obj)
   {
      assert(TREE_CODE(obj) == GT(FUNCTION_DECL));
      const llvm::Function* fd = reinterpret_cast<const llvm::Function*>(obj);
      stream << "\n;; Function " << fd->getName() << "(" << fd->getName() << ")\n\n";
      stream << ";; " << fd->getName() << "(";
      stream << ")\n";
   }

   unsigned int DumpGimpleRaw::queue(const void* obj)
   {
      unsigned int index = ++last_used_index;
      assert(llvm2index.find(obj) == llvm2index.end());
      llvm2index[obj] = index;
      assert(HAS_CODE(obj));
      Queue.push_back(obj);
      return index;
   }

   void DumpGimpleRaw::SerializeGimpleGlobalTreeNode(const void* obj)
   {
      if(TREE_CODE(obj) == GT(FUNCTION_DECL))
      {
         SerializeGimpleFunctionHeader(obj);
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
         case llvm::Intrinsic::fabs:
         case llvm::Intrinsic::memcpy:
         case llvm::Intrinsic::memset:
         case llvm::Intrinsic::memmove:
         case llvm::Intrinsic::trap:
         case llvm::Intrinsic::rint:
         case llvm::Intrinsic::fmuladd:
         case llvm::Intrinsic::minnum:
         case llvm::Intrinsic::maxnum:
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
            return true;
         default:
            return false;
      }
   }

   static bool addrIsOfIntArrayType(llvm::Value* DstAddr, unsigned& Align, const llvm::DataLayout* DL)
   {
      llvm::Type* srcType = nullptr;
      if(llvm::dyn_cast<llvm::BitCastInst>(DstAddr))
      {
         srcType = llvm::cast<llvm::BitCastInst>(DstAddr)->getSrcTy();
      }
      else if(llvm::dyn_cast<llvm::ConstantExpr>(DstAddr) && cast<llvm::ConstantExpr>(DstAddr)->getOpcode() == llvm::Instruction::BitCast)
      {
         srcType = cast<llvm::ConstantExpr>(DstAddr)->getOperand(0)->getType();
      }
      if(srcType)
      {
         if(srcType->isPointerTy())
         {
            auto pointee = llvm::cast<llvm::PointerType>(srcType)->getElementType();
            if(pointee->isArrayTy())
            {
               auto elType = llvm::cast<llvm::ArrayType>(pointee)->getArrayElementType();
               auto size = elType->isSized() ? DL->getTypeAllocSizeInBits(elType) : 8ULL;
               if(size <= Align * 8)
               {
                  Align = size / 8;
                  return elType->isIntegerTy();
               }
            }
         }
      }
      return false;
   }
   static unsigned getLoopOperandSizeInBytesLocal(llvm::Type* Type)
   {
      if(llvm::VectorType* VTy = dyn_cast<llvm::VectorType>(Type))
      {
         return VTy->getBitWidth() / 8;
      }
      return Type->getPrimitiveSizeInBits() / 8;
   }

   static llvm::Type* getMemcpyLoopLoweringTypeLocal(llvm::LLVMContext& Context, llvm::ConstantInt* Length, unsigned SrcAlign, unsigned DestAlign, llvm::Value* SrcAddr, llvm::Value* DstAddr, const llvm::DataLayout* DL, bool isVolatile, bool& Optimize)
   {
      if(!isVolatile)
      {
         if(SrcAlign == DestAlign && DestAlign == Length->getZExtValue() && DestAlign <= 8)
         {
#if PRINT_DBG_MSG
            llvm::errs() << "memcpy can be optimized\n";
            llvm::errs() << "Align=" << SrcAlign << "\n";
#endif
            return llvm::Type::getIntNTy(Context, SrcAlign * 8);
         }
         unsigned localSrcAlign = SrcAlign;
         auto srcCheck = addrIsOfIntArrayType(SrcAddr, localSrcAlign, DL);
         if(srcCheck)
         {
            unsigned localDstAlign = DestAlign;
            auto dstCheck = addrIsOfIntArrayType(DstAddr, localDstAlign, DL);
            if(dstCheck && localSrcAlign == localDstAlign)
            {
               Optimize = true;
#if PRINT_DBG_MSG
               llvm::errs() << "memcpy can be optimized\n";
               llvm::errs() << "Align=" << SrcAlign << "\n";
#endif
               return llvm::Type::getIntNTy(Context, SrcAlign * 8);
            }
         }
      }
      return llvm::Type::getInt8Ty(Context);
   }

   static void getMemcpyLoopResidualLoweringTypeLocal(llvm::SmallVectorImpl<llvm::Type*>& OpsOut, llvm::LLVMContext& Context, unsigned RemainingBytes, unsigned SrcAlign, unsigned DestAlign)
   {
      for(unsigned i = 0; i != RemainingBytes; ++i)
         OpsOut.push_back(llvm::Type::getInt8Ty(Context));
   }

   static void createMemCpyLoopKnownSizeLocal(llvm::Instruction* InsertBefore, llvm::Value* SrcAddr, llvm::Value* DstAddr, llvm::ConstantInt* CopyLen, unsigned SrcAlign, unsigned DestAlign, bool SrcIsVolatile, bool DstIsVolatile,
                                              const llvm::DataLayout* DL)
   {
      // No need to expand zero length copies.
      if(CopyLen->isZero())
         return;

      llvm::BasicBlock* PreLoopBB = InsertBefore->getParent();
      llvm::BasicBlock* PostLoopBB = nullptr;
      llvm::Function* ParentFunc = PreLoopBB->getParent();
      llvm::LLVMContext& Ctx = PreLoopBB->getContext();

      bool PeelCandidate = false;
      llvm::Type* TypeOfCopyLen = CopyLen->getType();
      llvm::Type* LoopOpType = getMemcpyLoopLoweringTypeLocal(Ctx, CopyLen, SrcAlign, DestAlign, SrcAddr, DstAddr, DL, SrcIsVolatile || DstIsVolatile, PeelCandidate);

      unsigned LoopOpSize = getLoopOperandSizeInBytesLocal(LoopOpType);
      uint64_t LoopEndCount = CopyLen->getZExtValue() / LoopOpSize;

      unsigned SrcAS = cast<llvm::PointerType>(SrcAddr->getType())->getAddressSpace();
      unsigned DstAS = cast<llvm::PointerType>(DstAddr->getType())->getAddressSpace();

      uint64_t BytesCopied = LoopEndCount * LoopOpSize;
      uint64_t RemainingBytes = CopyLen->getZExtValue() - BytesCopied;

      bool do_unrolling;
#if __clang_major__ == 7
      do_unrolling = false;
#else
      do_unrolling = true;
#endif
      bool srcIsAlloca = false;
      bool srcIsGlobal = false;
      if(llvm::dyn_cast<llvm::BitCastInst>(SrcAddr) && dyn_cast<llvm::AllocaInst>(llvm::dyn_cast<llvm::BitCastInst>(SrcAddr)->getOperand(0)))
      {
         srcIsAlloca = true;
         do_unrolling = do_unrolling && (LoopEndCount <= PEEL_THRESHOLD);
      }
      else if(llvm::dyn_cast<llvm::ConstantExpr>(SrcAddr) && cast<llvm::ConstantExpr>(SrcAddr)->getOpcode() == llvm::Instruction::BitCast && dyn_cast<llvm::GlobalVariable>(cast<llvm::ConstantExpr>(SrcAddr)->getOperand(0)))
      {
         srcIsGlobal = true;
         do_unrolling = do_unrolling && (dyn_cast<llvm::GlobalVariable>(cast<llvm::ConstantExpr>(SrcAddr)->getOperand(0))->isConstant() || (LoopEndCount <= PEEL_THRESHOLD));
      }
      else if(LoopEndCount == 1)
         do_unrolling = true;
      if(do_unrolling && !SrcIsVolatile && !DstIsVolatile && (srcIsAlloca || srcIsGlobal) && llvm::dyn_cast<llvm::BitCastInst>(DstAddr) && PeelCandidate)
      {
         llvm::PointerType* SrcOpType = llvm::PointerType::get(LoopOpType, SrcAS);
         llvm::PointerType* DstOpType = llvm::PointerType::get(LoopOpType, DstAS);
         llvm::IRBuilder<> Builder(InsertBefore);
         auto srcAddress = Builder.CreateBitCast(srcIsAlloca ? llvm::dyn_cast<llvm::BitCastInst>(SrcAddr)->getOperand(0) : cast<llvm::ConstantExpr>(SrcAddr)->getOperand(0), SrcOpType);
         auto dstAddress = Builder.CreateBitCast(llvm::dyn_cast<llvm::BitCastInst>(DstAddr)->getOperand(0), DstOpType);
         for(auto LI = 0u; LI < LoopEndCount; ++LI)
         {
            llvm::Value* SrcGEP = Builder.CreateInBoundsGEP(LoopOpType, srcAddress, llvm::ConstantInt::get(TypeOfCopyLen, LI));
            llvm::Value* Load = Builder.CreateLoad(SrcGEP, SrcIsVolatile);
            llvm::Value* DstGEP = Builder.CreateInBoundsGEP(LoopOpType, dstAddress, llvm::ConstantInt::get(TypeOfCopyLen, LI));
            Builder.CreateStore(Load, DstGEP, DstIsVolatile);
         }
      }
      else if(LoopEndCount != 0)
      {
         // Split
         PostLoopBB = PreLoopBB->splitBasicBlock(InsertBefore, "memcpy-split");
         llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(Ctx, "load-store-loop", ParentFunc, PostLoopBB);
         PreLoopBB->getTerminator()->setSuccessor(0, LoopBB);

         llvm::IRBuilder<> PLBuilder(PreLoopBB->getTerminator());

         // Cast the Src and Dst pointers to pointers to the loop operand type (if
         // needed).
         llvm::PointerType* SrcOpType = llvm::PointerType::get(LoopOpType, SrcAS);
         llvm::PointerType* DstOpType = llvm::PointerType::get(LoopOpType, DstAS);
         if(SrcAddr->getType() != SrcOpType)
         {
            SrcAddr = PLBuilder.CreateBitCast(SrcAddr, SrcOpType);
         }
         if(DstAddr->getType() != DstOpType)
         {
            DstAddr = PLBuilder.CreateBitCast(DstAddr, DstOpType);
         }

         llvm::IRBuilder<> LoopBuilder(LoopBB);
         llvm::PHINode* LoopIndex = LoopBuilder.CreatePHI(TypeOfCopyLen, 2, "loop-index");
         LoopIndex->addIncoming(llvm::ConstantInt::get(TypeOfCopyLen, 0U), PreLoopBB);
         // Loop Body
         llvm::Value* SrcGEP = LoopBuilder.CreateInBoundsGEP(LoopOpType, SrcAddr, LoopIndex);
         llvm::Value* Load = LoopBuilder.CreateLoad(SrcGEP, SrcIsVolatile);
         llvm::Value* DstGEP = LoopBuilder.CreateInBoundsGEP(LoopOpType, DstAddr, LoopIndex);
         LoopBuilder.CreateStore(Load, DstGEP, DstIsVolatile);

         llvm::Value* NewIndex = LoopBuilder.CreateAdd(LoopIndex, llvm::ConstantInt::get(TypeOfCopyLen, 1U));
         LoopIndex->addIncoming(NewIndex, LoopBB);

         // Create the loop branch condition.
         llvm::Constant* LoopEndCI = llvm::ConstantInt::get(TypeOfCopyLen, LoopEndCount);
         LoopBuilder.CreateCondBr(LoopBuilder.CreateICmpULT(NewIndex, LoopEndCI), LoopBB, PostLoopBB);
      }

      if(RemainingBytes)
      {
         llvm::IRBuilder<> RBuilder(PostLoopBB ? PostLoopBB->getFirstNonPHI() : InsertBefore);

         // Update the alignment based on the copy size used in the loop body.
         SrcAlign = std::min(SrcAlign, LoopOpSize);
         DestAlign = std::min(DestAlign, LoopOpSize);

         llvm::SmallVector<llvm::Type*, 5> RemainingOps;
         getMemcpyLoopResidualLoweringTypeLocal(RemainingOps, Ctx, RemainingBytes, SrcAlign, DestAlign);

         for(auto OpTy : RemainingOps)
         {
            // Calculate the new index
            unsigned OperandSize = getLoopOperandSizeInBytesLocal(OpTy);
            uint64_t GepIndex = BytesCopied / OperandSize;
            assert(GepIndex * OperandSize == BytesCopied && "Division should have no Remainder!");
            // Cast source to operand type and load
            llvm::PointerType* SrcPtrType = llvm::PointerType::get(OpTy, SrcAS);
            llvm::Value* CastedSrc = SrcAddr->getType() == SrcPtrType ? SrcAddr : RBuilder.CreateBitCast(SrcAddr, SrcPtrType);
            llvm::Value* SrcGEP = RBuilder.CreateInBoundsGEP(OpTy, CastedSrc, llvm::ConstantInt::get(TypeOfCopyLen, GepIndex));
            llvm::Value* Load = RBuilder.CreateLoad(SrcGEP, SrcIsVolatile);

            // Cast destination to operand type and store.
            llvm::PointerType* DstPtrType = llvm::PointerType::get(OpTy, DstAS);
            llvm::Value* CastedDst = DstAddr->getType() == DstPtrType ? DstAddr : RBuilder.CreateBitCast(DstAddr, DstPtrType);
            llvm::Value* DstGEP = RBuilder.CreateInBoundsGEP(OpTy, CastedDst, llvm::ConstantInt::get(TypeOfCopyLen, GepIndex));
            RBuilder.CreateStore(Load, DstGEP, DstIsVolatile);

            BytesCopied += OperandSize;
         }
      }
      assert(BytesCopied == CopyLen->getZExtValue() && "Bytes copied should match size in the call!");
   }

   static void expandMemSetAsLoopLocal(llvm::MemSetInst* Memset, const llvm::DataLayout* DL)
   {
      llvm::Instruction* InsertBefore = Memset;
      llvm::Value* DstAddr = Memset->getRawDest();
      llvm::Value* CopyLen = Memset->getLength();
      llvm::Value* SetValue = Memset->getValue();
#if __clang_major__ >= 7
      unsigned Align = Memset->getDestAlignment();
#else
      unsigned Align = Memset->getAlignment();
#endif
      bool IsVolatile = Memset->isVolatile();
      llvm::Type* TypeOfCopyLen = CopyLen->getType();
      llvm::BasicBlock* OrigBB = InsertBefore->getParent();
      llvm::Function* F = OrigBB->getParent();
      llvm::BasicBlock* NewBB = OrigBB->splitBasicBlock(InsertBefore, "split");
      llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(F->getContext(), "loadstoreloop", F, NewBB);

      addrIsOfIntArrayType(DstAddr, Align, DL);

      bool AlignCanBeUsed = false;
      if(isa<llvm::ConstantInt>(CopyLen) && isa<llvm::Constant>(SetValue) && cast<llvm::Constant>(SetValue)->isNullValue() && Align > 1 && Align <= 8 && SetValue->getType()->isIntegerTy())
         AlignCanBeUsed = true;
      if(AlignCanBeUsed)
      {
#if PRINT_DBG_MSG
         llvm::errs() << "memset can be optimized\n";
         llvm::errs() << "Align=" << Align << "\n";
#endif
         SetValue = llvm::ConstantInt::get(llvm::Type::getIntNTy(F->getContext(), Align * 8), 0);
      }
#if PRINT_DBG_MSG
      else
         llvm::errs() << "memset cannot be optimized\n";
#endif
      llvm::IRBuilder<> Builder(OrigBB->getTerminator());

      auto ActualCopyLen = AlignCanBeUsed ? llvm::ConstantInt::get(TypeOfCopyLen, cast<llvm::ConstantInt>(CopyLen)->getValue().udiv(llvm::APInt(TypeOfCopyLen->getIntegerBitWidth(), Align))) : CopyLen;

      // Cast pointer to the type of value getting stored
      unsigned dstAS = cast<llvm::PointerType>(DstAddr->getType())->getAddressSpace();
      DstAddr = Builder.CreateBitCast(DstAddr, llvm::PointerType::get(SetValue->getType(), dstAS));

      Builder.CreateCondBr(Builder.CreateICmpEQ(llvm::ConstantInt::get(TypeOfCopyLen, 0), CopyLen), NewBB, LoopBB);
      OrigBB->getTerminator()->eraseFromParent();

      llvm::IRBuilder<> LoopBuilder(LoopBB);
      llvm::PHINode* LoopIndex = LoopBuilder.CreatePHI(TypeOfCopyLen, 0);
      LoopIndex->addIncoming(llvm::ConstantInt::get(TypeOfCopyLen, 0), OrigBB);

      if(AlignCanBeUsed)
         LoopBuilder.CreateAlignedStore(SetValue, LoopBuilder.CreateInBoundsGEP(SetValue->getType(), DstAddr, LoopIndex), Align, IsVolatile);
      else
         LoopBuilder.CreateStore(SetValue, LoopBuilder.CreateInBoundsGEP(SetValue->getType(), DstAddr, LoopIndex), IsVolatile);

      llvm::Value* NewIndex = LoopBuilder.CreateAdd(LoopIndex, llvm::ConstantInt::get(TypeOfCopyLen, 1));
      LoopIndex->addIncoming(NewIndex, LoopBB);

      LoopBuilder.CreateCondBr(LoopBuilder.CreateICmpULT(NewIndex, ActualCopyLen), LoopBB, NewBB);
   }

   void DumpGimpleRaw::buildMetaDataMap(llvm::Module& M)
   {
      for(auto& fun : M.getFunctionList())
      {
         if(!fun.isIntrinsic() && !fun.isDeclaration())
         {
            for(const auto& BB : fun.getBasicBlockList())
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
                        {
                           //                           llvm::errs() << "Inst: ";
                           //                           inst.print(llvm::errs());
                           //                           llvm::errs() << "\n";
                           //                           llvm::errs() << "Value: ";
                           //                           val->print(llvm::errs());
                           //                           llvm::errs() << "\n";
                           //                           llvm::errs() << "Metadata: ";
                           //                           dbgInstrCall->getRawVariable()->print(llvm::errs());
                           //                           llvm::errs() <<"\n";
                        }
                     }
                  }
               }
            }
         }
      }
   }

   /// Intrinsic lowering
   bool DumpGimpleRaw::lowerMemIntrinsics(llvm::Module& M)
   {
      auto res = false;
      auto currFuncIterator = M.getFunctionList().begin();
      while(currFuncIterator != M.getFunctionList().end())
      {
         auto& F = *currFuncIterator;
         const llvm::TargetTransformInfo& TTI = modulePass->getAnalysis<llvm::TargetTransformInfoWrapperPass>().getTTI(F);
         auto fname = std::string(currFuncIterator->getName());
         llvm::SmallVector<llvm::MemIntrinsic*, 4> MemCalls;
         for(llvm::Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI)
         {
            for(llvm::BasicBlock::iterator II = BI->begin(), IE = BI->end(); II != IE; ++II)
            {
               if(llvm::MemIntrinsic* InstrCall = dyn_cast<llvm::MemIntrinsic>(II))
               {
#if PRINT_DBG_MSG
                  llvm::errs() << "Found a memIntrinsic Call\n";
#endif
                  MemCalls.push_back(InstrCall);
#if PRINT_DBG_MSG
                  if(llvm::MemCpyInst* Memcpy = dyn_cast<llvm::MemCpyInst>(InstrCall))
                  {
                     if(llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(Memcpy->getLength()))
                        llvm::errs() << "Found a memcpy with a constant number of iterations\n";
                     else
                        llvm::errs() << "Found a memcpy with an unknown number of iterations\n";
                  }
                  else if(llvm::MemSetInst* Memset = dyn_cast<llvm::MemSetInst>(InstrCall))
                  {
                     llvm::errs() << "Found a memset intrinsic\n";
                  }
#endif
               }
            }
         }
         // Transform mem* intrinsic calls.
         for(llvm::MemIntrinsic* MemCall : MemCalls)
         {
            bool do_erase;
            do_erase = false;
            if(llvm::MemCpyInst* Memcpy = dyn_cast<llvm::MemCpyInst>(MemCall))
            {
               if(llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(Memcpy->getLength()))
               {
                  createMemCpyLoopKnownSizeLocal(Memcpy, Memcpy->getRawSource(), Memcpy->getRawDest(), CI,
#if __clang_major__ > 6
                                                 Memcpy->getSourceAlignment(), Memcpy->getDestAlignment(),
#else
                                                 Memcpy->getAlignment(), Memcpy->getAlignment(),
#endif
                                                 Memcpy->isVolatile(), Memcpy->isVolatile(), DL);
                  do_erase = true;
               }
#if __clang_major__ != 4
               else
               {
                  llvm::expandMemCpyAsLoop(Memcpy, TTI);
                  do_erase = true;
               }
#endif
            }
            else if(llvm::MemMoveInst* Memmove = dyn_cast<llvm::MemMoveInst>(MemCall))
            {
#if __clang_major__ != 4
               llvm::expandMemMoveAsLoop(Memmove);
               do_erase = true;
#endif
            }
            else if(llvm::MemSetInst* Memset = dyn_cast<llvm::MemSetInst>(MemCall))
            {
               expandMemSetAsLoopLocal(Memset, DL);
               do_erase = true;
            }
            if(do_erase)
               MemCall->eraseFromParent();
         }
         ++currFuncIterator;
      }
      return res;
   }

   static bool isSimpleEnoughPointerToCommitLocal(llvm::Constant* C)
   {
      // Conservatively, avoid aggregate types. This is because we don't
      // want to worry about them partially overlapping other stores.
      if(!cast<llvm::PointerType>(C->getType())->getElementType()->isSingleValueType())
         return false;

      if(llvm::GlobalVariable* GV = dyn_cast<llvm::GlobalVariable>(C))
         // Do not allow weak/*_odr/linkonce linkage or external globals.
         return GV->hasUniqueInitializer();

      if(llvm::ConstantExpr* CE = dyn_cast<llvm::ConstantExpr>(C))
      {
         // Handle a constantexpr gep.
         if(CE->getOpcode() == llvm::Instruction::GetElementPtr && isa<llvm::GlobalVariable>(CE->getOperand(0)) && cast<llvm::GEPOperator>(CE)->isInBounds())
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
            if(!CE->isGEPWithNoNotionalOverIndexing())
               return false;

            return ConstantFoldLoadThroughGEPConstantExpr(GV->getInitializer(), CE);

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
   static llvm::Constant* ConstantFoldLoadThroughBitcastLocal(llvm::Constant* C, llvm::Type* DestTy, const llvm::DataLayout& DL)
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
   static llvm::Constant* EvaluateStoreIntoLocal(llvm::Constant* Init, llvm::Constant* Val, llvm::ConstantExpr* Addr, unsigned OpNo)
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
      llvm::SequentialType* InitTy = cast<llvm::SequentialType>(Init->getType());
      uint64_t NumElts = InitTy->getNumElements();

      // Break up the array into elements.
      for(uint64_t i = 0, e = NumElts; i != e; ++i)
         Elts.push_back(Init->getAggregateElement(i));

      assert(CI->getZExtValue() < NumElts);
      Elts[CI->getZExtValue()] = EvaluateStoreIntoLocal(Elts[CI->getZExtValue()], Val, Addr, OpNo + 1);

      if(Init->getType()->isArrayTy())
         return llvm::ConstantArray::get(cast<llvm::ArrayType>(InitTy), Elts);
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
            unsigned NumElts;
            if(auto* STy = dyn_cast<llvm::StructType>(Ty))
               NumElts = STy->getNumElements();
            else
               NumElts = cast<llvm::SequentialType>(Ty)->getNumElements();
            for(unsigned i = 0, e = NumElts; i != e; ++i)
               Elts.push_back(Init->getAggregateElement(i));
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

   static bool removableStore(llvm::StoreInst* SI, llvm::GlobalVariable* GV, llvm::TargetLibraryInfo& TLI, const llvm::DataLayout& DL, llvm::Constant*& Ptr, llvm::Constant*& Val, bool firstPart)
   {
      if(!SI->isSimple())
      {
         // llvm::errs() << "Store is not simple! Can not evaluate.\n";
         return false; // no volatile/atomic accesses.
      }

      if(!dyn_cast<llvm::Constant>(SI->getOperand(1)))
      {
         // llvm::errs() << "Ptr is not constant.\n";
         return false;
      }
      Ptr = dyn_cast<llvm::Constant>(SI->getOperand(1));
      if(auto* FoldedPtr = llvm::ConstantFoldConstant(Ptr, DL, &TLI))
      {
         // llvm::errs() << "Folding constant ptr expression: " << *Ptr;
         Ptr = FoldedPtr;
         // llvm::errs() << "; To: " << *Ptr << "\n";
      }
      if(!isSimpleEnoughPointerToCommitLocal(Ptr))
      {
         // If this is too complex for us to commit, reject it.
         // llvm::errs() << "Pointer is too complex for us to evaluate store.";
         return false;
      }
      if(!dyn_cast<llvm::Constant>(SI->getOperand(0)))
      {
         // llvm::errs() << "Value stored is not constant.\n";
         return false;
      }
      Val = dyn_cast<llvm::Constant>(SI->getOperand(0));
      if(llvm::ConstantExpr* CE = dyn_cast<llvm::ConstantExpr>(Ptr))
      {
         if(CE->getOpcode() == llvm::Instruction::BitCast)
         {
            // llvm::errs() << "Attempting to resolve bitcast on constant ptr.\n";
            // If we're evaluating a store through a bitcast, then we need
            // to pull the bitcast off the pointer type and push it onto the
            // stored value.
            Ptr = CE->getOperand(0);

            llvm::Type* NewTy = cast<llvm::PointerType>(Ptr->getType())->getElementType();

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
                  // llvm::errs() << "Failed to bitcast constant ptr, can not evaluate.\n";
                  return false;
               }
            }
            Val = NewVal;
            // llvm::errs() << "Evaluated bitcast: " << *Val << "\n";
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

   llvm::Constant* ComputeLoadResultLocal(llvm::Constant* P, llvm::DenseMap<llvm::Constant*, llvm::Constant*>& MutatedMemory, const llvm::DataLayout& DL)
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
                  return llvm::ConstantFoldLoadThroughGEPConstantExpr(I, CE);
               break;
               // Handle a constantexpr bitcast.
            case llvm::Instruction::BitCast:
               llvm::Constant* Val = dyn_cast<llvm::Constant>(CE->getOperand(0));
               auto MM = MutatedMemory.find(Val);
               auto* I = (MM != MutatedMemory.end()) ? MM->second : getInitializerLocal(CE->getOperand(0));
               if(I)
                  return ConstantFoldLoadThroughBitcastLocal(I, P->getType()->getPointerElementType(), DL);
               break;
         }
      }

      return nullptr; // don't know how to evaluate.
   }

   static void updateLoads(llvm::GlobalVariable* GV, llvm::DenseMap<llvm::Constant*, llvm::Constant*>& MutatedMemory, llvm::TargetLibraryInfo& TLI, const llvm::DataLayout& DL, std::list<llvm::Instruction*>& deadList)
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
                     auto InstResult = ComputeLoadResultLocal(Ptr, MutatedMemory, DL);
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
   bool DumpGimpleRaw::RebuildConstants(llvm::Module& M)
   {
      llvm::TargetLibraryInfo& TLI = modulePass->getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();
      llvm::SmallPtrSet<llvm::GlobalVariable*, 8> Invariants;
      llvm::SmallPtrSet<llvm::Instruction*, 8> Stores;
      auto res = false;
      auto currFuncIterator = M.getFunctionList().begin();
      while(currFuncIterator != M.getFunctionList().end())
      {
         auto fname = std::string(currFuncIterator->getName());
         auto& F = *currFuncIterator;
         std::list<llvm::Instruction*> deadList;
         for(llvm::Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI)
         {
            for(llvm::BasicBlock::iterator II = BI->begin(), IE = BI->end(); II != IE; ++II)
            {
               if(isa<llvm::CallInst>(II) || isa<llvm::InvokeInst>(II))
               {
                  llvm::CallSite CS(&*II);
                  llvm::IntrinsicInst* IntInst = dyn_cast<llvm::IntrinsicInst>(CS.getInstruction());
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
                              llvm::errs() << "Found a global var that is an invariant: " << *GV << "\n";
                              for(llvm::BasicBlock::iterator CurInst = BI->begin(); CurInst != II;)
                              {
                                 llvm::Constant* Val;
                                 llvm::Constant* Ptr;
                                 assert(dyn_cast<llvm::StoreInst>(CurInst) && removableStore(dyn_cast<llvm::StoreInst>(CurInst), GV, TLI, *DL, Ptr, Val, false));
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
                                 llvm::Constant* Val;
                                 llvm::Constant* Ptr;
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
                                          char* demangled_outbuffer = abi::__cxa_demangle(declname.c_str(), nullptr, nullptr, &status);
                                          if(status == 0)
                                          {
                                             declname = demangled_outbuffer;
                                             if(declname.find("guard variable for") != std::string::npos)
                                             {
                                                std::string declname1 = std::string(GV->getName());
                                                char* demangled_outbuffer1 = abi::__cxa_demangle(declname1.c_str(), nullptr, nullptr, &status);
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
                           llvm::errs() << "Found a global var, but can not treat it as an invariant.\n";
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
            const llvm::TargetTransformInfo& TTI = modulePass->getAnalysis<llvm::TargetTransformInfoWrapperPass>().getTTI(F);
            for(llvm::Function::iterator BBIt = F.begin(); BBIt != F.end();)
               llvm::SimplifyInstructionsInBlock(&*BBIt++, &TLI);
            for(llvm::Function::iterator BBIt = F.begin(); BBIt != F.end();)
#if __clang_major__ >= 6
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
   bool DumpGimpleRaw::lowerIntrinsics(llvm::Module& M)
   {
      auto res = false;
      llvm::IntrinsicLowering* IL = new llvm::IntrinsicLowering(*DL);
      auto currFuncIterator = M.getFunctionList().begin();
      while(currFuncIterator != M.getFunctionList().end())
      {
         auto fname = std::string(currFuncIterator->getName());
         for(auto& BB : currFuncIterator->getBasicBlockList())
         {
            auto curInstIterator = BB.getInstList().begin();
            while(curInstIterator != BB.getInstList().end())
            {
               if(isa<llvm::CallInst>(*curInstIterator))
               {
                  auto& ci = cast<llvm::CallInst>(*curInstIterator);
                  llvm::Function* Callee = ci.getCalledFunction();
                  if(Callee && Callee->isIntrinsic() && (!skipIntrinsic(Callee->getIntrinsicID()) || fname == getIntrinsicName(Callee)))
                  {
                     res = true;
                     auto me = curInstIterator;
                     bool atBegin(BB.getInstList().begin() == me);
                     if(!atBegin)
                        --me;
                     if(noLoweringIntrinsic(Callee->getIntrinsicID()))
                     {
                        assert(ci.use_empty() && "Lowering should have eliminated any uses of the intrinsic call!");
                        ci.eraseFromParent();
                     }
#if __clang_major__ >= 8 || defined(_WIN32)
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
                        curInstIterator = BB.getInstList().begin();
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

   void DumpGimpleRaw::compute_eSSA(llvm::Module& M)
   {
      eSSA eSSAHelper;
      for(auto& fun : M.getFunctionList())
      {
         if(!fun.isIntrinsic() && !fun.isDeclaration())
            eSSAHelper.runOnFunction(fun, modulePass);
      }
   }

   void DumpGimpleRaw::computeValueRange(llvm::Module& M)
   {
#if __clang_major__ >= 8
      RA = nullptr;
#else
      RA = new RangeAnalysis::InterProceduralRACropDFSHelper();
      RA->runOnModule(M, modulePass, PtoSets_AA);
#endif
   }

   void DumpGimpleRaw::ValueRangeOptimizer(llvm::Module& M)
   {
      if(RA)
      {
         llvm::TargetLibraryInfo& TLI = modulePass->getAnalysis<llvm::TargetLibraryInfoWrapperPass>().getTLI();

         for(llvm::Function& F : M)
         {
#ifdef DEBUG_RA
            llvm::errs() << "ValueRangeOptimizer: Analysis for function: " << F.getName() << "\n";
#endif
            std::list<llvm::Instruction*> deadList;
            for(auto& BB : F.getBasicBlockList())
            {
               auto curInstIterator = BB.getInstList().begin();
               while(curInstIterator != BB.getInstList().end())
               {
                  llvm::Instruction* I = &*curInstIterator;
                  assert(I->getParent());
                  if(I->getType()->isVoidTy())
                  {
                     ++curInstIterator;
                     continue;
                  }
                  if(I->user_empty() && llvm::isInstructionTriviallyDead(I, &TLI))
                  {
                     llvm::errs() << "this instruction is not used by anyone: ";
                     I->print(llvm::errs());
                     llvm::errs() << "\n";
                     deadList.push_back(I);
                     ++curInstIterator;
                     continue;
                  }
                  RangeAnalysis::Range R = RA->getRange(I);
                  if(R.isEmpty())
                  {
                     llvm::errs() << "this instruction is dead: ";
                     I->print(llvm::errs());
                     llvm::errs() << "\n";
                     assert(!I->getType()->isVoidTy());
                     I->replaceAllUsesWith(llvm::UndefValue::get(I->getType()));
                     if(llvm::isInstructionTriviallyDead(I, &TLI))
                        deadList.push_back(I);
                  }
                  else if(dyn_cast<llvm::PHINode>(I))
                  {
                     ++curInstIterator;
                     continue;
                  }
                  else if(!R.isUnknown())
                  {
                     if(!R.isMaxRange())
                     {
                        if(R.isConstant())
                        {
                           llvm::errs() << "the value associated with this instruction is constant and could be propagated: ";
                           I->print(llvm::errs());
                           llvm::errs() << " -> ";
                           R.print(llvm::errs());
                           llvm::errs() << "\n";
                           auto cInt = R.getLower().sextOrTrunc(I->getType()->getPrimitiveSizeInBits());
                           auto C = llvm::ConstantInt::get(I->getContext(), cInt);
                           I->replaceAllUsesWith(C);
                           if(llvm::isInstructionTriviallyDead(I, &TLI))
                              deadList.push_back(I);
                        }
                        else
                        {
                           auto nbitU = RangeAnalysis::Range_base::neededBits(R.getUnsignedMin(), R.getUnsignedMax(), false);
                           auto nbitS = RangeAnalysis::Range_base::neededBits(R.getSignedMin(), R.getSignedMax(), true);
                           auto bw = R.getBitWidth();
                           if(nbitS < nbitU && nbitS < bw && !isSignedResult(I))
                           {
                              assert(bw > nbitS);
                              llvm::errs() << "the range associated with this unsigned instruction could be reduced with the signed range: ";
                              I->print(llvm::errs());
                              llvm::errs() << " -> " << R.getBitWidth() << " -> " << nbitS << " -> " << nbitU << " -> ";
                              R.print(llvm::errs());
                              llvm::errs() << "\n";
                              llvm::IRBuilder<> B(curInstIterator->getNextNode());

                              auto leftShiftConstant = B.getIntN(bw, bw - nbitS);
                              auto lsh = B.CreateShl(dyn_cast<llvm::Value>(I), leftShiftConstant);
                              auto ashr = B.CreateAShr(lsh, leftShiftConstant);
                              I->replaceAllUsesWith(ashr);
                              cast<llvm::Instruction>(lsh)->setOperand(0, I);
                           }
                        }
                     }
                  }
                  ++curInstIterator;
               }
            }
            for(auto I : deadList)
               if(llvm::isInstructionTriviallyDead(I, &TLI))
                  I->eraseFromParent();
            if(!deadList.empty())
            {
               //               const llvm::TargetTransformInfo& TTI = modulePass->getAnalysis<llvm::TargetTransformInfoWrapperPass>().getTTI(F);
               for(llvm::Function::iterator BBIt = F.begin(); BBIt != F.end();)
                  llvm::SimplifyInstructionsInBlock(&*BBIt++, &TLI);
               //               for(llvm::Function::iterator BBIt = F.begin(); BBIt != F.end();)
               //#if __clang_major__ >= 6
               //                  llvm::simplifyCFG(&*BBIt++, TTI, 1);
               //#else
               //                  llvm::SimplifyCFG(&*BBIt++, TTI, 1);
               //#endif
               llvm::removeUnreachableBlocks(F);
            }
         }
      }
   }

   bool DumpGimpleRaw::LoadStoreOptimizer(llvm::Module& M)
   {
      bool changed = false;
      for(llvm::Function& F : M)
      {
         std::list<llvm::Instruction*> StoreLoadList;
         std::list<llvm::BasicBlock*> BBlist;
         for(auto& BB : F.getBasicBlockList())
            BBlist.push_back(&BB);
         for(auto BB : BBlist)
         {
            auto curInstIterator = BB->getInstList().begin();
            while(curInstIterator != BB->getInstList().end())
            {
#if HAVE_LIBBDD
               llvm::Instruction* I = &*curInstIterator;
               if(dyn_cast<llvm::StoreInst>(I) || dyn_cast<llvm::LoadInst>(I))
               {
                  auto PO = dyn_cast<llvm::StoreInst>(I) ? dyn_cast<llvm::StoreInst>(I)->getPointerOperand() : dyn_cast<llvm::LoadInst>(I)->getPointerOperand();
                  auto PTS = PtoSets_AA->pointsToSet(PO);
                  if(PTS->size() > 1)
                  {
                     bool res = true;
                     for(auto var : *PTS)
                     {
                        auto varValue = PtoSets_AA->getValue(var);
                        if(!varValue)
                        {
                           res = false;
                           break;
                        }
                        auto vid = varValue->getValueID();
                        if(vid != llvm::Value::InstructionVal + llvm::Instruction::Alloca && vid != llvm::Value::GlobalVariableVal)
                        {
                           res = false;
                           break;
                        }
                        auto AI = llvm::dyn_cast<const llvm::AllocaInst>(varValue);
                        auto GV = llvm::dyn_cast<const llvm::GlobalVariable>(varValue);
                        if(AI && AI->getAllocatedType()->isAggregateType() && !AI->getAllocatedType()->isArrayTy())
                        {
                           res = false;
                           break;
                        }
                        else if(GV && GV->getValueType()->isAggregateType() && !GV->getValueType()->isArrayTy())
                        {
                           res = false;
                           break;
                        }
                     }
                     if(res)
                     {
                        StoreLoadList.push_back(I);
                     }
                  }
               }
#endif
               ++curInstIterator;
            }
         }
#if HAVE_LIBBDD
         for(auto& I : StoreLoadList)
         {
            if(llvm::StoreInst* SI = dyn_cast<llvm::StoreInst>(I))
            {
               auto PO = SI->getPointerOperand();
               auto PTS = PtoSets_AA->pointsToSet(PO);
               llvm::IRBuilder<> Builder(SI);
               llvm::errs() << "Store to be optimized: ";
               SI->print(llvm::errs());
               llvm::errs() << "\n";
               llvm::errs() << "PO to be used: ";
               PO->print(llvm::errs());
               llvm::errs() << "\n";
               llvm::errs() << "Var to be written: ";
               SI->getValueOperand()->print(llvm::errs());
               llvm::errs() << "\n";
               llvm::BasicBlock* BB0 = SI->getParent();
               auto LastBB = BB0->splitBasicBlock(SI);
               llvm::BasicBlock* PrevBB = BB0;
               llvm::IRBuilder<> Builder0(BB0->getTerminator());
               auto ptrToInt1 = Builder0.CreatePtrToInt(PO, llvm::Type::getInt64Ty(F.getContext()));

               for(auto var : *PTS)
               {
                  auto varValue = PtoSets_AA->getValue(var);
                  auto AI = llvm::dyn_cast<const llvm::AllocaInst>(varValue);
                  auto GV = llvm::dyn_cast<const llvm::GlobalVariable>(varValue);
                  assert(AI || GV);
                  llvm::Type* objType;
                  if(AI)
                     objType = AI->getAllocatedType();
                  else if(GV)
                     objType = GV->getValueType();
                  else
                     llvm_unreachable("unexpected condition");

                  if(objType->isArrayTy())
                  {
                     auto gep1 = Builder0.CreateBitCast(const_cast<llvm::Value*>(varValue), PO->getType());

                     auto ptrToInt2 = Builder0.CreatePtrToInt(gep1, llvm::Type::getInt64Ty(F.getContext()));
                     auto sub1 = Builder0.CreateSub(ptrToInt1, ptrToInt2);

                     auto sge1 = Builder0.CreateICmpSGE(sub1, Builder0.getInt64(0));
                     auto varValueSize = objType->isSized() ? DL->getTypeAllocSize(objType) : 1ULL;

                     auto ult1 = Builder0.CreateICmpULT(sub1, Builder0.getInt64(varValueSize));
                     auto and1 = Builder0.CreateAnd(sge1, ult1);

                     llvm::BasicBlock* TrueBB = llvm::BasicBlock::Create(F.getContext(), "StoreBB", &F, LastBB);
                     llvm::BasicBlock* FalseBB = llvm::BasicBlock::Create(F.getContext(), "StoreBB", &F, LastBB);
                     PrevBB->getTerminator()->eraseFromParent();
                     Builder.SetInsertPoint(PrevBB);
                     Builder.CreateCondBr(and1, TrueBB, FalseBB);
                     if(PrevBB == BB0)
                        Builder0.SetInsertPoint(BB0->getTerminator());
                     PrevBB = FalseBB;
                     llvm::IRBuilder<> FalseBuilder(FalseBB);
                     FalseBuilder.CreateBr(LastBB);

                     llvm::IRBuilder<> StoreBuilder(TrueBB);
                     auto bitcast1 = StoreBuilder.CreateBitCast(gep1, llvm::Type::getInt8PtrTy(F.getContext()));
                     auto gep3 = StoreBuilder.CreateGEP(bitcast1, sub1);
                     auto bitcast2 = StoreBuilder.CreateBitCast(gep3, PO->getType());
                     StoreBuilder.CreateStore(SI->getValueOperand(), bitcast2);
                     StoreBuilder.CreateBr(LastBB);
                     changed = true;
                  }
                  else
                  {
                     auto gep1 = Builder0.CreateBitCast(const_cast<llvm::Value*>(varValue), PO->getType());
                     auto eq1 = Builder0.CreateICmpEQ(PO, gep1);
                     llvm::BasicBlock* TrueBB = llvm::BasicBlock::Create(F.getContext(), "StoreBB", &F, LastBB);
                     llvm::BasicBlock* FalseBB = llvm::BasicBlock::Create(F.getContext(), "StoreBB", &F, LastBB);
                     PrevBB->getTerminator()->eraseFromParent();
                     Builder.SetInsertPoint(PrevBB);
                     Builder.CreateCondBr(eq1, TrueBB, FalseBB);
                     if(PrevBB == BB0)
                        Builder0.SetInsertPoint(BB0->getTerminator());
                     PrevBB = FalseBB;
                     llvm::IRBuilder<> FalseBuilder(FalseBB);
                     FalseBuilder.CreateBr(LastBB);
                     llvm::IRBuilder<> StoreBuilder(TrueBB);
                     StoreBuilder.CreateStore(SI->getValueOperand(), gep1);
                     StoreBuilder.CreateBr(LastBB);
                     changed = true;
                  }
               }
            }
            else if(llvm::LoadInst* LI = dyn_cast<llvm::LoadInst>(I))
            {
               auto PO = LI->getPointerOperand();
               auto PTS = PtoSets_AA->pointsToSet(PO);

               llvm::BasicBlock* BB0 = LI->getParent();
               auto LastBB = BB0->splitBasicBlock(LI);
               llvm::BasicBlock* PrevBB = BB0;
               llvm::IRBuilder<> Builder0(BB0->getTerminator());
               auto ptrToInt1 = Builder0.CreatePtrToInt(PO, llvm::Type::getInt64Ty(F.getContext()));
               llvm::IRBuilder<> Builder(LI);
               auto phi1 = Builder.CreatePHI(LI->getType(), 1 + PTS->size());
               LI->replaceAllUsesWith(phi1);
               unsigned index = 0;

               for(auto var : *PTS)
               {
                  ++index;
                  auto varValue = PtoSets_AA->getValue(var);
                  auto AI = llvm::dyn_cast<const llvm::AllocaInst>(varValue);
                  auto GV = llvm::dyn_cast<const llvm::GlobalVariable>(varValue);
                  assert(AI || GV);
                  llvm::Type* objType;
                  if(AI)
                     objType = AI->getAllocatedType();
                  else if(GV)
                     objType = GV->getValueType();
                  else
                     llvm_unreachable("unexpected condition");

                  if(objType->isArrayTy())
                  {
                     auto gep1 = Builder0.CreateBitCast(const_cast<llvm::Value*>(varValue), PO->getType());

                     auto ptrToInt2 = Builder0.CreatePtrToInt(gep1, llvm::Type::getInt64Ty(F.getContext()));
                     auto sub1 = Builder0.CreateSub(ptrToInt1, ptrToInt2);

                     auto sge1 = Builder0.CreateICmpSGE(sub1, Builder0.getInt64(0));
                     auto varValueSize = objType->isSized() ? DL->getTypeAllocSize(objType) : 1ULL;

                     auto ult1 = Builder0.CreateICmpULT(sub1, Builder0.getInt64(varValueSize));
                     auto and1 = Builder0.CreateAnd(sge1, ult1);

                     llvm::BasicBlock* TrueBB = llvm::BasicBlock::Create(F.getContext(), "StoreBB", &F, LastBB);
                     llvm::BasicBlock* FalseBB = llvm::BasicBlock::Create(F.getContext(), "StoreBB", &F, LastBB);
                     PrevBB->getTerminator()->eraseFromParent();
                     Builder.SetInsertPoint(PrevBB);
                     Builder.CreateCondBr(and1, TrueBB, FalseBB);
                     if(PrevBB == BB0)
                        Builder0.SetInsertPoint(BB0->getTerminator());
                     PrevBB = FalseBB;
                     llvm::IRBuilder<> FalseBuilder(FalseBB);
                     FalseBuilder.CreateBr(LastBB);

                     llvm::IRBuilder<> LoadBuilder(TrueBB);
                     auto bitcast1 = LoadBuilder.CreateBitCast(gep1, llvm::Type::getInt8PtrTy(F.getContext()));
                     auto gep3 = LoadBuilder.CreateGEP(bitcast1, sub1);
                     auto bitcast2 = LoadBuilder.CreateBitCast(gep3, PO->getType());
                     auto load1 = LoadBuilder.CreateLoad(bitcast2);
                     phi1->addIncoming(load1, TrueBB);
                     LoadBuilder.CreateBr(LastBB);
                     if(index == PTS->size())
                        phi1->addIncoming(llvm::UndefValue::get(I->getType()), FalseBB);
                     changed = true;
                  }
                  else
                  {
                     auto gep1 = Builder0.CreateBitCast(const_cast<llvm::Value*>(varValue), PO->getType());
                     auto eq1 = Builder0.CreateICmpEQ(PO, gep1);
                     llvm::BasicBlock* TrueBB = llvm::BasicBlock::Create(F.getContext(), "LoadBB", &F, LastBB);
                     llvm::BasicBlock* FalseBB = llvm::BasicBlock::Create(F.getContext(), "LoadBB", &F, LastBB);
                     PrevBB->getTerminator()->eraseFromParent();
                     Builder.SetInsertPoint(PrevBB);
                     Builder.CreateCondBr(eq1, TrueBB, FalseBB);
                     if(PrevBB == BB0)
                        Builder0.SetInsertPoint(BB0->getTerminator());
                     PrevBB = FalseBB;
                     llvm::IRBuilder<> FalseBuilder(FalseBB);
                     FalseBuilder.CreateBr(LastBB);

                     llvm::IRBuilder<> LoadBuilder(TrueBB);
                     auto load1 = LoadBuilder.CreateLoad(gep1);
                     phi1->addIncoming(load1, TrueBB);
                     LoadBuilder.CreateBr(LastBB);
                     if(index == PTS->size())
                        phi1->addIncoming(llvm::UndefValue::get(I->getType()), FalseBB);
                     changed = true;
                  }
               }
            }
            else
               llvm_unreachable("unexpected condition");
         }
#endif
         for(auto& I : StoreLoadList)
            I->eraseFromParent();
      }
      // M.print(llvm::errs(),nullptr);
      return changed;
   }

   void DumpGimpleRaw::computeMAEntryDefs(const llvm::Function* F, std::map<const llvm::Function*, std::map<const void*, std::set<const llvm::Instruction*>>>& CurrentListofMAEntryDef, llvm::ModulePass* modulePass)
   {
      auto& MSSA = modulePass->getAnalysis<llvm::MemorySSAWrapperPass>(*const_cast<llvm::Function*>(F)).getMSSA();
      for(const auto& BB : F->getBasicBlockList())
      {
         for(const auto& inst : BB)
         {
            const llvm::MemoryUseOrDef* ma = MSSA.getMemoryAccess(&inst);
            if(ma && ma->getValueID() == llvm::Value::MemoryDefVal)
            {
               auto defMA = MSSA.getWalker()->getClobberingMemoryAccess(&inst);
               if(MSSA.isLiveOnEntryDef(defMA))
                  CurrentListofMAEntryDef[F][nullptr].insert(&inst);
               else if(defMA->getValueID() == llvm::Value::MemoryPhiVal)
                  CurrentListofMAEntryDef[F][gimple_phi_virtual_result(getVirtualGimplePhi(dyn_cast<llvm::MemoryPhi>(defMA), MSSA))].insert(&inst);
               else
               {
                  assert(defMA->getValueID() == llvm::Value::MemoryDefVal);
                  CurrentListofMAEntryDef[F][dyn_cast<llvm::MemoryUseOrDef>(defMA)->getMemoryInst()].insert(&inst);
               }
            }
         }
      }
   }
   bool DumpGimpleRaw::runOnModule(llvm::Module& M, llvm::ModulePass* _modulePass, const std::string& _TopFunctionName)
   {
      DL = &M.getDataLayout();
      modulePass = _modulePass;
      moduleContext = &M.getContext();
      TopFunctionName = _TopFunctionName;
      if(!earlyAnalysis)
         buildMetaDataMap(M);
      auto res = !earlyAnalysis && lowerMemIntrinsics(M);

      res = res | (!earlyAnalysis && RebuildConstants(M));
      res = res | (!earlyAnalysis && lowerIntrinsics(M));
      compute_eSSA(M);
#if HAVE_LIBBDD
      if(!earlyAnalysis && !onlyGlobals)
      {
         std::string starting_function = TopFunctionName;
         if(starting_function == "")
         {
            const llvm::Function* main = M.getFunction("main");
            if(main)
               starting_function = "main";
         }
         if(starting_function != "")
         {
#define ANDERSEN 1
#if ANDERSEN
            PtoSets_AA = new Andersen_AA(starting_function);
#else
            PtoSets_AA = new Staged_Flow_Sensitive_AA(starting_function);
#endif
            PtoSets_AA->computePointToSet(M);
            //            auto changed = LoadStoreOptimizer(M);
            //            res = res | changed;
         }
      }
#endif
      computeValueRange(M);
#ifdef DEBUG_RA
      assert(!llvm::verifyModule(M, &llvm::errs()));
#endif
      ValueRangeOptimizer(M);
#ifdef DEBUG_RA
      assert(!llvm::verifyModule(M, &llvm::errs()));
#endif

      if(!earlyAnalysis && !onlyGlobals)
      {
         for(const auto& fun : M.getFunctionList())
         {
            if(!fun.isDeclaration() && !fun.isIntrinsic())
            {
               computeMAEntryDefs(&fun, CurrentListofMAEntryDef, modulePass);
            }
         }
      }

      if(!earlyAnalysis)
      {
         for(const auto& globalVar : M.getGlobalList())
         {
#if PRINT_DBG_MSG
            llvm::errs() << "Found global name: " << globalVar.getName() << "|" << ValueTyNames[globalVar.getValueID()] << "\n";
#endif
            SerializeGimpleGlobalTreeNode(assignCodeAuto(&globalVar));
         }
      }
      if(!earlyAnalysis && !onlyGlobals)
      {
         for(const auto& fun : M.getFunctionList())
         {
            if(fun.isIntrinsic())
            {
#if PRINT_DBG_MSG
               llvm::errs() << "Function intrinsic skipped: " << fun.getName() << "|" << ValueTyNames[fun.getValueID()] << "\n";
#endif
            }
            else
            {
#if PRINT_DBG_MSG
               llvm::errs() << "Found function: " << fun.getName() << "|" << ValueTyNames[fun.getValueID()] << "\n";
#endif
               SerializeGimpleGlobalTreeNode(assignCodeAuto(&fun));
            }
         }
         CurrentListofMAEntryDef.clear();
      }
#if HAVE_LIBBDD
      if(PtoSets_AA)
      {
         delete PtoSets_AA;
         PtoSets_AA = nullptr;
      }
#endif
      if(RA)
      {
         RA->printRanges(M, llvm::errs());
         delete RA;
      }
      // M.print(llvm::errs(), nullptr);
      return res;
   }
} // namespace llvm
