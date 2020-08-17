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
*              Copyright (C) 2004-2020 Politecnico di Milano
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
* @file dumpGimple.c
* @brief Functions to print gimple and tree representations
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
* @author Marco Lattuada <marco.lattuada@polimi.it>
*
*/
#include "plugin_includes.h"

#if (__GNUC__ > 4) 
#include "hashtab.h"
#endif
/* Value range array.  */
static unsigned num_vr_values=0;
tree* VRP_min_array=0;
tree* VRP_max_array=0;
bool* p_VRP_min=0;
bool* p_VRP_max=0;

char dump_base_name_trimmed[1024];

void set_num_vr_values(unsigned int n)
{
  num_vr_values = n;
}


int plugin_is_GPL_compatible;

int serialize_state;

static char *outdir_name;

/* Information about a node to be dumped.  */
#define ZERO_OR_ONE(val) (val != 0 ? 1 : 0)
#define MAKE_ANN(binfo_val, gimple_val, statement_val) (ZERO_OR_ONE(binfo_val) | (ZERO_OR_ONE(gimple_val) << 1) | (ZERO_OR_ONE(statement_val) << 2) )
#define IS_BINFO(val) (val&1)
#define IS_GIMPLE(val) (val&2)
#define IS_STATEMENT(val) (val&4)

typedef struct same_vuse_linked_list
{
      GIMPLE_type data;
      struct same_vuse_linked_list *next;
} * same_vuse_linked_list_p;

/* A serialize_queue is a link in the queue of things to be dumped.  */
typedef struct serialize_queue
{
  /* The queued tree node.  */
  struct tree_2_ints * node_index;
  /* The next node in the queue.  */
  struct serialize_queue *next;
} *serialize_queue_p;

/* List of tree node that has to be rehashed */
typedef struct toberehashedlist
{
  /* The tree node.  */
  tree node;
  /* previous hash value for node*/
  hashval_t hash;
  /* The next node in the list.  */
  struct toberehashedlist *next;
} *toberehashedlist_p;

/* A SerializeInfo gives information about how we should perform the serialization
   and about the current state of the serialization.  */
struct SerializeGimpleInfo
{
  /* The stream on which to dump the information.  */
  FILE *stream;
  /* The original node.  */
  const_tree node;
  /* The next column.  */
  unsigned int column;
  /* The first node in the queue of nodes to be written out.  */
  serialize_queue_p queue;
  /* The last node in the queue.  */
  serialize_queue_p queue_end;
  /* Free queue nodes.  */
  serialize_queue_p free_list;
  /* list of gimple nodes*/
  serialize_queue_p gimple_list;
  /* list of cfg nodes*/
  serialize_queue_p cfg_list;
};

/** The current state of the gimple serialization */
static struct SerializeGimpleInfo serialize_gimple_info;


/* The tree nodes which we have already written out.  The
     keys are the addresses of the nodes; the values are the integer
     indices we assigned them.  */
#if (__GNUC__ > 4)

#else
/* Return true if the from tree in both tree 2 int map are equal.  */
int
tree_2_ints_eq (const void *va, const void *vb)
{
  const struct tree_2_ints  *const a = (const struct tree_2_ints *) va,
    *const b = (const struct tree_2_ints *) vb;
  return (a->key == b->key && a->ann == b->ann );
}

/* Hash a from tree in a tree_2_ints.  */
unsigned int
tree_2_ints_hash (const void *item)
{
  return htab_hash_pointer (((const struct tree_2_ints *)item)->key);
}

/* Return true if the DECL_UID in both trees are equal.  */
int
dg_descriptor_tree_eq (const void *va, const void *vb)
{
  const struct dg_descriptor_tree  *const a = (const struct dg_descriptor_tree *) va,
    *const b = (const struct dg_descriptor_tree *) vb;
  return (a->hash == b->hash);
}
/* Hash a tree in a uid_decl_map.  */
unsigned int
dg_descriptor_tree_hash (const void *item)
{
  return ((const struct dg_descriptor_tree *)item)->hash;
}


#endif

/* Return true if this tree map structure is marked for garbage collection
   purposes.  We simply return true if the from tree is marked, so that this
   structure goes away when the from tree goes away.  */
int
tree_2_ints_marked_p (const void *p)
{
  return ggc_marked_p (((const struct tree_2_ints *) p)->key);
}

int
dg_descriptor_tree_marked_p(const void *p)
{
  return ggc_marked_p (((const struct dg_descriptor_tree *) p)->vd);
}

#if (__GNUC__ > 4)

#include "ggc_cpplike_static.h"

#else

#include "ggc_clike_static.h"

#endif

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wundef"
#endif
#if (__GNUC__ == 8)
#include "gtype_roots_gcc8.h"
#endif
#if (__GNUC__ == 7)
#include "gtype_roots_gcc7.h"
#endif
#if (__GNUC__ == 6)
#include "gtype_roots_gcc6.h"
#endif
#if (__GNUC__ == 5)
#include "gtype_roots_gcc5.h"
#endif
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 9)
#include "gtype_roots_gcc49.h"
#endif
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 8)
#include "gtype_roots_gcc48.h"
#endif
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 7)
#include "gtype_roots_gcc47.h"
#endif
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 6)
#include "gtype_roots_gcc46.h"
#endif
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif

/* Serialize the CHILD and its children.  */
#define serialize_child(field, child) \
  queue_and_serialize_index (field, child)
/* Serialize the GIMPLE CHILD and its children.  */
#define serialize_gimple_child(field, child) \
  queue_and_serialize_gimple_index (field, child)
/* Serialize the STATEMENT CHILD and its children.  */
#define serialize_statement_child(field, child) \
  queue_and_serialize_statement_index (field, child)

#define LOCATION_COLUMN(LOC) ((expand_location (LOC)).column)
#define EXPR_COLUMNNO(NODE) LOCATION_COLUMN (EXPR_CHECK (NODE)->exp.locus)


static unsigned int queue (tree);
static unsigned int queue_gimple ( GIMPLE_type t);
static unsigned int queue_statement (struct control_flow_graph * cfg);
static void serialize_index (unsigned int);
static void queue_and_serialize_index (const char *field, tree t);
static void queue_and_serialize_gimple_index (const char *field, GIMPLE_type t);
static void queue_and_serialize_statement_index (const char *field, struct control_flow_graph * cfg);
static void queue_and_serialize_type (const_tree t);
static void dequeue_and_serialize(void);
static void dequeue_and_serialize_gimple(void);
static void dequeue_and_serialize_statement (void);
static void serialize_new_line(void);
static void serialize_maybe_newline(void);
static void serialize_pointer (const char *field, void *ptr);
static void serialize_int (const char *field, int i);
static void serialize_wide_int (const char *field, HOST_WIDE_INT i);
static void serialize_real (tree t);
static int serialize_with_double_quote(const char * input, FILE * output, int length);
static int serialize_with_escape(const char * input, FILE * output, int length);
static void serialize_fixed (const char *field, const FIXED_VALUE_TYPE *f);
static void serialize_string (const char *string);
static void serialize_string_field (const char *field, const char *str);
static void serialize_gimple_dependent_stmts_load(GIMPLE_type gs);
static void add_referenced_var_map(tree var);
static void compute_referenced_var_map(void);


/**
 * Initialize structure to perform gimple serialization:
 * - open gimple serialization file; if it already exists open in append mode
 * - initialize global splay tree node if they have not yet been initialized
 * @param file_name is the name of the file to be written
 */
void SerializeGimpleBegin(char * file_name);

/**
 * Close gimple serialization file and remove gimples from splay tree tables
 */
void SerializeGimpleEnd(void);

/**
 * Serialize the header of a function declaration
 * @param fn is the function decl
 */
static void SerializeGimpleFunctionHeader(tree fn);

/**
 * Serialize a global symbol (function_decl or global var_decl
 * @param t is the symbol to be serialized
 */
void SerializeGimpleGlobalTreeNode(tree t);

/**
 * Add anti_dependence by dumping further vuse to the currently analyzed gimple
 * @param current is the currently analyzed gimple
 * @param gs is the other gimple
 */
void
SerializeGimpleUseDefs(const GIMPLE_type current, const GIMPLE_type next_def);

/**
 * Add output dependencies by dumping further vdef to the currently analyzed gimple
 * @param previous is potentially the first gimple of the pair
 * @param current is the currently analyzed gimple 
 */
void
SerializeGimpleDefsDef(const GIMPLE_type previous, const GIMPLE_type current);
void
SerializeGimpleDefsDef2(const GIMPLE_type current);

/**
 * Return the right operand of a gimple or NULL_TREE if this cannot be determined
 */
tree *
GetRightOperand(GIMPLE_type gs);

/**
 * Return the left operand of a gimple or NULL_TREE if this cannot be determined
 */
tree *
GetLeftOperand(const GIMPLE_type gs);

/**
 * Return true if the gimple has to be considered a barrier (i.e., it writes and reads everything
 */
bool
IsBarrier(const GIMPLE_type gs);

/**
 * Return true if the gimple corresponds to a pragma placeholder (function call)
 */
bool
IsPragma(const GIMPLE_type gs);

static void serialize_globals()
{
   struct varpool_node *vnode;

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
   FOR_EACH_DEFINED_VARIABLE (vnode)
   {
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
      tree var = vnode->decl;
#else
      tree var = vnode->symbol.decl;
#endif
#else
   vnode = varpool_nodes_queue;
   while (vnode)
   {
      tree var = vnode->decl;
      vnode = vnode->next_needed;
#endif
      if (TREE_CODE (var) == VAR_DECL && DECL_FILE_SCOPE_P(var))
      {
         SerializeGimpleGlobalTreeNode(var);
      }
   }

}

static void serialize_symbols (const char * field, bitmap syms);
static void serialize_vops (GIMPLE_type stmt);
static tree gimple_call_expr_arglist (GIMPLE_type g);
static tree build_custom_function_call_expr (location_t loc, tree fn, tree arglist);
static tree gimple_call_expr_arglist (GIMPLE_type g);
#if (__GNUC__ > 5)
static tree build_custom_function_call_expr_internal_fn (location_t loc, internal_fn fn, tree type, tree arglist);
extern tree build_call_expr_internal_loc_array (location_t loc, internal_fn ifn,
                tree type, int n, const tree *args);

#endif
static void
DumpGimpleWalker(void *arg ATTRIBUTE_UNUSED)
{
  if(di_local_nodes_index) ggc_mark(di_local_nodes_index);
  if(di_local_referenced_var_htab) ggc_mark(di_local_referenced_var_htab);
}

#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
static struct ggc_root_tab DumpGimpleRoot = { (char*)"", 1, 1, DumpGimpleWalker, NULL };
#endif

static void
DumpGimpleTreeWalker(void *arg ATTRIBUTE_UNUSED)
{
}

static struct ggc_root_tab DumpGimpleTreeRoot = {NULL , 1, 1, DumpGimpleTreeWalker, NULL };

void
DumpGimpleInit(char *_outdir_name)
{
    /* Register our GC root-walking callback: */
#if (__GNUC__ == 8)
    ggc_register_root_tab(gt_ggc_r_gtype_roots_gcc8_h);
#elif (__GNUC__ == 7)
    ggc_register_root_tab(gt_ggc_r_gtype_roots_gcc7_h);
#elif (__GNUC__ == 6)
    ggc_register_root_tab(gt_ggc_r_gtype_roots_gcc6_h);
#elif (__GNUC__ == 5)
    ggc_register_root_tab(gt_ggc_r_gtype_roots_gcc5_h);
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 9)
    ggc_register_root_tab(gt_ggc_r_gtype_roots_gcc49_h);
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 8)
    ggc_register_root_tab(gt_ggc_r_gtype_roots_gcc48_h);
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 7)
    ggc_register_root_tab(gt_ggc_r_gtype_roots_gcc47_h);
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 6)
    ggc_register_root_tab(gt_ggc_r_gtype_roots_gcc46_h);
#else
    ggc_register_root_tab(&DumpGimpleRoot);
#endif
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 9)
    ggc_register_cache_tab(gt_ggc_rc_gtype_roots_gcc49_h);
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 8)
    ggc_register_cache_tab(gt_ggc_rc_gtype_roots_gcc48_h);
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 7)
    ggc_register_cache_tab(gt_ggc_rc_gtype_roots_gcc47_h);
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 6)
    ggc_register_cache_tab(gt_ggc_rc_gtype_roots_gcc46_h);
#else
#endif
    ggc_register_root_tab(&DumpGimpleTreeRoot);
    di_local_index = 0;
    di_local_nodes_index = 0;
    di_local_referenced_var_htab = 0;
    outdir_name = _outdir_name;
}


/**
 * Dump the current version of gcc + current version of plugin
 */
static
void DumpVersion(FILE * stream)
{
   const char * panda_plugin_version = (const char *) PANDA_PLUGIN_VERSION;
   int version = __GNUC__, minor = __GNUC_MINOR__, patchlevel = __GNUC_PATCHLEVEL__;
   fprintf(stream, "GCC_VERSION: \"%d.%d.%d\"\n", version, minor, patchlevel);
   fprintf(stream, "PLUGIN_VERSION: \"%s\"\n", panda_plugin_version);
}


/**
 * Initialize structure to perform gimple serialization:
 * - open gimple serialization file; if it already exists open in append mode
 * - initialize global splay tree node if they have not yet been initialized
 * @param file_name is the name of the file to be written
 */
void
SerializeGimpleBegin(char * name)
{
   serialize_gimple_info.stream = fopen (name, serialize_state < 0 ? "w" : "a");
   serialize_gimple_info.node = 0;
   serialize_gimple_info.column = 0;
   serialize_gimple_info.queue = 0;
   serialize_gimple_info.queue_end = 0;
   serialize_gimple_info.free_list = 0;
   serialize_gimple_info.gimple_list = 0;
   serialize_gimple_info.cfg_list = 0;
   if (serialize_gimple_info.stream && serialize_state < 0)
   {
      DumpVersion(serialize_gimple_info.stream);
   }
   if (!serialize_gimple_info.stream)
      error ("could not open serialize file %qs: %m", name);
   else
      serialize_state = 1;
   free (name);
   /* initialization of di_local */
   if (!di_local_nodes_index)
   {
#if (__GNUC__ > 4) 
      di_local_nodes_index = hash_table<t2is_hasher>::create_ggc (128);
      di_local_referenced_var_htab = hash_table<dg_hasher>::create_ggc (1024);
#else
      di_local_nodes_index = htab_create_ggc (128, tree_2_ints_hash, tree_2_ints_eq, NULL);
      di_local_referenced_var_htab = htab_create_ggc (20, dg_descriptor_tree_hash, dg_descriptor_tree_eq, NULL);
#endif
   }
   else
   {
      /// check if DECL_UID changed for some of the tree nodes stored in di_local_referenced_var_htab
#if (__GNUC__ > 4)
      hash_table <dg_hasher>::iterator hi;
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 9)
      void ** slot;
      void **limit;
#else
      htab_iterator hi;
#endif
      struct dg_descriptor_tree *el;
      toberehashedlist_p head=NULL;
#if (__GNUC__ > 4)
      FOR_EACH_HASH_TABLE_ELEMENT (*di_local_referenced_var_htab, el, struct dg_descriptor_tree *, hi)
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 9)
      slot = di_local_referenced_var_htab->entries;
      limit = slot + htab_size (di_local_referenced_var_htab);
      do
      {
         void *currx;
         currx = *slot;

         if (currx != HTAB_EMPTY_ENTRY && currx != HTAB_DELETED_ENTRY)
         {
            el = (struct dg_descriptor_tree *)(*slot);
#else
      FOR_EACH_HTAB_ELEMENT (di_local_referenced_var_htab, el, struct dg_descriptor_tree *, hi)
#endif
      {
         if(DECL_UID(el->vd) != el->hash)
         {
            if(head)
            {
               toberehashedlist_p curr;
               curr = XNEW(struct toberehashedlist);
               curr->node = el->vd;
               curr->hash = el->hash;
               curr->next = head;
               head = curr;
            }
            else
            {
               head = XNEW(struct toberehashedlist);
               head->node = el->vd;
               head->hash = el->hash;
               head->next = NULL;
            }
         }
      }
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 9)
      }} while  (++slot < limit);
