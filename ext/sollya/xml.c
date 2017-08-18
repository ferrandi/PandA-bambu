/*

Copyright 2007-2011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

and by

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France.

Contributors Ch. Lauter, S. Chevillard, N. Jourdan

christoph.lauter@ens-lyon.org
sylvain.chevillard@ens-lyon.org

This software is a computer program whose purpose is to provide an
environment for safe floating-point code development. It is
particularily targeted to the automatized implementation of
mathematical floating-point libraries (libm). Amongst other features,
it offers a certified infinity norm, an automatic polynomial
implementer and a fast Remez algorithm.

This software is governed by the CeCILL-C license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL-C
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-C license and that you accept its terms.

This program is distributed WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#include <mpfr.h>
#include "mpfi-compat.h"
#include <gmp.h>
#include "execute.h"
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <errno.h>
#include "general.h"
#include "expression.h"
#include "xml.h"
#include "execute.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

void printXml(node *tree) {
  fPrintXml(stdout,tree);
}


void fPrintXmlInner(FILE *fd, node *tree) {
  char *procString;

  if (tree == NULL) return;
  switch (tree->nodeType) {
  case VARIABLE:
    if (variablename == NULL) 
      sollyaFprintf(fd,"<ci> x </ci>\n");
    else 
      sollyaFprintf(fd,"<ci> %s </ci>\n",variablename);
    break;
  case CONSTANT:
    fprintValueForXml(fd, *(tree->value));
    break;
  case ADD:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<plus/>\n");
    fPrintXmlInner(fd, tree->child1);
    fPrintXmlInner(fd, tree->child2);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case SUB:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<minus/>\n");
    fPrintXmlInner(fd, tree->child1);
    fPrintXmlInner(fd, tree->child2);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case MUL:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<times/>\n");
    fPrintXmlInner(fd, tree->child1);
    fPrintXmlInner(fd, tree->child2);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case DIV:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<divide/>\n");
    fPrintXmlInner(fd, tree->child1);
    fPrintXmlInner(fd, tree->child2);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case SQRT:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<root/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case EXP:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<exp/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case LOG:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<ln/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case LOG_2:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<log/><logbase><cn>2</cn></logbase>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case LOG_10:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<log/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case SIN:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<sin/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case COS:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<cos/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case TAN:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<tan/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case ASIN:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<arcsin/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case ACOS:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<arccos/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case ATAN:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<arctan/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case SINH:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<sinh/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case COSH:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<cosh/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case TANH:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<tanh/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case ASINH:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<arcsinh/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case ACOSH:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<arccosh/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case ATANH:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<arctanh/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case POW:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<power/>\n");
    fPrintXmlInner(fd, tree->child1);
    fPrintXmlInner(fd, tree->child2);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case NEG:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<minus/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case ABS:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<abs/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case DOUBLE:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">double</csymbol>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case SINGLE:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">single</csymbol>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case QUAD:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">quad</csymbol>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case HALFPRECISION:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">halfprecision</csymbol>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case DOUBLEDOUBLE:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">doubledouble</csymbol>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case TRIPLEDOUBLE:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">tripledouble</csymbol>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case ERF: 
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.openmath.org/CDs/errorFresnelInts.ocd\" encoding=\"OpenMath\">erf</csymbol>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case ERFC:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.openmath.org/CDs/errorFresnelInts.ocd\" encoding=\"OpenMath\">erfc</csymbol>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case LOG_1P:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">log1p</csymbol>\n");
    //    fprintf(fd,"<log/><apply><plus/><cn>1</cn>\n");
    fPrintXmlInner(fd, tree->child1);
    //    fprintf(fd,"</apply></apply>\n");
    sollyaFprintf(fd,"</apply>\n");
    break;
  case EXP_M1:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">expm1</csymbol>\n");
    //    fprintf(fd,"<apply><minus/><apply><exp>\n");
    fPrintXmlInner(fd, tree->child1);
    //    fprintf(fd,"</apply><cn>1</cn></apply>\n");
    sollyaFprintf(fd,"</apply>\n");
    break;
  case DOUBLEEXTENDED:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">doubleextended</csymbol>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case LIBRARYFUNCTION:
    if (tree->libFunDeriv == 0) {
      sollyaFprintf(fd,"<apply>\n");
      sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">%s</csymbol>\n",
	      tree->libFun->functionName);
      fPrintXmlInner(fd, tree->child1);
      sollyaFprintf(fd,"</apply>\n");	 
    } else {
      sollyaFprintf(fd,"<apply>\n");
      sollyaFprintf(fd,"<diff/>\n");
      sollyaFprintf(fd,"<bvar>\n");
      if (variablename == NULL) 
	sollyaFprintf(fd,"<ci> x </ci>\n");
      else 
	sollyaFprintf(fd,"<ci> %s </ci>\n",variablename);
      sollyaFprintf(fd,"<degree>\n");
      sollyaFprintf(fd,"<cn> %d </cn>\n",tree->libFunDeriv);
      sollyaFprintf(fd,"</degree>\n");
      sollyaFprintf(fd,"</bvar>\n");
      sollyaFprintf(fd,"<apply>\n");
      sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">%s</csymbol>\n",
	      tree->libFun->functionName);
      fPrintXmlInner(fd, tree->child1);
      sollyaFprintf(fd,"</apply>\n");	 
      sollyaFprintf(fd,"</apply>\n");
    }
    break;
  case PROCEDUREFUNCTION:
    if (tree->libFunDeriv == 0) {
      procString = sPrintThing(tree->child2);
      sollyaFprintf(fd,"<apply>\n");
      sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">function(%s)</csymbol>\n",
	      procString);
      free(procString);
      fPrintXmlInner(fd, tree->child1);
      sollyaFprintf(fd,"</apply>\n");	 
    } else {
      sollyaFprintf(fd,"<apply>\n");
      sollyaFprintf(fd,"<diff/>\n");
      sollyaFprintf(fd,"<bvar>\n");
      if (variablename == NULL) 
	sollyaFprintf(fd,"<ci> x </ci>\n");
      else 
	sollyaFprintf(fd,"<ci> %s </ci>\n",variablename);
      sollyaFprintf(fd,"<degree>\n");
      sollyaFprintf(fd,"<cn> %d </cn>\n",tree->libFunDeriv);
      sollyaFprintf(fd,"</degree>\n");
      sollyaFprintf(fd,"</bvar>\n");
      sollyaFprintf(fd,"<apply>\n");
      procString = sPrintThing(tree->child2);
      sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">function(%s)</csymbol>\n",
	      procString);
      free(procString);
      fPrintXmlInner(fd, tree->child1);
      sollyaFprintf(fd,"</apply>\n");	 
      sollyaFprintf(fd,"</apply>\n");
    }
    break;
  case CEIL:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<ceiling/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case FLOOR:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<floor/>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case NEARESTINT:
    sollyaFprintf(fd,"<apply>\n");
    sollyaFprintf(fd,"<csymbol definitionURL=\"http://www.google.com/\" encoding=\"OpenMath\">nearestint</csymbol>\n");
    fPrintXmlInner(fd, tree->child1);
    sollyaFprintf(fd,"</apply>\n");
    break;
  case PI_CONST:
    sollyaFprintf(fd,"<pi/>");
    break;
  default:
    sollyaFprintf(stderr,"Error: fPrintXml: unknown identifier (%d) in the tree\n",tree->nodeType);
    exit(1);
  }
  return;  
}

void fPrintXml(FILE *fd, node *tree) {
  sollyaFprintf(fd,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  sollyaFprintf(fd,"<!-- generated by %s: http://sollya.gforge.inria.fr/ -->\n",PACKAGE_NAME);
  sollyaFprintf(fd,"<!-- syntax: printxml(...);   example: printxml(x^2-2*x+5); -->\n");
  //  fprintf(fd,"<!DOCTYPE math PUBLIC \"-//W3C//DTD MathML 2.0//EN\" \"http://www.w3.org/Math/DTD/mathml2/mathml2.dtd\" >\n");
  //  fprintf(fd,"<!-- MathML-Content (section 4 of MathML 2.0) -->\n");
  sollyaFprintf(fd,"<?xml-stylesheet type=\"text/xsl\" href=\"http://sollya.gforge.inria.fr/mathmlc2p-web.xsl\"?>\n");
  sollyaFprintf(fd,"<?xml-stylesheet type=\"text/xsl\" href=\"mathmlc2p-web.xsl\"?>\n");
  sollyaFprintf(fd,"<!-- This stylesheet allows direct web browsing of MathML-c XML files (http:// or file://) -->\n");
  sollyaFprintf(fd,"\n<math xmlns=\"http://www.w3.org/1998/Math/MathML\">\n");
  sollyaFprintf(fd,"<semantics>\n");
  sollyaFprintf(fd,"<annotation-xml encoding=\"MathML-Content\">\n");
  sollyaFprintf(fd,"<lambda>\n");
  if (variablename != NULL) 
    sollyaFprintf(fd,"<bvar><ci> %s </ci></bvar>\n",variablename);
  else 
    sollyaFprintf(fd,"<bvar><ci> x </ci></bvar>\n");
  sollyaFprintf(fd,"<apply>\n");
  fPrintXmlInner(fd,tree);
  sollyaFprintf(fd,"</apply>\n");
  sollyaFprintf(fd,"</lambda>\n");
  sollyaFprintf(fd,"</annotation-xml>\n");
  sollyaFprintf(fd,"<annotation encoding=\"sollya/text\">");
  fprintTree(fd, tree);
  sollyaFprintf(fd,"</annotation>\n");
  sollyaFprintf(fd,"</semantics>\n");
  sollyaFprintf(fd,"</math>\n\n");
}

// Nico:
// From: libxml2-2.6.30 and reader1.c example
// http://xmlsoft.org/examples/index.html#reader1.c

/** 
 * section: xmlReader
 * synopsis: Parse an XML file with an xmlReader
 * purpose: Demonstrate the use of xmlReaderForFile() to parse an XML file
 *          and dump the informations about the nodes found in the process.
 *          (Note that the XMLReader functions require libxml2 version later
 *          than 2.6.)
 * usage: reader1 <filename>
 * test: reader1 test2.xml > reader1.tmp ; diff reader1.tmp reader1.res ; rm reader1.tmp
 * author: Daniel Veillard
 * copy: see Copyright for the status of this software.
 */

