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
* @file plugin_topfname.c
* @brief Plugin making visible only the top name function
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*
*/
#include "plugin_includes.h"

int plugin_is_GPL_compatible;
static char TopFunctionName[1024];

#if __GNUC__ == 4 && __GNUC_MINOR__ == 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include "calls.h"

/* Return true when NODE can not be local. Worker for cgraph_local_node_p.  */

static bool
cgraph_non_local_node_p_1 (struct cgraph_node *node, void *data ATTRIBUTE_UNUSED)
{
   /* FIXME: Aliases can be local, but i386 gets thunks wrong then.  */
   return !(cgraph_only_called_directly_or_aliased_p (node)
	    && !ipa_ref_has_aliases_p (&node->ref_list)
	    && node->definition
	    && !DECL_EXTERNAL (node->decl)
	    && !node->externally_visible
	    && !node->used_from_other_partition
	    && !node->in_other_partition);
}

/* Return true when function can be marked local.  */

static bool
cgraph_local_node_p (struct cgraph_node *node)
{
   struct cgraph_node *n = cgraph_function_or_thunk_node (node, NULL);

   /* FIXME: thunks can be considered local, but we need prevent i386
      from attempting to change calling convention of them.  */
   if (n->thunk.thunk_p)
     return false;
   return !cgraph_for_node_and_aliases (n,
					cgraph_non_local_node_p_1, NULL, true);
					
}


/* Return true when there is a reference to node and it is not vtable.  */
static bool
address_taken_from_non_vtable_p (symtab_node *node)
{
  int i;
  struct ipa_ref *ref;
  for (i = 0; ipa_ref_list_referring_iterate (&node->ref_list,
					     i, ref); i++)
    if (ref->use == IPA_REF_ADDR)
      {
	varpool_node *node;
	if (is_a <cgraph_node> (ref->referring))
	  return true;
	node = ipa_ref_referring_varpool_node (ref);
	if (!DECL_VIRTUAL_P (node->decl))
	  return true;
      }
  return false;
}

/* A helper for comdat_can_be_unshared_p.  */

static bool
comdat_can_be_unshared_p_1 (symtab_node *node)
{
  if (!node->externally_visible)
    return true;
  /* When address is taken, we don't know if equality comparison won't
     break eventually. Exception are virutal functions, C++
     constructors/destructors and vtables, where this is not possible by
     language standard.  */
  if (!DECL_VIRTUAL_P (node->decl)
      && (TREE_CODE (node->decl) != FUNCTION_DECL
	  || (!DECL_CXX_CONSTRUCTOR_P (node->decl)
	      && !DECL_CXX_DESTRUCTOR_P (node->decl)))
      && address_taken_from_non_vtable_p (node))
    return false;

  /* If the symbol is used in some weird way, better to not touch it.  */
  if (node->force_output)
    return false;


  /* Non-readonly and volatile variables can not be duplicated.  */
  if (is_a <varpool_node> (node)
      && (!TREE_READONLY (node->decl)
	  || TREE_THIS_VOLATILE (node->decl)))
    return false;
  return true;
}

/* COMDAT functions must be shared only if they have address taken,
   otherwise we can produce our own private implementation with
   -fwhole-program.  
   Return true when turning COMDAT functoin static can not lead to wrong
   code when the resulting object links with a library defining same COMDAT.

   Virtual functions do have their addresses taken from the vtables,
   but in C++ there is no way to compare their addresses for equality.  */

static bool
comdat_can_be_unshared_p (symtab_node *node)
{
  if (!comdat_can_be_unshared_p_1 (node))
    return false;
  if (node->same_comdat_group)
    {
      symtab_node *next;

      /* If more than one function is in the same COMDAT group, it must
         be shared even if just one function in the comdat group has
         address taken.  */
      for (next = node->same_comdat_group;
	   next != node; next = next->same_comdat_group)
        if (!comdat_can_be_unshared_p_1 (next))
          return false;
    }
  return true;
}

