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
 * @file plugin_includes.hpp
 * @brief Class used to dump in a gimple IR format the LLVM IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef PLUGIN_INCLUDES_HPP
#define PLUGIN_INCLUDES_HPP

#ifndef __clang_major__
#define __clang_major__ 7
#endif
/// Autoheader include
#include "config_HAVE_LIBBDD.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#if __clang_major__ != 4
#include "llvm/Transforms/Utils/PredicateInfo.h"
#endif
#include <deque>
#include <list>
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

#define GT(code) tree_codes::code
#define LOCAL_BUFFER_LEN 512

#if __clang_major__ == 11
#define CLANG_VERSION_SYMBOL(SYMBOL) clang11##SYMBOL
#define CLANG_VERSION_STRING(SYMBOL) "clang11" #SYMBOL
#elif __clang_major__ == 10
#define CLANG_VERSION_SYMBOL(SYMBOL) clang10##SYMBOL
#define CLANG_VERSION_STRING(SYMBOL) "clang10" #SYMBOL
#elif __clang_major__ == 9
#define CLANG_VERSION_SYMBOL(SYMBOL) clang9##SYMBOL
#define CLANG_VERSION_STRING(SYMBOL) "clang9" #SYMBOL
#elif __clang_major__ == 8
#define CLANG_VERSION_SYMBOL(SYMBOL) clang8##SYMBOL
#define CLANG_VERSION_STRING(SYMBOL) "clang8" #SYMBOL
#elif __clang_major__ == 7
#define CLANG_VERSION_SYMBOL(SYMBOL) clang7##SYMBOL
#define CLANG_VERSION_STRING(SYMBOL) "clang7" #SYMBOL
#elif __clang_major__ == 6
#define CLANG_VERSION_SYMBOL(SYMBOL) clang6##SYMBOL
#define CLANG_VERSION_STRING(SYMBOL) "clang6" #SYMBOL
#elif __clang_major__ == 5
#define CLANG_VERSION_SYMBOL(SYMBOL) clang5##SYMBOL
#define CLANG_VERSION_STRING(SYMBOL) "clang5" #SYMBOL
#elif __clang_major__ == 4
#define CLANG_VERSION_SYMBOL(SYMBOL) clang4##SYMBOL
#define CLANG_VERSION_STRING(SYMBOL) "clang4" #SYMBOL
#else
#error
#endif

namespace llvm
{
   class Module;
   class Type;
   class DataLayout;
   class Constant;
   class ModulePass;
   class GEPOperator;
   class User;
   class Value;
   class BasicBlock;
   class MemoryUseOrDef;
   class MemoryPhi;
   class MemorySSA;
   class MemoryAccess;
   class MemoryLocation;
   class AllocaInst;
   class TargetLibraryInfo;
#if __clang_major__ != 4
   class PredicateInfo;
#endif
   class Metadata;
   class Argument;
} // namespace llvm

namespace RangeAnalysis
{
   class InterProceduralRACropDFSHelper;
}
class Andersen_AA;

namespace llvm
{
   class DumpGimpleRaw
   {
      bool changed;
      bool earlyAnalysis;
      /* Serialize column control */
      const int SOL_COLUMN = 25;       /* Start of line column.  */
      const int EOL_COLUMN = 55;       /* End of line column.  */
      const int COLUMN_ALIGNMENT = 15; /* Alignment.  */

