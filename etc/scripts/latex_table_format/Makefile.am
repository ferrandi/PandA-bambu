#this is file latex_table_format/Makefile.am
DOX_TAGFILES =

EXTRA_DIST = latex_format_bambu_results.xml latex_format_bambu_results_altera.xml latex_format_bambu_results_xilinx.xml latex_format_bambu_results_lattice.xml latex_format_stat.xml

latex_table_format_dir = $(pkgdatadir)/latex_table_format

latex_table_format__DATA = \
	latex_format_bambu_results.xml \
	latex_format_bambu_results_xilinx.xml \
	latex_format_bambu_results_altera.xml \
	latex_format_bambu_results_lattice.xml

if BUILD_LIB_EXPERIMENTAL
EXTRA_DIST += latex_format_af_edges.xml latex_format_pdg_reduction.xml latex_format_edges_reduction.xml
latex_table_format__DATA += latex_format_af_edges.xml latex_format_pdg_reduction.xml latex_format_edges_reduction.xml
endif

#do not touch the following line