#include <stdio.h>
#include <libxml/xmlreader.h>

#ifdef LIBXML_READER_ENABLED

#define change_xmlparser(new_parser) do { \
	printMessage(3,"%p => ",next_xmlparser); \
	next_xmlparser=new_parser; \
	printMessage(3,"%p\n",next_xmlparser); } while(0)



// PARSER MATHML - return: 0 (not found) -1 (sync lost) 1 (found)
int search_annotations		(xmlTextReaderPtr reader);
int process_annotation		(xmlTextReaderPtr reader);
int search_lambda     		(xmlTextReaderPtr reader);
int search_text   			(xmlTextReaderPtr reader);
int search_basic_element 	(xmlTextReaderPtr reader);
int search_return_offset 	(xmlTextReaderPtr reader);
int search_math_tree 		(xmlTextReaderPtr reader);

static int (*next_xmlparser)(xmlTextReaderPtr reader)=search_basic_element; //search_mathml;

static int				current_depth;
static const xmlChar *xml_name, *xml_value;
static node				*result_node;
char						*var_name;

int action_variable (xmlTextReaderPtr reader)
{
  if (!xmlTextReaderHasValue(reader)) return 0;
  var_name=(char*)xml_value;
  return 1;
}

//typedef node*(*pfn_op_binaire)(node*,node*);
typedef struct treemath nodemath;
struct treemath {
  nodemath*	child;
  nodemath*	parent;
  node*			(*operator)(node*,node*);
  int			op_type; // 0=no use, 1=csymbol, 2=ci, 3=cn
  node*			op1;
  node*			op2;
} math_tree={NULL,NULL,NULL,0,NULL,NULL},*mthis=&math_tree;