/* Return true when function NODE should be considered externally visible.  */

static bool
cgraph_externally_visible_p (struct cgraph_node *node)
{
  if (!node->definition)
    return false;
  if (!TREE_PUBLIC (node->decl)
      || DECL_EXTERNAL (node->decl))
    return false;

  /* Do not try to localize built-in functions yet.  One of problems is that we
     end up mangling their asm for WHOPR that makes it impossible to call them
     using the implicit built-in declarations anymore.  Similarly this enables
     us to remove them as unreachable before actual calls may appear during
     expansion or folding.  */
  if (DECL_BUILT_IN (node->decl))
    return true;

  /* If linker counts on us, we must preserve the function.  */
  if (symtab_used_from_object_file_p (node))
    return true;
  if (DECL_PRESERVE_P (node->decl))
    return true;
  if (lookup_attribute ("externally_visible",
			DECL_ATTRIBUTES (node->decl)))
    return true;
  if (TARGET_DLLIMPORT_DECL_ATTRIBUTES
      && lookup_attribute ("dllexport",
			   DECL_ATTRIBUTES (node->decl)))
    return true;
  if (node->resolution == LDPR_PREVAILING_DEF_IRONLY)
    return false;
  /* When doing LTO or whole program, we can bring COMDAT functoins static.
     This improves code quality and we know we will duplicate them at most twice
     (in the case that we are not using plugin and link with object file
      implementing same COMDAT)  */
  if (DECL_COMDAT (node->decl)
      && comdat_can_be_unshared_p (node))
    return false;

#if 0
  /* When doing link time optimizations, hidden symbols become local.  */
  if ((DECL_VISIBILITY (node->decl) == VISIBILITY_HIDDEN
	  || DECL_VISIBILITY (node->decl) == VISIBILITY_INTERNAL)
      /* Be sure that node is defined in IR file, not in other object
	 file.  In that case we don't set used_from_other_object_file.  */
      && node->definition)
    ;
#endif
  if (!strcmp(IDENTIFIER_POINTER(IDENTIFIER_NODE_CHECK (DECL_NAME (node->decl))), TopFunctionName))
    return true;

  return false;
}

/* Return true if reference to NODE can be replaced by a local alias.
   Local aliases save dynamic linking overhead and enable more optimizations.
 */

static bool
can_replace_by_local_alias (symtab_node *node)
{
  return (symtab_node_availability (node) > AVAIL_OVERWRITABLE
	  && !symtab_can_be_discarded (node));
}

/* Return true when variable VNODE should be considered externally visible.  */

static bool
varpool_externally_visible_p_local (varpool_node *vnode)
{
  if (DECL_EXTERNAL (vnode->decl))
    return true;

  if (!TREE_PUBLIC (vnode->decl))
    return false;

  /* If linker counts on us, we must preserve the function.  */
  if (symtab_used_from_object_file_p (vnode))
    return true;

  if (DECL_HARD_REGISTER (vnode->decl))
    return true;
  if (DECL_PRESERVE_P (vnode->decl))
    return true;
  if (lookup_attribute ("externally_visible",
			DECL_ATTRIBUTES (vnode->decl)))
    return true;
  if (TARGET_DLLIMPORT_DECL_ATTRIBUTES
      && lookup_attribute ("dllexport",
			   DECL_ATTRIBUTES (vnode->decl)))
    return true;

  /* See if we have linker information about symbol not being used or
     if we need to make guess based on the declaration.

     Even if the linker clams the symbol is unused, never bring internal
     symbols that are declared by user as used or externally visible.
     This is needed for i.e. references from asm statements.   */
  if (symtab_used_from_object_file_p (vnode))
    return true;
  if (vnode->resolution == LDPR_PREVAILING_DEF_IRONLY)
    return false;

  /* As a special case, the COMDAT virtual tables can be unshared.
     In LTO mode turn vtables into static variables.  The variable is readonly,
     so this does not enable more optimization, but referring static var
     is faster for dynamic linking.  Also this match logic hidding vtables
     from LTO symbol tables.  */
  if (DECL_COMDAT (vnode->decl)
      && comdat_can_be_unshared_p (vnode))
    return false;

#if 0
  /* When doing link time optimizations, hidden symbols become local.  */
  if ((DECL_VISIBILITY (vnode->decl) == VISIBILITY_HIDDEN
	  || DECL_VISIBILITY (vnode->decl) == VISIBILITY_INTERNAL)
      /* Be sure that node is defined in IR file, not in other object
	 file.  In that case we don't set used_from_other_object_file.  */
      && vnode->definition)
    ;
#endif

  /* Do not attempt to privatize COMDATS by default.
     This would break linking with C++ libraries sharing
     inline definitions.

     FIXME: We can do so for readonly vars with no address taken and
     possibly also for vtables since no direct pointer comparsion is done.
     It might be interesting to do so to reduce linking overhead.  */
  if (DECL_COMDAT (vnode->decl) || DECL_WEAK (vnode->decl))
    return true;
  return false;
}


