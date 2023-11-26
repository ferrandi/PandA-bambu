#ifndef H_READ_FILE_H
#define H_READ_FILE_H

#include <stdlib.h>
#include <stdio.h>

template <int DIM1, int DIM2, typename T> void read_file(char *filename, T outBuf[DIM1][DIM2] )
{
	int i, j;

	float data;
	char str[300];

    FILE *fid = fopen( filename, "r");
    if (fid==NULL)
    	fprintf(stderr, "error in opening input file %s\n", filename);

    for (i = 0; i < DIM1; i++)
    {
		for (j = 0; j < DIM2; j++)
		{
		  fscanf(fid,"%s", str);
		  data = atof(str);
		  outBuf[i][j] = (T) data;
		  //fprintf(stderr,"%f ", (float) data);
		}
		fscanf(fid,"\n");
		//fprintf(stderr,"\n");
	}

    fclose(fid);

	return;
}



#endif