      enum tree_codes_class
      {
         tcc_exceptional = 0, /* An exceptional code (fits no category).  */
         tcc_constant,        /* A constant.  */
         /* Order of tcc_type and tcc_declaration is important.  */
         tcc_type,        /* A type object code.  */
         tcc_declaration, /* A declaration (also serving as variable refs).  */
         tcc_reference,   /* A reference to storage.  */
         tcc_comparison,  /* A comparison expression.  */
         tcc_unary,       /* A unary arithmetic expression.  */
         tcc_binary,      /* A binary arithmetic expression.  */
         tcc_statement,   /* A statement expression, which have side effects
                     but usually no interesting value.  */
         tcc_vl_exp,      /* A function call or other expression with a
                     variable-length operand vector.  */
         tcc_expression   /* Any other expression.  */
      };
      enum gimple_rhs_class
      {
         GIMPLE_INVALID_RHS, /* The expression cannot be used on the RHS.  */
         GIMPLE_TERNARY_RHS, /* The expression is a ternary operation.  */
         GIMPLE_BINARY_RHS,  /* The expression is a binary operation.  */
         GIMPLE_UNARY_RHS,   /* The expression is a unary operation.  */
         GIMPLE_SINGLE_RHS   /* The expression is a single object (an SSA
                       name, a _DECL, a _REF, etc.  */
      };
      enum class tree_codes;
      static const char* tree_codesNames[];
      static const tree_codes_class tree_codes2tree_codes_class[];
      static const unsigned int tree_codes2nargs[];
      static const gimple_rhs_class gimple_rhs_class_table[];
      static const char* ValueTyNames[];
      static const std::set<std::string> builtinsNames;

      struct tree_list
      {
         const void* purp;
         const void* valu;
         unsigned int chan;
         tree_list() : purp(nullptr), valu(nullptr), chan(0)
         {
         }
      };
      std::map<unsigned int, tree_list> index2tree_list;
      /// memoization map used to avoid the re-computation of tree associated with a given node
      std::map<const void*, const void*> memoization_tree_list;

      struct tree_vec
      {
         std::vector<const void*> data;
      };
      std::map<unsigned int, tree_vec> index2tree_vec;
      /// memoization map used to avoid the re-computation of tree associated with a given node
      std::map<const void*, const void*> memoization_tree_vec;

      struct field_decl
      {
         const void* name;
         const void* type;
         const void* scpe;
         const void* size;
         unsigned int algn;
         const void* bpos;
         field_decl() : name(nullptr), type(nullptr), scpe(nullptr), size(nullptr), algn(0), bpos(nullptr)
         {
         }
      };
      std::map<std::pair<const void*, unsigned int>, field_decl> index2field_decl;

      struct label_decl
      {
         const void* type;
         const void* scpe;
         label_decl() : type(nullptr), scpe(nullptr)
         {
         }
      };
      std::map<const llvm::BasicBlock*, label_decl> index2label_decl;

      std::error_code EC;
      const std::string outdir_name;
      const std::string InFile;
      std::string filename;
      /// stream associated with the gimple raw file
      llvm::raw_fd_ostream stream;
      /// when true only the global variables are serialized
      bool onlyGlobals;
      /// map function with name of its parameters
      const std::map<std::string, std::vector<std::string>>* fun2params;
      std::map<const llvm::Argument*, std::string> argNameTable;
      const llvm::DataLayout* DL;
      /// current module pass
      llvm::ModulePass* modulePass;
      llvm::LLVMContext* moduleContext;
      std::string TopFunctionName;

      /// relation between LLVM object and serialization index
      std::map<const void*, unsigned int> llvm2index;
      /// relation between LLVM object and TREE_CODE
      std::map<const void*, tree_codes> llvm2tree_code;
      unsigned int last_used_index;
      std::deque<const void*> Queue;
      std::set<const void*> setOfStatementsList;
      std::set<const void*> setOfGimples;

      /// serialization data
      int column;

      /// internal identifier table
      std::set<std::string> identifierTable;
      /// unsigned integer constant table
      std::map<uint64_t, const void*> uicTable;
      /// type_integer with specific max value
      std::map<const void*, unsigned long long int> maxValueITtable;
      std::map<const void*, llvm::LLVMContext*> ArraysContexts;

      std::string getTypeName(const void* ty) const;

      static char buffer[LOCAL_BUFFER_LEN];