node* xml_make_neg (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeNeg  (n1);  return 0; }
node* xml_make_add (node* n1,node* n2) {if (n1&&n2) return makeAdd(n1,n2); return n1; }
node* xml_make_sub (node* n1,node* n2) {if (n1&&n2) return makeSub(n1,n2); return xml_make_neg(n1,0); }
node* xml_make_mul (node* n1,node* n2) {if (n1&&n2) return makeMul(n1,n2); return 0; }
node* xml_make_div (node* n1,node* n2) {if (n1&&n2) return makeDiv(n1,n2); return 0; }
node* xml_make_sqrt(node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeSqrt (n1);  return 0; }
node* xml_make_exp (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeExp  (n1);  return 0; }
node* xml_make_log (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeLog  (n1);  return 0; }
node* xml_make_log2(node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeLog2 (n1);  return 0; }
node* xml_make_logA(node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeLog10(n1);  return 0; }
node* xml_make_sin (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeSin  (n1);  return 0; }
node* xml_make_cos (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeCos  (n1);  return 0; }
node* xml_make_tan (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeTan  (n1);  return 0; }
node* xml_make_asin(node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeAsin (n1);  return 0; }
node* xml_make_acos(node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeAcos (n1);  return 0; }
node* xml_make_atan(node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeAtan (n1);  return 0; }
node* xml_make_sh  (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeSinh (n1);  return 0; }
node* xml_make_ch  (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeCosh (n1);  return 0; }
node* xml_make_th  (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeTanh (n1);  return 0; }
node* xml_make_ash (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeAsinh(n1);  return 0; }
node* xml_make_ach (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeAcosh(n1);  return 0; }
node* xml_make_ath (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeAtanh(n1);  return 0; }
node* xml_make_pow (node* n1,node* n2) {if (n1&&n2) return makePow(n1,n2); return 0; }
node* xml_make_abs (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeAbs  (n1);  return 0; }
node* xml_make_db  (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeDouble(n1); return 0; }
node* xml_make_sg  (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeSingle(n1); return 0; }
node* xml_make_hp  (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeHalfPrecision(n1); return 0; }
node* xml_make_qd  (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeQuad(n1); return 0; }
node* xml_make_db2 (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeDoubledouble(n1); return 0; }
node* xml_make_db3 (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeTripledouble(n1); return 0; }
node* xml_make_dbex(node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeDoubleextended(n1); return 0; }
node* xml_make_ceil(node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeCeil (n1);  return 0; }
node* xml_make_floor(node* n1,node* n2){UNUSED_PARAM(n2); if (n1)     return makeFloor(n1);  return 0; }
node* xml_make_erf (node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeErf  (n1);  return 0; }
node* xml_make_erfc(node* n1,node* n2) {UNUSED_PARAM(n2); if (n1)     return makeErfc (n1);  return 0; }
node* xml_make_log1p(node* n1,node* n2){UNUSED_PARAM(n2); if (n1)     return makeLog1p(n1);  return 0; }
node* xml_make_expm1(node* n1,node* n2){UNUSED_PARAM(n2); if (n1)     return makeExpm1(n1);  return 0; }

