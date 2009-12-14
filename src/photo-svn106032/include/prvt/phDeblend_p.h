
/* Definitions that are local to the Deblending code */


/* Stuff to allocate arrays as arrays of pointers */
extern int **make2darrayi(int nrows, int ncols);
extern short **make2darrays(int nrows, int ncols);
extern float **make2darrayf(int nrows, int ncols);
extern void clear2darrayf(float **p, int nrows, int ncols);
extern void free2darrays(short **p, int nrows);
extern void free2darrayi(int **p, int nrows);
extern void free2darrayf(float **p, int nrows);
extern int ***make3darrayi(int nx, int ny, int nz);
extern float ***make3darrayf(int nx, int ny, int nz);
extern void clear3darrayf(float ***p, int nx, int ny, int nz);
extern void free3darrayi(int ***p, int nx, int ny);
extern void free3darrayf(float ***p, int nx, int ny);
extern void print3darrayi(int ***p, int nx, int ny, int nz);
extern void print3darrayf(float ***p, int nx, int ny, int nz);


/*********************************************************************/
/*                                                                   */
/*      Include file for Deblending/Photometry Software              */
/*      Written by: Istvan Szapudi, February 1990                    */
/*      Modified by: M. Postman, June 1990                           */
/*                                                                   */
/*********************************************************************/
#define MAINBUFFERSIZE 256
#define YES 1
#define NO 0
#define MODEPICTURE  1     /* modes for postscript output */
#define MODECONTOUR  2 
#define MINWTVAL 0.00001   /* Pixel intensity for sub-threshold pixels */

typedef struct GlbRunType  /* for global runs           */
{
  int LHS;                 /* leftmost pixel            */
  int RHS;                 /* rightmost pixel           */
  int Scan;                /* scan #                    */
} GLBRUNTYPE;


typedef struct TreeType
{
  struct TreeType * Parent; /* pointer to its parent node       */
  int Children;             /* number of nodes pointing to here */
  int N;                    /* number of corresponding runs     */
  GLBRUNTYPE * Arr;         /* pointer to the actual data       */
  int Level;                /* threshold level in the tree      */
} TREETYPE;


typedef struct ObjectType      /* linear list of objs in MainBuffer      */
{
  int BFLAG;                   /* Deblender flag                         */
  TREETYPE *PID;               /* Parent ID Number                       */
  int X, Y;                    /* coordinates of peak pixel              */
  float XC, YC;                 /* Centroided coordinates of object       */
  int XMIN, XMAX;              /* Min, Max X extent of object            */
  int YMIN, YMAX;              /* Min, Max Y extent of object            */
  int EDGE;                    /* Edge Warning Flag                      */
  float XX;                     /* flux weighted moments                  */
  float XY;
  float YY;
  float UXX;                    /* unweighted moments                     */
  float UXY;
  float UYY;
  float B;                      /* rough Gaussian parameter               */ 
  float IMax;                   /* maximum of pixelvalue                  */
  float m;                      /* Total luminosity  - Gaussian weighting */
  float AREA;                   /* Total Area - Gaussian weighting        */
  float DTHRESH;                /* Detection Threshold                    */
  float ECC;                    /* Eccentricity                           */
  float AXR, EAXR;              /* Axial Ratio and Error                  */
  float AX, BX;                 /* Major axis (AX) and Minor Axis (BX)    */
  float PA, EPA;                /* Position Angle and Error               */
  struct ObjectType * Next;    /* pointer to next element                */
  TREETYPE * TreePtr;          /* pointer to the corresponding tree leaf */
} OBJECTTYPE;

struct shapestr {              /* Object Shape Parameter Data            */
   float pa;                    /* Position Angle                         */
   float epa;                   /* Error in Position Angle                */
   float axr;                   /* Axial Ratio = a/b                      */
   float eaxr;                  /* Error in Axial Ratio                   */
   float a, b;                  /* Major and Minor axis lengths           */
   float xx, yy, xy;            /* Moments                                */
};
 
extern float MainBuffer[MAINBUFFERSIZE][MAINBUFFERSIZE]; 
extern float WorkBuffer[MAINBUFFERSIZE][MAINBUFFERSIZE]; 
extern OBJECTTYPE *LastObjPtr;         /* last elem. of the linear list      */

extern int ArraySizeX;
extern int ArraySizeY;
extern int ObjectNum;
extern int WeightAreas;

/* modul dphio.c */
void PrintBuffer(int Mode);
FILE * MyOpen( char * Name,char * Access );
void PsOut( char * OutFileName, int Mode );
void PrintNode( TREETYPE * TreePtr );
void PrintTree( void );
void BlendStatus( void );
void TestAreas( void );
void LoadBuffer(char * InFileName);
int PassParams(float      ** ImagePtr,
                     int * ArraySizeXPtr,
		     int * ArraySizeYPtr,
                     int * ImageSizeXPtr,
                     int * ImageSizeYPtr,
               float      * SkyBackgroundPtr,
               short int * MinPixelPtr,
                     int * ThresholdArrPtr,
               short int * LevelNumPtr,
               short int * MaxObjectPtr,
                     int * IerrorPtr);

/* modul dphdet */
void SmoothToWork( int N);
void IntObjs( float *** FPAPtr);
void CenMoments( void );
void InitObjectParams( float ** Params, int * SizeAPMPtr);
void FillObjectParams( float ** Params, int * SizeAPMPtr, float *** FPAPtr);

/* modul dphcon.c */
int GetNextComp(int * Size, GLBRUNTYPE ** GlbRunPtr);
int Connect( void );
void Sort(int, GLBRUNTYPE *);

/* modul dphtree.c */
int BuildTree( void );
void FreeAll( void );



