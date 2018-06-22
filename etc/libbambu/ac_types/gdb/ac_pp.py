"""************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 3.7                                                 *
 *                                                                        *
 *  Release Date    : Tue May 30 14:25:58 PDT 2017                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 3.7.2                                               *
 *                                                                        *
 *  Copyright 2013-2015, Mentor Graphics Corporation,                     *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *  
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      * 
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   * 
 *  distributed under the License is distributed on an "AS IS" BASIS,     * 
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              * 
 *  See the License for the specific language governing permissions and   * 
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  The most recent version of this package is available at github.       *
 *                                                                        *
 ***********************************************************************"""
#
# GDB Pretty Printer for AC Datatypes
#
# Authors: 
#   Lucien Murrey-Pitts
#     First version for ac_int/ac_fixed limited to 64-bits 
#   Andres Takach 
#     Arbitrary-length ac_int, ac_fixed, ac_float, ac_complex  
#
# Requires a GDB that supports Python
#   available from the "stable" GDB releases in 
#   - ftp://sourceware.org/pub/gdb/snapshots/current/
#
# Alternatives ways to import this script in gdb 
# 1. run gdb and specify -ex 'py execfile("ac_pp.py")'
#
# 2. run gdb, and type the command: 
#    py execfile("ac_pp.py")
#
# 3  load it into the .gdbinit file by adding: 
#    py execfile("ac_pp.py")
#

# Pretty print parameters for AC Datatypes
#   set pp-ac-radix {dec, hex, bin}
#   set pp-ac_int-radix {default, dec, hex, bin}
#      default -> defaults to pp-ac-radix setting
#   set pp-ac_fixed-radix {default, dec, hex, bin}
#      default -> defaults to pp-ac-radix setting
#
# Known Limitations:
#   No direct parameter control for ac_float
#     Mantissa/Exponent view affected by ac_fixed/ac_int settings
#   Printing of negative numbers in hex/bin takes the
#     2's complement and represents the number using "-"
#     in front. No setting to get actual bit pattern for them. 

import re
import decimal
import gdb

def real_type (val):
    # Get the type.
    type = val.type

    # If it points to a reference, get the reference.
    if type.code == gdb.TYPE_CODE_REF:
        type = type.target ()

    # Get the unqualified type, stripped of typedefs.
    type = type.unqualified ().strip_typedefs ()

    return type

class pp_iv:
    def __init__(self, val):
        self.val = val
        fields = real_type(val).fields()
        ts = fields[0].name
        m1 = re.match( r'.*ac_private::iv<([0-9]*)>.*', ts )
        if m1==None:
            fields = fields[0].type.fields() 
            ts = fields[0].name
            m1 = re.match( r'.*ac_private::iv<([0-9]*)>.*', ts )
        self.words = long(m1.group(1))

    def to_long(self):
        v = self.val["v"]
        int_val = long(v[self.words-1])
        for word_i in range(self.words-2, -1, -1):
            int_val *= (2 ** 32)
            int_val += long((v[word_i] & 0xffffffff))
        return int_val

class pp_ac_int:
    def __init__(self, val):
        self.val = val
        t_name = real_type(val).__str__()
        m1 = re.match( r'ac_int<([0-9]*),\s*(.*)>$', t_name )
        self.w = int(m1.group(1))

    def to_long(self):
        return pp_iv(self.val).to_long() 

    def to_string_base(self,radix):
        if radix == "dec":
            return str(self.to_long())
        elif radix == "hex":
            hex_len = (self.w + 3)//4 
            hex_str = "{0:=#0{width}x}".format(self.to_long(), width=hex_len+2)
            return hex_str 
        elif radix == "bin":
            return str(bin(self.to_long())) 

    def to_string(self):
        radix = gdb.parameter('pp-ac_int-radix')
        if radix == "default":
           radix = gdb.parameter('pp-ac-radix')
        return self.to_string_base(radix)

class pp_ac_fixed:
    def __init__(self, val):
        self.val = val
        t_name = real_type(val).__str__()
        # when the second argument (I) is negative, for some versions, it is written out in hex!
        m1 = re.match( r'^ac_fixed<([0-9]*),\s*([-]?[x0-9a-fA-F]*),\s*(.*),\s*(.*),\s*(.*)>$', t_name )
        self.w = int(m1.group(1))
        self.i = int(m1.group(2),0)

    def to_decimal(self):
        base_iv = pp_iv(self.val)
        return decimal.Decimal(base_iv.to_long()) * (decimal.Decimal(2) ** (self.i - self.w)) 

    def to_string_base(self,radix):
        if radix == "dec":
            return str(self.to_decimal())
        else:
            if radix == "hex":
               bits = 4
               base_letter = 'x'
            elif radix == "bin":
               bits = 1
               base_letter = 'b'

            lsb = self.i - self.w 
            msb = self.i - 1
            base_lsb = lsb // bits
            base_msb = msb // bits
            if base_lsb > 0:
               base_lsb = 0
            if base_msb < 0:
               base_msb = 0
            base_len = base_msb - base_lsb + 1
            extra_lsb_bits = lsb - bits * base_lsb 
            int_val = pp_iv(self.val).to_long() * (2 ** extra_lsb_bits)
            s = "{0:=#0{width}{base_letter}}".format(int_val, width=base_len+2, base_letter=base_letter)
            if lsb < 0:
               s = s[:base_lsb] + '.' + s[base_lsb:]
            return s

    def to_string(self):
        radix = gdb.parameter('pp-ac_fixed-radix')
        if radix == "default":
           radix = gdb.parameter('pp-ac-radix')
        return self.to_string_base(radix)