#endif
      while(head)
      {
         struct dg_descriptor_tree ** loc;
         struct dg_descriptor_tree * desc;
         struct dg_descriptor_tree in;
         in.hash = head->hash;
         in.vd = head->node;
         toberehashedlist_p prev_head = head;

#if (__GNUC__ > 4)
         di_local_referenced_var_htab->remove_elt(&in);
#else
         htab_remove_elt(di_local_referenced_var_htab, &in);
#endif
         in.hash = DECL_UID(in.vd);
#if (__GNUC__ > 4)
         loc = di_local_referenced_var_htab->find_slot_with_hash (&in,
                                                                  in.hash, INSERT);
#else
         loc = (struct dg_descriptor_tree **)htab_find_slot_with_hash (di_local_referenced_var_htab,
                                                                       &in, in.hash, INSERT);
#endif

         desc = *loc;

         if (desc == 0)
         {
#if (__GNUC__ > 4)
            desc = ggc_alloc<dg_descriptor_tree> ();
#else
#if (__GNUC__ == 4 && __GNUC_MINOR__ ==  6)
            desc = ((struct dg_descriptor_tree *)(ggc_internal_alloc_stat (sizeof (struct dg_descriptor_tree) MEM_STAT_INFO)));
#elif (__GNUC__ == 4 && __GNUC_MINOR__ > 6)
            desc = ggc_alloc_dg_descriptor_tree ();
#else
            desc = (struct dg_descriptor_tree*)ggc_alloc (sizeof(struct dg_descriptor_tree));
#endif
#endif
            desc->vd = in.vd;
            desc->hash = in.hash;
            *loc = desc;
         }
         /* now move to the next element */
         head = head->next;
         XDELETE(prev_head);
      }
   }
}


/**
 * Close gimple serialization file and remove gimples from splay tree tables
 */
void
SerializeGimpleEnd()
{
   /* Pointer to the current element of the queue */
   serialize_queue_p dq;

   /* Pointer to the next element of the queue */
   serialize_queue_p next_dq;

   struct tree_2_ints in;

   /* Now, clean up.  */
   for (dq = serialize_gimple_info.free_list; dq; dq = next_dq)
   {
      next_dq = dq->next;
      free (dq);
   }
   serialize_gimple_info.free_list=0;
   /* Now remove gimple objects. They could not be used anymore */
   for(dq = serialize_gimple_info.gimple_list; dq; dq = next_dq)
   {
      next_dq = dq->next;
      unsigned int ann = MAKE_ANN(0, 1, 0);
      in.key = dq->node_index->key;
      in.ann = ann;
#if (__GNUC__ > 4) 
      di_local_nodes_index->remove_elt(&in);
#else
      htab_remove_elt(di_local_nodes_index, &in);
#endif
      free (dq);
   }
   serialize_gimple_info.gimple_list=0;
   /* Now remove cfg objects. They could not be used anymore */
   for(dq = serialize_gimple_info.cfg_list; dq; dq = next_dq)
   {
      next_dq = dq->next;
      unsigned int ann = MAKE_ANN(0, 0, 1);
      in.key = dq->node_index->key;
      in.ann = ann;
#if (__GNUC__ > 4) 
      di_local_nodes_index->remove_elt(&in);
#else
      htab_remove_elt(di_local_nodes_index, &in);
#endif
      free (dq);
   }
   serialize_gimple_info.cfg_list = 0;
   fclose (serialize_gimple_info.stream);
}

unsigned int
gimplePssa (void)
{
   size_t nc = strlen(dump_base_name);
   size_t nc_index=0;
   size_t last_slash=0;
   while (nc_index<nc)
   {
      if(dump_base_name[nc_index] == '/')
         last_slash = nc_index+1;
      ++nc_index;
   }
   strcpy(dump_base_name_trimmed, dump_base_name+last_slash);

   /* dummy pass */
   if (!(cfun->curr_properties & PROP_cfg))
      fatal_error(INLOC "PROP_cfg marked in the pass descriptor but not found");
   if (!(cfun->curr_properties & PROP_ssa))
      fatal_error(INLOC "PROP_ssa marked in the pass descriptor but not found");
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8))
   if(cfun->cfg && basic_block_info_for_fn(cfun))
#else
   if(cfun->cfg && basic_block_info)
#endif
   {
      char * name = concat (outdir_name, "/", dump_base_name_trimmed, ".gimplePSSA", NULL);
      bool has_loop = current_loops != NULL;
      if(!has_loop)
         loop_optimizer_init (AVOID_CFG_MODIFICATIONS);
      SerializeGimpleBegin(name);
      compute_referenced_var_map();
      serialize_globals();
      SerializeGimpleGlobalTreeNode(cfun->decl);
      SerializeGimpleEnd();
      if(!has_loop)
         loop_optimizer_finalize ();
    }

 return 0;
}


static
void dump_pt_solution(struct pt_solution *pt, const char * first_tag, const char* second_tag)
{
    if (pt->anything)
        serialize_string_field (first_tag, "anything");
    if (pt->nonlocal)
        serialize_string_field (first_tag, "nonlocal");
    if (pt->escaped)
        serialize_string_field (first_tag, "escaped");
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
    if (pt->ipa_escaped)
        serialize_string_field (first_tag, "ipa_escaped");
#endif
    if (pt->null)
        serialize_string_field (first_tag, "null");

    if (pt->vars && !bitmap_empty_p (pt->vars))
        serialize_symbols(second_tag, pt->vars);

}


/* Add T to the end of the queue of nodes to serialize.  Returns the index
   assigned to T.  */
void index_insert(serialize_queue_p dq, unsigned int index, unsigned int ann, tree t)
{
   struct tree_2_ints min;
   min.key = (tree)t;
   min.ann = ann;
#if (__GNUC__ > 4)
   tree_2_ints ** slot;
#else
   void **slot;
#endif
#if (__GNUC__ > 4)
   slot = di_local_nodes_index->find_slot (&min, INSERT);
#else
   slot = htab_find_slot (di_local_nodes_index, &min, INSERT);
#endif
   gcc_assert (!*slot);
   struct tree_2_ints *m;
#if (__GNUC__ > 4)
       m = ggc_alloc<tree_2_ints> ();
#else
#if (__GNUC__ == 4 && __GNUC_MINOR__ ==  6)
       m = ((struct tree_2_ints *)(ggc_internal_alloc_stat (sizeof (struct tree_2_ints) MEM_STAT_INFO)));
#elif (__GNUC__ == 4 && __GNUC_MINOR__ > 6)
       m = ggc_alloc_tree_2_ints ();
#else
       m = (struct tree_2_ints*)ggc_alloc (sizeof(struct tree_2_ints));
#endif
#endif
   m->key = t;
   m->value = index;
   gcc_assert(index <= di_local_index);
   m->ann = ann;

#if (__GNUC__ > 4)
   *slot = m;
#else
   *slot = (void *) m;
#endif
   dq->node_index = m;
}

static unsigned int
queue (tree t)
{
  serialize_queue_p dq;
  unsigned int index;
  struct tree_2_ints in;
  unsigned int ann = MAKE_ANN(0, 0, 0);
  in.key = (tree) t;
  in.ann = ann;
#if (__GNUC__ > 4)
  assert (!di_local_nodes_index->find (&in));
#else
  assert (!htab_find (di_local_nodes_index, &in));
#endif

  /* Assign the next available index to T.  */
  index = ++di_local_index;

  /* Obtain a new queue node.  */
  if (serialize_gimple_info.free_list)
    {
      dq = serialize_gimple_info.free_list;
      serialize_gimple_info.free_list = dq->next;
    }
  else
    dq = XNEW (struct serialize_queue);

  /* Create a new entry in the hash table.  */
  index_insert(dq, index, ann, t);

  /* Add it to the end of the queue.  */
  dq->next = 0;
  if (!serialize_gimple_info.queue_end)
    serialize_gimple_info.queue = dq;
  else
    serialize_gimple_info.queue_end->next = dq;
  serialize_gimple_info.queue_end = dq;

  /* Return the index.  */
  return index;
}

/* Add G to the end of the queue of nodes to serialize.  Returns the index
   assigned to G.  */
static unsigned int
queue_gimple (GIMPLE_type g)
{
  serialize_queue_p dq, dq_gimple;
  unsigned int index;

  struct tree_2_ints in;
  unsigned int ann = MAKE_ANN(0, 1, 0);
  in.key = (tree) g;
  in.ann = ann;

#if (__GNUC__ > 4) 
  assert (!di_local_nodes_index->find (&in));
#else
  assert (!htab_find (di_local_nodes_index, &in));
#endif
  
  /* Assign the next available index to G.  */
  index = ++di_local_index;

  /* Obtain a new queue node.  */
  if (serialize_gimple_info.free_list)
    {
      dq = serialize_gimple_info.free_list;
      serialize_gimple_info.free_list = dq->next;
    }
  else
    dq = XNEW (struct serialize_queue);

  /* obtain another one for the gimple list */
  if (serialize_gimple_info.free_list)
    {
      dq_gimple = serialize_gimple_info.free_list;
      serialize_gimple_info.free_list = dq_gimple->next;
    }
  else
    dq_gimple = XNEW (struct serialize_queue);

  /* Create a new entry in the hash table.  */
  index_insert(dq, index, ann, (tree)g);
  dq_gimple->node_index = dq->node_index;

  /* Add it to the end of the queue.  */
  dq->next = 0;
  if (!serialize_gimple_info.queue_end)
    serialize_gimple_info.queue = dq;
  else
    serialize_gimple_info.queue_end->next = dq;
  serialize_gimple_info.queue_end = dq;

  /* Add in front of the gimple list.  */
  dq_gimple->next = serialize_gimple_info.gimple_list;
  serialize_gimple_info.gimple_list = dq_gimple;

  /* Return the index.  */
  return index;
}

/* Add CFG to the end of the queue of nodes to serialize.  Returns the index
   assigned to CFG.  */
static unsigned int
queue_statement (struct control_flow_graph * cfg)
{
  serialize_queue_p dq, dq_cfg;
  unsigned int index;
  struct tree_2_ints in;
  unsigned int ann = MAKE_ANN(0,0,1);
  in.key = (tree) cfg;
  in.ann = ann;
#if (__GNUC__ > 4)
  assert (!di_local_nodes_index->find (&in));
#else
  assert (!htab_find (di_local_nodes_index, &in));
#endif
  /* Assign the next available index to G.  */
  index = ++di_local_index;

  /* Obtain a new queue node.  */
  if (serialize_gimple_info.free_list)
    {
      dq = serialize_gimple_info.free_list;
      serialize_gimple_info.free_list = dq->next;
    }
  else
    dq = XNEW (struct serialize_queue);

  /* obtain another one for the cfg list */
  if (serialize_gimple_info.free_list)
    {
      dq_cfg = serialize_gimple_info.free_list;
      serialize_gimple_info.free_list = dq_cfg->next;
    }
  else
    dq_cfg = XNEW (struct serialize_queue);

  /* Create a new entry in the hash table.  */
  index_insert(dq, index, ann, (tree)cfg);
  dq_cfg->node_index = dq->node_index;

  /* Add it to the end of the queue.  */
  dq->next = 0;
  if (!serialize_gimple_info.queue_end)
    serialize_gimple_info.queue = dq;
  else
    serialize_gimple_info.queue_end->next = dq;
  serialize_gimple_info.queue_end = dq;

  /* Add in front of the gimple list.  */
  dq_cfg->next = serialize_gimple_info.cfg_list;
  serialize_gimple_info.cfg_list = dq_cfg;

  /* Return the index.  */
  return index;
}

static void
serialize_index (unsigned int index)
{
  fprintf (serialize_gimple_info.stream, "@%-6u ", index);
  serialize_gimple_info.column += 8;
}

/* If T has not already been output, queue it for subsequent output.
   FIELD is a string to print before printing the index.  Then, the
   index of T is printed.  */
static void
queue_and_serialize_index (const char *field, tree t)
{
  unsigned int index;
  struct tree_2_ints *n, in;
  /* If there's no node, just return.  This makes for fewer checks in
     our callers.  */
  if (!t)
    return;

  /* See if we've already queued or dumped this node.  */
  unsigned int ann = MAKE_ANN(0, 0, 0);
  in.key = t;
  in.ann = ann;
#if (__GNUC__ > 4) 
  n = di_local_nodes_index->find (&in);
#else
  n = (struct tree_2_ints *) htab_find (di_local_nodes_index, &in);
#endif
  if (n)
  {
     index = (unsigned int)n->value;
     gcc_assert(index <= di_local_index);
  }
  else
  {
    /* If we haven't, add it to the queue.  */
    index = queue (t);
  }

  /* Print the index of the node.  */
  serialize_maybe_newline ();
  fprintf (serialize_gimple_info.stream, "%-4s: ", field);
  serialize_gimple_info.column += 6;
  serialize_index (index);
}

/* If G has not already been output, queue it for subsequent output.
   FIELD is a string to print before printing the index.  Then, the
   index of G is printed.  */
static void
queue_and_serialize_gimple_index (const char *field, GIMPLE_type g)
{
  unsigned int index;
  struct tree_2_ints *n, in;

  /* If there's no node, just return.  This makes for fewer checks in
     our callers.  */
  if (!g)
    return;

  /* See if we've already queued or dumped this node.  */
  unsigned int ann = MAKE_ANN(0, 1, 0);
  in.key = (tree)g;
  in.ann = ann;
#if (__GNUC__ > 4)
  n = di_local_nodes_index->find (&in);
#else
  n = (struct tree_2_ints *)htab_find (di_local_nodes_index, &in);
#endif
  if (n)
  {
     index = (unsigned int)n->value;
     gcc_assert(index <= di_local_index);
  }
  else
  {
    /* If we haven't, add it to the queue.  */
    index = queue_gimple (g);
  }

  /* Print the index of the node.  */
  serialize_maybe_newline ();
  fprintf (serialize_gimple_info.stream, "%-4s: ", field);
  serialize_gimple_info.column += 6;
  serialize_index (index);
}

/* If T has not already been output, queue it for subsequent output.
   FIELD is a string to print before printing the index.  Then, the
   index of T is printed.  */
static void
queue_and_serialize_statement_index (const char *field, struct control_flow_graph * cfg)
{
  unsigned int index;
  struct tree_2_ints *n, in;

  /* If there's no node, just return.  This makes for fewer checks in
     our callers.  */
  if (!cfg)
    return;

  /* See if we've already queued or dumped this node.  */
  unsigned int ann = MAKE_ANN(0,0,1);
  in.key = (tree)cfg;
  in.ann = ann;
#if (__GNUC__ > 4)
  n = di_local_nodes_index->find (&in);
#else
  n = (struct tree_2_ints *)htab_find (di_local_nodes_index, &in);
#endif
  if (n)
  {
     index = (unsigned int)n->value;
     gcc_assert(index <= di_local_index);
  }
  else
  {
    /* If we haven't, add it to the queue.  */
    index = queue_statement (cfg);
  }

  /* Print the index of the node.  */
  serialize_maybe_newline ();
  fprintf (serialize_gimple_info.stream, "%-4s: ", field);
  serialize_gimple_info.column += 6;
  serialize_index (index);
}

/* Serialize the type of T.  */
static void
queue_and_serialize_type (const_tree t)
{
  queue_and_serialize_index ("type", TREE_TYPE (t));
}

/* Serialize column control */
#define SOL_COLUMN 25		/* Start of line column.  */
#define EOL_COLUMN 55		/* End of line column.  */
#define COLUMN_ALIGNMENT 15	/* Alignment.  */

/* Insert a new line in the serialize output, and indent to an appropriate
   place to start printing more fields.  */

static void
serialize_new_line ()
{
  fprintf (serialize_gimple_info.stream, "\n%*s", SOL_COLUMN, "");
  serialize_gimple_info.column = SOL_COLUMN;
}

