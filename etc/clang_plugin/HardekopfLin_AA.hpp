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
*              Copyright(c) 2018 Politecnico di Milano
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

#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <map>
#include <stack>

#include "llvm/ADT/SparseBitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "bdd.h"
#include "fdd.h"

namespace llvm {
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
};

#define NOVAR_ID ~0u

#define BM_ELSZ   128
typedef llvm::SparseBitVector<BM_ELSZ>  bitmap;

class Node;
class Constraint;
class Worklist;

//External function types
//Assume a call in the form LHS= F(arg0, arg1, arg2, arg3).
enum extf_t
{
   EFT_NOOP= 0,      //no effect on pointers
   EFT_ALLOC,        //returns a ptr to a newly allocated object
   EFT_REALLOC,      //like L_A0 if arg0 is a non-null ptr, else ALLOC
   EFT_NOSTRUCT_ALLOC, //like ALLOC but only allocates non-struct data
   EFT_STAT,         //retval points to an unknown static var X
   EFT_STAT2,        //ret -> X -> Y (X, Y - external static vars)
   EFT_L_A0,         //copies arg0, arg1, or arg2 into LHS
   EFT_L_A1,
   EFT_L_A2,
   EFT_L_A8,
   EFT_L_A0__A0R_A1R,  //copies the data that arg1 points to into the location
   //  arg0 points to; note that several fields may be
   //  copied at once if both point to structs.
   //  Returns arg0.
   EFT_A1R_A0R,      //copies *arg0 into *arg1, with non-ptr return
   EFT_A3R_A1R_NS,   //copies *arg1 into *arg3 (non-struct copy only)
   EFT_A1R_A0,       //stores arg0 into *arg1
   EFT_A2R_A1,       //stores arg1 into *arg2
   EFT_A4R_A1,       //stores arg1 into *arg4
   EFT_L_A0__A2R_A0, //stores arg0 into *arg2 and returns it
   EFT_A0R_NEW,      //stores a pointer to an allocated object in *arg0
   EFT_A1R_NEW,      //as above, into *arg1, etc.
   EFT_A2R_NEW,
   EFT_A4R_NEW,
   EFT_A11R_NEW,
   EFT_OTHER         //not found in the list
};

//------------------------------------------------------------------------------
class ExtInfo
{
   private:

      //Each function name is mapped to its extf_t
      //  (unordered_map and map are much slower).
      llvm::StringMap<extf_t> info;
      //A cache of is_ext results for all Function*'s.
      std::unordered_map<const llvm::Function*, bool> isext_cache;

      void init(); //fill in the map

   public:
      //------------------------------------------------------------------------------
      ExtInfo()
      {
         init();
         isext_cache.clear();
      }

      //Return the extf_t of (F).
      extf_t get_type(const llvm::Function *F) const;

      //Does (F) have a static var X (unavailable to us) that its return points to?
      bool has_static(const llvm::Function *F) const
      {
         extf_t t= get_type(F);
         return t==EFT_STAT || t==EFT_STAT2;
      }
      //Assuming hasStatic(F), does (F) have a second static Y where X -> Y?
      bool has_static2(const llvm::Function *F) const
      {
         return get_type(F) == EFT_STAT2;
      }
      //Does (F) allocate a new object?
      bool is_alloc(const llvm::Function *F) const
      {
         extf_t t= get_type(F);
         return t==EFT_ALLOC || t==EFT_NOSTRUCT_ALLOC;
      }
      //Does (F) allocate only non-struct objects?
      bool no_struct_alloc(const llvm::Function *F) const
      {
         return get_type(F) == EFT_NOSTRUCT_ALLOC;
      }
      //Does (F) not do anything with the known pointers?
      bool is_noop(const llvm::Function *F) const
      {
         return get_type(F) == EFT_NOOP;
      }

      //Should (F) be considered "external" (either not defined in the program
      //  or a user-defined version of a known alloc or no-op)?
      bool is_ext(const llvm::Function *F);
};

enum ConsType : unsigned int;

typedef unsigned int u32;

class Andersen_AA
{
      //top function name
      const std::string TopFunctionName;

      //------------------------------------------------------------------------------
      // Analysis results (should remain in memory after the run completes)
      //------------------------------------------------------------------------------

      //The nodes of the constraint and points-to graph
      std::vector<Node*> nodes;

      //The ID of the last object node (set by clump_addr_taken).
      u32 last_obj_node;

