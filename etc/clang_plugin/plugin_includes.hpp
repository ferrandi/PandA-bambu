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

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

#define GT(code) tree_codes::code
namespace clang {

   class DumpGimpleRaw : public ASTConsumer
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

         std::error_code EC;
         const std::string outdir_name;
         const std::string InFile;
         std::string filename;
         CompilerInstance &Instance;
         ///stream associated with the gimple raw file
         llvm::raw_fd_ostream stream;
         ///when true only the global variables are serialized
         bool onlyGlobals;
         /// relation between LLVM object and serialization index
         std::map<const void*, unsigned int> llvm2index;
         /// relation between LLVM object and TREE_CODE
         std::map<const void*, tree_codes> llvm2tree_code;
         unsigned int last_free_index;
         std::deque<const void*> Queue;

         /// serialization data
         int column;

         ///internal identifier table
         std::map<const std::string, IdentifierInfo*> internalIdentifierTable;
         std::set<std::string> allocatedStrings;

         /// relation helpers
         void assignCode(const void*o, tree_codes c) {assert(llvm2tree_code.find(o) == llvm2tree_code.end() || llvm2tree_code.find(o)->second==c); llvm2tree_code[o]=c;}
         const void *assignCodeStmtClass(const void * t);

         bool IS_EXPR_CODE_CLASS(tree_codes_class CLASS) const {return ((CLASS) >= tcc_reference && (CLASS) <= tcc_expression);}
         tree_codes TREE_CODE(const void* NODE) const {assert(llvm2tree_code.find(NODE) != llvm2tree_code.end()); return llvm2tree_code.find(NODE)->second;}
         tree_codes_class TREE_CODE_CLASS(tree_codes CODE) const {return tree_codes2tree_codes_class[static_cast<unsigned int>(CODE)];}
         const char * GET_TREE_CODE_NAME(tree_codes CODE) const {return tree_codesNames[static_cast<unsigned int>(CODE)];}
         bool DECL_P(const void* NODE) const {return (TREE_CODE_CLASS (TREE_CODE (NODE)) == tcc_declaration);}
         const void* DECL_NAME(const void* t);
         const char* IDENTIFIER_POINTER (const void* t) const;
         int IDENTIFIER_LENGTH(const void* t) const;
         const void* DECL_CONTEXT(const void* t);
         bool DECL_C_BIT_FIELD (const void* t) const;
         bool DECL_EXTERNAL (const void* t) const;
         bool TREE_PUBLIC (const void* t) const;
         bool TREE_STATIC (const void* t) const;
         const void* DECL_ARG_TYPE (const void* t);
         const void* DECL_INITIAL (const void*t);
         const void* DECL_SIZE (const void*t);
         int DECL_ALIGN (const void* t) const;
         const void* TREE_OPERAND (const void* t, unsigned index);

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

         void serialize_child(const char * field, const void*child) {queue_and_serialize_index (field, child);}

         unsigned int queue (const void* obj);

         void SerializeGimpleFunctionHeader(const void*obj);

         void SerializeGimpleGlobalTreeNode(const void*obj);

         void dequeue_and_serialize();

         void ManageSRCP(const SourceLocation &xloc);

      public:
         DumpGimpleRaw(CompilerInstance &_Instance,
                       const std::string& _outdir_name, const std::string& _InFile, bool onlyGlobals);


         virtual bool HandleTopLevelDecl(DeclGroupRef d) override;

   };
}

#endif
