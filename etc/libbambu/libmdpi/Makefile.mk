# 
# 
#                    _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
#                   _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
#                  _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
#                 _/      _/    _/ _/    _/ _/   _/ _/    _/
#                _/      _/    _/ _/    _/ _/_/_/  _/    _/
# 
#              ***********************************************
#                               PandA Project
#                      URL: http://panda.dei.polimi.it
#                        Politecnico di Milano - DEIB
#                         System Architectures Group
#              ***********************************************
#               Copyright (C) 2004-2023 Politecnico di Milano
# 
#    This file is part of the PandA framework.
# 
#    The PandA framework is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 3 of the License, or
#    (at your option) any later version.
# 
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
# 
#####################################################################
# @file Makefile
#
# @author Michele Fiorito <michele.fiorito@polimi.it>
# $Revision$
# $Date$
# Last modified by $Author$
#
#####################################################################

default: all

check_defined = \
    $(strip $(foreach 1,$1, \
        $(call __check_defined,$1,$(strip $(value 2)))))
__check_defined = \
    $(if $(value $1),, \
      $(error Undefined $1$(if $2, ($2))))

libmdpi_root := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

$(call check_defined, CC)         # PandA Bambu HLS frontend compiler
$(call check_defined, TOP_FNAME)  # top function mangled name
$(call check_defined, MTOP_FNAME) # top function mangled name with __m_ prefix
$(call check_defined, SIM_DIR)    # HLS_output/simulation absolute path
$(call check_defined, BEH_DIR)    # HLS_output/<sim_id>_beh absolute path
$(call check_defined, SRCS)       # PandA Bambu HLS input source files
$(call check_defined, COSIM_SRCS) # PandA Bambu HLS generated co-simulation source files

###
# The following variables may also be defined:
#   BEH_CC     : alternative compiler for libmdpi compilation
#   CFLAGS     : compilation flags for SRCS, COSIM_SRCS, and TB_SRCS
#   BEH_CFLAGS : includes and defines for svdpi.h
#   TB_CFLAGS  : additional compilation flags for TB_SRCS
#   TB_SRCS    : additional testbench source files
#   PP_SRCS    : pretty print source file
###

### Environment below is automatically populated, do not override

IPC_FILENAME := $(shell echo "$(BEH_CFLAGS)" | sed -E 's/.*__M_IPC_FILENAME=?\"([^ ]+)?\".*/\1/')
OBJ_DIR := $(SIM_DIR)/obj

DRIVER_SRCS := $(libmdpi_root)/mdpi_driver.cpp
MDPI_SRCS := $(libmdpi_root)/mdpi.c

OBJS := $(patsubst %.ll,$(OBJ_DIR)/%.o,$(patsubst %.c,$(OBJ_DIR)/%.o,$(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(SRCS)))))
DRIVER_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(DRIVER_SRCS))))
COSIM_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(COSIM_SRCS))))
TB_OBJS := $(patsubst %.ll,$(OBJ_DIR)/%.tb.o,$(patsubst %.c,$(OBJ_DIR)/%.tb.o,$(patsubst %.cpp,$(OBJ_DIR)/%.tb.o,$(notdir $(TB_SRCS)))))

override TB_CFLAGS := $(patsubst -fno-exceptions,,$(CFLAGS)) $(TB_CFLAGS) -I$(libmdpi_root)/include
MDPI_CFLAGS := $(BEH_CFLAGS) # $(shell echo "$(CFLAGS)" | grep -oE '(-mx?[0-9]+)' | sed -E 's/-mx?/-DM/' | tr '[:lower:]' '[:upper:]')
LIB_CFLAGS := $(MDPI_CFLAGS) -std=c11
DRIVER_CFLAGS := $(shell echo "$(TB_CFLAGS)" | grep -oE '(-mx?[0-9]+)')
DRIVER_CFLAGS += $(shell echo "$(TB_CFLAGS)" | grep -oE '( (-I|-isystem) ?[^ ]+)' | tr '\n' ' ')
DRIVER_CFLAGS += $(shell echo "$(TB_CFLAGS)" | grep -oE '( -D(\\.|[^ ])+)' | tr '\n' ' ')
DRIVER_CFLAGS += $(MDPI_CFLAGS) -std=c++11 -DMDPI_PARALLEL_VERIFICATION

COSIM_CFLAGS := -DLIBMDPI_DRIVER
ifdef PP_SRCS
	ifneq ($(TOP_FNAME),main)
		$(call check_defined, MPPTOP_FNAME)
  		COSIM_CFLAGS += -DPP_VERIFICATION
		PP_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(PP_SRCS)))
	endif
endif