/* If necessary, insert a new line.  */
static void
serialize_maybe_newline ()
{
  int extra;

  /* See if we need a new line.  */
  if (serialize_gimple_info.column > EOL_COLUMN)
    serialize_new_line ();
  /* See if we need any padding.  */
  else if ((extra = (serialize_gimple_info.column - SOL_COLUMN) % COLUMN_ALIGNMENT) != 0)
    {
      fprintf (serialize_gimple_info.stream, "%*s", COLUMN_ALIGNMENT - extra, "");
      serialize_gimple_info.column += COLUMN_ALIGNMENT - extra;
    }
}

/* Serialize pointer PTR using FIELD to identify it.  */
static void
serialize_pointer (const char *field, void *ptr)
{
  serialize_maybe_newline ();
  fprintf (serialize_gimple_info.stream, "%-4s: %-8llx ", field, (unsigned long long) ptr);
  serialize_gimple_info.column += 15;
}

/* Serialize integer I using FIELD to identify it.  */
static void
serialize_int (const char *field, int i)
{
  serialize_maybe_newline ();
  fprintf (serialize_gimple_info.stream, "%-4s: %-7d ", field, i);
  serialize_gimple_info.column += 14;
}

/* Serialize wide integer i using FIELD to identify it.  */
static void
serialize_wide_int (const char *field, HOST_WIDE_INT i)
{
   serialize_maybe_newline ();
   fprintf (serialize_gimple_info.stream, "%-4s: " HOST_WIDE_INT_PRINT_DEC, field, i);
   serialize_gimple_info.column += 21;
}

/* Serialize real r using FIELD to identify it.  */
static void
serialize_real (tree t)
{
#if !defined(REAL_IS_NOT_DOUBLE) || defined(REAL_ARITHMETIC)
   REAL_VALUE_TYPE d;
#endif
   static char string[1024];
   serialize_maybe_newline ();
   /* Code copied from print_node.  */
   if (TREE_OVERFLOW (t))
   {
      fprintf (serialize_gimple_info.stream, "%s ", "overflow");
      serialize_gimple_info.column += 8;
   }
#if !defined(REAL_IS_NOT_DOUBLE) || defined(REAL_ARITHMETIC)
   d = TREE_REAL_CST (t);
   if (REAL_VALUE_ISINF (d))
      fprintf (serialize_gimple_info.stream, "valr: %-7s ", "\"Inf\"");
   else if (REAL_VALUE_ISNAN (d))
      fprintf (serialize_gimple_info.stream, "valr: %-7s ", "\"Nan\"");
   else
   {
      real_to_decimal (string, &d, sizeof (string), 0, 1);
      fprintf (serialize_gimple_info.stream, "valr: \"%s\" ", string);
   }
#endif
   {
      fprintf (serialize_gimple_info.stream, "valx: \"");
      real_to_hexadecimal(string, &d, sizeof (string), 0, 0);
      fprintf (serialize_gimple_info.stream, "%s", string);
      fprintf (serialize_gimple_info.stream, "\"");
   }
   serialize_gimple_info.column += 21;
}

/* Serialize a string and add double quotes */
static int
serialize_with_double_quote(const char * input, FILE * output, int length)
{
   int new_length;
   fprintf(output, "\"");
   new_length = serialize_with_escape(input, output, length);
   fprintf(output, "\"");
   return new_length + 2;
}


/* Add a backslash before an escape sequence to serialize the string
   with the escape sequence */
static int
serialize_with_escape(const char * input, FILE * output, int length)
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
            fprintf(output, "\\");
            fprintf(output, "n");
            k += 2;
            break;
         }
         case '\t':
         {
            /* horizontal tab */
            fprintf(output, "\\");
            fprintf(output, "t");
            k += 2;
            break;
         }
         case '\v':
         {
            /* vertical tab */
            fprintf(output, "\\");
            fprintf(output, "v");
            k += 2;
            break;
         }
         case '\b':
         {
            /* backspace */
            fprintf(output, "\\");
            fprintf(output, "b");
            k += 2;
            break;
         }
         case '\r':
         {
            /* carriage return */
            fprintf(output, "\\");
            fprintf(output, "r");
            k += 2;
            break;
         }
         case '\f':
         {
            /* jump page */
            fprintf(output, "\\");
            fprintf(output, "f");
            k += 2;
            break;
         }
         case '\a':
         {
            /* alarm */
            fprintf(output, "\\");
            fprintf(output, "a");
            k += 2;
            break;
         }
         case '\\':
         {
            /* backslash */
            fprintf(output, "\\");
            fprintf(output, "\\");
            k += 2;
            break;
         }
         case '\"':
         {
            /* double quote */
            fprintf(output, "\\");
            fprintf(output, "\"");
            k += 2;
            break;
         }
         case '\'':
         {
            /* quote */
            fprintf(output, "\\");
            fprintf(output, "\'");
            k += 2;
            break;
         }
         case '\0':
         {
            /* null */
            fprintf(output, "\\");
            fprintf(output, "0");
            k += 2;
            break;
         }
         default:
         {
            fprintf(output, "%c", input[i]);
            k++;
         }
      }
   }
   return k;
}

/* Serialize the fixed-point value F, using FIELD to identify it.  */
static void
serialize_fixed (const char *field, const FIXED_VALUE_TYPE *f)
{
  char buf[32];
  fixed_to_decimal (buf, f, sizeof (buf));
  serialize_maybe_newline ();
  fprintf (serialize_gimple_info.stream, "%-4s: %s ", field, buf);
  serialize_gimple_info.column += strlen (buf) + 7;
}

/* Serialize the string S.  */
static void
serialize_string (const char *string)
{
  serialize_maybe_newline ();
  fprintf (serialize_gimple_info.stream, "%-13s ", string);
  if (strlen (string) > 13)
    serialize_gimple_info.column += strlen (string) + 1;
  else
    serialize_gimple_info.column += 14;
}

/* Serialize the string field S.  */
static void
serialize_string_field (const char *field, const char *str)
{
   int length;
   serialize_maybe_newline ();
   fprintf (serialize_gimple_info.stream, "%-4s: ", field);
   length = strlen(str);
   length = serialize_with_double_quote(str, serialize_gimple_info.stream, length);
   if (length > 7)
      serialize_gimple_info.column += 6 + length + 1;
   else
      serialize_gimple_info.column += 14;
}

/* Serialize symbols */
static void
serialize_symbols (const char * field, bitmap syms)
{
   unsigned uid;
   bitmap_iterator bi;

   if (syms == NULL)
      serialize_string_field (field, "NIL");
   else
   {

      EXECUTE_IF_SET_IN_BITMAP (syms, 0, uid, bi)
      {
         struct dg_descriptor_tree in;
         struct dg_descriptor_tree * h;
         in.hash = uid;

#if (__GNUC__ > 4) 
         h = di_local_referenced_var_htab->find_with_hash (&in, uid);
#else
         h = (struct dg_descriptor_tree *) htab_find_with_hash (di_local_referenced_var_htab, &in, uid);
#endif
         if(h)
         {
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
            if(!get_var_ann (h->vd)->is_heapvar)
#endif
               serialize_child (field, h->vd);
         }
#if 1
         else
            printf("unknown var %d\n", uid);
#endif
      }
   }
}
/* Serialize virtual operands */
static void
serialize_vops (GIMPLE_type gs)
{

   tree vdef = gimple_vdef (gs);
   tree vuse = gimple_vuse (gs);

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
   if (!ssa_operands_active (DECL_STRUCT_FUNCTION (current_function_decl)) || !gimple_references_memory_p (gs))
      return;
#else
   if (!ssa_operands_active () || !gimple_references_memory_p (gs))
      return;
#endif
   if (vuse != NULL_TREE)
   {
      serialize_child ("memuse", vuse);
   }
   if (vdef != NULL_TREE)
   {
      serialize_child ("memdef", vdef);
   }
   /// check for true dependencies by exploiting GCC alias analysis infrastructure
   if(vuse != NULL_TREE)
   {
      ///Serialize gimple pairs because of use after def chain
      serialize_gimple_dependent_stmts_load(gs);
      if(SSA_NAME_IS_DEFAULT_DEF(vuse))
         serialize_child ("vuse", vuse);
      ///Serialize gimple pairs because of def after use chain

      ///The other uses
      ssa_use_operand_t * other_uses = &(SSA_NAME_IMM_USE_NODE(vuse));

      ///Iterate over all the gimple using the same ssa virtual
      ssa_use_operand_t * ptr;
      for (ptr = other_uses->next; ptr != other_uses; ptr = ptr->next)
      {
         GIMPLE_type other_use_stmt = USE_STMT(ptr);
         ///If this is one of the next element of the chain
         if(gimple_vdef(other_use_stmt))
         {
            ///Check if the operation containing the next vdef in the chain is the current one, we do not need to add anti dependency
            if(other_use_stmt == gs)
            {
               continue;
            }
            SerializeGimpleUseDefs(gs, other_use_stmt);
         }
      }
   }

   if(vdef != NULL_TREE)
   {
      serialize_child ("vdef", vdef);
   }

   if(vdef != NULL_TREE && vuse != NULL_TREE)
   {
      ///The gimple which defines the use
      GIMPLE_type def_stmt = SSA_NAME_DEF_STMT (vdef);
      SerializeGimpleDefsDef(def_stmt, gs);
      SerializeGimpleDefsDef2(gs);

   }
}

static void
serialize_string_cst (const char *field, const char *str, int length, unsigned int precision)
{
   int new_length;
   serialize_maybe_newline ();
   if (precision == 8)
   {
      fprintf (serialize_gimple_info.stream, "%-4s: ", field);
      new_length = serialize_with_double_quote(str, serialize_gimple_info.stream, length - 1);
      if (new_length > 7)
         serialize_gimple_info.column += 6 + new_length + 1;
  else
    serialize_gimple_info.column += 14;
      serialize_int ("lngt", length);
   }
   else
   {
      const unsigned int * string = (const unsigned int *) str;
      unsigned int i, lngt = length / 4 - 1;
      fprintf (serialize_gimple_info.stream, "%-4s: \"", field);
      for (i = 0; i < lngt; i++)
         fprintf (serialize_gimple_info.stream, "\\x%x", string[i]);
      fprintf (serialize_gimple_info.stream, "\" ");
      serialize_gimple_info.column += 7 + lngt;
      serialize_int ("lngt", lngt + 1);
   }
}
#ifdef CPP_LANGUAGE
/* Dump a representation of the specific operator for an overloaded
   operator associated with node t.  */

static void
serialize_op (tree t)
{
#if (__GNUC__ > 7)
   switch (DECL_OVERLOADED_OPERATOR_CODE_RAW (t))
#else
   switch (DECL_OVERLOADED_OPERATOR_P (t))
#endif
   {
#if (__GNUC__ > 7)
      case OVL_OP_NEW_EXPR:
#else
      case NEW_EXPR:
#endif
         serialize_string ("new");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_VEC_NEW_EXPR:
#else
      case VEC_NEW_EXPR:
#endif
         serialize_string ("vecnew");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_DELETE_EXPR:
#else
      case DELETE_EXPR:
#endif
         serialize_string ("delete");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_VEC_DELETE_EXPR:
#else
      case VEC_DELETE_EXPR:
#endif
         serialize_string ("vecdelete");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_UNARY_PLUS_EXPR:
#else
      case UNARY_PLUS_EXPR:
#endif
         serialize_string ("pos");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_NEGATE_EXPR:
#else
      case NEGATE_EXPR:
#endif
         serialize_string ("neg");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_ADDR_EXPR:
#else
      case ADDR_EXPR:
#endif
         serialize_string ("addr");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_INDIRECT_REF:
#else
      case INDIRECT_REF:
#endif
         serialize_string("deref");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_BIT_NOT_EXPR:
#else
      case BIT_NOT_EXPR:
#endif
         serialize_string("not");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_TRUTH_NOT_EXPR:
#else
      case TRUTH_NOT_EXPR:
#endif
         serialize_string("lnot");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_PREINCREMENT_EXPR:
#else
      case PREINCREMENT_EXPR:
#endif
         serialize_string("preinc");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_PREDECREMENT_EXPR:
#else
      case PREDECREMENT_EXPR:
#endif
         serialize_string("predec");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_PLUS_EXPR:
#else
      case PLUS_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("plusassign");
         else
            serialize_string("plus");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_MINUS_EXPR:
#else
      case MINUS_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("minusassign");
         else
            serialize_string("minus");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_MULT_EXPR:
#else
      case MULT_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("multassign");
         else
            serialize_string ("mult");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_TRUNC_DIV_EXPR:
#else
      case TRUNC_DIV_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("divassign");
         else
            serialize_string ("div");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_TRUNC_MOD_EXPR:
#else
      case TRUNC_MOD_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("modassign");
         else
            serialize_string ("mod");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_BIT_AND_EXPR:
#else
      case BIT_AND_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("andassign");
         else
            serialize_string ("and");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_BIT_IOR_EXPR:
#else
      case BIT_IOR_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("orassign");
         else
            serialize_string ("or");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_BIT_XOR_EXPR:
#else
      case BIT_XOR_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("xorassign");
         else
            serialize_string ("xor");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_LSHIFT_EXPR:
#else
      case LSHIFT_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("lshiftassign");
         else
            serialize_string ("lshift");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_RSHIFT_EXPR:
#else
      case RSHIFT_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("rshiftassign");
         else
            serialize_string ("rshift");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_EQ_EXPR:
#else
      case EQ_EXPR:
#endif
         serialize_string ("eq");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_NE_EXPR:
#else
      case NE_EXPR:
#endif
         serialize_string ("ne");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_LT_EXPR:
#else
      case LT_EXPR:
#endif
         serialize_string ("lt");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_GT_EXPR:
#else
      case GT_EXPR:
#endif
         serialize_string ("gt");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_LE_EXPR:
#else
      case LE_EXPR:
#endif
         serialize_string ("le");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_GE_EXPR:
#else
      case GE_EXPR:
#endif
         serialize_string ("ge");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_TRUTH_ANDIF_EXPR:
#else
      case TRUTH_ANDIF_EXPR:
#endif
         serialize_string ("land");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_TRUTH_ORIF_EXPR:
#else
      case TRUTH_ORIF_EXPR:
#endif
         serialize_string ("lor");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_COMPOUND_EXPR:
#else
      case COMPOUND_EXPR:
#endif
         serialize_string ("compound");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_MEMBER_REF:
#else
      case MEMBER_REF:
#endif
         serialize_string ("memref");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_COMPONENT_REF:
#else
      case COMPONENT_REF:
#endif
         serialize_string ("ref");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_ARRAY_REF:
#else
      case ARRAY_REF:
#endif
         serialize_string ("subs");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_POSTINCREMENT_EXPR:
#else
      case POSTINCREMENT_EXPR:
#endif
         serialize_string ("postinc");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_POSTDECREMENT_EXPR:
#else
      case POSTDECREMENT_EXPR:
#endif
         serialize_string ("postdec");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_CALL_EXPR:
#else
      case CALL_EXPR:
#endif
         serialize_string ("call");
         break;
#if (__GNUC__ > 7)
      case OVL_OP_NOP_EXPR:
#else
      case NOP_EXPR:
#endif
         if (DECL_ASSIGNMENT_OPERATOR_P (t))
            serialize_string ("assign");
         break;
      default:
         break;
  }
}

/* Dump information common to statements from STMT.  */

static void
serialize_stmt (const_tree t)
{
  if (EXPR_HAS_LOCATION (t))
    serialize_int ("line", EXPR_LINENO (t));
}