/* Mark visibility of all functions.

   A local function is one whose calls can occur only in the current
   compilation unit and all its calls are explicit, so we can change
   its calling convention.  We simply mark all static functions whose
   address is not taken as local.

   We also change the TREE_PUBLIC flag of all declarations that are public
   in language point of view but we want to overwrite this default
   via visibilities for the backend point of view.  */

static unsigned int
function_and_variable_visibility ()
{
  struct cgraph_node *node;
  varpool_node *vnode;

  /* All aliases should be procssed at this point.  */
  gcc_checking_assert (!alias_pairs || !alias_pairs->length ());

  FOR_EACH_FUNCTION (node)
    {
      int flags = flags_from_decl_or_type (node->decl);

      /* Optimize away PURE and CONST constructors and destructors.  */
      if (optimize
	  && (flags & (ECF_CONST | ECF_PURE))
	  && !(flags & ECF_LOOPING_CONST_OR_PURE))
	{
	  DECL_STATIC_CONSTRUCTOR (node->decl) = 0;
	  DECL_STATIC_DESTRUCTOR (node->decl) = 0;
	}

      /* Frontends and alias code marks nodes as needed before parsing is finished.
	 We may end up marking as node external nodes where this flag is meaningless
	 strip it.  */
      if (DECL_EXTERNAL (node->decl) || !node->definition)
	{
	  node->force_output = 0;
	  node->forced_by_abi = 0;
	}

      /* C++ FE on lack of COMDAT support create local COMDAT functions
	 (that ought to be shared but can not due to object format
	 limitations).  It is necessary to keep the flag to make rest of C++ FE
	 happy.  Clear the flag here to avoid confusion in middle-end.  */
      if (DECL_COMDAT (node->decl) && !TREE_PUBLIC (node->decl))
        DECL_COMDAT (node->decl) = 0;

      /* For external decls stop tracking same_comdat_group. It doesn't matter
	 what comdat group they are in when they won't be emitted in this TU.  */
      if (node->same_comdat_group && DECL_EXTERNAL (node->decl))
	{
#ifdef ENABLE_CHECKING
	  symtab_node *n;

	  for (n = node->same_comdat_group;
	       n != node;
	       n = n->same_comdat_group)
	      /* If at least one of same comdat group functions is external,
		 all of them have to be, otherwise it is a front-end bug.  */
	      gcc_assert (DECL_EXTERNAL (n->decl));
#endif
	  symtab_dissolve_same_comdat_group_list (node);
	}
      gcc_assert ((!DECL_WEAK (node->decl)
		  && !DECL_COMDAT (node->decl))
      	          || TREE_PUBLIC (node->decl)
		  || node->weakref
		  || DECL_EXTERNAL (node->decl));
      if (cgraph_externally_visible_p (node))
        {
	  gcc_assert (!node->global.inlined_to);
	  node->externally_visible = true;
	}
      else
	{
	  node->externally_visible = false;
	  node->forced_by_abi = false;
	}
      if (!node->externally_visible
	  && node->definition && !node->weakref
	  && !DECL_EXTERNAL (node->decl))
	{
	  node->unique_name = ((node->resolution == LDPR_PREVAILING_DEF_IRONLY
				|| node->unique_name
				|| node->resolution == LDPR_PREVAILING_DEF_IRONLY_EXP)
				&& TREE_PUBLIC (node->decl));
	  node->resolution = LDPR_PREVAILING_DEF_IRONLY;
	  if (node->same_comdat_group && TREE_PUBLIC (node->decl))
	    {
	      symtab_node *next = node;

	      /* Set all members of comdat group local.  */
	      if (node->same_comdat_group)
		for (next = node->same_comdat_group;
		     next != node;
		     next = next->same_comdat_group)
		{
		  symtab_make_decl_local (next->decl);
		  next->unique_name = ((next->resolution == LDPR_PREVAILING_DEF_IRONLY
					|| next->unique_name
					|| next->resolution == LDPR_PREVAILING_DEF_IRONLY_EXP)
					&& TREE_PUBLIC (next->decl));
		}
	      /* cgraph_externally_visible_p has already checked all other nodes
	         in the group and they will all be made local.  We need to
	         dissolve the group at once so that the predicate does not
	         segfault though. */
	      symtab_dissolve_same_comdat_group_list (node);
	    }
	  symtab_make_decl_local (node->decl);
	}

      if (node->thunk.thunk_p
	  && TREE_PUBLIC (node->decl))
	{
	  struct cgraph_node *decl_node = node;

	  decl_node = cgraph_function_node (decl_node->callees->callee, NULL);

	  /* Thunks have the same visibility as function they are attached to.
	     Make sure the C++ front end set this up properly.  */
	  if (DECL_ONE_ONLY (decl_node->decl))
	    {
	      gcc_checking_assert (DECL_COMDAT (node->decl)
				   == DECL_COMDAT (decl_node->decl));
	      gcc_checking_assert (DECL_COMDAT_GROUP (node->decl)
				   == DECL_COMDAT_GROUP (decl_node->decl));
	      gcc_checking_assert (node->same_comdat_group);
	    }
	  node->forced_by_abi = decl_node->forced_by_abi;
	  if (DECL_EXTERNAL (decl_node->decl))
	    DECL_EXTERNAL (node->decl) = 1;
	}

      /* If whole comdat group is used only within LTO code, we can dissolve it,
	 we handle the unification ourselves.
	 We keep COMDAT and weak so visibility out of DSO does not change.
	 Later we may bring the symbols static if they are not exported.  */
      if (DECL_ONE_ONLY (node->decl)
	  && (node->resolution == LDPR_PREVAILING_DEF_IRONLY
	      || node->resolution == LDPR_PREVAILING_DEF_IRONLY_EXP))
	{
	  symtab_node *next = node;

	  if (node->same_comdat_group)
	    for (next = node->same_comdat_group;
		 next != node;
		 next = next->same_comdat_group)
	      if (next->externally_visible
		  && (next->resolution != LDPR_PREVAILING_DEF_IRONLY
		      && next->resolution != LDPR_PREVAILING_DEF_IRONLY_EXP))
		break;
	  if (node == next)
	    {
	      if (node->same_comdat_group)
	        for (next = node->same_comdat_group;
		     next != node;
		     next = next->same_comdat_group)
		{
		  DECL_COMDAT_GROUP (next->decl) = NULL;
		  DECL_WEAK (next->decl) = false;
		}
	      DECL_COMDAT_GROUP (node->decl) = NULL;
	      symtab_dissolve_same_comdat_group_list (node);
	    }
	}
    }
  FOR_EACH_DEFINED_FUNCTION (node)
    {
      node->local.local |= cgraph_local_node_p (node);

      /* If we know that function can not be overwritten by a different semantics
	 and moreover its section can not be discarded, replace all direct calls
	 by calls to an nonoverwritable alias.  This make dynamic linking
	 cheaper and enable more optimization.

	 TODO: We can also update virtual tables.  */
      if (node->callers && can_replace_by_local_alias (node))
	{
	  struct cgraph_node *alias = cgraph (symtab_nonoverwritable_alias (node));

	  if (alias && alias != node)
	    {
	      while (node->callers)
		{
		  struct cgraph_edge *e = node->callers;

		  cgraph_redirect_edge_callee (e, alias);
		  if (gimple_has_body_p (e->caller->decl))
		    {
		      push_cfun (DECL_STRUCT_FUNCTION (e->caller->decl));
		      cgraph_redirect_edge_call_stmt_to_callee (e);
		      pop_cfun ();
		    }
		}
	    }
	}
    }
  FOR_EACH_VARIABLE (vnode)
    {
      /* weak flag makes no sense on local variables.  */
      gcc_assert (!DECL_WEAK (vnode->decl)
		  || vnode->weakref
      		  || TREE_PUBLIC (vnode->decl)
		  || DECL_EXTERNAL (vnode->decl));
      /* In several cases declarations can not be common:

	 - when declaration has initializer
	 - when it is in weak
	 - when it has specific section
	 - when it resides in non-generic address space.
	 - if declaration is local, it will get into .local common section
	   so common flag is not needed.  Frontends still produce these in
	   certain cases, such as for:

	     static int a __attribute__ ((common))

	 Canonicalize things here and clear the redundant flag.  */
      if (DECL_COMMON (vnode->decl)
	  && (!(TREE_PUBLIC (vnode->decl)
	      || DECL_EXTERNAL (vnode->decl))
	      || (DECL_INITIAL (vnode->decl)
		  && DECL_INITIAL (vnode->decl) != error_mark_node)
	      || DECL_WEAK (vnode->decl)
	      || DECL_SECTION_NAME (vnode->decl) != NULL
	      || ! (ADDR_SPACE_GENERIC_P
		    (TYPE_ADDR_SPACE (TREE_TYPE (vnode->decl))))))
	DECL_COMMON (vnode->decl) = 0;
    }
  FOR_EACH_DEFINED_VARIABLE (vnode)
    {
      if (!vnode->definition)
        continue;
      if (varpool_externally_visible_p_local (vnode))
	vnode->externally_visible = true;
      else
	{
          vnode->externally_visible = false;
	  vnode->forced_by_abi = false;
	}
      if (!vnode->externally_visible
	  && !vnode->weakref)
	{
	  vnode->unique_name = ((vnode->resolution == LDPR_PREVAILING_DEF_IRONLY
				       || vnode->resolution == LDPR_PREVAILING_DEF_IRONLY_EXP)
				       && TREE_PUBLIC (vnode->decl));
	  symtab_make_decl_local (vnode->decl);
	  if (vnode->same_comdat_group)
	    symtab_dissolve_same_comdat_group_list (vnode);
	  vnode->resolution = LDPR_PREVAILING_DEF_IRONLY;
	}
    }

  cgraph_function_flags_ready = true;
  return 0;
}