struct {
  char*	element;				// name of element, "math" for <math>
  int	type;					// type of XML chunk, 1 = <math>, 15 = </math>, ...
  int	(*parser)(xmlTextReaderPtr reader); // parser to use to detect/validate this statement
  int	next_xmlparser;	// index in mml_parser[] in case of parsing success
  int	onerror_parser;	// index in mml_parser[] in case of parsing error
  int	depth; 				// current XML depth
  int	offset_depth;		// offset of next parser, (-1)==any child level, 1==next child level 
  int	(*action)(xmlTextReaderPtr reader);	// ?
} mml_parser[]={
  // element:		type:	parser:					next:	err:	d:	offs:		
  {	"math",			1,	search_basic_element,	1,	-1,	0,	-1,	0	}, // search_mathml
  {	"semantics",	1,	search_basic_element,	2,	0,		0,	1,		0	}, // search_semantics
  {	"annotations",	1,	search_annotations,		3,	1,		0,	1,		0	}, // search_annotations
  {	"lambda",		1,	search_basic_element,	4,	2,		0,	-1,	0	}, // search_lambda
  {	"bvar",			1,	search_basic_element,	5,	3,		0,	1,		0	}, // search_bvar
  {	"ci",				1,	search_basic_element,	6,	3,		0,	1,		0	}, // search_ci
  {	"#text",			3,	search_basic_element,	7,	3,		0,	1,		action_variable	}, // search_variable
  {	"ci",				15,search_basic_element,	8,	3,		0,	1,		0	}, // search_ci
  {	"bvar",			15,search_basic_element,	9,	3,		0,	1,		0	}, // search_bvar
  {	"bvar",			15,search_math_tree,			10,3,		0,	1,		0	}, // math tree
  {	"simul-error",	0, search_return_offset,	-1,4,		0, -1,	0	},
},*current_parser=&mml_parser[0];


