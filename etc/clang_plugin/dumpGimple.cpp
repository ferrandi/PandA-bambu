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
* @file plugin_dumpGimpleSSA.cpp
* @brief Plugin to dump functions and global variables in gimple raw format starting from LLVM IR
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*
*/
#include "plugin_includes.hpp"
#include "llvm/Support/FileSystem.h"
#include "llvm/ADT/APFloat.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/Stmt.h"
#include "clang/Lex/Preprocessor.h"

#include <iomanip>

#define LOCAL_BUFFER_LEN 512

static std::string create_file_name_string(const std::string &outdir_name, const std::string & original_filename)
{
   std::size_t found = original_filename.find_last_of("/\\");
   std::string dump_base_name;
   if(found == std::string::npos)
      dump_base_name = original_filename;
   else
      dump_base_name = original_filename.substr(found+1);
   return outdir_name + "/" + dump_base_name + ".gimplePSSA";
}


namespace clang
{
   DumpGimpleRaw::DumpGimpleRaw(CompilerInstance &_Instance,
                          const std::string& _outdir_name, const std::string& _InFile, bool _onlyGlobals)
      : outdir_name(_outdir_name), InFile(_InFile), filename(create_file_name_string(_outdir_name,_InFile)), Instance(_Instance),
        stream(create_file_name_string(_outdir_name,_InFile), EC, llvm::sys::fs::F_RW), onlyGlobals(_onlyGlobals),
        last_free_index(0), column(0)
   {
      if( EC)
      {
         DiagnosticsEngine &D = Instance.getDiagnostics();
         D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                    "not able to open the output raw file"));

      }
      DumpVersion(stream);
   }

#define DEFTREECODE(SYM, STRING, TYPE, NARGS)   SYM,
   enum class DumpGimpleRaw::tree_codes
   {
     #include "gcc/tree.def"
     #include "gcc/c-common.def"
     #include "gcc/cp-tree.def"
   };
#undef DEFTREECODE
/* Codes of tree nodes.  */
#define DEFTREECODE(SYM, STRING, TYPE, NARGS)   STRING,
   const char* DumpGimpleRaw::tree_codesNames[] =
   {
     #include "gcc/tree.def"
     #include "gcc/c-common.def"
     #include "gcc/cp-tree.def"
   };
#undef DEFTREECODE
#define DEFTREECODE(SYM, STRING, TYPE, NARGS)   TYPE,
   const DumpGimpleRaw::tree_codes_class DumpGimpleRaw::tree_codes2tree_codes_class[] =
   {
     #include "gcc/tree.def"
     #include "gcc/c-common.def"
     #include "gcc/cp-tree.def"
   };
#undef DEFTREECODE
#define DEFTREECODE(SYM, STRING, TYPE, NARGS)   NARGS,
   const unsigned int DumpGimpleRaw::tree_codes2nargs[] =
   {
     #include "gcc/tree.def"
     #include "gcc/c-common.def"
     #include "gcc/cp-tree.def"
   };
