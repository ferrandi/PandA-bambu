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
*              Copyright (c) 2004-2017 Politecnico di Milano
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

#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include <set>
#include <unordered_set>
#include <map>
#include <list>

#define GT(code) tree_codes::code
#define LOCAL_BUFFER_LEN 512

class dummyConsumer : public clang::ASTConsumer
{
   public:
      dummyConsumer() {}

};

namespace llvm {
   class Module;
   class Type;
   class DataLayout;
   class Constant;
   class ModulePass;
}


namespace clang {


   class DumpGimpleRaw
   {
         /* Serialize column control */
         const int SOL_COLUMN=25;		/* Start of line column.  */
         const int EOL_COLUMN=55;		/* End of line column.  */
         const int COLUMN_ALIGNMENT=15;	/* Alignment.  */

         enum tree_codes_class {
           tcc_exceptional=0, /* An exceptional code (fits no category).  */
           tcc_constant,    /* A constant.  */
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
         enum class tree_codes;
         static const char* tree_codesNames[];
         static const tree_codes_class tree_codes2tree_codes_class[];
         static const unsigned int tree_codes2nargs[];
         static const char* ValueTyNames[];

         struct tree_list
         {
               const void * purp;
               const void * valu;
               unsigned int chan;
               tree_list(): purp(nullptr), valu(nullptr), chan(0) {}
         };
         std::map<unsigned int, tree_list> index2tree_list;
         /// memoization map used to avoid the recompuation of tree associated with a given node
         std::map<const void*, const void*> memoization_tree_list;

         struct field_decl
         {
               const void* name;
               const void* type;
               const void* scpe;
               const void* size;
               unsigned int algn;
               const void* bpos;
               field_decl() : name(nullptr), type(nullptr), scpe(nullptr), size(nullptr), algn(0), bpos(nullptr) {}
         };
         std::map<std::pair<const void*, unsigned int>, field_decl> index2field_decl;

         std::error_code EC;
         const std::string outdir_name;
         const std::string InFile;
         std::string filename;
         CompilerInstance &Instance;
         ///stream associated with the gimple raw file
         llvm::raw_fd_ostream stream;
         ///when true only the global variables are serialized
         bool onlyGlobals;
         const llvm::DataLayout* DL;
         /// current module pass
         llvm::ModulePass* modulePass;


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

         ///internal identifier table
         std::set<std::string> identifierTable;
         ///unsigned integer constant table
         std::map<uint64_t, llvm::Constant*> uicTable;
         /// type_integer with specific max value
         std::map<const void*, unsigned int> maxValueITtable;
         std::map<const void*, llvm::LLVMContext*> ArraysContexts;

         std::string getTypeName(const void * ty) const;

         static char buffer [LOCAL_BUFFER_LEN];

         /// relation helpers
         const void* assignCode(const void*o, tree_codes c)
         {
            if((llvm2tree_code.find(o) != llvm2tree_code.end()) && (llvm2tree_code.find(o)->second!=c))
               llvm::errs() << GET_TREE_CODE_NAME(c) << " vs " << GET_TREE_CODE_NAME(llvm2tree_code.find(o)->second) << "\n";
            //assert((llvm2tree_code.find(o) == llvm2tree_code.end()) || (llvm2tree_code.find(o)->second==c));
            llvm2tree_code[o]=c;
            return o;
         }
         const void* assignCodeAuto(const void*o);
         const void* assignCodeType(const llvm::Type *ty);

         bool CheckSignedTag(const llvm::Type *t) const {return reinterpret_cast<size_t>(t)&1;}
         const llvm::Type * NormalizeSignedTag(const llvm::Type *t) const {return reinterpret_cast<const llvm::Type*>(reinterpret_cast<size_t>(t)&(~1ULL));}
         const llvm::Type * AddSignedTag(const llvm::Type *t) const {assert(CheckSignedTag(t)==false);return reinterpret_cast<const llvm::Type*>(reinterpret_cast<size_t>(t)|1);}
         const void * AddSignedTag(const void *t) const {return AddSignedTag(reinterpret_cast<const llvm::Type*>(t));}


         struct expanded_location
         {
               std::string filename;
               const char * file;
               unsigned int line;
               unsigned int column;
               expanded_location() : file(nullptr), line(0), column(0) {}
               expanded_location(const expanded_location& el)
               {
                  filename = el.filename;
                  file = filename.c_str();
                  line = el.line;
                  column = el.column;
               }
               expanded_location & operator=(const expanded_location&el)
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

         expanded_location expand_location(const void * i) const;
         /// currrently expressions do not have source file associated
         bool EXPR_HAS_LOCATION(const void*) const {return false;}
         char * EXPR_FILENAME(const void*) const {return nullptr;}
         unsigned int EXPR_LINENO(const void*) const {return 0;}
         unsigned int EXPR_COLUMNNO(const void*) const {return 0;}

