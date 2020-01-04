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
* @file plugin_dumpGimpleEmpty.c
* @brief Plugin to dump global variables defined in files not containing functions
*
* @author Marco Lattuada <marco.lattuada@polimi.it>
*
*/
#include "plugin_includes.h"

extern void serialize_global_variable(void *gcc_data, void *user_data);
static char outdir_name[1024];

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

  const char *plugin_name = plugin_info->base_name;
/*  int argc = plugin_info->argc; */
/*  struct plugin_argument *argv = plugin_info->argv;*/

  /* Register this new pass with GCC */
  register_callback (plugin_name, PLUGIN_ALL_PASSES_START, serialize_global_variable, (void*)outdir_name);
  serialize_state = -1;
  return 0;
}