      /// relation helpers
      const void* assignCode(const void* o, tree_codes c)
      {
         if(HAS_CODE(o) && (TREE_CODE(o) != c))
            llvm::errs() << GET_TREE_CODE_NAME(c) << " vs " << GET_TREE_CODE_NAME(TREE_CODE(o)) << "\n";
         assert(!HAS_CODE(o) || (TREE_CODE(o) == c));
         llvm2tree_code[o] = c;
         return o;
      }
      const void* assignCodeAuto(const void* o);
      const void* assignCodeType(const llvm::Type* ty);

      bool CheckSignedTag(const llvm::Type* t) const
      {
         return reinterpret_cast<size_t>(t) & 1;
      }
      bool CheckSignedTag(const void* t) const
      {
         return reinterpret_cast<size_t>(t) & 1;
      }
      const llvm::Type* NormalizeSignedTag(const llvm::Type* t) const
      {
         return reinterpret_cast<const llvm::Type*>(reinterpret_cast<size_t>(t) & (~1ULL));
      }
      const void* NormalizeSignedTag(const void* t) const
      {
         return reinterpret_cast<const void*>(reinterpret_cast<size_t>(t) & (~1ULL));
      }
      const llvm::Type* AddSignedTag(const llvm::Type* t) const
      {
         return reinterpret_cast<const llvm::Type*>(reinterpret_cast<size_t>(t) | 1);
      }
      const void* AddSignedTag(const void* t) const
      {
         return AddSignedTag(reinterpret_cast<const llvm::Type*>(t));
      }

      struct expanded_location
      {
         std::string filename;
         const char* file;
         unsigned int line;
         unsigned int column;
         expanded_location() : file(nullptr), line(0), column(0)
         {
         }
         expanded_location(const expanded_location& el)
         {
            filename = el.filename;
            file = filename.c_str();
            line = el.line;
            column = el.column;
         }
         expanded_location& operator=(const expanded_location& el)
         {
            if(this != &el)
            {
               filename = el.filename;
               file = filename.c_str();
               line = el.line;
               column = el.column;
            }
            return *this;
         }
      };

      expanded_location expand_location(const void* i) const;
      bool gimple_has_location(const void* g) const;
      const void* gimple_location(const void* t) const;
      tree_codes gimple_code(const void* g) const
      {
         assert(HAS_CODE(g));
         return TREE_CODE(g);
      }
      struct pt_solution
      {
         bool anything;
         bool nonlocal;
         bool escaped;
         bool ipa_escaped;
         bool null;
         std::set<const void*> vars;
         pt_solution() : anything(false), nonlocal(false), escaped(false), ipa_escaped(false), null(false)
         {
         }
      };

      struct pt_info
      {
         bool valid;
         pt_solution pt;
         pt_info() : valid(false)
         {
         }
      };
      const pt_info* SSA_NAME_PTR_INFO(const void* t) const;
      Andersen_AA* PtoSets_AA;
      /// integer type used to convert a pointer in a signed integer type
      unsigned int SignedPointerTypeReference;

      struct ssa_name
      {
         int vers;
         pt_info ptr_info;
         const void* type;
         const void* var;
         const void* def_stmts;
         bool isVirtual;
         bool isDefault;
         ssa_name() : vers(-1), type(nullptr), var(nullptr), def_stmts(nullptr), isVirtual(false), isDefault(false)
         {
         }
      };
      void dump_pt_solution(const pt_solution* pt, const char* first_tag, const char* second_tag);

      std::map<std::pair<const void*, bool>, ssa_name> index2ssa_name;
      int last_memory_ssa_vers;
      std::map<const void*, int> memoryaccess2ssaindex;

      int last_BB_index;
      std::map<const llvm::BasicBlock*, int> BB_index_map;

      int getBB_index(const llvm::BasicBlock* BB);

