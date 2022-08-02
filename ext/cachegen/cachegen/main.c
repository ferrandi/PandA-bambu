#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "design/cam.def"
#include "design/encoder.def"
#include "design/cache_top.def"
#include "design/directcache_top.def"
#include "design/twowaydctcache_top.def"

#define CAM16X5FILE "design/cam16x5.vlg"
#define CACHE_TOPFILE "design/cache_top.vlg"
#define DIRECTCACHE_TOPFILE "design/directcache_top.vlg"
#define TWOWAYDCTCACHE_TOPFILE "design/twowaydctcache_top.vlg"
#define QUARTUSCACHEFILE "design/cache_top.quartus"
#define QUARTUSDCACHEFILE "design/dcache_top.quartus"
#define QUARTUSCAMFILE "design/cam.quartus"
#define DEF "define"
#define UNDEF "undef"

#define BASECAMWIDTH 5
#define BASECAMDEPTH 16
#define MAX_BUF 2048
#define NUMOPTIONS 4
#define NUMBITS(x) ((int)(ceil(log((float)x)/log(2))))


typedef enum {CACHEHIT=0, EXPAND=1, INT=2, EXPANDWRITE=3} Option;

char cmdoptions[NUMOPTIONS][2]={
		"h",
		"e",
		"i",
		"w"
		};

void fatalerror(char *msg){
	printf("\nError: %s\n",msg);
	exit(-1);
}

void Usage() {
		printf("\nCAM GENERATION\n");
		printf("Usage: cachegen -c <depth> <width> <output location>\n");
		printf("ASSOCIATIVE CACHE GENERATION\n");
		printf("Usage: cachegen -a [-hei] <cache depth> <address width> <data width> <output location>\n");
		printf("DIRECT MAPPED CACHE GENERATION\n");
		printf("Usage: cachegen -d [-hei] <cache depth> <address width> <data width> <output location>\n");
		printf("TWO WAY SET ASSOCIATIVE CACHE GENERATION\n");
		printf("Usage: cachegen -t [-heiw] <cache depth> <address width> <data width> <output location>\n");
		printf("\n\tOPTIONS:\n");
		printf("\t-h Cache Hit - outputs a signal that indicates when a cache hit occurs\n");
		printf("\t-e Expand Read - Adds an extra cycle to the read operation improving fmax\n");
		printf("\t-i Intercept - passes read write signals to slave only when necessary\n");
		printf("\t-w Expand Write - Adds an extra cycle to the write operation improving fmax\n");
		exit(-1);
}

char * catFilenametoDir(char* dir, char *filename){
	if (dir[strlen(dir)-1]!='/')
		strcat(dir,"/");
	return strcat(dir,filename);
}

/** Copies src into current position in destination, DOES NOT CLOSE FILE **/
void copyFileInto(FILE * dst, char *srcfilename){
	FILE *src;
	char buf[MAX_BUF];
	
	if (!(src=fopen(srcfilename,"r")))
		fatalerror("failed to open source file for copying");
	while(fgets(buf,MAX_BUF,src)){
		if (!(fputs(buf,dst)>=0)){
			fatalerror("failed writing to destination during copying");		
			}
		}
}

void copyFile(char *target, char * src){
	FILE *f;

	if (!(f=fopen(target,"w")))
		fatalerror("failed to open target file for file copying");
	copyFileInto(f,src);

}

/*********  Return 2^x for x>=0  ***********/
int twoRaisedto(int x){
	int y,i;
	y=1;
	for (i=0; i<x; i++)
		y*=2;
	return y;
}

void encodergen(int numinputs,char *dir){
	int i,j,k,numouts,numins;
	FILE *f;

	if (!(f=fopen(catFilenametoDir(dir,"encoder.v"),"w")))
		fatalerror("failed to open file for encoder generation");

	numouts=NUMBITS(numinputs);
	numins=twoRaisedto(numouts);

	printf("Generating %d:%d encoder ... ",numins,numouts);
	
	fprintf(f,ENCMODULE);
	fprintf(f,ENCINPUT,numinputs);
	fprintf(f,ENCOUTPUT,numouts);
	for (i=0; i<numouts; i++){
		fprintf(f,ENCASSIGN,i);
		for (j=twoRaisedto(i); j<numinputs; j=j+twoRaisedto(i+1)){
			for (k=0; k<twoRaisedto(i); k++){
					fprintf(f,ENCINVAR,j+k);
					if ((j+k)<numinputs-1) fprintf(f,",");
					}
			}
		fprintf(f,"};\n");
		}
		
	fprintf(f,ENCEND);
	printf("done\n");
	fclose(f);
}