class pp_ac_float:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        m = self.val["m"]
        e = self.val["e"]
        m0 = pp_ac_fixed(m)
        e0 = pp_ac_int(e)
        flt = m0.to_decimal() * (decimal.Decimal(2) ** e0.to_long()) 
        return str(flt) + " = " + m0.to_string() + "*2^" + e0.to_string() 

class pp_ac_complex:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        r = self.val["_r"]
        i = self.val["_i"]
        return r.to_string() + " + j " + i.to_string()

def ac_lookup_function (val):
    "Look-up and return a pretty-printer that can print val."

    # Get the real type.
    type = real_type(val)

    # Get the type name.    
    typename = type.tag

    if typename == None:
        return None

    # Iterate over local dictionary of types to determine
    # if a printer is registered for that type.  Return an
    # instantiation of the printer if found.
    for function in ac_pretty_printers_dict:
        if function.match (typename):
            return ac_pretty_printers_dict[function] (val)
        
    # Cannot find a pretty printer.  Return None.

    return None

def register_ac_pretty_printers ():
    # Note that we purposely omit the typedef names here.
    # Printer lookup is based on canonical name.
    # However, we do need both tagged and untagged variants, to handle
    # both the C and C++ cases.
    ac_pretty_printers_dict[re.compile ('^class ac_int<.*>$')]   = pp_ac_int
    ac_pretty_printers_dict[re.compile ('^ac_int<.*>$')]   = pp_ac_int

    ac_pretty_printers_dict[re.compile ('^class ac_fixed<.*>$')]   = pp_ac_fixed
    ac_pretty_printers_dict[re.compile ('^ac_fixed<.*>$')]   = pp_ac_fixed

    ac_pretty_printers_dict[re.compile ('^class ac_float<.*>$')]   = pp_ac_float
    ac_pretty_printers_dict[re.compile ('^ac_float<.*>$')]   = pp_ac_float

    ac_pretty_printers_dict[re.compile ('^class ac_complex<.*>$')]   = pp_ac_complex
    ac_pretty_printers_dict[re.compile ('^ac_complex<.*>$')]   = pp_ac_complex

class PP_Param_ac_Radix (gdb.Parameter):
    set_doc = "Radix setting for Pretty Print for Algorithmic C Datatypes"
    show_doc = "Radix setting for Pretty Print for Algorithmic C Datatypes"
    def __init__(self):
        super (PP_Param_ac_Radix, self).__init__("pp-ac-radix", gdb.COMMAND_DATA, gdb.PARAM_ENUM, ["dec","hex","bin"]) 
        self.value = "dec" 
    
    def get_set_string(self):
        return self.value

    def get_show_string(self,svalue):
        svalue = self.value
        return svalue 

class PP_Param_ac_int_Radix (gdb.Parameter):
    set_doc = "Radix setting for Pretty Print for ac_int"
    show_doc = "Radix setting for Pretty Print for ac_int"
    def __init__(self):
        super (PP_Param_ac_int_Radix, self).__init__("pp-ac_int-radix", gdb.COMMAND_DATA, gdb.PARAM_ENUM, ["default","dec","hex","bin"]) 
        self.value = "default" 
    
    def get_set_string(self):
        return self.value

    def get_show_string(self,svalue):
        svalue = self.value
        return svalue 

class PP_Param_ac_fixed_Radix (gdb.Parameter):
    set_doc = "Radix setting for Pretty Print for ac_fixed"
    show_doc = "Radix setting for Pretty Print for ac_fixed"
    def __init__(self):
        super (PP_Param_ac_fixed_Radix, self).__init__("pp-ac_fixed-radix", gdb.COMMAND_DATA, gdb.PARAM_ENUM, ["default","dec","hex","bin"]) 
        self.value = "default" 
    
    def get_set_string(self):
        return self.value

    def get_show_string(self,svalue):
        svalue = self.value
        return svalue 

if __name__ == "__main__":
    ac_pretty_printers_dict = {}
    register_ac_pretty_printers ()
    gdb.pretty_printers.append (ac_lookup_function)
    PP_Param_ac_Radix()
    PP_Param_ac_int_Radix()
    PP_Param_ac_fixed_Radix()