/* Bring functionss local at LTO time with -fwhole-program.  */

static unsigned int
whole_program_function_and_variable_visibility (void)
{
  printf("optimize\n");
  function_and_variable_visibility ();
  if (optimize)
    ipa_discover_readonly_nonaddressable_vars ();
  return 0;
}
#pragma GCC diagnostic pop

#else
static unsigned int
whole_program_function_and_variable_visibility (void)
{
  return 0;
}

#endif
unsigned int TopFnameIPA (void)
{
  gcc_assert (!flag_whole_program);
  gcc_assert(*TopFunctionName);
  //printf("TopFnameIPA %s\n", TopFunctionName);
  whole_program_function_and_variable_visibility();
  return 0;
}


#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
namespace {
const pass_data pass_data_topfname =
#else
static struct
ipa_opt_pass_d pass_topfname =
#endif
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)
  {
#endif
    IPA_PASS,
    "topfname",				/* name */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
  OPTGROUP_NONE,                        /* optinfo_flags */
#endif
#if (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
    true, /* has_gate */
    true, /* has_execute */
#elif (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)
    is_alwaysTrue,				/* gate */
    TopFnameIPA, 				/* execute */
    NULL,					/* sub */
    NULL,					/* next */
    0,						/* static_pass_number */
#endif
    TV_CGRAPHOPT,				/* tv_id */
    0,						/* properties_required */
    0,						/* properties_provided */
    0,						/* properties_destroyed */
    0,						/* todo_flags_start */
    (/* todo_flags_finish */
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
    TODO_dump_cgraph|
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 6)
    TODO_dump_cgraph|TODO_ggc_collect|
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 7)
    TODO_dump_cgraph|TODO_ggc_collect|
