#this file is src/ilp/Makefile.am
noinst_LTLIBRARIES = lib_ilp.la

lib_ilp_la_CPPFLAGS = -I$(top_srcdir)/src/utility -I$(BOOST_DIR) $(all_includes)

lib_ilp_cbc_la_CPPFLAGS=-I$(top_srcdir)/src/utility -I$(BOOST_DIR) $(all_includes)
if BUILD_COIN_OR
   noinst_LTLIBRARIES += lib_ilp_cbc.la 
   lib_ilp_cbc_la_CPPFLAGS += -DHAVE_CONFIG_H \
                              -I$(COIN_OR_DIR)/BuildTools/headers \
                              -I$(COIN_OR_DIR)/Cbc/src \
                              -I$(COIN_OR_DIR)/Cbc/src/OsiCbc/ \
                              -I$(COIN_OR_DIR)/Cgl/src \
                              -I$(COIN_OR_DIR)/Cgl/src/CglClique \
                              -I$(COIN_OR_DIR)/Cgl/src/CglFlowCover \
                              -I$(COIN_OR_DIR)/Cgl/src/CglGomory \
                              -I$(COIN_OR_DIR)/Cgl/src/CglKnapsackCover \
                              -I$(COIN_OR_DIR)/Cgl/src/CglMixedIntegerRounding \
                              -I$(COIN_OR_DIR)/Cgl/src/CglMixedIntegerRounding2 \
                              -I$(COIN_OR_DIR)/Cgl/src/CglOddHole \
                              -I$(COIN_OR_DIR)/Cgl/src/CglPreProcess \
                              -I$(COIN_OR_DIR)/Cgl/src/CglProbing \
                              -I$(COIN_OR_DIR)/Cgl/src/CglRedSplit \
                              -I$(COIN_OR_DIR)/Cgl/src/CglTwomir \
                              -I$(COIN_OR_DIR)/Cgl/src/CglLandP \
                              -I$(COIN_OR_DIR)/Cgl/src/CglResidualCapacity \
                              -I$(COIN_OR_DIR)/Clp/src/ \
                              -I$(COIN_OR_DIR)/Clp/src/OsiClp \
                              -I$(COIN_OR_DIR)/CoinUtils/src \
                              -I$(COIN_OR_DIR)/Osi/src/Osi \
                              -I$(top_builddir)/$(COIN_OR_OBJDIR)/Cbc/src \
                              -I$(top_builddir)/$(COIN_OR_OBJDIR)/Clp/src \
                              -I$(top_builddir)/$(COIN_OR_OBJDIR)/Cgl/src \
                              -I$(top_builddir)/$(COIN_OR_OBJDIR)/Osi/src \
                              -I$(top_builddir)/$(COIN_OR_OBJDIR)/CoinUtils/src \
                              -I$(top_srcdir)/src/HLS/scheduling/ilp_scheduling
endif

if BUILD_LP_SOLVE
   lib_ilp_la_CPPFLAGS += -I$(top_srcdir)/ext/lpsolve5 
endif

METASOURCES = AUTO

lib_ilp_la_SOURCES = objective_function.cpp problem_dim.cpp meilp_solver.cpp

noinst_HEADERS = objective_function.hpp \
        problem_dim.hpp \
        CbcBranchUser.hpp \
        meilp_solver.hpp 

if BUILD_GLPK
   lib_ilp_la_SOURCES += glpk_solver.cpp
   noinst_HEADERS += glpk_solver.hpp
endif
lib_ilp_cbc_la_SOURCES=

lib_ilp_la_LIBADD =
if BUILD_GLPK
  lib_ilp_la_LIBADD += -lglpk -lltdl -lamd -lcolamd -lgmp $(LIB_SUITESPARSECONFIG) 
endif

DOX_TAGFILES= 

PRJ_DOC = ilp.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

