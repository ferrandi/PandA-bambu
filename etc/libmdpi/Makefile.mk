#    Copyright (C) 2023-2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#    This file is part of the PandA/Bambu MDPI Library.
#
#    author Michele Fiorito <michele.fiorito@polimi.it>
#
# Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

default: all

check_defined = $(strip $(foreach 1,$1, $(call __check_defined,$1,$(strip $(value 2)))))
__check_defined = $(if $(value $1),, $(error Undefined $1$(if $2, ($2))))

libmdpi_root := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
panda_includes := $(libmdpi_root)/../../../include/panda

$(call check_defined, CC)         # PandA Bambu HLS frontend compiler
$(call check_defined, TOP_FNAME)  # top function mangled name
$(call check_defined, MTOP_FNAME) # top function mangled name with __m_ prefix
$(call check_defined, SIM_DIR)    # HLS_output/simulation absolute path
$(call check_defined, BEH_DIR)    # HLS_output/<sim_id>_beh absolute path

###
# The following variables may also be defined:
#   SRCS       : PandA Bambu HLS input source files
#   BEH_CC     : alternative compiler for libmdpi compilation
#   CFLAGS     : compilation flags for SRCS, WRAPPER_SRC, and TB_SRCS
#   BEH_CFLAGS : includes and defines for svdpi.h
#   TB_CFLAGS  : additional compilation flags for TB_SRCS
#   TB_SRCS    : additional testbench source files
#   PP_SRC     : pretty print source file
###

### Environment below is automatically populated, do not override

V ?= 0
ifeq ($(V), 0)
   Q = @
else
   Q =
endif

override SRCS := $(abspath $(filter-out %.bambuir, $(SRCS)))
override TB_SRCS := $(abspath $(TB_SRCS))
SRC_DIR := $(shell echo "$(SRCS)" | sed 's/ /\n/g' | sed -e '$$!{N;s/^\(.*\).*\n\1.*$$/\1\n\1/;D;}' | sed 's/\(.*\)\/.*/\1/')
TB_SRC_DIR := $(shell echo "$(TB_SRCS)" | sed 's/ /\n/g' | sed -e '$$!{N;s/^\(.*\).*\n\1.*$$/\1\n\1/;D;}' | sed 's/\(.*\)\/.*/\1/')
BUILD_DIR := $(SIM_DIR)/build
OBJ_DIR := $(BUILD_DIR)/obj
TB_OBJ_DIR := $(BUILD_DIR)/tb
MDPI_OBJ_DIR := $(BUILD_DIR)/mdpi

DRIVER_SRC := $(libmdpi_root)/mdpi_driver.cpp
MDPI_SRCS := $(libmdpi_root)/mdpi.c
WRAPPER_MDPI_SRC := $(SIM_DIR)/mdpi_wrapper.cpp

OBJS := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%.o, $(SRCS))
TB_OBJS := $(patsubst $(TB_SRC_DIR)/%,$(TB_OBJ_DIR)/%.o, $(TB_SRCS))
DRIVER_OBJ := $(patsubst %,$(MDPI_OBJ_DIR)/%.o, $(notdir $(DRIVER_SRC)))
WRAPPER_MDPI_OBJ := $(patsubst %,$(MDPI_OBJ_DIR)/%.o, $(notdir $(WRAPPER_MDPI_SRC)))
WRAPPER_CSROA_OBJ := $(patsubst %,$(MDPI_OBJ_DIR)/%.o, $(notdir $(WRAPPER_CSROA_SRC)))
WRAPPER_OBJ := $(WRAPPER_MDPI_OBJ) $(WRAPPER_CSROA_OBJ)

override TB_CFLAGS := $(patsubst -fno-exceptions,,$(CFLAGS)) $(TB_CFLAGS)
MDPI_CFLAGS := $(BEH_CFLAGS) -D_GNU_SOURCE -isystem$(panda_includes)
LIB_CFLAGS := $(MDPI_CFLAGS)
ifdef BEH_CC
	LIB_CFLAGS += $(shell if basename $(BEH_CC) | grep -q '++'; then echo -std=c++11; else echo -std=c11; fi)
endif
DRIVER_CFLAGS := $(shell echo "$(TB_CFLAGS)" | grep -oE '(-mx?[0-9]+)')
DRIVER_CFLAGS += $(shell echo "$(TB_CFLAGS)" | grep -oE '( (-I|-isystem) ?[^ ]+)' | tr '\n' ' ')
DRIVER_CFLAGS += $(shell echo "$(TB_CFLAGS)" | grep -oE '( -D(\\.|[^ ])+)' | tr '\n' ' ')
DRIVER_CFLAGS += $(MDPI_CFLAGS) -std=c++11 -fno-exceptions

WRAPPER_CFLAGS := $(MDPI_CFLAGS) -std=c++11 -fno-exceptions $(shell echo "$(CFLAGS)" | sed -E 's/(-{1,2}std=(c|gnu)([0-9]+|\+\+(0|9)([0-9]|x)))//g') -DLIBMDPI_DRIVER
ifdef PP_SRC
	ifneq ($(TOP_FNAME),main)
		ifndef MPPTOP_FNAME
			$(error Undefined MPPTOP_FNAME)
		endif
		WRAPPER_CFLAGS += -DPP_VERIFICATION
		PP_OBJ := $(patsubst %,$(OBJ_DIR)/%.pp.o,$(notdir $(PP_SRC)))
	endif
endif

PP_CFLAGS := -fno-strict-aliasing $(shell if basename "$(CC)" | grep -q clang; then echo -Wno-error=int-conversion; fi)
PP_CFLAGS += $(shell if [ ! -z "$$(basename $(CC) | grep clang)" ]; then echo "-fbracket-depth=1024"; fi)

