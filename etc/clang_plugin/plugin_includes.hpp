/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2018-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file plugin_includes.hpp
 * @brief Class used to dump in the IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef PLUGIN_INCLUDES_HPP
#define PLUGIN_INCLUDES_HPP

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "panda_clang_compat.hpp"

#if PANDA_LLVM_CLANG_MAJOR > 4
#include "llvm/Analysis/MemorySSA.h"
#else
#include "llvm/Transforms/Utils/MemorySSA.h"
#endif
#if PANDA_LLVM_CLANG_MAJOR > 5
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#endif

#include "debug_print.hpp"

#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#define IRC(code) ir_codes::code
#define LOCAL_BUFFER_LEN 512

namespace llvm
{
   class AllocaInst;
   class Argument;
   class AssumptionCache;
   class BasicBlock;
   class Constant;
   class DataLayout;
   class DominatorTree;
   class GEPOperator;
   class LazyValueInfo;
   class LoopInfo;
   class MemoryAccess;
   class MemoryLocation;
   class MemoryPhi;
   class MemorySSA;
   class MemoryUseOrDef;
   class Metadata;
   class Module;
   class ModulePass;
   class TargetLibraryInfo;
   class TargetTransformInfo;
   class Type;
   class User;
   class Value;
} // namespace llvm

namespace RangeAnalysis
{
   class InterProceduralRACropDFSHelper;
}
class Andersen_AA;

using MemorySSAAnalysisResult =
#if PANDA_LLVM_CLANG_MAJOR >= 13
    llvm::MemorySSAAnalysis::Result;
#else
    llvm::MemorySSAWrapperPass;
#endif

namespace llvm
{
   struct APIntCompare
   {
      bool operator()(const llvm::APInt& lhs, const llvm::APInt& rhs) const
      {
         return lhs.getBitWidth() < rhs.getBitWidth() || (lhs.getBitWidth() == rhs.getBitWidth() && lhs.ult(rhs));
      }
   };

   class DumpBambuIR
   {
#if PANDA_LLVM_CLANG_MAJOR >= 11
      bool changed;
#endif

      llvm::function_ref<llvm::TargetLibraryInfo&(llvm::Function&)> GetTLI;
      llvm::function_ref<llvm::TargetTransformInfo&(llvm::Function&)> GetTTI;
      llvm::function_ref<llvm::DominatorTree&(llvm::Function&)> GetDomTree;
      llvm::function_ref<llvm::LoopInfo&(llvm::Function&)> GetLI;
      llvm::function_ref<MemorySSAAnalysisResult&(llvm::Function&)> GetMSSA;
      llvm::function_ref<llvm::LazyValueInfo&(llvm::Function&)> GetLVI;
      llvm::function_ref<llvm::AssumptionCache&(llvm::Function&)> GetAC;
#if PANDA_LLVM_CLANG_MAJOR > 5
      llvm::function_ref<llvm::OptimizationRemarkEmitter&(llvm::Function&)> GetORE;
#endif

      bool earlyAnalysis;
      /* Serialize column control */
      const int SOL_COLUMN = 25;       /* Start of line column.  */
      const int EOL_COLUMN = 55;       /* End of line column.  */
      const int COLUMN_ALIGNMENT = 15; /* Alignment.  */

      enum irClass
      {
         ir_declaration = 0,
         ir_type,
         ir_constant,
         ir_other,
         ir_statement,
         ir_unary,
         ir_binary,
         ir_ternary,
         ir_other_expression,
         ir_reference
      };
      enum ir_rhs_class
      {
         IR_INVALID_RHS, /* The expression cannot be used on the RHS.  */
         IR_TERNARY_RHS, /* The expression is a ternary operation.  */
         IR_BINARY_RHS,  /* The expression is a binary operation.  */
         IR_UNARY_RHS,   /* The expression is a unary operation.  */
         IR_SINGLE_RHS   /* The expression is a single object (an SSA
                       name, a _DECL, a _REF, etc.  */
      };
      enum class ir_codes;
      static const char* ir_codesNames[];
      static const irClass ir_codes2irClass[];
      static const ir_rhs_class ir_rhs_class_table[];
      static const char* ValueTyNames[];
      static const std::set<std::string> builtinsNames;
      static std::string getName(const llvm::GlobalObject*);

      struct ifelseif
      {
         std::list<std::pair<const void*, unsigned int>> list_of_cond;

         ifelseif()
         {
         }
      };
      std::map<const void*, ifelseif> index2ifelseif;