PP_CFLAGS := -fno-strict-aliasing -Wno-error=int-conversion
PP_CFLAGS += $(shell if [ ! -z "$$(basename $(CC) | grep clang)" ]; then echo "-fbracket-depth=1024"; fi)

DRIVER_LDFLAGS := $(shell echo "$(TB_CFLAGS)" | grep -oE '(-mx?[0-9]+)')
DRIVER_LDFLAGS += $(shell echo "$(TB_CFLAGS)" | grep -oE '( -[Ll](\\.|[^ ])+)' | tr '\n' ' ')
DRIVER_LDFLAGS += $(shell echo :$$LD_LIBRARY_PATH | sed 's/:$$//' | sed 's/:/ -L/g')
DRIVER_LDFLAGS += -lpthread -lstdc++

LIB_LDFLAGS := 
ifeq ($(BEH_CC),xsc)
	LIB_CFLAGS := $(addprefix -gcc_compile_options=, $(LIB_CFLAGS))
	LIB_LDFLAGS := -work $(shell realpath --relative-to $(libmdpi_root) $(BEH_DIR)) $(addprefix -gcc_link_options=, $(LIB_LDFLAGS))
else
	LIB_LDFLAGS += -shared -fPIC -Bsymbolic -Wl,-z,defs
endif

REDEFINE_SYS := --redefine-sym exit=__m_exit --redefine-sym abort=__m_abort --redefine-sym __assert_fail=__m_assert_fail
REDEFINE_TOP := --weaken --redefine-sym $(TOP_FNAME)=$(MTOP_FNAME)
WEAKEN_TOP := -W $(TOP_FNAME)

DRIVER_LIB := $(SIM_DIR)/libmdpi_driver.so
MDPI_LIB := $(BEH_DIR)/libmdpi.so
TB_TARGET := $(SIM_DIR)/testbench

$(shell mkdir -p $(OBJ_DIR))

.PHONY: all libs libmdpi libmdpi_driver testbench clean

all: libs testbench

libs: libmdpi libmdpi_driver

libmdpi: $(MDPI_LIB)

libmdpi_driver: $(DRIVER_LIB)

testbench: $(TB_TARGET)
	@rm -rf $(IPC_FILENAME)

clean:
	@rm -rf $(OBJ_DIR) $(DRIVER_LIB) $(MDPI_LIB) $(IPC_FILENAME)

$(MDPI_LIB): $(MDPI_SRCS)
ifdef BEH_CC
	@echo "Compiling $(notdir $@)"
	@$(BEH_CC) $(LIB_LDFLAGS) $(LIB_CFLAGS) -o $@ $^
endif

.SECONDEXPANSION:
$(OBJS): %.o: $$(SRCS)
	@echo "Compiling $(notdir $<)"
	@$(CC) $(CFLAGS) -fPIC -c -o $@ $<
	@objcopy $(REDEFINE_TOP) $@

.SECONDEXPANSION:
$(COSIM_OBJS): %.o: $$(COSIM_SRCS)
	@echo "Compiling $(notdir $<)"
	@$(CC) $(CFLAGS) $(MDPI_CFLAGS) $(COSIM_CFLAGS) -fPIC -c -o $@ $<

.SECONDEXPANSION:
$(PP_OBJS): %.o: $$(PP_SRCS)
	@echo "Compiling $(notdir $<)"
	@$(CC) $(CFLAGS) $(PP_CFLAGS) -fPIC -c -o $@ $<
	@objcopy --keep-global-symbol $(TOP_FNAME) $$(nm $@ | grep -o '[^[:space:]]*get_pc_thunk[^[:space:]]*' | sed 's/^/--keep-global-symbol /' | tr '\\n' ' ') $@
	@objcopy --redefine-sym $(TOP_FNAME)=$(MPPTOP_FNAME) $@

.SECONDEXPANSION:
$(DRIVER_OBJS): %.o: $$(DRIVER_SRCS)
	@echo "Compiling $(notdir $<)"
	@$(CC) $(DRIVER_CFLAGS) -fPIC -c -o $@ $<

$(DRIVER_LIB): $(OBJS) $(DRIVER_OBJS) $(COSIM_OBJS) $(PP_OBJS)
	@echo "Linking $(notdir $@)"
	@$(CC) $(DRIVER_LDFLAGS) -shared -o $@ $^
	@objcopy $(REDEFINE_SYS) $@

.SECONDEXPANSION:
$(TB_OBJS): %.o: $$(TB_SRCS)
	@echo "Compiling testbench $(notdir $<)"
	@$(CC) $(TB_CFLAGS) -c -o $@ $<

$(TB_TARGET): $(TB_OBJS) $(DRIVER_LIB)
ifdef TB_OBJS
	@echo "Linking $(notdir $@)"
	@$(CC) $(TB_CFLAGS) -o $@ $^
endif
