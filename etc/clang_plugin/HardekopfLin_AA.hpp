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
 *              Copyright(C) 2018-2020 Politecnico di Milano
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
 * @file HardekopfLin_AA.hpp
 * @brief Header file including Andersen's alias analysis.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef HARDEKOPFLIN_AA_HPP
#define HARDEKOPFLIN_AA_HPP

#include <map>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "bdd.h"
#include "fdd.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SparseBitVector.h"
#include "llvm/ADT/StringMap.h"

namespace llvm
{
   class Module;
   class Function;
   class GlobalVariable;
   class Constant;
   class ConstantExpr;
   class User;
   class Type;
   class StructType;
   class Value;
   class Instruction;
   class BasicBlock;
   class ImmutableCallSite;
   class TargetLibraryInfo;
   class CallInst;
} // namespace llvm

#define NOVAR_ID ~0u

#define BM_ELSZ 128
typedef llvm::SparseBitVector<BM_ELSZ> bitmap;

class Node;
class Constraint;
class Worklist;

// External function types
// Assume a call in the form LHS= F(arg0, arg1, arg2, arg3).
enum extf_t
{
   EFT_NOOP = 0,       // no effect on pointers
   EFT_ALLOC,          // returns a ptr to a newly allocated object
   EFT_REALLOC,        // like L_A0 if arg0 is a non-null ptr, else ALLOC
   EFT_NOSTRUCT_ALLOC, // like ALLOC but only allocates non-struct data
   EFT_STAT,           // retval points to an unknown static var X
   EFT_STAT2,          // ret -> X -> Y (X, Y - external static vars)
   EFT_L_A0,           // copies arg0, arg1, or arg2 into LHS
   EFT_L_A1,
   EFT_L_A2,
   EFT_L_A8,
   EFT_L_A0__A0R_A1R, // copies the data that arg1 points to into the location
   //  arg0 points to; note that several fields may be
   //  copied at once if both point to structs.
   //  Returns arg0.
   EFT_A1R_A0R,      // copies *arg0 into *arg1, with non-ptr return
   EFT_A3R_A1R_NS,   // copies *arg1 into *arg3 (non-struct copy only)
   EFT_A1R_A0,       // stores arg0 into *arg1
   EFT_A2R_A1,       // stores arg1 into *arg2
   EFT_A4R_A1,       // stores arg1 into *arg4
   EFT_L_A0__A2R_A0, // stores arg0 into *arg2 and returns it
   EFT_A0R_NEW,      // stores a pointer to an allocated object in *arg0
   EFT_A1R_NEW,      // as above, into *arg1, etc.
   EFT_A2R_NEW,
   EFT_A4R_NEW,
   EFT_A11R_NEW,
   EFT_OTHER // not found in the list
};

//------------------------------------------------------------------------------
class ExtInfo
{
 private:
   // Each function name is mapped to its extf_t
   //  (unordered_map and map are much slower).
   llvm::StringMap<extf_t> info;
   // A cache of is_ext results for all Function*'s.
   std::unordered_map<const llvm::Function*, bool> isext_cache;

   void init(); // fill in the map

 public:
   //------------------------------------------------------------------------------
   ExtInfo()
   {
      init();
      isext_cache.clear();
   }

   // Return the extf_t of (F).
   extf_t get_type(const llvm::Function* F) const;

   // Does (F) have a static var X (unavailable to us) that its return points to?
   bool has_static(const llvm::Function* F) const
   {
      extf_t t = get_type(F);
      return t == EFT_STAT || t == EFT_STAT2;
   }
   // Assuming hasStatic(F), does (F) have a second static Y where X -> Y?
   bool has_static2(const llvm::Function* F) const
   {
      return get_type(F) == EFT_STAT2;
   }
   // Does (F) allocate a new object?
   bool is_alloc(const llvm::Function* F) const
   {
      extf_t t = get_type(F);
      return t == EFT_ALLOC || t == EFT_NOSTRUCT_ALLOC;
   }
   // Does (F) allocate only non-struct objects?
   bool no_struct_alloc(const llvm::Function* F) const
   {
      return get_type(F) == EFT_NOSTRUCT_ALLOC;
   }
   // Does (F) not do anything with the known pointers?
   bool is_noop(const llvm::Function* F) const
   {
      return get_type(F) == EFT_NOOP;
   }

