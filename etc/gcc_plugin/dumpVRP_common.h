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
* @file dumpVRP_common.h
* @brief
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*
*/
#ifndef DUMPVRP_COMMON
#define DUMPVRP_COMMON
#if  (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
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
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wundef"
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
/* Return the insn_code for a FLOAT_EXPR.  */
extern int can_float_p (enum machine_mode, enum machine_mode, int);
#define CODE_FOR_nothing 0
#endif

extern void scev_initialize(void);
extern void scev_finalize (void);

/******************************* tree-ssa-propagate.h ****************************/
/* If SIM_P is true, statement S will be simulated again.  */

static inline void
prop_set_simulate_again (GIMPLE_type s, bool visit_p)
{
  gimple_set_visited (s, visit_p);
}

/* Return true if statement T should be simulated again.  */

static inline bool
prop_simulate_again_p (GIMPLE_type s)
{
  return gimple_visited_p (s);
}


/* Lattice values used for propagation purposes.  Specific instances
   of a propagation engine must return these values from the statement
   and PHI visit functions to direct the engine.  */
enum ssa_prop_result {
    /* The statement produces nothing of interest.  No edges will be
       added to the work lists.  */
    SSA_PROP_NOT_INTERESTING,

    /* The statement produces an interesting value.  The set SSA_NAMEs
       returned by SSA_PROP_VISIT_STMT should be added to
       INTERESTING_SSA_EDGES.  If the statement being visited is a
       conditional jump, SSA_PROP_VISIT_STMT should indicate which edge
       out of the basic block should be marked executable.  */
    SSA_PROP_INTERESTING,

    /* The statement produces a varying (i.e., useless) value and
       should not be simulated again.  If the statement being visited
       is a conditional jump, all the edges coming out of the block
       will be considered executable.  */
    SSA_PROP_VARYING
};

struct prop_value_d {
    /* Lattice value.  Each propagator is free to define its own
       lattice and this field is only meaningful while propagating.
       It will not be used by substitute_and_fold.  */
    unsigned lattice_val;

    /* Propagated value.  */
    tree value;
};

typedef struct prop_value_d prop_value_t;

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
typedef enum ssa_prop_result (*ssa_prop_visit_stmt_fn) (GIMPLE_type, edge *, tree *);
typedef enum ssa_prop_result (*ssa_prop_visit_phi_fn) (GIMPLE_type);
extern void ssa_propagate (ssa_prop_visit_stmt_fn, ssa_prop_visit_phi_fn);
#endif
typedef bool (*ssa_prop_fold_stmt_fn) (gimple_stmt_iterator *gsi);
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
typedef tree (*ssa_prop_get_value_fn) (tree);
extern bool substitute_and_fold (ssa_prop_get_value_fn, ssa_prop_fold_stmt_fn, bool);
#else
bool substitute_and_fold (prop_value_t *, ssa_prop_fold_stmt_fn, bool);
#endif

/*********************************************************************************/

/******************************* tree-scalar-evolution.h *************************/
extern tree instantiate_scev (basic_block, struct loop *, tree);
extern void scev_analysis (void);
extern tree analyze_scalar_evolution (struct loop *, tree);

/* Returns the basic block preceding LOOP or ENTRY_BLOCK_PTR when the
   loop is function's body.  */

static inline basic_block
block_before_loop (loop_p loop)
{
  edge preheader = loop_preheader_edge (loop);
  return (preheader ? preheader->src : ENTRY_BLOCK_PTR);
}

/* Analyze all the parameters of the chrec that were left under a
   symbolic form.  LOOP is the loop in which symbolic names have to
   be analyzed and instantiated.  */

static inline tree
instantiate_parameters (struct loop *loop, tree chrec)
{
  return instantiate_scev (block_before_loop (loop), loop, chrec);
}

/* Returns the loop of the polynomial chrec CHREC.  */

static inline struct loop *
get_chrec_loop (const_tree chrec)
{
  return get_loop (
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
  cfun, 
#endif
  CHREC_VARIABLE (chrec)
  );
}

/*********************************************************************************/

/******************************* tree-chrec.h ************************************/
extern tree initial_condition_in_loop_num (tree, unsigned);
extern tree evolution_part_in_loop_num (tree, unsigned);

/*********************************************************************************/

#if  (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#endif
