#include "mypgm.h"

#include <stdio.h>
#include <stdlib.h>


/* Main body of functions */

void load_image_data(unsigned char *image1, unsigned int * x_size1, unsigned int * y_size1 )
/* Input of header & body information of pgm file */
/* for image1[ ][ ]，x_size1，y_size1 */
{
  char file_name[MAX_FILENAME];
  char buffer[MAX_BUFFERSIZE];
  FILE *fp; /* File pointer */
  int max_gray; /* Maximum gray level */
  int x, y; /* Loop variable */

  /* Input file open */
  printf("\n-----------------------------------------------------\n");
  printf("Monochromatic image file input routine \n");
  printf("-----------------------------------------------------\n\n");
  printf("     Only pgm binary file is acceptable\n\n");
  printf("Name of input image file? (*.pgm) : ");
  scanf("%s", file_name);
  fp = fopen(file_name, "rb");
  if (NULL == fp) {
    printf("     The file doesn't exist!\n\n");
    exit(1);
  }
  /* Check of file-type ---P5 */
  fgets(buffer, MAX_BUFFERSIZE, fp);
  if (buffer[0] != 'P' || buffer[1] != '5') {
    printf("     Mistaken file format, not P5!\n\n");
    exit(1);
  }
  /* input of x_size1, y_size1 */
  *x_size1 = 0;
  *y_size1 = 0;
  while (*x_size1 == 0 || *y_size1 == 0) {
    fgets(buffer, MAX_BUFFERSIZE, fp);
    if (buffer[0] != '#') {
      sscanf(buffer, "%d %d", x_size1, y_size1);
    }
  }
  /* input of max_gray */
  max_gray = 0;
  while (max_gray == 0) {
    fgets(buffer, MAX_BUFFERSIZE, fp);
    if (buffer[0] != '#') {
      sscanf(buffer, "%d", &max_gray);
    }
  }
  /* Display of parameters */
  printf("\n     Image width = %d, Image height = %d\n", *x_size1, *y_size1);
  printf("     Maximum gray level = %d\n\n",max_gray);
  if (*x_size1 > MAX_IMAGESIZE || *y_size1 > MAX_IMAGESIZE) {
    printf("     Image size exceeds %d x %d\n\n", 
	   MAX_IMAGESIZE, MAX_IMAGESIZE);
    printf("     Please use smaller images!\n\n");
    exit(1);
  }
  if (max_gray != MAX_BRIGHTNESS) {
    printf("     Invalid value of maximum gray level!\n\n");
    exit(1);
  }
  /* Input of image data*/
  for (y = 0; y < *y_size1; y++) {
    for (x = 0; x < *x_size1; x++) {
      *(image1 + y + x * MAX_IMAGESIZE) = (unsigned char)fgetc(fp);
    }
  }
  printf("-----Image data input OK-----\n\n");
  printf("-----------------------------------------------------\n\n");
  fclose(fp);
}

void save_image_data(unsigned char * image2, unsigned int x_size2, unsigned int y_size2)
/* Output of image2[ ][ ], x_size2, y_size2 in pgm format*/
     
{
  char file_name[MAX_FILENAME];
  FILE *fp; /* File pointer */
  int x, y; /* Loop variable */
  
  /* Output file open */
  printf("-----------------------------------------------------\n");
  printf("Monochromatic image file output routine\n");
  printf("-----------------------------------------------------\n\n");
  printf("Name of output image file? (*.pgm) : ");
  scanf("%s",file_name);
  fp = fopen(file_name, "wb");
  /* output of pgm file header information */
  fputs("P5\n", fp);
  fputs("# Created by Image Processing\n", fp);
  fprintf(fp, "%d %d\n", x_size2, y_size2);
  fprintf(fp, "%d\n", MAX_BRIGHTNESS);
  /* Output of image data */
  for (y = 0; y < y_size2; y++) {
    for (x = 0; x < x_size2; x++) {
      fputc(*(image2 + y + x * MAX_IMAGESIZE), fp);
    }
  }
  printf("\n-----Image data output OK-----\n\n");
  printf("-----------------------------------------------------\n\n");
  fclose(fp);
}

void load_image_file(char *filename, unsigned char **image1,
                     unsigned int * x_size1, unsigned int * y_size1)
/* Input of header & body information of pgm file */
/* for image1[ ][ ]，x_size1，y_size1 */
{
#if 0
  char buffer[MAX_BUFFERSIZE];
  FILE *fp; /* File pointer */
  int max_gray; /* Maximum gray level */
  int x, y; /* Loop variable */
  
  /* Input file open */
  fp = fopen(filename, "rb");
  if (NULL == fp) {
    printf("     The file doesn't exist!\n\n");
    exit(1);
  }
  /* Check of file-type ---P5 */
  fgets(buffer, MAX_BUFFERSIZE, fp);
  if (buffer[0] != 'P' || buffer[1] != '5') {
    printf("     Mistaken file format, not P5!\n\n");
    exit(1);
  }
  /* input of x_size1, y_size1 */
  *x_size1 = 0;
  *y_size1 = 0;
  while (x_size1 == 0 || y_size1 == 0) {
    fgets(buffer, MAX_BUFFERSIZE, fp);
    if (buffer[0] != '#') {
      sscanf(buffer, "%d %d", x_size1, y_size1);
    }
  }
  /* input of max_gray */
  max_gray = 0;
  while (max_gray == 0) {
    fgets(buffer, MAX_BUFFERSIZE, fp);
    if (buffer[0] != '#') {
      sscanf(buffer, "%d", &max_gray);
    }
  }
  if (x_size1 > MAX_IMAGESIZE || y_size1 > MAX_IMAGESIZE) {
    printf("     Image size exceeds %d x %d\n\n", 
	   MAX_IMAGESIZE, MAX_IMAGESIZE);
    printf("     Please use smaller images!\n\n");
    exit(1);
  }
  if (max_gray != MAX_BRIGHTNESS) {
    printf("     Invalid value of maximum gray level!\n\n");
    exit(1);
  }
  /* Input of image data*/
  for (y = 0; y < y_size1; y++) {
    for (x = 0; x < x_size1; x++) {
      image1[y][x] = (unsigned char)fgetc(fp);
    }
  }
  fclose(fp);
#endif
}

void save_image_file(char *filename)
/* Output of image2[ ][ ], x_size2, y_size2 */ 
/* into pgm file with header & body information */
{
#if 0
  FILE *fp; /* File pointer */
  int x, y; /* Loop variable */
  
  fp = fopen(filename, "wb");
  /* output of pgm file header information */
  fputs("P5\n", fp);
  fputs("# Created by Image Processing\n", fp);
  fprintf(fp, "%d %d\n", x_size2, y_size2);
  fprintf(fp, "%d\n", MAX_BRIGHTNESS);
  /* Output of image data */
  for (y = 0; y < y_size2; y++) {
    for (x = 0; x < x_size2; x++) {
      fputc(image2[y][x], fp);
    }
  }
  fclose(fp);
#endif
}