   // Should (F) be considered "external" (either not defined in the program
   //  or a user-defined version of a known alloc or no-op)?
   bool is_ext(const llvm::Function* F);
};

enum ConsType : unsigned int;

typedef unsigned int u32;

// The starting union-find rank of any node.
const u32 node_rank_min = 0xf0000000;

//------------------------------------------------------------------------------
// A node for the offline constraint graph.
// This graph has VAL nodes for the top-level pointer variables (aka value
//  nodes) and AFP nodes for the parameters and return values of address-taken
//  functions (which are object nodes in the main graph but are used as normal
//  variables within the function). There is also a corresponding
//  REF node for the dereference (*p) of any VAL/AFP node (p).
class OffNode
{
 public:
   // Outgoing constraint edges: X -> Y for any cons. X = Y (where X/Y may
   //  be VAR or REF nodes).
   bitmap edges;
   // Outgoing implicit edges:
   //  any cons. edge X -> Y has a corresponding impl. edge *X -> *Y.
   bitmap impl_edges;
   // The union-find rank of this node if >= node_rank_min, else the number of
   //  another node in the same SCC.
   u32 rep;
   // The set of pointer-equivalence labels (singleton for HVN, any size for HU).
   //  This is empty for unlabeled nodes, and contains 0 for non-pointers.
   bitmap ptr_eq;
   // The node of the main graph corresponding to us (for HCD).
   u32 main_node{0};
   // The number of the DFS call that first visited us (0 if unvisited).
   u32 dfs_id{0};
   // True if this is the root of an already processed SCC.
   bool scc_root{false};
   // A VAL node is indirect if it's the LHS of a load+offset constraint;
   //  the LHS of addr_of and GEP are pre-labeled when building the graph
   //  and so don't need another unique label.
   // All AFP and REF nodes are indirect.
   bool indirect;

   OffNode(bool ind = false) : rep(node_rank_min), indirect(ind)
   {
   }

   bool is_rep() const
   {
      return rep >= node_rank_min;
   }
};

class Andersen_AA
{
   friend Constraint;

   bool BDD_INIT_DONE;

 protected:
   // top function name
   const std::string TopFunctionName;

   //------------------------------------------------------------------------------
   // Analysis results (should remain in memory after the run completes)
   //------------------------------------------------------------------------------

   // The node ID of each value, the first node of its object (if it has one),
   //  and the nodes for the return value and varargs of a function.
   llvm::DenseMap<const llvm::Value*, u32> val_node, obj_node;

   // The ID of the last object node (set by clump_addr_taken).
   u32 last_obj_node;

   // The nodes of the constraint and points-to graph
   std::vector<Node*> nodes;

   // The number (within a function) of each unnamed return or vararg node.
   llvm::DenseMap<const llvm::Function*, u32> ret_node, vararg_node;

 private:
   // The number (within a function) of each unnamed temporary value.
   // These numbers should match those in the bitcode disassembly.
   llvm::DenseMap<const llvm::Instruction*, u32> tmp_num;

   // The set of BDD variables in FDD domain 0 (which is used to represent
   //  the IDs of points-to members).
   bdd pts_dom;
   // Map the GEP-result FDD domain (1) to the points-to domain (0).
   bddPair* gep2pts;
   // For offset (i), geps[i] is the BDD function from the points-to domain
   //  to the GEP domain that adds (i) to each points-to member and removes
   //  the members too small for this offset.
   // The offsets that don't occur in actual constraints are mapped to bddfalse.
   std::vector<bdd> geps;

