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
*              Copyright(c) 2004-2017 Politecnico di Milano
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
#include "plugin_includes.hpp"

#include "clang/AST/AST.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/CFG.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/FileSystem.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/Stmt.h"
#include "clang/Lex/Preprocessor.h"

#include <iomanip>
#include <cxxabi.h>

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

   char DumpGimpleRaw::buffer [LOCAL_BUFFER_LEN];

   DumpGimpleRaw::DumpGimpleRaw(CompilerInstance &_Instance,
                          const std::string& _outdir_name, const std::string& _InFile, bool _onlyGlobals)
      : outdir_name(_outdir_name), InFile(_InFile), filename(create_file_name_string(_outdir_name,_InFile)), Instance(_Instance),
        stream(create_file_name_string(_outdir_name,_InFile), EC, llvm::sys::fs::F_RW), onlyGlobals(_onlyGlobals),
        DL(0),modulePass(0),
        last_used_index(0), column(0)
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
#define DEFGSCODE(SYM, NAME, GSSCODE)	SYM,
   enum class DumpGimpleRaw::tree_codes
   {
     #include "gcc/tree.def"
     #include "gcc/c-common.def"
     #include "gcc/cp-tree.def"
     #include "gcc/gimple.def"
   };
#undef DEFTREECODE
#undef DEFGSCODE
/* Codes of tree nodes.  */
#define DEFTREECODE(SYM, STRING, TYPE, NARGS)   STRING,
#define DEFGSCODE(SYM, NAME, GSSCODE)	NAME,
   const char* DumpGimpleRaw::tree_codesNames[] =
   {
     #include "gcc/tree.def"
     #include "gcc/c-common.def"
     #include "gcc/cp-tree.def"
     #include "gcc/gimple.def"
   };
#undef DEFTREECODE
#undef DEFGSCODE
#define DEFTREECODE(SYM, STRING, TYPE, NARGS)   TYPE,
#define DEFGSCODE(SYM, NAME, GSSCODE)	tcc_statement,
   const DumpGimpleRaw::tree_codes_class DumpGimpleRaw::tree_codes2tree_codes_class[] =
   {
     #include "gcc/tree.def"
     #include "gcc/c-common.def"
     #include "gcc/cp-tree.def"
     #include "gcc/gimple.def"
   };
#undef DEFTREECODE
#undef DEFGSCODE
#define DEFTREECODE(SYM, STRING, TYPE, NARGS)   NARGS,
#define DEFGSCODE(SYM, NAME, GSSCODE)	0,
   const unsigned int DumpGimpleRaw::tree_codes2nargs[] =
   {
     #include "gcc/tree.def"
     #include "gcc/c-common.def"
     #include "gcc/cp-tree.def"
     #include "gcc/gimple.def"
   };