void switch_parser_index (int new_index)
{
  printMessage(3,"%s => %s\n",current_parser->element,mml_parser[new_index].element);
  current_parser=&mml_parser[new_index];
  change_xmlparser(current_parser->parser);
  printMessage(3,"depth: %i\n",current_parser->depth);
}

int search_math_tree (xmlTextReaderPtr reader)
{
  node* temp=0;

  if (xmlTextReaderNodeType(reader)==1)
    {
      if (!strcmp((char*)xml_name, "apply"))
	{
	  mthis->child=safeMalloc(sizeof(nodemath));
	  mthis->child->parent=mthis;
	  mthis=mthis->child;
	  mthis->child=0;
	  mthis->operator=0;
	  mthis->op_type=0;
	  mthis->op1=0;
	  mthis->op2=0;
	  printMessage(3,"This: %p Parent: %p Name: %s\n",mthis,mthis->parent,xml_name);
	}
      else if (!strcmp((char*)xml_name, "csymbol")) mthis->op_type=1;
      else if (!strcmp((char*)xml_name, "ci")) 		mthis->op_type=2;
      else if (!strcmp((char*)xml_name, "cn")) 		mthis->op_type=3;
      else if (!strcmp((char*)xml_name, "plus"))   	mthis->operator=xml_make_add;
      else if (!strcmp((char*)xml_name, "minus")) 	mthis->operator=xml_make_sub;
      else if (!strcmp((char*)xml_name, "times")) 	mthis->operator=xml_make_mul;
      else if (!strcmp((char*)xml_name, "divide"))	mthis->operator=xml_make_div;
      else if (!strcmp((char*)xml_name, "root")) 	mthis->operator=xml_make_sqrt;
      else if (!strcmp((char*)xml_name, "exp"))   	mthis->operator=xml_make_exp;
      else if (!strcmp((char*)xml_name, "ln")) 	   mthis->operator=xml_make_log;
      else if (!strcmp((char*)xml_name, "log"))		mthis->operator=xml_make_logA;
      else if (!strcmp((char*)xml_name, "sin")) 		mthis->operator=xml_make_sin;
      else if (!strcmp((char*)xml_name, "cos")) 		mthis->operator=xml_make_cos;
      else if (!strcmp((char*)xml_name, "tan")) 		mthis->operator=xml_make_tan;
      else if (!strcmp((char*)xml_name, "arcsin"))	mthis->operator=xml_make_asin;
      else if (!strcmp((char*)xml_name, "arccos")) 	mthis->operator=xml_make_acos;
      else if (!strcmp((char*)xml_name, "arctan")) 	mthis->operator=xml_make_atan;
      else if (!strcmp((char*)xml_name, "sinh")) 	mthis->operator=xml_make_sh;
      else if (!strcmp((char*)xml_name, "cosh")) 	mthis->operator=xml_make_ch;
      else if (!strcmp((char*)xml_name, "tanh")) 	mthis->operator=xml_make_th;
      else if (!strcmp((char*)xml_name, "arcsinh"))	mthis->operator=xml_make_ash;
      else if (!strcmp((char*)xml_name, "arccosh")) mthis->operator=xml_make_ach;
      else if (!strcmp((char*)xml_name, "arctanh")) mthis->operator=xml_make_ath;
      else if (!strcmp((char*)xml_name, "power")) 	mthis->operator=xml_make_pow;
      else if (!strcmp((char*)xml_name, "abs")) 		mthis->operator=xml_make_abs;
      else if (!strcmp((char*)xml_name, "ceiling"))	mthis->operator=xml_make_ceil;
      else if (!strcmp((char*)xml_name, "floor")) 	mthis->operator=xml_make_floor;
      else if (!strcmp((char*)xml_name, "pi")) 		     temp=makePi();
      else if (!strcmp((char*)xml_name, "exponentiale")) temp=makeExp(makeConstantDouble(1.));
      else { switch_parser_index(current_parser->onerror_parser); return -1; } // error: markup unknown
    }
  else if (xmlTextReaderNodeType(reader)==3) // Text
    {
      if (mthis->op_type==1) // csymbol
	{
	  if (!strcmp((char*)xml_value,"erf"))		mthis->operator=xml_make_erf;
	  else if (!strcmp((char*)xml_value,"erfc"))		mthis->operator=xml_make_erfc;
	  else if (!strcmp((char*)xml_value,"log1p"))	mthis->operator=xml_make_log1p;
	  else if (!strcmp((char*)xml_value,"expm1"))	mthis->operator=xml_make_expm1;
	  else if (!strcmp((char*)xml_value,"single"))	mthis->operator=xml_make_sg;
	  else if (!strcmp((char*)xml_value,"halfprecision"))	mthis->operator=xml_make_hp;
	  else if (!strcmp((char*)xml_value,"quad"))	mthis->operator=xml_make_qd;
	  else if (!strcmp((char*)xml_value,"double"))	mthis->operator=xml_make_db;
	  else if (!strcmp((char*)xml_value,"doubledouble"))	mthis->operator=xml_make_db2;
	  else if (!strcmp((char*)xml_value,"tripledouble"))	mthis->operator=xml_make_db3;
	  else if (!strcmp((char*)xml_value,"doubleextended"))mthis->operator=xml_make_dbex;
	}
      if (mthis->op_type==2) // temp=makeVariable(); // ci
	{ // Test only one variable (in bvar) and always the same name 
	  if(!strcmp((char*)xml_value,var_name)) temp=makeVariable(); // ci
	  else { switch_parser_index(current_parser->onerror_parser); return -1; } // error: variable unknown
	}
      if (mthis->op_type==3) //temp=parseString((char*)xml_value);// cn
	{
	  mpfr_t 	n;
	  mpfr_init2(n,tools_precision);
	  if (readDecimalConstant(n,(char*)xml_value)) {
	    if (!noRoundingWarnings) {
	      printMessage(1,"Warning: rounding has happened upon reading constant \"%s\" in an XML file.\n",xml_value);
	    }
	  }
	  temp = makeConstant(n);
	  mpfr_clear(n);
	} 
    }
  else if (xmlTextReaderNodeType(reader)==15)
    {
      if (!strcmp((char*)xml_name, "lambda"))
	{
	  result_node=mthis->op1;
	  printMessage(3,"This: %p Result: %p Name: /%s\n",mthis,result_node,xml_name);
	  change_xmlparser(search_annotations);
	}
      else if (!strcmp((char*)xml_name, "apply") && mthis->operator)
	{
	  printMessage(3,"op1: %p op2: %p op: %p\n",mthis->op1,mthis->op2,mthis->operator);
	  temp=mthis->operator(mthis->op1,mthis->op2);
	  //printTree(temp);  printMessage(2,"\n");
	  if (mthis->parent) mthis=mthis->parent;
	  if (mthis->child)  free(mthis->child);
	  mthis->child=0;
	}
      else if (!strcmp((char*)xml_name, "csymbol")) mthis->op_type=0;
      /* the following line should test the case of a logarithm in an exotic basis */
      /* Currently we DO NOT support this kind of logarithm but only ln and log10 */
      /* Any patch is welcome */
      /* if (!strcmp((char*)xml_name, "logbase")) ... */
      printMessage(3,"This: %p Temp: %p Name: /%s\n",mthis,temp,xml_name);
    }
  if (temp) // some value/result to add in the tree
    {
      if (!mthis->op1) mthis->op1=temp;
      else if (!mthis->op2) mthis->op2=temp;
      else if (mthis->operator)
	{
	  mthis->op1=mthis->operator(mthis->op1,mthis->op2);
	  mthis->op2=temp;
	}
      else mthis->op1=mthis->op2=0,mthis->operator=0; // error
      printMessage(3,"This: %p Temp: %p Name: /%s\n",mthis,temp,xml_name);
    }
  return 1;
}


