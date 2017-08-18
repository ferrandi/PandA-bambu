/* pgm file IO headerfile ------ mypgm.h */

#ifndef MYPGM_H
#define MYPGM_H

/* Constant declaration */
#define MAX_IMAGESIZE  256
#define MAX_BRIGHTNESS  255 /* Maximum gray level */
#define GRAYLEVEL       256 /* No. of gray levels */
#define MAX_FILENAME    256 /* Filename length limit */
#define MAX_BUFFERSIZE  256

/* Global constant declaration */
/* Image storage arrays */
/* Prototype declaration of functions */
void load_image_data(unsigned char *image1, unsigned int * x_size1, unsigned int * y_size1 );
void save_image_data(unsigned char * image2, unsigned int x_size2, unsigned int y_size2);
void load_image_file(char *filename, unsigned char **image1,
                     unsigned int * x_size1, unsigned int * y_size1);
void save_image_file(char *); /* image output*/
#endif /* MYPGM_H */
