#this file is src/constants/Makefile.am
METASOURCES = AUTO

SUBDIRS=

noinst_HEADERS= file_IO_constants.hpp treegcc_xml.hpp constants.hpp copyrights_strings.hpp

if BUILD_BAMBU
   noinst_HEADERS += bambu_results_xml.hpp testbench_generation_constants.hpp
endif

if BUILD_LIB_FROM_AADL_ASN
   noinst_HEADERS += aadl_constants.hpp
endif

if BUILD_LIB_FROM_C
   noinst_HEADERS += treegcc_constants.hpp
endif

if BUILD_LIB_FROM_DATA_XML
   noinst_HEADERS += data_xml.hpp
endif

if BUILD_LIB_FROM_PRAGMA
   noinst_HEADERS += pragma_constants.hpp
endif

if BUILD_LIB_HLS
   noinst_HEADERS += allocation_constants.hpp constraints_xml.hpp
endif

if BUILD_LIB_HOST_PROFILING
   noinst_HEADERS += host_profiling_constants.hpp host_profiling_xml.hpp
endif

if BUILD_LIB_TECHNOLOGY
   noinst_HEADERS += technology_xml.hpp
endif

if BUILD_LIB_TO_DATAFILE
   noinst_HEADERS += latex_table_constants.hpp latex_table_xml.hpp
endif

if BUILD_SPIDER
   noinst_HEADERS += experimental_setup_xml.hpp
endif


if BUILD_SYNTHESIS
   noinst_HEADERS += synthesis_constants.hpp
endif

if BUILD_TASTE
   noinst_HEADERS += taste_constants.hpp
endif

if USE_WEIGHT_MODELS_XML
   noinst_HEADERS += weights_xml.hpp
endif

#do not touch the following line