   //------------------------------------------------------------------------------
   // Data required during the entire run (may be deleted when the solver exits)
   //------------------------------------------------------------------------------

 protected:
   // The constraint list
   std::vector<Constraint> constraints;

   // The function pointer nodes used for indirect calls
   std::set<u32> ind_calls;

   // The load/store constraints representing an indirect call's
   //  return and args are mapped to the instruction of that call.
   //  Because constraints referring to different calls may be merged,
   //  1 cons. may map to several calls.
   llvm::DenseMap<Constraint, std::set<const llvm::Instruction*>> icall_cons;

   ExtInfo* extinfo;

 private:
   // The map of a dereferenced node to the VAR node in its SCC; see hcd().
   llvm::DenseMap<u32, u32> hcd_var;

   // For every valid offset (i), off_mask[i] is the set of nodes
   //  with obj_sz > i (for faster handling of load/store with offset).
   std::vector<bdd> off_mask;

   // The set of obj. nodes representing ext. functions.
   // The second version is for quickly testing if a given node is an ext.func.
   bdd ext_func_nodes;
   std::set<u32> ext_func_node_set;
   // The set of all nodes that start a function object
   //  (superset of ext_func_node_set).
   std::set<u32> func_node_set;

   // The solver's worklist.
   Worklist* WL;

 protected:
   // How many times solve_node() has run.
   u32 n_node_runs{};

 private:
   // The n_node_runs at the time of the last LCD run.
   u32 last_lcd{};

 protected:
   // The sequence number of the current node visit
   u32 vtime{};

 private:
   // The number of the current lcd_dfs run
   u32 curr_lcd_dfs{};
   // The copy edges across which we already found identical points-to sets
   llvm::DenseSet<std::pair<u32, u32>> lcd_edges;
   // The next LCD run should start from these nodes.
   std::set<u32> lcd_starts;
   // The DFS timestamp of each visited node
   std::map<u32, u32> lcd_dfs_id;
   // The stack of nodes visited by lcd_dfs()
   std::stack<u32> lcd_stk;
   // The roots of SCCs collapsed on this pass
   std::set<u32> lcd_roots;
   // The indirect ext. calls that have already been handled
   std::set<std::pair<const llvm::Function*, const llvm::Instruction*>> ext_seen;
   // The result of fdd_ithvar for each object node ID.
   std::vector<bdd> node_vars;
   // The names of unsupported external functions that were indirectly called.
   std::set<std::string> ext_failed;
   // The complex constraints (load, store, GEP) from the optimized list.
   std::vector<Constraint> cplx_cons;

   //------------------------------------------------------------------------------
   // Data required only for object/constraint ID
   //   (may be deleted before optimize/solve)
   //------------------------------------------------------------------------------

 protected:
   // The ID of the node to create next (should equal nodes.size())
   u32 next_node{};

 private:
   // The struct type with the most fields
   //  (or min_struct if no structs are found).
   // All allocations are assumed to be of this type,
   //  unless trace_alloc_type() succeeds.
   const llvm::Type* min_struct{};
   const llvm::Type* max_struct{};
   // The # of fields in max_struct (0 for min_struct)
   u32 max_struct_sz{};
   // Every struct type T is mapped to the vectors S (first) and O (second).
   // If field [i] in the expanded struct T begins an embedded struct,
   //  S[i] is the # of fields in the largest such struct, else S[i] = 1.
   // S[0] is always the size of the expanded struct T, since a pointer to
   //  the first field of T can mean all of T.
   // Also, if a field has index (j) in the original struct, it has index
   //  O[j] in the expanded struct.
   typedef llvm::DenseMap<const llvm::StructType*, std::pair<std::vector<u32>, std::vector<u32>>> structinfo_t;

   structinfo_t struct_info;

   // The list of value nodes for constant GEP expr.
   std::vector<u32> gep_ce;