      //The node ID of each value, the first node of its object (if it has one),
      //  and the nodes for the return value and varargs of a function.
      llvm::DenseMap<const llvm::Value*, u32> val_node, obj_node;
      llvm::DenseMap<const llvm::Function*, u32> ret_node, vararg_node;

      //The number (within a function) of each unnamed temporary value.
      //These numbers should match those in the bitcode disassembly.
      llvm::DenseMap<const llvm::Instruction*, u32> tmp_num;

      //The set of BDD variables in FDD domain 0 (which is used to represent
      //  the IDs of points-to members).
      bdd pts_dom;
      //Map the GEP-result FDD domain (1) to the points-to domain (0).
      bddPair *gep2pts;
      //For offset (i), geps[i] is the BDD function from the points-to domain
      //  to the GEP domain that adds (i) to each points-to member and removes
      //  the members too small for this offset.
      //The offsets that don't occur in actual constraints are mapped to bddfalse.
      std::vector<bdd> geps;

      //------------------------------------------------------------------------------
      // Data required during the entire run (may be deleted when the solver exits)
      //------------------------------------------------------------------------------

      //The constraint list
      std::vector<Constraint> constraints;
      //The function pointer nodes used for indirect calls
      std::set<u32> ind_calls;

      //The load/store constraints representing an indirect call's
      //  return and args are mapped to the instruction of that call.
      //  Because constraints referring to different calls may be merged,
      //  1 cons. may map to several calls.
      llvm::DenseMap<Constraint, std::set<const llvm::Instruction*> > icall_cons;

      ExtInfo *extinfo;

      //The map of a dereferenced node to the VAR node in its SCC; see hcd().
      llvm::DenseMap<u32, u32> hcd_var;

      //For every valid offset (i), off_mask[i] is the set of nodes
      //  with obj_sz > i (for faster handling of load/store with offset).
      std::vector<bdd> off_mask;

      //The set of obj. nodes representing ext. functions.
      //The second version is for quickly testing if a given node is an ext.func.
      bdd ext_func_nodes;
      std::set<u32> ext_func_node_set;
      //The set of all nodes that start a function object
      //  (superset of ext_func_node_set).
      std::set<u32> func_node_set;

      //The solver's worklist.
      Worklist *WL;
      //How many times solve_node() has run.
      u32 n_node_runs;
      //The n_node_runs at the time of the last LCD run.
      u32 last_lcd;
      //The sequence number of the current node visit
      u32 vtime;
      //The number of the current lcd_dfs run
      u32 curr_lcd_dfs;
      //The copy edges across which we already found identical points-to sets
      llvm::DenseSet<std::pair<u32, u32> > lcd_edges;
      //The next LCD run should start from these nodes.
      std::set<u32> lcd_starts;
      //The DFS timestamp of each visited node
      std::map<u32,u32> lcd_dfs_id;
      //The stack of nodes visited by lcd_dfs()
      std::stack<u32> lcd_stk;
      //The roots of SCCs collapsed on this pass
      std::set<u32> lcd_roots;
      //The indirect ext. calls that have already been handled
      std::set<std::pair<const llvm::Function*, const llvm::Instruction*> > ext_seen;
      //The result of fdd_ithvar for each object node ID.
      std::vector<bdd> node_vars;
      //The names of unsupported external functions that were indirectly called.
      std::set<std::string> ext_failed;
      //The complex constraints (load, store, GEP) from the optimized list.
      std::vector<Constraint> cplx_cons;


      //------------------------------------------------------------------------------
      // Data required only for object/constraint ID
      //   (may be deleted before optimize/solve)
      //------------------------------------------------------------------------------

      //The ID of the node to create next (should equal nodes.size())
      u32 next_node;
      //The struct type with the most fields
      //  (or min_struct if no structs are found).
      //All allocations are assumed to be of this type,
      //  unless trace_alloc_type() succeeds.
      const llvm::Type* min_struct;
      const llvm::Type* max_struct;
      //The # of fields in max_struct (0 for min_struct)
      u32 max_struct_sz;
      //Every struct type T is mapped to the vectors S (first) and O (second).
      //If field [i] in the expanded struct T begins an embedded struct,
      //  S[i] is the # of fields in the largest such struct, else S[i] = 1.
      //S[0] is always the size of the expanded struct T, since a pointer to
      //  the first field of T can mean all of T.
      //Also, if a field has index (j) in the original struct, it has index
      //  O[j] in the expanded struct.
      typedef llvm::DenseMap<const llvm::StructType*, std::pair<std::vector<u32>, std::vector<u32> > > structinfo_t;

