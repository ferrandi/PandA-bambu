#this file is src/polixml/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES = lib_polixml.la

noinst_HEADERS = xml_att_decl_node.hpp xml_attribute.hpp xml_comment_node.hpp xml_document.hpp xml_element.hpp xml_node.hpp xml_text_node.hpp polixml.hpp

lib_polixml_la_SOURCES = xml_node.cpp

lib_polixml_la_CPPFLAGS = \
   -I$(top_srcdir)/src/utility \
   -I$(BOOST_DIR) $(all_includes)

#do not touch the following line

