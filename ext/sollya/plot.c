/*

Copyright 2006-2010 by 

Laboratoire de l'Informatique du Parallelisme, 
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

and by

LORIA (CNRS, INPL, INRIA, UHP, U-Nancy 2)

Contributors Ch. Lauter, S. Chevillard

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

#include <gmp.h>
#include <mpfr.h>
#include <stdio.h> /* fprintf, fopen, fclose, */
#include <sys/types.h> /* pid_t */
#include <sys/wait.h> /* wait */
#include <unistd.h> /* execve, fork, daemon */
#include <stdlib.h> /* exit, free, mktemp */
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "plot.h"
#include "expression.h"
#include "general.h"
#include "infnorm.h"
#include "chain.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern int fileNumber;

void checkFileDescriptor(FILE *fd, char *s) {
  if (fd == NULL) {
    sollyaFprintf(stderr,"Error: the file %s requested by plot could not be opened for writing: ",s);
    sollyaFprintf(stderr,"\"%s\".\n",strerror(errno));
    recoverFromError();

  }
  return;
}

void plotTree(chain *treeList, mpfr_t a, mpfr_t b, unsigned long int points, mp_prec_t prec, char *name, int type) {
  int test, i, flush;
  chain *list;
  node *tree;
  mpfr_t current_x, x, y, step, perturb, cutoff;
  gmp_randstate_t random_state;
  double xd, yd, ad, bd;
  FILE *file;
  char *gplotname;
  char *dataname;
  char *outputname;
  int tern;
  mp_prec_t p, pp;
  int overflow;

  pp = prec;
  if (prec < 64) pp = 64;

  mpfr_init2(x, pp);
  mpfr_init2(step, pp);
  mpfr_init2(y, 64);
 
  mpfr_sub(step, b, a, GMP_RNDN);
  mpfr_div_ui(step, step, points, GMP_RNDN);
 
  if (mpfr_sgn(step) == 0) {
    list = treeList;
    mpfr_set_prec(y,prec);
    while(list != NULL) {
      tree = (node *)(list->value);
      evaluateFaithful(y,tree,a,prec);
      if (!mpfr_number_p(y)) {
	printMessage(1,"Warning: this constant function is not evaluable by this tool.\n");
      } 
      outputMode();
      printValue(&y);
      sollyaPrintf("\n");
      list = list->next;
    }
    mpfr_clear(x); mpfr_clear(y); mpfr_clear(step);
    return;
  }

  if (mpfr_sgn(step) < 0) {
    printMessage(1,"Warning: the interval is empty\n");
    mpfr_clear(x); mpfr_clear(y); mpfr_clear(step);
    return;
  }

  test=1;
  list = treeList;
  while(list != NULL && (test==1)) {
    tree = (node *)(list->value);
    if(!isConstant(tree)) test=0;
    list = list->next;
  }
 
  if (test) {
    mpfr_set_prec(y,prec);
    mpfr_set_d(x,1.0,GMP_RNDN);
    list = treeList;
    while(list != NULL && (test==1)) {
      tree = (node *)(list->value);
      evaluateFaithful(y,tree,x,prec);
      if (!mpfr_number_p(y)) {
	printMessage(1,"Warning: this constant function is not evaluable by this tool.\n");
      } 
      outputMode();
      printValue(&y);
      sollyaPrintf("\n");
      list = list->next;
    }
    mpfr_clear(x); mpfr_clear(y); mpfr_clear(step);
    return;
  }

  if(name==NULL) {
    gplotname = (char *)safeCalloc(13 + strlen(PACKAGE_NAME), sizeof(char));
    sprintf(gplotname,"/tmp/%s-%04d.p",PACKAGE_NAME,fileNumber);
    dataname = (char *)safeCalloc(15 + strlen(PACKAGE_NAME), sizeof(char));
    sprintf(dataname,"/tmp/%s-%04d.dat",PACKAGE_NAME,fileNumber);
    outputname = (char *)safeCalloc(1, sizeof(char));
    fileNumber++;
    if (fileNumber >= NUMBEROFFILES) fileNumber=0;
  }
  else {
    gplotname = (char *)safeCalloc(strlen(name)+3,sizeof(char));
    sprintf(gplotname,"%s.p",name);
    dataname = (char *)safeCalloc(strlen(name)+5,sizeof(char));
    sprintf(dataname,"%s.dat",name);
    outputname = (char *)safeCalloc(strlen(name)+5,sizeof(char));   
    if ((type==PLOTPOSTSCRIPT) || (type==PLOTPOSTSCRIPTFILE)) sprintf(outputname,"%s.eps",name);
  }

  file = fopen(gplotname, "w");
  checkFileDescriptor(file, gplotname);
  sollyaFprintf(file, "# Gnuplot script generated by %s\n",PACKAGE_NAME);
  ad = mpfr_get_d(a, GMP_RNDD);
  bd = mpfr_get_d(b, GMP_RNDU);
  if ((!(ad-ad == 0.0)) || (!(bd-bd == 0.0)))
    printMessage(1, "Warning: an overflow occurred in a conversion mpfr to double while plotting.\n");
  if (!(ad-ad == 0.0))
    ad = -MAX_VALUE_GNUPLOT;
  if (!(bd-bd == 0.0))
    bd = MAX_VALUE_GNUPLOT;
  if ((type==PLOTPOSTSCRIPT) || (type==PLOTPOSTSCRIPTFILE)) sollyaFprintf(file,"set terminal postscript eps color\nset out \"%s\"\n",outputname);
  sollyaFprintf(file, "set xrange [%1.50e:%1.50e]\n",  ad, bd);

  sollyaFprintf(file, "plot ");
  i=2;
  list = treeList;
  while(list != NULL) {
    sollyaFprintf(file,"\"%s\" using 1:%d with lines t \"\"",dataname,i);
    if(list->next != NULL) sollyaFprintf(file,",");
    i++;
    list = list->next;
  }
  sollyaFprintf(file,"\n");
  fclose(file);

  file = fopen(dataname, "w");
  checkFileDescriptor(file, dataname);

  mpfr_init2(cutoff, pp);
  mpfr_set_d(cutoff,1.0,GMP_RNDN);
  p = prec;
  if (p < 64) p = 64;
  mpfr_div_2ui(cutoff,cutoff,4*p,GMP_RNDN);
 
  overflow = 0;
  flush = 0;

  mpfr_init2(current_x, pp);
  gmp_randinit_default(random_state);
  gmp_randseed_ui(random_state, 65845285);
  mpfr_init2(perturb, pp);
  
  mpfr_set(current_x,a,GMP_RNDN);
  mpfr_set(x, current_x, GMP_RNDN);
  while(mpfr_lessequal_p(x,b)) {
    xd =  mpfr_get_d(x, GMP_RNDN);
    if (xd >= MAX_VALUE_GNUPLOT) xd = MAX_VALUE_GNUPLOT;
    if (xd <= -MAX_VALUE_GNUPLOT) xd = -MAX_VALUE_GNUPLOT;
    sollyaFprintf(file, "%1.50e",xd);

    list = treeList;
    while(list != NULL) {
      tree = (node *)(list->value);
      tern = evaluateFaithfulWithCutOff(y, tree, x, cutoff, prec);
      if (tern == 2) {
	flush = 1;
	if (verbosity >= 2) { 	
	  changeToWarningMode(); 
	  sollyaPrintf("Information: function image proven to be less than 2^(-%d) on point %s = ",(int)p,variablename);
	  printValue(&x);
	  sollyaPrintf("\nThis point will be plotted as the midpoint of the proof interval.\n");
	  restoreMode();
	}
      }
      if (!mpfr_number_p(y)) {
	if (verbosity >= 2) {
	  changeToWarningMode();
	  sollyaPrintf("Information: function undefined or not evaluable in point %s = ",variablename);
	  printValue(&x);
	  sollyaPrintf("\nThis point will not be plotted.\n");
	  restoreMode();
	}
      }
      yd = mpfr_get_d(y, GMP_RNDN);
      if (!(yd-yd == 0.0)) overflow = 1;
      if (yd >= MAX_VALUE_GNUPLOT) {
	yd = MAX_VALUE_GNUPLOT;
	overflow = 1;
      }
      if (yd <= -MAX_VALUE_GNUPLOT) {
	yd = -MAX_VALUE_GNUPLOT;
	overflow = 1;
      }
      sollyaFprintf(file, "\t%1.50e", yd);
      
      list = list->next;
    }
    sollyaFprintf(file,"\n");

    mpfr_add(current_x, current_x,step,GMP_RNDN);
    mpfr_urandomb(perturb, random_state); mpfr_mul_2ui(perturb, perturb, 1, GMP_RNDN);
    mpfr_sub_ui(perturb, perturb, 1, GMP_RNDN); mpfr_div_2ui(perturb, perturb, 2, GMP_RNDN);
    mpfr_mul(perturb, perturb, step, GMP_RNDN); // perturb \in [-step/4; step/4]
    mpfr_add(x, current_x, perturb, GMP_RNDU);
  }

  mpfr_clear(cutoff);
  mpfr_clear(perturb);
  mpfr_clear(current_x);
  gmp_randclear(random_state);
 
  fclose(file);

  if (overflow)
    printMessage(1, "Warning: an overflow occurred in a conversion mpfr to double while plotting.\n");

  mpfr_clear(x); mpfr_clear(y); mpfr_clear(step);

  if (flush) {
    printMessage(1,"Warning: the evaluation of the image on at least one point of at least one function has not been faithfully accurate.\n");
  }

  if ((name==NULL) || (type==PLOTFILE)) {
    if (fork()==0) {
      daemon(1,1);
      execlp("gnuplot", "gnuplot", "-persist", gplotname, NULL);
      perror("An error occurred when calling gnuplot ");
      exit(1);
    }
    else wait(NULL);
  }
  else { /* Case we have an output: no daemon */
    if (fork()==0) {
      execlp("gnuplot", "gnuplot", "-persist", gplotname, NULL);
      perror("An error occurred when calling gnuplot ");
      exit(1);
    }
    else {
      wait(NULL);
      if((type==PLOTPOSTSCRIPT)) {
	remove(gplotname);
	remove(dataname);
      }
    }
  }
  
  free(gplotname);
  free(dataname);
  free(outputname);
  return;

}