void camgen(int depth, int width, char *dir) {
	int i,j,widthfactor, depthfactor,n;
	FILE * f;
	char wire[MAX_BUF],tmp[10];

	widthfactor=ceil(((double)width)/((double)BASECAMWIDTH));
	depthfactor=ceil(((double)depth)/((double)BASECAMDEPTH));

	if (!(f=fopen(catFilenametoDir(dir,"cam.v"),"w")))
		fatalerror("failed to open file for cam generation");

	n=NUMBITS(depth);
	
	printf("Generating %dx%d cam ... ",depth,width);
	/*****   Generate Cam Header ******/
	fprintf(f,CAMMODULE);
	fprintf(f,CAMPARAM);
	fprintf(f,CAMINPUT1);
	fprintf(f,CAMINPUT2,width-1);
	fprintf(f,CAMINPUT3,NUMBITS(depth)-1);
	fprintf(f,CAMINPUT4);
	fprintf(f,CAMOUTPUT1,depth-1);
	fprintf(f,CAMWIRES);
	fprintf(f,"p0");
	for (i=1; i<widthfactor*depthfactor; i++){
		sprintf(tmp,",p%d",i);
		fprintf(f,"%s",tmp);
		}
	
	/*****   Generate Cam instantiation ******/
	for (i=0; i<depthfactor; i++){
		char wren[MAX_BUF],wrpos[12];
		wren[0]='\0';
		for (j=NUMBITS(BASECAMDEPTH); j<n; j++){
			strcat(wren,"&");
			if ((i&(int)pow(2,j-NUMBITS(BASECAMDEPTH)))==0)
				strcat(wren,"~");
			sprintf(wrpos,"wrpos[%d]",j);
			strcat(wren,wrpos);
			}
		for (j=0; j<widthfactor; j++){
			int k=i*widthfactor+j;
			fprintf(f,CAMINST1,k,j*BASECAMWIDTH+4,j*BASECAMWIDTH);
			fprintf(f,CAMINST2,wren,k,k);
			}
		}
		
	fprintf(f,CAMOUT,wire);

	/***** Generate Cam Output ******/
	wire[0]='\0';
	sprintf(wire,"p%d",depthfactor*widthfactor-1);
	for (i=depthfactor*widthfactor-2; i>=0; i--){
		char buf[10];
		if (((i+1)%widthfactor)==0)
			strcat(wire,",");
		else
			strcat(wire,"&");
		sprintf(buf,"p%d",i);
		strcat(wire,buf);
		fprintf(f,"%s",wire);
		wire[0]='\0';
		}
		
	fprintf(f,CAMEND);
	
	copyFileInto(f,CAM16X5FILE);
	fclose(f);
	printf("done\n");
}

void asscachegen(int w, int d, int data, int intercept, int cachehit, int threecycle, char *dir){
	int i;
	FILE *f;
	if (!(f=fopen(catFilenametoDir(dir,"cache_top.v"),"w")))
		fatalerror("failed to open file for cam generation");

	printf("Generating %dx%d associative cache ... ",d,data);

	fprintf(f,CACHEINT,(intercept) ? DEF : UNDEF);
	fprintf(f,CACHECHIT,(cachehit) ? DEF : UNDEF);
	fprintf(f,CACHETWO,(!threecycle) ? DEF : UNDEF);

	fprintf(f,CACHEMOD);
	fprintf(f,CACHECHITOUT);
	fprintf(f,CACHESLAVE);
	fprintf(f,CACHEDEBUGOUTS);
	fprintf(f,CACHEWIDTHS,w,data);
	fprintf(f,CACHEDEPTH,d,NUMBITS(d));
	
	copyFileInto(f,CACHE_TOPFILE);
	fclose(f);
	printf("done\n");
}

void directcachegen(int w, int d, int data, int intercept, int cachehit, int twocycle, char *dir){
	int i;
	FILE *f;
	if (!(f=fopen(catFilenametoDir(dir,"cache_top.v"),"w")))
		fatalerror("failed to open file for cam generation");

	printf("Generating %dx%d direct-mapped cache ... ",d,data);

	fprintf(f,DCACHEINT,(intercept) ? DEF : UNDEF);
	fprintf(f,DCACHECHIT,(cachehit) ? DEF : UNDEF);
	fprintf(f,DCACHEONE,(!twocycle) ? DEF : UNDEF);

	fprintf(f,DCACHEMOD);
	fprintf(f,DCACHECHITOUT);
	fprintf(f,DCACHESLAVE);
	fprintf(f,DCACHEWIDTHS,w,data);
	fprintf(f,DCACHEDEPTH,d,NUMBITS(d));
	
	copyFileInto(f,DIRECTCACHE_TOPFILE);
	fclose(f);
	printf("done\n");
}

