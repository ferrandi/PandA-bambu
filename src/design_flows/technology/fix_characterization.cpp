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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file fix_characterization.cpp
 * @brief Step to fix components characterization
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Header include
#include "fix_characterization.hpp"

///. include
#include "Parameter.hpp"

/// STD include
#include <string>

/// STL includes
#include "custom_set.hpp"
#include <vector>

/// technology include
#include "technology_manager.hpp"

/// technology/physical_library includes
#include "library_manager.hpp"
#include "technology_node.hpp"

/// technology/physical_library/models include
#include "clb_model.hpp"
#include "time_model.hpp"

/// technology/target_device include
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "target_device.hpp"

FixCharacterization::FixCharacterization(const technology_managerRef _TM, const target_deviceRef _target, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : TechnologyFlowStep(_TM, _target, _design_flow_manager, TechnologyFlowStep_Type::FIX_CHARACTERIZATION, _parameters), assignment_execution_time(0.0), connection_time(0.0)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

FixCharacterization::~FixCharacterization() = default;

void FixCharacterization::Initialize()
{
   assignment_execution_time = TM->CGetSetupHoldTime();
   assignment_characterization_timestamp = TM->CGetSetupHoldTimeStamp();

   const double connection_time_ratio = parameters->IsParameter("RelativeConnectionOffset") ? parameters->GetParameter<double>("RelativeConnectionOffset") : 0.15;
   connection_time = assignment_execution_time * connection_time_ratio;
}

DesignFlowStep_Status FixCharacterization::Exec()
{
   const auto libraries = TM->get_library_list();
   for(const auto& library : libraries)
   {
      const auto LM = TM->get_library_manager(library);
      const auto fus = LM->get_library_fu();
      for(const auto& fu : fus)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + fu.first);
         auto single_fu = GetPointer<functional_unit>(fu.second);
         if(!single_fu)
            continue;
         auto template_name = single_fu->fu_template_name;
         auto fu_name = single_fu->functional_unit_name;
         if(single_fu)
         {
            /// assignment
            if(fu_name == "ASSIGN_REAL_FU" || fu_name == "ASSIGN_SIGNED_FU" || fu_name == "ASSIGN_UNSIGNED_FU" || fu_name == "ASSIGN_VECTOR_BOOL_FU" || fu_name == "addr_expr_FU" || fu_name == "fp_view_convert_expr_FU" ||
               fu_name == "ui_view_convert_expr_FU" || fu_name == "view_convert_expr_FU" || fu_name == "assert_expr_FU")
            {
               single_fu->area_m->set_area_value(1);
               for(auto op : single_fu->get_operations())
               {
                  if(assignment_execution_time != (GetPointer<operation>(op))->time_m->get_execution_time())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing execution time of " + GetPointer<const operation>(op)->operation_name + " on " + fu.first);
                     (GetPointer<operation>(op))->time_m->set_execution_time(assignment_execution_time, 0);
                     single_fu->characterization_timestamp = assignment_characterization_timestamp;
                  }
               }
            }
            /// shift ops
            if(template_name == "rshift_expr_FU" || template_name == "ui_rshift_expr_FU" || template_name == "lshift_expr_FU" || template_name == "ui_lshift_expr_FU" || template_name == "ui_lrotate_expr_FU" || template_name == "ui_rrotate_expr_FU")
            {
               std::vector<std::string> template_parameters = SplitString(single_fu->fu_template_parameters, " ");
               THROW_ASSERT(template_parameters.size() == 3, single_fu->fu_template_parameters);
               if(template_parameters[1] == "0")
               {
                  single_fu->area_m->set_area_value(1);
                  for(auto op : single_fu->get_operations())
                  {
                     if(assignment_execution_time != (GetPointer<operation>(op))->time_m->get_execution_time())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing execution time of " + GetPointer<const operation>(op)->operation_name + " on " + fu.first);
                        (GetPointer<operation>(op))->time_m->set_execution_time(assignment_execution_time, 0);
                        single_fu->characterization_timestamp = assignment_characterization_timestamp;
                     }
                  }
               }
            }
            /// vectorize shift ops
            if(template_name == "vec_rshift_expr_FU" || template_name == "ui_vec_rshift_expr_FU" || template_name == "vec1_rshift_expr_FU" || template_name == "ui_vec1_rshift_expr_FU" || template_name == "vec_lshift_expr_FU" ||
               template_name == "ui_vec_lshift_expr_FU" || template_name == "vec1_lshift_expr_FU" || template_name == "ui_vec1_lshift_expr_FU")
            {
               std::vector<std::string> template_parameters = SplitString(single_fu->fu_template_parameters, " ");
               THROW_ASSERT(template_parameters.size() >= 4, single_fu->fu_template_parameters);
               if(template_parameters[2] == "0")
               {
                  single_fu->area_m->set_area_value(1);
                  for(auto op : single_fu->get_operations())
                  {
                     if(assignment_execution_time != (GetPointer<operation>(op))->time_m->get_execution_time())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing execution time of " + GetPointer<const operation>(op)->operation_name + " on " + fu.first);
                        (GetPointer<operation>(op))->time_m->set_execution_time(assignment_execution_time, 0);
                        single_fu->characterization_timestamp = assignment_characterization_timestamp;
                     }
                  }
               }
            }
            /// conversion
            if(fu_name == "IIdata_converter_FU" || fu_name == "UIdata_converter_FU" || fu_name == "IUdata_converter_FU" || fu_name == "UUdata_converter_FU" || fu_name == "IIconvert_expr_FU" || fu_name == "IUconvert_expr_FU" ||
               fu_name == "UIconvert_expr_FU" || fu_name == "UUconvert_expr_FU")
            {
               single_fu->area_m->set_area_value(1);
               for(auto op : single_fu->get_operations())
               {
                  if(assignment_execution_time != (GetPointer<operation>(op))->time_m->get_execution_time())
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing execution time of " + GetPointer<const operation>(op)->operation_name + " on " + fu.first);
                     (GetPointer<operation>(op))->time_m->set_execution_time(assignment_execution_time, 0);
                     single_fu->characterization_timestamp = assignment_characterization_timestamp;
                  }
               }
            }
            /// bitwise ops
            if(template_name == "bit_and_expr_FU" || template_name == "ui_bit_and_expr_FU" || template_name == "bit_ior_expr_FU" || template_name == "ui_bit_ior_expr_FU")
            {
               std::vector<std::string> template_parameters = SplitString(single_fu->fu_template_parameters, " ");
               THROW_ASSERT(template_parameters.size() == 3, single_fu->fu_template_parameters);
               if(template_parameters[0] == "0" || template_parameters[1] == "0")
               {
                  single_fu->area_m->set_area_value(1);
                  for(auto op : single_fu->get_operations())
                  {
                     if(assignment_execution_time != (GetPointer<operation>(op))->time_m->get_execution_time())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing execution time of " + GetPointer<const operation>(op)->operation_name + " on " + fu.first);
                        (GetPointer<operation>(op))->time_m->set_execution_time(assignment_execution_time, 0);
                        single_fu->characterization_timestamp = assignment_characterization_timestamp;
                     }
                  }
               }
            }
            /// vectorize bitwise ops
            if(template_name == "vec_bit_and_expr_FU" || template_name == "ui_vec_bit_and_expr_FU" || template_name == "vec_bit_ior_expr_FU" || template_name == "ui_vec_bit_ior_expr_FU")
            {
               std::vector<std::string> template_parameters = SplitString(single_fu->fu_template_parameters, " ");
               THROW_ASSERT(template_parameters.size() >= 5, single_fu->fu_template_parameters);
               if(template_parameters[0] == "0" || template_parameters[2] == "0")
               {
                  single_fu->area_m->set_area_value(1);
                  for(auto op : single_fu->get_operations())
                  {
                     if(assignment_execution_time != (GetPointer<operation>(op))->time_m->get_execution_time())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing execution time of " + GetPointer<const operation>(op)->operation_name + " on " + fu.first);
                        (GetPointer<operation>(op))->time_m->set_execution_time(assignment_execution_time, 0);
                        single_fu->characterization_timestamp = assignment_characterization_timestamp;
                     }
                  }
               }
            }
            /// cond expr ops
            if(template_name == "cond_expr_FU" || template_name == "ui_cond_expr_FU" || template_name == "fp_cond_expr_FU")
            {
               std::vector<std::string> template_parameters = SplitString(single_fu->fu_template_parameters, " ");
               THROW_ASSERT(template_parameters.size() == 4, single_fu->fu_template_parameters);
               if(template_parameters[0] == "0")
               {
                  single_fu->area_m->set_area_value(1);
                  for(auto op : single_fu->get_operations())
                  {
                     if(assignment_execution_time != (GetPointer<operation>(op))->time_m->get_execution_time())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing execution time of " + GetPointer<const operation>(op)->operation_name + " on " + fu.first);
                        (GetPointer<operation>(op))->time_m->set_execution_time(assignment_execution_time, 0);
                        single_fu->characterization_timestamp = assignment_characterization_timestamp;
                     }
                  }
               }
            }
            /// vectorize cond expr ops
            if(template_name == "vec_cond_expr_FU" || template_name == "ui_vec_cond_expr_FU")
            {
               std::vector<std::string> template_parameters = SplitString(single_fu->fu_template_parameters, " ");
               THROW_ASSERT(template_parameters.size() >= 6, single_fu->fu_template_parameters);
               if(template_parameters[0] == "0")
               {
                  single_fu->area_m->set_area_value(1);
                  for(auto op : single_fu->get_operations())
                  {
                     if(assignment_execution_time != (GetPointer<operation>(op))->time_m->get_execution_time())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing execution time of " + GetPointer<const operation>(op)->operation_name + " on " + fu.first);
                        (GetPointer<operation>(op))->time_m->set_execution_time(assignment_execution_time, 0);
                        single_fu->characterization_timestamp = assignment_characterization_timestamp;
                     }
                  }
               }
            }