static bool serialize_cpp_specifics(tree t)
{
   enum tree_code code;

   tree templ_t = NULL_TREE;
   tree templ = NULL_TREE;
   tree template_args = NULL_TREE;
   tree template_parms = NULL_TREE;
gcc_assert(t!=0);
   /* Figure out what kind of node this is.  */
   code = TREE_CODE (t);

   if (DECL_P (t))
   {
      templ = t;
      templ_t = t;
      if (TREE_CODE (templ_t) == TEMPLATE_DECL)
         templ_t = DECL_TEMPLATE_RESULT (templ_t);

      /* Print template instantiations only.  */
      if (DECL_LANG_SPECIFIC (templ_t))
      {
         int i = 0;
         /* Indicates whether or not (and how) a template was expanded for this
                    FUNCTION_DECL or VAR_DECL or TYPE_DECL.
                      0=normal declaration, e.g. int min (int, int);
                      1=implicit template instantiation
                      2=explicit template specialization, e.g. int min<int> (int, int);
                      3=explicit template instantiation, e.g. template int min<int> (int, int);  */
         i = DECL_USE_TEMPLATE(templ_t);

         if (TREE_CODE(templ) == VAR_DECL)
            serialize_int ("use_tmpl", i);

         if ((TREE_CODE(templ) == VAR_DECL) || (TREE_CODE(templ) == FUNCTION_DECL)
             || (TREE_CODE(templ) == TYPE_DECL) )
         {


            if ((DECL_TEMPLATE_INFO (templ_t) != NULL_TREE) && (i != 0))
            {
               tree tmpl;

               template_args = DECL_TI_ARGS (templ_t);

               tmpl = most_general_template (templ_t);

               if (tmpl && TREE_CODE (tmpl) == TEMPLATE_DECL)
               {
                  template_parms = DECL_TEMPLATE_PARMS (tmpl);

               }
            }
         }
      }
   }

   if (TREE_CODE(t) == RECORD_TYPE && CLASS_TYPE_P(t))
   {
      int i = 0;
      /* Indicates whether or not (and how) a template was expanded for this class.
                 0=no information yet/non-template class
                 1=implicit template instantiation
                 2=explicit template specialization
                 3=explicit template instantiation  */
      i = CLASSTYPE_USE_TEMPLATE(t);

      if ((CLASSTYPE_TEMPLATE_INFO (t) != NULL_TREE) && (i != 0))
      {
         tree tmpl;

         template_args = CLASSTYPE_TI_ARGS (t);

         tmpl = CLASSTYPE_TI_TEMPLATE(t);

         if (tmpl && TREE_CODE (tmpl) == TEMPLATE_DECL)
         {
            template_parms = DECL_TEMPLATE_PARMS (tmpl);

         }
      }

      if (template_parms)
      {
         serialize_child ("tmpl_parms", template_parms);
         serialize_child ("tmpl_args", template_args);
      }
   }

   switch (code)
   {
      case IDENTIFIER_NODE:
#if (__GNUC__ > 7)
         if (IDENTIFIER_ANY_OP_P (t))
#else
         if (IDENTIFIER_OPNAME_P (t))
#endif
         {
            serialize_string ("operator");
            return true;
         }
#if (__GNUC__ > 7)
         else if (IDENTIFIER_CONV_OP_P (t))
#else
         else if (IDENTIFIER_TYPENAME_P (t))
#endif
         {
            serialize_child ("tynm", TREE_TYPE (t));
            return true;
         }
         break;

      case OFFSET_TYPE:
         serialize_string ("ptrmem");
         serialize_child ("ptd", TYPE_PTRMEM_POINTED_TO_TYPE (t));
         serialize_child ("cls", TYPE_PTRMEM_CLASS_TYPE (t));
         return true;

      case RECORD_TYPE:
         if (TYPE_PTRMEMFUNC_P (t))
         {
            serialize_string ("ptrmem");
            serialize_child ("ptd", TYPE_PTRMEM_POINTED_TO_TYPE (t));
            serialize_child ("cls", TYPE_PTRMEM_CLASS_TYPE (t));
            return true;
         }
         /* Fall through.  */

      case UNION_TYPE:
         /* Is it a type used as a base? */
         if (TYPE_CONTEXT (t) && TREE_CODE (TYPE_CONTEXT (t)) == TREE_CODE (t)
             && CLASSTYPE_AS_BASE (TYPE_CONTEXT (t)) == t)
         {
            serialize_child ("bfld", TYPE_CONTEXT (t));
            return true;
         }

         if (! MAYBE_CLASS_TYPE_P (t))
            break;

         serialize_child ("vfld", TYPE_VFIELD (t));
         if (CLASSTYPE_TEMPLATE_SPECIALIZATION(t))
            serialize_string("spec");

         break;

      case FIELD_DECL:
         if (DECL_MUTABLE_P (t))
            serialize_string ("mutable");
         break;

      case FUNCTION_DECL:
         if (!DECL_THUNK_P (t))
         {
            if (DECL_OVERLOADED_OPERATOR_P (t))
            {
               serialize_string ("operator");
               serialize_op (t);
            }
            if (LANG_DECL_FN_CHECK(t) && DECL_FUNCTION_MEMBER_P (t))
            {
               serialize_string ("member");
            }
            if (LANG_DECL_FN_CHECK(t) && DECL_PURE_VIRTUAL_P (t))
               serialize_string ("pure");
            if (DECL_VIRTUAL_P (t))
               serialize_string ("virtual");
            if (DECL_CONSTRUCTOR_P (t))
               serialize_string ("constructor");
            if (DECL_DESTRUCTOR_P (t))
               serialize_string ("destructor");
            if (DECL_CONV_FN_P (t))
               serialize_string ("conversion");
            if (LANG_DECL_FN_CHECK(t) && DECL_GLOBAL_CTOR_P (t))
               serialize_string ("global_init");
            if (LANG_DECL_FN_CHECK(t) && DECL_GLOBAL_DTOR_P (t))
               serialize_string ("global_fini");
           if (DECL_LANG_SPECIFIC (t) && DECL_FRIEND_PSEUDO_TEMPLATE_INSTANTIATION (t))
               serialize_string ("pseudo_tmpl");
         }
         else
         {
            tree virt = THUNK_VIRTUAL_OFFSET (t);

            serialize_string ("thunk");
            if (DECL_THIS_THUNK_P (t))
               serialize_string ("this_adjusting");
            else
            {
               serialize_string ("result_adjusting");
               if (virt)
                  virt = BINFO_VPTR_FIELD (virt);
            }
            serialize_int ("fixd", THUNK_FIXED_OFFSET (t));
            if (virt)
               serialize_int ("virt", TREE_INT_CST_LOW (virt));
            serialize_child ("fn", DECL_INITIAL (t));
         }
         break;

      case TYPE_DECL:
      {
         if (template_parms)
         {
            serialize_child ("tmpl_parms", template_parms);
            serialize_child ("tmpl_args", template_args);
         }

      }
         break;

      case NAMESPACE_DECL:
         if (DECL_NAMESPACE_ALIAS (t))
            serialize_child ("alis", DECL_NAMESPACE_ALIAS (t));
         else
            serialize_child ("dcls", NAMESPACE_LEVEL (t)->names);
         break;

      case TEMPLATE_DECL:
         serialize_child ("rslt", DECL_TEMPLATE_RESULT (t));
         serialize_child ("inst", DECL_TEMPLATE_INSTANTIATIONS (t));
         serialize_child ("spcs", DECL_TEMPLATE_SPECIALIZATIONS (t));
         serialize_child ("prms", DECL_TEMPLATE_PARMS (t));
         break;

      case TEMPLATE_PARM_INDEX:
         serialize_child ("type", TREE_TYPE (t));
         serialize_child ("decl", TEMPLATE_PARM_DECL (t));
         if(TREE_CONSTANT(t))
            serialize_string ("constant");
         if(TREE_READONLY(t))
            serialize_string ("readonly");
         serialize_int("index", TEMPLATE_PARM_IDX(t));
         serialize_int("level", TEMPLATE_PARM_LEVEL(t));
         serialize_int("orig_level", TEMPLATE_PARM_ORIG_LEVEL(t));
         break;

      case OVERLOAD:
#if (__GNUC__ > 7)
         serialize_child ("crnt", OVL_FIRST (t));
#else
         serialize_child ("crnt", OVL_CURRENT (t));
#endif
         serialize_child ("chan", OVL_CHAIN (t));
         break;

      case TRY_BLOCK:
         serialize_stmt (t);
         if (CLEANUP_P (t))
            serialize_string ("cleanup");
         serialize_child ("body", TRY_STMTS (t));
         serialize_child ("hdlr", TRY_HANDLERS (t));
         break;

      case EH_SPEC_BLOCK:
         serialize_stmt (t);
         serialize_child ("body", EH_SPEC_STMTS (t));
         serialize_child ("raises", EH_SPEC_RAISES (t));
         break;

      case PTRMEM_CST:
         serialize_child ("clas", PTRMEM_CST_CLASS (t));
         serialize_child ("mbr", PTRMEM_CST_MEMBER (t));
         break;

      case THROW_EXPR:
         /* These nodes are unary, but do not have code class `1'.  */
         serialize_child ("op", TREE_OPERAND (t, 0));
         break;

      case AGGR_INIT_EXPR:
      {
         tree arg;
         aggr_init_expr_arg_iterator iter;
         serialize_child ("fn", AGGR_INIT_EXPR_FN (t));
         FOR_EACH_AGGR_INIT_EXPR_ARG (arg, iter, t)
         {
            serialize_child ("arg", arg);
         }
         serialize_int ("ctor", AGGR_INIT_VIA_CTOR_P (t));
         serialize_child ("slot", AGGR_INIT_EXPR_SLOT (t));
         break;
      }
      case HANDLER:
         serialize_stmt (t);
         serialize_child ("parm", HANDLER_PARMS (t));
         serialize_child ("body", HANDLER_BODY (t));
         break;

      case MUST_NOT_THROW_EXPR:
         serialize_stmt (t);
         serialize_child ("body", TREE_OPERAND (t, 0));
         break;

      case USING_STMT:
         serialize_stmt (t);
         serialize_child ("nmsp", USING_STMT_NAMESPACE (t));
         break;

      case CLEANUP_STMT:
         serialize_stmt (t);
         serialize_child ("decl", CLEANUP_DECL (t));
         serialize_child ("expr", CLEANUP_EXPR (t));
         serialize_child ("body", CLEANUP_BODY (t));
         break;

      case IF_STMT:
         serialize_stmt (t);
         serialize_child ("cond", IF_COND (t));
         serialize_child ("then", THEN_CLAUSE (t));
         serialize_child ("else", ELSE_CLAUSE (t));
         break;

      case BREAK_STMT:
      case CONTINUE_STMT:
         serialize_stmt (t);
         break;

      case DO_STMT:
         serialize_stmt (t);
         serialize_child ("body", DO_BODY (t));
         serialize_child ("cond", DO_COND (t));
         break;

      case FOR_STMT:
         serialize_stmt (t);
         serialize_child ("init", FOR_INIT_STMT (t));
         serialize_child ("cond", FOR_COND (t));
         serialize_child ("expr", FOR_EXPR (t));
         serialize_child ("body", FOR_BODY (t));
         break;

      case SWITCH_STMT:
         serialize_stmt (t);
         serialize_child ("cond", SWITCH_STMT_COND (t));
         serialize_child ("body", SWITCH_STMT_BODY (t));
         break;

      case WHILE_STMT:
         serialize_stmt (t);
         serialize_child ("cond", WHILE_COND (t));
         serialize_child ("body", WHILE_BODY (t));
         break;

      case STMT_EXPR:
         serialize_child ("stmt", STMT_EXPR_STMT (t));
         break;

      case EXPR_STMT:
         serialize_stmt (t);
         serialize_child ("expr", EXPR_STMT_EXPR (t));
         break;
      case SCOPE_REF:
         serialize_child ("op", TREE_OPERAND (t, 0));
         serialize_child ("op", TREE_OPERAND (t, 1));
         break;
      case TYPE_ARGUMENT_PACK:
         serialize_child ("arg", ARGUMENT_PACK_ARGS(t) );
         break;
      case NONTYPE_ARGUMENT_PACK:
         serialize_child ("arg", ARGUMENT_PACK_ARGS(t) );
         break;
      case TYPE_PACK_EXPANSION:
      case EXPR_PACK_EXPANSION:
         serialize_child ("op", PACK_EXPANSION_PATTERN(t) );
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
         serialize_child ("param_packs", PACK_EXPANSION_PARAMETER_PACKS(t) );
         serialize_child ("arg", PACK_EXPANSION_EXTRA_ARGS(t) );
#endif
         break;
      default:
         break;
   }
   return false;

}
#endif

/* Serialize the next node in the queue.  */
static void
dequeue_and_serialize ()
{
  serialize_queue_p dq;
  struct tree_2_ints * stn_index;
  tree t, arg;
  unsigned int index;
  enum tree_code code;
  enum tree_code_class code_class;
  const char* code_name;
  int ann;

  /* Get the next node from the queue.  */
  dq = serialize_gimple_info.queue;
  stn_index = dq->node_index;
  index = (int) stn_index->value;
  gcc_assert(index <= di_local_index);
  ann = stn_index->ann;
  if(IS_GIMPLE(ann))
  {
    dequeue_and_serialize_gimple ();
    return;
  }
  else if(IS_STATEMENT(ann))
  {
    dequeue_and_serialize_statement ();
    return;
  }

  t = (tree) stn_index->key;

  /* Remove the node from the queue, and put it on the free list.  */
  serialize_gimple_info.queue = dq->next;
  if (!serialize_gimple_info.queue)
    serialize_gimple_info.queue_end = 0;
  dq->next = serialize_gimple_info.free_list;
  serialize_gimple_info.free_list = dq;

  /* Print the node index.  */
  serialize_index (index);

  /* And the type of node this is.  */
  if (IS_BINFO(ann))
    code_name = "binfo";
  else if(TREE_CODE (t) == TARGET_MEM_REF)
    code_name = "target_mem_ref461";
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 && __GNUC_PATCHLEVEL__>= 1)
  else if(TREE_CODE (t) == BIT_NOT_EXPR && TREE_CODE (TREE_TYPE (t)) == BOOLEAN_TYPE) /*incorrect encoding of not of a boolean expression*/
    code_name ="truth_not_expr";
#endif
#if __GNUC__ > 7
  else if(TREE_CODE (t) == POINTER_DIFF_EXPR)
     code_name ="minus_expr";