int search_return_offset (xmlTextReaderPtr reader) {
  UNUSED_PARAM(reader);
  switch_parser_index(current_parser->onerror_parser);
  return current_parser->offset_depth;
}

int search_basic_element (xmlTextReaderPtr reader)
{
  // on_error 
  if (current_parser->onerror_parser!=-1 && current_parser->depth>=xmlTextReaderDepth(reader))
    {
      switch_parser_index(current_parser->onerror_parser);
      return -1;
    }
  // on_cannot_find
  if (xmlTextReaderIsEmptyElement(reader) ||
      strcmp((char*)xml_name,current_parser->element) ||
      xmlTextReaderNodeType(reader)!=current_parser->type ||
      (current_parser->offset_depth!=-1 && current_parser->depth+1!=xmlTextReaderDepth(reader))
      ) return 0;
  // on_found
  if (current_parser->action) if (!current_parser->action(reader)) return 0;
  switch_parser_index(current_parser->next_xmlparser);
  current_parser->depth=xmlTextReaderDepth(reader);
  if (current_parser->type==15) current_parser->depth-=2;
  return 1;
}



int process_annotation(xmlTextReaderPtr reader)
{
  // using external node *parseString(char *)
  // Depth: 03 Type: 03 Name: #text (HasValue) ((x^(1b1)) + (1b2 * x)) - sin(x)
  // on_error 
  if (current_parser->depth+1>=xmlTextReaderDepth(reader))
    { change_xmlparser(search_annotations); return -1; }
  // on_search
  if (xmlTextReaderIsEmptyElement(reader) ||
      xmlTextReaderNodeType(reader)!=3 ||
      current_parser->depth+2!=xmlTextReaderDepth(reader)
      ) return 0;
  result_node=parseString((char*)xml_value);
  change_xmlparser(search_annotations);
  return 1;
}


