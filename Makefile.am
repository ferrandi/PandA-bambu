SUBDIRS =
DIST_SUBDIRS =

if BUILD_EXT
   SUBDIRS += ext
   DIST_SUBDIRS += ext
endif

SUBDIRS += panda_regressions panda_unit src etc examples style
DIST_SUBDIRS += panda_regressions panda_unit src etc examples style documentation

#########################################################
# Documentation section and rules
#########################################################
DOX_PROJECT_NAME= $(strip $(if $(subst $(shell (cd $(top_builddir); pwd )),,$(PWD)), $(strip $(subst /,_, $(subst $(shell (cd $(top_builddir); pwd ))/,,$(PWD)))), PandA))
# Directories and files to be scanned for the generation of the documentation
DOX_SRCFILES ="$(top_srcdir)/documentation $(top_srcdir)/examples $(top_srcdir)/src"
# Directory where the Doxygen \include command looks for file to be included
DOX_EXAMPLE_PATH=$(top_srcdir)
# Directory where the documentation will be placed
DOX_OUTPUTDIR=$(top_builddir)/documentation/
# Place where images can be found
DOX_IMG=$(top_srcdir)/documentation/img
# Logo
DOX_PROJECT_LOGO=$(top_srcdir)/style/img/panda.png.in
# Directory where custom header and footer can be found.
# Used only for latex with the new documentation build system
DOX_HF_DIR=$(top_srcdir)/style

.PHONY: documentation

documentation: $(top_builddir)/documentation/$(DOX_PROJECT_NAME)_html/index.html

$(top_builddir)/documentation/$(DOX_PROJECT_NAME)_html/index.html: Makefile $(top_srcdir)/etc/Doxyfile
	DOX_PROJECT_NAME=$(DOX_PROJECT_NAME) \
	DOX_VERSION=$(PACKAGE_VERSION) \
	DOX_PROJECT_LOGO=$(DOX_PROJECT_LOGO) \
	DOX_OUTPUTDIR=$(DOX_OUTPUTDIR) \
	DOX_SRCFILES=$(DOX_SRCFILES) \
	DOX_EXAMPLE_PATH=$(DOX_EXAMPLE_PATH) \
	DOX_IMG=$(DOX_IMG) \
	DOX_HF_DIR=$(DOX_HF_DIR) \
	  $(DOXYGEN) $(top_srcdir)/etc/Doxyfile;

EXTRA_DIST = Makefile.init README.md

#Replace acinclude.m4
ACLOCAL_AMFLAGS = -I etc/macros


#do not touch the following line


dist-hook:
	rm $(distdir)/aclocal.m4
	rm $(distdir)/missing
	rm $(distdir)/configure
	rm $(distdir)/depcomp
	rm $(distdir)/etc/macros/ltversion.m4
	rm $(distdir)/ext/abseil/aclocal.m4
	rm $(distdir)/ext/abseil/configure
	rm $(distdir)/ext/flopoco/aclocal.m4
	rm $(distdir)/ext/flopoco/configure
	rm $(distdir)/ext/flopoco/m4/ltversion.m4
	rm $(distdir)/ext/libfplll-4.0.3/aclocal.m4
	rm $(distdir)/ext/libfplll-4.0.3/configure
	rm $(distdir)/ext/sollya/aclocal.m4
	rm $(distdir)/ext/sollya/configure
	rm -f $(distdir)/ext/sollya/m4/ltversion.m4
	rm $(distdir)/ltmain.sh
	rm `find $(distdir) -name Makefile.in`
	rm `find $(distdir) -name libtool.m4`
	rm `find $(distdir) -name lt~obsolete.m4`
	rm `find $(distdir) -name ltoptions.m4`
	rm `find $(distdir) -name ltsugar.m4`
	rm `find $(distdir) -name config.h.in`
	rm `find $(distdir) -name config.guess`
	rm `find $(distdir) -name ylwrap`
	rm `find $(distdir) -name compile`
	if test -f $(distdir)/test-driver; then rm $(distdir)/test-driver; fi
	cp $(srcdir)/ext/flopoco/Makefile.autotools $(distdir)/ext/flopoco/Makefile.autotools
	mkdir $(distdir)/ext/flopoco/autotools
	cp $(srcdir)/ext/flopoco/autotools/Makefile.am $(distdir)/ext/flopoco/autotools/Makefile.am
	cp $(srcdir)/ext/flopoco/autotools/configure.ac $(distdir)/ext/flopoco/autotools/configure.ac
	mkdir $(distdir)/ext/flopoco/autotools/src
	cp $(srcdir)/ext/flopoco/autotools/src/Makefile.am $(distdir)/ext/flopoco/autotools/src/Makefile.am
	cat $(builddir)/src/revision_hash.hpp > $(distdir)/dist_revision_info
	cat $(builddir)/src/branch_name.hpp >> $(distdir)/dist_revision_info
	for file in \
        $(distdir)/ext/flopoco/src/ExpressionScanner.cpp $(distdir)/ext/flopoco/src/ExpressionParser.cpp \
	$(distdir)/src/parser/polixml/xmlLexer.cpp $(distdir)/src/parser/polixml/xmlParser.hpp $(distdir)/src/parser/polixml/xmlParser.h $(distdir)/src/parser/polixml/xmlParser.cpp \
	$(distdir)/src/parser/compiler/treeLexer.cpp $(distdir)/src/parser/compiler/treeParser.hpp $(distdir)/src/parser/compiler/treeParser.h $(distdir)/src/parser/compiler/treeParser.cpp \
	$(distdir)/src/parser/aadl/aadl_lexer.cpp $(distdir)/src/parser/aadl/aadl_yparser.hpp $(distdir)/src/parser/aadl/aadl_yparser.h $(distdir)/src/parser/aadl/aadl_yparser.cpp \
	$(distdir)/src/parser/asn/asn_lexer.cpp $(distdir)/src/parser/asn/asn_yparser.hpp $(distdir)/src/parser/asn/asn_yparser.h $(distdir)/src/parser/asn/asn_yparser.cpp \
	$(distdir)/src/parser/discrepancy/discrepancyLexer.cpp $(distdir)/src/parser/discrepancy/discrepancyParser.hpp $(distdir)/src/parser/discrepancy/discrepancyParser.h $(distdir)/src/parser/discrepancy/discrepancyParser.cpp \
	$(distdir)/src/HLS/simulation/c_initialization_yparser.hpp $(distdir)/src/HLS/simulation/c_initialization_flex_lexer.cpp $(distdir)/src/HLS/simulation/c_initialization_yparser.cpp; \
	do\
	   if test -f $$file; then\
	      rm $$file;\
	   fi \
	done