   // The nodes that have already been visited by proc_global_init or proc_gep_ce.
   // The value is the return of proc_global_init, or 1 for proc_gep_ce.
   llvm::DenseMap<u32, u32> global_init_done;

   // The name of every has_static() ext. function is mapped to
   //  the node of the static object that its return points to.
   std::map<std::string, u32> stat_ret_node;

   // The args of addr-taken func. (exception for the node-info check)
   llvm::DenseSet<const llvm::Value*> at_args;

 protected:
   const llvm::Type* getmin_struct(llvm::Module& M);
   void run_init();
   void obj_cons_id(const llvm::Module& M, const llvm::Type* MS);
   void clump_addr_taken();
   void pre_opt_cleanup();

 private:
   //------------------------------------------------------------------------------
   // Private methods
   //------------------------------------------------------------------------------
   void run_cleanup();
   void pts_cleanup();

 protected:
   const Constraint& add_cons(ConsType t, u32 dest, u32 src, u32 off = 0, bool off_mandatory = false);

 private:
   void verify_nodes();
   void analyze_struct(const llvm::StructType* T);

   u32 compute_gep_off(const llvm::User* V);
   const llvm::Type* trace_alloc_type(const llvm::Instruction* I);
   size_t get_max_offset(const llvm::Value* V);
   u32 get_val_node_cptr(const llvm::Value* V);
   void id_func(const llvm::Function* F);
   void id_gep_ce(const llvm::Value* G);

 protected:
   void id_i2p_insn(const llvm::Value* V);
   void id_bitcast_insn(const llvm::Instruction* I);
   void id_phi_insn(const llvm::Instruction* I);
   void id_select_insn(const llvm::Instruction* I);
   void id_vaarg_insn(const llvm::Instruction* I);
   void id_extract_insn(const llvm::Instruction* I);

 private:
   void id_call_obj(u32 vnI, const llvm::Function* F);
   template <class CallInstOrInvokeInst>
   void id_dir_call(const CallInstOrInvokeInst* I, const llvm::Function* F);
   template <class CallInstOrInvokeInst>
   void id_ind_call(const CallInstOrInvokeInst* I);

   void id_global(const llvm::GlobalVariable* G);
   u32 get_ret_node(const llvm::Function* F) const;
   u32 get_vararg_node(const llvm::Function* F) const;
   std::string get_type_name(const llvm::Type* T) const;

   void print_struct_info(const llvm::Type* T) const;
   void print_all_structs() const;
   void print_val(const llvm::Value* V, u32 n = 0, bool const_with_val = 1, bool first = 1) const;
   void print_const(const llvm::Constant* C, u32 n, bool const_with_val, bool first) const;
   void print_const_expr(const llvm::ConstantExpr* E) const;
   void print_node(u32 n) const;
   void print_all_nodes() const;
   void print_all_constraints() const;
   void print_cons_graph(bool points_to_only) const;
   void print_node_cons(const bitmap& L) const;
   void print_metrics() const;

   void list_ext_unknown(const llvm::Module& M) const;

   void proc_gep_ce(u32 vnE);
   u32 proc_global_init(u32 onG, const llvm::Constant* C, bool first = true);

 protected:
   virtual void visit_func(const llvm::Function* F);

 private:
   bool trace_int(const llvm::Value* V, llvm::DenseSet<const llvm::Value*>& src, llvm::DenseMap<const llvm::Value*, bool>& seen, u32 depth = 0);

 protected:
   void id_ret_insn(const llvm::Instruction* I);
   template <class CallInstOrInvokeInst>
   void id_call_insn(const CallInstOrInvokeInst* I);
   void id_alloc_insn(const llvm::Instruction* I);
   void id_load_insn(const llvm::Instruction* I);
   void id_store_insn(const llvm::Instruction* I);
   void id_gep_insn(const llvm::User* gep);

