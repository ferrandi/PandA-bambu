/*

Copyright 2006-2011 by

Laboratoire de l'Informatique du Parallelisme,
UMR CNRS - ENS Lyon - UCB Lyon 1 - INRIA 5668

and by

Laboratoire d'Informatique de Paris 6, equipe PEQUAN,
UPMC Universite Paris 06 - CNRS - UMR 7606 - LIP6, Paris, France,

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


#include <unistd.h> /* execve, fork, daemon */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <gmp.h>
#include <mpfr.h>
#include "expression.h"
#include "external.h"
#include "plot.h"
#include "infnorm.h"
#include "general.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define READ_BUFFER_SIZE 2048

extern int fileNumber;

int bashExecute(char *command) {
  int i;
  fflush(NULL);
  i = system(command);
  fflush(NULL);
  return WEXITSTATUS(i);
}

char *evaluateStringAsBashCommand(char *command, char *input) {
  char *res;
  int okay, errorOnInput;
  int exitcode;
  int pipesToBash[2];
  int pipesFromBash[2];
  pid_t pid;
  int childStatus;
  char readBuffer[READ_BUFFER_SIZE];
  int readLen;
  char *buf;
  int len;
  int i;

  if ((command == NULL) || (strlen(command) == 0)) {
    printMessage(1,"Warning in bashevaluate: no command provided\n");
    return NULL;
  }

  res = NULL;
  okay = 0;
  fflush(NULL);
  
  // Create two unnamed pipe
  if ((input != NULL) && (pipe(pipesToBash) == -1)) {
    // Error creating the pipe
    printMessage(1,"Warning in bashevaluate: error while creating a pipe");
  } else {
    if (pipe(pipesFromBash) == -1) {
      // Error creating the pipe
      printMessage(1, "Warning in bashevaluate: error while creating a pipe");
    } else {
      // Fork
      //
      // Flush before forking
      //
      fflush(NULL);
      if ((pid = fork()) == -1) {
	// Error forking
	printMessage(1, "Warning in bashevaluate: error while forking");
      } else {
	// Fork worked
	if (pid == 0) {
	  // I am the child
	  //
	  // Close the unneeded ends of the pipes.
	  //
	  if (input != NULL) close(pipesToBash[1]);
	  close(pipesFromBash[0]);
	  
	  // Connect my input and output to the pipe
	  //
	  if (input != NULL) {
	    if (dup2(pipesToBash[0],0) == -1) {
	      _exit(1);
	    }
	  }
	  if (dup2(pipesFromBash[1],1) == -1) {
	    _exit(1);
	  }
	  
	  // Execute bash
	  //
	  fflush(NULL);
	  execlp("sh","sh","-c",command,(char *) NULL);
	  fflush(NULL);

	  _exit(1);
	} else {
	  // I am the father
	  //
	  // Close the unneeded ends of the pipes.
	  //
	  if (input != NULL) close(pipesToBash[0]);
	  close(pipesFromBash[1]);
	  
	  // Do my job
	  //
	  errorOnInput = 0;
	  if (input != NULL) {
	    if (write(pipesToBash[1],input,
		      strlen(input) * sizeof(char)) == -1) {
	      printMessage(1,"Warning in bashevaluate: unable to write to bash");
	      errorOnInput = 1;
	    }
	    close(pipesToBash[1]);
	  }

	  fflush(NULL);

	  if (!errorOnInput) {
	    do {
	      readLen = read(pipesFromBash[0],readBuffer,READ_BUFFER_SIZE);
	      if (readLen > 0) {
		if (res == NULL) {
		  res = safeCalloc(readLen + 1, sizeof(char));
		  buf = res;
		} else {
		  len = strlen(res);
		  buf = safeCalloc(len + readLen + 1, sizeof(char));
		  strcpy(buf,res);
		  free(res);
		  res = buf;
		  buf += len;
		}
		for (i=0;i<readLen;i++) {
		  *buf = (readBuffer[i] == '\0') ? '?' : readBuffer[i];
		  buf++;
		}
	      }
	    } while (readLen == READ_BUFFER_SIZE);

	    // Wait for my child to exit
	    wait(&childStatus);
	    
	    // Read the rest of the pipe if it filled up again after 
	    // having been emptied already.
	    do {
	      readLen = read(pipesFromBash[0],readBuffer,READ_BUFFER_SIZE);
	      if (readLen > 0) {
		if (res == NULL) {
		  res = safeCalloc(readLen + 1, sizeof(char));
		  buf = res;
		} else {
		  len = strlen(res);
		  buf = safeCalloc(len + readLen + 1, sizeof(char));
		  strcpy(buf,res);
		  free(res);
		  res = buf;
		  buf += len;
		}
		for (i=0;i<readLen;i++) {
		  *buf = (readBuffer[i] == '\0') ? '?' : readBuffer[i];
		  buf++;
		}
	      }
	    } while (readLen == READ_BUFFER_SIZE);

	    if (WEXITSTATUS(childStatus) != 0) {
	      printMessage(1, "Warning in bashevaluate: the exit code of the child process is %d.\n", WEXITSTATUS(childStatus));
	    } else {
	      printMessage(2, "Information in bashevaluate: the exit code of the child process is %d.\n", WEXITSTATUS(childStatus));
	    }

	    close(pipesFromBash[0]);

	    okay = 1;
	    if (res == NULL) {
	      res = safeCalloc(2, sizeof(char));
	    }
	    len = strlen(res);
	    if (len >= 1) {
	      if (res[len-1] == '\n') res[len-1] = '\0';
	    }
	  }
	}
      }
    }
  }

  if (!okay) {
    if (res != NULL) free(res);
    res = NULL;
  }

  fflush(NULL);

  return res;
}


