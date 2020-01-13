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
 * @file latex_table_xml.hpp
 * @brief xml nodes used in file specifying latex table format
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef LATEX_TABLE_XML
#define LATEX_TABLE_XML

/// The node containing type of alignment
#define STR_XML_latex_table_alignment "alignment"

/// The node containing information about single column
#define STR_XML_latex_table_column "column"

/// The node containing information about the name of the column
#define STR_XML_latex_table_column_name "name"

/// The node containing information about column comparison
#define STR_XML_latex_table_comparison "compare"

/// The attribute containing information about type of comparison
#define STR_XML_latex_table_operator "operator"

/// The node containg information about the precision of the number
#define STR_XML_latex_table_precision "precision"

/// The node containing type of text to be printed
#define STR_XML_latex_table_text_format "text_format"

/// The root node
#define STR_XML_latex_table_root "latex_table_format"

/// The node containing the source name of this column
#define STR_XML_latex_table_source_name "source_name"

/// The node containing information about column total
#define STR_XML_latex_table_total "total"

/// The attribute containing the value of the field
#define STR_XML_latex_table_value "value"

#endif