      structinfo_t struct_info;

      //The list of value nodes for constant GEP expr.
      std::vector<u32> gep_ce;

      //The nodes that have already been visited by proc_global_init or proc_gep_ce.
      //The value is the return of proc_global_init, or 1 for proc_gep_ce.
      llvm::DenseMap<u32, u32> global_init_done;

      //The name of every has_static() ext. function is mapped to
      //  the node of the static object that its return points to.
      std::map<std::string, u32> stat_ret_node;

      //The args of addr-taken func. (exception for the node-info check)
      llvm::DenseSet<const llvm::Value*> at_args;


      //------------------------------------------------------------------------------
      // Private methods
      //------------------------------------------------------------------------------
      void releaseMemory();
      void run_init();
      void run_cleanup();
      void pts_cleanup();

      const llvm::Type * getmin_struct(llvm::Module &M);

      bool add_cons(ConsType t, u32 dest, u32 src, u32 off=0);
      void verify_nodes();
      void analyze_struct(const llvm::StructType *T);

      u32 compute_gep_off(const llvm::User *V);
      const llvm::Type* trace_alloc_type(const llvm::Instruction *I);
      size_t get_max_offset(const llvm::Value *V);
      u32 get_val_node_cptr(const llvm::Value *V);
      bool escapes(const llvm::Value *V) const;
      void id_func(const llvm::Function *F);
      void id_gep_ce(const llvm::Value *G);
      void id_i2p_insn(const llvm::Value *V);
      void id_bitcast_insn(const llvm::Instruction *I);
      void id_phi_insn(const llvm::Instruction *I);
      void id_select_insn(const llvm::Instruction *I);
      void id_vaarg_insn(const llvm::Instruction *I);
      void id_extract_insn(const llvm::Instruction *I);
      void id_call_obj(u32 vnI, const llvm::Function *F);
      void id_dir_call(llvm::ImmutableCallSite CS, const llvm::Function *F);
      void id_ind_call(llvm::ImmutableCallSite CS);

      void id_global(const llvm::GlobalVariable *G);
      u32 get_ret_node(const llvm::Function *F) const;
      u32 get_vararg_node(const llvm::Function *F) const;
      std::string get_type_name(const llvm::Type *T) const;

      void print_struct_info(const llvm::Type *T) const;
      void print_all_structs() const;
      void print_val(const llvm::Value *V, u32 n=0, bool const_with_val=1, bool first=1) const;
      void print_const(const llvm::Constant *C, u32 n, bool const_with_val, bool first) const;
      void print_const_expr(const llvm::ConstantExpr *E) const;
      void print_node(u32 n) const;
      void print_all_nodes() const;
      void print_constraint(const Constraint &C) const;
      void print_all_constraints() const;
      void print_cons_graph(bool points_to_only) const;
      void print_node_cons(const bitmap &L) const;
      void print_metrics() const;

      void list_ext_unknown(const llvm::Module &M) const;

      void proc_gep_ce(u32 vnE);
      u32 proc_global_init(u32 onG, const llvm::Constant *C, bool first=true);
      void visit_func(const llvm::Function *F, const llvm::TargetLibraryInfo *TLI);
      bool trace_int(const llvm::Value *V, llvm::DenseSet<const llvm::Value*> &src,
          llvm::DenseMap<const llvm::Value*, bool> &seen, u32 depth = 0);
      void id_ret_insn(const llvm::Instruction *I);
      void id_call_insn(const llvm::Instruction *I);
      void id_malloc_insn(const llvm::Instruction *I);
      void id_alloc_insn(const llvm::Instruction *I);
      void id_load_insn(const llvm::Instruction *I);
      void id_store_insn(const llvm::Instruction *I);
      void id_gep_insn(const llvm::User *gep);

      void id_ext_call(const llvm::ImmutableCallSite &CS, const llvm::Function *F);
      void add_store2_cons(const llvm::Value *D, const llvm::Value *S, size_t sz=0);

      void processBlock(const llvm::BasicBlock *BB, const llvm::TargetLibraryInfo *TLI);

      void obj_cons_id(const llvm::Module &M, const llvm::TargetLibraryInfo *TLI, const llvm::Type *MS);