void twowaydctcachegen(int w, int d, int data, int intercept, int cachehit, int twocycle, int twocyclewrite, char *dir){
	int i;
	FILE *f;
	if (!(f=fopen(catFilenametoDir(dir,"cache_top.v"),"w")))
		fatalerror("failed to open file for cam generation");

	printf("Generating %dx%d two way set associative cache ... ",d,data);

	fprintf(f,TWODCACHEINT,(intercept) ? DEF : UNDEF);
	fprintf(f,TWODCACHECHIT,(cachehit) ? DEF : UNDEF);
	fprintf(f,TWODCACHEONE,(!twocycle) ? DEF : UNDEF);
	fprintf(f,TWODCACHEONEW,(!twocyclewrite) ? DEF : UNDEF);

	fprintf(f,TWODCACHEMOD);
	fprintf(f,TWODCACHECHITOUT);
	fprintf(f,TWODCACHESLAVE);
	fprintf(f,TWODCACHEWIDTHS,w,data);
	fprintf(f,TWODCACHEDEPTH,d,NUMBITS(d));
	
	copyFileInto(f,TWOWAYDCTCACHE_TOPFILE);
	fclose(f);
	printf("done\n");
}


int* getOptions(char options[],int * o){
	int i;
	
	if (!options || options[0]!='-') Usage();
	
	o[CACHEHIT]=(strstr(options,cmdoptions[CACHEHIT])!=NULL);
	o[EXPAND]=(strstr(options,cmdoptions[EXPAND])!=NULL);
	o[INT]=(strstr(options,cmdoptions[INT])!=NULL);
	o[EXPANDWRITE]=(strstr(options,cmdoptions[EXPANDWRITE])!=NULL);
	return o;
}

int main(int argc, char *argv[]) {
	int i,depth,width,datawidth;
	int *o=NULL;
	char dir[MAX_BUF];
	char mkdir[MAX_BUF];
	
	if (argc<5 || argc>7 || argv[1][0]!='-') Usage();

	strcpy(dir,argv[argc-1]);
	strcpy(mkdir,"mkdir ");
	strcat(mkdir,dir);
	printf("Creating directory %s\n",dir);
	system(mkdir);

	o=(int *)malloc(NUMOPTIONS*sizeof(int));
	for (i=0; i<NUMOPTIONS; i++)
		o[i]=0;

	switch (argv[1][1]){
		case 'c':
			if (argc!=5) Usage();
			width=atoi(argv[argc-2]);
			depth=atoi(argv[argc-3]);
			camgen(depth,width,dir); 
			copyFile(catFilenametoDir(dir,"cam.quartus"),QUARTUSCAMFILE);
			break;
		case 'd':
			if (argc<6) Usage();
			datawidth=atoi(argv[argc-2]);
			width=atoi(argv[argc-3]);
			depth=atoi(argv[argc-4]);
			if (argc==7)
				o=getOptions(argv[2],o);
			directcachegen(width,depth,datawidth,o[INT],o[CACHEHIT],o[EXPAND],dir);
			strcpy(dir,argv[argc-1]);
			copyFile(catFilenametoDir(dir,"cache_top.quartus"),QUARTUSDCACHEFILE);
			break;
		case 'a':
			if (argc<6) Usage();
			datawidth=atoi(argv[argc-2]);
			width=atoi(argv[argc-3]);
			depth=atoi(argv[argc-4]);
			if (argc==7)
				o=getOptions(argv[2],o);
			asscachegen(width,depth,datawidth,o[INT],o[CACHEHIT],o[EXPAND],dir);
			strcpy(dir,argv[argc-1]);
			encodergen(depth,dir);
			strcpy(dir,argv[argc-1]);
			camgen(depth,width,dir);
			strcpy(dir,argv[argc-1]);
			copyFile(catFilenametoDir(dir,"cache_top.quartus"),QUARTUSCACHEFILE);
			break;
		case 't':
			if (argc<6) Usage();
			datawidth=atoi(argv[argc-2]);
			width=atoi(argv[argc-3]);
			depth=atoi(argv[argc-4]);
			if (argc==7)
				o=getOptions(argv[2],o);
			twowaydctcachegen(width,depth,datawidth,o[INT],o[CACHEHIT],o[EXPAND],o[EXPANDWRITE],dir);
			strcpy(dir,argv[argc-1]);
			copyFile(catFilenametoDir(dir,"cache_top.quartus"),QUARTUSDCACHEFILE);
			break;
		}
			
	free(o);
	return 0;
}