      struct field_val_node
      {
         const void* name;
         const void* type;
         const void* parent;
         unsigned int size;
         unsigned int algn;
         std::string offset;
         field_val_node() : name(nullptr), type(nullptr), parent(nullptr), size(0), algn(0)
         {
         }
      };
      std::map<std::pair<const void*, unsigned int>, field_val_node> index2field_val_node;

      std::error_code EC;
      const std::string outdir_name;
      const std::string InFile;
      std::string filename;
      /// stream associated with the IR raw file
      llvm::raw_fd_ostream stream;
      /// when true only the global variables are serialized
      bool onlyGlobals;
      /// map function with name of its parameters
      const std::map<std::string, std::vector<std::string>>* fun2params;
      std::map<const llvm::Argument*, std::string> argNameTable;
      const llvm::DataLayout* DL;
      /// current module pass
      llvm::LLVMContext* moduleContext;
      std::vector<std::string> TopFunctionNames;

      /// relation between LLVM object and serialization index
      std::map<const void*, unsigned int> llvm2index;
      /// relation between LLVM object and IR_CODE
      std::map<const void*, ir_codes> llvm2ir_code;
      unsigned int last_used_index;
      std::deque<const void*> Queue;
      std::set<const void*> setOfStatementsList;
      std::set<const void*> setOfStmts;

      /// serialization data
      int column;

      /// internal identifier table
      std::set<std::string> identifierTable;
      /// unsigned integer constant table
      std::map<llvm::APInt, const void*, APIntCompare> uicTable;
      /// type_integer with specific max value
      std::map<const void*, unsigned long long int> maxValueITtable;
      std::map<const void*, llvm::LLVMContext*> ArraysContexts;

      static char buffer[LOCAL_BUFFER_LEN];