 private:
   template <class CallInstOrInvokeInst>
   void id_ext_call(const CallInstOrInvokeInst* I, const llvm::Function* F);
   void add_store2_cons(const llvm::Value* D, const llvm::Value* S, size_t sz = 0);

   void processBlock(const llvm::BasicBlock* BB, std::set<const llvm::BasicBlock*>& bb_seen);

 protected:
   u32 merge_nodes(u32 n1, u32 n2);

 private:
   void hvn(bool do_union);
   void hr(bool do_union, u32 min_del);
   void make_off_nodes(std::vector<u32>& main2off, std::vector<OffNode>& off_nodes);
   void add_off_edges(std::vector<u32>& main2off, std::vector<OffNode>& off_nodes, u32& next_ptr_eq, bool hcd = false);
   void hvn_dfs(std::unordered_map<bitmap, u32>& lbl2pe, std::stack<u32>& dfs_stk, u32& curr_dfs, std::vector<OffNode>& off_nodes, u32& next_ptr_eq, u32 n, bool do_union);
   void hvn_check_edge(std::unordered_map<bitmap, u32>& lbl2pe, std::stack<u32>& dfs_stk, u32& curr_dfs, std::vector<OffNode>& off_nodes, u32& next_ptr_eq, u32 n, u32 dest, bool do_union);
   void hvn_label(std::unordered_map<bitmap, u32>& lbl2pe, std::vector<OffNode>& off_nodes, u32& next_ptr_eq, u32 n);
   void hu_label(std::vector<OffNode>& off_nodes, u32& next_ptr_eq, u32 n);
   void merge_ptr_eq(std::vector<u32>& main2off, std::vector<OffNode>& off_nodes);
   void hcd();
   void hcd_dfs(std::stack<u32>& dfs_stk, u32& curr_dfs, std::vector<OffNode>& off_nodes, u32 n);
   void factor_ls();
   void factor_ls(llvm::DenseMap<Constraint, u32>& factored_cons, const std::set<u32>& other, u32 ref, u32 off, bool load);
   void cons_opt();

   void pts_init();
   void solve_init();
   bool solve();
   void run_lcd();
   void solve_node(u32 n);
   bool solve_ls_cons(u32 n, u32 hcd_rep, const bdd& d_points_to, std::set<Constraint>& cons_seen, Constraint& C);
   void solve_ls_off(const bdd& d_points_to, bool load, u32 dest, u32 src, u32 off, const std::set<const llvm::Instruction*>* I);
   void solve_ls_n(const u32* pdpts, const u32* edpts, bool load, u32 dest, u32 src);
   bool solve_gep_cons(u32 n, const bdd& d_points_to, std::set<Constraint>& cons_seen, Constraint& C);
   bool add_copy_edge(u32 src, u32 dest);
   void solve_prop(u32 n, const bdd& d_points_to);
   template <class CallInstOrInvokeInst>
   void handle_ext(const llvm::Function* F, const CallInstOrInvokeInst* I);
   void lcd_dfs(u32 n);

 protected:
   //------------------------------------------------------------------------------
   // Inline methods
   //------------------------------------------------------------------------------
   // Find the node representing the value (V).
   // If V is not mapped to any node, returns 0 when allow_null is 1, else aborts.
   u32 get_val_node(const llvm::Value* V, bool allow_null = 0) const
   {
      assert(V);
      auto it = val_node.find(V);
      if(it == val_node.end())
      {
         if(allow_null)
            return 0;
         else
         {
            llvm::errs() << "\nValue has no node:  ";
            print_val(V);
            llvm::errs() << '\n';
            assert(0);
         }
      }
      u32 vn = it->second;
      assert(vn && "val_node map has a 0 entry");
      return vn;
   }

   // Find the starting node of the object of (V).
   u32 get_obj_node(const llvm::Value* V, bool allow_null = 0) const
   {
      assert(V);
      auto it = obj_node.find(V);
      if(it == obj_node.end())
      {
         if(allow_null)
            return 0;
         else
         {
            llvm::errs() << "\nValue has no obj node:  ";
            print_val(V);
            llvm::errs() << '\n';
            assert(0);
         }
      }
      u32 on = it->second;
      assert(on && "obj_node map has a 0 entry");
      return on;
   }

