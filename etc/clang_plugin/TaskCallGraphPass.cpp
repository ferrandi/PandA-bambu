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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
#include "panda_clang_compat.hpp"

/**
 * @file TaskCallGraphPass.cpp
 *
 * @author Kaixi Matteo Chen <kaiximatteo.chen@mail.polimi.it>
 *
 */

#if PANDA_LLVM_CLANG_MAJOR >= 13

#include "DotGenerator.hpp"
#include "OmpTaskGraphConstants.hpp"
#include "OpenMPTaskPassConfig.hpp"
#include "TopoGraphGenerator.hpp"
#include "Types.hpp"
#include "Utils.hpp"

#include "pugixml.hpp"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace llvm;
using namespace Bambu::OpenMP::Task;

//-----------------------------------------------------------------------------
// TaskCallGraphPass implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace
{
#if PANDA_LLVM_CLANG_MAJOR >= 19

   cl::opt<std::string> outDirName("outputdir", cl::desc("Destination path for the output XML file"),
                                   cl::value_desc("directory path"), cl::init(""));

   cl::opt<unsigned> resourceDuplicationNum("resource-duplication", cl::desc("The number of duplicated resources"),
                                            cl::value_desc("number of resources duplication"), cl::init(1));

   Function* requireDirectCallee(const CallInst* call)
   {
      auto* callee = call->getCalledFunction();
      if(!callee)
      {
         report_fatal_error("OpenMP task extraction does not support indirect calls in the task graph");
      }
      return callee;
   }

   // Forward declaration
   void visitor(Function& F, std::set<unsigned> const* entryPointInputs, unsigned& taskHash, TopoGraphGenerator& topo,
                bool clearDeps = true);

   struct InputSetCmp
   {
      bool operator()(std::pair<unsigned, unsigned> const& a, std::pair<unsigned, unsigned> const& b) const
      {
         return a.second != b.second ? a.second > b.second : a.first < b.first;
      }
   };

   // key: unique task id
   // std::get<0>(value): in_deps
   // std::get<1>(value): out_deps
   // std::get<2>(value): inout_deps
   std::map<std::string, std::tuple<std::set<unsigned>, std::set<unsigned>, std::set<unsigned>>> dependentTasks;

   // Track the order in which tasks are created
   std::vector<std::string> taskCreationOrderList;

   // A map from the function name to its Function reference
   std::unordered_map<std::string, Function*> fnCache;

   // A map from a task entry ID to its input list
   std::unordered_map<std::string, std::set<std::pair<unsigned, unsigned>, InputSetCmp>> taskEntryInputs;

   // Currently not used
   std::unordered_map<unsigned, std::set<std::pair<unsigned, unsigned>, InputSetCmp>> taskEntryParentInputs;

   // A map from a task entry ID to its constant input list
   std::unordered_map<std::string, std::vector<std::pair<unsigned, unsigned>>> taskEntryConstInputs;

   // A map from task alloc instructions to its input stores and lambda entry
   // point. It is unecessary to map the function name as we are using pointers
   std::unordered_map<Instruction*, std::pair<Value*, std::vector<std::pair<Instruction*, unsigned>>>>
       taskEntryInputsRef;

   // A set containing free tasks with zero dependencies
   std::unordered_map<std::string, unsigned> freeTasks;

   // A map tracking for each `ptrtoint` instruction source operand, its parent
   // source and any offset if they are array accesses. The llvm reference is being
   // cached too for code instrumentation
   std::map<std::string, std::map<unsigned, std::tuple<bool, int, unsigned long, const Value*>>> ptrToIntSourcesParents;

   /// A set to track never referenced base inputs inside ptrToIntSourcesParents
   std::set<unsigned> neverReferencedBaseInputs;

   // A map to track any proxy label for each input variable
   std::unordered_map<std::string, std::unordered_map<unsigned, std::vector<unsigned>>> fnInputProxies;

   // A map to track any proxy label for each input variable
   std::unordered_map<std::string, std::unordered_map<unsigned, std::vector<Instruction*>>> fnInputProxiesRefs;

   // A map from lambda entry point references to its unique name generated from
   // the pass based on its deps. We use the Instruction where the lambda is being
   // allocated in the task as many task alloc may use the same entry point fn
   // hence overriding this map
   std::unordered_map<Instruction*, std::pair<std::string, unsigned>> lambdaEntryPointUniqueNameDict;

   std::unordered_map<std::string, std::unordered_map<std::string, std::pair<unsigned, std::unordered_set<unsigned>>>>
       sameGEPs;

   std::unordered_map<std::string, std::unordered_map<unsigned, unsigned>> sameGEPsReverse;

   // Taken from
   // https://vscode.dev/github/llvm/llvm-project/blob/main/openmp/runtime/src/kmp.h#L2505
   const std::unordered_set<uint8_t> outDepsFlags = {2};

   // Taken from
   // https://vscode.dev/github/llvm/llvm-project/blob/main/openmp/runtime/src/kmp.h#L2505
   const std::unordered_set<uint8_t> inOutDepsFlags = {3};

   // Taken from
   // https://vscode.dev/github/llvm/llvm-project/blob/main/openmp/runtime/src/kmp.h#L2505
   const std::unordered_set<uint8_t> inDepsFlags = {1};

   /// Cache for each lambda its intrumented call argument list
   std::unordered_map<Value*, std::vector<Type*>> taskEntryPointArgTypesList;

   /// Map a duplicated task entry to the original one
   std::unordered_map<Function*, Function*> duplicatedFnToOriginal;

   /// Duplicated resources (task entries)
   std::unordered_map<Function*, std::vector<Function*>> duplicatedResourcesDict;

   /// Detect if a label str is a ptr reference or not
   /// @param str A label string
   /// @return The query stating if the input label is a variable proxy
   bool isPtr(const std::string& str)
   {
      return str.find('%') != std::string::npos;
   }

   /// Retrieve the first available numerical label (IR proxy) after the given char
   /// @param str The input string referring to a @ref llvm::Instruction
   /// @param c The starting point char inside the input string
   std::string retrieveLabelFromChar(std::string const& str, const char c)
   {
      const auto start = str.find(c);
      if(start == std::string::npos)
      {
         return "";
      }
      const auto end = str.find_first_not_of("0123456789", start + 1);
      return str.substr(start + 1, end == std::string::npos ? end : end - start - 1);
   }

   /// Retrieve the proxy label which is unnamed from an instruction string. This
   /// will retrieve effectively the first proxy being encountered inside the
   /// instruction string
   /// @param str A string representing a @ref llvm::Instruction
   std::string parseUnamedNameFromInstruction(std::string const& str)
   {
      const auto equalPos = str.find('=');
      if(equalPos == std::string::npos)
      {
         return "Not Proxy";
      }
      return retrieveLabelFromChar(str, '%');
   };

   std::string subStrFromChar(std::string const& str, const char c, const unsigned offset)
   {
      const auto pos = str.find(c);
      assert(pos != std::string::npos);

      return str.substr(pos + 2);
   }

   /// Retrieve the proxy label of a `ptrtoint` source
   /// @note This function does not check that the provided instruction is
   /// effectively a `ptrtoint` instruction
   /// @param str A string representing a @ref llvm::Instruction
   std::string parsePtrToIntSource(std::string const& str)
   {
      unsigned i = 0;
      while(i < str.size() && str[i] != '%')
      {
         i++;
      }
      return retrieveLabelFromChar(str.substr(i + 1), '%');
   };

   /// Remove any whitespaces that prefix a string
   /// @param str The input string
   /// @return The modified string
   std::string removeEmptyPrefix(std::string const& str)
   {
      const auto start = str.find_first_not_of(' ');
      return start == std::string::npos ? "" : str.substr(start);
   };

   /// Create an unique task id identified for task entry lambdas that are shared
   /// between different contexts
   /// @param id The original task entry id
   /// @param unique The unique identifier for the original id
   /// @return The unique task entry identifier
   std::string uniqueTaskEntryIdentifier(std::string const& id, const unsigned unique)
   {
      return id + "" + std::to_string(unique);
   }

   /// Recover the original task entry id from an unique generated id. See @ref
   /// uniqueTaskEntryIdentifier
   /// @param uniqueId The unique task entry id
   /// @return The original task id without the unique identifier
   unsigned recoverLambdaIdFromUniqueId(std::string const& uniqueId)
   {
      unsigned s = 0;
      while(s < uniqueId.size() && (uniqueId[s] < '0' || uniqueId[s] > '9'))
      {
         s++;
      }
      const auto pos = uniqueId.find('#');
      return static_cast<unsigned>(std::stoi(uniqueId.substr(s, pos)));
   }

   LoadInst const* loadFromGep(GetElementPtrInst const* gep)
   {
      assert(gep != nullptr);
      auto* op = gep->getPointerOperand();
      if(isa<LoadInst>(op))
      {
         return dyn_cast<LoadInst>(op);
      }
      if(!isa<GetElementPtrInst>(op))
      {
         report_fatal_error("not impl");
      }
      return loadFromGep(dyn_cast<GetElementPtrInst>(op));
   }

   Value const* argFromLoad(LoadInst const* load)
   {
      assert(load != nullptr);
      auto* op = load->getPointerOperand();
      if(isa<Argument>(op))
      {
         return op;
      }
      if(!isa<LoadInst>(op))
      {
         report_fatal_error("not impl");
      }
      return argFromLoad(dyn_cast<LoadInst>(op));
   }

   /// Check if the instruction is part of a omp task call
   /// @param inst A @ref llvm::Instruction
   /// @param flag To control open or closing omp task calls (task alloc and task
   /// submission)
   /// @return The query result
   bool isTarget(Instruction const& inst, const uint8_t flag)
   {
      StringRef instruction_name = inst.getName();
      StringRef opcode_name = inst.getOpcodeName();
      if(auto* CI = dyn_cast<CallInst>(&inst))
      {
         StringRef name = requireDirectCallee(CI)->getName();
         if(flag == Constants::OPEN_FLAG && name.data() == Constants::targetOmpTaskAllocName)
         {
            return true;
         }
         else if(flag == Constants::CLOSE_FLAG_WITH_DEPS && name.data() == Constants::targetOmpTaskSubmissionDepsName)
         {
            return true;
         }
         else if(flag == Constants::CLOSE_FLAG && name.data() == Constants::targetOmpTaskSubmissionName)
         {
            return true;
         }
      }
      return false;
   }

   /// Retrieve the instructions wrapping a task creation and submission
   /// A single basic block may contain multiple task creation instructions
   /// @param block A basic block
   /// @param includeAllBeforeTaskAllocation A flag to control if to include all
   /// instructions or not
   /// @return A list of possible instructions containing task creation
   void taskWithDepsInstructions(BasicBlock& block, bool includeAllBeforeTaskAllocation,
                                 std::vector<std::vector<Instruction*>>& collector)
   {
      bool inside = 0;
      std::vector<Instruction*> instructions;
      for(Instruction& inst : block)
      {
         if((!inside && isTarget(inst, Constants::OPEN_FLAG)))
         {
            inside = 1;
            instructions.push_back(&inst);
         }
         else if(inside && (isTarget(inst, Constants::CLOSE_FLAG_WITH_DEPS) || (isTarget(inst, Constants::CLOSE_FLAG))))
         {
            inside = 0;
            instructions.push_back(&inst);
            collector.push_back(std::vector(instructions));
            instructions.clear();
         }
         else if(inside)
         {
            instructions.push_back(&inst);
         }
         else if(!inside && includeAllBeforeTaskAllocation)
         {
            instructions.push_back(&inst);
         }
      }
      if(inside)
      {
         report_fatal_error("OpenMP task allocation and submission must be in the same basic block");
      }
   }

   /// Function to check if the GEP is accessing a specific struct
   /// @param GEP A pointer to a @ref llvm::GetElementPtrInst
   /// @param targetStructName The target struct name
   /// @return A flag stating if the GEP accesses an instance of the target struct
   template <int indent = 2>
   bool GetElementPtrInstIsSpecificStruct(const GetElementPtrInst* GEP, const std::string& targetStructName)
   {
      // Get the source type of the GEP (the type that is being indexed)
      Type* sourceType = GEP->getSourceElementType();

      if(sourceType->isStructTy())
      {
         StructType* structType = cast<StructType>(sourceType);
         const auto structName = structType->getName().str();
         if(structName.find(targetStructName) != std::string::npos)
         {
            Utils::log(errs(), indent, 1, "This GEP is accessing the struct:", structName);
            return true;
         }
         else
         {
            Utils::log(errs(), indent, 1,
                       "This GEP is accessing an array of the struct different "
                       "from the target struct: ",
                       structType->getName());
            return false;
         }
      }
      return false;
   }

   /// Function to check if the @ref llvm::AllocaInst is accessing an array of a
   /// specific struct
   /// @param I A pointer to a @ref llvm::AllocaInst
   /// @param structName The target struct name
   /// @return A flag stating if the allocation accesses an array of the target
   /// struct
   bool AllocInstIsArrayOfSpecificStruct(const AllocaInst* I, const std::string& structName)
   {
      Type* allocType = I->getAllocatedType();
      if(auto* arrayType = dyn_cast<ArrayType>(allocType))
      {
         Type* elementType = arrayType->getElementType();
         if(auto* structType = dyn_cast<StructType>(elementType))
         {
            if(structType->getName().str() == structName)
            {
               return true;
            }
         }
      }
      return false;
   }

   /// Function to check if the @ref llvm::GetElementPtrInst is accessing an array
   /// of a specific struct
   /// @param GEP A pointer to a @ref llvm::GetElementPtrInst
   /// @param structName The target struct name
   /// @return A flag stating if the GEP accesses an array of the target struct
   bool GetElementPtrInstIsArrayOfSpecificStruct(const GetElementPtrInst* GEP, const std::string& structName)
   {
      // Get the source type of the GEP (the type that is being indexed)
      Type* sourceType = GEP->getSourceElementType();
      std::string stringBuff;
      raw_string_ostream stream(stringBuff);
      sourceType->print(stream);

      // Check if the source type is an array
      if(sourceType->isArrayTy())
      {
         Utils::log(errs(), 2, 1, "The type being indexed is an array");
         // Get the element type of the array
         ArrayType* arrayType = cast<ArrayType>(sourceType);
         Type* elementType = arrayType->getElementType();

         // Check if the element type is a struct
         if(elementType->isStructTy())
         {
            StructType* structType = cast<StructType>(elementType);

            // Compare the struct name with the one we are looking for
            if(structType->getName().data() == structName)
            {
               Utils::log(errs(), 2, 1, "This GEP is accessing an array of the struct:", structName);
               return true;
            }
            else
            {
               Utils::log(errs(), 2, 1,
                          "This GEP is accessing an array of the struct different "
                          "from the target struct: ",
                          structType->getName());
            }
         }
      }
      else
      {
         Utils::log(errs(), 2, 1, "The type being indexed is not an array:", stringBuff);
      }
      return false;
   }

   /// Find the GEP reference usage and print it
   /// @param GEP A pointer to a @ref GetElementPtrInst instance
   void GEPUses(const GetElementPtrInst* GEP)
   {
      std::string stringBuff;
      raw_string_ostream stream(stringBuff);
      GEP->print(stream);
      Utils::log(errs(), 2, 1, "Analyzing uses of GEP instruction: ", stringBuff);

      // Iterate through all uses of the GEP instruction
      for(const Use& U : GEP->uses())
      {
         const User* user = U.getUser(); // Get the user (instruction that uses the GEP result)

         // Check if the GEP result is used in a store instruction
         if(const StoreInst* storeInst = dyn_cast<StoreInst>(user))
         {
            Utils::log(errs(), 2, 1, "GEP result is being stored in memory");
         }
         // Check if the GEP result is used in a load instruction
         else if(const LoadInst* loadInst = dyn_cast<LoadInst>(user))
         {
            Utils::log(errs(), 2, 1, "GEP result is being loaded from memory");
         }
         // Check if the GEP result is passed as an argument to a function call
         else if(const CallInst* callInst = dyn_cast<CallInst>(user))
         {
            Utils::log(errs(), 2, 1, "GEP result is being used in a function call");
         }
         // Handle other types of instructions
         else
         {
            Utils::log(errs(), 2, 1, "Not recognized use");
         }
      }
   }

   /// Define the store instruction data type
   /// Used in @ref collectUsesAsStoreInstOf
   enum StoreType
   {
      PointerStore,
      ValueStore
   };

   /// Retrieve for a `ptrtoint` source proxy the parent proxy. If the parent proxy
   /// comes from a GEP instruction this might tied to an array access with index
   /// greater than 0
   /// @param instructions A list of instructions of the visited function
   /// @param source The `ptrtoint` source proxy
   /// @param index The starting index to start the research from inside the
   /// instruction list
   /// @return The a tuple containing: <isConstantOffset, parentProxy,
   /// offsetSource>. The last twos are NA_VALUE if not available
   /// TODO: speed up this
   std::tuple<bool, int, unsigned long> findPtrToIntSourceParent(std::vector<Instruction*> const& instructions,
                                                                 const std::string& source, const unsigned index)
   {
      assert(index > 0);
      assert(index < instructions.size());
      std::string stringBuff;
      raw_string_ostream stream(stringBuff);
      auto it = instructions.begin();
      std::advance(it, index);
      while(it >= instructions.begin())
      {
         (*it)->print(stream);
         const auto label = parseUnamedNameFromInstruction(stringBuff);
         stringBuff.clear();
         if(label == source)
         {
            // Don't use continue as we have to check the iterator position after this
            // block
            if(isa<GetElementPtrInst>(*it))
            {
               GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(*it);
               // Other cases are not handled yet
               if(isa<Constant>(GEP->getOperand(GEP->getNumOperands() - 1)))
               {
                  auto* offsetOperand = dyn_cast<ConstantInt>(GEP->getOperand(GEP->getNumOperands() - 1));
                  // We want to track index accesses greater than 0 as they represent
                  // index access in an input array. For base index accesses we have not
                  // seen GEP calls so far
                  if(offsetOperand->getValue() != 0)
                  {
                     auto* sourceOperand = dyn_cast<Value>(GEP->getPointerOperand());
                     assert(sourceOperand);
                     sourceOperand->print(stream);
                     const auto ptr = retrieveLabelFromChar(stringBuff, '%');
                     stringBuff.clear();
                     const auto offset = retrieveLabelFromChar(stringBuff, '%');
                     const std::tuple<bool, int, unsigned long> tup =
                         std::make_tuple(true, std::stoi(ptr), offsetOperand->getValue().getZExtValue());
                     return tup;
                  }
               }
               else
               {
                  // For now non const values are treated as accesses with index gt 0
                  auto* sourceOperand = dyn_cast<Value>(GEP->getPointerOperand());
                  auto* offsetOperand = dyn_cast<Value>(GEP->getOperand(GEP->getNumOperands() - 1));
                  assert(sourceOperand);
                  assert(offsetOperand);
                  sourceOperand->print(stream);
                  const auto ptr = retrieveLabelFromChar(stringBuff, '%');
                  stringBuff.clear();
                  offsetOperand->print(stream);
                  const auto offset = retrieveLabelFromChar(stringBuff, '%');
                  const std::tuple<bool, int, unsigned long> tup =
                      std::make_tuple(false, std::stoi(ptr), std::stoi(offset));
                  return tup;
               }
            }
         }
         if(it == instructions.begin())
         {
            break;
         }
         --it;
      }
      return std::make_tuple(false, NA_VALUE, NA_VALUE);
   }

   /// Precompute target `ptrtoint` source ptrs' parent proxies:
   /// For example:
   ///  %65 = load float**, float*** %3, align 8, !tbaa !12
   ///  %66 = getelementptr inbounds float*, float** %65, i64 %53
   ///  %67 = ptrtoint float** %66 to i64
   /// @param instructions An instructions list
   /// @param fnName The function being visited name
   void precomputePtrToIntSourcesParents(std::vector<Instruction*> const& instructions, const std::string& fnName)
   {
      std::string stringBuff;
      raw_string_ostream stream(stringBuff);
      unsigned index = 0;
      for(auto* I : instructions)
      {
         stringBuff.clear();
         if(!isa<PtrToIntInst>(I))
         {
            index++;
            continue;
         }
         const auto* ptrtoint = dyn_cast<PtrToIntInst>(I);
         auto* source = ptrtoint->getPointerOperand();
         source->print(stream);
         const auto sourceLabel = retrieveLabelFromChar(stringBuff, '%');
         const auto sourceParent = findPtrToIntSourceParent(instructions, sourceLabel, index - 1);
         const auto key = std::stoi(sourceLabel);
         if(ptrToIntSourcesParents[fnName].find(key) == ptrToIntSourcesParents[fnName].end())
         {
            ptrToIntSourcesParents[fnName][key] = std::tuple_cat(sourceParent, std::make_tuple(source));
         }

         // If the source of the previous insertion was not a load instruction we need to find it and add it as
         // the load instruction represent the top owner of the current ptrtoint
         if(isa<GetElementPtrInst>(source))
         {
            auto* sourceLoad = loadFromGep(dyn_cast<GetElementPtrInst>(source));
            stringBuff.clear();
            sourceLoad->print(stream);

            const auto sourceLabel = retrieveLabelFromChar(stringBuff, '%');
            const auto sourceParent = findPtrToIntSourceParent(instructions, sourceLabel, index - 1);
            const auto key = std::stoi(sourceLabel);
            if(ptrToIntSourcesParents[fnName].find(key) == ptrToIntSourcesParents[fnName].end())
            {
               ptrToIntSourcesParents[fnName][key] = std::tuple_cat(sourceParent, std::make_tuple(sourceLoad));
            }
         }
         else if(!isa<LoadInst>(source) && !isa<Argument>(source))
         {
            stringBuff.clear();
            source->print(stream);
            report_fatal_error(std::string("not impl. Instruction is " + stringBuff).c_str());
         }
         index++;
      }
   }

   /// Recover the source pointer casted to int in a `ptrtoint` call.
   /// This function does a linear scan over the instructions and finds the
   /// associated `ptrtoint` call that matches the provided label
   /// @param instructions The instructions list
   /// @param start_it The starting iterator for the instructions list
   /// @param label The result proxy where the `ptrtoint` operation is being saved
   /// @param fnName The current function being visited
   /// @return The source pointer inside the `ptrtoint` call label
   std::string recoverPtrToIntSource(std::vector<Instruction*> const& instructions,
                                     std::vector<Instruction*>::const_reverse_iterator it, const std::string& label,
                                     const std::string& fnName)
   {
      std::string sourceLabel = "";
      std::string stringBuff;
      raw_string_ostream stream(stringBuff);

      while(it != instructions.rend())
      {
         (*it)->print(stream);
         const auto currLabel = parseUnamedNameFromInstruction(stringBuff);

         if(currLabel == label)
         {
            sourceLabel = parsePtrToIntSource(stringBuff);
            break;
         }
         stringBuff.clear();

         ++it;
      }
      assert(sourceLabel.size() > 0);
      return sourceLabel;
   }

   /// Collect @ref llvm::StoreInst instructions as uses of a given @ref llvm::GetElementPtrInst
   /// @tparam T The type of storage we are looking for. @ref
   /// StoreType::PointerStore for dependencies sources and @ref
   /// StoreType::ValueStore for dependencies types
   /// @param GEP A pointer to a get element pointer instance
   /// @return A vector containing a tuple of the extracted stored proxy label and
   /// the store instruction as string and pointer
   template <StoreType T>
   std::vector<std::tuple<int, std::string, Instruction const*>> collectUsesAsStoreInstOf(const GetElementPtrInst* GEP)
   {
      // Retrieve the label of a pointer
      auto getPtrLabel = [](const std::string& str) {
         std::string label = "";
         unsigned i = 0;
         // Remove any initial whitespace
         while(i < str.size() && str[i] == ' ')
         {
            i++;
         }
         while(i < str.size() && str[i] != ' ')
         {
            if(str[i] >= '0' && str[i] <= '9')
            {
               label += str[i];
            }
            i++;
         }
         return label;
      };

      // Retrieve the label of a constant value
      auto getValueLabel = [](const std::string& str) {
         std::string label = "";
         unsigned i = 0;
         // Remove any initial whitespace
         while(i < str.size() && str[i] == ' ')
         {
            i++;
         }
         uint8_t encounteredSpaces = 0;
         // Move pointer to the value being stored
         while(i < str.size() && str[i] != ' ')
         {
            i++;
         }
         i++;
         // Save our value
         while(i < str.size() && (str[i] >= '0' && str[i] <= '9'))
         {
            label += str[i];
            i++;
         }
         return label;
      };

      std::vector<std::tuple<int, std::string, Instruction const*>> res;
      std::string stringBuff;
      raw_string_ostream stream(stringBuff);

      GEP->print(stream);
      Utils::log(errs(), 2, 1, "Analyzing store uses of GEP instruction: ", stringBuff);
      stringBuff.clear();

      // Iterate through all uses of the GEP instruction
      for(const Use& U : GEP->uses())
      {
         const User* user = U.getUser(); // Get the user (instruction that uses the GEP result)

         // Check if the GEP result is used in a store instruction. We are interested
         // in the first one (there should be only one use though).
         if(const StoreInst* storeInst = dyn_cast<StoreInst>(user))
         {
            // TODO: can this be done in a easier way?
            // Retrieve the label being store
            const Value* storedValue = storeInst->getValueOperand();
            storedValue->print(stream);
            Utils::log(errs(), 2, 1, "Stored value: ", stringBuff);

            std::string label;
            if constexpr(T == StoreType::PointerStore)
            {
               label = getPtrLabel(stringBuff);
            }
            else
            {
               label = getValueLabel(stringBuff);
            }
            stringBuff.clear();
            storeInst->print(stream);
            const std::string storeInstructionStr = removeEmptyPrefix(stringBuff);
            res.push_back({std::stoi(label), storeInstructionStr, static_cast<Instruction const*>(storeInst)});
            stringBuff.clear();
         }
      }
      return res;
   }

   /// Retrieve the omp task entry id from its whole name
   /// @param name The task entry point lambda name
   /// @return The ID associated with the task entry lambda
   std::string getOmpTaskEntryId(std::string const& name)
   {
      unsigned i = 0;
      // Prepend with a zero as the id will be converted to an unsigned. The first
      // created task entry has no numeric id hence we assign it to zero
      std::string res = "0";
      // We could directly jump to the index as they all have the same prefix. But
      // for safety let's go in this way for now
      while(i < name.size() && (name[i] < '0' || name[i] > '9'))
      {
         i++;
      }
      while(i < name.size())
      {
         res += name[i++];
      }
      return res;
   }

   /// Find the input labels to a omp task entry lambda
   /// @param I The omp task alloc instruction
   /// @param swapMap A map containing swap values for input proxy labels
   /// @param fnName The current function being visited
   /// @return A tuple containing inputs, const inputs and inputs @ref
   /// llvm::Instruction reference
   std::tuple<std::set<std::pair<unsigned, unsigned>, InputSetCmp>, std::vector<std::pair<unsigned, unsigned>>,
              std::vector<std::pair<Instruction*, unsigned>>>
   findTaskEntrypointInputs(Instruction& I, std::unordered_map<unsigned, unsigned> const& swapMap,
                            const std::string& fnName)
   {
      // Not constant inputs
      std::set<std::pair<unsigned, unsigned>, InputSetCmp> inputs;
      // Constant inputs with its position
      std::vector<std::pair<unsigned, unsigned>> constInputs;
      // Not constants inputs reference with its position
      std::vector<std::pair<Instruction*, unsigned>> inputsRef;
      // The input position in the argument list
      unsigned position = 0;

      auto cleanOperandString = [](std::string const& str) {
         unsigned index = 0;
         std::string cleaned = "";
         while(index < str.size() && str[index] != '%')
         {
            index++;
         }
         index++;
         while(index < str.size() && str[index] >= '0' && str[index] <= '9')
         {
            cleaned += str[index++];
         }
         return cleaned;
      };

      // A const input is expected to work correctly
      auto cleanOperandString2 = [](std::string const& str) {
         unsigned index = 0;
         std::string cleaned = "";
         uint8_t spaces = 0;
         while(index < str.size() && spaces < 1)
         {
            if(str[index] == ' ')
               spaces++;
            index++;
         }
         // index++;
         if(index < str.size() && str[index] == '%')
         {
            index++;
         }
         while(index < str.size() && str[index] >= '0' && str[index] <= '9')
         {
            cleaned += str[index++];
         }
         return cleaned;
      };

      auto saveStore = [&swapMap, &cleanOperandString, &cleanOperandString2, &inputs, &constInputs, &inputsRef,
                        &position](StoreInst* storeInst, const auto index) {
         std::string stringBuff;
         raw_string_ostream stream(stringBuff);
         storeInst->getOperand(index)->print(stream);
         std::string parsedOperand = "";
         const bool validPtr = isPtr(stringBuff);
         if(validPtr)
         {
            parsedOperand = cleanOperandString(stringBuff);
         }
         else
         {
            parsedOperand = cleanOperandString2(stringBuff);
         }
         Utils::log(errs(), 2, 1,
                    "Found a store instruction linked to the omp task "
                    "allocation struct (aka task input). Stored value label:",
                    parsedOperand, "Original buf:", stringBuff);
         const unsigned numericalValue = std::stoi(parsedOperand);

         // Swap the label if any replacement has been provided
         const auto v = swapMap.find(numericalValue) == swapMap.end() ? numericalValue : swapMap.at(numericalValue);
         if(validPtr)
         {
            inputs.insert({v, position});

            // Save this save instruction reference only if not constant
            inputsRef.push_back({storeInst, position++});
         }
         else
         {
            constInputs.push_back({v, position++});
         }
      };

      // Parse ptr inputs
      if(auto* callInst = dyn_cast<CallInst>(&I))
      {
         // Check if it's a call to __kmpc_omp_task_alloc
         assert(requireDirectCallee(callInst)->getName() == Constants::targetOmpTaskAllocName);

         for(auto* user : callInst->users())
         {
            if(auto* loadInst = dyn_cast<LoadInst>(user))
            {
               for(auto* loadUser : loadInst->users())
               {
                  if(auto* storeInst = dyn_cast<StoreInst>(loadUser))
                  {
                     saveStore(storeInst, 0);
                  }
                  else if(auto* gepInst = dyn_cast<GetElementPtrInst>(loadUser))
                  {
                     for(auto* gepInstUser : gepInst->users())
                     {
                        if(auto* storeInstOfGep = dyn_cast<StoreInst>(gepInstUser))
                        {
                           saveStore(storeInstOfGep, 0);
                        }
                     }
                  }
               }
            }
            else if(auto* gepInst = dyn_cast<GetElementPtrInst>(user))
            {
               // Parse const inputs (firstprivate)
               for(auto* gepInstUser : gepInst->users())
               {
                  if(auto* storeInst = dyn_cast<StoreInst>(gepInstUser))
                  {
                     // Parse only the fist operand as it contains
                     // the reference we are looking for
                     saveStore(storeInst, 0);
                  }
               }
            }
         }
      }
      // In the ll, the const input are parsed in the reverse order
      std::reverse(constInputs.begin(), constInputs.end());
      return {inputs, constInputs, inputsRef};
   }

   void handleNeverLoadedBasePtrs(Function& F)
   {
      const auto fnName = F.getName();
      for(auto& [input, meta] : ptrToIntSourcesParents[fnName.str()])
      {
         if(std::get<1>(meta) == NA_VALUE)
         {
            neverReferencedBaseInputs.insert(input);
         }
      }

      Utils::log(errs(), 0, 1, "Base inputs");
      for(auto v : neverReferencedBaseInputs)
      {
         Utils::log(errs(), 2, 1, v);
      }

      for(auto& [input, meta] : ptrToIntSourcesParents[fnName.str()])
      {
         if(std::get<1>(meta) != NA_VALUE)
         {
            neverReferencedBaseInputs.erase(std::get<1>(meta));
         }
      }

      if(neverReferencedBaseInputs.size())
      {
         Utils::log(errs(), 0, 1, "Base inputs never referenced");
         for(auto v : neverReferencedBaseInputs)
         {
            Utils::log(errs(), 2, 1, v);
         }
      }
      else
      {
         Utils::log(errs(), 0, 1, "No base inputs never referenced");
      }

      // Update never referenced ptrs from base to shifted (they remain themselfs with offset zero)
      for(auto v : neverReferencedBaseInputs)
      {
         std::get<0>(ptrToIntSourcesParents[fnName.str()][v]) = false;
         std::get<1>(ptrToIntSourcesParents[fnName.str()][v]) = v;
         std::get<2>(ptrToIntSourcesParents[fnName.str()][v]) = 0;
         std::get<3>(ptrToIntSourcesParents[fnName.str()][v]) = nullptr;
      }
   }

   /// Print to std error the ptrToIntSourcesParents data structure
   /// Do not use @ref llvm::Value pointers stored inside ptrToIntSourcesParents
   /// as they might have been invalidated at this point.
   void printPtrs()
   {
      for(auto& [fnName, m] : ptrToIntSourcesParents)
      {
         Utils::log(errs(), 0, 2, "Function:", fnName);
         for(auto& [a, b] : m)
         {
            if(std::get<1>(b) != NA_VALUE)
            {
               Utils::log(errs(), 0, 1, a, "(", std::get<3>(b), ") is the ptr", std::get<1>(b), "+ offset(",
                          std::get<2>(b), ')');
            }
            else
            {
               Utils::log(errs(), 0, 1, a, "(", std::get<3>(b), ") is the ptr", a, "+ 0");
            }
         }
      }
   }

   /// Assign a valid weight for non base inputs inside the task entry dictionary
   /// @param fnName The function being visited
   void updateTaskEntryParamsWeights(std::string const& fnName)
   {
      assert(ptrToIntSourcesParents.count(fnName));

      constexpr unsigned NOT_FOUND = UINT32_MAX;

      Utils::log(errs(), 0, 1, "Updating pointer parents inside task inputs");

      const auto findParentWeight = [](const unsigned key, std::string const& id) {
         for(auto& p : taskEntryInputs[id])
         {
            if(p.first == key)
            {
               return p.second;
            }
         }
         Utils::log(errs(), 0, 1, "Can not find parent weight for", key, "inside the task entry list");
         Utils::log(errs(), 0, 1, "Task entry list for", id, ":");
         for(auto& p : taskEntryInputs[id])
         {
            Utils::log(errs(), 0, 1, p.first);
         }

         return NOT_FOUND;
      };

      const auto updateWeight = [&fnName](const unsigned key, const unsigned w) {
         for(auto it = taskEntryInputs[fnName].begin(); it != taskEntryInputs[fnName].end(); it++)
         {
            if(it->first == key)
            {
               const auto input = it->first;
               taskEntryInputs[fnName].erase(it);
               taskEntryInputs[fnName].insert({input, w});
               return true;
            }
         }
         return false;
      };

      for(auto& [taskId, params] : taskEntryInputs)
      {
         std::vector<std::pair<unsigned, unsigned>> toDelete;
         std::vector<std::pair<unsigned, unsigned>> toAdd;
         Utils::log(errs(), 2, 1, "Inputs for", taskId, "before");
         for(auto& [a, b] : params)
         {
            Utils::log(errs(), 2, 1, a, ":", b);
         }
         for(auto& [param, w] : params)
         {
            assert(ptrToIntSourcesParents[fnName].count(param));
            if(std::get<1>(ptrToIntSourcesParents[fnName][param]) == NA_VALUE)
            {
               continue;
            }
            const auto parent = std::get<1>(ptrToIntSourcesParents[fnName][param]);
            const auto parentW = findParentWeight(parent, taskId);
            assert(parentW != NOT_FOUND);
            // Update child weight
            toDelete.push_back({param, w});
            toAdd.push_back({param, parentW * 1000});
         }
         for(auto& d : toDelete)
         {
            params.erase(d);
         }
         for(auto& d : toAdd)
         {
            params.insert(d);
         }
         Utils::log(errs(), 2, 1, "Inputs for", taskId, "after");
         for(auto& [a, b] : params)
         {
            Utils::log(errs(), 2, 1, a, ":", b);
         }
      }
   }

   /// Promotes omp outlined input argument addresses
   /// @param F The omp outlined function
   void promoteOmpOutlinedBaseAddr(Function& F)
   {
      assert(F.getName().find(Constants::targetOmpOutlinedFnNamePrefix) != std::string::npos &&
             "Input function is not the outlined");
      assert(
          F.getName().find(Constants::bambuHostReservedFnName) == std::string::npos &&
          "Input function is a reserved function. You have either mistaken the call or used a reserved function name");
      Utils::log(errs(), 0, 1, "Promoting omp_outlined ptrs");

      const auto fnName = F.getName().str();
      std::vector<Instruction*> toDelete;
      std::unordered_map<Value const*, unsigned> swaps;
      std::string strBuff;
      raw_string_ostream stream(strBuff);
      unsigned argCount = 0;
      for(auto& arg : F.args())
      {
         // First two args are reserved
         if(argCount < 2)
         {
            argCount++;
            continue;
         }
         Utils::log(errs(), 2, 1, "Analyzing uses of ");
         if(Bambu::OpenMP::Task::Utils::Verbose)
         {
            arg.print(errs());
         }
         Utils::log(errs(), 0, 1, "");
         for(auto& u : arg.uses())
         {
            if(auto* loadInst = dyn_cast<LoadInst>(u.getUser()))
            {
               strBuff.clear();
               Utils::log(errs(), 2, 0, "replacing ");
               if(Bambu::OpenMP::Task::Utils::Verbose)
               {
                  loadInst->print(errs());
               }
               Utils::log(errs(), 0, 0, " with ");
               if(Bambu::OpenMP::Task::Utils::Verbose)
               {
                  arg.print(errs());
               }
               Utils::log(errs(), 0, 1, "");

               auto swapFrom = loadInst;
               arg.print(stream);
               auto swapTo = retrieveLabelFromChar(strBuff, '%');
               swaps[swapFrom] = std::stoul(swapTo);
               loadInst->replaceAllUsesWith(&arg);
               toDelete.push_back(loadInst);
            }
         }
      }

      // Update the pointer map
      std::unordered_map<Value const*, unsigned> baseAddrAfterLoad;
      for(auto& [ptrLabel, val] : ptrToIntSourcesParents[fnName])
      {
         if(std::get<1>(val) == NA_VALUE)
         {
            baseAddrAfterLoad[std::get<3>(val)] = ptrLabel;
         }
      }

#ifdef VERBOSE
      Utils::log(errs(), 2, 1, "Swap map");
      Utils::log(errs(), 2, 1, "Size:", swaps.size());
      for(auto s : swaps)
      {
         Utils::log(errs(), 2, 1, s.first, "->", s.second);
      }
#endif

#ifdef VERBOSE
      Utils::log(errs(), 2, 1, "Base ptr labels");
      Utils::log(errs(), 2, 1, "Size:", baseAddrAfterLoad.size());
      for(auto addr : baseAddrAfterLoad)
      {
         Utils::log(errs(), 2, 1, addr.first, addr.second);
      }
#endif

      for(auto const addr : baseAddrAfterLoad)
      {
         // Some inputs don't have a load before the load instruction
         if(swaps.find(addr.first) != swaps.end())
         {
            {
               for(auto& [ptrLabel, val] : ptrToIntSourcesParents[fnName])
               {
                  // Utils::log(errs(), 2, 1, std::get<3>(val));
                  if(std::get<1>(val) == addr.second)
                  {
                     std::get<1>(val) = swaps[addr.first];
                  }
               }
            }
            // Update the mappings
            ptrToIntSourcesParents[fnName][swaps[addr.first]] = {false, NA_VALUE, NA_VALUE, nullptr};
            ptrToIntSourcesParents[fnName].erase(addr.second);
            ptrToIntSourcesParents[fnName][addr.second] = {false, swaps[addr.first], 0U, nullptr};
         }
      }

      Utils::log(errs(), 0, 1, "Update ptr offsets:");
      printPtrs();
      updateTaskEntryParamsWeights(fnName);
      handleNeverLoadedBasePtrs(F);

      for(auto* I : toDelete)
      {
         I->eraseFromParent();
      }
   }

   /// @brief Adds a basic index GEP (GetElementPtr) instruction after each AllocaInst
   ///        that allocates an array of a specific struct type.
   ///
   /// This function iterates over all instructions in the given function and looks
   /// for AllocaInst instructions that allocate an array of a specific struct type
   /// (identified by Constants::targetOmpDepsStructName). For each such AllocaInst,
   /// it inserts a GEP instruction that computes the address of the first element
   /// of the allocated array.
   ///
   /// @param F The function to process.
   void addBaseIdxGep(Function& F)
   {
      for(auto& BB : F)
      {
         for(auto& I : BB)
         {
            if(auto* AI = dyn_cast<AllocaInst>(&I))
            {
               Type* AllocType = AI->getAllocatedType();

               if(auto* ArrTy = dyn_cast<ArrayType>(AllocType))
               {
                  if(auto* ST = dyn_cast<StructType>(ArrTy->getElementType()))
                  {
                     if(ST->getName() == Constants::targetOmpDepsStructName)
                     {
                        BasicBlock::iterator it(AI);
                        // Advance the iterator to point just after I.
                        ++it;
                        IRBuilder<> builder(AI->getParent(), it);
                        Type* elemTy = builder.getInt8Ty();
                        Value* index = builder.getInt64(0);
                        // Create an inbounds GEP.
                        Value* newGEP = builder.CreateGEP(elemTy, &I, index, "baseidxgep");

                        AI->replaceUsesWithIf(newGEP, [=](Use& u) {
                           if(auto* isGep = dyn_cast<GetElementPtrInst>(u.getUser()))
                           {
                              return false;
                           }
                           if(auto* isGep = dyn_cast<CallInst>(u.getUser()))
                           {
                              return false;
                           }
                           return true;
                        });
                     }
                  }
               }
            }
         }
      }
   }

   /// Collects and sorts GetElementPtr instructions that access OpenMP dependency info structures.
   ///
   /// This function scans through a list of instructions to find allocations of arrays containing
   /// OpenMP dependency info structures. For each such allocation, it collects all GetElementPtr
   /// instructions that are used to access elements of these arrays. The collected GEP instructions
   /// are sorted based on their base pointers and indices.
   ///
   /// The sorting is done to ensure consistent ordering of dependency accesses, which is important
   /// for analyzing task dependencies. GEP instructions are sorted first by their base pointer names,
   /// and for GEPs with the same base pointer, by their first index value.
   ///
   /// @param blockInstructions Input vector of instructions to scan through
   /// @param collector Output vector where collected GEP instructions will be stored
   ///
   /// The function:
   /// 1. Finds AllocaInst instructions that allocate arrays of OpenMP dependency info structures
   /// 2. For each such allocation, collects all GEP instructions used to access it
   /// 3. Sorts the collected GEP instructions by:
   ///    - Base pointer name (primary key)
   ///    - First index value (secondary key, assumes ConstantInt indices)
   ///
   /// Assumptions:
   /// - The allocated arrays have the expected alignment (Constants::structAlignment)
   /// - GEP instructions accessing the arrays have ConstantInt as their first index operand
   ///
   /// Example usage:
   /// ```cpp
   /// std::vector<Instruction*> blockInsts = [...];
   /// std::vector<Instruction*> gepInsts;
   /// collectGepsFromDependInfoAlloc(blockInsts, gepInsts);
   /// // gepInsts now contains sorted GEP instructions accessing dependency info
   /// ```
   void collectGepsFromDependInfoAlloc(std::vector<Instruction*> const& blockInstructions,
                                       std::vector<Instruction*>& collector)
   {
      for(auto* I : blockInstructions)
      {
         if(auto* AI = dyn_cast<AllocaInst>(I))
         {
            Type* AllocType = AI->getAllocatedType();

            if(auto* ArrTy = dyn_cast<ArrayType>(AllocType))
            {
               if(auto* ST = dyn_cast<StructType>(ArrTy->getElementType()))
               {
                  if(ST->getName() == Constants::targetOmpDepsStructName)
                  {
                     assert(AI->getAlign().value() == Constants::structAlignment);
                     for(auto& U : I->uses())
                     {
                        if(auto* GEP = dyn_cast<GetElementPtrInst>(U.getUser()))
                        {
                           collector.push_back(GEP);
                        }
                     }
                  }
               }
            }
         }
      }

      std::sort(collector.begin(), collector.end(), [](const Instruction* a, const Instruction* b) {
         auto* gepA = dyn_cast<GetElementPtrInst>(a);
         auto* gepB = dyn_cast<GetElementPtrInst>(b);
         // Get the base pointer operands.
         const auto* ptrOpA = gepA->getPointerOperand();
         const auto* ptrOpB = gepB->getPointerOperand();

         // Compare ptr operands
         std::string stringBuff = "";
         raw_string_ostream stream(stringBuff);
         ptrOpA->print(stream);
         std::string nameA = stringBuff;
         stringBuff.clear();
         ptrOpB->print(stream);
         std::string nameB = stringBuff;
         if(nameA != nameB)
         {
            return nameA < nameB;
         }

         // If they have the same base pointer, compare by the first index.
         // We assume the first index operand (after the base) is a ConstantInt.
         auto* constOpA = dyn_cast<ConstantInt>(a->getOperand(1));
         auto* constOpB = dyn_cast<ConstantInt>(b->getOperand(1));
         return constOpA->getZExtValue() < constOpB->getZExtValue();
      });
   }

#ifdef ENABLE_RECURSIVE_SEARCH
   /// Visit and find dependencies, inputs of children tasks (tasks created inside
   /// a task)
   /// @param inputs The children input list
   /// @param entryPointName The children task entry point lambda name
   /// @param taskHash A reference to the task hash generator
   void ompDependencyFinderChilds(std::set<unsigned> const& inputs, std::string const& entryPointName,
                                  unsigned& taskHash)
   {
      // Recover the cached function
      Function* fn = fnCache[entryPointName];
      assert(fn);
      Utils::log(errs(), 2, 1, "Recovered child fn", fn->getName());

      // Log
      Utils::log(errs(), 2, 1, "Inputs to the entry point:");
      Utils::log(errs(), 2, 0, "[");
      for(auto input : inputs)
      {
         const std::string formattedInput = "%" + std::to_string(input);
         Utils::log(errs(), 2, 0, formattedInput);
      }
      Utils::log(errs(), 2, 1, "]");

      // Recursively visit this task entry point
      TopoGraphGenerator topo;
      visitor(*fn, &inputs, taskHash, topo, false);
   }
#endif

   /// Compute all the GEP instructions to the target dependency struct by
   /// extracting dependency sources and types
   /// @param blockInstructions The instructions of a function block
   /// @param inputMap A label swap map
   /// @param fnName The current function being visited
   /// @return The extracted informations as mapping
   void storeGEPDepStructAccesses(std::vector<Instruction*> const& blockInstructions,
                                  std::unordered_map<unsigned, unsigned> const& inputMap, const std::string& fnName,
                                  std::unordered_map<std::string, std::pair<unsigned, unsigned>>& collector)
   {
      std::vector<unsigned> foundDepsLabels;
      std::vector<std::string> depsStoreKeys;
      std::unordered_map<Instruction const*, std::vector<Instruction*>::const_iterator> instructionToVectorPos;
      std::vector<Instruction*> gepInstructions;

      for(auto it = blockInstructions.cbegin(); it != blockInstructions.end(); it++)
      {
         instructionToVectorPos[*it] = it;
      }

      collectGepsFromDependInfoAlloc(blockInstructions, gepInstructions);

      // Utils::log(errs(), 0, 1, "Collected geps:\n");
      // for(auto& e : gepInstructions)
      //{
      //    e->print(errs());
      //    errs() << "\n";
      // }

      // The following block relies on the fact that gep accesses to the same omp
      // deps struct are ordered by field
      auto it = gepInstructions.cbegin();
      for(; it != gepInstructions.end(); it++)
      {
         if(auto* GEP = dyn_cast<GetElementPtrInst>(*it))
         {
            // This block retrieves the dependency sources and ther types (in or
            // out)

            // KAIXI: Do refer to technical docs for these accesses
            auto lastIndex = GEP->getNumOperands();
            // We do expect structs with more than one field
            assert(lastIndex > 0);
            lastIndex--;

            Value* value = GEP->getOperand(lastIndex);
            if(const ConstantInt* CI = dyn_cast<ConstantInt>(value))
            {
               // get the GEP access index inside the dep struct
               auto offSet = CI->getValue().getSExtValue();
               // For reference: %struct.kmp_depend_info = type { i64, i64, i8 }
               // %5 = alloca [2 x %struct.kmp_depend_info], align 8
               offSet = (offSet % (Constants::structAlignment * 3)) / Constants::structAlignment;
               Utils::log(errs(), 0, 1, "Visiting GEP on allocation instruction with offset", offSet);

               if(offSet == 0)
               {
                  Utils::log(errs(), 2, 1, "GEP instruction on the dependency source field");
                  const auto uses = collectUsesAsStoreInstOf<StoreType::PointerStore>(GEP);

                  for(auto& use : uses)
                  {
                     int label = std::get<0>(use);
                     depsStoreKeys.push_back(std::get<1>(use));
                     // The label being stored in the GEP reference will be the output
                     // alias from a `ptrtoint` instruction. The real data source is
                     // available inside the `ptrtoint` call which is before this
                     // instruction. Recover it.
                     label = std::stoi(recoverPtrToIntSource(
                         blockInstructions, std::make_reverse_iterator(instructionToVectorPos[std::get<2>(use)]),
                         std::to_string(label), fnName));
                     auto foundDepsLabel = label;
                     // Map the label to a parent source if available
                     if(inputMap.find(foundDepsLabel) != inputMap.end())
                     {
                        foundDepsLabel = inputMap.at(foundDepsLabel);
                     }
                     foundDepsLabels.push_back(foundDepsLabel);
                  }
               }
               else if(offSet == 2)
               {
                  Utils::log(errs(), 2, 1, "GEP instruction on the dependency type field");
                  const auto uses = collectUsesAsStoreInstOf<StoreType::ValueStore>(GEP);
                  // If the compiler swaps the GEP accesses order this pass will fail
                  // at the next assert. It's rare but it can happen.
                  assert(foundDepsLabels.size());
                  assert(foundDepsLabels.size() == uses.size() && uses.size() == depsStoreKeys.size());
                  const unsigned size = uses.size();
                  for(unsigned i = 0; i < size; ++i)
                  {
                     const int depType = std::get<0>(uses[i]);
                     collector[depsStoreKeys[i]] = {foundDepsLabels[i], depType};
                  }
                  // GEPs in this current struct are sorted by field, reset the
                  // buffers for the next struct
                  foundDepsLabels.clear();
                  depsStoreKeys.clear();
               }
            }
         }
      }
   }

   /// Find omp dependencies from a list of instructions containing a sequence
   /// of omp task calls. Dependencies are discovered by leveraging GEP calls
   /// which populates omp deps structs. Other instructions might be present
   /// inside the input sequence and are filtered out. This pass will also save
   /// for each omp task entry lambda the relative inputs.
   /// GEP accesses to the resolved dependencies structs (with dependency sources
   /// and types) are provided as input.
   /// @param fnName The function being visited
   /// @param instructions A @ref std::vector of @ref Instruction
   /// @param allFnInstruction The complete @ref llvm::Instruction list of the
   /// current visited function
   /// @param targetStructName The struct name to search
   /// @param entryPointInputs A vector of the current task entry lambda inputs
   /// @param precomputedDeps A map between dependencies store instructions to the
   /// @param taskEntryPointCount A counting map from task entry name to its
   /// frequency in the current @ref llvm::Function block relative dependency
   /// source and type labels
   /// @param taskHash A reference to the task hash generator
   template <bool ShowInbounds = false>
   void ompDependenciesFinder(const std::string& fnName, std::vector<Instruction*> const& instructions,
                              std::vector<Instruction*> const& allFnInstructions, const std::string& targetStructName,
                              std::set<unsigned> const* entryPointInputs,
                              std::unordered_map<std::string, std::pair<unsigned, unsigned>> const& precomputedDeps,
                              std::unordered_map<std::string, unsigned> const& taskEntryPointCount, unsigned& taskHash)
   {
      enum DepType
      {
         IN_DEP,
         OUT_DEP,
         INOUT_DEP
      };

      const auto addDeps = [](auto& deps, auto& lambdaId, auto& source, const DepType type) {
         if(type == IN_DEP)
         {
            std::get<0>(deps[lambdaId]).insert(source);
         }
         else if(type == OUT_DEP)
         {
            std::get<1>(deps[lambdaId]).insert(source);
         }
         else if(type == INOUT_DEP)
         {
            std::get<2>(deps[lambdaId]).insert(source);
         }
      };

      std::unordered_map<unsigned, unsigned> inputMap;
      std::string ompTaskEntryLambdaName = "";
      Value* ompTaskEntryLambdaArg = nullptr;
      Instruction* taskAllocCallRef = nullptr;
      std::string stringBuff = "";
      raw_string_ostream stream(stringBuff);
      // Using sorted set to have labels sorted
      std::unordered_map<std::string,
                         std::tuple<std::set<unsigned>, std::set<unsigned>, std::set<unsigned>, Instruction*>>
          tempDeps;
      std::unordered_map<std::string, std::vector<std::pair<unsigned, unsigned>>> tempConstInputs;
      std::unordered_map<std::string, std::set<std::pair<unsigned, unsigned>, InputSetCmp>> tempInputs;

      Utils::log(errs(), 0, 1, "");
      Utils::log(errs(), 0, 1, "First instruction:", *(instructions[0]));
      Utils::log(errs(), 0, 1, "Last instruction:", *(instructions[instructions.size() - 1]));

      auto it = entryPointInputs ? entryPointInputs->begin() : decltype(entryPointInputs->begin()){};

      for(auto& inst : instructions)
      {
         if(CallInst* ompTaskAPICall = dyn_cast<CallInst>(inst))
         {
            auto calledFnName = requireDirectCallee(ompTaskAPICall)->getName().str();
            // It's not either a task creation open call or close call
            if((calledFnName != Constants::targetOmpTaskAllocName) &&
               (calledFnName != Constants::targetOmpTaskSubmissionDepsName) &&
               (calledFnName != Constants::targetOmpTaskSubmissionName))
            {
               report_fatal_error("Call instruction inside the lambda regions are not supported.");
            }
            else if(calledFnName == Constants::targetOmpTaskAllocName)
            {
               // Extract the executed lambda reference
               const auto noOperands = ompTaskAPICall->getNumOperands();
               // Cache the task alloc call instruction
               taskAllocCallRef = ompTaskAPICall;
               // The task entry lambda arguments is stored at the -2 index
               ompTaskEntryLambdaArg = ompTaskAPICall->getArgOperand(noOperands - 2);
               Utils::log(errs(), 2, 1, "Lambda called name:", ompTaskEntryLambdaArg->getName());
               ompTaskEntryLambdaName = ompTaskEntryLambdaArg->getName().str();

               inst->print(stream);
               const std::string calledInstruction = stringBuff;
               stringBuff.clear();
               Utils::log(errs(), 2, 1, "From instruction:", calledInstruction);

               // Find the task inputs. Dependencies data will figure in this
               // input vector as they must be shared variables from a parallel region
               const auto inputs = findTaskEntrypointInputs(*inst, inputMap, fnName);
               // Cache the input stores refs for this instruction
               taskEntryInputsRef[inst] = {ompTaskEntryLambdaArg, std::get<2>(inputs)};
               const auto id = getOmpTaskEntryId(ompTaskEntryLambdaName);
               taskEntryParentInputs[std::stoi(getOmpTaskEntryId(ompTaskEntryLambdaName))] = std::get<0>(inputs);
               tempInputs[id] = std::get<0>(inputs);
               tempConstInputs[id] = std::get<1>(inputs);
            }
            else if(calledFnName == Constants::targetOmpTaskSubmissionName)
            {
               // When encountering a closing API call, if it's a task with no
               // deps save it
               freeTasks[(getOmpTaskEntryId(ompTaskEntryLambdaName))]++;
               // Save the inputs and const inputs
               const auto id = getOmpTaskEntryId(ompTaskEntryLambdaName);
               taskEntryConstInputs[id] = tempConstInputs[id];
               taskEntryInputs[id] = tempInputs[id];
               // The inputs will have the original labels defined inside the parallel
               // region entry point (omp_outlined) to have a consistent global
               // identification

#ifdef ENABLE_RECURSIVE_SEARCH
               ompDependencyFinderChilds(tempInputs[id], ompTaskEntryLambdaName,
                                         taskHash); // for any bugs here try to use
                                                    // taskEntryParentInputs
#endif
            }
            else if(calledFnName == Constants::targetOmpTaskSubmissionDepsName)
            {
// The inputs will have the original labels defined inside the parallel
// region entry point (omp_outlined) to have a consistent global
// identification
#ifdef ENABLE_RECURSIVE_SEARCH
               ompDependencyFinderChilds(
                   tempInputs[(getOmpTaskEntryId(ompTaskEntryLambdaName))], // for any bugs here try to use
                                                                            // taskEntryParentInputs
                   ompTaskEntryLambdaName, taskHash);
#endif
            }
         }
         else if(auto* storeInst = dyn_cast<StoreInst>(inst))
         {
            // A temporary deps map will be filled as multiple store calls may be
            // present for the same task entry lambda
            storeInst->print(stream);
            const std::string storeInstStr = removeEmptyPrefix(stringBuff);
            if(precomputedDeps.find(storeInstStr) != precomputedDeps.end())
            {
               Utils::log(errs(), 2, 1,
                          "Found a precomputed dependency instruction. Assigning to "
                          "the current task entry");
               assert(precomputedDeps.find(storeInstStr) != precomputedDeps.end());
               const unsigned depType = precomputedDeps.at(storeInstStr).second;
               unsigned depSource = precomputedDeps.at(storeInstStr).first;
               // We map any local proxy to its global ones
               if(inputMap.find(depSource) != inputMap.end())
               {
                  depSource = inputMap[depSource];
               }
               // Swap any same GEP access
               if(sameGEPsReverse[fnName].find(depSource) != sameGEPsReverse[fnName].end())
               {
                  depSource = sameGEPsReverse[fnName][depSource];
               }

               const auto lambdaId = getOmpTaskEntryId(ompTaskEntryLambdaName);
               // Save the called lambda Value
               std::get<3>(tempDeps[lambdaId]) = taskAllocCallRef;
               // Dispatch input deps and output deps
               if(inOutDepsFlags.find(depType) != inOutDepsFlags.end())
               {
                  Utils::log(errs(), 2, 1, "Assigning dependent tasks. Holder:", ompTaskEntryLambdaName,
                             " inputoutput dep with:", depSource);
                  addDeps(tempDeps, lambdaId, depSource, INOUT_DEP);
                  // addDeps(tempDeps, lambdaId, depSource, OUT_DEP);
               }
               else if(inDepsFlags.find(depType) != inDepsFlags.end())
               {
                  Utils::log(errs(), 2, 1, "Assigning dependent tasks. Holder:", ompTaskEntryLambdaName,
                             " input dep with:", depSource);
                  addDeps(tempDeps, lambdaId, depSource, IN_DEP);
               }
               else if(outDepsFlags.find(depType) != outDepsFlags.end())
               {
                  Utils::log(errs(), 2, 1, "Assigning dependent tasks. Holder:", ompTaskEntryLambdaName,
                             " output dep with:", depSource);
                  addDeps(tempDeps, lambdaId, depSource, OUT_DEP);
               }
            }
            else
            {
               // Utils::log(errs(), 2, 1,
               //            "Not recognized store instruction:", storeInstStr);
            }
            stringBuff.clear();
         }
         else if(auto* GEP = dyn_cast<GetElementPtrInst>(inst))
         {
            if(GetElementPtrInstIsSpecificStruct(GEP, Constants::targetOmpAnonStructName))
            {
               // This is needed to retrieve the inputs loaded from an anon struct
               // These structs contain the inputs for the current task entry point.
               // Their original reference are stored inside entryPointInputs which is
               // a list of inputs for the current task entry point but filled at the
               // parent level. Used for nested task structures.
               //
               // An example:
               //
               // %1 = task_alloc(..., task_entry_X)
               // # compiler stores inside %1 the allocated struct inputs
               // # The previous inputs are captured at previous stages of this pass as
               // forwarded here inside the `entryPointInputs` variable
               //
               // void task_entry_X():
               //    %1 = struct.anon
               //    %2 = load type, struct.anon %1 <-- we have to match this proxy
               //    with its original proxy to have global identifiers
               Utils::log(errs(), 2, 1, "Found anon struct. #uses:", GEP->getNumUses());
               for(const auto* anonUser : GEP->users())
               {
                  anonUser->print(stream);
                  Utils::log(errs(), 2, 1, "Anon struct ref used in:", stringBuff);
                  stringBuff.clear();
                  if(auto* loadInst = dyn_cast<LoadInst>(anonUser))
                  {
                     loadInst->print(stream);
                     const auto parsed = parseUnamedNameFromInstruction(stringBuff);
                     stringBuff.clear();
                     Utils::log(errs(), 2, 1,
                                "The previous use is a target load instruction relative "
                                "to the task entry inputs. Value "
                                "being saved in proxy:",
                                parsed);

                     // If the current task entry lambda has `load` from a `struct.anon`
                     // it must have inputs
                     assert(it != entryPointInputs->end());
                     // Map the parent label to the current one
                     inputMap[std::stoi(parsed)] = *it;
                     it++;
                     break;
                  }
               }
            }
         }
      }

      const auto formatId = [](std::string s) {
         size_t pos = 0;
         const std::string from = ",}";
         const std::string to = "}";
         while((pos = s.find(from, pos)) != std::string::npos)
         {
            s.replace(pos, from.length(), to);
         }
         return s;
      };

      // As different task entry lambdas can be called on different
      // dependencies we have to create an unique lambda identifier.
      //
      // Unify the temp deps
      for(auto& p : tempDeps)
      {
         std::string id = p.first;
         const auto idCopy = std::string(id);
         std::string in = "#I{";
         std::string out = "#O{";
         std::string inout = "#IO{";
         for(auto n : std::get<0>(p.second))
         {
            in += std::to_string(n) + ',';
         }
         in += '}';
         for(auto n : std::get<1>(p.second))
         {
            out += std::to_string(n) + ',';
         }
         out += '}';
         for(auto n : std::get<2>(p.second))
         {
            inout += std::to_string(n) + ',';
         }
         inout += '}';
         id += formatId(in + out + inout);
         for(auto n : std::get<0>(p.second))
         {
            std::get<0>(dependentTasks[id]).insert(n);
         }
         for(auto n : std::get<1>(p.second))
         {
            std::get<1>(dependentTasks[id]).insert(n);
         }
         for(auto n : std::get<2>(p.second))
         {
            std::get<2>(dependentTasks[id]).insert(n);
         }
         taskCreationOrderList.push_back(id);
         // Copy the const arguments for the unique generated task id
         taskEntryConstInputs[id] = tempConstInputs[idCopy];

         // Save the unique name generated for this lambda
         lambdaEntryPointUniqueNameDict[std::get<3>(p.second)] = {id, taskHash++};

         // By default, the previous passes assign as tasks input the base addresses
         // for array types even though the index accessed inside the deps or fn
         // bodies are different from zero. Perform array base pointer to index
         // pointer swap
         {
            auto& inputs = tempInputs[idCopy];
            // We can also loop over the tuple but let's not...
            std::vector<std::set<unsigned int>*> unifiedDeps = {
                &std::get<0>(dependentTasks[id]),
                &std::get<1>(dependentTasks[id]),
                &std::get<2>(dependentTasks[id]),
            };
            unsigned dummyW = UINT32_MAX;
            for(auto deps : unifiedDeps)
            {
               for(auto n : *deps)
               {
                  if(ptrToIntSourcesParents[fnName].find(n) != ptrToIntSourcesParents[fnName].end())
                  {
                     // Check only zero offsets
                     if(std::get<2>(ptrToIntSourcesParents[fnName][n]) == 0)
                     {
                        // Remove k
                        auto k = std::get<1>(ptrToIntSourcesParents[fnName][n]);
                        auto it = std::find_if(inputs.begin(), inputs.end(),
                                               [k](const std::pair<unsigned, unsigned>& p) { return p.first == k; });
                        if(it != inputs.end())
                        {
                           Utils::log(errs(), 2, 1, "removing old input key", k);
                           inputs.erase(it);
                        }
                     }
                     Utils::log(errs(), 2, 1, "adding new key", n);
                     inputs.insert({n, dummyW--});
                  }
               }
            }
         }
         taskEntryInputs[id] = tempInputs[idCopy];

#ifdef VERBOSE
         Utils::log(errs(), 2, 1, "updated inputs for", id);
         for(auto& input : taskEntryInputs[id])
         {
            Utils::log(errs(), 4, 1, input.first);
         }
#endif
      }
   }

   /// @brief Generates a dependency graph for tasks.
   ///
   /// This function generates a dependency graph for tasks based on their creation order and dependencies.
   /// It logs the dependency graph, creates edges between dependent tasks, and populates various maps
   /// to keep track of input, output, and input-output dependencies.
   ///
   /// @param fnName The name of the function.
   /// @param nodes A reference to a vector of nodes representing tasks.
   /// @param adjMap A reference to an adjacency map representing the edges between tasks.
   void generateDepsGraph(const std::string& fnName, Nodes& nodes, AdjMap& adjMap)
   {
      Utils::log(errs(), 0, 1, "\n\nDependency graph:");
      std::unordered_map<unsigned, std::vector<std::string>> outDepsMap;
      std::unordered_map<unsigned, std::vector<std::string>> inDepsMap;
      std::unordered_map<unsigned, std::vector<std::string>> inoutDepsMap;

      std::set<std::pair<std::string, std::string>> edges;

      for(auto& k : taskCreationOrderList)
      {
         Utils::log(errs(), 0, 1, "\n\n===================================== New task:");
         // Add new node
         nodes.push_back(k);
         const std::pair<std::string, std::tuple<std::set<unsigned>, std::set<unsigned>, std::set<unsigned>>> kv = {
             k, dependentTasks[k]};
         const std::string formattedLambdaName = 'L' + (kv.first);
         Utils::log(errs(), 0, 1, "\n\nLambda name id:", formattedLambdaName);
         if(taskEntryInputs[(kv.first)].size())
         {
            Utils::log(errs(), 0, 1, "Const inputs:");
            Utils::log(errs(), 0, 0, "[");
            for(auto& input : taskEntryConstInputs[(kv.first)])
            {
               const std::string formattedInput = std::to_string(input.first);
               Utils::log(errs(), 0, 0, formattedInput);
            }
            Utils::log(errs(), 0, 1, "]");
            Utils::log(errs(), 0, 1, "Inputs:");
            Utils::log(errs(), 0, 0, "[");
            for(auto& input : taskEntryInputs[(kv.first)])
            {
               std::string formattedInput = '%' + std::to_string(input.first);
#ifdef VERBOSE
#ifdef SHOW_INPUT_PROXIES
               if(fnInputProxies[fnName][input.first].size())
               {
                  formattedInput += "(Proxies: ";
                  for(auto n : fnInputProxies[fnName][input.first])
                  {
                     formattedInput += std::to_string(n) + ',';
                  }
                  formattedInput += ')';
               }
#endif
#endif
               Utils::log(errs(), 0, 0, formattedInput);
            }
            Utils::log(errs(), 0, 1, "]");
         }

         // Just an util for printing
         std::set<std::string> uniqueParents;
         // Rules can be found here:
         // https://stackoverflow.com/questions/27475174/task-dependency-in-openmp-4
         // Input deps:
         for(auto& d : std::get<0>(kv.second))
         {
            for(auto& parent : outDepsMap[d])
            {
               const auto parentId = recoverLambdaIdFromUniqueId(parent);
               const auto thisId = recoverLambdaIdFromUniqueId(kv.first);
               // We rely on the fact that inner created tasks have a lower id
               // Utils::log(errs(), 0 , 1, "Checking dep for parent", parentId, "and
               // child", thisId);
               // This work because we are iterating over sorted tasks (based on the
               // task creation order)
               uniqueParents.insert(parent);
               // TODO: support nested tasks

               // Create edge
               edges.insert({parent, kv.first});
            }
            for(auto& parent : inoutDepsMap[d])
            {
               const auto parentId = recoverLambdaIdFromUniqueId(parent);
               const auto thisId = recoverLambdaIdFromUniqueId(kv.first);
               // We rely on the fact that inner created tasks have a lower id
               // Utils::log(errs(), 0 , 1, "Checking dep for parent", parentId, "and
               // child", thisId);
               // This work because we are iterating over sorted tasks (based on the
               // task creation order)
               uniqueParents.insert(parent);
               // TODO: support nested tasks

               // Create edge
               edges.insert({parent, kv.first});
            }
         }
         // Output deps:
         for(auto& d : std::get<1>(kv.second))
         {
            for(auto& parent : inDepsMap[d])
            {
               const auto parentId = recoverLambdaIdFromUniqueId(parent);
               const auto thisId = recoverLambdaIdFromUniqueId(kv.first);
               // We rely on the fact that inner created tasks have a lower id
               // Utils::log(errs(), 0 , 1, "Checking dep for parent", parentId, "and
               // child", thisId);
               // This work because we are iterating over sorted tasks (based on the
               // task creation order)
               uniqueParents.insert(parent);
               // TODO: support nested tasks

               // Create edge
               edges.insert({parent, kv.first});
            }
            for(auto& parent : outDepsMap[d])
            {
               const auto parentId = recoverLambdaIdFromUniqueId(parent);
               const auto thisId = recoverLambdaIdFromUniqueId(kv.first);
               // We rely on the fact that inner created tasks have a lower id
               // Utils::log(errs(), 0 , 1, "Checking dep for parent", parentId, "and
               // child", thisId);
               // This work because we are iterating over sorted tasks (based on the
               // task creation order)
               uniqueParents.insert(parent);
               // TODO: support nested tasks

               // Create edge
               edges.insert({parent, kv.first});
            }
            for(auto& parent : inoutDepsMap[d])
            {
               const auto parentId = recoverLambdaIdFromUniqueId(parent);
               const auto thisId = recoverLambdaIdFromUniqueId(kv.first);
               // We rely on the fact that inner created tasks have a lower id
               // Utils::log(errs(), 0 , 1, "Checking dep for parent", parentId, "and
               // child", thisId);
               // This work because we are iterating over sorted tasks (based on the
               // task creation order)
               uniqueParents.insert(parent);
               // TODO: support nested tasks

               // Create edge
               edges.insert({parent, kv.first});
            }
         }
         // InputOutput deps:
         for(auto& d : std::get<2>(kv.second))
         {
            for(auto& parent : inDepsMap[d])
            {
               const auto parentId = recoverLambdaIdFromUniqueId(parent);
               const auto thisId = recoverLambdaIdFromUniqueId(kv.first);
               // We rely on the fact that inner created tasks have a lower id
               // Utils::log(errs(), 0 , 1, "Checking dep for parent", parentId, "and
               // child", thisId);
               // This work because we are iterating over sorted tasks (based on the
               // task creation order)
               uniqueParents.insert(parent);
               // TODO: support nested tasks

               // Create edge
               edges.insert({parent, kv.first});
            }
            for(auto& parent : outDepsMap[d])
            {
               const auto parentId = recoverLambdaIdFromUniqueId(parent);
               const auto thisId = recoverLambdaIdFromUniqueId(kv.first);
               // We rely on the fact that inner created tasks have a lower id
               // Utils::log(errs(), 0 , 1, "Checking dep for parent", parentId, "and
               // child", thisId);
               // This work because we are iterating over sorted tasks (based on the
               // task creation order)
               uniqueParents.insert(parent);
               // TODO: support nested tasks

               // Create edge
               edges.insert({parent, kv.first});
            }
            for(auto& parent : inoutDepsMap[d])
            {
               const auto parentId = recoverLambdaIdFromUniqueId(parent);
               const auto thisId = recoverLambdaIdFromUniqueId(kv.first);
               // We rely on the fact that inner created tasks have a lower id
               // Utils::log(errs(), 0 , 1, "Checking dep for parent", parentId, "and
               // child", thisId);
               // This work because we are iterating over sorted tasks (based on the
               // task creation order)
               uniqueParents.insert(parent);
               // TODO: support nested tasks

               // Create edge
               edges.insert({parent, kv.first});
            }
         }

         if(uniqueParents.size())
         {
            Utils::log(errs(), 0, 1,
                       "\nThis task depends on the termination of the "
                       "following parents:");
            Utils::log(errs(), 0, 0, "[");
            for(auto& parent : uniqueParents)
            {
               const std::string formattedLambdaName = 'L' + (parent);
               Utils::log(errs(), 0, 0, formattedLambdaName);
            }
            Utils::log(errs(), 0, 1, "]");
         }
         else
         {
            Utils::log(errs(), 0, 1, " ");
         }

         // Note, these deps tells us nothing about read or write ops. This is task
         // entry dependent For the task_var_accesses generation we assume both read
         // and write ops If a task has a deps type we assume that it will do reads
         // and writes to the dependent variable For this reason we aggregate all the
         // input and output deps in one single list as we don't know they access
         // type

         Utils::log(errs(), 0, 0, "\nInput dependencies:");
         for(auto& d : std::get<0>(kv.second))
         {
            // Dynamically populate the output dependency map
            inDepsMap[d].push_back(kv.first);
            const std::string formatted = '%' + std::to_string(d);
            Utils::log(errs(), 0, 0, formatted);
         }

         Utils::log(errs(), 0, 0, "\nOut dependencies:");
         for(auto& d : std::get<1>(kv.second))
         {
            // Dynamically populate the output dependency map
            outDepsMap[d].push_back(kv.first);
            const std::string formatted = '%' + std::to_string(d);
            Utils::log(errs(), 0, 0, formatted);
         }

         Utils::log(errs(), 0, 0, "\nInputOutput dependencies:");
         for(auto& d : std::get<2>(kv.second))
         {
            // Dynamically populate the output dependency map
            inoutDepsMap[d].push_back(kv.first);
            const std::string formatted = '%' + std::to_string(d);
            Utils::log(errs(), 0, 0, formatted);
         }
      }

      Utils::log(errs(), 0, 1, "\n\n\nFree tasks:");
      Utils::log(errs(), 0, 0, "[");
      for(auto& task : freeTasks)
      {
         // Add node
         nodes.push_back(task.first);

         const std::string formattedLambdaName = 'L' + task.first;
         Utils::log(errs(), 0, 0, formattedLambdaName, "x", task.second);
         Utils::log(errs(), 0, 0, "(Const inputs:");
         Utils::log(errs(), 0, 0, "[");
         for(auto& input : taskEntryConstInputs[task.first])
         {
            const std::string formattedInput = std::to_string(input.first);
            Utils::log(errs(), 0, 0, formattedInput);
         }
         Utils::log(errs(), 0, 0, "] ");
         Utils::log(errs(), 0, 0, "Inputs:");
         Utils::log(errs(), 0, 0, "[");
         for(auto& input : taskEntryInputs[(task.first)])
         {
            const std::string formattedInput = '%' + std::to_string(input.first);
            Utils::log(errs(), 0, 0, formattedInput);
         }
         Utils::log(errs(), 0, 0, "] ");
      }
      Utils::log(errs(), 0, 3, "]");

      Utils::log(errs(), 0, 1,
                 "The following proxies are just base pointer addresses plus some "
                 "offsets\n");
      printPtrs();

      // Create the dot file
      const Generator g;
      const std::vector<std::pair<std::string, std::string>> vecEdges(edges.begin(), edges.end());
      Utils::log(errs(), 0, 1, "");
      const std::string dotPath = outDirName.empty() ? ("graph.dot") : (outDirName.getValue() + "/" + "graph.dot");
      if(Utils::Verbose)
      {
         g.generate_dot(errs(), dotPath, nodes, vecEdges);
      }

      // Populate edge map
      for(auto& p : edges)
      {
         adjMap[p.first].insert(p.second);
      }
   }

   /// Find for each task entry of a @ref llvm::Function block its usage count
   /// frequency
   /// @param instructions A list of @ref llvm::Instruction
   /// @return The frequency count as mapping
   std::unordered_map<std::string, unsigned> ompTaskEntryPointUsesCount(std::vector<Instruction*> const& instructions)
   {
      std::unordered_map<std::string, unsigned> counts;
      for(auto& I : instructions)
      {
         if(CallInst* ompTaskAPICall = dyn_cast<CallInst>(I))
         {
            auto calledFnName = requireDirectCallee(ompTaskAPICall)->getName().str();
            // It's not either a task creation open call or close call
            if(calledFnName == Constants::targetOmpTaskAllocName)
            {
               // Extract the executed lambda reference
               const auto noOperands = ompTaskAPICall->getNumOperands();
               // The task entry lambda arguments is stored in the -2 index
               Value* ompTaskEntryLambdaArg = ompTaskAPICall->getArgOperand(noOperands - 2);
               Utils::log(errs(), 2, 1, "Lambda called name:", ompTaskEntryLambdaArg->getName());
               counts[ompTaskEntryLambdaArg->getName().str()]++;
            }
         }
      }
      return counts;
   }

   /// Parse any load calls to a proxy label for each input of a function
   /// @param F The input function
   void parseFnInputArgProxies(Function& F)
   {
      const auto fnName = F.getName().str();
      for(auto& a : F.args())
      {
         std::string stringBuff;
         raw_string_ostream stream(stringBuff);
         a.print(stream);
         const auto id = retrieveLabelFromChar(stringBuff, '%');
         stringBuff.clear();
         for(auto u : a.users())
         {
            if(auto* loadInst = dyn_cast<LoadInst>(u))
            {
               loadInst->print(stream);
               const auto proxy = parseUnamedNameFromInstruction(stringBuff);
               fnInputProxies[fnName][std::stoi(id)].push_back(std::stoi(proxy));
               fnInputProxiesRefs[fnName][std::stoi(id)].push_back(loadInst);
               stringBuff.clear();
            }
         }
      }
   }

   /// Precompute the same GEP calls and cache them. This will create a key GEP and
   /// all the next GEP calls will be mapped to the key proxy
   /// @param fnInstructions A vector of function instructions
   /// @param fnName A reference to the function name being visited
   void precomputeSameGeps(std::vector<Instruction*>& fnInstructions, std::string const& fnName)
   {
      std::string stringBuff;
      raw_string_ostream stream(stringBuff);
      for(auto* I : fnInstructions)
      {
         if(auto* gep = dyn_cast<GetElementPtrInst>(I))
         {
            gep->print(stream);
            // errs() << "GEP: " << stringBuff
            const auto label = parseUnamedNameFromInstruction(stringBuff);
            const auto gepStr = subStrFromChar(stringBuff, '=', 2);
            if(gepStr.find("kmp") == std::string::npos)
            {
               unsigned labelValue = std::stoi(label);
               if(sameGEPs[fnName].find(gepStr) == sameGEPs[fnName].end())
               {
                  sameGEPs[fnName][gepStr] = {labelValue, {labelValue}};
               }
               else
               {
                  sameGEPs[fnName][gepStr].second.insert(labelValue);
                  sameGEPsReverse[fnName][labelValue] = sameGEPs[fnName][gepStr].first;
               }
            }
            stringBuff.clear();
         }
      }
   }

   /// Create a new function in the module_o for the task call instructions
   /// @param M Input module_o
   /// @param Name The function name
   /// @param FTy Function type
   Function* createTaskCallFn(Module* M, StringRef Name, FunctionType* FTy)
   {
      // Create the function
      Function* F = Function::Create(FTy, Function::InternalLinkage, Name, M);
      F->addFnAttr(Attribute::NoInline);

      // Create entry basic block
      BasicBlock* EntryBB = BasicBlock::Create(M->getContext(), "entry", F);
      IRBuilder<> Builder(EntryBB);

      // Get function arguments
      auto args_it = F->arg_begin();
      Value* taskId = args_it++;  // First arg is task ID
      Value* taskPtr = args_it++; // Second arg is task function pointer

      // Create vector of data arguments
      std::vector<Value*> dataArgs;
      while(args_it != F->arg_end())
      {
         dataArgs.push_back(args_it++);
      }

      // Create the actual function call
      // Cast the task function pointer to the correct type
      std::vector<Type*> taskArgTypes;
      for(unsigned i = 0; i < dataArgs.size(); i++)
      {
         taskArgTypes.push_back(dataArgs[i]->getType());
      }
      FunctionType* TaskTy = FunctionType::get(Type::getVoidTy(M->getContext()), taskArgTypes, false);

      // Create bitcast of function pointer if needed
      Value* castTaskPtr = Builder.CreateBitCast(taskPtr, TaskTy->getPointerTo(), "task.fptr");

      // Create the call to the task function
      Builder.CreateCall(TaskTy, castTaskPtr, dataArgs);

      // Add return void
      Builder.CreateRetVoid();

      return F;
   }

   struct SortedSetValuePtrCmp
   {
      bool operator()(const Value* const a, const Value* const b) const
      {
         std::string buff;
         raw_string_ostream stream(buff);
         a->print(stream);
         const auto aId = std::stoi(retrieveLabelFromChar(buff, '%'));
         buff.clear();
         b->print(stream);
         const auto bId = std::stoi(retrieveLabelFromChar(buff, '%'));
         return aId < bId;
      }
   };

   /// For each task entry @ref llvm::Function creates its duplicates
   /// @param taskEntries The set of task entries in the module_o
   /// @param duplicatedDict The duplicates collector mapping
   void duplicateResources(std::unordered_set<Function*> taskEntries,
                           std::unordered_map<Function*, std::vector<Function*>>& duplicatedDict)
   {
#ifdef DISABLE_RESOURCE_DUPLICATION
      const unsigned duplicationCount = 1;
#else
      const unsigned duplicationCount = resourceDuplicationNum.getValue();
#endif
      if(duplicationCount < 2)
      {
         return;
      }
      for(auto& F : taskEntries)
      {
         std::vector<Function*> duplicated;
         for(unsigned i = 0; i < duplicationCount - 1; i++)
         {
            Utils::log(errs(), 0, 1, "Duplicating task entry point", F->getName());
            Module* M = F->getParent();
            LLVMContext& Context = M->getContext();

            // Creation of transformed function
            std::vector<Type*> argTypes;
            for(auto& A : F->args())
            {
               argTypes.push_back(A.getType());
            }

            FunctionType* NewFuncType = FunctionType::get(F->getReturnType(), argTypes, F->isVarArg());
            const auto newFunctionName = F->getName() + "_duplicated_" + std::to_string(i);
            Function* newFunction = Function::Create(NewFuncType, F->getLinkage(), newFunctionName, M);
            duplicated.push_back(newFunction);

            // Map old arguments to new arguments
            ValueToValueMapTy vMap;
            auto it = newFunction->arg_begin();
            for(auto& oldArg : F->args())
            {
               vMap[&oldArg] = &*it;
               it->setName(oldArg.getName());
               ++it;
            }

            // Clone the function body
            SmallVector<ReturnInst*> returns;
            CloneFunctionInto(newFunction, F, vMap, CloneFunctionChangeType::LocalChangesOnly, returns);

            duplicatedFnToOriginal[newFunction] = F;
         }
         duplicatedDict[F] = duplicated;
      }
   }

   /// Instrument the IR. This call we insert dummy fn calls that mimic the task
   /// graph calls
   /// @param F A @ref llvm::Function
   void instrumentCode(Function& F)
   {
      Module* M = F.getParent();
      LLVMContext& Context = M->getContext();
      const auto fnName = F.getName().str();

      Function* externFn = nullptr;
      std::vector<Value*> externFnInputValues;
      std::unordered_map<std::string, Function*> fnCache;
      std::vector<std::tuple<Instruction*, Function*, std::vector<Value*>>> instructionsToAdd;
      std::unordered_set<Value*> visited;
      unsigned dummyInteger = 0;

      for(BasicBlock& BB : F)
      {
         for(auto it = BB.begin(); it != BB.end(); ++it)
         {
            if(auto* callInst = dyn_cast<CallInst>(&(*it)))
            {
               // Filter out only calls instructions
               if(!callInst)
               {
                  continue;
               }
               auto* called = requireDirectCallee(callInst);

               // Here we look for task alloc and task submit

               // Inject the extern call
               if(called->getName() == Constants::targetOmpTaskSubmissionDepsName ||
                  called->getName() == Constants::targetOmpTaskSubmissionName)
               {
                  auto nextIt = it;
                  std::advance(nextIt, 1);
                  if(!externFn || externFnInputValues.size() < 2)
                  {
                     report_fatal_error("OpenMP task submission without a matching allocation is unsupported");
                  }
                  instructionsToAdd.push_back({&(*nextIt), externFn, externFnInputValues});

                  // Handle resource duplication. Duplicated calls will be inserted with dummy inputs for const values
                  if(visited.count(externFnInputValues[1]) == 0)
                  {
                     // Recover the duplicated fns
                     auto& duplicates = duplicatedResourcesDict[cast<Function>(externFnInputValues[1])];
                     for(auto* F : duplicates)
                     {
#ifdef TWO_CALLS_FOR_DUPLICATION
                        for(uint8_t j = 0; j < 2; j++)
                        {
#endif
                           auto copiedInputs = externFnInputValues;
                           copiedInputs[1] = F;
                           for(unsigned i = 0; i < copiedInputs.size(); i++)
                           {
                              if(copiedInputs[i]->getType()->isIntegerTy())
                              {
                                 copiedInputs[i] = ConstantInt::get(Type::getInt32Ty(Context), dummyInteger++);
                              }
                           }
                           auto nextIt = it;
                           std::advance(nextIt, 1);
                           instructionsToAdd.push_back({&(*nextIt), externFn, copiedInputs});

#ifdef TWO_CALLS_FOR_DUPLICATION
                        }
#endif
                     }
                     visited.insert(externFnInputValues[1]);
                  }
                  continue;
               }
               // Prepare the extern call
               if(called->getName() != Constants::targetOmpTaskAllocName)
               {
                  continue;
               }
               Utils::log(errs(), 0, 1, "\nFound a task alloc. Instrumenting...");
               Utils::log(errs(), 0, 1, "This task alloc has", taskEntryInputsRef[callInst].second.size(), "inputs");

               Value* lambda = taskEntryInputsRef[callInst].first;
               const auto lambdaId = lambdaEntryPointUniqueNameDict[callInst].first;
               Utils::log(errs(), 0, 1, "Unique name:", lambdaId);

               // Get non const inputs. We retain the sorted order with a sorted
               // container. By design the inputs that a task entry will receive are
               // sorted in the struct anon by their labels
               std::set<const Value*, SortedSetValuePtrCmp> uniqueAccesses;
               const auto taskDepAccesses = dependentTasks[lambdaEntryPointUniqueNameDict[callInst].first];
               for(auto n : std::get<0>(taskDepAccesses))
               {
                  assert(ptrToIntSourcesParents[fnName].find(n) != ptrToIntSourcesParents[fnName].end());
                  uniqueAccesses.insert(std::get<3>(ptrToIntSourcesParents[fnName][n]));
               }
               for(auto n : std::get<1>(taskDepAccesses))
               {
                  assert(ptrToIntSourcesParents[fnName].find(n) != ptrToIntSourcesParents[fnName].end());
                  uniqueAccesses.insert(std::get<3>(ptrToIntSourcesParents[fnName][n]));
               }
               for(auto n : std::get<2>(taskDepAccesses))
               {
                  assert(ptrToIntSourcesParents[fnName].find(n) != ptrToIntSourcesParents[fnName].end());
                  uniqueAccesses.insert(std::get<3>(ptrToIntSourcesParents[fnName][n]));
               }

               // Sizes
               const auto inputsRef = taskEntryInputs[lambdaId];
#ifdef VERBOSE
               if(inputsRef.size())
               {
                  Utils::log(errs(), 2, 1, "inputs for", lambdaId);
                  for(auto i : inputsRef)
                  {
                     Utils::log(errs(), 2, 1, i.first);
                  }
               }
#endif
               const auto constInputsRef = taskEntryConstInputs[lambdaId];
               const auto numConstInputs = constInputsRef.size();
               const auto numInputs = uniqueAccesses.size() + 1; // +1 as we add the lambda entry point pointer
               Utils::log(errs(), 0, 1, "Input size:", numInputs);
               Utils::log(errs(), 0, 1, "Const input size:", numConstInputs);

               std::unordered_map<unsigned, unsigned> inputsRefWeights;
               unsigned weight = numInputs;
               // Input ptr of interest are located at the end
               auto endIt = inputsRef.rbegin();
               std::advance(endIt, numInputs - 1);
               for(auto it = inputsRef.rbegin(); it != endIt; it++)
               {
                  inputsRefWeights[it->first] = weight--;
#ifdef VERBOSE
                  Utils::log(errs(), 2, 1, "weight for", it->first, "=", inputsRefWeights[it->first]);
#endif
               }

               // Type for const int32 inputs
               Type* int32Type = Type::getInt32Ty(Context);
               // Type for generic inputs references
               PointerType* ptrType = PointerType::get(Context, 0); // opaque pointer
               // Generate the argument list types
               std::vector<Type*> vecTypes = {int32Type}; // for the lambda entry hash (node id)
               for(unsigned i = 0; i < numInputs; ++i)
               {
                  vecTypes.push_back(ptrType);
               }
               for(unsigned i = 0; i < numConstInputs; ++i)
               {
                  vecTypes.push_back(int32Type);
               }

               auto const taskId = getOmpTaskEntryId(lambda->getName().str());
               if(fnCache.find(taskId) == fnCache.end())
               {
                  // Define the external fn call as void
                  FunctionType* externFnType = FunctionType::get(Type::getVoidTy(Context), vecTypes, false);
                  externFn = createTaskCallFn(M, Constants::injectedTaskNamePreifx + taskId + "_call", externFnType);
                  fnCache[taskId] = externFn;
               }
               else
               {
                  externFn = fnCache[taskId];
               }

               // Cache the arg list types
               taskEntryPointArgTypesList[lambda] = vecTypes;

               Utils::log(errs(), 0, 1, "Unique accesses size:", uniqueAccesses.size());
               std::unordered_map<Value const*, Value const*> accessToSource;
               for(auto a : uniqueAccesses)
               {
                  if(Bambu::OpenMP::Task::Utils::Verbose)
                  {
                     a->print(errs());
                  }
                  Utils::log(errs(), 2, 1, "");
                  if(auto* loadInst = dyn_cast<LoadInst>(a))
                  {
                     Utils::log(errs(), 4, 0, "load instruction, operand: ");
                     Value const* argOp = nullptr;
                     if(isa<LoadInst>(loadInst->getPointerOperand()))
                     {
                        argOp = argFromLoad(cast<LoadInst>(loadInst->getPointerOperand()));
                     }
                     else if(isa<Argument>(loadInst->getPointerOperand()))
                     {
                        argOp = loadInst->getPointerOperand();
                     }
                     else
                     {
                        report_fatal_error("not impl");
                     }
                     if(Bambu::OpenMP::Task::Utils::Verbose)
                     {
                        argOp->print(errs());
                     }
                     Utils::log(errs(), 0, 1, "");

                     accessToSource[a] = argOp;
                  }
                  else if(auto* gep = dyn_cast<GetElementPtrInst>(a))
                  {
                     Utils::log(errs(), 4, 0, "gep instruction, operand: ");
                     if(Bambu::OpenMP::Task::Utils::Verbose)
                     {
                        gep->getPointerOperand()->print(errs());
                     }
                     auto* fromArg = argFromLoad(loadFromGep(gep));
                     Utils::log(errs(), 0, 1, "");
                     Utils::log(errs(), 4, 0, "from arg ");
                     if(Bambu::OpenMP::Task::Utils::Verbose)
                     {
                        fromArg->print(errs());
                     }
                     Utils::log(errs(), 0, 1, "");

                     accessToSource[a] = fromArg;
                  }
                  else if(auto* arg = dyn_cast<Argument>(a))
                  {
                     Utils::log(errs(), 4, 0, "argument value: ");
                     if(Bambu::OpenMP::Task::Utils::Verbose)
                     {
                        arg->print(errs());
                     }
                     Utils::log(errs(), 0, 1, "");

                     accessToSource[a] = arg;
                  }
                  else
                  {
                     report_fatal_error("not impl");
                  }
               }
               // Build the instrumented fn call arguments
               std::vector<Value*> vecValues = {
                   ConstantInt::get(int32Type, lambdaEntryPointUniqueNameDict[callInst].second), lambda};
               std::vector<Value*> accessValues;
               for(auto& V : uniqueAccesses)
               {
                  accessValues.push_back(const_cast<Value*>(V));
               }
               std::sort(accessValues.begin(), accessValues.end(), [&](Value* a, Value* b) {
                  assert(accessToSource.count(a) > 0 && accessToSource.count(b) > 0);
                  // Get the argument
                  auto* argA = accessToSource[a];
                  auto* argB = accessToSource[b];

                  assert(isa<Argument>(argA));
                  assert(isa<Argument>(argB));

                  std::string stringBuff;
                  raw_string_ostream stream(stringBuff);
                  argA->print(stream);
                  const auto labelA = static_cast<unsigned>(std::stoul(retrieveLabelFromChar(stringBuff, '%')));
                  stringBuff.clear();
                  argB->print(stream);
                  const auto labelB = static_cast<unsigned>(std::stoul(retrieveLabelFromChar(stringBuff, '%')));

                  return inputsRefWeights[labelA] < inputsRefWeights[labelB];
               });

               vecValues.insert(vecValues.end(), accessValues.begin(), accessValues.end());
               for(unsigned i = 0; i < numConstInputs; ++i)
               {
                  vecValues.push_back(ConstantInt::get(int32Type, constInputsRef[i].first));
               }
               assert(vecValues.size() == vecTypes.size());
               // Cache for later
               externFnInputValues = vecValues;
            }
         }
      }

      for(auto& [insertPoint, called, args] : instructionsToAdd)
      {
         IRBuilder<> builder(insertPoint);
         builder.CreateCall(called, args);
      }

#ifdef REMOVE_OUTLINED_GEP
      // Remove unused gep now as we have base ptr and this will be gepped inside the tasks
      for(auto& BB : F)
      {
         for(auto& I : BB)
         {
            if(auto* gep = dyn_cast<GetElementPtrInst>(&I))
            {
               Value* ptr = gep->getPointerOperand();
               gep->replaceAllUsesWith(ptr);
            }
         }
      }
#endif
   }

   /// Collects in order the instruction to be erased from parents. The collection
   /// follows a depth first search approach as instructions with no uses are
   /// deleted first
   /// @param V the initial value
   /// @param deleteArr a reference to the buffer where the instructions to be
   /// erased are cached
   void collectInstructionsToDelete(Value* V, std::vector<Instruction*>& deleteArr)
   {
      Instruction* I = dyn_cast<Instruction>(V);
      if(I)
      {
         for(auto* U : I->users())
         {
            collectInstructionsToDelete(U, deleteArr);
         }
         deleteArr.push_back(I);
      }
   }

   /// Remove from IR the calls to the openMP runtime APIs
   /// @param F the function being visited
   void removeRunTimeCalls(Function& F)
   {
      std::vector<Instruction*> deleteArr;
      for(BasicBlock& BB : F)
      {
         for(auto it = BB.begin(); it != BB.end(); ++it)
         {
            if(auto* callInst = dyn_cast<CallInst>(&(*it)))
            {
               // Filter out only calls instructions
               if(!callInst)
               {
                  continue;
               }
               auto* called = requireDirectCallee(callInst);

               if(called->getName() == Constants::targetOmpTaskAllocName ||
                  called->getName() == Constants::targetOmpEndSingle ||
                  called->getName() == Constants::targetOmpBarrierName)
               {
                  collectInstructionsToDelete(callInst, deleteArr);
               }
               if(called->getName() == Constants::targetOmpSingle)
               {
                  IRBuilder<> builder(&(*it));
                  Value* zeroValue = builder.getInt32(1);
                  callInst->replaceAllUsesWith(zeroValue);
                  collectInstructionsToDelete(callInst, deleteArr);
               }
            }
         }
      }

      for(auto* I : deleteArr)
      {
         // errs() << "Deleting ";
         // I->print(errs());
         // errs() << "\n\n";
         I->eraseFromParent();
      }
   }

   /// Remove an instruction with the given type, checker function and name. This
   /// will delete all its users
   /// @tparam InstructionType the @ref llvm::Instruction type
   /// @tparam Callable the checker function type
   /// @param F the function being visited
   /// @param checker the checker function
   /// @param name the target name for the checker function
   template <typename InstructionType, typename Callable>
   void removeInstructionsWithName(Function& F, Callable cheker, const std::string& name)
   {
      std::vector<Instruction*> deleteArr;
      for(BasicBlock& BB : F)
      {
         for(auto it = BB.begin(); it != BB.end(); ++it)
         {
            if(auto* I = dyn_cast<InstructionType>(&(*it)))
            {
               if(cheker(I, name))
               {
                  collectInstructionsToDelete(I, deleteArr);
               }
            }
         }
      }

      for(auto* I : deleteArr)
      {
         // errs() << "Deleting ";
         // I->print(errs());
         // errs() << "\n\n";
         I->eraseFromParent();
      }
   }

   /// Removed all the instructions with no uses (dce). Not recursive
   /// @param F the function being visited
   void removeInstructionsWithNoUses(Function& F)
   {
      // TODO: make this recursive
      std::vector<Instruction*> deleteArr;
      for(BasicBlock& BB : F)
      {
         for(auto it = BB.begin(); it != BB.end(); ++it)
         {
            // collectInstructionsToDelete(&*it, deleteArr);
            if(it->hasNUses(0) && !it->getType()->isVoidTy() && !isa<PtrToIntInst>(*it))
            {
               // Check if it is a call instruction
               if(auto* callInst = dyn_cast<CallInst>(&*it))
               {
                  // Don't remove calls that are not omp runtime
                  if(requireDirectCallee(callInst)->getName().find("kmp") != std::string::npos)
                  {
                     deleteArr.push_back(&(*it));
                  }
               }
               else
               {
                  deleteArr.push_back(&(*it));
               }
            }
         }
      }
      for(auto* I : deleteArr)
      {
         I->eraseFromParent();
      }
   }

   /// @brief Adjusts the offsets of pointer-to-integer source parents for a given function.
   ///
   /// This function processes the `ptrToIntSourcesParents` map for the given function `F` and adjusts
   /// the offsets of the pointer-to-integer source parents. It first recovers the base offsets and then
   /// adjusts the offsets for other entries based on the recovered base offsets.
   /// This is needed as with opaque ptrs we have only bytes information within GEPs and we miss the index a given GEP
   /// is accessing from an array.
   ///
   /// @param F The function for which the pointer-to-integer source parents' offsets need to be adjusted.
   void adjustPtrToIntSourcesParentsOffsets(Function& F)
   {
      const auto fnName = F.getName();
      std::unordered_map<unsigned, unsigned> offsetMap;
      for(auto it = ptrToIntSourcesParents[fnName.str()].begin(); it != ptrToIntSourcesParents[fnName.str()].end();
          it++)
      {
         if(std::get<0>(it->second) > 0)
         {
            const auto target = std::get<1>(it->second);
            if(offsetMap.find(target) == offsetMap.end())
            {
               offsetMap[target] = std::get<2>(it->second);
            }
            std::get<2>(it->second) = std::get<2>(it->second) / offsetMap[std::get<1>(it->second)];
         }
      }
   }

   /// This method implements what the pass does
   /// @param F The @ref llvm:Function to visit
   /// @param entryPointInputs The current task entry point lambda inputs arguments
   /// with labels referring to those present in the omp_outlined fn (original
   /// labels). Null pointer if this visitor call is called when visiting the
   /// omp_outlined itself
   /// @param taskHash A reference to the task hash generator
   /// @param topo A topological builder object
   /// @param cleanDeps A flag controlling if the known dependency structures have
   /// to be cleaned or not for a new parallel task region analysis
   void visitor(Function& F, std::set<unsigned> const* entryPointInputs, unsigned& taskHash, TopoGraphGenerator& topo,
                bool clearDeps)
   {
#ifdef ENABLE_RECURSIVE_SEARCH
      // Clear our dependentTasks holder for now as it will be only printed to
      // standard error
      if(clearDeps)
      {
         dependentTasks.clear();
         freeTasks.clear();
      }
#else
      dependentTasks.clear();
      freeTasks.clear();
#endif

// Control the deps log. We log only if this visitor call is not a recursive
// call but emitted from an omp_outlined (parallel task region entry point)
#ifdef ENABLE_RECURSIVE_SEARCH
      const auto logDeps = clearDeps;
#endif
      const auto fnName = F.getName().str();

      Utils::log(errs(), 0, 1, "Function name:", fnName, "#args:", F.arg_size());

      std::vector<Instruction*> allFnInstructions;
      for(auto& BB : F)
      {
         for(auto& I : BB)
         {
            allFnInstructions.push_back(&I);
         }
      }

      // For GEPs on the depend struct allocations inject the GEP with offset 0
      addBaseIdxGep(F);

      // Precompute target `ptrtoint` source ptrs' parent proxies
      // Utils::measure_void("precomputePtrToIntSourcesParents", precomputePtrToIntSourcesParents, allFnInstructions,
      //                    fnName);
      precomputePtrToIntSourcesParents(allFnInstructions, fnName);

      adjustPtrToIntSourcesParentsOffsets(F);

      // Precompute common GEP, this may be done with a separate pass but with GEP
      // allocated in different blocks this cannot be done. For our purpose we can
      // doing this computation by hand without modifying the IR representation as
      // we have only to extract symbolic informations.
      // Utils::measure_void("precomputeSameGeps", precomputeSameGeps, allFnInstructions, fnName);
      precomputeSameGeps(allFnInstructions, fnName);

      // Precompute dependencies accesses for the current function
      // const auto precomputedDeps =
      //    Utils::measure_not_void<>("storeGEPDepStructAccesses", storeGEPDepStructAccesses, allFnInstructions,
      //                              std::unordered_map<unsigned, unsigned>{}, fnName);
      std::unordered_map<std::string, std::pair<unsigned, unsigned>> precomputedDeps;
      storeGEPDepStructAccesses(allFnInstructions, {}, fnName, precomputedDeps);
      Utils::log(errs(), 0, 1, "Precomputed deps size:", precomputedDeps.size());

      // Parse any fn input arg proxy
      // Utils::measure_void("parseFnInputArgProxies", parseFnInputArgProxies, F);
      parseFnInputArgProxies(F);

      // Iterate over basic blocks in the function
      for(BasicBlock& BB : F)
      {
         // For each basic block retrieves the instructions sequence defining a task
         // creation and submission.  `clearDeps` is true when the visitor is called
         // at omp_outlined level. In this case we don't want to include all the
         // instructions before that task alloc.  In case this visitor is called in a
         // nested call we want to include the informations before the task alloc as
         // they contains data for task entry inputs and dependencies sources.
         std::vector<std::vector<Instruction*>> parsedInstructions;
         taskWithDepsInstructions(BB, true, parsedInstructions);
         Utils::log(errs(), 0, 1, "\nBasic Block task with dependentTasks #:", parsedInstructions.size());

         // Recover all the block instructions
         std::vector<Instruction*> blockInstructions;
         for(auto& i : parsedInstructions)
         {
            blockInstructions.insert(blockInstructions.end(), i.begin(), i.end());
         }

         const auto taskEntryPointCount = ompTaskEntryPointUsesCount(blockInstructions);

         for(auto& i : parsedInstructions)
         {
            // for (auto &j : i) {
            //   errs() << *j << "\n";
            // }
            ompDependenciesFinder(fnName, i, blockInstructions, Constants::targetOmpDepsStructName, entryPointInputs,
                                  precomputedDeps, taskEntryPointCount, taskHash);
         }
      }

      // Remove inline attribute
      if(F.hasFnAttribute(Attribute::AlwaysInline))
      {
         F.removeFnAttr(Attribute::AlwaysInline);
      }
      F.addFnAttr(Attribute::NoInline);

#ifdef ENABLE_RECURSIVE_SEARCH
      if(logDeps)
      {
#endif
         // Build the task graph
         Nodes nodes;
         AdjMap adjMap;
         std::unordered_map<std::string, std::pair<Inputs, ConstInputs>> nodesInputs;
         generateDepsGraph(fnName, nodes, adjMap);
         instrumentCode(F);
         removeRunTimeCalls(F);
         removeInstructionsWithName<AllocaInst>(F, AllocInstIsArrayOfSpecificStruct,
                                                Constants::targetOmpDepsStructName);
         removeInstructionsWithNoUses(F);
         promoteOmpOutlinedBaseAddr(F);

         // Generate xml for this outlined module_o
         {
            // Give an index to the inputs
            std::map<InputId, InputIdIndex> topModuleInputMap;
            unsigned inputIndex = 0;
            for(auto& [fnName, m] : ptrToIntSourcesParents)
            {
               for(auto& [a, b] : m)
               {
                  // Set as top inputs all those base addresses or fake base addresses (base addresses but saved as they
                  // are not)
                  if(std::get<1>(b) == NA_VALUE || neverReferencedBaseInputs.count(a))
                  {
                     topModuleInputMap[a] = inputIndex++;
                  }
               }
            }

            // Populate node inputs
            for(auto n : nodes)
            {
               Inputs inputList;
               ConstInputs constInputList;
               // ptr inputs
               for(auto& input : taskEntryInputs[n])
               {
                  if(ptrToIntSourcesParents[fnName].find(input.first) != ptrToIntSourcesParents[fnName].end() &&
                     std::get<1>(ptrToIntSourcesParents[fnName][input.first]) > 0)
                  {
                     if(std::get<1>(ptrToIntSourcesParents[fnName][input.first]) == NA_VALUE)
                     {
                        continue;
                     }
                     inputList.push_back({InputType::Offset,
                                          std::get<1>(ptrToIntSourcesParents[fnName][input.first]) > 0 ?
                                              std::get<1>(ptrToIntSourcesParents[fnName][input.first]) :
                                              input.first,
                                          std::get<1>(ptrToIntSourcesParents[fnName][input.first]) > 0 ?
                                              std::get<2>(ptrToIntSourcesParents[fnName][input.first]) :
                                              0});
                  }
                  else
                  {
                     inputList.push_back({InputType::Base, input.first, 0});
                  }
               }
               // const inputs
               for(auto& input : taskEntryConstInputs[n])
               {
                  constInputList.push_back(input.first);
               }
               nodesInputs[n] = {inputList, constInputList};
            }

            std::unordered_set<unsigned> lambdaTypes;
            for(auto& n : nodes)
            {
               lambdaTypes.insert(Utils::id_num_from_id(n));
            }
            std::unordered_map<unsigned, unsigned> resources;
            for(auto& t : lambdaTypes)
            {
               // TODO: consider use or remove this
               resources[t] = 1;
            }
            topo.setGraphParams(adjMap, nodes, resources, nodesInputs, topModuleInputMap);
         }

#ifdef ENABLE_RECURSIVE_SEARCH
      }
#endif
   }

   /// Add an attribute from a @ref Function
   /// @tparam AttributeType the @ref llvm::Attribute
   /// @param F the function being visited
   /// @param A the attribute to be added
   template <typename AttributeType>
   void addFnAttribute(Function& F, AttributeType A)
   {
      if(!F.hasFnAttribute(A))
      {
         F.addFnAttr(A);
      }
   }

   /// Remove an attribute from a @ref Function
   /// @tparam AttributeType the @ref llvm::Attribute
   /// @param F the function being visited
   /// @param A the attribute to be removed
   template <typename AttributeType>
   void removeFnAttribute(Function& F, AttributeType A)
   {
      if(F.hasFnAttribute(A))
      {
         F.removeFnAttr(A);
      }
   }

   /// Promotes fork call parameters removing a pointer layer
   /// @param F The omp outlined caller
   void promoteForkCallBaseAddr(Function& F)
   {
      Utils::log(errs(), 0, 1, "Promoting fork call params ptrs of", F.getName());
      // Check that fn is correct
      bool found = false;
      CallInst* forkCall = nullptr;
      for(auto& BB : F)
      {
         for(auto& I : BB)
         {
            if(auto* callInst = dyn_cast<CallInst>(&I))
            {
               if(requireDirectCallee(callInst)->getName().find(Constants::targetOmpForkCall) != std::string::npos)
               {
                  forkCall = callInst;
                  found = true;
                  break;
               }
            }
         }
      }

      assert(found && forkCall && "Input function is not the outlined caller");

      const auto numParams = forkCall->getNumOperands() - 4;
      // First operand starts at 3
      std::set<Value*> paramsRef;
      for(unsigned i = 3; i < 3 + numParams; i++)
      {
         paramsRef.insert(forkCall->getOperand(i));
      }

      std::vector<Instruction*> storesToRemove;
      for(auto& BB : F)
      {
         for(auto& I : BB)
         {
            if(auto* storeInst = dyn_cast<StoreInst>(&I))
            {
               if(paramsRef.find(storeInst->getPointerOperand()) != paramsRef.end())
               {
                  auto it = paramsRef.find(storeInst->getPointerOperand());
                  Utils::log(errs(), 2, 0, "replacing ");
                  if(Bambu::OpenMP::Task::Utils::Verbose)
                  {
                     storeInst->getPointerOperand()->print(errs());
                  }
                  Utils::log(errs(), 0, 0, " with ");
                  if(Bambu::OpenMP::Task::Utils::Verbose)
                  {
                     storeInst->getValueOperand()->print(errs());
                  }
                  Utils::log(errs(), 0, 1, "");
                  (*it)->replaceAllUsesWith(storeInst->getValueOperand());
                  storesToRemove.push_back(storeInst);
               }
            }
         }
      }

      for(auto* I : storesToRemove)
      {
         I->eraseFromParent();
      }
   }

   /// Transform an openMP entry point function. Switch inputs from `struct.anon`
   /// to function input arguments as injected by code instrumentation.
   /// TODO: Currently this transform does not work if a shared variable is being
   /// used inside the lambda body and it's not present in the dependencies list.
   /// @param F The input function
   /// @param newFnNames The updated function names
   /// @param argsDict The input types argument dictionary
   Function* transformTaskEntryFn(Function& F, std::vector<std::string>& newFnNames,
                                  std::unordered_map<Value*, std::vector<Type*>> const& argsDict)
   {
      Utils::log(errs(), 0, 1, "Transforming task entry point", F.getName());
      Module* M = F.getParent();
      LLVMContext& Context = M->getContext();

      assert(argsDict.find(&F) != argsDict.end());
      const std::vector<Type*>& newTypes = argsDict.at(&F);
      // We skip the first two elements as they are an unique id for the call and
      // the lambda ptr itself
      const uint8_t newTypesStartIndex = 2;

      // Here we recognize some ptrs + some integers
      bool isStillPtr = true;
      unsigned newTypesPtrSize = 0;
      for(unsigned i = 2; i < newTypes.size(); ++i)
      {
         if(isStillPtr && newTypes[i]->isPointerTy())
         {
            newTypesPtrSize++;
         }
         else if(isStillPtr && !newTypes[i]->isPointerTy())
         {
            isStillPtr = false;
         }
         else if(!isStillPtr)
         {
            assert(newTypes[i]->isIntegerTy());
         }
      }

      Utils::log(errs(), 2, 1, "New func ptr type size:", newTypesPtrSize);

      // Creation of transformed function
      std::vector<Type*> argTypes;
      const auto argsOffset = F.arg_size();
      for(auto& A : F.args())
      {
         argTypes.push_back(A.getType());
      }
      for(unsigned i = newTypesStartIndex; i < newTypes.size(); i++)
      {
         argTypes.push_back(newTypes[i]);
      }

      FunctionType* NewFuncType = FunctionType::get(F.getReturnType(), argTypes, F.isVarArg());
      const auto newFunctionName = F.getName() + "_transformed";
      Function* newFunction = Function::Create(NewFuncType, F.getLinkage(), newFunctionName, M);
      newFnNames.push_back(newFunctionName.str());

      // Map old arguments to new arguments
      ValueToValueMapTy vMap;
      auto it = newFunction->arg_begin();
      for(auto& oldArg : F.args())
      {
         vMap[&oldArg] = &*it;
         it->setName(oldArg.getName());
         ++it;
      }
      for(unsigned i = newTypesStartIndex; i < newTypes.size(); i++, it++)
      {
         it->setName(Twine("injected" + std::to_string(i - newTypesStartIndex + 1)));
      }

      // Clone the function body
      SmallVector<ReturnInst*> returns;
      CloneFunctionInto(newFunction, &F, vMap, CloneFunctionChangeType::LocalChangesOnly, returns);

      // Define order map to simulate domination
      std::unordered_map<User const*, unsigned> orderMap;
      unsigned order = 0;
      for(auto& BB : *newFunction)
      {
         for(auto& I : BB)
         {
            orderMap[&I] = order++;
         }
      }

      // Start from the input struct. This is where the entry function inputs are referenced
      const auto lastArg = newFunction->getArg(1);
      std::vector<Use*> lastArgUses;

      const auto makeUsesSorted = [&orderMap](std::vector<Use*>& dest, auto fromIt, auto endIt) {
         while(fromIt != endIt)
         {
            dest.push_back(&(*fromIt));
            fromIt++;
         }
         std::sort(dest.begin(), dest.end(), [&orderMap](Use const* a, Use const* b) {
            assert(orderMap.find(a->getUser()) != orderMap.end() && orderMap.find(a->getUser()) != orderMap.end() &&
                   "Can not find in map");
            return orderMap[a->getUser()] < orderMap[b->getUser()];
         });
      };

      std::vector<Use*> sortedLastArgUses;
      makeUsesSorted(sortedLastArgUses, lastArg->use_begin(), lastArg->use_end());
#ifdef VERBOSE
      {
         Utils::log(errs(), 2, 1, "Sorted order of last input arg uses");
         std::string stringBuff;
         raw_string_ostream stream(stringBuff);
         for(auto& e : sortedLastArgUses)
         {
            stringBuff.clear();
            e->getUser()->print(stream);
            Utils::log(errs(), 4, 1, stringBuff);
         }
      }
#endif
      // We have to recover some informations about geps regarding the const inputs inside the omp struct:
      // - base offse
      // - delta offset between geps for the next constant input
      std::set<int64_t> allGepOffsets;
      for(auto& e : sortedLastArgUses)
      {
         if(auto* gep = dyn_cast<GetElementPtrInst>(e->getUser()))
         {
            const auto offsetVal = gep->getOperand(gep->getNumOperands() - 1);
            if(auto* CI = dyn_cast<llvm::ConstantInt>(offsetVal))
            {
               int64_t offset = CI->getSExtValue();
               allGepOffsets.insert(offset);
            }
         }
      }
      assert(allGepOffsets.size() > 0);
      Utils::log(errs(), 2, 1, "Base gep for const values in input struct is", *allGepOffsets.begin());
      unsigned gepOffsetsDelta = 1;
      for(auto it = allGepOffsets.begin(); it != allGepOffsets.end(); it++)
      {
         auto itNext = it;
         std::advance(itNext, 1);
         if(itNext != allGepOffsets.end())
         {
            const auto deltaOffset = *itNext - *it;
            if(gepOffsetsDelta == 1)
            {
               gepOffsetsDelta = deltaOffset;
            }
            else
            {
               assert(gepOffsetsDelta == deltaOffset);
            }
         }
      }
      Utils::log(errs(), 2, 1, "Offset delta is", gepOffsetsDelta);

      for(auto& u1 : sortedLastArgUses)
      {
         if(auto* gepUser = dyn_cast<GetElementPtrInst>(u1->getUser()))
         {
            const auto offsetVal = gepUser->getOperand(gepUser->getNumOperands() - 1);
            int64_t offset = 0;
            if(auto* CI = dyn_cast<llvm::ConstantInt>(offsetVal))
            {
               offset = CI->getSExtValue();
            }
            else
            {
               report_fatal_error("can not retrieve the offset");
            }
            assert(allGepOffsets.find(offset) != allGepOffsets.end());
            unsigned constArgIndex = (offset - *allGepOffsets.begin()) / gepOffsetsDelta;
            Utils::log(errs(), 2, 1, "Retrieved const arg index is", constArgIndex);
            std::vector<Use*> sortedGepUserUses;
            makeUsesSorted(sortedGepUserUses, gepUser->use_begin(), gepUser->use_end());
#ifdef VERBOSE
            {
               Utils::log(errs(), 2, 1, "Sorted order of gep user uses");
               std::string stringBuff;
               raw_string_ostream stream(stringBuff);
               for(auto& e : sortedGepUserUses)
               {
                  stringBuff.clear();
                  e->getUser()->print(stream);
                  Utils::log(errs(), 4, 1, stringBuff);
               }
            }
#endif
            bool found = false;
            for(auto& u2 : sortedGepUserUses)
            {
               if(auto* loadUser2 = dyn_cast<LoadInst>(u2->getUser()))
               {
                  found = true;
                  Value* V = newFunction->getArg(argsOffset + newTypesPtrSize + constArgIndex);
#ifdef VERBOSE

                  {
                     std::string stringBuff1;
                     raw_string_ostream stream1(stringBuff1);
                     std::string stringBuff2;
                     raw_string_ostream stream2(stringBuff2);
                     loadUser2->print(stream1);
                     V->print(stream2);
                     Utils::log(errs(), 2, 1, "Replacing", stringBuff1, "with", stringBuff2);
                  }
#endif
                  loadUser2->replaceAllUsesWith(V);
               }
            }
            // Increment the counter after all the uses of GEP. They can be loaded multiple times
            // but these loads have the same injected value
            assert(found && "expect to use the injected constant");
         }
      }

      // Reset the counter
      unsigned gepToReplaceCounter = 0;
      // Parse first ptr
      Utils::log(errs(), 2, 1, "Replacing first pointer");
      for(auto& u1 : sortedLastArgUses)
      {
         if(auto* load1 = dyn_cast<LoadInst>(u1->getUser()))
         {
            std::vector<Use*> load1SortedUses;
            makeUsesSorted(load1SortedUses, load1->use_begin(), load1->use_end());
#ifdef VERBOSE
            {
               Utils::log(errs(), 2, 1, "Sorted order of load1 uses");
               std::string stringBuff;
               raw_string_ostream stream(stringBuff);
               for(auto& e : load1SortedUses)
               {
                  stringBuff.clear();
                  e->getUser()->print(stream);
                  Utils::log(errs(), 4, 1, stringBuff);
               }
            }
#endif
            for(auto* u2 : load1SortedUses)
            {
               if(auto* load2 = dyn_cast<LoadInst>(u2->getUser()))
               {
                  std::vector<Use*> load2SortedUses;
                  makeUsesSorted(load2SortedUses, load2->use_begin(), load2->use_end());
#ifdef VERBOSE
                  {
                     Utils::log(errs(), 2, 1, "Sorted order of load2 uses");
                     std::string stringBuff;
                     raw_string_ostream stream(stringBuff);
                     for(auto& e : load2SortedUses)
                     {
                        stringBuff.clear();
                        e->getUser()->print(stream);
                        Utils::log(errs(), 4, 1, stringBuff);
                     }
                  }
#endif
                  bool found = false;
                  for(auto* u3 : load2SortedUses)
                  {
                     if(auto* store1 = dyn_cast<StoreInst>(u3->getUser()))
                     {
#ifdef REMOVE_OUTLINED_GEP
                        found = true;
                        assert(gepToReplaceCounter < newTypesPtrSize);
                        Value* V = newFunction->getArg(argsOffset + gepToReplaceCounter);
#ifdef VERBOSE

                        {
                           std::string stringBuff1;
                           raw_string_ostream stream1(stringBuff1);
                           std::string stringBuff2;
                           raw_string_ostream stream2(stringBuff2);
                           store1->print(stream1);
                           V->print(stream2);
                           Utils::log(errs(), 2, 1, "Replacing", stringBuff1, "with", stringBuff2);
                        }
#endif
                        store1->replaceAllUsesWith(V);
#else
#error "Not impl. Please define REMOVE_OUTLINED_GEP"
#endif
                     }
                     else if(auto* load3 = dyn_cast<LoadInst>(u3->getUser()))
                     {
#ifdef REMOVE_OUTLINED_GEP
                        found = true;
                        assert(gepToReplaceCounter < newTypesPtrSize);
                        Value* V = newFunction->getArg(argsOffset + gepToReplaceCounter);
#ifdef VERBOSE

                        {
                           std::string stringBuff1;
                           raw_string_ostream stream1(stringBuff1);
                           std::string stringBuff2;
                           raw_string_ostream stream2(stringBuff2);
                           load3->print(stream1);
                           V->print(stream2);
                           Utils::log(errs(), 2, 1, "Replacing", stringBuff1, "with", stringBuff2);
                        }
#endif
                        load3->replaceAllUsesWith(V);
#else
                        std::vector<Use*> load3SortedUses;
                        makeUsesSorted(load3SortedUses, load3->use_begin(), load3->use_end());
                        {
#ifdef VERBOSE
                           Utils::log(errs(), 2, 1, "Sorted order of load3 uses");
                           std::string stringBuff;
                           raw_string_ostream stream(stringBuff);
#endif
                           unsigned gepCount = 0;
                           for(auto& e : load3SortedUses)
                           {
#ifdef VERBOSE
                              stringBuff.clear();
                              e->getUser()->print(stream);
                              Utils::log(errs(), 4, 1, stringBuff);
#endif
                              if(auto* isGep = dyn_cast<GetElementPtrInst>(e->getUser()))
                              {
                                 gepCount++;
                              }
                           }
                           assert(gepCount < 2 &&
                                  "omp_struct to ptr promotion can not be performed as multiple gep on the same load "
                                  "may reference different inputs. Please try to define REMOVE_OUTLINED_GEP");
                        }
                        for(auto* u4 : load3SortedUses)
                        {
                           if(auto* gep = dyn_cast<GetElementPtrInst>(u4->getUser()))
                           {
                              found = true;
                              assert(gepToReplaceCounter < newTypesPtrSize);
                              Value* V = newFunction->getArg(argsOffset + gepToReplaceCounter);
#ifdef VERBOSE
                              {
                                 std::string stringBuff1;
                                 raw_string_ostream stream1(stringBuff1);
                                 std::string stringBuff2;
                                 raw_string_ostream stream2(stringBuff2);
                                 gep->print(stream1);
                                 V->print(stream2);
                                 Utils::log(errs(), 2, 1, "Replacing", stringBuff1, "with", stringBuff2);
                              }
#endif
                              gep->replaceAllUsesWith(V);
                           }
                        }
#endif
                     }
                  }
                  assert(found && "expected to found at least one node");
                  gepToReplaceCounter++;
               }
            }

            // We do look only for 1 load inst
            // break;
         }
      }

      assert(gepToReplaceCounter == 1U);

      Utils::log(errs(), 2, 1, "Replacing other pointers");
      /// Parse other ptrs. This handles multiple inputs when they are different ptr:
      /// \code{.cpp}
      /// int* d1;
      /// int* d2;
      /// fn(d1, d2)
      /// \endcode
      for(auto& u1 : sortedLastArgUses)
      {
         if(auto* load1 = dyn_cast<LoadInst>(u1->getUser()))
         {
            std::vector<Use*> load1SortedUses;
            makeUsesSorted(load1SortedUses, load1->use_begin(), load1->use_end());
#ifdef VERBOSE
            {
               Utils::log(errs(), 2, 1, "Sorted order of load1 uses");
               std::string stringBuff;
               raw_string_ostream stream(stringBuff);
               for(auto& e : load1SortedUses)
               {
                  stringBuff.clear();
                  e->getUser()->print(stream);
                  Utils::log(errs(), 4, 1, stringBuff);
               }
            }
#endif
            unsigned gepCount = 0;
            for(auto* I : load1SortedUses)
            {
               if(auto* gep1 = dyn_cast<GetElementPtrInst>(I->getUser()))
               {
                  gepCount++;
               }
            }
            // TODO: if multiple gep are present we don't have to break from the next loop
            for(auto* u2 : load1SortedUses)
            {
               if(auto* gep1 = dyn_cast<GetElementPtrInst>(u2->getUser()))
               {
                  std::vector<Use*> gep1SortedUses;
                  const auto* gepOffsetPtr = gep1->getOperand(gep1->getNumOperands() - 1);
                  unsigned gepOffset = 0;
                  if(auto* constInt = dyn_cast<ConstantInt>(gepOffsetPtr))
                  {
                     // We found a ConstantInt index!
                     gepOffset = constInt->getSExtValue();
                  }
                  else
                  {
                     report_fatal_error("Can not retrive gep offset");
                  }
                  makeUsesSorted(gep1SortedUses, gep1->use_begin(), gep1->use_end());
#ifdef VERBOSE
                  {
                     Utils::log(errs(), 2, 1, "Sorted order of gep1 uses");
                     std::string stringBuff;
                     raw_string_ostream stream(stringBuff);
                     for(auto& e : gep1SortedUses)
                     {
                        stringBuff.clear();
                        e->getUser()->print(stream);
                        Utils::log(errs(), 4, 1, stringBuff);
                     }
                  }

#endif
                  for(auto* u3 : gep1SortedUses)
                  {
                     if(auto* load2 = dyn_cast<LoadInst>(u3->getUser()))
                     {
                        std::vector<Use*> load2SortedUses;
                        makeUsesSorted(load2SortedUses, load2->use_begin(), load2->use_end());
#ifdef VERBOSE
                        {
                           Utils::log(errs(), 2, 1, "Sorted order of load2 uses");
                           std::string stringBuff;
                           raw_string_ostream stream(stringBuff);
                           for(auto& e : load2SortedUses)
                           {
                              stringBuff.clear();
                              e->getUser()->print(stream);
                              Utils::log(errs(), 4, 1, stringBuff);
                           }
                        }
#endif
                        bool found = false;
                        for(auto* u4 : load2SortedUses)
                        {
                           if(auto* load3 = dyn_cast<LoadInst>(u4->getUser()))
                           {
#ifdef REMOVE_OUTLINED_GEP
                              found = true;
                              // assert(gepToReplaceCounter < newTypesPtrSize);
                              assert(gepOffset % Constants::structAlignment == 0);
                              // We prefer this version as GEP offset inside the IR may be shuffled. With this op we
                              // can always retrive the right offset for getArg
                              Value* V = newFunction->getArg(argsOffset + (gepOffset / Constants::structAlignment));
// Value* V = newFunction->getArg(argsOffset + gepToReplaceCounter);
#ifdef VERBOSE
                              {
                                 std::string stringBuff1;
                                 raw_string_ostream stream1(stringBuff1);
                                 std::string stringBuff2;
                                 raw_string_ostream stream2(stringBuff2);
                                 load3->print(stream1);
                                 V->print(stream2);
                                 Utils::log(errs(), 2, 1, "Replacing", stringBuff1, "with", stringBuff2);
                              }
#endif
                              load3->replaceAllUsesWith(V);
#else
                              std::vector<Use*> load3SortedUses;
                              makeUsesSorted(load3SortedUses, load3->use_begin(), load3->use_end());
                              for(auto* u5 : load3SortedUses)
                              {
                                 if(auto* gep2 = dyn_cast<GetElementPtrInst>(u5->getUser()))
                                 {
                                    found = true;
                                    assert(gepToReplaceCounter < newTypesPtrSize);
                                    // We prefer this version as GEP offset inside the IR may be shuffled. With this op
                                    // we can always retrive the right offset for getArg
                                    assert(gepOffset % Constants::structAlignment == 0);
                                    Value* V =
                                        newFunction->getArg(argsOffset + (gepOffset / Constants::structAlignment));
                                    // Value* V = newFunction->getArg(argsOffset + gepToReplaceCounter);
#ifdef VERBOSE
                                    {
                                       std::string stringBuff1;
                                       raw_string_ostream stream1(stringBuff1);
                                       std::string stringBuff2;
                                       raw_string_ostream stream2(stringBuff2);
                                       gep2->print(stream1);
                                       V->print(stream2);
                                       Utils::log(errs(), 2, 1, "Replacing", stringBuff1, "with", stringBuff2);
                                    }
#endif
                                    gep2->replaceAllUsesWith(V);
                                 }
                              }
#endif
                           }
                        }
                        assert(found && "expected to found at least one node");
                        // gepToReplaceCounter++;
                     }
                  }
               }
            }

            // We do look only for 1 load inst
            break;
         }
      }
      assert(verifyFunction(*newFunction, &errs()) == false);

      F.replaceAllUsesWith(newFunction);
      removeInstructionsWithName<GetElementPtrInst>(*newFunction, GetElementPtrInstIsSpecificStruct<0>,
                                                    Constants::targetOmpAnonStructName);
      removeInstructionsWithName<GetElementPtrInst>(*newFunction, GetElementPtrInstIsSpecificStruct<0>,
                                                    Constants::targetOmpTaskTWithPrivatesStructName);
      removeInstructionsWithNoUses(*newFunction);
      // Substitute inside mapping
      if(duplicatedResourcesDict.count(&F) > 0)
      {
         duplicatedResourcesDict[newFunction] = duplicatedResourcesDict[&F];
         duplicatedResourcesDict.erase(&F);
      }
      return newFunction;
   }

   /// Collect all the omp entry @ref llvm::Function functions which are called from a parent function.
   /// @param F the parent function
   /// @param collector the references collector
   void collectCalledOmpEntries(Function* F, std::unordered_set<Function*>& collector, unsigned operandIndex = 5U)
   {
      Utils::log(errs(), 0, 1, "Collecting omp entries");
      assert(F && "nullptr");
      for(auto& BB : *F)
      {
         for(auto& I : BB)
         {
            if(auto* callInst = dyn_cast<CallInst>(&I))
            {
               auto* called = requireDirectCallee(callInst);
               if(!called)
               {
                  continue;
               }
               if(called->getName().find(Constants::targetOmpTaskAllocName) != std::string::npos)
               {
                  Value* operand = callInst->getOperand(operandIndex);
                  assert(isa<Function>(operand));
                  Utils::log(errs(), 2, 1, "collected", cast<Function>(operand)->getName());
                  collector.insert(cast<Function>(operand));
               }
            }
         }
      }
   }

   PreservedAnalyses execute(Module& M, ModuleAnalysisManager& MAM)
   {
      if(M.getDataLayout().getPointerSizeInBits() != 32)
      {
         report_fatal_error("OpenMP task extraction requires 32-bit pointers (use I386_CLANG19)");
      }
      if(resourceDuplicationNum == 0)
      {
         report_fatal_error("OpenMP task resource duplication must be greater than zero");
      }
      // These runtime operations carry synchronization semantics not represented
      // by the static dependency graph. Do not erase them during simplification.
      for(const auto& F : M)
      {
         if(F.getName().contains(Constants::bambuHostReservedFnName))
         {
            continue;
         }
         for(const auto& BB : F)
         {
            for(const auto& I : BB)
            {
               const auto* call = dyn_cast<CallBase>(&I);
               const auto* callee = call ? call->getCalledFunction() : nullptr;
               if(!callee)
               {
                  continue;
               }
               const auto name = callee->getName();
               if(name.starts_with("__kmpc_omp_taskwait") || name.starts_with("__kmpc_taskgroup") ||
                  name.starts_with("__kmpc_end_taskgroup") || name.starts_with("__kmpc_omp_taskyield") ||
                  name.starts_with("__kmpc_taskloop"))
               {
                  report_fatal_error(Twine("OpenMP task extraction does not support ") + name);
               }
            }
         }
      }
      if(NamedMDNode* NMD = M.getNamedMetadata(Constants::llvmIndentName))
      {
         for(unsigned i = 0, e = NMD->getNumOperands(); i < e; ++i)
         {
            MDNode* Node = NMD->getOperand(i);
            if(auto* MS = dyn_cast<MDString>(Node->getOperand(0)))
            {
               const auto versionStr = MS->getString();
               const auto pos = versionStr.find(Constants::llvmVersionPrefix);
               assert(pos != std::string::npos);
               const auto version = versionStr.substr(pos + Constants::llvmVersionPrefix.size(), 2);
               assert(version == Constants::supportedLlvmVersion &&
                      "IR input generated from a not supported clang version");
            }
         }
      }
      else
      {
         report_fatal_error("Can not find clang version in metadata");
      }
      // Cache the Function reference for recursive usage later inside the
      // visitor. Some visitor fn may access some Function which has not
      // been visited yet in the pass.
      for(Function& F : M)
      {
         const auto fnName = F.getName().str();
         fnCache[fnName] = &F;
      }
      for(auto& fn : fnCache)
      {
         Utils::log(errs(), 0, 1, "Cached fn name", fn.first);
      }

      Utils::log(errs(), 0, 2, "\n\nStarting the Function visit for the Module", M.getName());

      // Unique task hash generator
      unsigned moduleTaskCount = 0;

      // Iterate over all functions and visit them
      Function* targetOutlined = nullptr;
      TopoGraphGenerator topo;
      std::unordered_set<Function*> ompOutlinedCalledLambda;
      for(Function& F : M)
      {
         // Visit only functions of interest
         const auto fnName = F.getName().str();
         if(fnName.find(Constants::targetTaskEntryNamePrefix) != std::string::npos)
         {
            removeFnAttribute(F, Attribute::AlwaysInline);
            addFnAttribute(F, Attribute::NoInline);
         }
         if(fnName.find(Constants::targetOmpOutlinedFnNamePrefix) != std::string::npos &&
            fnName.find(Constants::bambuHostReservedFnName) == std::string::npos)
         {
            if(targetOutlined)
            {
               report_fatal_error("OpenMP task extraction supports a single parallel region");
            }
            targetOutlined = &F;

            // Recover the task entries
            collectCalledOmpEntries(targetOutlined, ompOutlinedCalledLambda);

            // Duplicated task entries
            duplicateResources(ompOutlinedCalledLambda, duplicatedResourcesDict);

            visitor(F, nullptr, moduleTaskCount, topo);
         }
         Utils::log(errs(), 0, 1, "Skipping the visit of function called", fnName, "as of not interest");
      }

      // Perform post graph extraction optimizations
      std::vector<std::string> newFnNames;
      for(Function& F : M)
      {
         // Visit only functions of interest
         if(F.isDeclaration())
         {
            continue;
         }

         for(auto& BB : F)
         {
            for(auto& I : BB)
            {
               if(auto* callInstr = dyn_cast<CallInst>(&I))
               {
                  auto* calledFn = callInstr->getCalledFunction();
                  if(calledFn != nullptr &&
                     calledFn->getName().find(Constants::targetOmpForkCall) != std::string::npos &&
                     F.getName().find(Constants::bambuHostReservedFnName) == std::string::npos)
                  {
                     promoteForkCallBaseAddr(F);
                  }
               }
            }
         }
      }

      for(auto* F : ompOutlinedCalledLambda)
      {
         removeFnAttribute(*F, Attribute::AlwaysInline);
         addFnAttribute(*F, Attribute::NoInline);
         transformTaskEntryFn(*F, newFnNames, taskEntryPointArgTypesList);
      }
      for(auto* F : ompOutlinedCalledLambda)
      {
         F->eraseFromParent();
      }
      std::unordered_map<Value*, std::vector<Type*>> duplicatedResourceInputArgsTypesDict;
      std::vector<std::string> untrackedNames;
      for(auto& resource : duplicatedResourcesDict)
      {
         std::vector<Function*> updated;
         for(auto& F : resource.second)
         {
            assert(duplicatedFnToOriginal.count(F) == 1);
            assert(taskEntryPointArgTypesList.count(duplicatedFnToOriginal[F]) == 1);
            duplicatedResourceInputArgsTypesDict[F] = taskEntryPointArgTypesList[duplicatedFnToOriginal[F]];
            removeFnAttribute(*F, Attribute::AlwaysInline);
            addFnAttribute(*F, Attribute::NoInline);
            updated.push_back(transformTaskEntryFn(*F, untrackedNames, duplicatedResourceInputArgsTypesDict));
         }
         for(auto& F : resource.second)
         {
            F->eraseFromParent();
         }
         // Old pointers have been invalidated
         resource.second = updated;
      }

      // Generate output XML
      topo.setTaskEntries(newFnNames);
      topo.buildTopo();
      std::unordered_map<std::string, std::string> fnNamesMap;
      for(const auto& f : newFnNames)
      {
         const std::string prefix = ".omp_task_entry._";
         std::string id = "0";
         size_t index = prefix.size();
         while(index < f.size() && f[index] >= '0' && f[index] <= '9')
         {
            id += f[index++];
         }
         fnNamesMap[id] = f;
      }
      topo.setNodeTaskEntryMap(fnNamesMap);
      const std::string xmlPath =
          outDirName.empty() ? (Constants::outXMLFileName) : (outDirName.getValue() + "/" + Constants::outXMLFileName);

      topo.dump_xml(xmlPath);

      return PreservedAnalyses::none();
   }

   struct TaskCallGraphPass : PassInfoMixin<TaskCallGraphPass>
   {
      PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM)
      {
         return execute(M, MAM);
      }

      static bool isRequired()
      {
         return true;
      }
   };