#endif
  else
    code_name = GET_TREE_CODE_NAME(TREE_CODE (t));
  fprintf (serialize_gimple_info.stream, "%-16s ", code_name);
  serialize_gimple_info.column = 25;

  /* Figure out what kind of node this is.  */
  code = TREE_CODE (t);
  code_class = TREE_CODE_CLASS (code);

   /* We use has_stmt_ann because CALL_EXPR can be both an expression
      and a statement, and we have no guarantee that it will have a
      stmt_ann when it is used as an RHS expression.  stmt_ann will assert
      if you call it on something with a non-stmt annotation attached.  */

  // if (code != ERROR_MARK
  //       && is_gimple_stmt (t)
  //       && has_stmt_ann (t)
  //       && code != PHI_NODE)
  // {
  //    serialize_vops(t);
  // }

  /* Although BINFOs are TREE_VECs, we serialize them specially so as to be
     more informative.  */
  if (IS_BINFO(ann))
  {
    unsigned ix;
    tree base;
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
    vec<tree,va_gc> *accesses = BINFO_BASE_ACCESSES (t);
#else
    VEC(tree,gc) *accesses = BINFO_BASE_ACCESSES (t);
#endif
    serialize_child ("type", BINFO_TYPE (t));

    if (BINFO_VIRTUAL_P (t))
      serialize_string ("virt");

    serialize_int ("bases", BINFO_N_BASE_BINFOS (t));
    for (ix = 0; BINFO_BASE_ITERATE (t, ix, base); ix++)
    {
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
      tree access = (accesses ? (*accesses)[ix] : access_public_node);
#else
      tree access = (accesses ? VEC_index (tree, accesses, ix) : access_public_node);
#endif
      const char *string = NULL;

      if (access == access_public_node)
        string = "pub";
      else if (access == access_protected_node)
        string = "prot";
      else if (access == access_private_node)
        string = "priv";
      else
        gcc_unreachable ();

      serialize_string (string);
    }

    goto done;
  }

  /* We can knock off a bunch of expression nodes in exactly the same
     way.  */
  if (IS_EXPR_CODE_CLASS (code_class))
  {
    /* If we're serializing children, serialize them now.  */
    if (!(code == MODIFY_EXPR))
      queue_and_serialize_type (t);
      if (EXPR_HAS_LOCATION(t))
      {
        serialize_maybe_newline ();
        fprintf (serialize_gimple_info.stream, "srcp: \"%s\":%-d:%-6d ", EXPR_FILENAME(t), EXPR_LINENO(t), EXPR_COLUMNNO(t));
        serialize_gimple_info.column += 12 + strlen(EXPR_FILENAME(t)) + 8;
     }

     switch (code_class)
     {
       case tcc_unary:
         serialize_child ("op", TREE_OPERAND (t, 0));
         break;

       case tcc_binary:
       case tcc_comparison:
        {
#if __GNUC__ > 7
           if(TREE_CODE (t) == POINTER_DIFF_EXPR)
           {
              serialize_child ("op", build1 (NOP_EXPR, TREE_TYPE (t), TREE_OPERAND (t, 0)));
              serialize_child ("op", build1 (NOP_EXPR, TREE_TYPE (t), TREE_OPERAND (t, 1)));
           }
           else
#endif
           {
              serialize_child ("op", TREE_OPERAND (t, 0));
              serialize_child ("op", TREE_OPERAND (t, 1));
           }
         break;
        }
       case tcc_expression:
       case tcc_reference:
       case tcc_statement:
       case tcc_vl_exp:
         /* These nodes are handled explicitly below.  */
         break;

       default:
         gcc_unreachable ();
    }
  }
  else if (DECL_P (t))
  {
    expanded_location xloc;
    /* All declarations have names.  */
    if (DECL_NAME (t))
      serialize_child ("name", DECL_NAME (t));
    if (HAS_DECL_ASSEMBLER_NAME_P (t)
        && DECL_ASSEMBLER_NAME_SET_P (t) && DECL_ASSEMBLER_NAME (t) != DECL_NAME (t))
      serialize_child ("mngl", DECL_ASSEMBLER_NAME (t));
    if (DECL_ABSTRACT_ORIGIN (t))
      serialize_child ("orig", DECL_ABSTRACT_ORIGIN (t));
    /* And types.  */
    queue_and_serialize_type (t);
    serialize_child ("scpe", DECL_CONTEXT (t));
    if (!DECL_SOURCE_LOCATION (t))
    {
      serialize_maybe_newline ();
      fprintf (serialize_gimple_info.stream, "srcp: \"<built-in>\":0:0 ");
      serialize_gimple_info.column += 12 + strlen ("<built-in>") + 8;
    }
    else
    {
       /* And a source position.  */
       xloc = expand_location (DECL_SOURCE_LOCATION (t));
       if (xloc.file)
       {
          serialize_maybe_newline ();
          fprintf (serialize_gimple_info.stream, "srcp: \"%s\":%-d:%-6d ", xloc.file,
                xloc.line, xloc.column);
          serialize_gimple_info.column += 12 + strlen (xloc.file) + 8;
       }
    }
    /* And any declaration can be compiler-generated.  */
    if (CODE_CONTAINS_STRUCT (TREE_CODE (t), TS_DECL_COMMON) && DECL_ARTIFICIAL (t))
       serialize_string ("artificial");
    if (code == LABEL_DECL && LABEL_DECL_UID(t) != -1)
       serialize_int("uid", LABEL_DECL_UID (t));
    else
       serialize_int("uid", DECL_UID(t));
    /*if (DECL_ATTRIBUTES(t) && !MTAG_P(t))
      {
      serialize_child("attributes", DECL_ATTRIBUTES(t));
      }*/
  }
  else if (code_class == tcc_type)
  {
     /* All types have qualifiers.  */
     int quals = lang_hooks.tree_dump.type_quals (t);

     if (quals != TYPE_UNQUALIFIED)
     {
        if(quals & TYPE_QUAL_VOLATILE)
           fprintf (serialize_gimple_info.stream, "qual: %c%c%c     ",
                 (quals & TYPE_QUAL_CONST) ? 'c' : ' ',
                 'v',
                 (quals & TYPE_QUAL_RESTRICT) ? 'r' : ' ');
        else
           fprintf (serialize_gimple_info.stream, "qual: %c%c     ",
                 (quals & TYPE_QUAL_CONST) ? 'c' : ' ',
                 (quals & TYPE_QUAL_RESTRICT) ? 'r' : ' ');
        serialize_gimple_info.column += 14;
     }

     /* All types have associated declarations.  */
     serialize_child ("name", TYPE_NAME (t));

     /* All types have a main variant.  */
     if (TYPE_MAIN_VARIANT (t) != t)
        serialize_child ("unql", TYPE_MAIN_VARIANT (t));

     /* And sizes.  */
     serialize_child ("size", TYPE_SIZE (t));

     /* All types have context.  */ /*NB TBC*/
     serialize_child ("scpe", TYPE_CONTEXT (t));/*NB TBC*/

     /* All types have alignments.  */
     serialize_int ("algn", TYPE_ALIGN (t));

     if(TYPE_PACKED(t))
     {
        serialize_string("packed");
     }
  }
  else if (code_class == tcc_constant)
     /* All constants can have types.  */
     queue_and_serialize_type (t);

  /* Give the language-specific code a chance to print something.  If
     it's completely taken care of things, don't bother printing
     anything more ourselves.
     */
#ifdef CPP_LANGUAGE
  if(serialize_cpp_specifics(t))
     goto done;
#endif
  /* Now handle the various kinds of nodes.  */
  switch (code)
    {
      int i;

    case IDENTIFIER_NODE:
      serialize_string_field ("strg", IDENTIFIER_POINTER (t));
      serialize_int ("lngt", IDENTIFIER_LENGTH (t));
      break;

    case TREE_LIST:
      serialize_child ("purp", TREE_PURPOSE (t));
      serialize_child ("valu", TREE_VALUE (t));
      serialize_child ("chan", TREE_CHAIN (t));
      break;

    case STATEMENT_LIST:
      {
	tree_stmt_iterator it;
	for (i = 0, it = tsi_start (t); !tsi_end_p (it); tsi_next (&it), i++)
	  {
	    char buffer[32];
	    sprintf (buffer, "%u", i);
	    serialize_child (buffer, tsi_stmt (it));
	  }
      }
      break;

    case TREE_VEC:
      serialize_int ("lngt", TREE_VEC_LENGTH (t));
      for (i = 0; i < TREE_VEC_LENGTH (t); ++i)
	{
	  serialize_child ("op", TREE_VEC_ELT (t, i));
	}
      break;

    case INTEGER_TYPE:
    case ENUMERAL_TYPE:
      serialize_int ("prec", TYPE_PRECISION (t));
      if (TYPE_UNSIGNED (t))
        serialize_string ("unsigned");
      serialize_child ("min", TYPE_MIN_VALUE (t));
      serialize_child ("max", TYPE_MAX_VALUE (t));

      if (code == ENUMERAL_TYPE)
	serialize_child ("csts", TYPE_VALUES (t));
      break;

    case COMPLEX_TYPE:
      if (TYPE_UNSIGNED (t))
        serialize_string ("unsigned");
      if(COMPLEX_FLOAT_TYPE_P(t))
         serialize_string ("real");
      break;

    case REAL_TYPE:
      serialize_int ("prec", TYPE_PRECISION (t));
      break;

    case FIXED_POINT_TYPE:
      serialize_int ("prec", TYPE_PRECISION (t));
      serialize_string_field ("sign", TYPE_UNSIGNED (t) ? "unsigned": "signed");
      serialize_string_field ("saturating",
			 TYPE_SATURATING (t) ? "saturating": "non-saturating");
      break;

    case POINTER_TYPE:
      serialize_child ("ptd", TREE_TYPE (t));
      break;

    case REFERENCE_TYPE:
      serialize_child ("refd", TREE_TYPE (t));
      break;

    case METHOD_TYPE:
      serialize_child ("retn", TREE_TYPE (t));
      serialize_child ("prms", TYPE_ARG_TYPES (t));
      serialize_child ("clas", TYPE_METHOD_BASETYPE (t));
      break;

    case FUNCTION_TYPE:
      serialize_child ("retn", TREE_TYPE (t));
      serialize_child ("prms", TYPE_ARG_TYPES (t));
      if (stdarg_p(t))
        serialize_string("varargs");
      break;

    case ARRAY_TYPE:
      serialize_child ("elts", TREE_TYPE (t));
      serialize_child ("domn", TYPE_DOMAIN (t));
      break;

    case VECTOR_TYPE:
      serialize_child ("elts", TREE_TYPE (t));
      break;

    case RECORD_TYPE:
    case UNION_TYPE:
    {
      tree op;
      if (TREE_CODE (t) == RECORD_TYPE)
	serialize_string ("struct");
      else
	serialize_string ("union");
      if (TYPE_FIELDS(t))
         for ( op = TYPE_FIELDS (t); op; op = TREE_CHAIN(op))
         {
            if(TREE_CODE(op) == FIELD_DECL || TREE_CODE (t) == UNION_TYPE)
               serialize_child ("flds", op);
            else
               serialize_child ("fncs", op);
         }

#if __GNUC__ < 8
      if (TYPE_METHODS(t))
         for ( op = TYPE_METHODS (t); op; op = TREE_CHAIN(op))
         {
            serialize_child ("fncs", op);
         }
#endif
      /*serialize_child ("flds", TYPE_FIELDS (t));
      serialize_child ("fncs", TYPE_METHODS (t));*/
      break;
    }
    case CONST_DECL:
      serialize_child ("cnst", DECL_INITIAL (t));
      break;
    case SSA_NAME:
    {
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#else
      size_t index_ssa;
#endif
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
      queue_and_serialize_type (t);
#else
      if(SSA_NAME_VAR (t))
         queue_and_serialize_type (SSA_NAME_VAR (t));
#endif
      serialize_child ("var", SSA_NAME_VAR (t));
      serialize_int ("vers", SSA_NAME_VERSION (t));
      if (POINTER_TYPE_P (TREE_TYPE (t)) && SSA_NAME_PTR_INFO (t))
        dump_pt_solution(&(SSA_NAME_PTR_INFO (t)->pt), "use", "use_vars");
      if (TREE_THIS_VOLATILE (t))
        serialize_string("volatile");
      else
        serialize_gimple_child("def_stmt", SSA_NAME_DEF_STMT(t));
      if (SSA_NAME_VAR (t) && !is_gimple_reg (SSA_NAME_VAR (t)))
        serialize_string("virtual");
      if (SSA_NAME_IS_DEFAULT_DEF(t))
         serialize_string("default");

#if (__GNUC__ > 4)
if (!POINTER_TYPE_P (TREE_TYPE (t))
      && SSA_NAME_RANGE_INFO (t))
    {
      wide_int min, max;
      const_tree t_const = t;
      value_range_type range_type = get_range_info (t_const, &min, &max);

      if (range_type == VR_RANGE)
	{
          serialize_child("min", wide_int_to_tree(TREE_TYPE (t), min));
          serialize_child("max", wide_int_to_tree(TREE_TYPE (t), max));
	}
    }
#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
if (!POINTER_TYPE_P (TREE_TYPE (t))
      && SSA_NAME_RANGE_INFO (t))
    {
      double_int min, max;
      const_tree t_const = t;
      value_range_type range_type = get_range_info (t_const, &min, &max);

      if (range_type == VR_RANGE)
	{
          serialize_child("min", double_int_to_tree(TREE_TYPE (t), min));
          serialize_child("max", double_int_to_tree(TREE_TYPE (t), max));
	}
    }
#else
      index_ssa = SSA_NAME_VERSION (t);
      if(num_vr_values > 0 && (p_VRP_min[index_ssa] || p_VRP_max[index_ssa]))
      {
          if (p_VRP_min[index_ssa]) serialize_child("min", VRP_min_array[index_ssa]);
          if (p_VRP_max[index_ssa]) serialize_child("max", VRP_max_array[index_ssa]);
      }
#endif
      break;
    }
    case DEBUG_EXPR_DECL:
      serialize_int ("-uid", DEBUG_TEMP_UID (t));
      /* Fall through.  */

    case VAR_DECL:
    case PARM_DECL:
    case FIELD_DECL:
    case RESULT_DECL:
      if (TREE_CODE (t) == FIELD_DECL && DECL_C_BIT_FIELD (t))
         serialize_string ("bitfield");
      if(TREE_CODE (t) == VAR_DECL)
      {
         if (DECL_EXTERNAL (t))
            serialize_string ("extern");
         else if (!TREE_PUBLIC (t) && TREE_STATIC (t))
            serialize_string ("static");
      }
      if (TREE_CODE (t) == PARM_DECL)
        serialize_child ("argt", DECL_ARG_TYPE (t));
      else if(DECL_INITIAL (t))
        serialize_child ("init", DECL_INITIAL (t));
#if __GNUC__ > 4 
      else if(TREE_READONLY(t) && TREE_CODE (t) == VAR_DECL && (TREE_STATIC (t) || DECL_EXTERNAL (t)))
      {
        varpool_node *node, *real_node;
        node = varpool_node::get (t);
        if (node)
        {
          tree real_decl;
          real_node = node->ultimate_alias_target ();
          real_decl = real_node->decl;
          if(real_decl) serialize_child ("init", DECL_INITIAL (real_decl));
        }
      }
#endif
      serialize_child ("size", DECL_SIZE (t));
      serialize_int ("algn", DECL_ALIGN (t));
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
      if(TREE_CODE (t) == VAR_DECL && !TREE_ADDRESSABLE(t))
         serialize_string ("addr_not_taken");
      break;

    case FUNCTION_DECL:
    {
      arg = DECL_ARGUMENTS (t);
      while (arg)
      {
        serialize_child ("arg", arg);
        arg = TREE_CHAIN (arg);
      }
      if (DECL_EXTERNAL (t))
        serialize_string ("undefined");
      if(is_builtin_fn(t))
        serialize_string ("builtin");
      if (!TREE_PUBLIC (t))
        serialize_string ("static");
      tree attributeList = DECL_ATTRIBUTES (t);
      if (attributeList != NULL_TREE)
      {
        tree attribute = lookup_attribute("hwcall", attributeList);
        if (attribute != NULL_TREE)
          serialize_string("hwcall");
      }

      if (gimple_has_body_p (t) && t == cfun->decl)
        serialize_statement_child("body", cfun->cfg);
      break;
    }
    case INTEGER_CST:
      serialize_wide_int("value", TREE_INT_CST_LOW(t));
      break;

    case STRING_CST:
      if (TREE_TYPE (t))
        serialize_string_cst("strg" , TREE_STRING_POINTER (t), TREE_STRING_LENGTH (t), TYPE_ALIGN(TREE_TYPE (t)));
      else
        serialize_string_cst("strg" , TREE_STRING_POINTER (t), TREE_STRING_LENGTH (t), 8);
      break;

    case REAL_CST:
      serialize_real (t);
      break;

    case COMPLEX_CST:
      serialize_child ("real", TREE_REALPART (t));
      serialize_child ("imag", TREE_IMAGPART (t));
      break;

    case FIXED_CST:
      serialize_fixed ("valu", TREE_FIXED_CST_PTR (t));
      break;

    case VECTOR_CST:
      {

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
	unsigned i;
#if (__GNUC__ > 7)
    for (i = 0; i < VECTOR_CST_NELTS (t).to_constant (); ++i)
#else
    for (i = 0; i < VECTOR_CST_NELTS (t); ++i)
#endif
            serialize_child ("valu", VECTOR_CST_ELT (t, i));
#else
         tree elt;
         for (elt = TREE_VECTOR_CST_ELTS (t); elt; elt = TREE_CHAIN (elt))
            serialize_child ("valu", TREE_VALUE (elt));
#endif
      }
      break;

    case TRUTH_NOT_EXPR:
    case ADDR_EXPR:
    case INDIRECT_REF:
#if __GNUC__ == 4 && __GNUC_MINOR__ ==  5
    case ALIGN_INDIRECT_REF:
    case MISALIGNED_INDIRECT_REF:
#endif
    case CLEANUP_POINT_EXPR:
    case SAVE_EXPR:
    case REALPART_EXPR:
    case IMAGPART_EXPR:
    case VIEW_CONVERT_EXPR:
      /* These nodes are unary, but do not have code class `1'.  */
      serialize_child ("op", TREE_OPERAND (t, 0));
      break;

#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && !defined(SPARC) && !defined(ARM)
    case MEM_REF:
      {
         serialize_child ("op", TREE_OPERAND (t, 0));
         serialize_child ("op", TREE_OPERAND (t, 1));
         break;
      }
#endif
    case TRUTH_ANDIF_EXPR:
    case TRUTH_ORIF_EXPR:
    case TRUTH_AND_EXPR:
    case TRUTH_OR_EXPR:
    case TRUTH_XOR_EXPR:
    case INIT_EXPR:
    case MODIFY_EXPR:
      serialize_child ("op", TREE_OPERAND (t, 0));
      serialize_child ("op", TREE_OPERAND (t, 1));
      break;
    case COMPOUND_EXPR:
    case PREDECREMENT_EXPR:
    case PREINCREMENT_EXPR:
    case POSTDECREMENT_EXPR:
    case POSTINCREMENT_EXPR:
    case WITH_SIZE_EXPR:
      /* These nodes are binary, but do not have code class `2'.  */
      serialize_child ("op", TREE_OPERAND (t, 0));
      serialize_child ("op", TREE_OPERAND (t, 1));
      break;

    case COMPONENT_REF:
      serialize_child ("op", TREE_OPERAND (t, 0));
      serialize_child ("op", TREE_OPERAND (t, 1));
      serialize_child ("op", TREE_OPERAND (t, 2));
      break;

    case ARRAY_REF:
    case ARRAY_RANGE_REF:
      serialize_child ("op", TREE_OPERAND (t, 0));
      serialize_child ("op", TREE_OPERAND (t, 1));
      serialize_child ("op", TREE_OPERAND (t, 2));
      serialize_child ("op", TREE_OPERAND (t, 3));
      break;

    case COND_EXPR:
      serialize_child ("op", TREE_OPERAND (t, 0));
      serialize_child ("op", TREE_OPERAND (t, 1));
      serialize_child ("op", TREE_OPERAND (t, 2));
      break;

    case BIT_FIELD_REF:
      serialize_child ("op", TREE_OPERAND (t, 0));
      serialize_child ("op", TREE_OPERAND (t, 1));
      serialize_child ("op", TREE_OPERAND (t, 2));
      /*in case t is the last statement of a basic block we have to dump the true and the false edge*/
      /*since it is more easy to compute that edge during the basic block that dump is performed there*/
      break;

    case TRY_FINALLY_EXPR:
      serialize_child ("op", TREE_OPERAND (t, 0));
      serialize_child ("op", TREE_OPERAND (t, 1));
      break;

    case CALL_EXPR:
      {
#if (__GNUC__ > 4)
         if( CALL_EXPR_FN (t))
#endif
            serialize_child ("fn", CALL_EXPR_FN (t));
#if (__GNUC__ > 4)
         else
            serialize_string_field ("ifn", internal_fn_name (CALL_EXPR_IFN (t)));
#endif
         tree arg;
         call_expr_arg_iterator iter;
         FOR_EACH_CALL_EXPR_ARG (arg, iter, t)
         {
            serialize_child ("arg", arg);
         }
         break;
      }
    case CONSTRUCTOR:
      {
        unsigned HOST_WIDE_INT cnt;
        tree index, value;
        queue_and_serialize_type (t);
        FOR_EACH_CONSTRUCTOR_ELT (CONSTRUCTOR_ELTS (t), cnt, index, value)
        {
          serialize_child ("idx", index);
          serialize_child ("valu", value);
        }
      }
      break;

    case BIND_EXPR:
      serialize_child ("vars", TREE_OPERAND (t, 0));
      serialize_child ("body", TREE_OPERAND (t, 1));
      break;

    case LOOP_EXPR:
    case EXIT_EXPR:
    case RETURN_EXPR:
      serialize_child ("op", TREE_OPERAND (t, 0));
      break;

    case TARGET_EXPR:
      serialize_child ("decl", TREE_OPERAND (t, 0));
      serialize_child ("init", TREE_OPERAND (t, 1));
      /* There really are two possible places the initializer can be.
    After RTL expansion, the second operand is moved to the
    position of the fourth operand, and the second operand
    becomes NULL.  */
      serialize_child ("init", TREE_OPERAND (t, 3));
      serialize_child ("clnp", TREE_OPERAND (t, 2));
      break;

    case CASE_LABEL_EXPR:
         if (CASE_LOW (t) && CASE_HIGH (t))
         {
            serialize_child ("op", CASE_LOW (t));
            serialize_child ("op", CASE_HIGH (t));
            serialize_child ("goto", CASE_LABEL (t));
         }
         else if (CASE_LOW (t))
         {
            serialize_child ("op", CASE_LOW (t));
            serialize_child ("goto", CASE_LABEL (t));
         }
         else
         {
            serialize_string ("default");
            serialize_child ("goto", CASE_LABEL (t));
         }
      break;
    case LABEL_EXPR:
      serialize_child ("name", TREE_OPERAND (t,0));
      break;
    case GOTO_EXPR:
      serialize_child ("labl", TREE_OPERAND (t, 0));
      break;
    case SWITCH_EXPR:
      serialize_child ("cond", TREE_OPERAND (t, 0));
      serialize_child ("body", TREE_OPERAND (t, 1));
      if (TREE_OPERAND (t, 2))
        {
      	  serialize_child ("labl", TREE_OPERAND (t,2));
        }
      break;
    case OMP_CLAUSE:
      {
	int i;
	fprintf (serialize_gimple_info.stream, "%s\n", omp_clause_code_name[OMP_CLAUSE_CODE (t)]);
	for (i = 0; i < omp_clause_num_ops[OMP_CLAUSE_CODE (t)]; i++)
	  serialize_child ("op: ", OMP_CLAUSE_OPERAND (t, i));
      }
      break;
      case TARGET_MEM_REF:
      {
#if 0
         unsigned HOST_WIDE_INT misalign=0;
         unsigned int align;
         tree temporary_addr;
         temporary_addr = build_fold_addr_expr(t);
#endif
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6 && __GNUC_PATCHLEVEL__>= 1)) && !defined(SPARC) && !defined(ARM)
#if 0
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
               get_pointer_alignment_1 (temporary_addr, &align, &misalign);