#if 0
            ///64 bits plus/minus
            if(template_name == "plus_expr_FU" or template_name == "ui_plus_expr_FU" or template_name == "minus_expr_FU" or template_name == "ui_minus_expr_FU")
            {
               std::vector<std::string> template_parameters = SplitString(single_fu->fu_template_parameters, " ");
               const auto output_size = boost::lexical_cast<size_t>(template_parameters.back());
               ///Strictly larger than 32 (i.e., not 32)
               if(output_size > 32)
               {
                  for(auto op : single_fu->get_operations())
                  {
                     if(assignment_execution_time != (GetPointer<operation>(op))->time_m->get_execution_time())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing execution time of " + GetPointer<const operation>(op)->operation_name + " on " + fu.first);
                        (GetPointer<operation>(op))->time_m->set_execution_time(GetPointer<operation>(op)->time_m->get_execution_time() + connection_time, 0);
                     }
                  }
               }
            }
            ///64 bits ternary
            if(template_name == "ternary_alu_expr_FU" or template_name == "ui_ternary_alu_expr_FU" or template_name == "ternary_mm_expr_FU" or template_name == "ui_ternary_mm_expr_FU" or template_name == "ternary_mp_expr_FU" or template_name == "ui_ternary_mp_expr_FU" or template_name == "ternary_pm_expr_FU" or template_name == "ui_ternary_pm_expr_FU" or template_name == "ternary_plus_expr_FU" or template_name == "ui_ternary_plus_expr_FU")
            {
               std::vector<std::string> template_parameters = SplitString(single_fu->fu_template_parameters, " ");
               const auto output_size = boost::lexical_cast<size_t>(template_parameters.back());
               ///Strictly larger than 32 (i.e., not 32)
               if(output_size > 32)
               {
                  for(auto op : single_fu->get_operations())
                  {
                     if(assignment_execution_time != (GetPointer<operation>(op))->time_m->get_execution_time())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Fixing execution time of " + GetPointer<const operation>(op)->operation_name + " on " + fu.first);
                        (GetPointer<operation>(op))->time_m->set_execution_time(GetPointer<operation>(op)->time_m->get_execution_time() + connection_time, 0);
                     }
                  }
               }
            }
