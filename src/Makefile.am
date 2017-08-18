#this file is src/Makefile.am
METASOURCES = AUTO

AM_CPPFLAGS = $(all_includes) -I$(BOOST_DIR)
AM_LDFLAGS += $(BOOST_LDFLAGS)

# main list of binary produced
bin_PROGRAMS =
noinst_LTLIBRARIES =

noinst_HEADERS = Parameter.hpp constant_strings.hpp global_variables.hpp

Parameter.cpp : branch_name.hpp revision_hash.hpp

if INSIDE_GIT
branch_name.hpp : $(GIT_WORKTREE_PATH)/HEAD $(GIT_WORKTREE_PATH)/index
	@echo "\"`git --git-dir $(GIT_WORKTREE_PATH) rev-parse --abbrev-ref HEAD`\"" >$@

revision_hash.hpp : $(GIT_WORKTREE_PATH)/HEAD $(GIT_WORKTREE_PATH)/index
	@echo "\"`git --git-dir $(GIT_WORKTREE_PATH) rev-parse HEAD`\"" > $@

else

branch_name.hpp :
	echo "" > $@

revision_hash.hpp :
	cat $(top_srcdir)/dist_revision_hash > $@

endif

SUBDIRS = constants
DIST_SUBDIRS = constants

PRJ_DOC = src.doc

if BUILD_LIB_ALGORITHMS
  SUBDIRS += algorithms
  DIST_SUBDIRS += algorithms
endif

if BUILD_LIB_BDD
  SUBDIRS += bdd
  DIST_SUBDIRS += bdd
endif

if BUILD_LIB_BEHAVIOR
  SUBDIRS += behavior
  DIST_SUBDIRS += behavior
endif

if BUILD_LIB_CIRCUIT
  SUBDIRS += circuit
  DIST_SUBDIRS += circuit
endif

if BUILD_LIB_DESIGN_ANALYSIS
  SUBDIRS += design_analysis
  DIST_SUBDIRS += design_analysis
endif

if BUILD_LIB_DESIGN_FLOWS
  SUBDIRS += design_flows
  DIST_SUBDIRS += design_flows
endif

if BUILD_LIB_FRONTEND_ANALYSIS
  SUBDIRS += frontend_analysis
  DIST_SUBDIRS += frontend_analysis
endif

if BUILD_LIB_GRAPH
  SUBDIRS += graph
  DIST_SUBDIRS += graph
  AM_CPPFLAGS += -I$(top_srcdir)/src/graph
endif

if BUILD_LIB_HLS
  SUBDIRS += HLS
  DIST_SUBDIRS += HLS
endif

if BUILD_LIB_INTERMEDIATE_REPRESENTATIONS
  SUBDIRS += intermediate_representations
  DIST_SUBDIRS += intermediate_representations
endif

if BUILD_LIB_FROM_IPXACT
  SUBDIRS += ip_xact
  DIST_SUBDIRS += ip_xact
endif

if BUILD_LIB_ILP
  SUBDIRS += ilp
  DIST_SUBDIRS += ilp
endif

if BUILD_LIB_LOGIC_SYNTHESIS
  SUBDIRS += logic_synthesis
  DIST_SUBDIRS += logic_synthesis
endif

if BUILD_LIB_ARCH
  SUBDIRS += machine
  DIST_SUBDIRS += machine
endif

if BUILD_LIB_PARSER
  SUBDIRS += parser
  DIST_SUBDIRS += parser
if BUILD_LIB_FROM_C
  AM_CPPFLAGS += \
     -I$(top_srcdir)/src/parser/treegcc
endif
endif

if BUILD_LIB_POLIXML
  SUBDIRS += polixml
  DIST_SUBDIRS += polixml
  AM_CPPFLAGS += -I$(top_srcdir)/src/polixml \
                 -I$(top_srcdir)/src/parser/polixml
endif

if BUILD_LIB_PRAGMA
  SUBDIRS += pragma
  DIST_SUBDIRS += pragma
endif

if BUILD_LIB_TREE
  SUBDIRS += tree
  DIST_SUBDIRS += tree
endif

if BUILD_LIB_WRAPPER
  SUBDIRS += wrapper
  DIST_SUBDIRS += wrapper
endif

if BUILD_LIB_2LEV
  SUBDIRS += logic_synthesis
  DIST_SUBDIRS += logic_synthesis
endif

if BUILD_LIB_RTL
  SUBDIRS += rtl
  DIST_SUBDIRS += rtl
endif

if BUILD_LIB_TECHNOLOGY
  SUBDIRS += technology
  DIST_SUBDIRS += technology
endif

if BUILD_LIB_UTILITY
  SUBDIRS += utility
  DIST_SUBDIRS += utility
  AM_CPPFLAGS += -I$(top_srcdir)/src/utility
endif

################################### Mongoose ###################################
if BUILD_MONGOOSE
include $(top_srcdir)/src/mongoose.am
endif
#################################### Spider ####################################
if BUILD_SPIDER
include $(top_srcdir)/src/spider.am
endif

############################### tree-panda-gcc #################################
if BUILD_TREE_PANDA_GCC
include $(top_srcdir)/src/tree-panda-gcc.am
endif

#################################### Zebu ######################################
if BUILD_ZEBU
include $(top_srcdir)/src/zebu.am
endif

########################### EUCALYPTUS SECTION ###########################
if BUILD_EUCALYPTUS
include $(top_srcdir)/src/eucalyptus.am
endif

########################### BAMBU SECTION ###########################
if BUILD_BAMBU
include $(top_srcdir)/src/bambu.am
endif

########################### GECCO SECTION ###########################
if BUILD_GECCO
include $(top_srcdir)/src/gecco.am
endif

########################### TUCANO SECTION ###########################	
if BUILD_TUCANO
include $(top_srcdir)/src/tucano.am
endif

########################### KOALA SECTION ###########################
if BUILD_KOALA
include $(top_srcdir)/src/koala.am
endif

########################### GRIZZLY SECTION ###########################	
if BUILD_GRIZZLY
include $(top_srcdir)/src/grizzly.am
endif

########################### TURTLE SECTION ###########################
if BUILD_SIMIT
include $(top_srcdir)/src/turtle.am
endif

########################### LEMUR SECTION ###########################
if BUILD_LEMUR
include $(top_srcdir)/src/lemur.am
endif

########################### DOC SECTION ###########################	
DOX_TAGFILES = src/tree src/graph src/behavior src/technology src/utility src/parser src/edif src/algorithms src/circuit src/ilp src/regularity src/backend src/codesign src/HLS src/HLS/module_allocation src/HLS/binding src/HLS/controller src/HLS/datapath src/HLS/estimation src/HLS/hls_flow src/HLS/target src/HLS/scheduling

EXTRA_DIST = $(PRJ_DOC) bambu.am

#do not touch the following line
include $(top_srcdir)/panda.am
