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
/*
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */
#ifndef ARR_PART_HPP
#define ARR_PART_HPP

#include <cstddef>
#include <cstdint>

#include "panda_clang_compat.hpp"
#include "llvm/Support/raw_ostream.h"
#include <llvm/ADT/Twine.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include <map>
#include <pugixml.hpp>
#include <string>
#include <vector>

#define BAMBU_CSROA_PARTITION_FUN_NAME "__bambu_csroa_partition__"

using namespace llvm;

/**
 * @brief Partitioning format.
 */
enum PartInfoFormat
{
   /** Entire object stored contiguously in a single memory */
   COMPLETE = 0,

   /** Object split into contiguous blocks */
   BLOCK = 1,

   /** Object distributed cyclically across memories */
   CYCLIC = 2,

   /** No storage / invalid or unused partition */
   NONE = 3
};

StringRef format_to_string(const PartInfoFormat& f);

/**
 * @brief Describes how a dimension of a data structure is partitioned.
 *
 * This struct contains metadata about a partitioned object, including
 * the partitioning format, number of memory blocks, and the partitioned
 * dimension.
 *
 * IMPORTANT: it describes only 1 dimension of the partitioned object. To have
 * all the information about the partitioning of the data structure, you should
 * retrieve the vector of PartInfo.
 */
class PartInfo
{
   void validatePartitionFactor(uint64_t numMemoriesArg, uint64_t dimSize);

 public:
   /** Partitioning format used for this part */
   PartInfoFormat format;

   /** Number of distinct memories involved in the partition */
   size_t factor;

   PartInfo() : format(PartInfoFormat::NONE), factor(0){};

   PartInfo(PartInfoFormat format, size_t factor) : format(format), factor(factor){};

   explicit PartInfo(size_t formatArg, size_t numMemoriesArg, size_t dimSize);

   [[nodiscard]] std::string to_string() const
   {
      return ("FORMAT " + format_to_string(format) + ", FACTOR " + Twine(factor)).str();
   }
};

using PartitionScheme = std::vector<PartInfo>;

Type* getArrayBaseType(Type* arrTy);
std::vector<size_t> getDimsFromArrayType(ArrayType* arrTy);
void initializePartitionScheme(PartitionScheme& scheme, size_t numDims);
void setPartitionInfo(PartitionScheme& scheme, const PartInfo& p, uint64_t dim);
[[nodiscard]] uint64_t getNumPartitionsFromScheme(const PartitionScheme& scheme);
/// Whether at least one dimension carries a partitioning request.
[[nodiscard]] inline bool hasAnyPartitionedDim(const PartitionScheme& scheme)
{
   return llvm::any_of(scheme, [](const PartInfo& p) { return p.format != PartInfoFormat::NONE; });
}
[[nodiscard]] std::vector<size_t> getPartitionedDimsFromScheme(const std::vector<size_t>& origDims,
                                                               const PartitionScheme& scheme);
[[nodiscard]] ArrayType* getPartitionedTypeFromDims(Type* originalType, const std::vector<size_t>& dims);
[[nodiscard]] std::string toStringWithPartitionScheme(const Value* value, const PartitionScheme& scheme);

/**
 * @brief Contains the allocaInst and how the original type is partitioned
 */
struct AllocaPartInfo
{
   /// An alloca is addressed from its base, so a GEP on it carries a leading whole-object index.
   static constexpr bool AddressedFromBase = true;

   AllocaInst* inst;
   PartitionScheme scheme;
   std::map<uint64_t, Value*> partitionMap;

   explicit AllocaPartInfo(AllocaInst* inst) : inst(inst)
   {
      initializePartitionScheme(scheme, getDimsFromArrayType(cast<ArrayType>(inst->getAllocatedType())).size());
   }

   const Value* key() const
   {
      return inst;
   }

   void setPartInfo(const PartInfo& p, uint64_t dim)
   {
      setPartitionInfo(scheme, p, dim);
   }

   [[nodiscard]] uint64_t getNumPartitions() const
   {
      return getNumPartitionsFromScheme(scheme);
   }

   [[nodiscard]] ArrayType* getPartitionedType() const
   {
      return getPartitionedTypeFromDims(inst->getAllocatedType(), getPartitionedDims());
   }

   [[nodiscard]] std::vector<size_t> getPartitionedDims() const
   {
      return getPartitionedDimsFromScheme(getOrigTypeDims(), scheme);
   }

   std::vector<size_t> getOrigTypeDims() const
   {
      return getDimsFromArrayType(cast<ArrayType>(inst->getAllocatedType()));
   }

   [[nodiscard]] std::string to_string() const
   {
      return toStringWithPartitionScheme(inst, scheme);
   }
};

/**
 * @brief Contains the Argument and how the original type is partitioned
 */
struct ArgPartInfo
{
   /// An array argument has already decayed to a pointer to its element type, so a GEP on it
   /// starts straight at the partitioning indices.
   static constexpr bool AddressedFromBase = false;

   std::string argName; // TODO(perf): Maybe it could be transformed into a StringRef for performance
   Argument* arg;
   PartitionScheme scheme;
   std::vector<size_t> origDims;
   std::map<uint64_t, Value*> partitionMap;

