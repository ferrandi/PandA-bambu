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
* @file plugin_dumpGimpleSSAVRP.c
* @brief
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*
*/
#include "plugin_includes.h"

extern void store_VRPs();

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#include "patched-tree-vrp.490.c"
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#include "patched-tree-vrp.480.c"
#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
#include "patched-tree-vrp.472.c"
#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#include "patched-tree-vrp.463.c"
#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#include "patched-tree-vrp.454.c"
#else
#error plugin_dumpSSAVRP can be created only for gcc versions greater than 4.5.4
#endif

#include "VRP_data.h"

static void restrict_range_to_consts()
{
  size_t i;
  unsigned num_vr_values = num_ssa_names;
  for (i = 0; i < num_vr_values; i++)
    if (vr_value[i])
      {
        value_range_t *vr = vr_value[i];
        tree type = TREE_TYPE (ssa_name(i));
        tree minimum = NULL;
        tree maximum = NULL;
        unsigned var_prec = TYPE_PRECISION(type);

	//fprintf(stderr, "%ld\n", i);

        if (INTEGRAL_TYPE_P(type) && vr->min && vr->max)
	  {
	    bool is_neg_inf = is_negative_overflow_infinity (vr->min) || 
                 (INTEGRAL_TYPE_P (type)
	           && !TYPE_UNSIGNED (type)
	           && vrp_val_is_min (vr->min));
	           
	    bool is_pos_inf = is_positive_overflow_infinity (vr->max) || 
	        (INTEGRAL_TYPE_P (type)
	         && vrp_val_is_max (vr->max));
	    if(TREE_CODE (vr->min) != INTEGER_CST && !is_neg_inf)
	    {
	      /// check if greater than zero
	      bool strict_overflow_p;
	      tree val = compare_name_with_value(GE_EXPR, ssa_name(i), integer_zero_node, &strict_overflow_p);
	      if(!strict_overflow_p && val)
	      {
	        if(integer_onep (val))
	        {
	          minimum = integer_zero_node;
	        }
	        else
	        {
	          tree neg_const;
	          unsigned prec_index = 1;
	          while(prec_index < var_prec && !strict_overflow_p)
	          {
	            neg_const = build_int_cst (type, -(((unsigned HOST_WIDE_INT)1) << prec_index));
	            tree val = compare_name_with_value(GE_EXPR, ssa_name(i), neg_const, &strict_overflow_p);
	            if(val && integer_onep (val))
	            {
	              minimum = neg_const;
	              break;
	            }
	            ++prec_index;
	          }
	        }
	      }
	    } else if(is_neg_inf)
	      minimum = vr->min;
	    
	    if(TREE_CODE (vr->max) != INTEGER_CST && !is_pos_inf)
	    {
	      bool strict_overflow_p=false;
	      tree pos_const;
	      unsigned prec_index = 0;
	      while(prec_index < var_prec && !strict_overflow_p)
	      {
	        pos_const = build_int_cst (type, (((unsigned HOST_WIDE_INT)1) << prec_index));
	        tree val = compare_name_with_value(LT_EXPR, ssa_name(i), pos_const, &strict_overflow_p);
	        if(val && integer_onep (val))
	        {
	          maximum = build_int_cst (type, (((unsigned HOST_WIDE_INT)1) << prec_index)-1);
	          break;
	        }
	        ++prec_index;
	      }
	    }
	    else if(is_pos_inf)
	      maximum = vr->max;
	      
	    if(minimum)
	    {
	      vr->min = minimum;
	      vr->type = VR_RANGE;
	    }
	    if(maximum)
	    {
	      vr->max = maximum;
	      vr->type = VR_RANGE;
	    }
	  }
    }
  // do further restrictions by exploiting assert_expr
  for (i = 0; i < num_vr_values; i++)
    if (vr_value[i])
    {
      tree type = TREE_TYPE (ssa_name(i));
      value_range_t *vr = vr_value[i];
      if(INTEGRAL_TYPE_P(type) && vr->type == VR_RANGE && vr->min && vr->max)
      {
        tree sa_var = ssa_name(i);
        GIMPLE_type def_stmt = SSA_NAME_DEF_STMT (sa_var );
        if(is_gimple_assign (def_stmt)
	  && gimple_assign_rhs_code (def_stmt) == ASSERT_EXPR)
	{
	  tree src_var = ASSERT_EXPR_VAR (gimple_assign_rhs1 (def_stmt));
	  value_range_t *src_vr = vr_value[SSA_NAME_VERSION(src_var)];
	  if(src_vr && src_vr->type == VR_RANGE && src_vr->min && src_vr->max)
	  {
	    bool strict_overflow_p=false;
	    tree val = compare_name_with_value(LT_EXPR, src_var, vr->max, &strict_overflow_p);
	    if(val && integer_onep (val))
	      vr->max = src_vr->max;
	    strict_overflow_p=false;
	    val = compare_name_with_value(GT_EXPR, src_var, vr->min, &strict_overflow_p);
	    if(val && integer_onep (val))
	      vr->min = src_vr->min;
	  }
	}
      }
    }
}