      gimple_rhs_class get_gimple_rhs_class(tree_codes code)
      {
         return gimple_rhs_class_table[static_cast<unsigned int>(code)];
      }
      template <class InstructionOrConstantExpr>
      tree_codes tree_expr_code(InstructionOrConstantExpr* inst);
      tree_codes gimple_expr_code(const void* stmt);
      tree_codes gimple_assign_rhs_code(const void* stmt)
      {
         return gimple_expr_code(stmt);
      }
      const void* getGimpleNop(const llvm::Value* operand, const void* scpe);
      template <class InstructionOrConstantExpr>
      bool isSignedInstruction(const InstructionOrConstantExpr* inst) const;
      template <class InstructionOrConstantExpr>
      bool isSignedResult(const InstructionOrConstantExpr* inst) const;
      const llvm::Type* getCondSignedResult(const llvm::Value* operand, const llvm::Type* type) const;
      template <class InstructionOrConstantExpr>
      bool isSignedOperand(const InstructionOrConstantExpr* inst, unsigned index) const;
      template <class InstructionOrConstantExpr>
      bool isUnsignedOperand(const InstructionOrConstantExpr* inst, unsigned index) const;
      const void* getSSA(const llvm::Value* operand, const void* def_stmt, const llvm::Function* currentFunction, bool isDefault);
      bool is_PTS(unsigned int varId, const llvm::TargetLibraryInfo& TLI, bool with_all = false);
      bool is_virtual_ssa(const void* t) const;
      bool SSA_NAME_IS_DEFAULT_DEF(const void* t) const;
      const void* LowerGetElementPtrOffset(const llvm::GEPOperator* gep, const llvm::Function* currentFunction, const void*& base_node, bool& isZero);
      const void* LowerGetElementPtr(const void* type, const llvm::User* gep, const llvm::Function* currentFunction);
      const void* gimple_assign_rhs_getelementptr(const void* g);
      bool temporary_addr_check(const llvm::User* inst, std::set<const llvm::User*>& visited, const llvm::TargetLibraryInfo& TLI);
      const void* getOperand(const llvm::Value* operand, const llvm::Function* currentFunction);
      const void* gimple_assign_lhs(const void* g);
      const void* gimple_assign_rhs_alloca(const void* g);
      void add_alloca_pt_solution(const void* lhs, const void* rhs);
      struct alloca_var
      {
         const llvm::AllocaInst* alloc_inst;
         bool addr;
      };
      std::map<const void*, alloca_var> index2alloca_var;
      struct orig_var
      {
         const void* orig;
      };
      std::map<const void*, orig_var> index2orig_var;
      const void* DECL_ABSTRACT_ORIGIN(const void* t);
      struct integer_cst_signed
      {
         const void* type;
         const void* ic;
      };
      std::map<const void*, integer_cst_signed> index2integer_cst_signed;
      template <class InstructionOrConstantExpr>
      const void* getSignedOperand(const InstructionOrConstantExpr* inst, const void* op, unsigned index);
      template <class InstructionOrConstantExpr>
      const void* getSignedOperandIndex(const InstructionOrConstantExpr* inst, unsigned index, const llvm::Function* currentFunction);
      const void* gimple_assign_rhsIndex(const void* g, unsigned index);
      const void* gimple_assign_rhs1(const void* g)
      {
         return gimple_assign_rhsIndex(g, 0);
      }
      const void* gimple_assign_rhs2(const void* g)
      {
         return gimple_assign_rhsIndex(g, 1);
      }
      const void* gimple_assign_rhs3(const void* g)
      {
         return gimple_assign_rhsIndex(g, 2);
      }
      tree_codes gimple_cond_code(const void* g)
      {
         return gimple_expr_code(g);
      }
      const void* boolean_type_node(const void* g);
      const char* gimple_asm_string(const void* g);
      const void* gimple_cond_op(const void* g)
      {
         return gimple_assign_rhsIndex(g, 0);
      }
      const void* gimple_label_label(const void* g);
      const void* gimple_phi_result(const void* g)
      {
         return gimple_assign_lhs(g);
      }
      const void* gimple_phi_virtual_result(const void* g) const;
      unsigned int gimple_phi_num_args(const void* g) const;
      unsigned int gimple_phi_virtual_num_args(const void* g) const;
      const void* gimple_phi_arg_def(const void* g, unsigned int index);
      const void* gimple_phi_virtual_arg_def(const void* g, unsigned int index);
      int gimple_phi_arg_edgeBBindex(const void* g, unsigned int index);
      int gimple_phi_virtual_arg_edgeBBindex(const void* g, unsigned int index);
      const void* gimple_call_fn(const void* g);
      unsigned int gimple_call_num_args(const void* g);
      const void* gimple_call_arg(const void* g, unsigned int arg_index);
      const void* gimple_return_retval(const void* g);
      const void* gimple_switch_index(const void* g);
      const void* gimple_switch_vec(const void* g);
      const void* build_custom_function_call_expr(const void* g);
      const void* call_expr_fn(const void* t);
      unsigned int call_expr_num_args(const void* t);
      const void* call_expr_arg(const void* t, unsigned int arg_index);