#else

   struct TaskCallGraphPass : PassInfoMixin<TaskCallGraphPass>
   {
      PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM)
      {
         return PreservedAnalyses::all();
      }

      static bool isRequired()
      {
         return true;
      }
   };

#endif

} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
PassPluginLibraryInfo getOmpDependencyFinderPlugInInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "TaskCallGraphPass", LLVM_VERSION_STRING, [](PassBuilder& PB) {
              PB.registerPipelineParsingCallback(
                  [](StringRef Name, ModulePassManager& MPM, ArrayRef<PassBuilder::PipelineElement>) {
                     if(Name == "openmpTask")
                     {
                        MPM.addPass(TaskCallGraphPass());
                        return true;
                     }
                     return false;
                  });
           }};
}

// This is the core interface for pass plugins. It guarantees that 'opt'
// will be able to recognize TaskCallGraphPass when added to the pass
// pipeline on the command line, i.e. via '-passes=fn-pass-and-analysis'
extern "C" PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default"))) llvmGetPassPluginInfo()
{
   return getOmpDependencyFinderPlugInInfo();
}

#else

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <llvm/IR/LegacyPassManager.h>

namespace
{
   struct TaskCallGraphPass : public llvm::ModulePass
   {
      char ID = 0;
      TaskCallGraphPass() : llvm::ModulePass(ID)
      {
      }

      bool runOnModule(llvm::Module& M) override
      {
         return false;
      }
   };

} // namespace

#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new TaskCallGraphPass());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses TaskCallGraphPass_Ox(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);

#endif

#endif