#elif(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
         align = get_pointer_alignment_1 (temporary_addr, &misalign);/*GCC 4.7*/
#else
         align = get_pointer_alignment (temporary_addr, BIGGEST_ALIGNMENT);/*GCC 4.6*/
#endif
#endif
         serialize_child ("base", TMR_BASE(t));
         serialize_child ("offset", TMR_OFFSET(t));
         serialize_child ("idx", TMR_INDEX(t));
         serialize_child ("step", TMR_STEP(t));
         serialize_child ("idx2", TMR_INDEX2(t));
#else
         if(TMR_SYMBOL(t))
         {
           serialize_child ("base", TMR_SYMBOL(t));
           serialize_child ("offset", TMR_OFFSET(t));
           serialize_child ("idx", TMR_INDEX(t));
           serialize_child ("step", TMR_STEP(t));
           serialize_child ("idx2", TMR_BASE(t));
         }
         else
         {
           serialize_child ("base", TMR_BASE(t));
           serialize_child ("offset", TMR_OFFSET(t));
           serialize_child ("idx", TMR_INDEX(t));
           serialize_child ("step", TMR_STEP(t));
           serialize_child ("idx2", TMR_SYMBOL(t));
         }
#if 0
         align = get_pointer_alignment(temporary_addr, BIGGEST_ALIGNMENT);
#endif
         //serialize_child ("orig", TMR_ORIGINAL(t));
#endif
#if 0
         serialize_int("align", align);
         serialize_int("misalign", misalign);
#endif
         break;
      }
      case ASSERT_EXPR:
         serialize_child ("op", TREE_OPERAND (t, 0));
         serialize_child ("op", TREE_OPERAND (t, 1));
         break;
      case VEC_COND_EXPR:
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
      case VEC_PERM_EXPR:
#endif
      case DOT_PROD_EXPR:
         serialize_child ("op", TREE_OPERAND (t, 0));
         serialize_child ("op", TREE_OPERAND (t, 1));
         serialize_child ("op", TREE_OPERAND (t, 2));
         break;
      case OBJ_TYPE_REF:
         serialize_child ("op", TREE_OPERAND (t, 0));
         serialize_child ("op", TREE_OPERAND (t, 1));
         serialize_child ("op", TREE_OPERAND (t, 2));
         break;

    default:
      /* There are no additional fields to print.  */
      break;
    }

 done:

  /* Terminate the line.  */
  fprintf (serialize_gimple_info.stream, "\n");
}

/* Serialize the next gimple node in the queue.  */
static void
dequeue_and_serialize_gimple ()
{
  serialize_queue_p dq;
  struct tree_2_ints * stn_index;
  GIMPLE_type g;
  unsigned int index;
  const char* code_name;
  expanded_location xloc;
  unsigned int i;
  tree t;

  /* Get the next node from the queue.  */
  dq = serialize_gimple_info.queue;
  stn_index = dq->node_index;
  index = (unsigned int) stn_index->value;
  gcc_assert(index <= di_local_index);
  g = (GIMPLE_type) stn_index->key;

  /* Remove the node from the queue, and put it on the free list.  */
  serialize_gimple_info.queue = dq->next;
  if (!serialize_gimple_info.queue)
    serialize_gimple_info.queue_end = 0;
  dq->next = serialize_gimple_info.free_list;
  serialize_gimple_info.free_list = dq;

  serialize_gimple_info.column = 0;
  /* Print the node index.  */
  serialize_index (index);
  if(gimple_code (g) == GIMPLE_CALL && gimple_call_lhs (g))
     code_name = "gimple_assign";
  else
     code_name = gimple_code_name[(int) gimple_code(g)];
  fprintf (serialize_gimple_info.stream, "%-16s ", code_name);

  serialize_gimple_info.column = 25;
   serialize_child("scpe", cfun->decl);
   if(gimple_bb(g))
      serialize_int("bb_index", gimple_bb(g)->index);
   else
      serialize_int("bb_index", 0);

   if (gimple_has_mem_ops (g))
     serialize_vops (g);

  if(gimple_has_location (g))
  {
    xloc = expand_location (gimple_location (g));
    serialize_maybe_newline ();
    fprintf (serialize_gimple_info.stream, "srcp: \"%s\":%-d:%-6d ", xloc.file, xloc.line, xloc.column);
    serialize_gimple_info.column += 12 + strlen(xloc.file) + 8;
  }

  serialize_int ("time_weight", estimate_num_insns(g, &eni_time_weights));
  serialize_int ("size_weight", estimate_num_insns(g, &eni_size_weights));
  switch (gimple_code (g))
    {

    case GIMPLE_ASM:
      {
        int i, n;
        if (gimple_asm_volatile_p (PTRCONV(gasm*,g)))
          serialize_string ("volatile");  
        serialize_string_field ("str", gimple_asm_string (PTRCONV(gasm*,g)));
        n = gimple_asm_noutputs (PTRCONV(gasm*,g));
        if (n > 0)
          {
            tree arglist = NULL_TREE;
            for (i = n - 1; i >= 0; i--)
            {
              tree item = gimple_asm_output_op (PTRCONV(gasm*,g), i);
              arglist = tree_cons (TREE_PURPOSE (item), TREE_VALUE (item), arglist);
            }
            serialize_child ("out", arglist);
          }
        n = gimple_asm_ninputs (PTRCONV(gasm*,g));
        if (n > 0)
          {
            tree arglist = NULL_TREE;
            for (i = n - 1; i >= 0; i--)
            {
              tree item = gimple_asm_input_op (PTRCONV(gasm*,g), i);
              arglist = tree_cons (TREE_PURPOSE (item), TREE_VALUE (item), arglist);
            }

            serialize_child ("in", arglist);
          }
        n = gimple_asm_nclobbers (PTRCONV(gasm*,g)); 
        if (n > 0)
          {
            tree arglist = NULL_TREE;
            for (i = n - 1; i >= 0; i--)
            {
              tree item = gimple_asm_clobber_op (PTRCONV(gasm*,g), i);
              arglist = tree_cons (TREE_PURPOSE (item), TREE_VALUE (item), arglist);
            }

            serialize_child ("clob", arglist);
          }
      }
      break;

    /* For instance, given the GENERIC expression, a = b + c, its tree representation is:
    MODIFY_EXPR <VAR_DECL  <a>, PLUS_EXPR <VAR_DECL <b>, VAR_DECL <c>>>
    In this case, the GIMPLE form for this statement is logically identical to its GENERIC form but in GIMPLE, 
    the PLUS_EXPR on the RHS of the assignment is not represented as a tree, instead the two operands are taken 
    out of the PLUS_EXPR sub-tree and flattened into the GIMPLE tuple as follows:
    GIMPLE_ASSIGN <PLUS_EXPR, VAR_DECL <a>, VAR_DECL <b>, VAR_DECL <c>> */

    /* The first operand is what to set; the second, the new value - consistent with olderversions */

    case GIMPLE_ASSIGN:
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && !defined(SPARC) && !defined(ARM)
      if (get_gimple_rhs_class (gimple_expr_code (g)) == GIMPLE_TERNARY_RHS)
      {
          serialize_child ("op", gimple_assign_lhs (g));
          serialize_child ("op", build3 (gimple_assign_rhs_code (g), TREE_TYPE (gimple_assign_lhs (g)), gimple_assign_rhs1 (g), gimple_assign_rhs2 (g), gimple_assign_rhs3 (g))); 
      }
      else
#endif
      if (get_gimple_rhs_class (gimple_expr_code (g)) == GIMPLE_BINARY_RHS) 
        {
          serialize_child ("op", gimple_assign_lhs (g));
          serialize_child ("op", build2 (gimple_assign_rhs_code (g), TREE_TYPE (gimple_assign_lhs (g)), gimple_assign_rhs1 (g), gimple_assign_rhs2 (g))); 
        }
      else if (get_gimple_rhs_class (gimple_expr_code (g)) == GIMPLE_UNARY_RHS)
        {
          serialize_child ("op", gimple_assign_lhs (g));
          serialize_child ("op", build1 (gimple_assign_rhs_code (g), TREE_TYPE (gimple_assign_lhs (g)), gimple_assign_rhs1 (g))); 
        }
      else if (get_gimple_rhs_class (gimple_expr_code (g)) == GIMPLE_SINGLE_RHS)
        {
          t = gimple_assign_rhs1 (g);
          serialize_child ("op", gimple_assign_lhs (g)); 
          serialize_child ("op", gimple_assign_rhs1 (g)); 
        }
      else
        gcc_unreachable ();
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)) && !defined(SPARC) && !defined(ARM)
          if (gimple_clobber_p (g))
            serialize_string("clobber");