      struct call_expr
      {
         const void* type;
         const void* fn;
         std::vector<const void*> args;
         call_expr() : type(nullptr), fn(nullptr)
         {
         }
      };
      std::map<const void*, call_expr> index2call_expr;

      struct tree_expr
      {
         tree_codes tc;
         const void* type;
         const void* op1;
         const void* op2;
         const void* op3;
         tree_expr() : tc(), type(nullptr), op1(nullptr), op2(nullptr), op3(nullptr)
         {
         }
      };
      std::list<tree_expr> index2tree_expr;

      struct gimple_nop
      {
         const void* scpe;
         gimple_nop() : scpe(nullptr)
         {
         }
      };
      std::map<const void*, gimple_nop> index2gimple_nop;

      struct gimple_phi_virtual
      {
         const void* scpe;
         int bb_index;
         const void* res;
         std::vector<std::pair<const void*, int>> def_edfe_pairs;

         gimple_phi_virtual() : scpe(nullptr), bb_index(-1), res(nullptr)
         {
         }
      };
      std::map<const llvm::BasicBlock*, gimple_phi_virtual> index2gimple_phi_virtual;

      struct gimple_label
      {
         const void* scpe;
         int bb_index;
         const void* op;

         gimple_label() : scpe(nullptr), bb_index(-1), op(nullptr)
         {
         }
      };
      std::map<const llvm::BasicBlock*, gimple_label> index2gimple_label;

      const void* createGimpleLabelStmt(const llvm::BasicBlock* BB);

      const void* getVirtualDefStatement(llvm::MemoryAccess* defAccess, bool& isDefault, const llvm::MemorySSA& MSSA, const llvm::Function* currentFunction);
      const void* getVirtualGimplePhi(llvm::MemoryPhi* mp, const llvm::MemorySSA& MSSA);

      const void* build3(tree_codes tc, const void* type, const void* op1, const void* op2, const void* op3);
      const void* build2(tree_codes tc, const void* type, const void* op1, const void* op2)
      {
         return build3(tc, type, op1, op2, nullptr);
      }
      const void* build1(tree_codes tc, const void* type, const void* op1)
      {
         return build3(tc, type, op1, nullptr, nullptr);
      }

      /// currently expressions do not have source file associated
      bool EXPR_HAS_LOCATION(const void*) const
      {
         return false;
      }
      char* EXPR_FILENAME(const void*) const
      {
         return nullptr;
      }
      unsigned int EXPR_LINENO(const void*) const
      {
         return 0;
      }
      unsigned int EXPR_COLUMNNO(const void*) const
      {
         return 0;
      }