   explicit ArgPartInfo(std::string& name, Argument* arg, const std::vector<size_t>& dims)
       : argName(name), arg(arg), origDims(dims)
   {
      initializePartitionScheme(scheme, dims.size());
   }

   const Value* key() const
   {
      return arg;
   }

   void setPartInfo(const PartInfo& p, uint64_t dim)
   {
      setPartitionInfo(scheme, p, dim);
   }

   uint64_t getNumPartitions() const
   {
      return getNumPartitionsFromScheme(scheme);
   }

   std::vector<size_t> getPartitionedDims() const
   {
      return getPartitionedDimsFromScheme(getOrigTypeDims(), scheme);
   }

   std::vector<size_t> getOrigTypeDims() const
   {
      return origDims;
   }

   Type* getPartitionedType() const
   {
#if PANDA_LLVM_CLANG_MAJOR < 16
      return getPartitionedTypeFromDims(arg->getType()->getPointerElementType(), getPartitionedDims());
#elif PANDA_LLVM_CLANG_MAJOR == 16
      return arg->getType()->isOpaquePointerTy() ?
                 arg->getType() :
                 getPartitionedTypeFromDims(arg->getType()->getNonOpaquePointerElementType(), getPartitionedDims());
#else
      return arg->getType();
#endif
   }

   [[nodiscard]] std::string to_string() const
   {
      return toStringWithPartitionScheme(arg, scheme);
   }
};

struct FnPartInfo
{
   std::string name;

   // It points to the original function
   Function* fnOriginal = nullptr;

   Function* fnPartitionedWithDupl = nullptr;

   Function* fnPartitioned = nullptr;

   // Error basic block used to not duplicate them if there are a lot of
   // memories to partition
   BasicBlock* errBB = nullptr;

   std::vector<ArgPartInfo> args;
   std::vector<AllocaPartInfo> allocs;

   FnPartInfo(Function* fnOriginal) : name(fnOriginal->getName().str()), fnOriginal(fnOriginal)
   {
   }

   void remap(ValueToValueMapTy& vMap);
};

/**
 * @brief Contains the globalVar and how the original type is partitioned
 */
struct GlobalPartInfo
{
   /// A global, like an alloca, is addressed from its base.
   static constexpr bool AddressedFromBase = true;

   GlobalVariable* var;
   PartitionScheme scheme;
   std::map<uint64_t, Value*> partitionMap;

   explicit GlobalPartInfo(GlobalVariable* var) : var(var)
   {
      initializePartitionScheme(scheme, getDimsFromArrayType(cast<ArrayType>(var->getValueType())).size());
   }

   const Value* key() const
   {
      return var;
   }

   void setPartInfo(const PartInfo& p, uint64_t dim)
   {
      setPartitionInfo(scheme, p, dim);
   }

   uint64_t getNumPartitions() const
   {
      return getNumPartitionsFromScheme(scheme);
   }

   ArrayType* getPartitionedType() const
   {
      return getPartitionedTypeFromDims(var->getValueType(), getPartitionedDims());
   }

   std::vector<size_t> getPartitionedDims() const
   {
      return getPartitionedDimsFromScheme(getOrigTypeDims(), scheme);
   }

   std::vector<size_t> getOrigTypeDims() const
   {
      return getDimsFromArrayType(cast<ArrayType>(var->getValueType()));
   }

   [[nodiscard]] std::string to_string() const
   {
      std::string s;
      llvm::raw_string_ostream os(s);
      os << var->getName();
      os << " ";
      var->getType()->print(os);
      os << " => ";
      for(size_t i = 0; i < scheme.size(); i++)
      {
         os << "[" << format_to_string(scheme[i].format) << ", " << scheme[i].factor << "]";
         if(i < scheme.size() - 1)
         {
            os << "; ";
         }
      }
      return os.str();
   }
};

struct ArrPartCtx
{
   pugi::xml_document* doc;
   Function* topFn = nullptr;
   StringMap<FnPartInfo> fnTable;
   std::vector<GlobalPartInfo> globalVars;
};

template <typename ValueT, typename PartInfoT>
auto findPartInfoInContainer(const ValueT* value, std::vector<PartInfoT>& partitionInfos)
{
   return llvm::find_if(partitionInfos, [&](const PartInfoT& p) { return p.key() == value; });
}

void describeArrPartRequests(ArrPartCtx& ctx);
void printModuleOnFile(Module& M, const std::string& outPath);
void diffuseArrPartConfigs(ArrPartCtx& arrPartCtx, std::vector<std::string>& workQueue);
inline bool isArgPartitionable(Argument* arg);
void populateArrPartCtx(Module& M, ArrPartCtx& arrPartCtx);
bool isArrPartFunctionPresent(Module& M);
FnPartInfo& getFnInfoOrDie(StringMap<FnPartInfo>& table, StringRef fnName);
void inverseTopologicalSort(Function* fn, std::vector<std::string>& workQueue);
bool loadXMLModule(pugi::xml_document& doc, std::string& outdirName);
void modifyXMLModule(ArrPartCtx& arrPartCtx, std::string& outdirName);
std::string getDemangled(const std::string& declname);
Function* findTopFunction(Module& M, std::string& topFunctionNameArgPass);

#endif // ARR_PART_HPP