#endif
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + fu.first);
      }
   }
   /// Second iteration of fixing
   for(const auto& library : libraries)
   {
      const auto LM = TM->get_library_manager(library);
      const auto fus = LM->get_library_fu();
      for(const auto& fu : fus)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + fu.first);
         auto single_fu = GetPointer<functional_unit>(fu.second);
         if(!single_fu)
            continue;
         auto template_name = single_fu->fu_template_name;
         if(single_fu)
         {
            /// 64 bits plus/minus
            if(template_name == "plus_expr_FU" or template_name == "ui_plus_expr_FU" or template_name == "minus_expr_FU" or template_name == "ui_minus_expr_FU" or template_name == "ternary_alu_expr_FU" or template_name == "ui_ternary_alu_expr_FU" or
               template_name == "ternary_mm_expr_FU" or template_name == "ui_ternary_mm_expr_FU" or template_name == "ternary_mp_expr_FU" or template_name == "ui_ternary_mp_expr_FU" or template_name == "ternary_pm_expr_FU" or
               template_name == "ui_ternary_pm_expr_FU" or template_name == "ternary_plus_expr_FU" or template_name == "ui_ternary_plus_expr_FU")
            {
               if(single_fu->fu_template_parameters.find(" 0") != std::string::npos or (single_fu->fu_template_parameters.size() >= 2 and single_fu->fu_template_parameters.substr(0, 2) == "0 "))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fixing " + single_fu->get_name());
                  std::vector<std::string> template_parameters = SplitString(single_fu->fu_template_parameters, " ");
                  const auto output_size = boost::lexical_cast<size_t>(template_parameters.back());
                  std::string cell_name = template_name;
                  for(const auto& template_parameter : template_parameters)
                  {
                     cell_name += "_";
                     if(template_parameter == "0")
                     {
                        cell_name += STR(output_size);
                     }
                     else
                     {
                        cell_name += template_parameter;
                     }
                  }
                  const technology_nodeConstRef nc_f_unit = TM->get_fu(cell_name, TM->get_library(cell_name));
                  THROW_ASSERT(nc_f_unit, "Library miss component: " + std::string(cell_name));
                  CustomMap<std::string, double> non_constant_execution_times;
                  for(const auto& op : GetPointer<const functional_unit>(nc_f_unit)->get_operations())
                  {
                     non_constant_execution_times[op->get_name()] = GetPointer<operation>(op)->time_m->get_execution_time();
                  }
                  for(const auto& op : single_fu->get_operations())
                  {
                     const std::string operation_name = op->get_name();
                     THROW_ASSERT(non_constant_execution_times.find(operation_name) != non_constant_execution_times.end(), operation_name);
                     const auto non_constant_execution_time = non_constant_execution_times.find(operation_name)->second;
                     GetPointer<operation>(op)->time_m->set_execution_time(non_constant_execution_time, 0);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed " + single_fu->get_name());
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + fu.first);
      }
   }
   return DesignFlowStep_Status::SUCCESS;
}

const CustomUnorderedSet<TechnologyFlowStep_Type> FixCharacterization::ComputeTechnologyRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<TechnologyFlowStep_Type> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(TechnologyFlowStep_Type::LOAD_DEFAULT_TECHNOLOGY);
         relationships.insert(TechnologyFlowStep_Type::LOAD_DEVICE_TECHNOLOGY);
         relationships.insert(TechnologyFlowStep_Type::LOAD_FILE_TECHNOLOGY);
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}