#undef DEFTREECODE
#undef DEFGSCODE

   const char* DumpGimpleRaw::ValueTyNames[] = {
   #define HANDLE_VALUE(Name) #Name ,
   #include "llvm/IR/Value.def"
   #define HANDLE_INST(N, OPC, CLASS) #OPC ,
   #include "llvm/IR/Instruction.def"
    };

   std::string DumpGimpleRaw::getTypeName(const void * t) const
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
            if(ty->getScalarSizeInBits()==1)
               return "_Bool";
            else if(ty->getScalarSizeInBits()>1 && ty->getScalarSizeInBits()<=8)
               return is_signed ? "signed char" : "unsigned char";
            else if(ty->getScalarSizeInBits()>8 && ty->getScalarSizeInBits()<=16)
               return is_signed ? "short int" : "unsigned short int";
            else if(ty->getScalarSizeInBits()>16 && ty->getScalarSizeInBits()<=32)
               return is_signed ? "int" : "unsigned int";
            else if(ty->getScalarSizeInBits()>32 && ty->getScalarSizeInBits()<=64)
               return is_signed ? "long long int" : "unsigned long long int";
            else
               llvm_unreachable("not expected integer bitwidth size");
         }
      }
      llvm_unreachable("not managed");
   }

   const void * DumpGimpleRaw::assignCodeAuto(const void * t)
   {
      assert(t);
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
         case llvm::Value::ConstantStructVal:
            return assignCode(t, GT(CONSTRUCTOR));
         case llvm::Value::ConstantAggregateZeroVal:
            return assignCode(t, GT(CONSTRUCTOR));
         case llvm::Value::ConstantDataArrayVal:
            return assignCode(t, GT(CONSTRUCTOR));
         #define HANDLE_BINARY_INST(N, OPC, CLASS)                   \
         case llvm::Value::InstructionVal + llvm::Instruction::OPC:  \
           return assignCode(t, GT(GIMPLE_ASSIGN));
         #include "llvm/IR/Instruction.def"
         case llvm::Value::InstructionVal+llvm::Instruction::Store:
         case llvm::Value::InstructionVal+llvm::Instruction::Load:
            return assignCode(t, GT(GIMPLE_ASSIGN));
         case llvm::Value::InstructionVal+llvm::Instruction::Ret:
            return assignCode(t, GT(GIMPLE_RETURN));
         default:
            llvm::errs() << "assignCodeAuto kind not supported: " << ValueTyNames[vid] << "\n";
            stream.close();
            llvm_unreachable("Plugin Error");
      }
   }
   bool DumpGimpleRaw::DECL_ASSEMBLER_NAME_SET_P(const void *t) const
   {
      if(TREE_CODE(t) == GT(TRANSLATION_UNIT_DECL)) return  false;
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      if(!llvm_obj->getName().empty())
      {
         std::string declname = std::string(llvm_obj->getName());
         int status;
         char * demangled_outbuffer = abi::__cxa_demangle(declname.c_str(), NULL, NULL, &status);
         if(status==0)
         {
            free(demangled_outbuffer);
            return true;
         }
         else
            assert(demangled_outbuffer==nullptr);
      }
      return false;
   }

   const void* DumpGimpleRaw::DECL_ASSEMBLER_NAME(const void * t)
   {
      assert(TREE_CODE(t) != GT(TRANSLATION_UNIT_DECL));
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      if(!llvm_obj->getName().empty())
      {
         std::string declname = std::string(llvm_obj->getName());
         if(identifierTable.find(declname) == identifierTable.end())
            identifierTable.insert(declname);
         const void * dn = identifierTable.find(declname)->c_str();
         return assignCode(dn, GT(IDENTIFIER_NODE));
      }
      else
         llvm_unreachable("DECL_ASSEMBLER_NAME: DECL_ASSEMBLER_NAME_SET_P is not true");
   }

   const void* DumpGimpleRaw::DECL_NAME(const void* t)
   {
      if(TREE_CODE(t) == GT(TRANSLATION_UNIT_DECL)) return nullptr;
      if(TREE_CODE(t) == GT(FIELD_DECL))
      {
         const field_decl* ty = reinterpret_cast<const field_decl*>(t);
         return ty->name;
      }
      const llvm::Value* llvm_obj = reinterpret_cast<const llvm::Value*>(t);
      if(llvm_obj->hasName())
      {
         std::string declname = std::string(llvm_obj->getName());
         int status;
         char * demangled_outbuffer = abi::__cxa_demangle(declname.c_str(), NULL, NULL, &status);
         if(status==0)
         {
            if(std::string(demangled_outbuffer).find(':') == std::string::npos)
            {
               declname = demangled_outbuffer;
               auto parPos = declname.find('(');
               if(parPos != std::string::npos)
                  declname = declname.substr(0,parPos);
            }
            free(demangled_outbuffer);
         }
         else
            assert(demangled_outbuffer==nullptr);

         if(identifierTable.find(declname) == identifierTable.end())
            identifierTable.insert(declname);
         const void * dn = identifierTable.find(declname)->c_str();
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
      return std::strlen(ii);
   }

   const void* DumpGimpleRaw::TREE_PURPOSE(const void *t)
   {
      const tree_list *tl = reinterpret_cast<const tree_list*>(t);
      return tl->purp;
   }

   const void* DumpGimpleRaw::TREE_VALUE(const void* t) const
   {
      const tree_list *tl = reinterpret_cast<const tree_list*>(t);
      return tl->valu;
   }

   const void* DumpGimpleRaw::TREE_CHAIN(const void* t) const
   {
      const tree_list *tl = reinterpret_cast<const tree_list*>(t);
      if(tl->chan)
      {
         assert(index2tree_list.find(tl->chan) != index2tree_list.end());
         return &index2tree_list.find(tl->chan)->second;
      }
      else
         return nullptr;
   }

   const void* DumpGimpleRaw::DECL_SOURCE_LOCATION(const void*t) const
   {
      if(TREE_CODE(t) == GT(TRANSLATION_UNIT_DECL))
         return nullptr;
      else if(TREE_CODE(t) == GT(FIELD_DECL))
      {
         return nullptr;
      }
      else if(TREE_CODE(t) == GT(PARM_DECL))
      {
         return nullptr;
      }
      else
      {
         const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
         return llvm_obj->hasMetadata() ? llvm_obj->getMetadata(llvm::LLVMContext::MD_dbg) : nullptr;

      }
   }

   DumpGimpleRaw::expanded_location DumpGimpleRaw::expand_location(const void * i) const
   {
      const llvm::MDNode *llvm_obj = reinterpret_cast<const llvm::MDNode*>(i);
      expanded_location res;
      if(dyn_cast<llvm::DIExpression>(llvm_obj)||
            dyn_cast<llvm::DIEnumerator>(llvm_obj) ||
            dyn_cast<llvm::DITemplateParameter>(llvm_obj) ||
            dyn_cast<llvm::DIEnumerator>(llvm_obj) ||
            dyn_cast<llvm::DISubrange>(llvm_obj) ||
            dyn_cast<llvm::GenericDINode>(llvm_obj) ||
            dyn_cast<llvm::MDTuple>(llvm_obj))
      {
      }
      else if(auto * di = dyn_cast<llvm::DIGlobalVariableExpression>(llvm_obj))
      {
         res = expand_location(di->getVariable());
      }
      else if(auto * di = dyn_cast<llvm::DIVariable>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto *di = dyn_cast<llvm::DISubprogram>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto * di = dyn_cast<llvm::DILocation>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.file = res.filename.c_str();
         res.line = di->getLine();
         res.column = di->getColumn();
      }
      else if(auto * di = dyn_cast<llvm::DILexicalBlock>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.file = res.filename.c_str();
         res.line = di->getLine();
         res.column = di->getColumn();
      }
      else if(auto * di = dyn_cast<llvm::DIMacroFile>(llvm_obj))
      {
         res.filename = di->getFile()->getFilename();
         res.file = res.filename.c_str();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto * di = dyn_cast<llvm::DIMacro>(llvm_obj))
      {
         res.line = di->getLine();
      }
      else if(auto * di = dyn_cast<llvm::DIImportedEntity>(llvm_obj))
      {
         //res.filename = di->getFile()->getFilename();
         //res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto * di = dyn_cast<llvm::DIObjCProperty>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto * di = dyn_cast<llvm::DICompileUnit>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
      }
      else if(auto * di = dyn_cast<llvm::DIFile>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
      }
      else if(auto * di = dyn_cast<llvm::DIType>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto * di = dyn_cast<llvm::DINamespace>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
         res.line = di->getLine();
      }
      else if(auto * di = dyn_cast<llvm::DIModule>(llvm_obj))
      {
         res.filename = di->getFilename();
         res.file = res.filename.c_str();
      }
      else if(auto * di = dyn_cast<llvm::DILexicalBlockFile>(llvm_obj))
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

   const void* DumpGimpleRaw::DECL_CONTEXT(const void* t)
   {
      if(TREE_CODE(t) == GT(TRANSLATION_UNIT_DECL)) return  nullptr;
      if(TREE_CODE(t) == GT(FIELD_DECL))
      {
         const field_decl* ty = reinterpret_cast<const field_decl*>(t);
         return ty->scpe;
      }
      if(TREE_CODE(t) == GT(PARM_DECL))
      {
         const llvm::Argument* llvm_obj = reinterpret_cast<const llvm::Argument*>(t);
         return assignCodeAuto(llvm_obj->getParent());
      }
      const llvm::GlobalObject* llvm_obj = reinterpret_cast<const llvm::GlobalObject*>(t);
      const llvm::Module * scpe = llvm_obj->getParent();
      assert(scpe);
      return assignCode(scpe,GT(TRANSLATION_UNIT_DECL));
   }

   int DumpGimpleRaw::DECL_UID(const void *t) const
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
      const llvm::GlobalValue* llvm_obj = reinterpret_cast<const llvm::GlobalValue*>(t);
      return llvm_obj->hasDefaultVisibility() && !llvm_obj->hasInternalLinkage();
   }

   bool DumpGimpleRaw::TREE_STATIC(const void* t) const
   {
      const llvm::GlobalValue* llvm_obj = reinterpret_cast<const llvm::GlobalValue*>(t);
      return llvm_obj->hasInternalLinkage();
   }

   const void* DumpGimpleRaw::DECL_ARG_TYPE(const void* t)
   {
      return TREE_TYPE(t);
   }

   const void* DumpGimpleRaw::DECL_INITIAL(const void* t)
   {
      if(TREE_CODE(t) == GT(FIELD_DECL)) return nullptr;
      const llvm::Value* llvm_obj = reinterpret_cast<const llvm::Value*>(t);
      if(isa<llvm::GlobalVariable>(llvm_obj) && cast<llvm::GlobalVariable>(llvm_obj)->hasInitializer())
      {
         return assignCodeAuto(cast<llvm::GlobalVariable>(llvm_obj)->getInitializer());
      }
      else
         return nullptr;
   }

   const void* DumpGimpleRaw::DECL_SIZE(const void*t)
   {
      return TYPE_SIZE(TREE_TYPE(t));
   }

   int DumpGimpleRaw::DECL_ALIGN(const void* t)
   {
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

   int DumpGimpleRaw::TREE_USED(const void*t) const
   {
      assert(TREE_CODE(t) == GT(VAR_DECL) || TREE_CODE(t) == GT(PARM_DECL));
      if(TREE_CODE(t) == GT(PARM_DECL)) return 1;
      const llvm::Constant* llvm_obj = reinterpret_cast<const llvm::Constant*>(t);
      if(llvm_obj->isConstantUsed())
         return 1;
      else
         return 0;
   }

   bool DumpGimpleRaw::DECL_REGISTER(const void* t) const
   {
      return false;
   }

   bool DumpGimpleRaw::TREE_READONLY(const void* t) const
   {
      if(TREE_CODE(t) == GT(FIELD_DECL)) return false;
      if(TREE_CODE(t) == GT(RESULT_DECL)) return false;
      const llvm::GlobalVariable* llvm_obj = reinterpret_cast<const llvm::GlobalVariable*>(t);
      return llvm_obj->isConstant();

   }
   const void* DumpGimpleRaw::TREE_OPERAND(const void* t, unsigned index)
   {
      llvm_unreachable("Plugin Error");//TBF
   }

   int64_t DumpGimpleRaw::TREE_INT_CST_LOW(const void*t) const
   {
      const llvm::ConstantInt* llvm_obj = reinterpret_cast<const llvm::ConstantInt*>(t);
      const llvm::APInt & val = llvm_obj->getValue();
      assert(val.getNumWords()==1);
      if(val.isNegative())
         return val.getSExtValue();
      else
         return static_cast<int64_t>(val.getZExtValue());
   }


   const void* DumpGimpleRaw::assignCodeType(const llvm::Type*ty)
   {
      assert(CheckSignedTag(ty)==false);
      auto typeId = ty->getTypeID();
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
      }
   }

   const void* DumpGimpleRaw::TREE_TYPE(const void*t)
   {
      tree_codes code = TREE_CODE(t);
      if(code == GT(TRANSLATION_UNIT_DECL)) return nullptr;
      if(code == GT(FIELD_DECL))
      {
         const field_decl* ty = reinterpret_cast<const field_decl*>(t);
         return ty->type;
      }
      tree_codes_class code_class = TREE_CODE_CLASS(code);
      if(code_class==tcc_type)
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
               llvm::errs() << "TREE_TYPE kind not supported: type of type\n";
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

         }
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

   bool DumpGimpleRaw::TYPE_UNSIGNED(const void*t) const
   {
      tree_codes code = TREE_CODE(t);
      if(code==GT(COMPLEX_TYPE))
         llvm_unreachable("unexpected call to TYPE_UNSIGNED");
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      assert(ty->isIntegerTy());
      return true;///TBF
   }

   bool DumpGimpleRaw::COMPLEX_FLOAT_TYPE_P(const void*t) const
   {
      llvm_unreachable("unexpected call to COMPLEX_FLOAT_TYPE_P");
   }

   bool DumpGimpleRaw::TYPE_SATURATING(const void*t) const
   {
      llvm_unreachable("unexpected call to COMPLEX_FLOAT_TYPE_P");
   }

   int DumpGimpleRaw::TYPE_PRECISION(const void*t) const
   {
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      auto typeId = ty->getTypeID();
      switch(typeId)
      {
         case llvm::Type::IntegerTyID:
            return cast<llvm::IntegerType>(ty)->getBitWidth();

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
            llvm::errs() << "TYPE_PRECISION kind not supported\n";
            llvm_unreachable("Plugin Error");


      }
   }

   const void* DumpGimpleRaw::TYPE_MIN_VALUE(const void*t)
   {
      llvm::Type* llvm_obj = const_cast<llvm::Type*>(reinterpret_cast<const llvm::Type*>(t));
      if(uicTable.find(0) == uicTable.end())
         uicTable[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_obj->getContext()), 0, false);
      return assignCodeAuto(uicTable.find(0)->second);

   }

   const void* DumpGimpleRaw::TYPE_MAX_VALUE(const void*t)
   {
      llvm::Type* llvm_obj = const_cast<llvm::Type*>(reinterpret_cast<const llvm::Type*>(t));
      auto obj_size = DL->getTypeSizeInBits(llvm_obj);
      auto maxvalue = llvm::APInt::getMaxValue(obj_size).getZExtValue();
      if(maxValueITtable.find(t) != maxValueITtable.end())
         maxvalue = maxValueITtable.find(t)->second;
      if(uicTable.find(maxvalue) == uicTable.end())
         uicTable[maxvalue] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_obj->getContext()), maxvalue, false);
      return assignCodeAuto(uicTable.find(maxvalue)->second);
   }

   const void* DumpGimpleRaw::TYPE_VALUES(const void* t)
   {
      llvm_unreachable("unexpected call to TYPE_VALUES");
   }

   const void* DumpGimpleRaw::TYPE_NAME(const void* t)
   {
      llvm::Type* llvm_obj = const_cast<llvm::Type*>(reinterpret_cast<const llvm::Type*>(t));
      if(llvm_obj->isStructTy())
      {
         auto st = cast<llvm::StructType>(llvm_obj);
         if(st->hasName())
         {
            std::string declname = st->getName();
            if(identifierTable.find(declname) == identifierTable.end())
               identifierTable.insert(declname);
            const void * dn = identifierTable.find(declname)->c_str();
            return assignCode(dn, GT(IDENTIFIER_NODE));
         }
      }
      return nullptr;/// TBF
   }

   const void* DumpGimpleRaw::TYPE_SIZE(const void* t)
   {
      llvm::Type* llvm_obj = const_cast<llvm::Type*>(reinterpret_cast<const llvm::Type*>(t));
      if(llvm_obj->isFunctionTy())
      {
         auto obj_size = 8u;
         if(uicTable.find(obj_size) == uicTable.end())
            uicTable[obj_size] = llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_obj->getContext()), obj_size, false);
         return assignCodeAuto(uicTable.find(obj_size)->second);
      }
      else if (llvm_obj->isVoidTy())
         return nullptr;
      else
      {
         auto obj_size = DL->getTypeSizeInBits(llvm_obj);
         if(uicTable.find(obj_size) == uicTable.end())
            uicTable[obj_size] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_obj->getContext()), obj_size, false);
         return assignCodeAuto(uicTable.find(obj_size)->second);
      }

   }

   const void* DumpGimpleRaw::TYPE_CONTEXT(const void* t)
   {
      return nullptr;/// TBF
   }

   bool DumpGimpleRaw::TYPE_PACKED(const void*t) const
   {
      llvm::Type* llvm_obj = const_cast<llvm::Type*>(reinterpret_cast<const llvm::Type*>(t));
      if(llvm_obj->isStructTy())
         return cast<llvm::StructType>(llvm_obj)->isPacked();
      else
         return false;
   }

   int DumpGimpleRaw::TYPE_ALIGN(const void*t) const
   {
      llvm::Type* llvm_obj = const_cast<llvm::Type*>(reinterpret_cast<const llvm::Type*>(t));
      return  std::max(8u,8*DL->getABITypeAlignment(llvm_obj));
   }

   const void* DumpGimpleRaw::TYPE_ARG_TYPES(const void* t)
   {
      llvm::Type* ty = const_cast<llvm::Type*>(reinterpret_cast<const llvm::Type*>(t));
      assert(isa<llvm::FunctionType>(*ty));
      const llvm::FunctionType* llvm_obj = reinterpret_cast<const llvm::FunctionType*>(t);
      if(llvm_obj->params().empty())
         return nullptr;
      if(memoization_tree_list.find(t) != memoization_tree_list.end())
         return memoization_tree_list.find(t)->second;
      bool is_first_element = true;
      void * res=nullptr;
      void * cur=nullptr;
      for(const auto& par: llvm_obj->params())
      {
         tree_list &tree_element = index2tree_list[last_used_index+1];
         assignCode(&tree_element, GT(TREE_LIST));
         unsigned int index = queue(&tree_element);
         if(llvm2index.find(par) != llvm2index.end())
            tree_element.valu = par;
         else
         {
            assignCodeType(par);
            tree_element.valu = par;
         }
         if(is_first_element)
         {
            is_first_element = false;
            res = &tree_element;
         }
         else
            reinterpret_cast<tree_list*>(cur)->chan=index;
         cur = &tree_element;
      }
      memoization_tree_list[t] = res;
      return res;
   }

   const void* DumpGimpleRaw::TYPE_DOMAIN(const void*t)
   {
      assert(TREE_CODE(t) == GT(ARRAY_TYPE));
      const llvm::ArrayType* at = reinterpret_cast<const llvm::ArrayType*>(t);
      uint64_t nelements = at->getNumElements();
      ///create the context
      if(ArraysContexts.find(at) == ArraysContexts.end())
         ArraysContexts[at]= new llvm::LLVMContext;
      llvm::Type* IT = llvm::Type::getInt32Ty(*ArraysContexts.find(at)->second);
      assignCodeType(IT);
      maxValueITtable[IT] = nelements-1;
      return IT;
   }

   bool DumpGimpleRaw::stdarg_p(const void* t) const
   {
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      assert(isa<llvm::FunctionType>(*ty));
      const llvm::FunctionType* llvm_obj = reinterpret_cast<const llvm::FunctionType*>(t);
      return llvm_obj->isVarArg();
   }

   llvm::ArrayRef<llvm::Type *> DumpGimpleRaw::TYPE_FIELDS(const void*t)
   {
      const llvm::Type* ty = reinterpret_cast<const llvm::Type*>(t);
      assert(isa<llvm::StructType>(*ty));
      return reinterpret_cast<const llvm::StructType*>(t)->elements();
   }

   const void * DumpGimpleRaw::GET_FIELD_DECL(const llvm::Type*t, unsigned int pos, const void * scpe)
   {
      const llvm::StructType* ty = reinterpret_cast<const llvm::StructType*>(scpe);
      if(index2field_decl.find(std::make_pair(scpe, pos)) == index2field_decl.end())
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "fd%d", pos);
         std::string fdName= buffer;
         if(identifierTable.find(fdName) == identifierTable.end())
            identifierTable.insert(fdName);
         index2field_decl[std::make_pair(scpe, pos)].name = assignCode(identifierTable.find(fdName)->c_str(), GT(IDENTIFIER_NODE));
         index2field_decl[std::make_pair(scpe, pos)].type = t;
         index2field_decl[std::make_pair(scpe, pos)].scpe = scpe;
         index2field_decl[std::make_pair(scpe, pos)].size = TYPE_SIZE(t);
         index2field_decl[std::make_pair(scpe, pos)].algn = TYPE_ALIGN(t);
         uint64_t  offset = DL->getStructLayout(const_cast<llvm::StructType*>(ty))->getElementOffsetInBits(pos);
         if(uicTable.find(offset) == uicTable.end())
            uicTable[offset] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ty->getContext()), offset, false);
         index2field_decl[std::make_pair(scpe, pos)].bpos =  assignCodeAuto(uicTable.find(offset)->second);
      }
      return assignCode(&index2field_decl.find(std::make_pair(scpe, pos))->second, GT(FIELD_DECL));
   }

   const void * DumpGimpleRaw::GET_METHOD_TYPE(const llvm::Type*t, unsigned int pos, const void * scpe)
   {
      llvm_unreachable("unexpected condition");
   }

   const void* DumpGimpleRaw::TYPE_METHOD_BASETYPE(const void* t)
   {
      llvm_unreachable("unexpected condition");
   }

   const void * DumpGimpleRaw::DECL_ARGUMENTS (const void*t)
   {
      const llvm::Function *fd = reinterpret_cast<const llvm::Function *>(t);
      if(fd->getArgumentList().empty())
         return nullptr;
      if(memoization_tree_list.find(t) != memoization_tree_list.end())
         return memoization_tree_list.find(t)->second;
      bool is_first_element = true;
      void * res=nullptr;
      void * cur=nullptr;
      for(const auto& par: fd->getArgumentList())
      {
         const void * parPtr = &par;
         tree_list &tree_element = index2tree_list[last_used_index+1];
         assignCode(&tree_element, GT(TREE_LIST));
         unsigned int index = queue(&tree_element);
         if(llvm2index.find(parPtr) != llvm2index.end())
            tree_element.valu = parPtr;
         else
         {
            assignCodeAuto(parPtr);
            tree_element.valu = parPtr;
         }
         if(is_first_element)
         {
            is_first_element = false;
            res = &tree_element;
         }
         else
            reinterpret_cast<tree_list*>(cur)->chan=index;
         cur = &tree_element;
      }
      memoization_tree_list[t] = res;
      return res;
   }

   const void *DumpGimpleRaw::getStatement_list(const void*t)
   {
      const llvm::Function *fd = reinterpret_cast<const llvm::Function *>(t);
      return assignCode(&fd->getBasicBlockList(), GT(STATEMENT_LIST));
   }

   const std::list<std::pair<const void *, const void*>> DumpGimpleRaw::CONSTRUCTOR_ELTS (const void*t)
   {
      std::list<std::pair<const void *, const void*>> res;
      const llvm::Value* llvm_obj = reinterpret_cast<const llvm::Value*>(t);
      auto vid = llvm_obj->getValueID();
      switch(vid)
      {
         case llvm::Value::ConstantAggregateZeroVal:
            return res;
         case llvm::Value::ConstantStructVal:
         {
            const llvm::ConstantStruct* val = reinterpret_cast<const llvm::ConstantStruct*>(t);
            const void * ty = TREE_TYPE(t);
            for(unsigned index = 0; index < val->getNumOperands(); ++index)
            {
               auto op = val->getOperand(index);
               const void* valu=assignCodeAuto(op);
               const void* idx =  GET_FIELD_DECL(reinterpret_cast<const llvm::Type*>(TREE_TYPE(assignCodeAuto(op))), index, ty);
               res.push_back(std::make_pair(idx,valu));
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
                  uicTable[index] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_obj->getContext()), index, false);
               const void* idx =  assignCodeAuto(uicTable.find(index)->second);
               const void* valu=assignCodeAuto(val->getElementAsConstant(index));
               res.push_back(std::make_pair(idx,valu));
            }
            return res;
         }
         default:
            llvm::errs() << "CONSTRUCTOR_ELTS kind not supported: " << ValueTyNames[vid] << "\n";
            stream.close();
            llvm_unreachable("Plugin Error");

      }
   }

   void DumpGimpleRaw::serialize_new_line()
   {
      snprintf(buffer, LOCAL_BUFFER_LEN, "\n%*s", SOL_COLUMN, "");
      stream << buffer;
      column = SOL_COLUMN;
   }

   void
   DumpGimpleRaw::serialize_maybe_newline()
   {
      int extra;

      /* See if we need a new line. */
      if(column > EOL_COLUMN)
         serialize_new_line();
      /* See if we need any padding.  */
      else if((extra =(column - SOL_COLUMN) % COLUMN_ALIGNMENT) != 0)
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "%*s", COLUMN_ALIGNMENT - extra, "");
         stream << buffer;
         column += COLUMN_ALIGNMENT - extra;
      }
   }

   void
   DumpGimpleRaw::serialize_pointer(const char *field, const void*ptr)
   {
     serialize_maybe_newline();
     snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: %-8lx ", field,(unsigned long) ptr);
     stream << buffer;
     column += 15;
   }

   void DumpGimpleRaw::DumpVersion(llvm::raw_fd_ostream &stream)
   {
      const char * panda_plugin_version =(const char *) PANDA_PLUGIN_VERSION;
      int version = __GNUC__, minor = __GNUC_MINOR__, patchlevel = __GNUC_PATCHLEVEL__;
      stream << "GCC_VERSION: \""<< version << "."<< minor << "." << patchlevel << "\"\n";
      stream << "PLUGIN_VERSION: \""<< panda_plugin_version << "\"\n";
   }

   void DumpGimpleRaw::serialize_int(const char *field, int i)
   {
     serialize_maybe_newline();
     snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: %-7d ", field, i);
     stream << buffer;
     column += 14;
   }

   /* Serialize wide integer i using FIELD to identify it.  */
   void DumpGimpleRaw::serialize_wide_int(const char *field, int64_t i)
   {
      serialize_maybe_newline();
      snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s: " "%" PRId64, field, i);
      stream << buffer;
      column += 21;
   }

   static void real_to_hexadecimal(char * buffer, unsigned size_buff, const llvm::APFloat&val)
   {
      llvm::APInt API = val.bitcastToAPInt();
      llvm::errs() << API << "\n";
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

      unsigned ExpBiased = API.lshr(nbitsMan).getZExtValue() & ((1U<<nbitsExp)-1);
      int ExpUnbiased = (llvm::APInt::getNullValue(API.getBitWidth()) == API) ? 0 : (ExpBiased - ((1U << (nbitsExp-1))-2));

      snprintf(buffer, size_buff, "p%+d", ExpUnbiased);

      llvm::APInt Mantissa = API & llvm::APInt::getAllOnesValue(nbitsMan).zext(API.getBitWidth());
      if(ExpBiased != 0)
         Mantissa.setBit(nbitsMan);

      size_t digits = size_buff - strlen(buffer) - (val.isNegative()?1:0) - 4 - 1;
      assert(digits <= size_buff);
      char * current = buffer;
      if (val.isNegative())
         *current++ = '-';
      *current++ = '0';
      *current++ = 'x';
      *current++ = '0';
      *current++ = '.';
      for (int index1 = (nbitsMan/4)-!(nbitsMan%4); index1 >= 0 && digits >0; --index1)
      {
          *current++ = "0123456789abcdef"[(Mantissa.lshr(index1*4).getLoBits(4).getZExtValue())];
         --digits;
      }
      sprintf (current, "p%+d", ExpUnbiased);
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
      assert(reinterpret_cast<const llvm::ConstantFP*>(t)->getValueID()==llvm::Value::ConstantFPVal);
      const llvm::APFloat& d = reinterpret_cast<const llvm::ConstantFP*>(t)->getValueAPF();
      if(d.isInfinity())
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "valr: %-7s ", "\"Inf\"");
         stream << buffer;
      }
      else if(d.isNaN())
      {
         snprintf(buffer, LOCAL_BUFFER_LEN, "valr: %-7s ", "\"Nan\"");
         stream << buffer;
      }
      else
      {
         bool isDouble = &d.getSemantics() == &llvm::APFloat::IEEEdouble();
         snprintf(buffer, LOCAL_BUFFER_LEN, "%f", (isDouble ? d.convertToDouble():d.convertToFloat()));
         stream << "valr: \""<< std::string(buffer) << "\" ";
      }
      {
         stream << "valx: \"";
         real_to_hexadecimal(buffer,LOCAL_BUFFER_LEN,d);
         stream << std::string(buffer);
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
      for(i = 0; i < length; i++)
      {
         switch(input[i])
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
   void DumpGimpleRaw::serialize_string(const char *string)
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
   void DumpGimpleRaw::serialize_string_field(const char *field, const char *str)
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

   void DumpGimpleRaw::serialize_string_cst(const char *field, const char *str, int length, unsigned int precision)
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
         const unsigned int * string =(const unsigned int *) str;
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


   void DumpGimpleRaw::queue_and_serialize_index(const char *field, const void* t)
   {
      unsigned int index;
      if(t==nullptr)
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

   static void computeLoopLabels (std::map<const llvm::Loop*, unsigned> &loopLabes, llvm::Loop* curLoop, unsigned int & label)
   {
      loopLabes[curLoop] = label;
      label++;
      for(auto it = curLoop->begin(); it != curLoop->end(); ++it)
      {
         computeLoopLabels (loopLabes, *it, label);
      }
   }

   void DumpGimpleRaw::dequeue_and_serialize_gimple(const void* t)
   {
      assert(llvm2index.find(t) != llvm2index.end());
      unsigned int index = llvm2index.find(t)->second;

     const char* code_name = GET_TREE_CODE_NAME(TREE_CODE(t));

     /* Print the node index.  */
     serialize_index (index);

     snprintf(buffer, LOCAL_BUFFER_LEN, "%-16s ", code_name);
     stream << buffer;
     column = 25;
     /* Terminate the line.  */
     stream << "\n";
   }

   void DumpGimpleRaw::dequeue_and_serialize_statement (const void* t)
   {
      assert(llvm2index.find(t) != llvm2index.end());
      unsigned int index = llvm2index.find(t)->second;

     const char* code_name = GET_TREE_CODE_NAME(TREE_CODE(t));

     /* Print the node index.  */
     serialize_index (index);

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
     const llvm::Function::BasicBlockListType & bblist = *reinterpret_cast<const llvm::Function::BasicBlockListType*>(t);
     llvm::Function *currentFunction = const_cast<llvm::Function *>(bblist.front().getParent());
     assert(modulePass);
     llvm::LoopInfo &LI = modulePass->getAnalysis<llvm::LoopInfoWrapperPass>(*currentFunction).getLoopInfo();
     std::map<const llvm::Loop*, unsigned> loopLabes;
     if(!LI.empty())
     {
        unsigned int label = 1;
        for(auto it=LI.begin(); it != LI.end(); ++it)
           computeLoopLabels(loopLabes, *it, label);
     }

     llvm::ModuleSlotTracker MST(currentFunction->getParent());
     MST.incorporateFunction(*currentFunction);
     for(const auto& BB: bblist)
     {
        const char *field;
        serialize_new_line ();
        serialize_int ("bloc", MST.getLocalSlot(&BB));
        if(LI.empty() || !LI.getLoopFor(&BB))
           serialize_int ("loop_id", 0 );
        else
           serialize_int ("loop_id", loopLabes.find(LI.getLoopFor(&BB))->second);
        if(!LI.empty() && LI.getLoopFor(&BB) && LI.getLoopFor(&BB)->getHeader() == &BB && LI.getLoopFor(&BB)->isAnnotatedParallel())
           serialize_string("hpl");
        if(llvm::pred_begin(&BB) == llvm::pred_end(&BB))
        {
           serialize_maybe_newline ();
           field = "pred: ENTRY";
           snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s ", field);
           stream << buffer;
           column += 14;
        }
        else
        {
           for(const auto pred: llvm::predecessors(&BB))
              serialize_int ("pred", MST.getLocalSlot(pred));
        }
        if(llvm::succ_begin(&BB) == llvm::succ_end(&BB))
        {
           serialize_maybe_newline ();
           field = "succ: EXIT";
           snprintf(buffer, LOCAL_BUFFER_LEN, "%-4s ", field);
           stream << buffer;
           column += 14;
        }
        else
        {
           for(const auto succ: llvm::successors(&BB))
              serialize_int ("succ", MST.getLocalSlot(succ));
        }
        if(isa<llvm::BranchInst>(BB.getTerminator()))
        {
           const llvm::BranchInst* bi = cast<llvm::BranchInst>(BB.getTerminator());
           if(bi->getNumOperands()==3)
           {
              serialize_int ("true_edge", MST.getLocalSlot(bi->getSuccessor(0)));
              serialize_int ("false_edge", MST.getLocalSlot(bi->getSuccessor(1)));
           }
        }
        for(const auto& inst: BB.getInstList())
        {
           if(isa<llvm::PHINode>(inst))
              serialize_gimple_child("phi", assignCodeAuto(&inst));
           else
              serialize_gimple_child("stmt", assignCodeAuto(&inst));
        }

     }
     /* Terminate the line.  */
     stream << "\n";
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

      const char * code_name = GET_TREE_CODE_NAME(TREE_CODE(t));
      llvm::errs() << "|" << code_name  << "\n";

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

         //if(DECL_ABSTRACT_ORIGIN(t))
         //  serialize_child("orig", DECL_ABSTRACT_ORIGIN(t));
         /* And types.  */
         queue_and_serialize_type(t);
         serialize_child("scpe", DECL_CONTEXT(t));

         if(!DECL_SOURCE_LOCATION(t))
         {
           serialize_maybe_newline();
           snprintf(buffer, LOCAL_BUFFER_LEN, "srcp: \"<built-in>\":0:0 ");
           stream << buffer;
           column += 12 + strlen("<built-in>") + 8;
         }
         else
         {
            /* And a source position.  */
            xloc = expand_location(DECL_SOURCE_LOCATION(t));
            if(xloc.file)
            {
               serialize_maybe_newline();
               snprintf(buffer, LOCAL_BUFFER_LEN, "srcp: \"%s\":%-d:%-6d ", xloc.file,
                     xloc.line, xloc.column);
               stream << buffer;
               column += 12 + strlen(xloc.file) + 8;
            }
         }

//         if(llvm_obj->isImplicit())
//            serialize_string("artificial");
         serialize_int("uid", DECL_UID(t));
      }
      else if(code_class == tcc_type)
      {
         /* All types have associated declarations.  */
         serialize_child("name", TYPE_NAME(t));
         /* And sizes.  */
         serialize_child("size", TYPE_SIZE(t));

         /* All types have context.  */ /*NB TBC*/
         serialize_child("scpe", TYPE_CONTEXT(t));/*NB TBC*/

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
           serialize_string_field("sign", TYPE_UNSIGNED(t) ? "unsigned": "signed");
           serialize_string_field("saturating",
                  TYPE_SATURATING(t) ? "saturating": "non-saturating");
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
           serialize_child("retn", TREE_TYPE(t));
           serialize_child("prms", TYPE_ARG_TYPES(t));
           if(stdarg_p(t))
             serialize_string("varargs");
           break;

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

         case GT(VAR_DECL):
         case GT(PARM_DECL):
         case GT(FIELD_DECL):
         case GT(RESULT_DECL):
            if(TREE_CODE(t) == GT(FIELD_DECL) && DECL_C_BIT_FIELD(t))
               serialize_string("bitfield");
            if(TREE_CODE(t) == GT(VAR_DECL))
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
            else if(TREE_CODE(t) == GT(VAR_DECL) || TREE_CODE(t) == GT(PARM_DECL))
            {
              serialize_int("used", TREE_USED(t));
              if(DECL_REGISTER(t))
                serialize_string("register");
            }
            if(TREE_READONLY(t) && TREE_CODE(t) != GT(RESULT_DECL) && TREE_CODE(t) != GT(FIELD_DECL))
               serialize_string("readonly");
            break;
         case GT(FUNCTION_DECL):
         {
            const void * arg = DECL_ARGUMENTS(t);
            while(arg)
            {
              serialize_child("arg", TREE_VALUE(arg));
              arg = TREE_CHAIN(arg);
            }
            if(DECL_EXTERNAL(t))
              serialize_string("undefined");
//            if(is_builtin_fn(t))
//              serialize_string("builtin");
            if(!TREE_PUBLIC(t))
              serialize_string("static");
            if (!DECL_EXTERNAL(t))
              serialize_statement_child("body", getStatement_list(t));

            break;
         }
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
           serialize_real (t);
           break;

//         case GT(COMPLEX_CST):
//           serialize_child ("real", TREE_REALPART (t));
//           serialize_child ("imag", TREE_IMAGPART (t));
//           break;

//         case GT(FIXED_CST):
//           serialize_fixed ("valu", TREE_FIXED_CST_PTR (t));
//           break;

         case GT(CONSTRUCTOR):
         {
            queue_and_serialize_type (t);
            for(const auto& el : CONSTRUCTOR_ELTS (t))
            {
               serialize_child ("idx", el.first);
               serialize_child ("valu", el.second);
            }
            break;
         }
         default:
            /* There are no additional fields to print.  */
            break;
      }
      stream << "\n";

   }

   void DumpGimpleRaw::SerializeGimpleFunctionHeader(const void*obj)
   {
      assert(TREE_CODE(obj) == GT(FUNCTION_DECL));
      const llvm::Function *fd = reinterpret_cast<const llvm::Function *>(obj);
      stream << "\n;; Function " << fd->getName() << "(" << fd->getName() << ")\n\n";
      stream << ";; " << fd->getName() << "(";
      stream << ")\n";
   }

   unsigned int DumpGimpleRaw::queue(const void* obj)
   {
      unsigned int index = ++last_used_index;
      assert(llvm2index.find(obj) == llvm2index.end());
      llvm2index[obj] = index;
      assert(llvm2tree_code.find(obj) != llvm2tree_code.end());
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

   bool DumpGimpleRaw::runOnModule(llvm::Module &M, llvm::ModulePass *_modulePass)
   {
      DL = &M.getDataLayout();
      modulePass=_modulePass;
      for(const auto& globalVar : M.getGlobalList())
      {
         llvm::errs() << "Found global name: " << globalVar.getName() << "|" << ValueTyNames[globalVar.getValueID()] << "\n";
         SerializeGimpleGlobalTreeNode(assignCodeAuto(&globalVar));
      }
      if(!onlyGlobals)
      {
         for(const auto& fun : M.getFunctionList())
         {
            if(fun.isIntrinsic())
               llvm::errs() << "Function intrinsic skipped: " << fun.getName() << "|" << ValueTyNames[fun.getValueID()] << "\n";
            else
            {
               llvm::errs() << "Found function: " << fun.getName() << "|" << ValueTyNames[fun.getValueID()] << "\n";
               SerializeGimpleGlobalTreeNode(assignCodeAuto(&fun));
            }
         }
      }
      return false;
   }

}
