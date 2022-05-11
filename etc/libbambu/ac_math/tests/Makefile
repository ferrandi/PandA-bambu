ifeq "$(AC_TYPES_INC)" ""
  $(error Error: The AC_TYPES_INC variable was not set. Cannot locate AC Datatypes installation)
endif

ifeq "$(TESTS_DIR)" ""
TESTS_DIR = .
endif

AC_MATH_INC = $(TESTS_DIR)/../include

ifeq "$(CXX)" ""
CXX = g++
endif

ifneq "$(CXXFLAGS)" ""
CXXUSERFLAGS = $(CXXFLAGS)
endif

CXXFLAGS = -O2 -std=c++14 -DAC_TYPES_INIT $(CXXUSERFLAGS)
LDFLAGS = -s -static-libstdc++
WD = $(shell pwd)
GCOV_ENABLED = false

ifneq "x$(GCOV_HOME)" "x"
GCOV_ENABLED = true
GCOV = $(GCOV_HOME)/gcov
LCOV = $(GCOV_HOME)/lcov
GENHTML = $(GCOV_HOME)/genhtml
CXXFLAGS += -coverage
LDFLAGS += -lgcov
GCOV_OPTS := --gcov-tool $(GCOV) --quiet --rc geninfo_auto_base=1 --rc lcov_branch_coverage=1 --external
GCCVER = $(shell $(CXX) -dumpversion)
GCOV_FILT_PAT := '/usr/include/*' '/usr/lib/*' '$(GCCVER)/*' 
endif

DIE        = || exit 1 ;
RM        := rm -f
ECHO      := echo

SOURCES_CPP = \
  rtest_ac_div.cpp \
  rtest_ac_atan_pwl.cpp \
  rtest_ac_tan_pwl.cpp \
  rtest_ac_sigmoid_pwl.cpp \
  rtest_ac_hyperbolic_tan_pwl.cpp \
  rtest_ac_cholinv.cpp \
  rtest_ac_exp_cordic.cpp \
  rtest_ac_exp2_cordic.cpp \
  rtest_ac_pow2_pwl.cpp \
  rtest_ac_exp_pwl.cpp \
  rtest_ac_determinant.cpp \
  rtest_ac_chol_d.cpp \
  rtest_ac_qrd.cpp \
  rtest_ac_abs.cpp \
  rtest_ac_arccos_cordic.cpp \
  rtest_ac_arcsin_cordic.cpp \
  rtest_ac_atan2_cordic.cpp \
  rtest_ac_exp_pwl.cpp \
  rtest_ac_inverse_sqrt_pwl.cpp \
  rtest_ac_log_cordic.cpp \
  rtest_ac_log2_cordic.cpp \
  rtest_ac_log2_pwl.cpp \
  rtest_ac_log_pwl.cpp \
  rtest_ac_matrix.cpp \
  rtest_ac_matrixmul.cpp \
  rtest_ac_normalize.cpp \
  rtest_ac_pow2_pwl.cpp \
  rtest_ac_pow_cordic.cpp \
  rtest_ac_reciprocal_pwl.cpp \
  rtest_ac_shift.cpp \
  rtest_ac_sincos_cordic.cpp \
  rtest_ac_sincos_lut.cpp \
  rtest_ac_sqrt.cpp \
  rtest_ac_sqrt_pwl.cpp

OBJS = $(SOURCES_CPP:.cpp=.o)

GCOVDAT = $(SOURCES_CPP:.cpp=.gcda) $(SOURCES_CPP:.cpp=.gcno) $(SOURCES_CPP:.cpp=.o.base.info) $(SOURCES_CPP:.cpp=.o.test.info) $(SOURCES_CPP:.cpp=.o.total.info) $(SOURCES_CPP:.cpp=.o.filt.info) 

# Compilation rule
%.o: $(TESTS_DIR)/%.cpp
ifeq "$(GCOV_ENABLED)" "true"
	-@$(RM) *.gcda *.gcno
endif
	-@$(ECHO) "------------------------------ Compile  $< ----------------------------------"
	@$(CXX) $(CXXFLAGS) -I$(AC_TYPES_INC) -I$(AC_MATH_INC) $< $(LDFLAGS) $(LINK_LIBNAMES) -o $@ $(DIE)
ifeq "$(GCOV_ENABLED)" "true"
	-@$(ECHO) "------------------------------ Prep     $< ----------------------------------"
	-@$(RM) *.info
	@-$(LCOV) $(GCOV_OPTS) --capture --initial --directory . --output-file $@.base.info
endif
	-@$(ECHO) "------------------------------ Running  $< ----------------------------------"
	@./$@ $(DIE)
ifeq "$(GCOV_ENABLED)" "true"
	-@$(ECHO) "------------------------------ Coverage $< ----------------------------------"
	@-$(LCOV) $(GCOV_OPTS) --capture --directory . --output-file $@.test.info
	@-$(LCOV) $(GCOV_OPTS) --add-tracefile $@.base.info --add-tracefile $@.test.info  --output-file $@.total.info
	@-$(LCOV) $(GCOV_OPTS) --remove $@.total.info $(GCOV_FILT_PAT) -o $@.filt.info
	@-$(GENHTML) --rc geninfo_auto_base=1 --rc lcov_branch_coverage=1 --ignore-errors source $@.filt.info --legend --output-directory=$@-test-lcov
	@-$(ECHO) "HTML coverage report written to: file://$(WD)/$@-test-lcov/index.html"
endif

all: $(OBJS)

.PHONY: clean
clean:
	@-$(RM) -r $(OBJS) $(GCOVDAT)