#elif (__GNUC__ == 4 && __GNUC_MINOR__ == 8)
    TODO_dump_symtab|TODO_ggc_collect|
#else
    TODO_dump_symtab|
#endif
    TODO_remove_functions)
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)
  },
 NULL,					/* generate_summary */
 NULL,					/* write_summary */
 NULL,					/* read_summary */
#if (__GNUC__ == 4 && __GNUC_MINOR__ != 5)
 NULL,					/* write_optimization_summary */
#endif
 NULL,					/* read_optimization_summary */
 NULL,					/* stmt_fixup */
 0,					/* TODOs */
 NULL,					/* function_transform */
 NULL					/* variable_transform */
#endif
};

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
class pass_topfname : public ipa_opt_pass_d
{
public:
  pass_topfname(gcc::context *ctxt)
    : ipa_opt_pass_d(pass_data_topfname, ctxt,
		      NULL, /* generate_summary */
		      NULL, /* write_summary */
		      NULL, /* read_summary */
		      NULL, /* write_optimization_summary */
		      NULL, /* read_optimization_summary */
		      NULL, /* stmt_fixup */
		      0, /* function_transform_todo_flags_start */
		      NULL, /* function_transform */
		      NULL) /* variable_transform */
  {}

  /* opt_pass methods: */
  bool gate (
#if __GNUC__ > 4 
               function *
#endif
            ) { return !flag_ltrans; }
  unsigned int execute (
#if __GNUC__ > 4 
                         function *
#endif
                       ) { return TopFnameIPA (); }

}; // class pass_topfname
	
} // anon namespace
ipa_opt_pass_d *
make_pass_topfname (gcc::context *ctxt)
{
  return new pass_topfname (ctxt);
}
#endif



int
plugin_init (struct plugin_name_args *plugin_info,
             struct plugin_gcc_version *version)
{
  if (!plugin_default_version_check (version, &gcc_version))
    return 1;

  struct register_pass_info pass_info;
  int argc_index;
  TopFunctionName[0]=0;
  for(argc_index=0; argc_index<plugin_info->argc; ++argc_index)
  {
    if(strcmp(plugin_info->argv[argc_index].key, "topfname")==0)
    {
       if(strlen(plugin_info->argv[argc_index].value)>1024)
         return 1;
       strcpy(TopFunctionName,plugin_info->argv[argc_index].value);
       break;
    }
  }

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
  pass_info.pass = make_pass_topfname(g);
#else
  pass_info.pass = &pass_topfname.pass;
#endif
  pass_info.reference_pass_name = "whole-program";

  /* Do it for every instance if it is 0. */
  pass_info.ref_pass_instance_number = 1;

  /* This pass can go almost anywhere as long as you're in SSA form */
  pass_info.pos_op = PASS_POS_INSERT_BEFORE;

  /* Register this new pass with GCC */
  register_callback (plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);

  return 0;
}