DRIVER_LDFLAGS := $(shell echo "$(TB_CFLAGS)" | grep -oE '(-mx?[0-9]+)')
DRIVER_LDFLAGS += $(shell echo " $(TB_CFLAGS)" | grep -oE '( -(L|l|Wl)(\\.|[^ ])+)' | tr '\n' ' ')
DRIVER_LDFLAGS += $(shell echo :$$LD_LIBRARY_PATH | sed 's/:$$//' | sed 's/:/ -L/g')
DRIVER_LDFLAGS += $(shell if basename $(CC) | grep -v '++' | grep -q 'clang'; then echo --driver-mode=g++; fi)
DRIVER_LDFLAGS += $(shell if basename $(CC) | grep -v '++' | grep -qv 'clang'; then echo -lstdc++; fi)
DRIVER_LDFLAGS += -lpthread -lm -ldl

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

DRIVER_DYN_LIB := $(SIM_DIR)/libmdpi_driver.so
DRIVER_STATIC_LIB := $(SIM_DIR)/libmdpi_driver.a
MDPI_LIB := $(BEH_DIR)/libmdpi.so
TB_TARGET := $(SIM_DIR)/testbench

.PHONY: all libmdpi dyn_driver static_driver testbench clean

all: dyn_driver

libmdpi: $(MDPI_LIB)

dyn_driver: $(DRIVER_DYN_LIB) libmdpi

static_driver: $(DRIVER_STATIC_LIB) libmdpi

testbench: $(TB_TARGET) libmdpi

clean:
	@rm -rf $(BUILD_DIR) $(DRIVER_DYN_LIB) $(DRIVER_STATIC_LIB) $(MDPI_LIB)

$(MDPI_LIB): $(MDPI_SRCS)
ifdef BEH_CC
	@echo "Compiling $(notdir $@)"
	$(Q)$(BEH_CC) $(LIB_CFLAGS) -o $@ $^ $(LIB_LDFLAGS)
endif

$(OBJ_DIR)/%.o: $(SRC_DIR)/%
	@echo "Compiling $(notdir $<)"
	$(Q)mkdir -p $$(dirname $@)
	$(Q)$(CC) $(if $(filter-out .F% .f%,$(suffix $(<F))),-MMD -MP -MT $(patsubst %.o,%.d, $@),) $(CFLAGS) -fPIC -c -o $@ $<
	$(Q)objcopy $(REDEFINE_TOP) $@

$(TB_OBJ_DIR)/%.o: $(TB_SRC_DIR)/%
	@echo "Compiling testbench $(notdir $<)"
	$(Q)mkdir -p $$(dirname $@)
	$(Q)$(CC) $(if $(filter-out .F% .f%,$(suffix $(<F))),-MMD -MP -MT $(patsubst %.o,%.d, $@),) $(TB_CFLAGS) -fPIC -c -o $@ $<
	$(Q)objcopy $(WEAKEN_TOP) $(REDEFINE_SYS) $@

$(PP_OBJ): $(PP_SRC)
	@echo "Compiling $(notdir $<)"
	$(Q)mkdir -p $$(dirname $@)
	$(Q)$(CC) $(CFLAGS) $(PP_CFLAGS) -fPIC -c -o $@ $<
	$(Q)objcopy --keep-global-symbol $(TOP_FNAME) $$(nm $@ | grep -o '[^[:space:]]*get_pc_thunk[^[:space:]]*' | sed 's/^/--keep-global-symbol /' | tr '\n' ' ') $@
	$(Q)objcopy --redefine-sym $(TOP_FNAME)=$(MPPTOP_FNAME) $@

$(WRAPPER_MDPI_OBJ): $(WRAPPER_MDPI_SRC)
	@echo "Compiling $(notdir $<)"
	$(Q)mkdir -p $$(dirname $@)
	$(Q)$(CC) $(WRAPPER_CFLAGS) -fPIC -c -o $@ $<

$(WRAPPER_CSROA_OBJ): $(WRAPPER_CSROA_SRC)
	@echo "Compiling $(notdir $<)"
	$(Q)mkdir -p $$(dirname $@)
	$(Q)$(CC) $(WRAPPER_CFLAGS) -fPIC -c -o $@ $<

$(DRIVER_OBJ): $(DRIVER_SRC)
	@echo "Compiling $(notdir $<)"
	$(Q)mkdir -p $$(dirname $@)
	$(Q)$(CC) $(DRIVER_CFLAGS) -fPIC -c -o $@ $<

$(DRIVER_DYN_LIB): $(OBJS) $(DRIVER_OBJ) $(WRAPPER_OBJ) $(PP_OBJ)
	@echo "Linking $(notdir $@)"
	$(Q)$(CC) -shared -o $@ $^ $(DRIVER_LDFLAGS)
	$(Q)objcopy $(REDEFINE_SYS) $@

$(DRIVER_STATIC_LIB): $(OBJS) $(DRIVER_OBJ) $(WRAPPER_OBJ) $(PP_OBJ)
	@echo "Packing $(notdir $@)"
	$(Q)ar rcs $@ $^
	$(Q)objcopy $(REDEFINE_SYS) $@

$(TB_TARGET): $(TB_OBJS) $(OBJS) $(DRIVER_OBJ) $(WRAPPER_OBJ) $(PP_OBJ)
	@echo "Linking $(notdir $@)"
	$(Q)$(CC) -o $@ $^ $(DRIVER_LDFLAGS)

-include $(patsubst %.o,%.d, $(filter-out %.ll, $(OBJS) $(TB_OBJS)))
