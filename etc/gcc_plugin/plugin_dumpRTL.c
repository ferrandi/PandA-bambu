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
* @file plugin_dumpRTL.c
* @brief Plugin to dump gimple and rtl representations
*
* @author Marco Lattuada <marco.lattuada@polimi.it>
*
*/
#include "plugin_includes.h"

#include "ssaexpand.h"

///Flag used to identify first execution of the RTL dumping; used to activate preamble dumping
static int RTL_serialize_state = 0;

///The pass number assigned to RTL dumping
static int RTL_dump_nr;

///The file where RTL is dumped
FILE * RTL_dump_file;

static void maybe_dump_rtl_for_gimple_stmt (GIMPLE_type stmt, rtx since);

static void dump_stack_var_partition ();

#if defined(ARM)

#ifdef ARM_GCC_4_5_2
#include "cfgexpand_4_5_2.c"
#else
#error Missing cfgexpand for this gcc version
#endif

#elif defined(SPARC)

#ifdef SPARC_GCC_4_5_2
#include "cfgexpand_4_5_2.c"
#else
#error Missing cfgexpand for this gcc version
#endif

#else

#error plugin_dumpRTL can be created only for ARM and SPARC

#endif

unsigned int custom_gimple_expand_cfg();

extern unsigned int gimplePssa (void);
extern void DumpGimpleInit(void);
///Defined in dumpGimple.c
extern splay_tree di_local_nodes_ann;

/**
 * Dump the current version of gcc + current version of plugin
 */
static
void DumpVersion()
{
   char * panda_plugin_version = (char *) PANDA_PLUGIN_VERSION;
   int version = __GNUC__, minor = __GNUC_MINOR__, patchlevel = __GNUC_PATCHLEVEL__;
   fprintf(RTL_dump_file, "GCC_VERSION: \"%d.%d.%d\"\n", version, minor, patchlevel);
   fprintf(RTL_dump_file, "PLUGIN_VERSION: \"%s\"\n", panda_plugin_version);
}

/**
 * Begin a RTL serialization and returns a stream to write to.
 * Multiple calls will reopen and append to the dump file.  
 * */
static void
SerializeRTLBegin(void)
{
   char *name;
   struct dump_file_info *dfi;
   char dump_id[10];
   char suffix;

   dfi = get_dump_file_info (RTL_dump_nr);

   suffix = 's';

   name = concat (dump_base_name, ".rtlExpand", NULL);

   RTL_dump_file = fopen (name, RTL_serialize_state == 0 ? "w" : "a");
   if (RTL_dump_file && RTL_serialize_state == 0)
   {
      DumpVersion(RTL_dump_file);
   }
   if (!RTL_dump_file)
      error ("could not open serialize file %qs: %s", name, strerror (errno));
   else
      RTL_serialize_state = 1;
   free (name);
}

/**
 * (Temporary) Ends rtl serialization
 */
static void
SerializeRTLEnd()
{
  fclose (RTL_dump_file);
}

static unsigned int
expand_cfg_and_dump (void)
{
   /* dummy pass */
   if (!(cfun->curr_properties & PROP_cfg))
      fatal_error("PROP_cfg marked in the pass descriptor but not found");
   if(cfun->cfg && basic_block_info)
   {
      SerializeRTLBegin();
      custom_gimple_expand_cfg();
      SerializeRTLEnd();
   }
   return 0;
}

struct rtl_opt_pass pass_expand_and_dump =
{
   {
      RTL_PASS,
      "expand_and_dump",          /* name */
      NULL,                       /* gate */
      expand_cfg_and_dump,        /* execute */
      NULL,                       /* sub */
      NULL,                       /* next */
      0,                          /* static_pass_number */
      TV_EXPAND,                  /* tv_id */
      PROP_ssa | PROP_gimple_leh | PROP_cfg
      | PROP_gimple_lcx,          /* properties_required */
      PROP_rtl,                   /* properties_provided */
      PROP_ssa | PROP_trees,      /* properties_destroyed */
      TODO_verify_ssa | TODO_verify_flow
      | TODO_verify_stmts,        /* todo_flags_start */
      TODO_dump_func
      | TODO_ggc_collect          /* todo_flags_finish */
   }
};


