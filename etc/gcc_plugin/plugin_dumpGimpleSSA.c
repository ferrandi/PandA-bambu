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
* @file plugin_dumpGimpleSSA.c
* @brief Plugin to dump functions and global variables
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
* @author Marco Lattuada <marco.lattuada@polimi.it>
*
*/
#include "plugin_includes.h"

extern unsigned int gimplePssa (void);
extern void DumpGimpleInit(char *);
static char outdir_name[1024];


#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
namespace {
const pass_data pass_data_dump_GSSA =
#else
static struct
gimple_opt_pass pass_dump_GSSA =
#endif
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)
  {
#endif
    GIMPLE_PASS,
    "gimplePSSA",				/* name */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
  OPTGROUP_NONE,                        /* optinfo_flags */
#endif
#if (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
    true, /* has_gate */
    true, /* has_execute */
#elif (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)
    is_alwaysTrue,				/* gate */
    gimplePssa, 				/* execute */
    NULL,					/* sub */
    NULL,					/* next */
    0,						/* static_pass_number */
#endif
    TV_DUMP,					/* tv_id */
    (PROP_cfg | PROP_ssa),			/* properties_required */
    0,						/* properties_provided */
    0,						/* properties_destroyed */
    TODO_rebuild_alias,	    /* todo_flags_start */
    (/* todo_flags_finish */
#if __GNUC__ == 4 
     TODO_verify_ssa| 
#endif
    TODO_cleanup_cfg
    | TODO_update_ssa)
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)
  }
#endif
};

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
class pass_dump_GSSA : public gimple_opt_pass
{
public:
  pass_dump_GSSA(gcc::context *ctxt)
    : gimple_opt_pass(pass_data_dump_GSSA, ctxt)
  {}

  /* opt_pass methods: */
  bool gate (
#if __GNUC__ > 4 
               function *
#endif
            ) { return true; }
  unsigned int execute (
#if __GNUC__ > 4 
                         function *
#endif
                       ) { return gimplePssa (); }

}; // class pass_dump_GSSA
	
} // anon namespace
gimple_opt_pass *
make_pass_dump_GSSA (gcc::context *ctxt)
{
  return new pass_dump_GSSA (ctxt);
}
#endif


#if __GNUC__ > 5
namespace {
   const pass_data pass_data_empty =
   {
       GIMPLE_PASS,
       "empty",				/* name */
       OPTGROUP_NONE,                        /* optinfo_flags */
       TV_TREE_CALL_CDCE,					/* tv_id */
      ( PROP_cfg | PROP_ssa ), /* properties_required */
      0, /* properties_provided */
      0, /* properties_destroyed */
      0, /* todo_flags_start */
      0, /* todo_flags_finish */
   };
class pass_empty : public gimple_opt_pass
{
public:
  pass_empty(gcc::context *ctxt)
    : gimple_opt_pass(pass_data_empty, ctxt)
  {}

  /* opt_pass methods: */
  bool gate (function *) { return true; }
  unsigned int execute (function *) { return 0; }

}; // class pass_empty
} // anon namespace
gimple_opt_pass *
make_pass_empty (gcc::context *ctxt)
{
  return new pass_empty (ctxt);
}
#endif

int
plugin_init (struct plugin_name_args *plugin_info,
             struct plugin_gcc_version *version)
{
  if (!plugin_default_version_check (version, &gcc_version))
    return 1;

  int argc_index;
  outdir_name[0]=0;
  for(argc_index=0; argc_index<plugin_info->argc; ++argc_index)
  {
    if(strcmp(plugin_info->argv[argc_index].key, "outputdir")==0)
    {
       if(strlen(plugin_info->argv[argc_index].value)>1024)
         return 1;
       strcpy(outdir_name,plugin_info->argv[argc_index].value);
       break;
    }
  }

  struct register_pass_info pass_info;
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
  pass_info.pass = make_pass_dump_GSSA(g);
#else
  pass_info.pass = &pass_dump_GSSA.pass;
#endif
  pass_info.reference_pass_name = "optimized";

  DumpGimpleInit(outdir_name);

  /* Do it for every instance if it is 0. */
  pass_info.ref_pass_instance_number = 1;

  /* This pass can go almost anywhere as long as you're in SSA form */
  pass_info.pos_op = PASS_POS_INSERT_AFTER;

  pass_init_dump_file (pass_info.pass);

  /* Register this new pass with GCC */
  register_callback (plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);
  serialize_state = -1;

#if __GNUC__ > 5
   //replace cdce with an empty pass
  pass_info.pass = make_pass_empty(g);
  pass_info.reference_pass_name = "cdce";
  pass_info.ref_pass_instance_number = 1;
  pass_info.pos_op = PASS_POS_REPLACE;
  register_callback ("empty_cdce", PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);
  //replace widening_mul with an empty pass
 pass_info.pass = make_pass_empty(g);
 pass_info.reference_pass_name = "widening_mul";
 pass_info.ref_pass_instance_number = 1;
 pass_info.pos_op = PASS_POS_REPLACE;
 register_callback ("empty_widening_mul", PLUGIN_PASS_MANAGER_SETUP, NULL,
                    &pass_info);

#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
  pass_info.pass = make_pass_forwprop(g);
  pass_info.reference_pass_name = "slsr";
  pass_info.pos_op = PASS_POS_INSERT_BEFORE;
  pass_info.ref_pass_instance_number = 0;
  register_callback ("fwdbslsr", PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);
  pass_info.pass = make_pass_ch(g);
  pass_info.reference_pass_name = "pre";
  pass_info.pos_op = PASS_POS_INSERT_AFTER;
  pass_info.ref_pass_instance_number = 1;
  register_callback ("chapre", PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);
#endif

//#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ == 9)
#if 0
  extern  gimple_opt_pass * make_pass_sra_earlyAggressive (gcc::context *ctxt);
  pass_info.pass = make_pass_sra_earlyAggressive(g);
  pass_info.reference_pass_name = "esra";
  pass_info.pos_op = PASS_POS_REPLACE;
  pass_info.ref_pass_instance_number = 1;
  register_callback ("esraAggressive", PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);
  extern gimple_opt_pass * make_pass_sraAggressive (gcc::context *ctxt);
  pass_info.pass = make_pass_sraAggressive(g);
  pass_info.reference_pass_name = "sra";
  pass_info.pos_op = PASS_POS_REPLACE;
  pass_info.ref_pass_instance_number = 1;
  register_callback ("sraAggressive", PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);
  extern gimple_opt_pass * make_pass_early_ipa_sraAggressive (gcc::context *ctxt);
  pass_info.pass = make_pass_early_ipa_sraAggressive(g);
  pass_info.reference_pass_name = "eipa_sra";
  pass_info.pos_op = PASS_POS_REPLACE;
  pass_info.ref_pass_instance_number = 1;
  register_callback ("eipa_sraAggressive", PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);
#endif

  return 0;
}