      /// relation helpers
      const void* assignCode(const void* o, ir_codes c)
      {
         if(HAS_CODE(o) && (IR_CODE(o) != c))
         {
            llvm::errs() << GET_IR_CODE_NAME(c) << " vs " << GET_IR_CODE_NAME(IR_CODE(o)) << "\n";
         }
         assert(!HAS_CODE(o) || (IR_CODE(o) == c));
         llvm2ir_code[o] = c;
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
         return reinterpret_cast<const llvm::Type*>(reinterpret_cast<size_t>(t) | 1);
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
      bool ir_has_location(const void* g) const;
      const void* ir_location(const void* t) const;
      ir_codes ir_code(const void* g) const
      {
         assert(HAS_CODE(g));
         return IR_CODE(g);
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
      const pt_info* IR_SSA_NAME_PTR_INFO(const void* t) const;
      Andersen_AA* PtoSets_AA;
      /// integer type used to convert a pointer in a signed integer type
      unsigned int SignedPointerTypeReference;

      struct ssa_node
      {
         int vers;
         pt_info ptr_info;
         const void* type;
         const void* var;
         const void* def_stmts;
         bool isVirtual;
         bool isDefault;
         ssa_node() : vers(-1), type(nullptr), var(nullptr), def_stmts(nullptr), isVirtual(false), isDefault(false)
         {
         }
      };
      void dump_pt_solution(const pt_solution* pt, const char* first_tag, const char* second_tag);

      std::map<std::pair<const void*, bool>, ssa_node> index2ssa_name;
      int last_memory_ssa_vers;
      std::map<const void*, int> memoryaccess2ssaindex;

      int last_BB_index;
      std::map<const llvm::BasicBlock*, int> BB_index_map;

      int getBB_index(const llvm::BasicBlock* BB);

      ir_rhs_class get_ir_rhs_class(ir_codes code)
      {
         return ir_rhs_class_table[static_cast<unsigned int>(code)];
      }
      template <class InstructionOrConstantExpr>
      ir_codes ir_node_code(InstructionOrConstantExpr* inst);
      ir_codes ir_expr_code(const void* stmt);
      ir_codes ir_assign_rhs_code(const void* stmt)
      {
         return ir_expr_code(stmt);
      }
      const void* getNop(const llvm::Value* operand, const void* parent);
      template <class InstructionOrConstantExpr>
      bool isSignedInstruction(const InstructionOrConstantExpr* inst) const;
      template <class InstructionOrConstantExpr>
      bool isSignedResult(const InstructionOrConstantExpr* inst) const;
      const llvm::Type* getCondSignedResult(const llvm::Value* operand, const llvm::Type* type) const;
      template <class InstructionOrConstantExpr>
      bool isSignedOperand(const InstructionOrConstantExpr* inst, unsigned index) const;
      template <class InstructionOrConstantExpr>
      bool isUnsignedOperand(const InstructionOrConstantExpr* inst, unsigned index) const;
      const void* getSSA(const llvm::Value* operand, const void* def_stmt, const llvm::Function* currentFunction,
                         bool isDefault);
      bool is_PTS(unsigned int varId, const llvm::TargetLibraryInfo& TLI, bool with_all = false);
      bool is_virtual_ssa(const void* t) const;
      bool IR_SSA_NAME_IS_DEFAULT_DEF(const void* t) const;
      const void* LowerGetElementPtrOffset(const llvm::GEPOperator* gep, const llvm::Function* currentFunction,
                                           const void*& base_node, bool& isZero);
      const void* LowerGetElementPtr(const void* type, const llvm::User* gep, const llvm::Function* currentFunction);
      const void* ir_assign_rhs_getelementptr(const void* g);
      const void* ir_assign_rhs_insertvalue(const void* g);
      const void* ir_assign_rhs_extractvalue(const void* g);
      bool temporary_addr_check(const llvm::User* inst, std::set<const llvm::User*>& visited,
                                const llvm::TargetLibraryInfo& TLI);
      const void* getOperand(const llvm::Value* operand, const llvm::Function* currentFunction);
      const void* ir_assign_lhs(const void* g);
      const void* ir_assign_rhs_alloca(const void* g);
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
      const void* IR_DECL_ABSTRACT_ORIGIN(const void* t);
      struct integer_cst_signed
      {
         const void* type;
         const void* ic;
      };
      std::map<const void*, integer_cst_signed> index2integer_cst_signed;
      template <class InstructionOrConstantExpr>
      const void* getSignedOperand(const InstructionOrConstantExpr* inst, const void* op, unsigned index);
      template <class InstructionOrConstantExpr>
      const void* getSignedOperandIndex(const InstructionOrConstantExpr* inst, unsigned index,
                                        const llvm::Function* currentFunction);
      const void* ir_assign_rhsIndex(const void* g, unsigned index);
      const void* ir_assign_rhs1(const void* g)
      {
         return ir_assign_rhsIndex(g, 0);
      }
      const void* ir_assign_rhs2(const void* g)
      {
         return ir_assign_rhsIndex(g, 1);
      }
      const void* ir_assign_rhs3(const void* g)
      {
         return ir_assign_rhsIndex(g, 2);
      }
      ir_codes ir_cond_code(const void* g)
      {
         return ir_expr_code(g);
      }
      const void* ir_cond_op(const void* g)
      {
         return ir_assign_rhsIndex(g, 0);
      }
      const void* ir_label_label(const void* g);
      const void* ir_phi_result(const void* g)
      {
         return ir_assign_lhs(g);
      }
      const void* ir_phi_virtual_result(const void* g) const;
      unsigned int ir_phi_num_args(const void* g) const;
      unsigned int ir_phi_virtual_num_args(const void* g) const;
      const void* ir_phi_arg_def(const void* g, unsigned int index);
      const void* ir_phi_virtual_arg_def(const void* g, unsigned int index);
      int ir_phi_arg_edgeBBindex(const void* g, unsigned int index);
      int ir_phi_virtual_arg_edgeBBindex(const void* g, unsigned int index);
      const void* ir_call_fn(const void* g);
      unsigned int ir_call_num_args(const void* g);
      const void* ir_call_arg(const void* g, unsigned int arg_index);
      const void* ir_return_retval(const void* g);
      const std::vector<std::pair<const void*, unsigned int>> ir_ifelseif_pairs(const void* g);
      const std::vector<std::pair<const void*, unsigned int>> ir_ifelse_pairs(const void* g);
      const void* build_custom_function_call_node(const void* g);
      const void* call_node_fn(const void* t);
      unsigned int call_node_num_args(const void* t);
      const void* call_node_arg(const void* t, unsigned int arg_index);

      struct call_node
      {
         const void* type;
         const void* fn;
         std::vector<const void*> args;
         call_node() : type(nullptr), fn(nullptr)
         {
         }
      };
      std::map<const void*, call_node> index2call_node;

      struct ir_node
      {
         ir_codes tc;
         const void* type;
         const void* op1;
         const void* op2;
         const void* op3;
         ir_node() : tc(), type(nullptr), op1(nullptr), op2(nullptr), op3(nullptr)
         {
         }
      };
      std::list<ir_node> index2ir_node;

      struct ir_nop
      {
         const void* parent;
         ir_nop() : parent(nullptr)
         {
         }
      };
      std::map<const void*, ir_nop> index2ir_nop;

      struct ir_phi_virtual
      {
         const void* parent;
         int bb_index;
         const void* res;
         std::vector<std::pair<const void*, int>> def_edfe_pairs;

         ir_phi_virtual() : parent(nullptr), bb_index(-1), res(nullptr)
         {
         }
      };
      std::map<const llvm::BasicBlock*, ir_phi_virtual> index2ir_phi_virtual;

      struct statement_list_node
      {
         const llvm::Function* F;
         statement_list_node() : F(nullptr)
         {
         }
      };
      std::map<const void*, statement_list_node> index2statement_list;

      const void* getVirtualDefStatement(llvm::MemoryAccess* defAccess, bool& isDefault, const llvm::MemorySSA& MSSA,
                                         const llvm::Function* currentFunction);
      const void* getVirtualPhi(llvm::MemoryPhi* mp, const llvm::MemorySSA& MSSA);

      const void* build3(ir_codes tc, const void* type, const void* op1, const void* op2, const void* op3);
      const void* build2(ir_codes tc, const void* type, const void* op1, const void* op2)
      {
         return build3(tc, type, op1, op2, nullptr);
      }
      const void* build1(ir_codes tc, const void* type, const void* op1)
      {
         return build3(tc, type, op1, nullptr, nullptr);
      }

      /// currently expressions do not have source file associated
      bool IR_EXPR_HAS_LOCATION(const void*) const
      {
         return false;
      }
      char* IR_EXPR_FILENAME(const void*) const
      {
         static char empty_filename[] = "";
         return empty_filename;
      }
      unsigned int IR_EXPR_LINENO(const void*) const
      {
         return 0;
      }
      unsigned int IR_EXPR_COLUMNNO(const void*) const
      {
         return 0;
      }

      bool IS_NODE_CODE_CLASS(irClass CLASS) const
      {
         return ((CLASS) >= ir_unary && (CLASS) <= ir_reference);
      }
      bool HAS_CODE(const void* NODE) const
      {
         return llvm2ir_code.find(NODE) != llvm2ir_code.end();
      }
      ir_codes IR_CODE(const void* NODE) const
      {
         assert(HAS_CODE(NODE));
         return llvm2ir_code.find(NODE)->second;
      }
      irClass IR_CODE_CLASS(ir_codes CODE) const
      {
         return ir_codes2irClass[static_cast<unsigned int>(CODE)];
      }
      const char* GET_IR_CODE_NAME(ir_codes CODE) const
      {
         return ir_codesNames[static_cast<unsigned int>(CODE)];
      }
      bool IR_DECL_P(const void* NODE) const
      {
         return (IR_CODE_CLASS(IR_CODE(NODE)) == ir_declaration);
      }
      bool IR_DECL_ASSEMBLER_NAME_SET_P(const void* t) const;
      const void* IR_DECL_ASSEMBLER_NAME(const void* t);
      const void* IR_DECL_NAME(const void* t);
      const char* IDENTIFIER_POINTER(const void* t) const;
      int IDENTIFIER_LENGTH(const void* t) const;
      int IR_VEC_LENGTH(const void* t) const;
      const void* IR_VEC_ELT(const void* t, int i) const;
      const void* IR_DECL_SOURCE_LOCATION(const void* t) const;
      const void* getParentDecl(const void* t);
      bool IR_DECL_C_BIT_FIELD(const void* t) const;
      bool IR_DECL_EXTERNAL(const void* t) const;
      bool IR_PUBLIC(const void* t) const;
      bool IR_STATIC(const void* t) const;
      bool is_builtin_fn(const void* t) const;
      const void* IR_DECL_INITIAL(const void* t);
      unsigned int IR_DECL_BITSIZEALLOC(const void* t);
      int IR_DECL_ALIGN(const void* t);
      bool IR_DECL_PACKED(const void* t) const;
      const std::string FIELD_VAL_NODE_OFFSET(const void* t);
      bool IR_READONLY(const void* t) const;
      bool IR_ADDRESSABLE(const void* t) const;
      const void* IR_OPERAND(const void* t, unsigned index);
      std::string IR_INT_CST(const void* t);
      const void* IR_TYPE_NODE(const void* t);
      bool IR_POINTER_TYPE_P(const void* t) const;
      bool IR_TYPE_UNSIGNED(const void* t) const;
      int IR_TYPE_BITSIZE(const void* t) const;
      const void* getIntegerCST(bool isSigned, llvm::LLVMContext& context, const APInt& val, const void* t);
      const void* IR_TYPE_VALUES(const void* t);
      const void* IR_TYPE_NAME(const void* t);
      unsigned int IR_TYPE_BITSIZEALLOC(const void* t);
      int IR_TYPE_ALIGN(const void* t) const;
      bool IR_TYPE_PACKED(const void* t) const;
      const std::vector<const void*> IR_TYPE_ARG_NODES(const void* t);
      uint64_t NELEMENTS(const void* t);
      bool stdarg_p(const void* t) const;
      llvm::ArrayRef<llvm::Type*> IR_TYPE_FIELDS(const void* t);
      const void* GET_FIELD_VAL_NODE(const void* t, unsigned int pos, const void* parent);

      const std::list<const void*> IR_DECL_ARGUMENTS(const void* t);
      const void* getStatement_list(const void* t);
      const void* getParentStmt(const void* g);
      int get_bb_index(const void* g);
      bool ir_has_mem_ops(const void* g);
      std::map<const llvm::Function*, std::map<const void*, std::set<const llvm::Instruction*>>>
          CurrentListofMAEntryDef;
      void serialize_vops(const void* g);
      void serialize_ir_aliased_reaching_defs(llvm::MemoryAccess* MA, llvm::MemorySSA& MSSA,
                                              std::set<llvm::MemoryAccess*>& visited,
                                              const llvm::Function* currentFunction, bool isMemDefVal,
                                              const llvm::MemoryLocation* Loc);

      const void* IR_SSA_NAME_VAR(const void* t) const;
      int IR_SSA_NAME_VERSION(const void* t) const;
      const void* IR_SSA_NAME_DEF_STMT(const void* t) const;
      const void* getMinValue(const void* t);
      const void* getMaxValue(const void* t);
      const std::list<std::pair<const void*, const void*>> CONSTRUCTOR_ELTS(const void* t);

      void DumpVersion(llvm::raw_fd_ostream& stream);

      void serialize_new_line();

      void serialize_maybe_newline();

      void serialize_pointer(const char* field, const void* ptr);

      void serialize_int(const char* field, int i);

      void serialize_int_cst(const char* field, const std::string& i);

      void serialize_real(const void* t);

      int serialize_with_double_quote(const char* input, int length);

      int serialize_with_escape(const char* input, int length);

      void serialize_string(const char* string);

      void serialize_string_field(const char* field, const char* str);

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
      void serialize_ir_child(const char* field, const void* child)
      {
         setOfStmts.insert(child);
         queue_and_serialize_index(field, child);
      }

      unsigned int queue(const void* obj);

      void SerializeFunctionHeader(const void* obj);

      void SerializeGlobalIRNode(const void* obj);

      void dequeue_and_serialize_ir(const void* t);
      void dequeue_and_serialize_statement(const void* t);
      std::string getHeaderForBuiltin(const void* t);
      void dequeue_and_serialize();

      std::map<const llvm::Value*, llvm::Metadata*> MetaDataMap;
      void buildMetaDataMap(const llvm::Module& M);

      bool lowerMemIntrinsics(llvm::Module& M);
      bool RebuildConstants(llvm::Module& M);
      bool lowerIntrinsics(llvm::Module& M);

      void
      computeMAEntryDefs(const llvm::Function* F,
                         std::map<const llvm::Function*, std::map<const void*, std::set<const llvm::Instruction*>>>&
                             CurrentListofMAEntryDef);

    public:
      DumpBambuIR(const std::string& _outdir_name, const std::string& _InFile, bool onlyGlobals,
                  std::map<std::string, std::vector<std::string>>* fun2params, bool early);

      bool exec(llvm::Module& M, const std::vector<std::string>& _TopFunctionName,
                llvm::function_ref<llvm::TargetLibraryInfo&(llvm::Function&)> GetTLI,
                llvm::function_ref<llvm::TargetTransformInfo&(llvm::Function&)> GetTTI,
                llvm::function_ref<llvm::DominatorTree&(llvm::Function&)> GetDomTree,
                llvm::function_ref<llvm::LoopInfo&(llvm::Function&)> GetLI,
                llvm::function_ref<MemorySSAAnalysisResult&(llvm::Function&)> GetMSSA,
                llvm::function_ref<llvm::LazyValueInfo&(llvm::Function&)> GetLVI,
                llvm::function_ref<llvm::AssumptionCache&(llvm::Function&)> GetAC,
#if PANDA_LLVM_CLANG_MAJOR > 5
                llvm::function_ref<llvm::OptimizationRemarkEmitter&(llvm::Function&)> GetORE,
#endif
                const std::string& costTable);
   };
} // namespace llvm

#endif