void removePlotFiles(void) {
  int i;
  char *name;
  name = (char *)safeCalloc(40 + strlen(PACKAGE_NAME), sizeof(char));

  for(i=0;i<NUMBEROFFILES;i++) {
    sprintf(name,"/tmp/%s-%04d.p",PACKAGE_NAME,i);
    remove(name);
    sprintf(name,"/tmp/%s-%04d.dat",PACKAGE_NAME,i);
    remove(name);
  }

  free(name);
  return;
}


void asciiPlotTree(node *tree, mpfr_t a, mpfr_t b, mp_prec_t prec) {
  mpfr_t y, x, step, minValue, maxValue;
  mpfr_t perturb;
  gmp_randstate_t random_state;
  int sizeX, sizeY, i, k, drawXAxis, drawYAxis, xAxis, yAxis;
#if defined(__CYGWIN__)
  struct winsize {
        unsigned short  ws_row;     /* rows in characters */
        unsigned short  ws_col;     /* columns in characters */
        unsigned short  ws_xpixel;  /* horizontal size in pixels (not used) */
        unsigned short  ws_ypixel;  /* vertical size in pixels (not used) */
  } size;
#else 
  struct winsize size;
#endif
  mpfr_t *values;
  char **lines;
  char *curr;
  int oldColor;

  mpfr_init2(y,prec);
  if ((mpfr_cmp(a,b) == 0) || isConstant(tree)) {
    evaluateFaithful(y,tree,a,prec);
    if (!mpfr_number_p(y)) {
      printMessage(1,"Warning: this constant function is not evaluable by this tool.\n");
    } 
    printValue(&y);
    sollyaPrintf("\n");
    mpfr_clear(y);
    return;
  }


#if defined(TIOCGWINSZ) && defined(STDIN_FILENO)
  if (eliminatePromptBackup == 1) {
    sizeX = 77;
    sizeY = 25;
  } else {
    ioctl(STDIN_FILENO, TIOCGWINSZ, (char *) &size);
    sizeX = size.ws_col;
    sizeY = size.ws_row;
    if (sizeX > 5000) sizeX = 5000;
    if (sizeY > 5000) sizeY = 5000;
  }
#else
  sizeX = 77;
  sizeY = 25;
#endif

  values = (mpfr_t *) safeCalloc(sizeX-1,sizeof(mpfr_t));
  for (i=0;i<sizeX-1;i++) {
    mpfr_init2(values[i],2*prec);
  }

  mpfr_init2(x,prec);
  mpfr_init2(step,prec);
  mpfr_sub(step,b,a,GMP_RNDN);
  mpfr_set_si(x,sizeX-2,GMP_RNDN);
  mpfr_div(step,step,x,GMP_RNDN);

  mpfr_init2(perturb, prec);
  gmp_randinit_default(random_state);
  gmp_randseed_ui(random_state, 65845285);

  for (i=0;i<sizeX-1;i++) {
    mpfr_set_si(x,i,GMP_RNDN);
    mpfr_mul(x,x,step,GMP_RNDN);
    mpfr_add(x,x,a,GMP_RNDN);

    mpfr_urandomb(perturb, random_state); mpfr_mul_2ui(perturb, perturb, 1, GMP_RNDN);
    mpfr_sub_ui(perturb, perturb, 1, GMP_RNDN); mpfr_div_2ui(perturb, perturb, 2, GMP_RNDN);
    mpfr_mul(perturb, perturb, step, GMP_RNDN); // perturb \in [-step/4; step/4]
    mpfr_add(x, x, perturb, GMP_RNDN);
    evaluateFaithful(values[i],tree,x,prec);
    if (!mpfr_number_p(values[i])) {
      mpfr_set_d(values[i],0.0,GMP_RNDN);
    }
  }

  mpfr_clear(perturb);
  gmp_randclear(random_state);

  mpfr_init2(minValue,prec);
  mpfr_init2(maxValue,prec);
  mpfr_set(minValue,values[0],GMP_RNDN);
  mpfr_set(maxValue,values[0],GMP_RNDN);
  for (i=1;i<sizeX-1;i++) {
    if (mpfr_cmp(values[i],minValue) < 0) {
      mpfr_set(minValue, values[i], GMP_RNDN);
    }
    if (mpfr_cmp(values[i],maxValue) > 0) {
      mpfr_set(maxValue, values[i], GMP_RNDN);
    }
  }

  drawXAxis = 0; drawYAxis = 0; xAxis = 0; yAxis = 0;
  if (mpfr_sgn(minValue) != mpfr_sgn(maxValue)) {
    drawXAxis = 1;
    mpfr_neg(x,minValue,GMP_RNDN);
    mpfr_sub(y,maxValue,minValue,GMP_RNDN);
    mpfr_div(x,x,y,GMP_RNDN);
    mpfr_mul_si(x,x,sizeY-2,GMP_RNDN);
    xAxis = mpfr_get_si(x,GMP_RNDN);
    if (xAxis < 0) xAxis = 0;
    if (xAxis > (sizeY - 2)) xAxis = sizeY - 2;
  }

  if (mpfr_sgn(a) != mpfr_sgn(b)) {
    drawYAxis = 1;
    mpfr_neg(x,a,GMP_RNDN);
    mpfr_div(x,x,step,GMP_RNDN);
    yAxis = mpfr_get_si(x,GMP_RNDN);
    if (yAxis < 0) yAxis = 0;
    if (yAxis > (sizeX - 2)) yAxis = 2;
  }

  for (i=0;i<sizeX-1;i++) {
    mpfr_sub(values[i],values[i],minValue,GMP_RNDN);
  }
  mpfr_sub(maxValue,maxValue,minValue,GMP_RNDN);
  for (i=0;i<sizeX-1;i++) {
    mpfr_div(values[i],values[i],maxValue,GMP_RNDN);
    mpfr_mul_si(values[i],values[i],sizeY-2,GMP_RNDN);
  }

  lines = (char **) safeCalloc(sizeY,sizeof(char *));
  for (k=0;k<sizeY-1;k++) {
    lines[k] = (char *) safeCalloc(sizeX,sizeof(char));
    for (i=0;i<sizeX-1;i++) 
      lines[k][i] = ' ';
  }

  if (drawXAxis) {
    for (i=0;i<sizeX-1;i++) 
      lines[(sizeY - 2) - xAxis][i] = '-';
  }

  if (drawYAxis) {
    for (k=0;k<sizeY-1;k++) 
      lines[k][yAxis] = '|';
  }

  if (drawXAxis && drawYAxis) {
    lines[(sizeY - 2) - xAxis][yAxis] = '+';
  }

  for (i=0;i<sizeX-1;i++) {
    k = mpfr_get_si(values[i],GMP_RNDN);
    if (k < 0) k = 0;
    if (k > (sizeY - 2)) k = sizeY - 2;
    lines[(sizeY - 2) - k][i] = 'x'; 
  }

  saveMode();
  for (k=0;k<sizeY-1;k++) { 
    curr = lines[k];
    while (*curr != '\0') {
      if (*curr != 'x')
	sollyaPrintf("%c",*(curr++));
      else {
	oldColor = getDisplayColor();
	redMode();
	sollyaPrintf("%c",*(curr++));
	setDisplayColor(oldColor);
      }
    }
    sollyaPrintf("\n");
  }

  restoreMode();

  for (k=0;k<sizeY-1;k++) free(lines[k]);
  free(lines);
  mpfr_clear(minValue);
  mpfr_clear(maxValue);
  mpfr_clear(y);
  mpfr_clear(x);
  mpfr_clear(step);
  for (i=0;i<sizeX-1;i++) {
    mpfr_clear(values[i]);
  }
  free(values);
  return;
}