      bool IS_EXPR_CODE_CLASS(tree_codes_class CLASS) const
      {
         return ((CLASS) >= tcc_reference && (CLASS) <= tcc_expression);
      }
      bool HAS_CODE(const void* NODE) const
      {
         return llvm2tree_code.find(NODE) != llvm2tree_code.end();
      }
      tree_codes TREE_CODE(const void* NODE) const
      {
         assert(HAS_CODE(NODE));
         return llvm2tree_code.find(NODE)->second;
      }
      tree_codes_class TREE_CODE_CLASS(tree_codes CODE) const
      {
         return tree_codes2tree_codes_class[static_cast<unsigned int>(CODE)];
      }
      const char* GET_TREE_CODE_NAME(tree_codes CODE) const
      {
         return tree_codesNames[static_cast<unsigned int>(CODE)];
      }
      bool DECL_P(const void* NODE) const
      {
         return (TREE_CODE_CLASS(TREE_CODE(NODE)) == tcc_declaration);
      }
      bool DECL_ASSEMBLER_NAME_SET_P(const void* t) const;
      const void* DECL_ASSEMBLER_NAME(const void* t);
      const void* DECL_NAME(const void* t);
      const char* IDENTIFIER_POINTER(const void* t) const;
      int IDENTIFIER_LENGTH(const void* t) const;
      const void* TREE_PURPOSE(const void* t) const;
      const void* TREE_VALUE(const void* t) const;
      const void* TREE_CHAIN(const void* t) const;
      int TREE_VEC_LENGTH(const void* t) const;
      const void* TREE_VEC_ELT(const void* t, int i) const;
      const void* DECL_SOURCE_LOCATION(const void* t) const;
      const void* DECL_CONTEXT(const void* t);
      int DECL_UID(const void* t) const;
      bool DECL_C_BIT_FIELD(const void* t) const;
      bool DECL_EXTERNAL(const void* t) const;
      bool TREE_PUBLIC(const void* t) const;
      bool TREE_STATIC(const void* t) const;
      bool is_builtin_fn(const void* t) const;
      const void* DECL_ARG_TYPE(const void* t);
      const void* DECL_INITIAL(const void* t);
      const void* DECL_SIZE(const void* t);
      int DECL_ALIGN(const void* t);
      bool DECL_PACKED(const void* t) const;
      bool DECL_FIELD_OFFSET(const void* t) const;
      const void* bit_position(const void* t);
      int TREE_USED(const void* t) const;
      bool DECL_REGISTER(const void* t) const;
      bool TREE_READONLY(const void* t) const;
      bool TREE_ADDRESSABLE(const void* t) const;
      const void* TREE_OPERAND(const void* t, unsigned index);
      int64_t TREE_INT_CST_LOW(const void* t);
      const void* TREE_TYPE(const void* t);
      bool POINTER_TYPE_P(const void* t) const;
      bool TYPE_UNSIGNED(const void* t) const;
      int TYPE_PRECISION(const void* t) const;
      bool COMPLEX_FLOAT_TYPE_P(const void* t) const;
      bool TYPE_SATURATING(const void* t) const;
      const void* TYPE_MIN_VALUE(const void* t);
      const void* TYPE_MAX_VALUE(const void* t);
      const void* TYPE_VALUES(const void* t);
      const void* TYPE_NAME(const void* t);
      const void* TYPE_SIZE(const void* t);
      const void* TYPE_CONTEXT(const void* t);
      int TYPE_ALIGN(const void* t) const;
      bool TYPE_PACKED(const void* t) const;
      const void* TYPE_ARG_TYPES(const void* t);
      const void* TYPE_DOMAIN(const void* t);
      bool stdarg_p(const void* t) const;
      llvm::ArrayRef<llvm::Type*> TYPE_FIELDS(const void* t);
      const void* GET_FIELD_DECL(const void* t, unsigned int pos, const void* scpe);
      const void* GET_METHOD_TYPE(const llvm::Type* t, unsigned int pos, const void* scpe);
      const void* TYPE_METHOD_BASETYPE(const void* t);