int search_annotations(xmlTextReaderPtr reader)
{
  // on_error 
  if (current_parser->depth>=xmlTextReaderDepth(reader))
    {
      printMessage(3,"%s => %s\n",current_parser->element,mml_parser[current_parser->onerror_parser].element);
      current_parser=&mml_parser[current_parser->onerror_parser];
      change_xmlparser(current_parser->parser);
      return -1;
    }
  // on_search
  if (xmlTextReaderIsEmptyElement(reader) ||
      xmlTextReaderNodeType(reader)!=1 ||
      current_parser->depth+1!=xmlTextReaderDepth(reader)
      ) return 0;
  if (!strcmp((char*)xml_name,"annotation") &&
      xmlTextReaderHasAttributes(reader) &&
      !strcmp((char *) xmlTextReaderGetAttribute(reader,(unsigned char *)"encoding"),"sollya/text") )
    {change_xmlparser(process_annotation); return 1;}
  if (!strcmp((char*)xml_name,"annotation-xml") &&
      xmlTextReaderHasAttributes(reader) &&
      !strcmp((char *) xmlTextReaderGetAttribute(reader,(unsigned char *)"encoding"),"MathML-Content") )
    { switch_parser_index(3); current_parser->depth=xmlTextReaderDepth(reader); return 1;}
  // on_not_found
  return 0;
}


