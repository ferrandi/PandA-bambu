#this is file latex_table_format/Makefile.am
DOX_TAGFILES =

EXTRA_DIST = latex_format_bambu_results.xml latex_format_bambu_results_altera.xml  latex_format_bambu_results_xilinx.xml latex_format_bambu_results_lattice.xml latex_format_stat.xml

if BUILD_LIB_EXPERIMENTAL
EXTRA_DIST += latex_format_af_edges.xml  latex_format_pdg_reduction.xml latex_format_edges_reduction.xml
endif

install-exec-hook:
	cp $(top_srcdir)/latex_table_format/latex_format_bambu_results.xml $(DESTDIR)$(libdir)
	cp $(top_srcdir)/latex_table_format/latex_format_bambu_results_xilinx.xml $(DESTDIR)$(libdir)
	cp $(top_srcdir)/latex_table_format/latex_format_bambu_results_altera.xml $(DESTDIR)$(libdir)
	cp $(top_srcdir)/latex_table_format/latex_format_bambu_results_lattice.xml $(DESTDIR)$(libdir)

#do not touch the following line