 private:
   //------------------------------------------------------------------------------
   // Get an iterator for struct_info[T], initializing as needed.
   // Do not call this directly; use the methods below.
   structinfo_t::iterator get_struct_info_iter(const llvm::StructType* T)
   {
      assert(T);
      auto it = struct_info.find(T);
      if(it != struct_info.end())
         return it;
      analyze_struct(T);
      return struct_info.find(T);
   }
   // Get a reference to struct_info[T].
   const std::pair<std::vector<u32>, std::vector<u32>>& get_struct_info(const llvm::StructType* T)
   {
      return get_struct_info_iter(T)->second;
   }
   // Get a reference to either component of struct_info.
   const std::vector<u32>& get_struct_sz(const llvm::StructType* T)
   {
      return get_struct_info_iter(T)->second.first;
   }
   const std::vector<u32>& get_struct_off(const llvm::StructType* T)
   {
      return get_struct_info_iter(T)->second.second;
   }

 protected:
   // Return the representative node # of the set containing node #n.
   // This also does path compression by setting the rep field
   //  of all nodes visited to the result.
   u32 get_node_rep(u32 n);

 private:
   // const version of the above, w/o path compression.
   u32 cget_node_rep(u32 n) const;

   //------------------------------------------------------------------------------
   // A caching wrapper for fdd_ithvar. This is faster than filling in
   //  all the entries at the start because we don't use all the nodes.
   bdd get_node_var(u32 n)
   {
      assert(n < node_vars.size() && "node ID out of range");
      bdd& b = node_vars[n];
      if(b == bddfalse)
         b = fdd_ithvar(0, static_cast<int>(n));
      return b;
   }

 public:
   Andersen_AA(std::string _TopFunctionName);
   virtual ~Andersen_AA();
   virtual void computePointToSet(llvm::Module& M);
   const std::vector<u32>* pointsToSet(const llvm::Value*, u32 = 0);
   const std::vector<u32>* pointsToSet(u32, u32 = 0);
   bool has_malloc_obj(u32 n, const llvm::TargetLibraryInfo* TLI, u32 off = 0);
   u32 PE(const llvm::Value*);
   u32 PE(u32);
   bool is_any(u32);
   bool is_null(u32, u32);
   bool is_single(u32 n, u32 off = 0);
   const llvm::Value* getValue(u32);
   const std::vector<bdd>& get_geps()
   {
      return geps;
   }
   void releaseMemory();
};

class PtsSet;
class DFG;
class SEGnode;
class OffNodeSFS;
class TpNode;
class LdNode;
class StNode;
class NpNode;
class PtsGraph;

class Staged_Flow_Sensitive_AA : public Andersen_AA
{
   friend Constraint;
   Andersen_AA PRE;           // for obj_cons_id, cons_opt
   std::vector<PtsSet> top;   // top-level var -> points-to set
   std::vector<bool> strong;  // object -> strong?
   std::vector<u32> priority; // the priority of each DFG node
   DFG* dfg;                  // the dataflow graph