#endif

      break;

    case GIMPLE_BIND:
      {
        tree op;
        if (gimple_bind_vars(PTRCONV(gbind*,g)))
          for ( op = gimple_bind_vars (PTRCONV(gbind*,g)); op; op = TREE_CHAIN (op))
            {
              serialize_child ("vars", op);
            }
        serialize_child ("body", gimple_op (g, 1));
      }
      break;

    case GIMPLE_CALL:
      /* Function call.  Operand 0 is the function.
      * Operand 1 is the argument list, a list of expressions made out of a chain of TREE_LIST nodes.
      * Operand 2 is the static chain argument, or NULL. */
      {
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) && !defined(SPARC) && !defined(ARM) && (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
        struct pt_solution *pt;
#endif
        tree lhs = gimple_call_lhs (g);
        if(lhs)
        {
           tree arg_list = gimple_call_expr_arglist (g);
#if (__GNUC__ > 5)
           if(gimple_call_internal_p(g))
           {
              internal_fn fn = gimple_call_internal_fn(g);
              t = build_custom_function_call_expr_internal_fn (UNKNOWN_LOCATION, fn, TREE_TYPE(lhs), arg_list);
           }
           else
#endif
           t = build_custom_function_call_expr (UNKNOWN_LOCATION, gimple_call_fn (g), arg_list);
           serialize_child ("op",lhs);
           serialize_child ("op", t);
        }
        else
        {
           serialize_child ("fn", gimple_call_fn (g));
           unsigned int arg_index;
           for(arg_index = 0; arg_index < gimple_call_num_args(g); arg_index++)
           {
              serialize_child ("arg", gimple_call_arg(g, arg_index));
           }
        }
#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) && !defined(SPARC) && !defined(ARM) && (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
        pt = gimple_call_use_set (g);
        if (!pt_solution_empty_p (pt))
           dump_pt_solution(pt, "use", "use_vars");
        pt = gimple_call_clobber_set (g);
        if (!pt_solution_empty_p (pt))
           dump_pt_solution(pt, "clb", "clb_vars");
#endif
      }
      break;

    case GIMPLE_COND:
      GIMPLE_CHECK (g, GIMPLE_COND);
      t = build2 (gimple_cond_code (g), boolean_type_node, gimple_cond_lhs (g), gimple_cond_rhs (g));
      serialize_child ("op", t);
      break;

    case GIMPLE_LABEL:
      serialize_child ("op", gimple_label_label (PTRCONV(glabel*,g)));
      break;

    case GIMPLE_GOTO:
      serialize_child ("op", gimple_goto_dest (g));
      break;

    case GIMPLE_NOP:
      break;

    case GIMPLE_RETURN:
      serialize_child ("op", gimple_return_retval (PTRCONV(greturn*,g)));
      break;

    case GIMPLE_SWITCH:
      {
         tree label_vec;
         size_t i, num_elmts = gimple_switch_num_labels (PTRCONV(gswitch*,g));

         label_vec = make_tree_vec (num_elmts);


         for (i = 0; i < num_elmts; i++)
         {
            TREE_VEC_ELT (label_vec, i) = gimple_switch_label (PTRCONV(gswitch*,g), i);
         }
         serialize_child ("op", gimple_switch_index (PTRCONV(gswitch*,g)));
         serialize_child ("op", label_vec);
      }
      break;

    case GIMPLE_TRY:
      error ("Encountered gimple instruction set GIMPLE_TRY");
      break;

    case GIMPLE_PHI:
      serialize_child ("res", gimple_phi_result (g));
      for (i = 0; i < gimple_phi_num_args (g); i++)
        {
          serialize_child ("def", gimple_phi_arg_def (g, i));
          serialize_int ("edge", gimple_phi_arg_edge (PTRCONV(gphi*,g), i)->src->index);
        }
      if(!is_gimple_reg (gimple_phi_result (g)))
        serialize_string("virtual");
      break;

    case GIMPLE_OMP_PARALLEL:
      error ("Encountered gimple instruction set GIMPLE_OMP_PARALLEL");
      break;

    case GIMPLE_OMP_TASK:
      error ("Encountered gimple instruction set GIMPLE_TASK");
      break;

    case GIMPLE_OMP_ATOMIC_LOAD:
      error ("Encountered gimple instruction set GIMPLE_OMP_ATOMIC_LOAD");
      break;

    case GIMPLE_OMP_ATOMIC_STORE:
      error ("Encountered gimple instruction set GIMPLE_OMP_ATOMIC_STORE");
      break;

    case GIMPLE_OMP_FOR:
      error ("Encountered gimple instruction set GIMPLE_OMP_FOR");
      break;

    case GIMPLE_OMP_CONTINUE:
      error ("Encountered gimple instruction set GIMPLE_OMP_CONTINUE");
      break;

    case GIMPLE_OMP_SINGLE:
      error ("Encountered gimple instruction set GIMPLE_OMP_SINGLE");
      break;

    case GIMPLE_OMP_RETURN:
      error ("Encountered gimple instruction set GIMPLE_OMP_RETURN");
      break;

    case GIMPLE_OMP_SECTIONS:
      error ("Encountered gimple instruction set GIMPLE_OMP_SECTIONS");
      break;

    case GIMPLE_OMP_SECTIONS_SWITCH:
      error ("Encountered gimple instruction set GIMPLE_OMP_SECTIONS_SWITCH");
      break;

    case GIMPLE_OMP_MASTER:
      error ("Encountered gimple instruction set GIMPLE_OMP_MASTER");
      break;

    case GIMPLE_OMP_ORDERED:
      error ("Encountered gimple instruction set GIMPLE_OMP_ORDERED");
      break;

    case GIMPLE_OMP_SECTION:
      error ("Encountered gimple instruction set GIMPLE_OMP_SECTION");
      break;

    case GIMPLE_OMP_CRITICAL:
      error ("Encountered gimple instruction set GIMPLE_OMP_CRITICAL");
      break;

    case GIMPLE_CATCH:
      error ("Encountered gimple instruction set GIMPLE_CATCH");
      break;

    case GIMPLE_EH_FILTER:
      error ("Encountered gimple instruction set GIMPLE_EH_FILTER");
      break;

    case GIMPLE_EH_MUST_NOT_THROW:
      error ("Encountered gimple instruction set GIMPLE_EH_MUST_NOT_THROW");
      break;

    case GIMPLE_RESX:
      error ("Encountered gimple instruction set GIMPLE_RESX");
      break;

    case GIMPLE_EH_DISPATCH:
      error ("Encountered gimple instruction set GIMPLE_EH_DISPATCH");
      break;

    case GIMPLE_DEBUG:
      if (gimple_debug_bind_p (g)) 
        {
          serialize_child ("var", gimple_debug_bind_get_var (g));
          serialize_child ("val", gimple_debug_bind_get_value (g));
        }
      break;

    case GIMPLE_PREDICT:
      error ("Encountered gimple instruction set GIMPLE_PREDICT");
      // t = build1 (NOP_EXPR, void_type_node, size_zero_node);
      break;

    default:
      error ("Encountered UNKNOWN gimple instruction set");
      break;
    }

  fprintf (serialize_gimple_info.stream, "\n");

}

/* Serialize the next statement node in the queue.  */
static void
dequeue_and_serialize_statement ()
{
  serialize_queue_p dq;
  struct tree_2_ints * stn_index;
  unsigned int index;
  unsigned int ann;
  const char* code_name;
  basic_block bb;

  /* Get the next node from the queue.  */
  dq = serialize_gimple_info.queue;
  stn_index = dq->node_index;
  index = stn_index->value;
  gcc_assert(index <= di_local_index);
  ann = stn_index->ann;

  /* Remove the node from the queue, and put it on the free list.  */
  serialize_gimple_info.queue = dq->next;
  if (!serialize_gimple_info.queue)
    serialize_gimple_info.queue_end = 0;
  dq->next = serialize_gimple_info.free_list;
  serialize_gimple_info.free_list = dq;

  /* Print the node index.  */
  serialize_index (index);

  /* And the type of node this is.  */
  if (IS_BINFO(ann))
    code_name = "binfo";
  else
    code_name = "statement_list";
  fprintf (serialize_gimple_info.stream, "%-16s ", code_name);
  serialize_gimple_info.column = 25;

  /* In case of basic blocks the function print:
                 + first a list of all statements
                 +  then for each basic block it prints:
                    - the number of the basic block
                    - the predecessor basic block
                    - the successor basic block
                    - list of statement
                 + otherwise it prints only the list of statements */
  FOR_EACH_BB_FN (bb, cfun)
    {
      gimple_stmt_iterator gsi;
      GIMPLE_type stmt;
      edge e;
      edge_iterator ei;
      const char *field;

      if (bb->index != 0)
        serialize_new_line ();
      serialize_int ("bloc", bb->index );
      if(bb->loop_father)
        serialize_int ("loop_id", bb->loop_father->num );
      else
        serialize_int ("loop_id", bb->index );


      if(bb->loop_father && bb->loop_father->can_be_parallel)
        serialize_string("hpl");
      
      FOR_EACH_EDGE (e, ei, bb->preds)
        if (e->src == ENTRY_BLOCK_PTR)
          {
            serialize_maybe_newline ();
            field = "pred: ENTRY";
            fprintf (serialize_gimple_info.stream, "%-4s ", field);
            serialize_gimple_info.column += 14;
	  }
        else
          serialize_int ("pred", e->src->index);
      FOR_EACH_EDGE (e, ei, bb->succs)
        if (e->dest == EXIT_BLOCK_PTR)
          {
            serialize_maybe_newline ();
            field = "succ: EXIT";
            fprintf (serialize_gimple_info.stream, "%-4s ", field);
            serialize_gimple_info.column += 14;
          }
        else
          serialize_int ("succ", e->dest->index);
      /* in case the last statement is a cond_expr we print also the type of the outgoing edges*/
      stmt = last_stmt (bb);
      if (stmt && gimple_code (stmt) == GIMPLE_COND)
        {
          edge true_edge, false_edge;
          /* When we are emitting the code or changing CFG, it is possible that
          the edges are not yet created.  When we are using debug_bb in such
          a situation, we do not want it to crash.  */
          if (EDGE_COUNT (bb->succs) == 2)
            {
              extract_true_false_edges_from_block (bb, &true_edge, &false_edge);
              serialize_int ("true_edge", true_edge->dest->index);
              serialize_int ("false_edge", false_edge->dest->index);
            }
        }
      /* dump phi information*/
      {
        gimple_stmt_iterator i;
        for (i = gsi_start_phis (bb); !gsi_end_p (i); gsi_next (&i))
          {
            GIMPLE_type phi = gsi_stmt (i);
            serialize_gimple_child("phi", phi);
          }
      }

      for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
        {
          stmt = gsi_stmt (gsi);
          serialize_gimple_child("stmt", stmt);
        }
    }
  /* Terminate the line.  */
  fprintf (serialize_gimple_info.stream, "\n");
}

/**
 * Serialize a global symbol (function_decl or global var_decl
 * @param t is the symbol to be serialized
 */
void
SerializeGimpleGlobalTreeNode(tree t)
{
   struct tree_2_ints *n, in;

   /* Print function header */
   if(TREE_CODE(t) == FUNCTION_DECL)
   {
      SerializeGimpleFunctionHeader(t);
   }

   /* Queue up the first node when not yet considered.  */
   unsigned int ann = MAKE_ANN(0, 0, 0);
   in.key = t;
   in.ann = ann;
#if (__GNUC__ > 4)
   n = di_local_nodes_index->find(&in);
#else
   n = (struct tree_2_ints *)htab_find (di_local_nodes_index, &in);
#endif
   if (!n)
      queue (t);
   else if (FUNCTION_DECL == TREE_CODE(t) && gimple_has_body_p (t))
   {
      in.key = t;
      in.ann = ann;
#if (__GNUC__ > 4)
      di_local_nodes_index->remove_elt(&in);
#else
      htab_remove_elt(di_local_nodes_index, &in);
#endif
      queue (t);
   }

  /* Until the queue is empty, keep dumping nodes.  */
  while (serialize_gimple_info.queue)
    dequeue_and_serialize();

}

/**
 * Serialize the header of a function declaration
 * @param fn is the function decl
 */
static void
SerializeGimpleFunctionHeader(tree fn)
{
  tree arg;
  fprintf (serialize_gimple_info.stream, "\n;; Function %s (%s)\n\n", lang_hooks.decl_printable_name (fn, 2), lang_hooks.decl_printable_name (fn, 2)); 
  fprintf (serialize_gimple_info.stream, ";; %s (", lang_hooks.decl_printable_name (fn, 2));
  arg = DECL_ARGUMENTS (fn);
  while (arg)
    {
      print_generic_expr (serialize_gimple_info.stream, TREE_TYPE (arg), TDF_SLIM);
      fprintf (serialize_gimple_info.stream, " ");
      print_generic_expr (serialize_gimple_info.stream, arg, TDF_SLIM);
      if (TREE_CHAIN (arg))
        fprintf (serialize_gimple_info.stream, ", ");
      arg = TREE_CHAIN (arg);
    }
  fprintf (serialize_gimple_info.stream, ")\n");
}

tree
gimple_call_expr_arglist (GIMPLE_type g)
{
  tree arglist = NULL_TREE;
  int i;
  for (i = gimple_call_num_args (g) - 1; i >= 0; i--)
    arglist = tree_cons (NULL_TREE, gimple_call_arg (g, i), arglist);
  return arglist;
}

tree
build_custom_function_call_expr (location_t loc, tree fn, tree arglist)
{
  tree fntype = TREE_TYPE (fn);
  int n = list_length (arglist);
  tree *argarray = (tree *) alloca (n * sizeof (tree));
  int i;
  tree tem;

  for (i = 0; i < n; i++, arglist = TREE_CHAIN (arglist))
    argarray[i] = TREE_VALUE (arglist);
    tem = build_call_array_loc (loc, TREE_TYPE (fntype), fn, n, argarray);
    return tem;
}

#if (__GNUC__ > 5)
tree
build_custom_function_call_expr_internal_fn (location_t loc, internal_fn fn, tree type, tree arglist)
{
  int n = list_length (arglist);
  tree *argarray = (tree *) alloca (n * sizeof (tree));
  int i;
  tree tem;

  for (i = 0; i < n; i++, arglist = TREE_CHAIN (arglist))
    argarray[i] = TREE_VALUE (arglist);
  tem = build_call_expr_internal_loc_array (loc, fn, type, n, argarray);
  return tem;
}
#endif

/**
 * Serialize globlal variable on files
 */
void serialize_global_variable(void * gcc_data, void * user_data)
{
   size_t nc = strlen(dump_base_name);
   size_t nc_index=0;
   size_t last_slash=0;
   while (nc_index<nc)
   {
      if(dump_base_name[nc_index] == '/')
         last_slash = nc_index+1;
      ++nc_index;
   }
   strcpy(dump_base_name_trimmed, dump_base_name+last_slash);

   char * name = concat ((char *)user_data, "/", dump_base_name_trimmed, ".001t.tu", NULL);
   SerializeGimpleBegin(name);
   serialize_globals();
   SerializeGimpleEnd();
}


/// alias based dependency analysis
///

/// auxiliary functions
///

/*
 * Worker for the walker that checks for true clobbering.
 */
static bool
serialize_gimple_aliased_reaching_defs_1 (ao_ref *dummy_1, tree vdef, void *dummy_2)
{
  serialize_child ("vuse", vdef);
  GIMPLE_type def_stmt = SSA_NAME_DEF_STMT (vdef);
  tree vuse = gimple_vuse (def_stmt);
  GIMPLE_type vuse_stmt = SSA_NAME_DEF_STMT (vuse);
  if (gimple_code (vuse_stmt) == GIMPLE_PHI)
     return false;
  else
     return false;
}

static void
serialize_gimple_aliased_reaching_defs (GIMPLE_type stmt, tree ref)
{
  ao_ref refd;
  ao_ref_init (&refd, ref);
  walk_aliased_vdefs (&refd, gimple_vuse (stmt),
                  serialize_gimple_aliased_reaching_defs_1,
                  gimple_bb (stmt), NULL);
}

static void
serialize_all_gimple_aliased_reaching_defs (GIMPLE_type stmt)
{
  walk_aliased_vdefs (NULL, gimple_vuse (stmt),
                  serialize_gimple_aliased_reaching_defs_1,
                  NULL, NULL);
}

static bool
serialize_gimple_aliased_reaching_defs_2 (ao_ref *dummy_1, tree vdef, void *dummy_2)
{
   serialize_child ("vover", vdef);
   return false;
}

static void
serialize_gimple_aliased_reaching_defsdefs (GIMPLE_type stmt, tree ref)
{
   ao_ref refd;
   ao_ref_init (&refd, ref);
   walk_aliased_vdefs (&refd, gimple_vuse (stmt),
                      serialize_gimple_aliased_reaching_defs_2,
                      gimple_bb (stmt), NULL);
}