/**
 * processNode:
 * @reader: the xmlReader
 *
 * Dump information about the current node
 */
static int
processNode(xmlTextReaderPtr reader) {
  // doc/info: http://xmlsoft.planetmirror.com/html/libxml-xmlreader.html
  int				ret;

  ret = xmlTextReaderRead(reader);
  if (ret!=1) { // EOF or ERROR
    if (ret) printMessage(1,"Warning: on parsing an XML file: failed to parse, return code %i\n",ret);
    return ret;
  }
  xml_name = xmlTextReaderConstName(reader);
  if (xml_name == NULL)
    xml_name = BAD_CAST "--";

  xml_value = xmlTextReaderConstValue(reader);

  printMessage(3,"Depth: %02d Type: %02d Name: %s", 
	       xmlTextReaderDepth(reader),
	       xmlTextReaderNodeType(reader),
	       xml_name);
  if (xmlTextReaderIsEmptyElement(reader)) printMessage(3," (EmptyElt)");
  if (xmlTextReaderHasValue(reader))       printMessage(3," (HasValue)");
  if (xmlTextReaderHasAttributes(reader))  printMessage(3," (HasAttrb)");
  if (xml_value == NULL || // XmlNodeType.SignificantWhitespace
      xmlTextReaderNodeType(reader)==14)  	printMessage(3,"\n");
  else {
    if (xmlStrlen(xml_value) > 40)       printMessage(3," %.40s...\n", xml_value);
    else                                 printMessage(3," %s\n", xml_value);  }
  while((*next_xmlparser)(reader)<0) printMessage(2,"Lost Sync! Try resync...\n");
  return ret;
}





/**
 * streamFile:
 * @filename: the file name to parse
 *
 * Parse and print information about an XML file.
 */
static node*
streamXmlFile(const char *filename) {
  xmlTextReaderPtr reader;

  reader = xmlReaderForFile(filename, NULL, 0);
  if (reader == NULL) { printMessage(1,"Warning: Unable to open %s\n", filename); return NULL; }
  for(result_node=NULL,current_depth=0;processNode(reader)==1 && !result_node;);
  xmlFreeTextReader(reader);
  return result_node;
}

/* Reads the file filename containing a lambda construct
   into a node * 
   Return NULL if parsing the file is impossible

   Indication: use make... found in expression.h for building the nodes.
   Attention: do not forget do free incomplete trees if the 
   parsing fails. Use free_memory(node *) for this (found in expression.h).

   If warnings to the user shall be indicated, use printMessage(verbosity level, format string, ...)
   verbosity level=0 nothing 1 important 2 information 3 internal/debug...
*/
node *readXml(char *filename) {
  node* result=NULL;
	
  /*
   * this initialize the library and check potential ABI mismatches
   * between the version it was compiled for and the actual shared
   * library used.
   */
  LIBXML_TEST_VERSION

    result=streamXmlFile(filename);

  /*
   * Cleanup function for the XML library.
   */
  xmlCleanupParser();
  /*
   * this is to debug memory for regression tests
   */
  xmlMemoryDump();

  return result;
}

#else
node *readXml(char *filename) {
  printMessage(1,"Warning: We should now read the XML file \"%s\".\n",filename);
  printMessage(1,"XInclude support not compiled in, cannot parse XML file.\n");
  return NULL;
}
#endif

