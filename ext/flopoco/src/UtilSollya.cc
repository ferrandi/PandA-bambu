#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SOLLYA

#include "UtilSollya.hh"
sollya_chain_t makeIntPtrChainToFromBy(int m, int n, int k) {
  int i,j;
  int *elem;
  sollya_chain_t c;
  
  c = NULL;
  i=m-(n-1)*k;
  for(j=0;j<n;j++) {
    elem = (int *) malloc(sizeof(int));
    *elem = i;
    c = addElement(c,elem);
    i=i+k;
  }

  return c;
}

sollya_chain_t makeIntPtrChainFromArray(int m, int *a) {
  int j;
  int *elem;
  sollya_chain_t c;
  
  c = NULL;
  
  for(j=0;j<m;j++) {
    elem = (int *) malloc(sizeof(int));
    *elem = a[j];
    c = addElement(c,elem);
   
  }

  return c;
}

void removeTrailingZeros(char *outbuf, char *inbuf) {
  char *temp, *temp2, *temp3;

  temp = inbuf; temp2 = outbuf; temp3 = outbuf;
  while ((temp != NULL) && (*temp != '\0')) {
    *temp2 = *temp;
    if (*temp2 != '0') {
      temp3 = temp2;
    }
    temp2++;
    temp++;
  }
  temp3++;
  *temp3 = '\0';
}


char *sPrintBinary(mpfr_t x) {
  mpfr_t xx;
  int negative;
  mp_prec_t prec;
  mp_exp_t expo;
  char *raw, *formatted, *temp1, *temp2, *str3;
  char *temp3=NULL;
  char *resultStr;
  int len;

  prec = mpfr_get_prec(x);
  mpfr_init2(xx,prec);
  mpfr_abs(xx,x,GMP_RNDN);
  negative = 0;
  if (mpfr_sgn(x) < 0) negative = 1;
  raw = mpfr_get_str(NULL,&expo,2,0,xx,GMP_RNDN);
  if (raw == NULL) {
    printf("Error: unable to get a string for the given number.\n");
    return NULL;
  } else {
    formatted =(char *) safeCalloc(strlen(raw) + 3, sizeof(char));
    temp1 = raw; temp2 = formatted;
    if (negative) {
      *temp2 = '-';
      temp2++;
    }
    *temp2 = *temp1; temp2++; temp1++;
    if (*temp1 != '\0') { 
      *temp2 = '.'; 
      temp2++;
    }
    while (*temp1 != '\0') {
      *temp2 = *temp1;
      temp2++; temp1++;
    }
    str3 = (char *) safeCalloc(strlen(formatted)+2,sizeof(char));
    removeTrailingZeros(str3,formatted);
    len = strlen(str3) - 1;
    if (str3[len] == '.') {
      str3[len] = '\0';
    }
    if (!mpfr_zero_p(x)) {
      if (mpfr_number_p(x)) {
	temp3 = (char *) safeCalloc(strlen(str3)+74,sizeof(char));
	if ((((int) expo)-1) != 0) 
	  sprintf(temp3,"%s_2 * 2^(%d)",str3,((int)expo)-1);  
	else
	  sprintf(temp3,"%s_2",str3);  
      } else {
	temp3 = (char *) safeCalloc(strlen(raw) + 2,sizeof(char));
	if (negative) 
	  sprintf(temp3,"-%s",raw); 
	else 
	  sprintf(temp3,"%s",raw); 
      }
    }
    else {
      temp3 = (char *) safeCalloc(2,sizeof(char));
      sprintf(temp3,"0");
    }
    free(formatted);
    free(str3);
  }
  mpfr_free_str(raw);  
  mpfr_clear(xx);
  resultStr = (char *) safeCalloc(strlen(temp3) + 1,sizeof(char));
  sprintf(resultStr,"%s",temp3);
  free(temp3);
  return resultStr;
}


char *sPrintBinaryZ(mpfr_t x) {
  mpfr_t xx;
  int negative;
  mp_prec_t prec;
  mp_exp_t expo;
  char *raw, *formatted, *temp1, *temp2, *str3;
  char *temp3=NULL;
  char *resultStr;
  int len;

  prec = mpfr_get_prec(x);
  mpfr_init2(xx,prec);
  mpfr_abs(xx,x,GMP_RNDN);
  negative = 0;
  if (mpfr_sgn(x) < 0) negative = 1;
  raw = mpfr_get_str(NULL,&expo,2,0,xx,GMP_RNDN);
  if (raw == NULL) {
    printf("Error: unable to get a string for the given number.\n");
    return NULL;
  } else {
    formatted =(char *) safeCalloc(strlen(raw) + 3, sizeof(char));
    temp1 = raw; temp2 = formatted;
    /*if (negative) {
      *temp2 = '-';
      temp2++;
    }*/
    *temp2 = *temp1; temp2++; temp1++;
    /*if (*temp1 != '\0') { 
      *temp2 = '.'; 
      temp2++;
    }*/
    while (*temp1 != '\0') {
      *temp2 = *temp1;
      temp2++; temp1++;
    }
    str3 = (char *) safeCalloc(strlen(formatted)+2,sizeof(char));
    removeTrailingZeros(str3,formatted);
    len = strlen(str3) - 1;
    if (str3[len] == '.') {
      str3[len] = '\0';
    }
    if (!mpfr_zero_p(x)) {
      if (mpfr_number_p(x)) {
	temp3 = (char *) safeCalloc(strlen(str3)+74,sizeof(char));
	if ((((int) expo)-1) != 0) 
	  //sprintf(temp3,"%s_2 * 2^(%d)",str3,((int)expo)-1);  
	  sprintf(temp3,"%s",str3);  
	else
	  sprintf(temp3,"%s",str3);  
      } else {
	temp3 = (char *) safeCalloc(strlen(raw) + 2,sizeof(char));
	if (negative) 
	 // sprintf(temp3,"-%s",raw); 
	 sprintf(temp3,"%s",raw); 
	else 
	  sprintf(temp3,"%s",raw); 
      }
    }
    else {
      temp3 = (char *) safeCalloc(2,sizeof(char));
      sprintf(temp3,"0");
    }
    free(formatted);
    free(str3);
  }
  mpfr_free_str(raw);  
  mpfr_clear(xx);
  resultStr = (char *) safeCalloc(strlen(temp3) + 1,sizeof(char));
  sprintf(resultStr,"%s",temp3);
  free(temp3);
  return resultStr;
}
#endif //HAVE_SOLLYA