#undef DEFTREECODE

   const void * DumpGimpleRaw::assignCodeStmtClass(const void * t)
   {
      const Expr * llvm_obj = reinterpret_cast<const Expr*>(t);
      switch (llvm_obj->getStmtClass())
      {
         case Expr::ImplicitCastExprClass:
            assignCode(t, GT(CONVERT_EXPR));
            break;
         case Expr::IntegerLiteralClass:
            assignCode(t, GT(INTEGER_CST));
            break;
         case Expr::FloatingLiteralClass:
            assignCode(t, GT(REAL_CST));
            break;
         default:
            llvm::errs() << "assignCodeStmtClass kind not supported: " << llvm_obj->getStmtClassName() << "\n";
            stream.close();
            llvm_unreachable("Plugin Error");
      }
      return t;
   }
   const void* DumpGimpleRaw::DECL_NAME(const void* t)
   {
      const Decl* llvm_obj = reinterpret_cast<const Decl*>(t);
      if(isa<NamedDecl>(*llvm_obj))
      {
         if(cast<NamedDecl>(*llvm_obj).getDeclName().isIdentifier())
         {
            const void * dn = cast<NamedDecl>(*llvm_obj).getIdentifier();
            assignCode(dn, GT(IDENTIFIER_NODE));
            return dn;
         }
         else
            return nullptr;
      }
      else
         return nullptr;
   }

   const char* DumpGimpleRaw::IDENTIFIER_POINTER (const void* t) const
   {
      const IdentifierInfo* ii = reinterpret_cast<const IdentifierInfo*>(t);
      return ii->getNameStart();
   }

   int DumpGimpleRaw::IDENTIFIER_LENGTH(const void* t) const
   {
      const IdentifierInfo* ii = reinterpret_cast<const IdentifierInfo*>(t);
      return ii->getLength();
   }

   const void* DumpGimpleRaw::DECL_CONTEXT(const void* t)
   {
      const Decl* llvm_obj = reinterpret_cast<const Decl*>(t);
      const DeclContext * scpe = llvm_obj->getDeclContext();
      if(scpe==nullptr)
         return nullptr;
      const Decl* scpeDecl= cast<Decl>(scpe);
      Decl::Kind scpe_decl_kind = scpe->getDeclKind();
      switch(scpe_decl_kind)
      {
         case Decl::Kind::TranslationUnit:
            assignCode(scpeDecl,GT(TRANSLATION_UNIT_DECL));
            break;
         default:
            llvm::errs() << "DECL_CONTEXT kind not supported: " << scpe->getDeclKindName() << "\n";
            stream.close();
            llvm_unreachable("Plugin Error");
      }
      return scpeDecl;
   }

   bool DumpGimpleRaw::DECL_C_BIT_FIELD(const void* t) const
   {
      if(TREE_CODE (t) == GT(FIELD_DECL))
      {
         const FieldDecl* llvm_obj = reinterpret_cast<const FieldDecl*>(t);
         return llvm_obj->isBitField();
      }
      else
         return false;
   }

   bool DumpGimpleRaw::DECL_EXTERNAL(const void* t) const
   {
      const NamedDecl* nd = reinterpret_cast<const NamedDecl*>(t);
      return (isa<VarDecl>(*nd) && cast<VarDecl>(*nd).hasExternalStorage()) || (isa<FunctionDecl>(*nd) && !cast<FunctionDecl>(*nd).hasBody());
   }

   bool DumpGimpleRaw::TREE_PUBLIC(const void* t) const
   {
      const NamedDecl* nd = reinterpret_cast<const NamedDecl*>(t);
      return nd->isExternallyVisible();
   }

   bool DumpGimpleRaw::TREE_STATIC(const void* t) const
   {
      const NamedDecl* nd = reinterpret_cast<const NamedDecl*>(t);
      return !nd->isExternallyVisible() && (isa<FunctionDecl>(*nd) || (isa<VarDecl>(* nd) && cast<VarDecl>(*nd).hasGlobalStorage()));

   }

   const void* DumpGimpleRaw::DECL_ARG_TYPE(const void* t)
   {
      //const ParmVarDecl * pvd = reinterpret_cast<const ParmVarDecl*>(t);
      return nullptr;//return &pvd->getType();
   }

   const void* DumpGimpleRaw::DECL_INITIAL(const void* t)
   {
      const ParmVarDecl * pvd = reinterpret_cast<const ParmVarDecl*>(t);
      if(pvd->hasInit())
      {
         return assignCodeStmtClass(pvd->getInit());
      }
      else
         return nullptr;
   }

   const void* DumpGimpleRaw::DECL_SIZE(const void*t)
   {
      const ValueDecl* llvm_obj = reinterpret_cast<const ValueDecl*>(t);
      llvm::errs() << "size: " << llvm_obj->getASTContext().getTypeInfo(llvm_obj->getType()).Width<< "\n";
      return nullptr;
   }

   int DumpGimpleRaw::DECL_ALIGN(const void* t) const
   {
      const ValueDecl* llvm_obj = reinterpret_cast<const ValueDecl*>(t);
      return llvm_obj->getASTContext().getTypeInfo(llvm_obj->getType()).Align;
   }

   const void* DumpGimpleRaw::TREE_OPERAND(const void* t, unsigned index)
   {
      const Expr * llvm_obj = reinterpret_cast<const Expr*>(t);
      switch (llvm_obj->getStmtClass())
      {
         case Expr::ImplicitCastExprClass:
            assert(index==0);
            return assignCodeStmtClass(cast<ImplicitCastExpr>(llvm_obj)->getSubExpr());
         default:
            llvm::errs() << "DECL_INITIAL kind not supported: " << llvm_obj->getStmtClassName() << "\n";
            stream.close();
            llvm_unreachable("Plugin Error");
      }
   }

   void DumpGimpleRaw::serialize_new_line()
   {
      char buffer [LOCAL_BUFFER_LEN];
      snprintf(buffer, LOCAL_BUFFER_LEN, "\n%*s", SOL_COLUMN, "");
      stream << buffer;
      column = SOL_COLUMN;
   }

   void
   DumpGimpleRaw::serialize_maybe_newline ()
   {
      int extra;

      /* See if we need a new line.  */
      if (column > EOL_COLUMN)
         serialize_new_line ();
      /* See if we need any padding.  */
      else if ((extra = (column - SOL_COLUMN) % COLUMN_ALIGNMENT) != 0)
      {
         char buffer [LOCAL_BUFFER_LEN];
         snprintf(buffer, LOCAL_BUFFER_LEN, "%*s", COLUMN_ALIGNMENT - extra, "");
         stream << buffer;
         column += COLUMN_ALIGNMENT - extra;
      }
   }

   void
   DumpGimpleRaw::serialize_pointer (const char *field, const void*ptr)
   {
     serialize_maybe_newline ();
     char buffer [LOCAL_BUFFER_LEN];
     snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: %-8lx ", field, (unsigned long) ptr);
     stream << buffer;
     column += 15;
   }

   void DumpGimpleRaw::DumpVersion(llvm::raw_fd_ostream &stream)
   {
      const char * panda_plugin_version = (const char *) PANDA_PLUGIN_VERSION;
      int version = __GNUC__, minor = __GNUC_MINOR__, patchlevel = __GNUC_PATCHLEVEL__;
      stream << "GCC_VERSION: \""<< version << "."<< minor << "." << patchlevel << "\"\n";
      stream << "PLUGIN_VERSION: \""<< panda_plugin_version << "\"\n";
   }

   void DumpGimpleRaw::serialize_int (const char *field, int i)
   {
     serialize_maybe_newline ();
     char buffer [LOCAL_BUFFER_LEN];
     snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: %-7d ", field, i);
     stream << buffer;
     column += 14;
   }

   /* Serialize wide integer i using FIELD to identify it.  */
   void DumpGimpleRaw::serialize_wide_int (const char *field, int64_t i)
   {
      serialize_maybe_newline ();
      char buffer [LOCAL_BUFFER_LEN];
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: " "%" PRId64, field, i);
      stream << buffer;
      column += 21;
   }

   /* Serialize real r using FIELD to identify it.  */
   void DumpGimpleRaw::serialize_real (const void* t)
   {
      static char string[1024];
      serialize_maybe_newline ();
      /* Code copied from print_node.  */
      /*if (TREE_OVERFLOW (t))
      {
         stream << "overflow ";
         column += 8;
      }*/
      const llvm::APFloat d = *reinterpret_cast<const llvm::APFloat*>(t);
      if (d.isInfinity())
      {
         char buffer [LOCAL_BUFFER_LEN];
         snprintf(buffer, LOCAL_BUFFER_LEN, "valr: %-7s ", "\"Inf\"");
         stream << buffer;
      }
      else if (d.isNaN())
      {
         char buffer [LOCAL_BUFFER_LEN];
         snprintf(buffer, LOCAL_BUFFER_LEN, "valr: %-7s ", "\"Nan\"");
         stream << buffer;
      }
      else
      {
         //real_to_decimal (string, &d, sizeof (string), 0, 1);
         stream << "valr: \""<< string << "\" ";
      }
      {
         stream << "valx: \"";
         //real_to_hexadecimal(string, &d, sizeof (string), 0, 0);
         stream << string;
         stream << "\"";
      }
      column += 21;
   }

   int DumpGimpleRaw::serialize_with_double_quote(const char * input, int length)
   {
      int new_length;
      stream << "\"";
      new_length = serialize_with_escape(input, length);
      stream << "\"";
      return new_length + 2;
   }

   /* Add a backslash before an escape sequence to serialize the string
      with the escape sequence */
   int DumpGimpleRaw::serialize_with_escape(const char * input, int length)
   {
      int i;
      int k = 0;
      for (i = 0; i < length; i++)
      {
         switch (input[i])
         {
            case '\n':
            {
               /* new line*/
               stream <<  "\\";
               stream <<  "n";
               k += 2;
               break;
            }
            case '\t':
            {
               /* horizontal tab */
               stream <<  "\\";
               stream <<  "t";
               k += 2;
               break;
            }
            case '\v':
            {
               /* vertical tab */
               stream <<  "\\";
               stream <<  "v";
               k += 2;
               break;
            }
            case '\b':
            {
               /* backspace */
               stream <<  "\\";
               stream <<  "b";
               k += 2;
               break;
            }
            case '\r':
            {
               /* carriage return */
               stream <<  "\\";
               stream <<  "r";
               k += 2;
               break;
            }
            case '\f':
            {
               /* jump page */
               stream <<  "\\";
               stream <<  "f";
               k += 2;
               break;
            }
            case '\a':
            {
               /* alarm */
               stream <<  "\\";
               stream <<  "a";
               k += 2;
               break;
            }
            case '\\':
            {
               /* backslash */
               stream <<  "\\";
               stream <<  "\\";
               k += 2;
               break;
            }
            case '\"':
            {
               /* double quote */
               stream <<  "\\";
               stream <<  "\"";
               k += 2;
               break;
            }
            case '\'':
            {
               /* quote */
               stream <<  "\\";
               stream <<  "\'";
               k += 2;
               break;
            }
            case '\0':
            {
               /* null */
               stream <<  "\\";
               stream <<  "0";
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
   void DumpGimpleRaw::serialize_string (const char *string)
   {
     serialize_maybe_newline ();
     char buffer [LOCAL_BUFFER_LEN];
     snprintf(buffer, LOCAL_BUFFER_LEN, "%-13s ", string);
     stream << buffer;
     if (std::strlen (string) > 13)
       column += strlen (string) + 1;
     else
       column += 14;
   }

   /* Serialize the string field S.  */
   void DumpGimpleRaw::serialize_string_field (const char *field, const char *str)
   {
      int length;
      serialize_maybe_newline ();
      char buffer [LOCAL_BUFFER_LEN];
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: ", field);
      stream << buffer;
      length = std::strlen(str);
      length = serialize_with_double_quote(str, length);
      if (length > 7)
         column += 6 + length + 1;
      else
         column += 14;
   }

   void DumpGimpleRaw::serialize_string_cst (const char *field, const char *str, int length, unsigned int precision)
   {
      int new_length;
      serialize_maybe_newline ();
      if (precision == 8)
      {
         char buffer [LOCAL_BUFFER_LEN];
         snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: ", field);
         stream << buffer;
         new_length = serialize_with_double_quote(str, length - 1);
         if (new_length > 7)
            column += 6 + new_length + 1;
         else
            column += 14;
         serialize_int ("lngt", length);
      }
      else
      {
         const unsigned int * string = (const unsigned int *) str;
         unsigned int i, lngt = length / 4 - 1;
         char buffer [LOCAL_BUFFER_LEN];
         snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: \"", field);
         stream << buffer;
         for (i = 0; i < lngt; i++)
         {
            snprintf(buffer, LOCAL_BUFFER_LEN, "\\x%x", string[i]);
            stream << buffer;
         }
         stream << "\" ";
         column += 7 + lngt;
         serialize_int ("lngt", lngt + 1);
      }
   }


   void DumpGimpleRaw::queue_and_serialize_index(const char *field, const void* t)
   {
      unsigned int index;
      if(t==nullptr)
         return;
      if(llvm2index.find(t) != llvm2index.end())
      {
         index = llvm2index.find(t)->second;
         assert(index <= last_free_index);
      }
      else
      {
        /* If we haven't, add it to the queue.  */
        index = queue (t);
      }
      serialize_maybe_newline ();
      char buffer [LOCAL_BUFFER_LEN];
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: ", field);
      stream << buffer;
      column += 6;
      serialize_index (index);

   }

   void DumpGimpleRaw::serialize_index (unsigned int index)
   {
      char buffer [LOCAL_BUFFER_LEN];
      snprintf(buffer, LOCAL_BUFFER_LEN, "@%-6u ", index);
      stream << buffer;
      column += 8;
   }

   void DumpGimpleRaw::queue_and_serialize_type (const void* t)
   {
     //queue_and_serialize_index ("type", TREE_TYPE (t));
   }


   void DumpGimpleRaw::ManageSRCP(const SourceLocation& xloc)
   {
      char buffer [LOCAL_BUFFER_LEN];
      const SourceManager& sm = Instance.getSourceManager();
      if(xloc.isInvalid())
      {
         serialize_maybe_newline ();
         snprintf(buffer, LOCAL_BUFFER_LEN, "srcp: \"<built-in>\":0:0 ");
         stream << buffer;
         column += 12 + strlen ("<built-in>") + 8;
      }
      else
      {
         if (xloc.isFileID())
         {
            PresumedLoc PLoc = sm.getPresumedLoc(xloc);
            if (PLoc.isValid())
            {
               serialize_maybe_newline ();
               snprintf(buffer, LOCAL_BUFFER_LEN, "srcp: \"%s\":%-d:%-6d ", PLoc.getFilename(),
                        PLoc.getLine(), PLoc.getColumn());
               stream << buffer;
               column += 12 + strlen (PLoc.getFilename()) + 8;
            }
            else
               llvm_unreachable("Unexpected source location case");
         }
         else if(xloc.isMacroID())
         {
            PresumedLoc PLoc = sm.getPresumedLoc(sm.getExpansionLoc(xloc));
            if (PLoc.isValid())
            {
               serialize_maybe_newline ();
               snprintf(buffer, LOCAL_BUFFER_LEN, "srcp: \"%s\":%-d:%-6d ", PLoc.getFilename(),
                        PLoc.getLine(), PLoc.getColumn());
               stream << buffer;
               column += 12 + strlen (PLoc.getFilename()) + 8;
            }
            else
               llvm_unreachable("Unexpected source location case");
         }
         else
            llvm_unreachable("Unexpected source location case");
      }
   }

   void DumpGimpleRaw::dequeue_and_serialize()
   {
      assert(!Queue.empty());
      const void* t = Queue.front();
      assert(t);
      Queue.pop_front();

      assert(llvm2index.find(t) != llvm2index.end());
      unsigned int index = llvm2index.find(t)->second;

      /* Print the node index.  */
      serialize_index (index);

      const char * code_name = GET_TREE_CODE_NAME(TREE_CODE (t));

      char buffer [LOCAL_BUFFER_LEN];
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-16s ", code_name);
      stream << buffer;
      column = 25;

      tree_codes code = TREE_CODE (t);
      tree_codes_class code_class = TREE_CODE_CLASS(code);

      if (IS_EXPR_CODE_CLASS (code_class))
      {
         queue_and_serialize_type (t);
         const Expr* llvm_obj = reinterpret_cast<const Expr*>(t);
         const SourceLocation xloc = llvm_obj->getExprLoc();
         ManageSRCP(xloc);
         switch (code_class)
         {
           case tcc_unary:
             serialize_child ("op", TREE_OPERAND (t, 0));
             break;

           case tcc_binary:
           case tcc_comparison:
             serialize_child ("op", TREE_OPERAND (t, 0));
             serialize_child ("op", TREE_OPERAND (t, 1));
             break;

           case tcc_expression:
           case tcc_reference:
           case tcc_statement:
           case tcc_vl_exp:
             /* These nodes are handled explicitly below.  */
             break;

           default:
             llvm_unreachable ("Unexpected case");
        }


      }
      else if(DECL_P (t))
      {
         const Decl* llvm_obj = reinterpret_cast<const Decl*>(t);
         llvm::errs() << "|" << llvm_obj->getDeclKindName() << "\n";
         if(DECL_NAME(t))
            serialize_child ("name", DECL_NAME (t));
         //if (DECL_ASSEMBLER_NAME_SET_P (t) && DECL_ASSEMBLER_NAME (t) != DECL_NAME (t))
         //  serialize_child ("mngl", DECL_ASSEMBLER_NAME (t));
         const auto getMangledName = [&](const NamedDecl* decl)
         {
            auto mangleContext = Instance.getASTContext().createMangleContext();
            if (!mangleContext->shouldMangleDeclName(decl))
               return reinterpret_cast<const void *>(decl);
            std::string mangledName;
            llvm::raw_string_ostream ostream(mangledName);
            mangleContext->mangleName(decl, ostream);
            ostream.flush();
            delete mangleContext;
            allocatedStrings.insert(mangledName);
            Preprocessor &PP = Instance.getPreprocessor();
            IdentifierTable& IT = PP.getIdentifierTable();
            if(internalIdentifierTable.find(mangledName) == internalIdentifierTable.end())
               internalIdentifierTable[mangledName] = &IT.get(mangledName);
            const IdentifierInfo* ii = internalIdentifierTable.find(mangledName)->second;
            assignCode(ii,GT(IDENTIFIER_NODE));
            return reinterpret_cast<const void *>(ii);
         };
         if(isa<NamedDecl>(*llvm_obj))
         {
            const NamedDecl* nd =static_cast<const NamedDecl*>(llvm_obj);
            if(!nd->getDeclName().isIdentifier() || t != getMangledName(nd))
               serialize_child ("mngl", getMangledName(nd));
         }
         //if (DECL_ABSTRACT_ORIGIN (t))
         //  serialize_child ("orig", DECL_ABSTRACT_ORIGIN (t));
         /* And types.  */
         queue_and_serialize_type (t);
         serialize_child ("scpe", DECL_CONTEXT (t));

         const SourceLocation xloc = llvm_obj->getLocation();
         ManageSRCP(xloc);
         if(llvm_obj->isImplicit())
            serialize_string ("artificial");
         if(llvm_obj->getGlobalID())
            serialize_int("uid", llvm_obj->getGlobalID());
         if(code == GT(FUNCTION_DECL))
         {
            llvm::errs() << "FUNCTION_DECL\n";
         }
         if(code == GT(VAR_DECL))
         {
            llvm::errs() << "VAR_DECL\n";
         }
      }
      else if (code_class == tcc_type)
      {

      }
      else if (code_class == tcc_constant)
         queue_and_serialize_type (t);


      switch (code)
      {
         case GT(IDENTIFIER_NODE):
           serialize_string_field ("strg", IDENTIFIER_POINTER (t));
           serialize_int ("lngt", IDENTIFIER_LENGTH (t));
           break;
         case GT(VAR_DECL):
         case GT(PARM_DECL):
         case GT(FIELD_DECL):
         case GT(RESULT_DECL):
            if (TREE_CODE (t) == GT(FIELD_DECL) && DECL_C_BIT_FIELD (t))
               serialize_string ("bitfield");
            if(TREE_CODE (t) == GT(VAR_DECL))
            {
               if (DECL_EXTERNAL (t))
                  serialize_string ("extern");
               else if (!TREE_PUBLIC (t) && TREE_STATIC (t))
                  serialize_string ("static");
            }
            if (TREE_CODE (t) == GT(PARM_DECL))
              serialize_child ("argt", DECL_ARG_TYPE (t));
            else if(DECL_INITIAL (t))
              serialize_child ("init", DECL_INITIAL (t));
            serialize_child ("size", DECL_SIZE (t));
            serialize_int ("algn", DECL_ALIGN (t));
#if 0
            if (TREE_CODE (t) == FIELD_DECL && DECL_PACKED(t))
            {
               serialize_string("packed");
            }

            if (TREE_CODE (t) == FIELD_DECL)
            {
              if (DECL_FIELD_OFFSET (t))
                serialize_child ("bpos", bit_position (t));
            }
            else if (TREE_CODE (t) == VAR_DECL || TREE_CODE (t) == PARM_DECL)
            {
              serialize_int ("used", TREE_USED (t));
              if (DECL_REGISTER (t))
                serialize_string ("register");
            }
            if(TREE_READONLY(t) && TREE_CODE (t) != RESULT_DECL && TREE_CODE (t) != FIELD_DECL)
               serialize_string ("readonly");
#endif
            break;
      }
      stream << "\n";

   }

   void DumpGimpleRaw::SerializeGimpleFunctionHeader(const void*obj)
   {
      assert(TREE_CODE(obj) == GT(FUNCTION_DECL));
      const FunctionDecl *fd = reinterpret_cast<const FunctionDecl *>(obj);
      stream << "\n;; Function " << fd->getNameAsString() << " (" << fd->getNameAsString() << ")\n\n";
      stream << ";; " << fd->getNameAsString() << " (";
      bool first_par = true;
      for(const auto par : fd->parameters())
      {
         if(first_par)
            first_par = false;
         else
            stream << ", ";
         stream << QualType::getAsString(par->getType().split()) << " ";
         stream << par->getNameAsString();
      }
      stream << ")\n";
   }

   unsigned int DumpGimpleRaw::queue (const void* obj)
   {
      unsigned int index = ++last_free_index;
      assert(llvm2index.find(obj) == llvm2index.end());
      llvm2index[obj] = index;
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

   bool DumpGimpleRaw::HandleTopLevelDecl(DeclGroupRef d)
   {
       /// dumping global variables
       for(const auto el : d)
       {
           const clang::VarDecl *vd = llvm::dyn_cast<clang::VarDecl>(el);
           if(vd && vd->isFileVarDecl())
           {
               llvm::errs() << "Read top-level variable decl: '";
               llvm::errs() << vd->getDeclName().getAsString() ;
               llvm::errs() << "\n";
               assignCode(vd, GT(VAR_DECL));
               SerializeGimpleGlobalTreeNode(vd);
           }
       }
       /// dumping function declaration
       if(!onlyGlobals)
       {
          for(const auto el : d)
          {
             const FunctionDecl *fd = dyn_cast<FunctionDecl>(el);
             if (fd)
             {
                llvm::errs() << "top-level-function declaration: \"" << fd->getNameAsString() << "\"\n";
                assignCode(fd, GT(FUNCTION_DECL));
                SerializeGimpleGlobalTreeNode(fd);
             }
          }
       }
       return true;
   }


}
