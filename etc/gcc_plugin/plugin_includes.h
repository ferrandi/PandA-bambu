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
* @file plugin_includes.h
* @brief Common includes for plugin
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
* @author Marco Lattuada <marco.lattuada@polimi.it>
*
*/
#ifndef PLUGIN_INCLUDES
#define PLUGIN_INCLUDES
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#if (__GNUC__ < 6)
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#endif
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wundef"
#endif

#include "gcc-plugin.h"

#include "config.h"
#include "system.h"
#include "coretypes.h"
#if __GNUC__ > 5
typedef gimple* GIMPLE_type;
#else
#define GIMPLE_type gimple
#endif

#include "tm.h"
#include "tree.h"
#include "cp/cp-tree.h"
#include "intl.h"

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
#include "tree-ssa-alias.h"
#include "internal-fn.h"
#include "is-a.h"
#include "predict.h"
#include "function.h"
#include "basic-block.h"
#include "gimple-expr.h"
#endif

#include "gimple.h"
#include "tree-pass.h"
#include "plugin-version.h"
#include "langhooks.h"
#include <string.h>
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
#include "gimplify.h"
#define GET_TREE_CODE_NAME(code) (get_tree_code_name(code))
#define ENTRY_BLOCK_PTR ENTRY_BLOCK_PTR_FOR_FN (cfun)
#define EXIT_BLOCK_PTR EXIT_BLOCK_PTR_FOR_FN (cfun)
#include "gimple-iterator.h"
#include "tree-cfg.h"
#include "cgraph.h"
#else
#include "tree-flow.h"
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
#include "tree-flow-inline.h"
#endif

#define GET_TREE_CODE_NAME(code) (tree_code_name[(int)code])
#endif
#include "tree-dump.h"
#include "tree-pass.h"
#include "basic-block.h"
#include "toplev.h"
#include "tree-inline.h"
#include "tree-iterator.h"
#include "real.h"
#include "fixed-value.h"
#include "tree-ssa-operands.h"
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && !defined(SPARC) && !defined(ARM)
#include "c-family/c-common.h"
#else
#include "c-common.h"
#endif
#include <assert.h>

#include "VRP_data.h"

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) && ! defined(SPARC) && !defined(ARM)
#include "gimple-pretty-print.h"
#include "tree-pretty-print.h"
#endif
#include "diagnostic.h"
#include "splay-tree.h"
#include "ggc.h"

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
#include "context.h"
#include "stringpool.h"
#if (__GNUC__ > 6)
#include "tree-vrp.h"
#endif
#include "tree-ssanames.h"
#endif
#include "cfgloop.h"

#if __GNUC__ > 4
#include "builtins.h"
#endif

#if __GNUC__ > 7
#include "attribs.h"
#endif

#if __GNUC__ > 4
#define PTRCONV(TYPE, data)  (is_a_helper<TYPE>::test(data) ? as_a<TYPE>(data) : 0)
#else
#define PTRCONV(TYPE, data) data
#endif

extern int plugin_is_GPL_compatible;

extern int serialize_state;

extern int tree_2_ints_eq (const void *, const void *);
extern unsigned int tree_2_ints_hash (const void *);
extern int tree_2_ints_marked_p (const void *);
extern int dg_descriptor_tree_marked_p (const void *);
extern int dg_descriptor_tree_eq (const void *, const void *);
unsigned int dg_descriptor_tree_hash (const void *);

#if (__GNUC__ > 4)
#include "ggc_cpplike_extern.h"
#else
#include "ggc_clike_extern.h"
#endif

#if (__GNUC__ > 4)
#define INLOC input_location, 
#else
#define INLOC
#endif

static bool
is_alwaysTrue (void)
{
  return 1;
}
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#pragma GCC diagnostic warning "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic warning "-fpermissive"
#pragma GCC diagnostic warning "-Wpermissive"
#endif
#endif