static
void serialize_gimple_dependent_stmts_load(GIMPLE_type gs)
{
   gcc_assert (gs);
   tree use;
   use = gimple_vuse (gs);
   gcc_assert (use);
   if (is_gimple_call (gs))
   {
      tree callee = gimple_call_fndecl (gs);

      /* Calls to functions that are merely acting as barriers
               or that only store to memory do not make any previous
               stores necessary.  */
      if (callee != NULL_TREE
          && DECL_BUILT_IN_CLASS (callee) == BUILT_IN_NORMAL
          && (DECL_FUNCTION_CODE (callee) == BUILT_IN_MEMSET
              || DECL_FUNCTION_CODE (callee) == BUILT_IN_MALLOC
              || DECL_FUNCTION_CODE (callee) == BUILT_IN_FREE
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
              || DECL_FUNCTION_CODE (callee) == BUILT_IN_MEMSET_CHK
              || DECL_FUNCTION_CODE (callee) == BUILT_IN_CALLOC
              || DECL_FUNCTION_CODE (callee) == BUILT_IN_VA_END
              || DECL_FUNCTION_CODE (callee) == BUILT_IN_ALLOCA
              || (DECL_FUNCTION_CODE (callee)
              == BUILT_IN_ALLOCA_WITH_ALIGN)
              || DECL_FUNCTION_CODE (callee) == BUILT_IN_STACK_SAVE
              || DECL_FUNCTION_CODE (callee) == BUILT_IN_STACK_RESTORE
              || DECL_FUNCTION_CODE (callee) == BUILT_IN_ASSUME_ALIGNED
#endif
              ))
         return;
      serialize_all_gimple_aliased_reaching_defs(gs);
      //serialize_child ("vdep_vuse", use);
   }
   else if (gimple_assign_single_p (gs))
   {
      tree rhs;
      /* If this is a load mark things necessary.  */
      rhs = gimple_assign_rhs1 (gs);
      if (TREE_CODE (rhs) != SSA_NAME
          && !is_gimple_min_invariant (rhs)
          && TREE_CODE (rhs) != CONSTRUCTOR)
      {
         serialize_gimple_aliased_reaching_defs (gs, rhs);
      }

   }
   else if (gimple_code (gs) == GIMPLE_RETURN)
   {
      tree rhs = gimple_return_retval (PTRCONV(greturn*,gs));
      /* A return statement may perform a load.  */
      if (rhs
          && TREE_CODE (rhs) != SSA_NAME
          && !is_gimple_min_invariant (rhs)
          && TREE_CODE (rhs) != CONSTRUCTOR)
      {
         serialize_gimple_aliased_reaching_defs (gs, rhs);
      }
   }
   else if (gimple_code (gs) == GIMPLE_ASM)
   {
      serialize_all_gimple_aliased_reaching_defs(gs);
      //serialize_child ("vdep_vuse", use);
   }
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
   else if (gimple_code (gs) == GIMPLE_TRANSACTION)
   {
      serialize_all_gimple_aliased_reaching_defs(gs);
      //serialize_child ("vdep_vuse", use);
   }
#endif
   else
      gcc_unreachable ();
}

static
bool gimples_may_conflict(GIMPLE_type gs, GIMPLE_type next_gs)
{
   tree *op0, *op0_next, *op1=0;

   if ((gimple_code (next_gs) == GIMPLE_CALL && !(gimple_call_flags (next_gs) & (ECF_CONST | ECF_PURE))) || (gimple_code (next_gs) == GIMPLE_ASM && (gimple_asm_volatile_p (PTRCONV(gasm*,next_gs)) || gimple_vuse (next_gs))))
      return true;

   if (gimple_code (next_gs) == GIMPLE_ASSIGN)
   {
      op0_next = gimple_assign_lhs_ptr (next_gs);
   }
   else if (gimple_code (next_gs) == GIMPLE_CALL)
   {
      op0_next = gimple_call_lhs_ptr (next_gs);
   }
   else
      return true;


   if ((gimple_code (gs) == GIMPLE_CALL
        && !(gimple_call_flags (gs) & (ECF_CONST | ECF_PURE)))
       || (gimple_code (gs) == GIMPLE_ASM
           && (gimple_asm_volatile_p (PTRCONV(gasm*,gs)) || gimple_vuse (gs))))
      return true;

   if (gimple_code (gs) == GIMPLE_ASSIGN)
   {
      tree base;
      tree *rhs = gimple_assign_rhs1_ptr (gs);
      if (DECL_P (*rhs)
      || (REFERENCE_CLASS_P (*rhs)
          && (base = get_base_address (*rhs))
          && TREE_CODE (base) != SSA_NAME))
         op1 = rhs;

      op0 = gimple_assign_lhs_ptr (gs);
   }
   else if (gimple_code (gs) == GIMPLE_CALL)
   {
      op0 = gimple_call_lhs_ptr (gs);
   }
   else
      return true;

   if ((op0
         && (DECL_P (*op0)
         || (REFERENCE_CLASS_P (*op0) && get_base_address (*op0))))
       && (op0_next
           && (DECL_P (*op0_next)
           || (REFERENCE_CLASS_P (*op0_next) && get_base_address (*op0_next)))))
   {
      ao_ref refd, refd_next;
      ao_ref_init (&refd, *op0);
      ao_ref_init (&refd_next, *op0_next);
      if (refs_may_alias_p_1 (&refd, &refd_next, true))
         return true;
   }
   if((op1
            && (DECL_P (*op1)
            || (REFERENCE_CLASS_P (*op1) && get_base_address (*op1))))
          && (op0_next
              && (DECL_P (*op0_next)
              || (REFERENCE_CLASS_P (*op0_next) && get_base_address (*op0_next)))))
      {
         ao_ref refd, refd_next;
         ao_ref_init (&refd, *op1);
         ao_ref_init (&refd_next, *op0_next);
         if (refs_may_alias_p_1 (&refd, &refd_next, true))
            return true;

      }
   return false;
}

/**
 * Add anti_dependence by dumping further vuse to the currently analyzed gimple
 * @param current is the currently analyzed gimple
 * @param gs is the other gimple
 */
void
SerializeGimpleUseDefs(GIMPLE_type current, GIMPLE_type next_def)
{
   /// now check if there is an anti-dependence because of barrier
   if(IsBarrier(next_def))
   {
      serialize_child("vuse", gimple_vdef(next_def));
      return;
   }
   ///Now check if there is an anti-dependence because of alias
   ///Get right operand of current gimple
   tree * use = GetRightOperand(current);
   ///Get left operand of other_use_stmt
   tree * def = GetLeftOperand(next_def);

   if(use == NULL || def == NULL)
   {
      serialize_child("vuse", gimple_vdef(next_def));
   }
   else if(refs_anti_dependent_p(*use, *def))
   {
      serialize_child("vuse", gimple_vdef(next_def));
   }
   ///The uses of next def
   ssa_use_operand_t * next_uses = &(SSA_NAME_IMM_USE_NODE(gimple_vdef(next_def)));
   ssa_use_operand_t * next_ptr;
   for (next_ptr = next_uses->next; next_ptr != next_uses; next_ptr = next_ptr->next)
   {
      if(gimple_vdef(USE_STMT(next_ptr)))
      {
         SerializeGimpleUseDefs(current, USE_STMT(next_ptr));
      }
   }
}

/**
 * Add output dependencies by dumping further vdef to the currently analyzed gimple
 * @param previous is potentially the first gimple of the pair
 * @param current is the currently analyzed gimple 
 */
void
SerializeGimpleDefsDef(const GIMPLE_type previous, const GIMPLE_type current)
{
   /// now check if there is an output-dependence because of barrier
   if(IsBarrier(previous))
   {
      serialize_child("vover", gimple_vdef(previous));
      return;
   }
   ///Now check if there is an output-dependence because of alias
   ///Get left operand of previous gimple
   tree * previous_def = GetLeftOperand(previous);
   ///Get right operand of current gimple
   tree * current_def = GetLeftOperand(current);
   if(previous_def == NULL || current_def == NULL)
   {
      serialize_child("vover", gimple_vdef(previous));
   }
   else if(refs_output_dependent_p(*previous_def, *current_def))
   {
      serialize_child("vover", gimple_vdef(previous));
   }
   ///The use of previous def
   if(gimple_vuse(previous))
   {
      tree vuse = gimple_vuse (previous);
      GIMPLE_type previous_previous = SSA_NAME_DEF_STMT(vuse);
      SerializeGimpleDefsDef(previous_previous, current);
   }
}

void
SerializeGimpleDefsDef2(const GIMPLE_type current)
{
   tree * current_def = GetLeftOperand(current);
   if(current_def)
     serialize_gimple_aliased_reaching_defsdefs(current,*current_def);
}


/**
 * Return true if the gimple has to be considered a barrier (i.e., it writes and reads everything
 */
bool
IsBarrier(const GIMPLE_type gs)
{
   if (gimple_code (gs) == GIMPLE_CALL)
   {
      tree callee = gimple_call_fndecl(gs);

      /** Functions which do not have side effects on other memory locations */
      if (callee != NULL_TREE
            && DECL_BUILT_IN_CLASS (callee) == BUILT_IN_NORMAL
            && (DECL_FUNCTION_CODE (callee) == BUILT_IN_MEMSET
               || DECL_FUNCTION_CODE (callee) == BUILT_IN_MALLOC
               || DECL_FUNCTION_CODE (callee) == BUILT_IN_FREE
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
               || DECL_FUNCTION_CODE (callee) == BUILT_IN_MEMSET_CHK
               || DECL_FUNCTION_CODE (callee) == BUILT_IN_CALLOC
               || DECL_FUNCTION_CODE (callee) == BUILT_IN_VA_END
               || DECL_FUNCTION_CODE (callee) == BUILT_IN_ALLOCA
               || (DECL_FUNCTION_CODE (callee)
                  == BUILT_IN_ALLOCA_WITH_ALIGN)
               || DECL_FUNCTION_CODE (callee) == BUILT_IN_STACK_SAVE
               || DECL_FUNCTION_CODE (callee) == BUILT_IN_STACK_RESTORE
               || DECL_FUNCTION_CODE (callee) == BUILT_IN_ASSUME_ALIGNED
#endif
               ))
         return false;
      ///Functions which have some side effects
      if(!(gimple_call_flags (gs) & (ECF_CONST | ECF_PURE)))
         return true;
      ///Functions which do no have side effects
      return false;
   }
   if(gimple_code (gs) == GIMPLE_ASM && (gimple_asm_volatile_p (PTRCONV(gasm*,gs)) || gimple_vuse (gs)))
      return true;
   if (gimple_code (gs) == GIMPLE_ASSIGN)
      return false;
   /**We consider that all the other statements */
   return true;
}

/**
 * Return true if the gimple corresponds to a pragma placeholder (function call)
 */
bool
IsPragma(const GIMPLE_type gs)
{
   if (gimple_code (gs) == GIMPLE_CALL)
   {
      tree callee = gimple_call_fndecl(gs);
      if (callee == NULL_TREE || !DECL_NAME(callee)) return false;
      const char * called_function_name = IDENTIFIER_POINTER(DECL_NAME(callee));
      if((strcmp(called_function_name, "__pragma_start__\0") == 0) || (strcmp(called_function_name, "__pragma_end__\0") == 0) || (strcmp(called_function_name, "__pragma_single_line_one_argument__\0") == 0))
      {
         return true;
      }
      return false;
   }
   return false;
}

/**
 * Return the right operand of a gimple or NULL_TREE if this cannot be determined
 */
tree *
GetRightOperand(GIMPLE_type gs)
{
   if (gimple_code (gs) == GIMPLE_ASSIGN)
   {
      tree base;
      tree *rhs = gimple_assign_rhs1_ptr (gs);
      if (DECL_P (*rhs) || (REFERENCE_CLASS_P (*rhs) && (base = get_base_address (*rhs))&& TREE_CODE (base) != SSA_NAME))
      {
         return rhs;
      }
   }
   return NULL;
}

/**
 * Return the left operand of a gimple or NULL_TREE if this cannot be determined
 */
tree *
GetLeftOperand(const GIMPLE_type gs)
{
   if (gimple_code (gs) == GIMPLE_ASSIGN)
   {
      return gimple_assign_lhs_ptr(gs);
   }
   if (gimple_code (gs) == GIMPLE_CALL && gimple_call_lhs(gs))
   {
      return gimple_call_lhs_ptr (gs);
   }
   return NULL;
}

static void add_referenced_var_map(tree var)
{
   int uid = DECL_UID (var);
   struct dg_descriptor_tree in;
   struct dg_descriptor_tree * h;
   struct dg_descriptor_tree ** loc;
   struct dg_descriptor_tree * desc;
   gcc_assert (TREE_CODE (var) == VAR_DECL
	      || TREE_CODE (var) == PARM_DECL
	      || TREE_CODE (var) == RESULT_DECL);
   in.hash = uid;
   in.vd = var;
#if (__GNUC__ > 4) 
   h = di_local_referenced_var_htab->find_with_hash (&in, uid);
#else
   h = (struct dg_descriptor_tree *)htab_find_with_hash (di_local_referenced_var_htab, &in, uid);
#endif
   if (h)
   {
      /* Sometime GCC 6 has different variables with the same DECL_UID*/
#if (__GNUC__ <= 5)
      gcc_assert (h->vd == var);
#endif
      return;
   }
#if (__GNUC__ > 4)
   loc = di_local_referenced_var_htab->find_slot_with_hash (&in,
						      uid, INSERT);
#else
   loc = (struct dg_descriptor_tree **)htab_find_slot_with_hash (di_local_referenced_var_htab,
                                            &in, uid, INSERT);
#endif

   desc = *loc;

   if (desc == 0)
   {
#if (__GNUC__ > 4)
       desc = ggc_alloc<dg_descriptor_tree> ();
#else
#if (__GNUC__ == 4 && __GNUC_MINOR__ ==  6)
       desc = ((struct dg_descriptor_tree *)(ggc_internal_alloc_stat (sizeof (struct dg_descriptor_tree) MEM_STAT_INFO)));
#elif (__GNUC__ == 4 && __GNUC_MINOR__ > 6)
       desc = ggc_alloc_dg_descriptor_tree ();
#else
       desc = (struct dg_descriptor_tree*)ggc_alloc (sizeof(struct dg_descriptor_tree));
#endif
#endif
       desc->vd = var;
       desc->hash = uid;
       *loc = desc;
   }
}

static void compute_referenced_var_map()
{
   struct varpool_node *vnode;

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
   FOR_EACH_DEFINED_VARIABLE (vnode)
   {

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
      tree var = vnode->decl;
#else
      tree var = vnode->symbol.decl;
#endif
#else
   vnode = varpool_nodes_queue;
   while (vnode)
   {
      tree var = vnode->decl;
      vnode = vnode->next_needed;
#endif
      if(TREE_CODE (var) == VAR_DECL)
         add_referenced_var_map (var);
   }
   {
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ > 6)
      struct cgraph_node *node;
      FOR_EACH_FUNCTION_WITH_GIMPLE_BODY (node)
      {
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 8)
         struct function *fn = DECL_STRUCT_FUNCTION (node->symbol.decl);
#else
         struct function *fn = DECL_STRUCT_FUNCTION (node->decl);
#endif
         tree lvar;
         unsigned ix;
         if(!fn) continue;
         FOR_EACH_LOCAL_DECL (fn, ix, lvar)
         {
            if(TREE_CODE (lvar) == VAR_DECL)
               add_referenced_var_map (lvar);
         }
      }
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 6)
      struct cgraph_node *node;
      for (node = cgraph_nodes; node; node = node->next)
      {
         struct function *fn;
         tree lvar;
         unsigned ix;
         /* Nodes without a body are not interesting.  */
         if (!gimple_has_body_p (node->decl)
             || node->clone_of)
            continue;
         fn = DECL_STRUCT_FUNCTION (node->decl);
         FOR_EACH_LOCAL_DECL (fn, ix, lvar)
         {
            if(TREE_CODE (lvar) == VAR_DECL)
               add_referenced_var_map (lvar);
         }
      }
#else
      struct cgraph_node *node;
      for (node = cgraph_nodes; node; node = node->next)
      {
         struct function *fn;
         tree old_func_decl;
         referenced_var_iterator rvi;
         tree lvar;
         /* Nodes without a body are not interesting.  */
         if (!gimple_has_body_p (node->decl)
             || node->clone_of)
            continue;
         fn = DECL_STRUCT_FUNCTION (node->decl);
         old_func_decl = current_function_decl;
         push_cfun (fn);
         current_function_decl = node->decl;
         FOR_EACH_REFERENCED_VAR (lvar, rvi)
         {
            if(TREE_CODE (lvar) == VAR_DECL)
               add_referenced_var_map (lvar);
         }
         current_function_decl = old_func_decl;
         pop_cfun ();
      }
#endif
   }


}