         bool IS_EXPR_CODE_CLASS(tree_codes_class CLASS) const {return ((CLASS) >= tcc_reference && (CLASS) <= tcc_expression);}
         tree_codes TREE_CODE(const void* NODE) const {assert(llvm2tree_code.find(NODE) != llvm2tree_code.end()); return llvm2tree_code.find(NODE)->second;}
         tree_codes_class TREE_CODE_CLASS(tree_codes CODE) const {return tree_codes2tree_codes_class[static_cast<unsigned int>(CODE)];}
         const char * GET_TREE_CODE_NAME(tree_codes CODE) const {return tree_codesNames[static_cast<unsigned int>(CODE)];}
         bool DECL_P(const void* NODE) const {return (TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_declaration);}
         bool DECL_ASSEMBLER_NAME_SET_P (const void *t) const;
         const void* DECL_ASSEMBLER_NAME (const void *t);
         const void* DECL_NAME(const void* t);
         const char* IDENTIFIER_POINTER (const void* t) const;
         int IDENTIFIER_LENGTH(const void* t) const;
         const void* TREE_PURPOSE(const void *t);
         const void* TREE_VALUE(const void* t) const;
         const void* TREE_CHAIN(const void* t) const;
         const void *DECL_SOURCE_LOCATION(const void*t) const;
         const void* DECL_CONTEXT(const void* t);
         int DECL_UID(const void *t) const;
         bool DECL_C_BIT_FIELD (const void* t) const;
         bool DECL_EXTERNAL (const void* t) const;
         bool TREE_PUBLIC (const void* t) const;
         bool TREE_STATIC (const void* t) const;
         const void* DECL_ARG_TYPE (const void* t);
         const void* DECL_INITIAL (const void*t);
         const void* DECL_SIZE (const void*t);
         int DECL_ALIGN (const void* t);
         bool DECL_PACKED(const void* t) const;
         bool DECL_FIELD_OFFSET (const void* t) const;
         const void* bit_position (const void* t);
         int TREE_USED (const void*t) const;
         bool DECL_REGISTER (const void* t) const;
         bool TREE_READONLY(const void* t) const;
         const void* TREE_OPERAND(const void* t, unsigned index);
         int64_t TREE_INT_CST_LOW(const void*t) const;

         const void* TREE_TYPE(const void*t);
         bool TYPE_UNSIGNED(const void*t) const;
         int TYPE_PRECISION (const void*t) const;
         bool COMPLEX_FLOAT_TYPE_P(const void*t) const;
         bool TYPE_SATURATING (const void*t) const;
         const void* TYPE_MIN_VALUE (const void*t);
         const void* TYPE_MAX_VALUE (const void*t);
         const void* TYPE_VALUES (const void* t);
         const void* TYPE_NAME(const void* t);
         const void* TYPE_SIZE (const void* t);
         const void* TYPE_CONTEXT (const void* t);
         int TYPE_ALIGN (const void*t) const;
         bool TYPE_PACKED(const void*t) const;
         const void* TYPE_ARG_TYPES (const void* t);
         const void* TYPE_DOMAIN(const void*t);
         bool stdarg_p(const void* t) const;
         llvm::ArrayRef<llvm::Type *> TYPE_FIELDS(const void*t);
         const void * GET_FIELD_DECL(const llvm::Type*t, unsigned int pos, const void * scpe);
         const void * GET_METHOD_TYPE(const llvm::Type*t, unsigned int pos, const void * scpe);
         const void* TYPE_METHOD_BASETYPE(const void* t);

         const void * DECL_ARGUMENTS (const void*t);
         const void* getStatement_list(const void*t);

         const std::list<std::pair<const void *, const void*>> CONSTRUCTOR_ELTS (const void*t);

         void DumpVersion(llvm::raw_fd_ostream &stream);

         void serialize_new_line();

         void serialize_maybe_newline ();

         void serialize_pointer(const char *field, const void*ptr);

         void serialize_int(const char *field, int i);

         void serialize_wide_int(const char *field, int64_t i);

         void serialize_real(const void* t);

         int serialize_with_double_quote(const char * input, int length);

         int serialize_with_escape(const char * input, int length);

         void serialize_string (const char *string);

         void serialize_string_field (const char *field, const char *str);

         void serialize_string_cst (const char *field, const char *str, int length, unsigned int precision);

         void serialize_index (unsigned int index);

         void queue_and_serialize_type (const void * t);
         void queue_and_serialize_index(const char *field, const void* t);

         void serialize_child(const char * field, const void*child) {queue_and_serialize_index(field, child);}
         void serialize_statement_child(const char * field, const void*child) {setOfStatementsList.insert(child);queue_and_serialize_index(field, child);}
         void serialize_gimple_child(const char * field, const void*child) {setOfGimples.insert(child);queue_and_serialize_index(field, child);}

         unsigned int queue (const void* obj);

         void SerializeGimpleFunctionHeader(const void*obj);

         void SerializeGimpleGlobalTreeNode(const void*obj);

         void dequeue_and_serialize_gimple(const void* t);
         void dequeue_and_serialize_statement (const void* t);
         void dequeue_and_serialize();

      public:
         DumpGimpleRaw(CompilerInstance &_Instance,
                       const std::string& _outdir_name, const std::string& _InFile, bool onlyGlobals);

         bool runOnModule(llvm::Module &M, llvm::ModulePass *modulePass);


   };
}

#endif