void externalPlot(char *library, mpfr_t a, mpfr_t b, mp_prec_t samplingPrecision, int random, node *func, int mode, mp_prec_t prec, char *name, int type) {
  void *descr;
  void  (*myFunction)(mpfr_t, mpfr_t);
  char *error;
  mpfr_t x_h,x,y,temp,perturb,ulp,min_value;
  double xd, yd;
  FILE *file;
  gmp_randstate_t state;
  char *gplotname;
  char *dataname;
  char *outputname;


  gmp_randinit_default (state);

  if(samplingPrecision > prec) {
    sollyaFprintf(stderr, "Error: you must use a sampling precision lower than the current precision\n");
    return;
  }

  descr = dlopen(library, RTLD_NOW);
  if (descr==NULL) {
    sollyaFprintf(stderr, "Error: the given library (%s) is not available (%s)!\n",library,dlerror());
    return;
  }

  dlerror(); /* Clear any existing error */
  myFunction = (void (*)(mpfr_t, mpfr_t)) dlsym(descr, "f");
  if ((error = dlerror()) != NULL) {
    sollyaFprintf(stderr, "Error: the function f cannot be found in library %s (%s)\n",library,error);
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

  
  /* Beginning of the interesting part of the code */
  file = fopen(gplotname, "w");
  if (file == NULL) {
    sollyaFprintf(stderr,"Error: the file %s requested by plot could not be opened for writing: ",gplotname);
    sollyaFprintf(stderr,"\"%s\".\n",strerror(errno));
    return;
  }
  sollyaFprintf(file, "# Gnuplot script generated by %s\n",PACKAGE_NAME);
  if ((type==PLOTPOSTSCRIPT) || (type==PLOTPOSTSCRIPTFILE)) sollyaFprintf(file,"set terminal postscript eps color\nset out \"%s\"\n",outputname);
  sollyaFprintf(file, "set xrange [%1.50e:%1.50e]\n", mpfr_get_d(a, GMP_RNDD),mpfr_get_d(b, GMP_RNDU));
  sollyaFprintf(file, "plot \"%s\" using 1:2 with dots t \"\"\n",dataname);
  fclose(file);

  file = fopen(dataname, "w");
  if (file == NULL) {
    sollyaFprintf(stderr,"Error: the file %s requested by plot could not be opened for writing: ",dataname);
    sollyaFprintf(stderr,"\"%s\".\n",strerror(errno));
    return;
  }

  mpfr_init2(x_h,samplingPrecision);
  mpfr_init2(perturb, prec);
  mpfr_init2(x,prec);
  mpfr_init2(y,prec);
  mpfr_init2(temp,prec);
  mpfr_init2(ulp,prec);
  mpfr_init2(min_value,53);

  mpfr_sub(min_value, b, a, GMP_RNDN);
  mpfr_div_2ui(min_value, min_value, 12, GMP_RNDN);

  mpfr_set(x_h,a,GMP_RNDD);
  
  while(mpfr_less_p(x_h,b)) {
    mpfr_set(x, x_h, GMP_RNDN); // exact
    
    if (mpfr_zero_p(x_h)) {
      mpfr_set(x_h, min_value, GMP_RNDU);
    }
    else {
      if (mpfr_cmpabs(x_h, min_value) < 0) mpfr_set_d(x_h, 0., GMP_RNDN);
      else mpfr_nextabove(x_h);
    }

    if(random) {
      mpfr_sub(ulp, x_h, x, GMP_RNDN);
      mpfr_urandomb(perturb, state);
      mpfr_mul(perturb, perturb, ulp, GMP_RNDN);
      mpfr_add(x, x, perturb, GMP_RNDN);
    }

    (*myFunction)(temp,x);
    evaluateFaithful(y, func, x,prec);
    mpfr_sub(temp, temp, y, GMP_RNDN);
    if(mode==RELATIVE) mpfr_div(temp, temp, y, GMP_RNDN);
    xd =  mpfr_get_d(x, GMP_RNDN);
    if (xd >= MAX_VALUE_GNUPLOT) xd = MAX_VALUE_GNUPLOT;
    if (xd <= -MAX_VALUE_GNUPLOT) xd = -MAX_VALUE_GNUPLOT;
    sollyaFprintf(file, "%1.50e",xd);
    if (!mpfr_number_p(temp)) {
      if (verbosity >= 2) {
	changeToWarningMode();
	sollyaPrintf("Information: function undefined or not evaluable in point %s = ",variablename);
	printValue(&x);
	sollyaPrintf("\nThis point will not be plotted.\n");
	restoreMode();
      }
    }
    yd = mpfr_get_d(temp, GMP_RNDN);
    if (yd >= MAX_VALUE_GNUPLOT) yd = MAX_VALUE_GNUPLOT;
    if (yd <= -MAX_VALUE_GNUPLOT) yd = -MAX_VALUE_GNUPLOT;
    sollyaFprintf(file, "\t%1.50e\n", yd);
  }

  fclose(file);
 
  /* End of the interesting part.... */

  dlclose(descr);
  mpfr_clear(x);
  mpfr_clear(y);
  mpfr_clear(x_h);
  mpfr_clear(temp);
  mpfr_clear(perturb);
  mpfr_clear(ulp);
  mpfr_clear(min_value);

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