      const std::list<const void*> DECL_ARGUMENTS(const void* t);
      const void* getStatement_list(const void* t);
      const void* getGimpleScpe(const void* g);
      int getGimple_bb_index(const void* g);
      bool gimple_has_mem_ops(const void* g);
      std::map<const llvm::Function*, std::map<const void*, std::set<const llvm::Instruction*>>> CurrentListofMAEntryDef;
      void serialize_vops(const void* g);
      void serialize_gimple_aliased_reaching_defs(llvm::MemoryAccess* MA, llvm::MemorySSA& MSSA, std::set<llvm::MemoryAccess*>& visited, const llvm::Function* currentFunction, const llvm::MemoryLocation* OrigLoc, const char* tag);

      const void* SSA_NAME_VAR(const void* t) const;
      int SSA_NAME_VERSION(const void* t) const;
      const void* SSA_NAME_DEF_STMT(const void* t) const;
      const void* getMinValue(const void* t);
      const void* getMaxValue(const void* t);
      RangeAnalysis::InterProceduralRACropDFSHelper* RA;
      const std::list<std::pair<const void*, const void*>> CONSTRUCTOR_ELTS(const void* t);

      const void* CASE_LOW(const void* t);
      const void* CASE_HIGH(const void* t);
      const void* CASE_LABEL(const void* t);

      void DumpVersion(llvm::raw_fd_ostream& stream);

      void serialize_new_line();

      void serialize_maybe_newline();

      void serialize_pointer(const char* field, const void* ptr);

      void serialize_int(const char* field, int i);

      void serialize_wide_int(const char* field, int64_t i);

      void serialize_real(const void* t);

      int serialize_with_double_quote(const char* input, int length);

      int serialize_with_escape(const char* input, int length);

      void serialize_string(const char* string);

      void serialize_string_field(const char* field, const char* str);

      void serialize_string_cst(const char* field, const char* str, int length, unsigned int precision);

      void serialize_index(unsigned int index);

      void queue_and_serialize_type(const void* t);
      void queue_and_serialize_index(const char* field, const void* t);

      void serialize_child(const char* field, const void* child)
      {
         queue_and_serialize_index(field, child);
      }
      void serialize_statement_child(const char* field, const void* child)
      {
         setOfStatementsList.insert(child);
         queue_and_serialize_index(field, child);
      }
      void serialize_gimple_child(const char* field, const void* child)
      {
         setOfGimples.insert(child);
         queue_and_serialize_index(field, child);
      }

      unsigned int queue(const void* obj);

      void SerializeGimpleFunctionHeader(const void* obj);

      void SerializeGimpleGlobalTreeNode(const void* obj);

      void dequeue_and_serialize_gimple(const void* t);
      void dequeue_and_serialize_statement(const void* t);
      std::string getHeaderForBuiltin(const void* t);
      void dequeue_and_serialize();

      std::map<const llvm::Value*, llvm::Metadata*> MetaDataMap;
      void buildMetaDataMap(const llvm::Module& M);

      bool lowerMemIntrinsics(llvm::Module& M);
      bool RebuildConstants(llvm::Module& M);
      bool lowerIntrinsics(llvm::Module& M);

      void compute_eSSA(llvm::Module& M, bool* changed);

      void computeValueRange(const llvm::Module& M);
      void ValueRangeOptimizer(llvm::Module& M);
      bool LoadStoreOptimizer(llvm::Module& M);
      void computeMAEntryDefs(const llvm::Function* F, std::map<const llvm::Function*, std::map<const void*, std::set<const llvm::Instruction*>>>& CurrentListofMAEntryDef, llvm::ModulePass* modulePass);

    public:
      DumpGimpleRaw(const std::string& _outdir_name, const std::string& _InFile, bool onlyGlobals, std::map<std::string, std::vector<std::string>>* fun2params, bool early);

      bool runOnModule(llvm::Module& M, llvm::ModulePass* modulePass, const std::string& TopFunctionName);
   };
} // namespace llvm

#endif