void store_VRPs()
{
  size_t i;
  unsigned num = num_ssa_names;
  if(VRP_min_array)
    free (VRP_min_array);
  if(VRP_max_array)
    free (VRP_max_array);
  if(p_VRP_min)
    free (p_VRP_min);
  if(p_VRP_max)
    free (p_VRP_max);
  VRP_min_array = XCNEWVEC (tree, num);
  VRP_max_array = XCNEWVEC (tree, num);
  p_VRP_min = XCNEWVEC (bool, num);
  p_VRP_max = XCNEWVEC (bool, num);

  //restrict_range_to_consts();
  
  for (i = 0; i < num; i++)
    if (vr_value[i] && vr_value[i]->type ==  VR_RANGE && 
        TREE_CODE(vr_value[i]->min) == INTEGER_CST && 
        TREE_CODE(vr_value[i]->max) == INTEGER_CST && 
        !TREE_OVERFLOW (vr_value[i]->min) &&
        !TREE_OVERFLOW (vr_value[i]->max))
    {
      p_VRP_min[i] = true;
      p_VRP_max[i] = true;
      VRP_min_array[i] = vr_value[i]->min;
      VRP_max_array[i] = vr_value[i]->max;
    }
    else
    {
      p_VRP_min[i] = false;
      p_VRP_max[i] = false;
    }
  set_num_vr_values(num);
}

extern unsigned int gimplePssaVRP (void);

extern int serialize_state;

extern unsigned int gimplePssa (void);
extern void DumpGimpleInit(char *);
static char outdir_name[1024];

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
namespace {
const pass_data pass_data_dump_GSSA =
#else
static struct gimple_opt_pass pass_dump_GSSA =
#endif
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)
  {
#endif
    GIMPLE_PASS,
    "gimplePSSA",         /* name */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
  OPTGROUP_NONE,                        /* optinfo_flags */
#endif
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
    true, /* has_gate */
    true, /* has_execute */
#else
    is_alwaysTrue,        /* gate */
    gimplePssa,           /* execute */
    NULL,                 /* sub */
    NULL,                 /* next */
    0,                    /* static_pass_number */
#endif
    TV_DUMP,              /* tv_id */
    PROP_cfg | PROP_ssa,  /* properties_required */
    0,                    /* properties_provided */
    0,                    /* properties_destroyed */
    0,                    /* todo_flags_start */
    TODO_verify_ssa       /* todo_flags_finish */
    | TODO_cleanup_cfg
    | TODO_update_ssa
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
  bool gate () { return true; }
  unsigned int execute () { return gimplePssa (); }

}; // class pass_dump_GSSA

} // anon namespace
gimple_opt_pass *
make_pass_dump_GSSA (gcc::context *ctxt)
{
  return new pass_dump_GSSA (ctxt);
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
  pass_info.pass = make_custom_pass_vrp(g);
#else
  pass_info.pass = &custom_pass_vrp.pass;
#endif
  pass_info.reference_pass_name = "vrp";

  /* Do it for every instance if it is 0. */
  pass_info.ref_pass_instance_number = 0;

  /* This pass can go almost anywhere as long as you're in SSA form */
  pass_info.pos_op = PASS_POS_REPLACE;

  /* Register this new pass with GCC */
  register_callback (plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);
  serialize_state = -1;


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

  /* Register this new pass with GCC */
  register_callback (plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass_info);
  serialize_state = -1;

  return 0;
}