   size_t prog_start_node;                                       // the program's initial SEG node
   std::vector<SEGnode> ICFG;                                    // the ICFG itself
   std::map<const llvm::Function*, u32> fun_start;               // function -> start node
   std::map<u32, std::vector<const llvm::Function*>> fun_cs;     // callsite -> function targets
   std::map<const llvm::Function*, u32> fun_ret;                 // function -> return node
   std::map<u32, u32> call_succ;                                 // callsite -> local successor
   std::map<const llvm::BasicBlock*, u32> bb_start;              // start nodes for BasicBlocks
   std::vector<std::pair<const llvm::CallInst*, u32>> idr_calls; // <idr call, callsite> pairs
   std::vector<u32> idr_cons;                                    // indices of constraints from idr calls
   std::map<u32, std::vector<const llvm::Function*>> tgts;       // fun ptr PRE rep -> internal targets
   u32 num_tmp;                                                  // number of temporary vars created by process_idr_cons()
   std::vector<u32> defs;                                        //   store constraint -> node containing store
   std::vector<u32> uses;                                        //    load constraint -> node containing load
   std::vector<u32> topo;                                        // topological number -> node, found by T4
   std::vector<u32> rq;                                          // RQ nodes found by T4 for T6
   std::vector<u32> t5_reps;                                     // nodes made into reps by T5
   std::vector<u32> not_nprq;                                    // !NP && !RQ nodes found by T6 for T5
   std::map<u32, std::vector<u32>> gv2n;                         // global var -> DFG init nodes

   std::set<u32> cons_strong;                 // store constraints that are strong updates
   std::map<u32, std::vector<u32>> cons_part; // constraint partitions
   std::map<u32, bitmap> rel;                 // obj -> constraint partitions
   std::vector<bitmap> var_part;              // access equivalence partition -> objects
   std::vector<u32> glob_init;                // global objects being initialized in a partition
   std::vector<u32> cons_store;               // store constraints for process_1{load,store}
   std::vector<u32> cons_load;                // load constraints for process_1{load,store}
   std::map<u32, std::vector<u32>> n2p;       // DFG node -> partitions
   std::map<u32, bitmap> n2g;                 // for sharing points-to graphs
   std::map<u32, u32> pass_defs;              // node -> store for add_ssa_info
   std::map<u32, u32> pass_node;              // node -> DFG index
   std::map<u32, std::vector<u32>> pass_uses; // node -> loads for add_ssa_info

   u32 pe_lbl;
   u32 dfs_num;             // for tarjans
   std::stack<u32> node_st; // for tarjans

   std::vector<OffNodeSFS> OCG; // the offline constraint graph

   std::map<std::pair<u32, u32>, u32> sq_map;
   std::map<u32, std::pair<u32, u32>> sq_unmap;

   Worklist* sfsWL{};

   u32 findOCG(u32 n);
   u32 uniteOCG(u32 a, u32 b);
   void hu(u32 n);
   void make_off_graph();
   void cons_opt(std::vector<u32>& redir);

   size_t create_node(bool np = false);
   void add_edge(u32 src, u32 dst);
   void erase_edge(u32 src, u32 dst);

   u32 find(u32 n);
   u32 unite(u32 a, u32 b, bool t2 = false, bool t5 = false);
   void clean_G();

   void T2();
   void t4_visit(u32 n);
   void T4();
   void t5_visit(u32 n);
   void T5();
   void t6_visit(u32 n, bool t7 = false);
   void T6(bool t7 = false);
   void rm_undef();
   void SEG(bool first = false);
   void sfs_id(llvm::Module& M);
   void visit_func(const llvm::Function* F);
   void processBlock(u32 parent, const llvm::BasicBlock* BB);
   void cons_opt_wrap();
   void icfg_inter_edges(llvm::Module& M);
   void process_idr_cons();
   void sfs_prep();
   u32 squeeze(u32 p, u32 o, bool save = false);
   std::pair<u32, u32> unsqueeze(u32 n);
   void partition_vars();
   void compute_seg();
   void process_1store(u32 part);
   void process_1load(u32 part);
   void process_seg(u32 part, u32 n);

   bool solve();
   void processTp(TpNode& N);
   void processLd(LdNode& N, u32 idx);
   void processSt(StNode& N, u32 idx);
   void processNp(NpNode& N);
   void processSharedLd(LdNode& N, PtsGraph& pts);

 public:
   Staged_Flow_Sensitive_AA(const std::string& _TopFunctionName);
   ~Staged_Flow_Sensitive_AA();
   void computePointToSet(llvm::Module& M);
};
#endif