       void clump_addr_taken();
       u32 merge_nodes(u32 n1, u32 n2);
       void pre_opt_cleanup();
       void hvn(bool do_union);
       void hr(bool do_union, u32 min_del);
       void make_off_nodes();
       void add_off_edges(bool hcd = false);
       void hvn_dfs(u32 n, bool do_union);
       void hvn_check_edge(u32 n, u32 dest, bool do_union);
       void hvn_label(u32 n);
       void hu_label(u32 n);
       void merge_ptr_eq();
       void hcd();
       void hcd_dfs(u32 n);
       void factor_ls();
       void factor_ls(const std::set<u32> &other, u32 ref, u32 off, bool load);
       void cons_opt();

       void pts_init();
       void solve_init();
       bool solve();
       void run_lcd();
       void solve_node(u32 n);
       bool solve_ls_cons(u32 n, u32 hcd_rep, bdd d_points_to,
           std::set<Constraint> &cons_seen, Constraint &C);
       void solve_ls_off(bdd d_points_to, bool load,
           u32 dest, u32 src, u32 off, const std::set<const llvm::Instruction *> *I);
       void solve_ls_n(const u32 *pdpts, const u32 *edpts, bool load,
           u32 dest, u32 src);
       bool solve_gep_cons(u32 n, bdd d_points_to,
         std::set<Constraint> &cons_seen, Constraint &C);
       bool add_copy_edge(u32 src, u32 dest);
       void solve_prop(u32 n, bdd d_points_to);
       void handle_ext(const llvm::Function *F, const llvm::Instruction *I);
       void lcd_dfs(u32 n);


      //------------------------------------------------------------------------------
      // Inline methods
      //------------------------------------------------------------------------------
      //Find the node representing the value (V).
      //If V is not mapped to any node, returns 0 when allow_null is 1, else aborts.
      u32 get_val_node(const llvm::Value *V, bool allow_null= 0) const
      {
         assert(V);
         auto it= val_node.find(V);
         if(it == val_node.end())
         {
            if(allow_null)
               return 0;
            else
            {
               llvm::errs()<< "\nValue has no node:  ";
               print_val(V);
               llvm::errs() <<'\n';
               assert(0);
            }
         }
         u32 vn= it->second;
         assert(vn && "val_node map has a 0 entry");
         return vn;
      }

      //Find the starting node of the object of (V).
      u32 get_obj_node(const llvm::Value *V, bool allow_null= 0) const
      {
         assert(V);
         auto it= obj_node.find(V);
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
         u32 on= it->second;
         assert(on && "obj_node map has a 0 entry");
         return on;
      }

      //------------------------------------------------------------------------------
      //Get an iterator for struct_info[T], initializing as needed.
      //Do not call this directly; use the methods below.
      structinfo_t::iterator get_struct_info_iter(const llvm::StructType *T)
      {
         assert(T);
         auto it = struct_info.find(T);
         if(it != struct_info.end())
            return it;
         analyze_struct(T);
         return struct_info.find(T);
      }
      //Get a reference to struct_info[T].
      const std::pair<std::vector<u32>, std::vector<u32> >& get_struct_info(const llvm::StructType *T)
      {
         return get_struct_info_iter(T)->second;
      }
      //Get a reference to either component of struct_info.
      const std::vector<u32>& get_struct_sz(const llvm::StructType *T)
      {
         return get_struct_info_iter(T)->second.first;
      }
      const std::vector<u32>& get_struct_off(const llvm::StructType *T)
      {
         return get_struct_info_iter(T)->second.second;
      }

      //Return the representative node # of the set containing node #n.
      //This also does path compression by setting the rep field
      //  of all nodes visited to the result.
      u32 get_node_rep(u32 n);

      //const version of the above, w/o path compression.
      u32 cget_node_rep(u32 n) const;

      //------------------------------------------------------------------------------
      //A caching wrapper for fdd_ithvar. This is faster than filling in
      //  all the entries at the start because we don't use all the nodes.
      bdd get_node_var(u32 n)
      {
         llvm::errs() << n << "\n";
         assert(n < node_vars.size() && "node ID out of range");
         bdd &b= node_vars[n];
         if(b == bddfalse)
            b = fdd_ithvar(0, static_cast<int>(n));
         return b;
      }


   public:
      Andersen_AA(const std::string& _TopFunctionName);
      ~Andersen_AA();
      void computePointToSet(llvm::Module &M, const llvm::TargetLibraryInfo *TLI);
      const std::vector<u32>* pointsToSet(const llvm::Value*, u32= 0);
      const std::vector<u32>* pointsToSet(u32, u32= 0);
      u32 PE(const llvm::Value *);
      u32 PE(u32);
      bool is_null(u32,u32);
      bool is_single(u32,u32);
      const llvm::Value *getValue(u32);
};

#endif