int
plugin_init (struct plugin_name_args *plugin_info,
             struct plugin_gcc_version *version)
{
  if (!plugin_default_version_check (version, &gcc_version))
    return 1;

  serialize_state = -1;

  struct register_pass_info pass_info;
  const char *plugin_name = plugin_info->base_name;

  pass_info.pass = &pass_expand_and_dump.pass;
  pass_info.reference_pass_name = "expand";

  /* Do it for every instance if it is 0. */
  pass_info.ref_pass_instance_number = 1;

  /* This pass can go almost anywhere as long as you're in SSA form */
  pass_info.pos_op = PASS_POS_REPLACE;

  /* Register this new pass with GCC */
  register_callback (plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);
  serialize_state = -1;
  RTL_dump_nr = pass_info.pass->static_pass_number;

  return 0;
}
unsigned int
custom_gimple_expand_cfg (void)
{
   basic_block bb, init_block;
   sbitmap blocks;
   edge_iterator ei;
   edge e;
   unsigned i;
   const char * dname, *aname;
   dname = lang_hooks.decl_printable_name (current_function_decl, 2);
   aname = (IDENTIFIER_POINTER  (DECL_ASSEMBLER_NAME (current_function_decl)));
   char * name = concat (dump_base_name, ".gimplePSSA", NULL);
   SerializeGimpleBegin(name);
   SerializeGimpleGlobalTreeNode(cfun->decl);
   fprintf (RTL_dump_file, "\n;; Function %s (%s)", dname, aname);

   rewrite_out_of_ssa (&SA);
   SA.partition_to_pseudo = (rtx *)xcalloc (SA.map->num_partitions,
                            sizeof (rtx));

   /* Some backends want to know that we are expanding to RTL.  */
   currently_expanding_to_rtl = 1;

   rtl_profile_for_bb (ENTRY_BLOCK_PTR);

   insn_locators_alloc ();
   if (!DECL_IS_BUILTIN (current_function_decl))
   {
      /* Eventually, all FEs should explicitly set function_start_locus.  */
      if (cfun->function_start_locus == UNKNOWN_LOCATION)
         set_curr_insn_source_location
         (DECL_SOURCE_LOCATION (current_function_decl));
      else
         set_curr_insn_source_location (cfun->function_start_locus);
   }
   set_curr_insn_block (DECL_INITIAL (current_function_decl));
   prologue_locator = curr_insn_locator ();

   /* Make sure first insn is a note even if we don't want linenums.
      This makes sure the first insn will never be deleted.
      Also, final expects a note to appear there.  */
   emit_note (NOTE_INSN_DELETED);

   /* Mark arrays indexed with non-constant indices with TREE_ADDRESSABLE.  */
   discover_nonconstant_array_refs ();

   targetm.expand_to_rtl_hook ();
   crtl->stack_alignment_needed = STACK_BOUNDARY;
   crtl->max_used_stack_slot_alignment = STACK_BOUNDARY;
   crtl->stack_alignment_estimated = 0;
   crtl->preferred_stack_boundary = STACK_BOUNDARY;
   cfun->cfg->max_jumptable_ents = 0;


   /* Expand the variables recorded during gimple lowering.  */
   expand_used_vars ();

   /* Honor stack protection warnings.  */
   if (warn_stack_protect)
   {
      if (cfun->calls_alloca)
         warning (OPT_Wstack_protector,
                  "not protecting local variables: variable length buffer");
      if (has_short_buffer && !crtl->stack_protect_guard)
         warning (OPT_Wstack_protector,
                  "not protecting function: no buffer at least %d bytes long",
                  (int) PARAM_VALUE (PARAM_SSP_BUFFER_SIZE));
   }

   /* Set up parameters and prepare for return, for the function.  */
   expand_function_start (current_function_decl);

   /* Now that we also have the parameter RTXs, copy them over to our
      partitions.  */
   for (i = 0; i < SA.map->num_partitions; i++)
   {
      tree var = SSA_NAME_VAR (partition_to_var (SA.map, i));

      if (TREE_CODE (var) != VAR_DECL
            && !SA.partition_to_pseudo[i])
         SA.partition_to_pseudo[i] = DECL_RTL_IF_SET (var);
      gcc_assert (SA.partition_to_pseudo[i]);

      /* If this decl was marked as living in multiple places, reset
         this now to NULL.  */
      if (DECL_RTL_IF_SET (var) == pc_rtx)
         SET_DECL_RTL (var, NULL);

      /* Some RTL parts really want to look at DECL_RTL(x) when x
         was a decl marked in REG_ATTR or MEM_ATTR.  We could use
      SET_DECL_RTL here making this available, but that would mean
      to select one of the potentially many RTLs for one DECL.  Instead
      of doing that we simply reset the MEM_EXPR of the RTL in question,
      then nobody can get at it and hence nobody can call DECL_RTL on it.  */
      if (!DECL_RTL_SET_P (var))
      {
         if (MEM_P (SA.partition_to_pseudo[i]))
            set_mem_expr (SA.partition_to_pseudo[i], NULL);
      }
   }

   /* If this function is `main', emit a call to `__main'
      to run global initializers, etc.  */
   if (DECL_NAME (current_function_decl)
         && MAIN_NAME_P (DECL_NAME (current_function_decl))
         && DECL_FILE_SCOPE_P (current_function_decl))
      expand_main_function ();

   /* Initialize the stack_protect_guard field.  This must happen after the
      call to __main (if any) so that the external decl is initialized.  */
   if (crtl->stack_protect_guard)
      stack_protect_prologue ();

   expand_phi_nodes (&SA);

   /* Register rtl specific functions for cfg.  */
   rtl_register_cfg_hooks ();

   init_block = construct_init_block ();

   /* Clear EDGE_EXECUTABLE on the entry edge(s).  It is cleaned from the
      remaining edges later.  */
   FOR_EACH_EDGE (e, ei, ENTRY_BLOCK_PTR->succs)
   e->flags &= ~EDGE_EXECUTABLE;

   lab_rtx_for_bb = pointer_map_create ();
   FOR_BB_BETWEEN (bb, init_block->next_bb, EXIT_BLOCK_PTR, next_bb)
   bb = expand_gimple_basic_block (bb);

   if (MAY_HAVE_DEBUG_INSNS)
      expand_debug_locations ();

   execute_free_datastructures ();
   finish_out_of_ssa (&SA);

   /* We are no longer in SSA form.  */
   cfun->gimple_df->in_ssa_p = false;

   /* Expansion is used by optimization passes too, set maybe_hot_insn_p
      conservatively to true until they are all profile aware.  */
   pointer_map_destroy (lab_rtx_for_bb);
   free_histograms ();

   construct_exit_block ();
   set_curr_insn_block (DECL_INITIAL (current_function_decl));
   insn_locators_finalize ();

   /* Zap the tree EH table.  */
   set_eh_throw_stmt_table (cfun, NULL);

   rebuild_jump_labels (get_insns ());

   FOR_BB_BETWEEN (bb, ENTRY_BLOCK_PTR, EXIT_BLOCK_PTR, next_bb)
   {
      edge e;
      edge_iterator ei;
      for (ei = ei_start (bb->succs); (e = ei_safe_edge (ei)); )
      {
         if (e->insns.r)
            commit_one_edge_insertion (e);
         else
            ei_next (&ei);
      }
   }

   /* We're done expanding trees to RTL.  */
   currently_expanding_to_rtl = 0;

   FOR_BB_BETWEEN (bb, ENTRY_BLOCK_PTR->next_bb, EXIT_BLOCK_PTR, next_bb)
   {
      edge e;
      edge_iterator ei;
      for (ei = ei_start (bb->succs); (e = ei_safe_edge (ei)); )
      {
         /* Clear EDGE_EXECUTABLE.  This flag is never used in the backend.  */
         e->flags &= ~EDGE_EXECUTABLE;

         /* At the moment not all abnormal edges match the RTL
            representation.  It is safe to remove them here as
            find_many_sub_basic_blocks will rediscover them.
            In the future we should get this fixed properly.  */
         if ((e->flags & EDGE_ABNORMAL)
               && !(e->flags & EDGE_SIBCALL))
            remove_edge (e);
         else
            ei_next (&ei);
      }
   }

   blocks = sbitmap_alloc (last_basic_block);
   sbitmap_ones (blocks);
   find_many_sub_basic_blocks (blocks);
   sbitmap_free (blocks);
   purge_all_dead_edges ();

   compact_blocks ();

   expand_stack_alignment ();

#ifdef ENABLE_CHECKING
   verify_flow_info ();
#endif

   /* There's no need to defer outputting this function any more; we
      know we want to output it.  */
   DECL_DEFER_OUTPUT (current_function_decl) = 0;

   /* Now that we're done expanding trees to RTL, we shouldn't have any
      more CONCATs anywhere.  */
   generating_concat_p = 0;

   if (RTL_dump_file)
   {
      fprintf (RTL_dump_file,
               "\n\n;;\n;; Full RTL generated for this function:\n;;\n");
      /* And the pass manager will dump RTL for us.  */
   }

   /* If we're emitting a nested function, make sure its parent gets
      emitted as well.  Doing otherwise confuses debug info.  */
   {
      tree parent;
      for (parent = DECL_CONTEXT (current_function_decl);
            parent != NULL_TREE;
            parent = get_containing_scope (parent))
         if (TREE_CODE (parent) == FUNCTION_DECL)
            TREE_SYMBOL_REFERENCED (DECL_ASSEMBLER_NAME (parent)) = 1;
   }

   /* We are now committed to emitting code for this function.  Do any
      preparation, such as emitting abstract debug info for the inline
      before it gets mangled by optimization.  */
   if (cgraph_function_possibly_inlined_p (current_function_decl))
      (*debug_hooks->outlining_inline_function) (current_function_decl);

   TREE_ASM_WRITTEN (current_function_decl) = 1;

   /* After expanding, the return labels are no longer needed. */
   return_label = NULL;
   naked_return_label = NULL;
   /* Tag the blocks with a depth number so that change_scope can find
      the common parent easily.  */
   set_block_levels (DECL_INITIAL (cfun->decl), 0);
   default_rtl_profile ();
   SerializeGimpleEnd();
   return 0;
}

/* If we need to produce a detailed dump, print the tree representation
   for STMT to the dump file.  SINCE is the last RTX after which the RTL
   generated for STMT should have been appended.  */

static void
maybe_dump_rtl_for_gimple_stmt (GIMPLE_type stmt, rtx since)
{
   if (RTL_dump_file)
   {
      unsigned int index;
      struct tree_2_ints *n, in;

      fprintf (RTL_dump_file, "\n;;# ");
      print_gimple_stmt (RTL_dump_file, stmt, 0, TDF_SLIM);
      in.key = (tree) stmt;
      n = (struct tree_2_ints *) htab_find (di_local_nodes_index, &in);
      if (n)
      {
         index = (unsigned int)n->value;
      }
      else
      {
         index = 0;
      }
      fprintf (RTL_dump_file, "#//TREE_NODE_INDEX %d\n", index);

      print_rtl (RTL_dump_file, since ? NEXT_INSN (since) : since);
   }
}

/* A debugging aid for expand_used_vars.  Dump the generated partitions.  */

static void
dump_stack_var_partition (void)
{
}


