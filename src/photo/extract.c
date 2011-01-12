/*
 * <INTRO>
 * `Profile' Extractor code for Measure objects module for SDSS
 *
 * Note that this code uses `x' to mean column, and `y' to mean row,
 * and that (as noted below) (0.5,0.5) is taken to be the centre of
 * a pixel --- but that this is allowed for in interpreting the inputs
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "dervish.h"
#include "atConversions.h"
#include "phExtract.h"
#include "phExtract_p.h"
#include "phSkyUtils.h"
#include "phObjects.h"

#include "shCFitsIo.h"

#include "phFake.h"

/*
 * BUGS: the model moments are placeholders; some research needs to be
 * done to discover what weighting is best, and in any case normalized
 * moments need to be calculated, which are not as yet. We may well want
 * in the final version to calculate several sets of cellmods for different
 * profile classes; we could accomodate quite a large number with little
 * memory impact (7kB per), but I would bet that a generic Hubble law
 * profile or some such would do just fine.
 *
 * For very large cells, we need to investigate whether appreciable speed
 * advantages accrue by sparse sampling to find the histogram limits
 * and whether it is in fact ever a good idea to do main body processing.
 *
 *
 * General Scheme:
 * These routines populate an array of annular sectors (cells) with approximate
 * ratios of outer to inner radii of 1.25/0.8 = 1.5625 and angular widths
 * of 30 degrees. There are thus twelve angular sectors, and there are
 * 15 radial bins, with a maximum outer radius of 657 pixels;
 *
 * In an inner region (r < SYNCRAD pixels) the image
 * is sync-shifted to center it on a pixel, and annuli 0-6 are populated
 * from these data. Moments are calculated directly from the innermost data,
 * (r<=4 pixels) and from the cell data in the region outside of this. Objects
 * are quantized in size in this scheme; the outer radii are always annulus
 * boundaries (see the array anndex below).
 *
 * The setup code allocates and populates a number of arrays, including:
 *
 *  cellinfo[cell],
 *      a vector of pstats structures, which contain various pieces of
 *      information about each cell required to histogram it and evaluate
 *      the histogram in terms of image moments. (6kB)
 *
 *  cellgeom[cell]
 *      a vector of cellgeom structures, which contain geometric information
 *	about the cells, such as angular and radial extents (~6kB)
 *
 *  cellmod[cell]
 *      a vector of cellmod structures, which contain geometric
 *      information about the cells, weighted by some intensity distribution.
 *      The information includes linear and quadratic moments. (7kB)
 *
 *  cellval[cell][j], which is a cache for all the pixel values in a cell,
 *      for histogramming. (2.9MB)
 *
 *  cellorig[sdy][line][intersection]
 *      is a matrix of structures describing the cell boundaries on a given
 *      line with a given fractional offset of the center of the pixel sdy.
 *      Cell origins, lengths, and cell ids are contained, so one knows for
 *      each pixel on each line which cell is involved. The origins and
 *      offsets are scaled to make accurate use of the fractional center
 *      offset in x. (~700kB)
 *
 *  cellid[y][x]
 *      is a matrix of struct cellid for the inner region which
 *      give directly the cell index and a shift for each pixel. This
 *      scheme is not used for the outer region for reasons of efficiency
 *      and storage. (1200B)
 *
 *  syncext
 *      a REGION into which the inner part of an object is to be sync-shifted.
 *      (2.0kB)
 *
 *  cellhist[]
 *      a U16 array of 64K entries for histogramming the cells.
 *
 * These arrays are all allocated and can be freed independently. A
 * substantial amount of computation is required for the initialization of
 * cellorig and cellmod (especially the latter) and it would be well to
 * do these (and cellid) in the pipeline initialization. The memory involved
 * in this `static' part is about 750kB.
 *
 * In operation, the steps are:
 *
 * 1. extract region (or set pointers) to be sync-shifted. The region
 * is SYNC_REG_SIZE*SYNC_REG_SIZE, so with a bell of SYBELL we need a
 * region SYNC_REG_SIZE + 2*(SYBELL - 1) == SYNC_REG_SIZEI
 *
 * 2. sync-shift it so center is on central pixel of
 * SYNC_REG_SIZE*SYNC_REG_SIZE target, which is the pixel (SYNCRAD,SYNCRAD)
 *
 * 3. calculate moments for region with r <= SYNCRAD and populate cell value
 * arrays with annulus index <= NSYNCANN. Objects closer to the edge than
 * SYNCRAD + SYBELL - 1 pixels may be discarded, since they will appear better
 * on adjacent frames, so the counts need not be calculated.
 *
 * 4. Calculate medians, means, sigmas, and iqrs for cells in annuli 5 and 6.
 
 * 5. Calculate means and sds for annulus 4.
 *
 * 6. Calculate means for cells in annuli 0-3
 *
 *   OK. We are done with the little region.
 *
 * 7. If the object fits in the little region (orad <= SYNCRAD), we are done.
 *    Otherwise,
 *
 * 8. Set up pointers for a square region of size 2*orad+1, centered on
 *     cxi= round(ox), cyi = round(oy). Calculate ox-cxi, oy-cyi and scale
 *     this fraction by 16 to represent the nearest ODD 32nd between
 *     -15/32 and +15/32; there are 16 of these, and the sdy index describing
 *     this decenter runs between 0 (-15/32) and 15 (+15/32);
 *     If the object is nearer the edge than orad, use a trucated region, but
 *     not symmetrically clipped. This need be a problem only for objects
 *     bigger than 46(?) pixels, because smaller ones close enough to an edge
 *     for this to be a problem will be better covered on an adjacent frame.
 *
 * 9. for each line, calculate the beginning & ending x-value from the
 *     cellorig array. Then step across the line, dropping the pixel values
 *     into the cell value cache, at the same time keeping track in the
 *     cellinfo array of the number of values found.
 *
 * 10. for each cell, find the mean, median, iqr.
 *
 * When we are done, we have a roughly scale-invariant representation of
 * the object in theta-logr space. The mean of the medians around an
 * annulus is a much better estimate of the flux than the median for
 * an annulus for flattened objects, and we are in a much better position
 * with the sectored data to reject contaminated cells (which we should not
 * have if the deblender works properly.)
 *
 */

/*****************************************************************************/
/*
 * NLCELL is set so that no cell can exceed 2^16 pixels, and corresponds to
 * the 46--71 cell in anndex
 *
 * SSEC1 is slightly bigger than 256*sin(PI/NCELL); 256*sin(15)=66.25; used
 * to flag possible trouble with the central sectors (3,9) intersecting
 * the region boundary
 */
#define NLCELL 23			/* number of linear cells */
#define SSEC1 68

/*
 * outer RADIUS (exclusive)--the scaled radius is strictly less than this,
 * though it may be within 1/32 of a pixel of it. This must be equal to
 * anndex[NANN]
 *
 * FRLIM is the float limiting radius for an object on center of center pixel
 */
#define RLIM 653
#define FRLIM ((double)RLIM - 0.5)

/*
 * (These definitions are now in phExtract.h)
 *
 * Define the region that should be sync shifted.
 *
 * SYNCRAD is the radial index of the outer edge of sync-shifted region,
 * and must equal anndex[NSYNCANN]-1 (n.b. that this is the _radial_ not
 * the array index -- see discussion of anndex[])
 *
 * The sync region definitions are set up as follows:
 *                                  central
 *                                   pixel
 *                                    ||
 *                                    \/
 * |              |    |             |  |             |    |              |
 * |              |    |             |  |             |    |              |
 * <-- SYBELL-1 -->    <-- SYNCRAD -->  <-- SYNCRAD -->    <-- SYBELL-1 -->
 *                <----> SYNCEXTRA                    <----> SYNCEXTRA
 *                     <-------- SYNC_REG_SIZE ------->
 * <---------------------------- SYNC_REG_SIZEI -------------------------->
 * <------------- yrin --------------->
 *
 */

/*
 * There are four groups of cells:
 *  25 cells with only a single pixel
 *  37 `simple' cells, with 4 or fewer pixels, r < 4.5;
 *     these are processed using simplestats()
 *  NCELLIN cells corresponding to annuli in the sync-shifted region;
 *     these are on a fixed grid and are processed using shinstats().
 *  ?? cells outside the sync-shifted region. Here the cells are accomodated
 *     to a decentered grid and are processed using shinstats() or pshqtile(),
 *     whichever is deemed likely to be faster
 */
#define NCELLIN ((NSYNCANN-1)*NSEC + 1)	/* # of cells in sync-shifted region */

/*****************************************************************************/
/*
 * scale factor for subpixellation in outer region
 */
#define YSHFT 4
#define YSCL (1<<YSHFT)
/*
 * scale factor for sync filter
 */
#define FSHFT 12    
#define FSCL (1<<FSHFT)

/*****************************************************************************/
/*
 * static prototypes
 */
static void cellboundary(int cell, struct cellgeom *cbsp);
static void do_pstats(struct pstats *cinfo, int floor,
					  int, float, float, const struct cellgeom *);
static void profallocv(void);
static void profallocs(void);
static void prosfree(void);
static void provfree(void);
static void makedssprof(int, int, double, double, int, float, float);
#if defined(DEBUG)
static void print_cellorig(int sdy);
#endif

/****************** ANNULUS DEFINITION ARRAY *****************************/

static const int anndex[NANN + 1] =
{0, 1, 2, 3, 5, 8, 12, 19, 29, 46, 71, 111, 173, 270, 421, 653};
/* 0  1  2  3  4  5   6   7   8   9  10   11   12   13   14   15 */

/* NB!!! these are beginning radial INDICES, not radii, for the annuli;
 * The radial indices and their relation to radii are discussed in the
 * section on `RING STUFF' below. The rounded radii for all but the
 * smallest annuli are between anndex[i] anndex[i+1]-1 inclusive
 * which means that scaled or float radii lie between anndex[i]-0.5 and
 * anndex[i+1]-0.5. OK? The small annuli (ann<=5) lie in the sync-shifted
 * region and have unique assignments to pixels. See `RING STUFF' below.
 * Note that anndex[NSYNCANN]-0.5 is the radius of the OUTER
 * BOUNDARY of the inner sync-shifted region, and FRLIM = andex[NANN] - 0.5
 * that of the outer boundary of the largest allowable object
 * (657.5 pixels=4.4 arcmin radius, 8.8 arcmin diameter). Objects
 * larger than this must use a rebinned frame to define their
 * outer parts.
 * NB!!!!!  This limit is set by the largest cell containing 65K pixels,
 * so if one wants to extend THESE algorithms very much, one must go to
 * int histograms.
 */
static CELL_STATS cstats;		/* return value for phProfileExtract*/

static float annrad[NANN+1];		/* float outer radii for above annuli*/
static float area[NANN];		/* areas of annuli */

static struct pstats *cellinfo = NULL;	/* about 7kB */
static float *nmod = NULL;		/* a few 100B */
static struct cellgeom *cellgeom = NULL; /* about 6 kB */
static struct cellmod *cellmod = NULL;	/* about 8 kB */
static struct cellgeom *lcellgeom = NULL; /* tiny; linear extractions */
static struct cellmod *lcellmod = NULL; /* also tiny; linear extractions */

static PIX **cellval=NULL;
/*
 * cache for values in cells for histogramming purposes. Each cell holds
 * roughly (anndex[annulus+1]-anndex[annulus]+1 -1)^2 shorts, for a total
 * size of roughly 7*RLIM^2, 3.0 MB. The memory is organized as a matrix of
 * PIX vectors of appropriate length. The cells are numbered along
 * annuli counterclockwise from the -x axis and then to the adjoining
 * (outer) annulus. Cell 0 is the center cell, cell 1 the first cell
 * in annulus 1, etc. There are only 8 cells in the first annulus, so
 * some games are played to populate the 12 sectors.
 *
 * The sector indices are arranged thus:
 *
 *                      9
 *                 10        8
 *
 *              11              7
 *
 *
 *             0        +         6
 *
 *
 *              1               5
 *
 *                 2         4
 *                      3
 *
 * The sector index is (cell-1)%NSEC, the annulus index (cell-1)/NSEC + 1
 * for cell > 0., and the cell number is (annulus-1)*NSEC + sector + 1
 *
 */

/*
 * matrix of cell origins, lengths, and ids, arranged as
 * cellorig[YSCL][line][n]; no numbers are stored, but
 * an origin value of 0x7fff terminates the sequence for a given line.
 * values are given for a quadrant only; the 1st cell origin for each line
 * whose center is in the outer region is given as zero, so the first
 * cell for those lines is really a half-cell, but the LENGTH is the
 * WHOLE length; this is necessary for rounding purposes, and in the
 * measure code this is treated as a whole cell. The cell ids are those
 * appropriate for each of the four quadrants.
 *
 * The c_orig and c_len elements are scaled up by YSCL
 */
struct cellorig{
    short c_orig;			/* scaled origin, referred to center */
    U16 c_len;				/* scaled length */
    unsigned char  c_llcell;		/* cell ids */
    unsigned char  c_lrcell;
    unsigned char  c_urcell;
    unsigned char  c_ulcell;
};

struct cellid{
    unsigned char c_id;			/* cell id */
    unsigned char c_shft;		/* to allow for double-counting */
};


static struct cellorig ***cellorig=NULL;

static struct cellid **cellid=NULL;	/* matrices to connect pixels to
									 cells for inner region. */
static U16 *cellhist = NULL;		/* histogram */

/*
 * REGIONs used by the sync-interpolation routines
 */
static REGION *syncreg = NULL;		/* synced region */
static REGION *syncreg_smoothed = NULL;	/* smoothed synced region */
static REGION *syncin;			/* region to be synced */
static REGION *syncscr;			/* scratch region used in sync */
static REGION *syncext;			/* syncreg extended by filter size */
static REGION *syncext_smoothed;	/* syncreg_smoothed extended
									 by filter size */

/********************** NOTE ON SUBPIXELLATION *****************************/
/*
 * In the outer region, we do not sync-shift but allow the object to have
 * a non-integral center. To this end, we adopt a subpixel grid in x and y
 * nominally 1/16 of a pixel). The CENTERS of these subpixels in x are at
 * i/16 + 1/32, 0<i<16*xsiz, simalarly in y. This keeps the geometry
 * completely symmetrical up-down and left-right; we have no special center
 * line. The central subpixel is taken to be the NEAREST subpixel center;
 * the subpixel offset runs from 0 (at -7/32) to 16 (at +7/32) from the
 * PIXEL center nearest the real center. This makes boundary calculations
 * straightforward; the scaled boundary corresponding to a float offset from
 * the center is just the rounded (offset*16); this is the last INCLUDED
 * subpixel; the next segment starts at this incremented by one; lengths
 * are just downshifted differences. NB!!! Actually, what seems to work best
 * is to add 15 (not 16) to the shifted origin and then downshift; this
 * results in exactly symmetric boundaries for centers exactly on a subpixel.
 */

/***************** RING STUFF FOR THE INNER 11 PIXELS **********************/
/*
 * Mirage profiles are always centered on a pixel, the central pixel
 * having index 0; the next ring has index 1 and consists of the four
 * nearest neighbor pixels and the 4 diagonal ones. Subsequent
 * rings have radii equal to the index, and except for the eight neighbor
 * points at (4,2), (2,4) and their various reflections about the center
 * the index is round(radius). Those points have radius 4.47 but are
 * assigned index 4, not 3, for the sake of better continuity in the mean
 * ring radius and the ring weight. For the SDSS profile code, the annuli
 * are constructed from unions of these Mirage rings accoring to the array
 * anndex[]. The sectors are defined by the angular rays at +/-15,45,and 75
 * degrees (annulus 0 includes the positive x axis; sector indices increas
 * in an anticlockwise direction). There is a problem with the pixels along
 * x=y, which always belong half to two sectors. This is dealt with by
 * double-counting everything ELSE in the inner region, and assigning values
 * here to both relevant pixels. This is also a problem in principle in the
 * outside, but we ignore it there for speed and simplicity.
 *
 * The SDSS coordinates are different from the Mirage ones by (0.5,0.5); this
 * is properly allowed for (but the code continues to be written using
 * mirage's convention; caveat lector).
 */

/*****************************************************************************/

#if 0
#define SETDEBUG
#endif
#if defined(SETDEBUG) || defined(SETDEBUGC)
int scr1 = 0;
#endif

/******************** PROSETUP() ******************************************/
/*
 * <AUTO EXTRACT>
 * Allocates and sets up all profile arrays not allocated.
 */
void
phInitProfileExtract(void)
{
    int i,j,k,ks;                 /* scratch indices*/
    int iy,jx;
    /* scratch arrays */
    int tanar[NSEC/4];
    int sxsec[NSEC/2+1];
    float segtan[NSEC/2+1];
    int sxann[NANN+1];

    int annlo;
    int nanni;
    int nseci;
    int sdy;
    int y_32;				/* distance from centre, in pixel/32 */
    int sx;
    int sx2,sy2,sr2;
    int clen;
    int corig;
    int oorig;
    int annulus;
    int sector;
    int cell;
    int co;
    int co2;
    int cso;
    int cso2;
    int cslen;
    int cell0;
    int scr;
    int ir2out,ir2,i2;
    int pshft;
    int pass;
    struct cellorig *pcell;
    struct cellorig *pcell0;
    struct cellmod *ipcell;
    struct cellid *pid ;
    float fx,fy;			/* distance from centre, in pixels */
    float fx2,fy2;			/* fx**2, fy**2 */
    float fxs;
    float fr,fr2;			/* radius, radius^2 */
    float ifr,ifr2;			/* 1/radius, 1/radius^2 */
    float fxsec[NSEC/2 + 1];
    float scrn;
    float scrx;
    float scry;
    float scrxy;
    float scry2;
    float cmul,cxmul;
#ifdef SETDEBUGC
    int sdebug;
#endif

    if(cellinfo != NULL) {		/* already initialised */
		cstats.id = -1;
		return;
    }
/*
 * Check that constraints are satisfied
 */
    shAssert(anndex[NANN] == RLIM);
    shAssert(anndex[NSYNCANN] - 1 == SYNCRAD);
    shAssert(SSEC1 > 256*sin(M_PI/NSEC) && SSEC1 < 256*sin(M_PI/NSEC) + 2);
/*
 * allocate memory and set up pointers
 */
    profallocv();
    profallocs();
/*
 * The subcell index sdy runs from 0 to 15, corresponding to the
 * fractional part fdy of the y center coordinate nearest
 *
 *   fdyl = sdy/16 - (15/32)
 *
 * iyc = round(yc);
 * fdy = yc - iyc;
 * sdy = 0.5*(32.*fdy +16.);   \ 16 instead of 15 to round
 *
 * Note that idy (in 32nds) is 2*sdy - 15
 *
 * populate segtan
 */
    for(i=0;i<NSEC/2;i++){
        segtan[i] = -1./tan(((float)i+0.5)*M_PI/(float)(NSEC/2));
    }
    segtan[NSEC/2] = 1.e37;   /* NB!! should use float limit */

/*
 * populate annrad--inner bdy radius for annuli. At the end of this function,
 * after we've defined the geometry using these values of annrad, we'll go
 * through the cells and refine annrad[] a little
 */
    for(i=0;i<=NANN;i++){
        annrad[i] = anndex[i] - 0.5;	/* boundary radii */
    }
    /* fix anomalies */
    annrad[4] = 4.46;			/* fixes 2,4 problem */
    annrad[0] = 0.0;			/* not -1 */
/*
 * populate tanar, the tangents of the sector lines in the first quadrant,
 * scaled up by 2**14
 */
    for(i=0;i < NSEC/4;i++) {
		tanar[i] = ((float)0x4000*
					tan(((double)i+0.5)*M_PI/(double)(NSEC/2))) + 0.5;
    }
/*
 * Populate cellorig.
 *
 * sdy is the distance from the centre of the pixel in 32nds of a pixel
 * (and corresponds to the index i)
 *
 * j is the distance of the coord centre from the centre of the object in
 * full pixels
 */
    for(sdy = -YSCL + 1;sdy < YSCL;sdy += 2) {
		i = (sdy + YSCL)/2;		/* i.e. 0...SDY-1 */
		for(j = 0;j < RLIM;j++) {
			y_32 = j*(2*YSCL) - sdy;	/* y in 32nds */
			fy = 0.03125*y_32;		/* float y (in pixels) */
			sy2 = (y_32*y_32)/4;		/* y**2 (in 16ths of a pixel) */
/*
 * Populate sxann, the intersections of the annuli with the current line.
 *
 * annlo is the beginning annulus at fx = 0; if >= NSYNCANN (i.e. central
 * column is in the outer region), begin at origin; otherwise start at
 * first annulus intersection
 *
 * nanni is the #of intersections in first quadrant, INCLUDING outer boundary
 * of last annulus AND beginning boundary if y < rsyncann
 */
			k = 0;
			while(annrad[k+1] <= fy) {
				k++;			/* k is first annulus whose
								 OUTER bdy intersects line */
			}

			ks = 0;
			if(k >= NSYNCANN) {
				annlo = k;
				nanni = NANN - annlo + 1;
				sxann[ks++] = 0;
			} else {
				k = NSYNCANN;
				annlo = NSYNCANN - 1;	/* inner boundary of outer region*/
				nanni = NANN - annlo;
			}

			for(;k <= NANN;k++) {
				sr2 = YSCL*anndex[k] - YSCL/2;
				sr2 *= sr2;
				if((sx2 = sr2 - sy2) > 0){
					sxann[ks++] = sqrt((double)sx2) + 1; /* floor+1, # within
														  is floor + 1/2 the
														  central one */
				}
			}
/*
 * at this point, sxann contains all the annulus intersections
 * with the line, including 0 if the line is outside the inner
 * region and the boundary with the inner region if it isn't,
 * and the outer boundary
 *
 * annulus is the index of the _end_ of the current annulus
 */
			annulus = annlo;
			if(annulus < NSYNCANN) annulus = NSYNCANN;

			/* beginning annulus */
			/*populate sxsec*/
			sx = sxann[0];
			nseci = 0;
			sector = NSEC/4;
			for(k=0;k < NSEC/4;k++){
				scr = ((y_32*tanar[k])>>15) + 1;
				if(scr <= sx){
					sector--;
				}else{
					sxsec[nseci++] = scr;
				}
			}
			sxsec[nseci] = 0xfffff;	/* big */
/*
 * sxsec contains the sector intersections in the region outside
 * FRSYNCANN; sector is the beginning sector index.
 */
#ifdef SETDEBUG
			if(i==7 && j == scr1){
				int k;
				printf("line=%d, annlo=%d y_32=%d sy2=%d YSCL=%d\n",
					   j,annlo,y_32,sy2,YSCL);
				printf("sector=%d, annulus=%d, nanni=%d nseci = %d\n",
					   sector,annulus,nanni,nseci);

				printf("sxsec\n");
				for(k = 0;k < nseci;k++) {
					printf("%d%c",sxsec[k],k%10==9 ? '\n' : ' ');
				}
				if(k%10 != 0) putchar('\n');

				printf("sxann\n");
				for(k = 0;k < nanni;k++) {
					printf("%d%c",sxann[k],k%10==9 ? '\n' : ' ');
				}
				if(k%10 != 0) putchar('\n');
			}
#endif
/*
 * populate cellorig; start with the first cell
 */
			pcell = pcell0 = cellorig[i][j];
			corig = pcell->c_orig = sxann[0]; /* either 0 or inner bdy of
											   1st annulus */
			pcell->c_len = 0;		/* we do the length later */
			cell0 = (annulus-1)*NSEC + 1;
			pcell->c_llcell = cell0 + sector;
			pcell->c_lrcell = cell0 + NSEC/2 - sector;
			pcell->c_urcell = cell0 + NSEC/2 + sector;
			if(sector > 0) {
				pcell->c_ulcell = cell0 + NSEC - sector;
			} else {
				pcell->c_ulcell = cell0;
			}
			oorig = corig;

#ifdef SETDEBUG
			if(i==7 && j == scr1) printf("cells: %d\n",(pcell-1)->c_llcell);
#endif
			pcell++;

			ks = 0;			/* index of first sector intersection*/
			for(k = 1;k < nanni;k++){
				cell0 = (annulus-1)*NSEC + 1; /* index of 1st cell in annulus */
				while(sxsec[ks] < sxann[k]) { /* sector boundary is next */
					sector--;
					corig = pcell->c_orig = sxsec[ks];
					clen = pcell->c_orig - oorig;
					if(oorig == 0) clen = 2*clen - 1;
					(pcell-1)->c_len = clen;
					pcell->c_llcell = cell0 + sector;
#ifdef SETDEBUG
					if(i==7 && j == scr1) printf(" %d",(pcell-1)->c_llcell);
#endif
					pcell->c_lrcell = cell0 + NSEC/2 - sector;
					pcell->c_urcell = cell0 + NSEC/2 + sector;
					if(sector > 0) {
						pcell->c_ulcell = cell0 + NSEC - sector;
					} else {
						pcell->c_ulcell = cell0;
					}
					oorig = corig;
					ks++;
					pcell++;
				}
/*
 * if there is no sector bdy inside the next annulus bdy, go to next annulus
 */
				annulus++;			/* next annulus */

				cell0 = (annulus-1)*NSEC + 1; /* index of 1st cell in annulus*/
				corig = pcell->c_orig = sxann[k];   /*next annulus boundary*/
				clen = pcell->c_orig - oorig;
				if(oorig == 0) clen = 2*clen - 1;
				(pcell-1)->c_len = clen;
				pcell->c_llcell = cell0 + sector;
				pcell->c_lrcell = cell0 + NSEC/2 - sector;
				pcell->c_urcell = cell0 + NSEC/2 + sector;
				if(sector) {
					pcell->c_ulcell = cell0 + NSEC - sector;
				} else {
					pcell->c_ulcell = cell0;
				}
				oorig = corig;
#ifdef SETDEBUG
				if(i==7 && j == scr1) printf(" %d",(pcell-1)->c_llcell);
#endif

				pcell++;
			}
			/* fix origin if middle cell */
			if(pcell0->c_orig == 0){
				pcell0->c_orig = -((pcell0+1)->c_orig -1);
			}
			/* mark end */
			pcell--;
			pcell->c_orig = 0x7fff;
			pcell->c_len = 1;
			pcell->c_llcell = NCELL;
			pcell->c_lrcell = NCELL;
			pcell->c_urcell = NCELL;
			pcell->c_ulcell = NCELL;
		}
    }
/*
 * set up cellid; this is tiny, and I am not going to be clever
 */
    ir2out = SYNCRAD*SYNCRAD + SYNCRAD + 1;  /* 133 */

    for(i=-SYNCRAD;i <= SYNCRAD;i++){
        i2 = i*i;
        iy = i + SYNCRAD;
        fy = i;
        fy2 = fy*fy;

        for(k=0;k<NSEC/2;k++) fxsec[k] = abs(i)*segtan[k];
        /*NB!!!! the next statement assumes that y=+/-x is a segment bdy;
         * this is true for NSEC=12, which is the case here, but need not
         * always be so; watch it.
         */
        fxsec[(NSEC/4)/2] = (i>0? -fy: fy);
        fxsec[((3*(NSEC))/4)/2] = (i>0 ? fy : -fy );

        fxsec[NSEC/2] = 1.e6;  /* big */
        ks = 0;     /* 0th sector */
        k = NSYNCANN-1;			/* outer annulus */
        for(j = -SYNCRAD;j <= SYNCRAD; j++){
            jx = j + SYNCRAD;
            pid = cellid[iy] + jx;
            ir2 = i2 + j*j;
            fx = j;
            fx2 = fx*fx;
            fr = sqrt(fx2+fy2);
            pshft = (abs(i) == abs(j)) ? 0 : 1 ;  /* 0 on diag, 1 off */
            if(ir2 < ir2out){
                while((fxs=fxsec[ks]) <= fx){
                    if(fx == fxs){
                        if(pshft) break;      /* not diag; do nothing */
                        if(i < 0) break;      /* lower half; delay incr */
                    }
                    /* this funniness is designed to guarantee that the
                     * shared diagonal pixels are assigned always to
                     * the cell with the smaller cell index for ease of
                     * later addressing.
                     */
                    if(i!=0){
                        ks++;     /* advance to next sector */
                    }else ks = NSEC/2; /* across org at y=0 */
                }
                if(j<=0){
                    while(fr < annrad[k]) k--;
                }else{
                    while(fr > annrad[k+1]) k++;
                }
                if(ks==0){
                    pid->c_id = (k-1)*NSEC + 1 ;
                }else{
                    if(i<=0) pid->c_id = ks + (k-1)*NSEC + 1 ;
                    else     pid->c_id = (NSEC-ks) + (k-1)*NSEC + 1;
                }
                pid->c_shft = pshft;
            }else{
                pid->c_id = NCELL; /* garbage dump; periphery */
                pid->c_shft = 0;
            }
        }
        pid = cellid[SYNCRAD] + SYNCRAD;   /* origin */
        pid->c_id = 0;
        pid->c_shft = 1;
    }

    /* populate cellmod--first clear everything */
    memset(cellmod,'\0',NCELL*sizeof(struct cellmod));
    memset(nmod,'\0',NCELL*sizeof(float));
    shAssert(nmod[0] == 0.0);		/* check that 0.0 is all-bits-0 */

    /* do outer region first */

#ifdef SETDEBUGC
    sdebug = 0;
#endif

    for(pass=0;pass <= 1;pass++){
		/* we have a problem; there is no central subline in the
		 * subpixel array; we must average the results from subcell 7 and 8;
		 * this actually has all manner of salutory features, like doing a
		 * reasonable job along the diagonals. We do only the first quadrant,
		 * which means we only half-populate the cells in sector 0; we
		 * fix that later. Note that it is necessary to weight line 0 by
		 * 1/2 in each pass.
		 */
        i = YSCL/2 - 1 + ((pass == 0) ? 0 : 1);

        for(j=0;j<RLIM;j++) {

            fy = j - .03125*(2*i - YSCL + 1);
            fy2 = fy*fy;
            pcell = cellorig[i][j];
            while((cso = pcell->c_orig) != 0x7fff){
                co = ((cso + YSCL - 1 + RLIM*YSCL)>>YSHFT) - RLIM;
				/* this hanky is to get floor; cso can be negative, but
				 * cso + srlim is always positive.
				 * NB!!!! the correct offset to go from scaled to unscaled
				 * origins appears to be YSCL-1; it is clear that
				 * it should be either YSCL-1 or YSCL, but it is NOT
				 * clear why it should be one or the other; YSCL-1
				 * works best for centered patterns, so we adopt it.
				 */
                cslen = pcell->c_len;
                cso2 = cso + cslen;
                co2 = ((cso2 + YSCL - 1)>>YSHFT);
				/* next origin in unscaled pix;
				 * this should never be negative,
				 * so no funny business req.
				 */
                cell = pcell->c_llcell;
                sector = (cell-1)%NSEC;

                cmul = cxmul = 1.;
                if(sector == NSEC/4 && co > 0){	/* horns */
                    cmul = 2.;
                    cxmul = 0.;
                }
                if(j == 0){
                    cmul = cxmul = 0.5;	/* center line */
                }
                fx = co;
                scrx = fx*cxmul;
                fx2 = fx*fx;
                scry = fy*cmul;
                scry2 = fy2*cmul;

                ipcell = &cellmod[cell];
				scrn = nmod[cell];
                for(k=co;k<co2;k++){
					ifr2 = 1.0/(fx2+fy2);
					ifr = sqrt(ifr2);
					fr = 1.0/ifr;
                    scrn += cmul;
                    ipcell->col -= scrx;
                    ipcell->row -= scry;
                    ipcell->col2 += cmul*fx2;
                    ipcell->row2 += scry2;
                    ipcell->colrow += scrx*fy;
                    ipcell->Q += cmul*(fx2 - fy2)*ifr2;
                    ipcell->U += 2*scrx*fy*ifr2;
                    ipcell->rmean += cmul*ifr;
                    ipcell->rvar += cmul*fr;
                    /* note that we will accum <r> here, and
                     * use <x**2 + y**2> to get variance
                     */
                    fx += 1.;
                    fx2 = fx*fx;
                    scrx = fx*cxmul;
                }
				nmod[cell] = scrn;
                pcell++;
            }
        }
    }
    /*
     * OK. have now accumulated things into cellmod structs for first
     * quadrant. Now finish computations and propagate to other quadrants.
     * Note that we have only half-populated the cells in sector 0.
     * this has no effect on anything except the model numbers and some
     * of the odd moments, which we correct here.
     */

    for(j = NSYNCANN;j < NANN;j++) {
		cell = (j-1)*NSEC + 1;
		for(i = 0;i <= NSEC/4;i++) {
			ipcell = &cellmod[cell + i];
			scrn = nmod[cell + i];

			ipcell->col2 /= scrn;
			ipcell->row2 /= scrn;
			ipcell->colrow /= scrn;
			ipcell->Q /= scrn;
			ipcell->U /= scrn;
			ipcell->col /= scrn;
			ipcell->row /= scrn;
			ipcell->rmean /= scrn;
			ipcell->rmean = 1./(ipcell->rmean) ;
			ipcell->rvar /= scrn;  /* mean r */
			ipcell->rvar = ipcell->col2 + ipcell->row2 -
				ipcell->rvar*ipcell->rvar;

			/* take care of boundary cells */
			if(i == 0) {
				ipcell->row = 0.;
				ipcell->U = ipcell->colrow = 0.;
			} else {
				nmod[cell + i] /= 2;		/* 2 passes, recall */
			}
		}
    }
/*
 * ensure that the U and Q coefficients have 4-fold symmetry and sum to 0
 */
    shAssert(NSEC/4 == 3);		/* we assume this in fixing Q, U */
    for(j = NSYNCANN;j < NANN;j++) {
		float tmp;
       
		cell = (j-1)*NSEC + 1;

		tmp = 0.5*(cellmod[cell].Q + cellmod[cell + NSEC/4].Q);
		cellmod[cell].Q -= tmp;
		cellmod[cell + NSEC/4].Q -= tmp;

		tmp = 0.5*(cellmod[cell + 1].Q + cellmod[cell + NSEC/4 - 1].Q);
		cellmod[cell + 1].Q -= tmp;
		cellmod[cell + NSEC/4 - 1].Q -= tmp;

		tmp = 0.5*(cellmod[cell].U + cellmod[cell + NSEC/4].U);
		cellmod[cell].U -= tmp;
		cellmod[cell + NSEC/4].U -= tmp;

		tmp = 0.5*(cellmod[cell + 1].U + cellmod[cell + NSEC/4 - 1].U);
		cellmod[cell + 1].U -= tmp;
		cellmod[cell + NSEC/4 - 1].U -= tmp;
    }
/*
 * propagate to other quadrants
 */
    for(i=NSEC/4 + 1;i<NSEC/2; i++) {    /* lr */
		for(j=NSYNCANN;j<NANN;j++){
			nmod[(j-1)*NSEC + 1 + i] = nmod[(j-1)*NSEC + 1 + (NSEC/2 - i)];
			cellmod[(j-1)*NSEC + 1 + i] = cellmod[(j-1)*NSEC + 1 + (NSEC/2 - i)];

			cellmod[(j-1)*NSEC + 1 + i].col = -cellmod[(j-1)*NSEC + 1 + i].col;
			cellmod[(j-1)*NSEC + 1 + i].colrow =
				-cellmod[(j-1)*NSEC + 1 + i].colrow;
			cellmod[(j-1)*NSEC + 1 + i].U = -cellmod[(j-1)*NSEC + 1 + i].U;
		}
    }
    for(i=NSEC/2;i<(3*NSEC)/4; i++) {    /* ur */
		for(j=NSYNCANN;j<NANN;j++){
			nmod[(j-1)*NSEC + 1 + i] = nmod[(j-1)*NSEC + 1 + (i - NSEC/2)];
			cellmod[(j-1)*NSEC + 1 + i] = cellmod[(j-1)*NSEC + 1 + (i - NSEC/2)];

			cellmod[(j-1)*NSEC + 1 + i].col = -cellmod[(j-1)*NSEC + 1 + i].col;
			cellmod[(j-1)*NSEC + 1 + i].row = -cellmod[(j-1)*NSEC + 1 + i].row;
		}
    }
    for(i=(3*NSEC)/4;i<NSEC; i++) {    /* ul */
		for(j=NSYNCANN;j<NANN;j++){
			nmod[(j-1)*NSEC + 1 + i] = nmod[(j-1)*NSEC + 1 + (NSEC - i)];
			cellmod[(j-1)*NSEC + 1 + i] = cellmod[(j-1)*NSEC + 1 + (NSEC - i)];

			cellmod[(j-1)*NSEC + 1 + i].row = -cellmod[(j-1)*NSEC + 1 + i].row;
			cellmod[(j-1)*NSEC + 1 + i].colrow =
				-cellmod[(j-1)*NSEC + 1 + i].colrow;
			cellmod[(j-1)*NSEC + 1 + i].U = -cellmod[(j-1)*NSEC + 1 + i].U;
		}
    }

    /* outer region is done. Now do inner. This should be cleaner. */

    ir2out = SYNCRAD*SYNCRAD + SYNCRAD + 1; /* 133 */
    for(i=-SYNCRAD;i <= SYNCRAD;i++) {
        i2 = i*i;
        iy = i + SYNCRAD;
        fy = i;
        fy2 = fy*fy;

        for(j = -SYNCRAD; j <= SYNCRAD; j++){
            ir2 = i2 + j*j;
            jx = j + SYNCRAD;
            fx = j;
            fx2 = fx*fx;
            fr2 = (fx2+fy2);
			if(fr2 == 0) {
				fr = ifr = ifr2 = 0;
			} else {
				ifr2 = 1.0/fr2;
				ifr = sqrt(ifr2);
				fr = 1.0/ifr;
			}

            if(ir2 < ir2out){
                pid = cellid[iy] + jx;
                cell = pid->c_id;
                ipcell = &cellmod[cell];
                pshft = pid->c_shft;
                cmul = (1<<pshft);
				scrxy = fx*fy;

                nmod[cell]    += cmul;
                ipcell->col2   += fx2*cmul;
                ipcell->row2   += fy2*cmul;
                ipcell->colrow    += cmul*scrxy;
                ipcell->Q   += (fx2 - fy2)*cmul*ifr2;
                ipcell->U    += cmul*2*scrxy*ifr2;
                ipcell->col     += fx*cmul;
                ipcell->row     += fy*cmul;
                ipcell->rvar  += fr*cmul;
                ipcell->rmean += cmul*ifr;
                if(pshft == 0){
                    ipcell += 1;   /* other half-cell */
                    nmod[cell + 1]+= 1.;
                    ipcell->col2   += fx2;
                    ipcell->row2   += fy2;
                    ipcell->colrow    += scrxy;
                    ipcell->Q   += (fx2 - fy2)*ifr2;
                    ipcell->U    += 2*scrxy*ifr2;
                    ipcell->col     += fx;
                    ipcell->row     += fy;
                    ipcell->rvar  += fr;
                    ipcell->rmean += ifr;
                }
            }
        }
    }
    /* fix up center cells */
    ipcell = cellmod;
    ipcell->rmean = 0.381;
    ipcell->col = ipcell->row = ipcell->colrow = 0;
    ipcell->col2 = ipcell->row2 = ipcell->rvar = 0;
    ipcell->Q = ipcell->U = 0;

    for(i=1;i < NCELLIN;i++) {
        ipcell = &cellmod[i];
        scrn = nmod[i];
        ipcell->col2 /= scrn;
        ipcell->row2 /= scrn;
        ipcell->colrow /= scrn;
        ipcell->Q /= scrn;
        ipcell->U /= scrn;
        ipcell->col /= scrn;
        ipcell->row /= scrn;
        ipcell->rmean /= scrn;
        ipcell->rmean = 1./(ipcell->rmean) ;
        ipcell->rvar /= scrn;		/* mean r */
        ipcell->rvar = ipcell->col2 + ipcell->row2 - ipcell->rvar*ipcell->rvar;
    }
/*
 * The cellgeom struct has a place for the number of pixels in each cell.
 * This is related to nmod, but unfortunately nmod is 2*n_cell in the inner
 * part, and n_cell in the outer so we cannot just use it.
 */
    for(i = 0;i < NCELLIN;i++) {
		cellgeom[i].n = nmod[i]/2;
		cellboundary(i,&cellgeom[i]);
    }
    for(;i<NCELL;i++) {
		cellgeom[i].n = nmod[i];
		cellboundary(i,&cellgeom[i]);
    }
/*
 * as promised, refine annrad[]
 */
    {
		float enclosed_area;		/* cumulative area */
       
		for(i = 0;i < NANN;i++) {
			area[i] = 0;			
		}
       
		for(i = 0;i < NCELL;i++) {	/* count pixels in all cells */
			int ind = cellgeom[i].ann;
			shAssert(ind >= 0 && ind < NANN);
			area[ind] += cellgeom[i].n;
		}
       
		enclosed_area = 0.0;
		for(i = 0;i < NANN;i++) {
			annrad[i] = sqrt(enclosed_area/M_PI);
			enclosed_area += area[i];
		}
		annrad[i] = sqrt(enclosed_area/M_PI);
    }
/*
 * The inner annuli are handled by phSincApertureMean(), so their cellmod[]
 * values are exact.  Replace the approximate (pixel-based) values that
 * we just calculated
 *
 * N.b. theta is measured from -ve x axis --- be careful with signs for column
 */
    for(i = 1;i < NCELLIN;i++) {
		float theta1 = cellgeom[i].ccw;
		float theta2 = cellgeom[i].cw;
		float r1 = annrad[i/NSEC + 1];
		float r2 = annrad[i/NSEC + 2];
		float area = (pow(r2, 2) - pow(r1, 2))/2*(theta2 - theta1);
#if 0
		printf("%d  %6.3f %6.3f  %6.3f %6.3f %6.3f\n", i,
			   cellmod[i].row, cellmod[i].col,
			   cellmod[i].row2, cellmod[i].colrow, cellmod[i].col2);
#endif
		cellmod[i].rmean = (r2 - r1)*(theta2 - theta1)/area;
       
		cellmod[i].row =
			(pow(r2, 3) - pow(r1, 3))/3*(-cos(theta2) + cos(theta1))/area;
		cellmod[i].col =
			-(pow(r2, 3) - pow(r1, 3))/3*(sin(theta2) - sin(theta1))/area;
	
		cellmod[i].col2 =
			(pow(r2, 4) - pow(r1, 4))/4*
			(theta2 + 0.5*sin(2*theta2) - (theta1 + 0.5*sin(2*theta1)))/2/area;
		cellmod[i].colrow =
			-(pow(r2, 4) - pow(r1, 4))/4*(cos(2*theta1) - cos(2*theta2))/4/area;
		cellmod[i].row2 =
			(pow(r2, 4) - pow(r1, 4))/4*
			(theta2 - 0.5*sin(2*theta2) - (theta1 - 0.5*sin(2*theta1)))/2/area;
#if 0
		printf("%d  %6.3f %6.3f  %6.3f %6.3f %6.3f\n", i,
			   cellmod[i].row, cellmod[i].col,
			   cellmod[i].row2, cellmod[i].colrow, cellmod[i].col2);
#endif
    }
    
    cstats.id = -1;			/* invalidate old values */
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Free all of the allocated arrays.
 */
void
phFiniProfileExtract(void)
{
	provfree();				/* volatile ones */
	prosfree();				/* static ones */
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Constructor for a CELL_PROF. If n is > 0, the radial arrays will be
 * allocated for you
 */
CELL_PROF *
phCellProfNew(int n)			/* number of data points */
{
	CELL_PROF *cprof = shMalloc(sizeof(CELL_PROF));

	cprof->ncell = n;
	cprof->is_median = -1;

	return(cprof);
}

/*
 * <AUTO EXTRACT>
 *
 * Allocate or extend the data arrays in a CELL_PROF
 */
void
phCellProfRealloc(CELL_PROF *cprof,	/* a pre-existing CELL_PROF */
				  int n)		/* number of data points */
{
	shAssert(cprof != NULL);
	cprof->ncell = n;
}

/*
 * <AUTO EXTRACT>
 *
 * Destructor for a CELL_PROF
 */
void
phCellProfDel(CELL_PROF *cprof)
{
	shFree(cprof);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Is profileExtract initialized
 */
int 
phProfileIsInitialised(void)
{
    return(cellinfo == NULL ? 0 : 1);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Fill out values in a CELL_STATS based on a CELL_PROF
 */
const CELL_STATS *
phProfileSetFromCellprof(const CELL_PROF *cprof)
{
	int i;				/* index into cprof */
	int i1, i2;				/* indices into cstats */
	int is_median;			/* == cstats.is_median */
   
	shAssert(phProfileIsInitialised() && cprof != NULL && cprof->ncell >= 1);

	is_median = cprof->is_median;
	cstats.id = -2;			/* invalid ID, set from CELL_PROF */
	cstats.annular = 1;
	cstats.ncell = 1 + 2*(cprof->ncell - 1);
	cstats.nannuli_c = cstats.nannuli = 1 + (cprof->ncell - 1)/(NSEC/2);
	cstats.syncreg = NULL;
   
	for(i = 0; i < cprof->ncell; i++) {
		i1 = (i == 0) ? 0 : 1 + NSEC*(int)((i - 1)/(NSEC/2)) + (i - 1)%(NSEC/2);
		i2 = (i == 0) ? i1 : i1 + NSEC/2;

		cstats.cells[i1].flg = cstats.cells[i2].flg = EXTRACT_NONE;
		if(cprof->area[i] > 0) {
			cstats.cells[i1].area = cstats.cells[i2].area = cprof->area[i];
		}
		if(is_median) {
			cstats.cells[i1].qt[1] = cstats.cells[i2].qt[1] = cprof->data[i];
		} else {
			cstats.cells[i1].mean = cstats.cells[i2].mean = cprof->data[i];
		}
		cstats.cells[i1].sig = cstats.cells[i2].sig = cprof->sig[i];
	}

	return(&cstats);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Extract the profile data (i.e. fill our pixel cache) for an object
 * centered at (fxc, fyc).  Note that the arguments "fyc" and "fxc"
 * do not take into account the (row0, col0) offset of the REGION.
 * Consider a full-frame REGION with 1354 rows by 2048 columns,
 * and imagine a star at (500, 500) in this frame.  If one makes
 * a 20x20 subRegion centered on the star, so that 
 *
 *     subRegion has row0=490, col0=490,   nrow=20, ncol=20
 *
 * and one passes the subRegion to this function, one must
 * pass
 *
 *     fyc = 10.0, fxc = 10.0
 *
 * and NOT use (fyc=500, fxc=500), as one might naively expect.
 *
 * Note that "fyc" is the row center, and "fxc" is the col center.
 *
 * The routine returns a structure describing the extracted profile
 *
 * If the value of id is the same as in the previous call to phProfileExtract
 * (and if it's positive), we'll assume that the same object is being
 * extracted with a different outer radius, and reuse as much of the
 * previously calculated profile information as possible.
 */
CELL_STATS *
phProfileExtract(int id,		/* ID for this profile */
				 int band,		/* which band is this?
								 Useful for debugging */
				 const REGION *in,	/* input region */
				 double fyc,            /* row centre of object */
				 /*    relative to REGION's origin */
				 double fxc,            /* col centre of object */
				 /*    relative to REGION's origin */
				 int orad,		/* desired radius of largest annulus;
								 if -ve, desired number of annuli */
				 double sky,		/* sky level */
				 double skysig,		/* sky sigma */
				 int keep_profile)	/* keep old profile at r > orad */
{
    int cellim0;			/* value of cellim from previous call
							 to phProfileExtract with this id */
    int i,j;
    int sdy_l, sdy_u;			/* These are the indexes into cellorig
								 based on which y-subpixel the
								 object's centre lies in, for the
								 lower and upper half of the object*/
    int rlim,rlimyl,rlimyu;
    int srlim;
    const int yrin = SYNCRAD + SYNCEXTRA + SYBELL - 1;
    int annlim;				/* first annulus beyond limit */
    int cellim;				/* first cell beyond limit */
    int cxext;
    int lxext,rxext;			/* extents left and right */
    int slxext,srxext;			/* scaled extents left and right */
    float filx[2*SYBELL - 1];		/* sync filters in x */
    float fily[2*SYBELL - 1];		/*                   and y */
    PIX *line;
    int iyc,ixc,sxc;
    float fdy,fdx;
    int slen,sxorig;
    int xorig, yorig;			/* col, row origin of a span */
    int corig;
    float min_shift = 2e-2;		/* minimum shift to bother with XXX */
    register int len,cell = 0;
    int nannuli_old;			/* old values of cstats.nannuli(_c)? */
    int nannuli_c_old;			/*      used if keep_profile is true */
    register PIX *ppix;
    register int p;
    register PIX *cache, *cache_end;
    struct cellorig *pcell;
    int iscr;
    int cello;
    int ndiag;
    int ccareful = 0;
    int lcareful = 0;			/* flags to be careful at edges */
    int rcareful = 0;
/*
 * Variables unpacked because of aliasing
 */
    PIX **rows;				/* == in->ROWS */
    int ncol, nrow;			/* == in->{ncol,nrow} */

    if(in == NULL) {			/* we just want to set id */
		cstats.id = id;
		return(NULL);
    }
    
    shAssert(in != NULL);
    shAssert(in->type == TYPE_PIX);
    shAssert(cellinfo != NULL);

    nrow = in->nrow; ncol = in->ncol;
    rows = in->ROWS;
/*
 * Give up if the object is entirely off the frame
 */
    if(fxc < 0 || fxc + 0.5 >= ncol ||
       fyc < 0 || fyc + 0.5 >= nrow) {
		shErrStackPush("object %d is off the frame",id);
		return(NULL);
    }
/*
 * find outermost annulus that we need to process given orad
 */
    if(orad > 0) {
		for(annlim = 0; annlim <= NANN && orad > anndex[annlim];annlim++) {
			continue;
		}
    } else {
		annlim = -orad;
    }
    if(annlim > NANN) {
		annlim = NANN;
    }
      
    if(annlim == 0) {
		shErrStackPush("Extraction for object %d has too small a radius",id);
		return(NULL);
    }
    rlim = anndex[annlim];		/* first radius beyond limit--ie
								 r < rlim */
/*
 * if the id is the same as the previous call, we may be able to reuse
 * some of the information in the cstats.

 * This causes problems if phProfileExtract() is called twice for the same
 * object, but the second call has a smaller outer radius than the first;
 * in this case we are done, so adjust the outer radius and return.
 *
 * If the quoted centre's different, none of this helps.
 *
 * The exception is when keep_profile is true; in the case we are explicitly
 * requested _NOT_ to invalidate parts of the profile beyond orad, even if
 * the centre's changed.  The user had better understand what they are doing.
 */
    if(id < 0 || id != cstats.id) {
		if(keep_profile) {
			keep_profile = 0;		/* the current profile is irrelevant */
		}
    } else if(annlim <= cstats.nannuli) {
		if(fabs(fxc - cstats.col_c) > 1e-4 || fabs(fyc - cstats.row_c) > 1e-4) {
			cstats.id = -1;		/* the centre's moved */
		} else {				/* no more work to do */
			if(!keep_profile) {
				cstats.nannuli = annlim;
				if(cstats.nannuli_c > cstats.nannuli) {
					cstats.nannuli_c = cstats.nannuli;
				}
			}
			return(&cstats);
		}
    }

    if(id < 0 || id != cstats.id) {	/* start from scratch */
		cellim0 = 0;
		cellinfo[NCELL].ntot = 0;
    } else {       
		shAssert(fabs(fxc - cstats.col_c) < 1e-4 &&
				 fabs(fyc - cstats.row_c) < 1e-4);
		cellim0 = (cstats.nannuli <= 0) ? 0 : NSEC*(cstats.nannuli - 1) + 1;
    }
/*
 * Set the parts of the return struct that we already know
 */
    cstats.id = id;
    cstats.annular = 1;
    cstats.col_c = fxc; cstats.row_c = fyc;
/*
 * process the desired outer radius
 */
    cstats.orad = rlim;

    srlim = rlim<<YSHFT;		/* scaled limiting radius */
    cellim = (annlim-1)*NSEC + 1;	/* first cell beyond limit */

    if(cellim < cellim0) {		/* we are already fully extracted */
		return(&cstats);
    }
/*
 * setup subpixellation
 *
 * N.b. for an object centred on a pixel, fdx == fdy == 0.0
 *                                        (sxc,syc)%YSCL = (7.5, 7.5)
 */
    iyc = (int)fyc;
    fdy = (fyc - iyc) - 0.5;
    sdy_u = (float)YSCL*(fdy + 0.499999); /* for upper half */
    shAssert(sdy_u >= 0 && sdy_u < YSCL);
    sdy_l = YSCL - sdy_u - 1;		/* for lower half */
    shAssert(sdy_u != sdy_l);

    rlimyl = rlimyu = rlim;
    if(rlimyl > iyc + 1) rlimyl = iyc + 1;
    if(rlimyu > nrow - iyc) rlimyu = nrow - iyc;

    ixc = (int)fxc;
    fdx = (fxc - ixc) - 0.5;
    sxc = (fxc - 0.5)*(float)YSCL + 0.5 ; /* nearest subpixel center */
/*
 * check on x edges; if the region intersects an edge, various
 * flags are set which can be checked later with no computation.
 * the flags are rcareful, which is set if the object overlaps
 * the right edge of the region, lcareful the left, and ccareful
 * if the central sectors might intersect the edge.
 */
    cxext = (rlim*SSEC1)>>8; /* approximate x-extent of sectors 3,9 */
    lxext = rxext = rlim;
    if(lxext > ixc){
        lxext = ixc;
        lcareful = 1;
        if(ixc < cxext) ccareful = 1;
    }
    slxext = lxext*YSCL;
    if(rxext > (iscr = ncol - ixc - 1)){
        rxext = iscr;
        rcareful = 1;
        if(iscr < cxext) ccareful = 1;
    }
    srxext = rxext*YSCL;
/*
 * clear the parts of the cellinfo array that we cannot reuse
 */
    if(cellim0 == 0) {
		if(cellim < NCELLIN) {
			memset(cellinfo,'\0', NCELLIN*sizeof(struct pstats));
		} else {
			memset(cellinfo,'\0', cellim*sizeof(struct pstats));
		}
		shAssert(cellinfo[0].ntot == 0);    /* check that memset correctly */
		shAssert(cellinfo[0].mean == 0.0);  /*   sets variables to */
		shAssert(cellinfo[0].data == NULL); /*   0 or 0.0 or NULL */
    } else {
		shAssert(cellim > cellim0);
		if(cellim < NCELLIN) {
			;				/* don't clear the sync region */
		} else {
			for(i = NCELLIN;i < cellim0;i++) {
				cellinfo[i].ntot = 0;
			}
			memset(&cellinfo[cellim0],'\0',
				   (cellim - cellim0)*sizeof(struct pstats));
		}
    }
/*
 * populate the cell cache
 *
 * First the outer region, if it's needed. We could do this by starting at the
 * central column and working to the right, then returning to the centre and
 * working left, but this isn't quite right as there's the sector that contains
 * the central column to worry about. This is the `central' column, and doesn't
 * exist if the line that we are processing passes through the central synced
 * region.
 *
 * First check if we are going to be able to do the sinc region
 */
    if(ixc < yrin || ixc + yrin >= ncol ||
       iyc < yrin || iyc + yrin >= nrow) {
		for(i = 0;i < NCELLIN;i++) {	/* too close to edge to sync */
			cellinfo[i].ntot = -1;
		}
		cstats.syncreg = NULL;		/* not shifted */
		cstats.nannuli = cstats.nannuli_c = cstats.orad = 0;
		return(&cstats);
    }
    cstats.syncreg = syncreg;

    if(cellim >= NCELLIN) {
/*
 * First the lower half (including the central row)
 */
		for(i = 0;i < rlimyl;i++) {
			pcell = cellorig[sdy_l][i];
			yorig = iyc - i;
			line = rows[yorig];
/*
 * Process the lower half, central segment, if it exists
 */
			if((corig = pcell[0].c_orig) < 0) {
				sxorig = (sxc + corig + YSCL - 1);
				xorig = ((sxorig + srlim)>>YSHFT) - rlim;  /* positivity */
				len  = ((sxorig + (pcell[0].c_len))>>YSHFT) - xorig;
				ppix = line + xorig;
				cell = pcell[0].c_lrcell;
				if(ccareful){
					if(xorig < 0){
						len += xorig;
						ppix = line;
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}else{
						if((iscr = xorig+len-ncol) > 0) {
							len -= iscr;
							cellinfo[cell].flg |= EXTRACT_EDGE;
						}
					}
				}
				if(len > 0) {
					if(yorig == 0) {
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}
					cache = cellval[cell] + cellinfo[cell].ntot;
					cache_end = cache + len;
					cellinfo[cell].ntot += len;
					do {
						*cache++ = *ppix++;
					} while(cache < cache_end);
				}

				pcell++;			/* we don't want to see the central
									 cell again */
			}
/*
 * Now the lower half, right side
 */
			for(j = 0;(corig = pcell[j].c_orig) < srxext &&
					(cell = pcell[j].c_lrcell) < cellim;j++) {
				sxorig = (sxc + corig + YSCL - 1);
				xorig = (sxorig >> YSHFT);
				len  = ((sxorig + (pcell[j].c_len))>>YSHFT) - xorig;
				ppix = line + xorig;
				if(rcareful){
					if((iscr=xorig+len-ncol) > 0) {
						len -= iscr;
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}
				}
				if(len > 0) {
					if(yorig == 0) {
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}
					cache = cellval[cell] + cellinfo[cell].ntot;
					cache_end = cache + len;
					cellinfo[cell].ntot += len;
					do {
						*cache++ = *ppix++;
					} while(cache < cache_end);
				}
			}
/*
 * and the lower half, left side
 */
			for(j = 0;(corig = pcell[j].c_orig) < slxext &&
					(cell=pcell[j].c_llcell) < cellim;j++) {
				slen = pcell[j].c_len;
				sxorig = (sxc - corig - slen + YSCL);
				xorig = ((sxorig+srlim)>>YSHFT) - rlim; /* positivity*/
				len  = ((sxorig + slen)>>YSHFT) - xorig;
				ppix = line + xorig;
				if(lcareful){
					if(xorig < 0){
						len += xorig;
						ppix = line;
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}
				}
				if(len > 0) {
					if(yorig == 0) {
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}
					cache = cellval[cell] + cellinfo[cell].ntot;
					cache_end = cache + len;
					cellinfo[cell].ntot += len;
					do {
						*cache++ = *ppix++;
					} while(cache < cache_end);
				}
			}
		}
/*
 * Now the upper half, again central, right, and left
 */
		for(i = 1;i < rlimyu;i++) {
			pcell = cellorig[sdy_u][i];
			yorig = iyc + i;
			line = rows[yorig];
/*
 * Central segment if it exists
 */
			if((corig = pcell[0].c_orig) < 0) {
				sxorig = (sxc + corig + YSCL - 1);
				xorig = ((sxorig + srlim)>>YSHFT) - rlim;  /* positivity */
				len  = ((sxorig + (pcell[0].c_len))>>YSHFT) - xorig;
				ppix = line + xorig;
				cell = pcell[0].c_urcell;

				if(ccareful){
					if(xorig < 0){
						len += xorig;
						ppix = line;
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}else{
						if((iscr = xorig+len-ncol) > 0) {
							len -= iscr;
							cellinfo[cell].flg |= EXTRACT_EDGE;
						}
					}
				}
				if(len > 0) {
					if(yorig == nrow - 1) {
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}
					cache = cellval[cell] + cellinfo[cell].ntot;
					cache_end = cache + len;
					cellinfo[cell].ntot += len;
					do {
						*cache++ = *ppix++;
					} while(cache < cache_end);
				}
				pcell++;
			}
/*
 * Now the upper half, right side
 */
			for(j = 0;(corig = pcell[j].c_orig) < srxext &&
					(cell=pcell[j].c_urcell) < cellim;j++) {
				/* in bounds */
				sxorig = (sxc + corig + YSCL - 1);   /* is this right ?? */
				xorig = (sxorig >> YSHFT);
				len  = ((sxorig + (pcell[j].c_len))>>YSHFT) - xorig;
				ppix = line + xorig;
				if(rcareful){
					if((iscr=xorig+len-ncol) > 0) {
						len -= iscr;
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}
				}

				if(len > 0) {
					if(yorig == nrow - 1) {
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}
					cache = cellval[cell] + cellinfo[cell].ntot;
					cache_end = cache + len;
					cellinfo[cell].ntot += len;
					do {
						*cache++ = *ppix++;
					} while(cache < cache_end);
				}
			}
/*
 * and the upper half, left side
 */
			for(j = 0;(corig = pcell[j].c_orig) < slxext &&
					(cell = pcell[j].c_ulcell) < cellim;j++) {
				/* in bounds */
				slen = pcell[j].c_len;
				sxorig = (sxc - corig - slen + YSCL);
				xorig = ((sxorig+srlim)>>YSHFT) - rlim; /* positivity*/
				len  = ((sxorig + slen)>>YSHFT) - xorig;
				ppix = line + xorig;
				if(lcareful){
					if(xorig < 0){
						len += xorig;
						ppix = line;
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}
				}
				if(len > 0) {
					if(yorig == nrow - 1) {
						cellinfo[cell].flg |= EXTRACT_EDGE;
					}
					cache = cellval[cell] + cellinfo[cell].ntot;
					cache_end = cache + len;
					cellinfo[cell].ntot += len;
					do {
						*cache++ = *ppix++;
					} while(cache < cache_end);
				}
			}
		}
/*
 * If we haven't already done so, populate the special cellinfo element
 * for all the pixels that are in the inner region, and are thus treated
 * differently from the outer pixels
 */
		if(cellinfo[NCELL].ntot == 0) {
			int x1, x2;

			rlim = anndex[NSYNCANN + 1];
			srlim = rlim<<YSHFT;	/* scaled limiting radius */
/*
 * First the lower half (including the central row)
 */
			for(i = 0;i < SYNCRAD + 1;i++) {
				pcell = cellorig[sdy_l][i];
				yorig = iyc - i;
				line = rows[yorig];
				if(pcell[0].c_orig < 0) {	/* central cell; skip */
					pcell++;
				}

				x1 = -1000; x2 = 100000;
				for(j = 0; pcell[j].c_llcell < NCELLIN + NSEC;j++) {
					corig = pcell[j].c_orig;
					slen = pcell[j].c_len;

					sxorig = (sxc - corig - slen + YSCL); /* left side */
					xorig = (sxorig + slen) >> YSHFT; /* end of span */
					if(xorig > x1) {
						x1 = xorig;
					}

					sxorig = (sxc + corig + YSCL - 1); /* right side */
					xorig = sxorig >> YSHFT; /* beginning of span */
					if(xorig < x2) {
						x2 = xorig;
					}
				}

				len  = x2 - x1;
				memcpy(&cellinfo[NCELL].data[cellinfo[NCELL].ntot],&line[x1],
					   len*sizeof(PIX));
				cellinfo[NCELL].ntot += len;
			}
/*
 * and now the upper half
 */
			for(i = 1;i < SYNCRAD + 1;i++) {
				pcell = cellorig[sdy_u][i];
				yorig = iyc + i;
				line = rows[yorig];
				if(pcell[0].c_orig < 0) {	/* central cell; skip */
					pcell++;
				}

				x1 = -1000; x2 = 100000;
				for(j = 0; pcell[j].c_ulcell < NCELLIN + NSEC;j++) {
					corig = pcell[j].c_orig;
					slen = pcell[j].c_len;

					sxorig = (sxc - corig - slen + YSCL); /* left side */
					xorig = (sxorig + slen) >> YSHFT; /* end of span */
					if(xorig > x1) {
						x1 = xorig;
					}

					sxorig = (sxc + corig + YSCL - 1); /* right side */
					xorig = sxorig >> YSHFT; /* beginning of span */
					if(xorig < x2) {
						x2 = xorig;
					}
				}

				len  = x2 - x1;
				memcpy(&cellinfo[NCELL].data[cellinfo[NCELL].ntot],&line[x1],
					   len*sizeof(PIX));
				cellinfo[NCELL].ntot += len;
			}

			makedssprof(NCELL, NCELL, sky, skysig, band, fxc, fyc);
		}
    }
/*
 * done--now do inner region
 */
    if(cellim0 > 0) {			/* already extracted */
		;
    } else {
/*
 * Extract the sync buffer: first set up the pointers.
 *
 * The input region, syncin, is of size SYNC_REG_SIZEI x SYNC_REG_SIZEI
 * with the central pixel at (xc,yc).
 *
 * First generate the sync filters, and do the sync interpolation. Note that
 * the REGION syncreg is aliased to the central part of syncext. We don't
 * bother to shift if the offset is too small
 */
		for(i=0;i < SYNC_REG_SIZEI;i++) {
			syncin->ROWS[i] = &rows[i+iyc-yrin][ixc-yrin];
		}
		syncin->ncol = SYNC_REG_SIZEI;

		if(fabs(fdx) <= min_shift && fabs(fdy) <= min_shift) {
			shRegIntCopy(syncext, syncin);
		} else {
			get_sync_with_cosbell(filx,SYBELL,fdx);
			get_sync_with_cosbell(fily,SYBELL,fdy);
       
			phConvolve(syncext,syncin,syncscr,2*SYBELL-1,2*SYBELL-1,filx,fily,0,
					   CONVOLVE_MULT);
		}
		syncreg->row0 = iyc - yrin + (SYBELL - 1);
		syncreg->col0 = ixc - yrin + (SYBELL - 1);
#if defined(DEBUG)
		{
			REGION *tmp = shRegNew("",syncreg->nrow,syncreg->ncol,TYPE_PIX);
			for(i = 0;i < tmp->nrow;i++) {
				memcpy(tmp->ROWS[i],syncreg->ROWS[i],sizeof(PIX)*tmp->ncol);
			}
			if(shRegWriteAsFits(tmp,"syncreg.fts",STANDARD,2,DEF_NONE,NULL,0) !=
			   SH_SUCCESS) {
				shErrStackPush("Failed to write sync shifted region 0x%x",tmp);
				return(NULL);		/* don't bother with proper cleanup */
			}
			shRegDel(tmp);
		}
#endif
/*
 * We start by setting cello to 0 rather than (say) -1 as it allows us
 * to avoid testing cello inside the loop
 */
		cello = 0;
		cache = cellval[cello] + cellinfo[cello].ntot;
       
		for(i=0;i < SYNC_REG_SIZE;i++) {
			PIX *row = &syncreg->ROWS[SYNCEXTRA + i][SYNCEXTRA];
			struct cellid *cells = cellid[i];
			for(j=0;j < SYNC_REG_SIZE;j++){
				if((cell = cells[j].c_id) >= NCELLIN) continue;
				p = row[j];
	     
				ndiag = cells[j].c_shft;
				if(cell != cello){
					cellinfo[cello].ntot = cache - cellval[cello];
		
					cello = cell;
					cache = cellval[cell] + cellinfo[cell].ntot;
				}
				*cache++ = p;
				if(ndiag){			/* double it if not on diag */
					*cache++ = p;
				} else {			/* diag, go to companion cell */
					if(cell != 0) {		/* central cell has no companions,
										 but wants to be doubled */
						cellinfo[cell].ntot = cache - cellval[cell];
		   
						cello = ++cell;
						cache = cellval[cell] + cellinfo[cell].ntot;
					}
					*cache++ = p;
				}
			}
		}
		if(cell != NCELL) {		/* NCELL was used as garbage marker */
			cellinfo[cell].ntot = cache - cellval[cell];
		}
       
		/* OK, we are done with the cache*/
    }
/*
 * now we have the pixels, calculate the desired quantities (mean, median, etc)
 */
    for(i = cellim0;i < cellim;i++){
		cellinfo[i].data = cellval[i];
    }
    makedssprof(cellim0, cellim - 1, sky, skysig, band, fxc, fyc);
/*
 * for the inner cells, those lying within the sinc region, evaluate the
 * means via integration over the sinc-interpolated function
 */
    if(cellim0 == 0 && cstats.syncreg != NULL) {
		for(i = 0;i < NCELLIN;i++) {
			j = phSincApertureMean(cstats.syncreg,i,sky,
								   syncreg->nrow/2, syncreg->ncol/2,
								   &cstats.cells[i].mean, &cstats.cells[i].area);
			shAssert(j == 0);
	  
		}
    }
/*
 * Find how many annuli we have data for, and how many are complete
 *
 * We do NOT consider any data in partial sectors
 */
    if(keep_profile) {
		nannuli_old = cstats.nannuli;
		nannuli_c_old = cstats.nannuli_c;
    } else {
		nannuli_old = nannuli_c_old = 0;	/* make gcc happy */
    }
    
    cstats.nannuli = (cellim - 1)/NSEC + 1;
    cstats.nannuli_c = -1;
    for(i = 1;i < cstats.nannuli;i++) {
		int nel;				/* sum of nel in an annulus */
		int flg;				/* | of flags for this annulus */
		flg = nel = 0;
		for(j = (i - 1)*NSEC + 1;j <= i*NSEC;j++) {
			shAssert(j < cellim);
			flg |= cstats.cells[j].flg;
			if(cstats.cells[j].nel > 0 && !(cstats.cells[j].flg & EXTRACT_EDGE)){
				nel += cstats.cells[j].nel;
			}
		}
		if(nel == 0) {			/* no data in this annulus */
			cstats.nannuli = i;
			break;
		}
		if(flg & EXTRACT_EDGE) {		/* annulus is incomplete */
			if(cstats.nannuli_c < 0) {
				cstats.nannuli_c = i;
			}
		}
    }
    if(cstats.nannuli_c < 0) {
		cstats.nannuli_c = cstats.nannuli;
    }
    shAssert(cstats.nannuli_c <= cstats.nannuli);

    cstats.ncell = (cstats.nannuli - 1)*NSEC + 1;
/*
 * there's a slight problem in the interface between the sinc-shifted
 * and the non-sinc-shifted regions. We can fix this up using the
 * information in prof->cells[NPROF], which gives the properties of
 * all pixels that are in the sinc-shifted region, from the point of
 * view of the outer parts
 *
 * The discrepant flux is assigned to the first annulus outside the
 * sinc region; this is equivalent to fixing up its inner boundary, on
 * the assumption that its _outer_ boundary is correct.
 *
 * We don't do this if orad just fits within the sinc region even if
 * keep_profile is true, as the outer profile has already been adjusted
 * for this effect once.
 */
    if(cellim0 < NCELLIN && cellim >= NCELLIN && cstats.nannuli_c > NSYNCANN) {
		float delta;			/* amount of missing flux */
		float mflux;			/* flux as measured */
		float pflux;			/* flux in pixels omitted from outer
								 parts as part of inner region */
       
		mflux = cstats.cells[0].sum;
		for(i = 1;i < NSYNCANN;i++) {
			for(j = 0;j < NSEC;j++) {
				cell = 1 + NSEC*(i-1) + j;
				mflux += cstats.cells[cell].sum;
			}
		}
		mflux *= 0.5;			/* we double count in the sinc region*/
       
		pflux = cstats.cells[NCELL].mean*cstats.cells[NCELL].ntot;
		delta = pflux - mflux;

		i = NSYNCANN;			/* and now this one */
		for(j = 0;j < NSEC;j++) {
			cstats.cells[1 + NSEC*(i - 1) + j].mean += delta/area[i];
		}
    }
/*
 * Were we asked to keep old profile it if extended beyond orad?
 */
    if(keep_profile) {
		if(nannuli_old > cstats.nannuli) {
			cstats.nannuli = nannuli_old;
		}
		if(nannuli_c_old > cstats.nannuli_c) {
			cstats.nannuli_c = nannuli_c_old;
		}
    }
/*
 * Write out file to help with debugging?
 */
    {
		static char *dump_filename = NULL; /* write data to this file?
											For use from gdb */
		if(dump_filename != NULL) {
			shRegWriteAsFits((REGION *)in, dump_filename,
							 STANDARD, 2, DEF_NONE, NULL, 0);
			dump_filename = NULL;
		}
    }

    return(&cstats);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return the last extracted profile
 */
CELL_STATS *
phProfileGetLast(void)
{
	return(&cstats);
}

/*****************************************************************************/
/*
 * Look up a cellid given a pixel's coordinates; (0,0) is object's centre
 */
int phGetCellid(int r, int c)
{
    r += SYNCRAD;
    c += SYNCRAD;
    
    if (r < 0 || r >= 2*SYNCRAD + 1 || c < 0 || c >= 2*SYNCRAD + 1) {
		return -1;
    }

    if (cellid == NULL) {
		phInitProfileExtract();
    }

    return cellid[r][c].c_id;
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Would a phProfileExtract call succeed in at least extracting the
 * profile of the central, sinc-shifted, part of the object?
 */
int
phProfileExtractOk(const REGION *in,	/* input region */
				   double fyc,		/* row centre of object
									 relative to REGION's origin */
				   double fxc)		/* col centre of object
									 relative to REGION's origin */
{
	int iyc, ixc;
	const int yrin = SYNCRAD + SYNCEXTRA + SYBELL - 1;

	if(in == NULL) return(0);
	shAssert(in->type == TYPE_PIX);

	iyc = (int)fyc; ixc = (int)fxc;
   
	if(ixc < yrin || ixc + yrin >= in->ncol ||
	   iyc < yrin || iyc + yrin >= in->nrow) {
		return(0);
	}
   
	return(1);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Return a CELL_STATS containing information geometry for all cells within a
 * specified radius; called during setup of (e.g.) measure objects
 */
const CELL_STATS *
phProfileGeometry(void)
{
	static CELL_STATS cstats;		/* return value */

	cstats.id = -1;
	cstats.annular = 1;
	cstats.col_c = cstats.row_c = 0;
	cstats.orad = -1;
	cstats.syncreg = NULL;

	cstats.radii = annrad;
	cstats.area = area;
	cstats.cells = NULL;
	cstats.geom = cellgeom;
	cstats.mgeom = NULL;

	cstats.ncell = NCELL;
   
	return(&cstats);
}

/*****************************************************************************/
/*
 * Calculate an object's moments from a CELL_STATS, returning <row^2 + col^2>
 */
float
phSigma2GetFromProfile(const CELL_STATS *cprof, /* extracted profile */
					   float *sigma_rr,	/* <row^2>, or NULL */
					   float *sigma_cc)	/* <col^2>, or NULL */
{
	float cflux;				/* flux within a cell */
	int i;
	float sigma_rc_s, *sigma_rc = &sigma_rc_s; /* <row col> */
	float sigma_rr_s, sigma_cc_s;	/* space for sigma_{rr,cc} if needed */
	double sum;				/* sums of intensity */
	double sum_rr, sum_rc, sum_cc;	/*          and intensity*[]rc]*[rc] */

	if(sigma_rr == NULL) sigma_rr = &sigma_rr_s;
	if(sigma_cc == NULL) sigma_cc = &sigma_cc_s;

	shAssert(cprof != NULL && cprof->annular == 1 && cprof->nannuli_c >= 1);

	sum = sum_rr = sum_rc = sum_cc = 0;
	for(i = 0; i < 1 + NSEC*(cprof->nannuli_c - 1); i++) {
		cflux = cprof->cells[i].mean*cprof->cells[i].area;
		sum += cflux;
		sum_rr += cflux*cprof->mgeom[i].row2;
		sum_rc += cflux*cprof->mgeom[i].colrow;
		sum_cc += cflux*cprof->mgeom[i].col2;
	}

	shAssert(sum != 0);

	*sigma_rr = sum_rr/sum;
	*sigma_rc = sum_rc/sum;
	*sigma_cc = sum_cc/sum;

	return((*sigma_rr + *sigma_cc)/2);
}

/*****************************************************************************/
/*
 * Extract a 1-d profile from a linear feature
 *
 * First a static function to do the work
 */
#define STEP(X,Y)			/* advance an Bresenham step */ \
	a -= dsml;												\
	if (a >= 0) {											\
		X += strx;											\
		Y += stry;											\
	} else {												\
		X += diagx;											\
		Y++;												\
		a += dlg;											\
	}

static void
setup_bresenham(int ix0, int iy0, int ix1, int iy1, /* end points */
				int *x0, int *y0, int *x1, int *y1, /* end points, as permuted
													 for Bresenham's use */
				int *a, int *dsml,	/* Bresenham internals */
				int *dlg, int *diagx,	/* "  "   "   "   */
				int *strx, int *stry)	/* "  "   "   "   */
{
	int dx = ix1 - ix0;
	int dy = iy1 - iy0;

	if(dy >= 0) {		/* find which octant and assign accordingly */
		*y0 = iy0;
		*x0 = ix0;
		*y1 = iy1;
		*x1 = ix1;
	} else {
		*x0 = ix1;
		*y0 = iy1;
		*x1 = ix0;
		*y1 = iy0;
		dy = -dy;
		dx = -dx;
	}
	if(dx >= 0) {
		if(dy >= dx) {
			*a = dy/2;
			*dlg = dy;
			*dsml = dx;
			*strx = 0;
			*stry = 1;
			*diagx = 1;
		} else {
			*dlg = dx;
			*dsml = dy;
			*a = dx/2;
			*strx = 1;
			*stry = 0;
			*diagx = 1;
		}
	} else {
		if(dy >= -dx) {
			*a = dy/2;
			*dlg = dy;
			*dsml = -dx;
			*strx = 0;
			*stry = 1;
			*diagx = -1;
		} else {
			*dlg = -dx;
			*dsml = dy;
			*a = -dx/2;
			*strx = -1;
			*stry = 0;
			*diagx = -1;
		}
	}
}

/*
 * trace out a single line in the image. We are guaranteed that dy >= 0
 */
static PIX *
lextract_pixels(PIX **const rows,	/* rows of region */
				int nrow, int ncol,	/* size of region */
				PIX *cache,		/* where to put the region's pixels */
				int x, int y,		/* start of line */
				int xend, int yend,	/* end of line */
				int a, int dsml, int dlg, /* Bresenham internals */
				int strx, int stry, int diagx /* "  "   "   "   */
	)
{
	int j;				/* how many steps have we taken? */

	j = 0;
/*
 * We may be below the region, so fix that
 */
	if(y < 0) {
		for(;j <= dlg;j++) {
			STEP(x,y);
			if(y >= 0) {
				break;
			}
		}
	}
/*
 * We may be to the left or right of region, so fix _that_
 */
	if(x < 0) {
		if(diagx < 0) {			/* going left; we'll never hit region*/
			return(cache);
		}
		for(;j <= dlg;j++) {
			STEP(x,y);
			if(x >= 0) {
				break;
			}
		}
	} else if(x >= ncol) {
		if(diagx > 0) {			/* going right; we'll never hit it */
			return(cache);
		}
		for(;j <= dlg;j++) {
			STEP(x,y);
			if(x < ncol) {
				break;
			}
		}
	}
/*
 * Check if we're above region; if so, we're done
 */
	if(y >= nrow) return(cache);
/*
 * We are in the interior or on the left, bottom, or right boundary
 */
	if(diagx > 0) {			/* going right */
		if(xend < ncol && yend < nrow) {	/* easy; no boundaries to check */
			for(;j <= dlg;j++) {
#if 0
				shAssert(y >= 0 && y < nrow && x >= 0 && x < ncol);
#endif
				trace("Grabbing pixel (x,y) = (%i,%i) = %i\n", x, y, (int)rows[y][x]);
				*cache++ = rows[y][x];
				STEP(x,y);
			}
		} else {
			for(;j <= dlg;j++) {
#if 0
				shAssert(y >= 0 && y < nrow && x >= 0 && x < ncol);
#endif
				trace("Grabbing pixel (x,y) = (%i,%i) = %i\n", x, y, (int)rows[y][x]);
				*cache++ = rows[y][x];
				STEP(x,y);
				if(y == nrow || x == ncol) break;
			}
		}
	} else {				/* going left */
		if(xend >= 0 && yend < nrow) {	/* easy; no boundaries to check */
			for(;j <= dlg;j++) {
#if 0
				shAssert(y >= 0 && y < nrow && x >= 0 && x < ncol);
#endif
				trace("Grabbing pixel (x,y) = (%i,%i) = %i\n", x, y, (int)rows[y][x]);
				*cache++ = rows[y][x];
				STEP(x,y);
			}
		} else {
			for(;j <= dlg;j++) {
#if 0
				shAssert(y >= 0 && y < nrow && x >= 0 && x < ncol);
#endif
				trace("Grabbing pixel (x,y) = (%i,%i) = %i\n", x, y, (int)rows[y][x]);
				*cache++ = rows[y][x];
				STEP(x,y);
				if(y == nrow || x == -1) break;
			}
		}
	}

	return(cache);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Extract a linear profile
 */
CELL_STATS *
phProfileExtractLinear(int id,		/* ID for this profile */
					   const REGION *in,
					   double fy0, double fx0,	/* left end of object */
					   double fy1, double fx1,	/* right end of object */
					   int hwidth,		/* desired half-width of
										 profile */
					   int bin,			/* width of bins; if 0 use the
										 ones defined by anndex[]*/
					   double sky,		/* sky level */
					   double skysig		/* sky sigma */
	)
{
	int ann;				/* index into anndex[] */
	PIX *cache;				/* cache for pixels in a cell */
	struct pstats *cinfo;
	static CELL_STATS cstats;		/* return value */
	float delta;				/* how far we are from ridgeline */
	int bounds[NANN + 1];		/* indices of boundaries of cells */
	float obound;			/* outer edge of outermost bin */
	int i;
	int diagx,dlg,dsml;			/* used by Bresenham's algorithm */
	int a,strx,stry;			/*   "  "    "   "       "   "   */
	int nbound;				/* number of values in bounds */
	int nrow, ncol;			/* in->{nrow,ncol} unaliased */
	int skyfloor = sky - 5.*skysig;	 /* sky - 5*sigma should be safe, but
									  region_maxmean_crude() checks */
	float theta;				/* angle of ridgeline to columns */
	int x0_r, y0_r, x1_r, y1_r;		/* integer endpoints of ridgeline */
	int x0, y0, x1, y1;			/*  "  "     "   "   "  line */

	shAssert(in != NULL);
	shAssert(in->type == TYPE_PIX);
	shAssert(cellinfo != NULL);
	ncol = in->ncol; nrow = in->nrow;
/*
 * See which index in anndex corresponds to hwidth if we are using anndex[]
 */
	if(bin == 0) {
		for(i = 0;hwidth > anndex[i];i++) continue;
		obound = (i == 0) ? 0 : anndex[i] - 0.5; /* first r beyond limit */
	} else {
		shAssert(bin > 0);
		obound = hwidth/bin;
	}
/*
 * Set up for Bresenham's algorithm
 */
	setup_bresenham(fx0, fy0, fx1, fy1, &x0_r, &y0_r, &x1_r, &y1_r,
					&a, &dsml, &dlg, &diagx, &strx, &stry);

	if(x0_r == x1_r && y0_r == y1_r) {
		shErrStackPush("Cannot extract profile between two coincident points");
		return(NULL);
	}
	theta = atan2(fy1 - fy0,fx1 - fx0);
/*
 * Find out where the cell boundaries are. They'd be at anndex[i] if the
 * ridgeline were vertical or horizontal, but in general it isn't.
 */
	if(bin == 0) {
		ann = 0;
		for(i = 0;;i++) {
			delta = fabs((i + 0.5)*(stry == 1 ? sin(theta) : cos(theta)));
			if(delta > anndex[ann]) {
				bounds[ann] = i;
				if(delta > obound) break;
				ann++;
			}
		}
		if(ann == 1) {
			ann++;
			bounds[ann] = i + 1;
		}
	} else {
		ann = 0;
		bounds[ann++] = 0;
		for(i = bin/2 + 1;i <= hwidth;i += bin) {
			bounds[ann++] = i;
			if(ann >= NANN) {
				break;
			}
		}
		if(ann > 1) ann--;

		if(ann == 1) {
			ann++;
			bounds[ann] = bounds[ann-1] + bin;
		}
	}
	nbound = ann;
/*
 * Now set the bits corresponding to that line. First on one side...
 */
	ann = 0;				/* index into anndex */
	cinfo = &cellinfo[nbound - 1];
	cache = cinfo->data = cellval[0];
	for(i = -bounds[1] + 1;;i++) {
		if(stry == 1) {			/* nearer the y- than the x- axis */
			x0 = x0_r + i;
			y0 = y0_r;
			x1 = x1_r + i;
			y1 = y1_r;
		} else {
			x0 = x0_r;
			y0 = y0_r + i;
			x1 = x1_r;
			y1 = y1_r + i;
		}
		if(i >= bounds[ann + 1]) {
			if((cinfo->ntot = cache - cinfo->data) > 0) {
				do_pstats(cinfo, skyfloor, -1, fx0, fy0, NULL);
			}
			if(++ann == nbound) break;

			cinfo = &cellinfo[nbound + ann - 1];
			cache = cinfo->data = cellval[0];
		}

		cache = lextract_pixels(in->ROWS, nrow, ncol, cache, x0, y0, x1, y1,
								a, dsml, dlg, strx, stry, diagx);
	}
/*
 * ... and then on the other
 */
	ann = 1;				/* index into anndex */
	cinfo = &cellinfo[nbound - 1 - ann];
	cache = cinfo->data = cellval[0];
	for(i = bounds[1];;i++) {
		if(stry == 1) {			/* nearer the y- than the x- axis */
			x0 = x0_r - i;
			y0 = y0_r;
			x1 = x1_r - i;
			y1 = y1_r;
		} else {
			x0 = x0_r;
			y0 = y0_r - i;
			x1 = x1_r;
			y1 = y1_r - i;
		}
		if(i >= bounds[ann + 1]) {
			if((cinfo->ntot = cache - cinfo->data) > 0) {
				do_pstats(cinfo, skyfloor, -1, fx0, fy0, NULL);
			}
			if(++ann == nbound) break;

			cinfo = &cellinfo[nbound - 1 - ann];
			cache = cinfo->data = cellval[0];
		}

		cache = lextract_pixels(in->ROWS,nrow, ncol, cache, x0, y0, x1, y1,
								a, dsml, dlg, strx, stry, diagx);
	}
/*
 * set return values
 */
	cstats.id = id;
	cstats.nannuli = cstats.ncell = 2*nbound - 1;
	cstats.nannuli_c = cstats.annular = 0;
	cstats.col_c = fx0; cstats.row_c = fy0;
	cstats.col_1 = fx1; cstats.row_1 = fy1;
	cstats.syncreg = NULL;
	cstats.radii = NULL;
	cstats.cells = cellinfo;
	cstats.geom = lcellgeom;
	cstats.mgeom = lcellmod;
/*
 * fill out geometry information
 */
	for(i = 0;i < nbound;i++) {
		ann = (nbound - 1) - i;
		if(bin == 0) {
			lcellmod[i].col = (ann == 0) ?
				0 : -((anndex[ann] + anndex[ann+1])/2.0-0.5);
		} else {
			lcellmod[i].col = (ann == 0) ?
				0 : -((bounds[ann] + bounds[ann+1])/2.0-0.5);
		}
		lcellmod[i].row = 0;
		lcellmod[i].rmean = 0;

		lcellmod[cstats.ncell - i - 1] = lcellmod[i];
		lcellmod[cstats.ncell - i - 1].col = -lcellmod[i].col;
	}

	for(i = 0;i < cstats.ncell;i++) {
		lcellgeom[i].n = cellinfo[i].ntot;
	}

	return(&cstats);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Extract a cross-section through an image, of specified width, and return
 * it as an array. Memory for this array is static, and should _not_ be
 * freed; it's overwritten at the next call to an extract function.
 *
 * The array's index corresponds to single pixel steps along the specified
 * ridgeline; if it's more nearly aligned with the columns, these steps will
 * be in y (i.e. along a column), otherwise they'll be along a row (i.e. in x).
 * 
 * The starting index is the endpoint nearest to the bottom left of the
 * frame, cast to int.
 *
 * Note that there will be some blurring along the direction of the ridgeline;
 * I could fix this, but I haven't. It's good enough for finding the location
 * of linear features, and that's what it was written for.
 *
 * There are no problems parallel to the row/column direction
 */
float *
phXsectionExtract(const REGION *in,
				  double fy0, double fx0, /* left end of section */
				  double fy1, double fx1, /* right end of section */
				  int hwidth,		/* desired half-width */
				  int *nval		/* number of points in cross section */
	)
{
	int i,j;
	PIX *cache;				/* cache for pixels in a cell */
	float delta;				/* how far we are from ridgeline */
	int diagx,dlg,dsml;			/* used by Bresenham's algorithm */
	int a,strx,stry;			/*   "  "    "   "       "   "   */
	int nrow, ncol;			/* in->{nrow,ncol} unaliased */
	float *section;			/* the extracted cross-section */
	float theta;				/* angle of ridgeline to columns */
	int x0_r, y0_r, x1_r, y1_r;		/* integer endpoints of ridgeline */
	int x0, y0, x1, y1;			/*  "  "     "   "   "  line */

	shAssert(in != NULL);
	shAssert(in->type == TYPE_PIX);
	shAssert(cellinfo != NULL);
	ncol = in->ncol; nrow = in->nrow;
/*
 * Set up for Bresenham's algorithm
 */
	setup_bresenham(fx0, fy0, fx1, fy1, &x0_r, &y0_r, &x1_r, &y1_r,
					&a, &dsml, &dlg, &diagx, &strx, &stry);

	if(x0_r == x1_r && y0_r == y1_r) {
		shErrStackPush("Cannot extract profile between two coincident points");
		return(NULL);
	}
	theta = atan2(fy1 - fy0,fx1 - fx0);
/*
 * Find out where the edge of the swath is; it'd be at hwidth if the
 * ridgeline were vertical or horizontal, but in general it isn't.
 */
	for(i = 0;;i++) {
		delta = fabs((i + 0.5)*(stry == 1 ? sin(theta) : cos(theta)));
		if(delta > hwidth) {
			break;
		}
	}
	hwidth = i;
/*
 * Now extract those pixels into the cross section
 */
	section = (float *)cellval[0];
	cache = cellval[0] + 4000*sizeof(float)/sizeof(PIX);
	shAssert(nrow + ncol <
	    	 4000*sizeof(float)/sizeof(PIX)); /* really hypot(ncol,nrow) */

	(void)lextract_pixels(in->ROWS,nrow, ncol, cache, x0_r, y0_r, x1_r, y1_r,
					      a, dsml, dlg, strx, stry, diagx);
	for(j = 0;j < dlg;j++) {
		section[j] = cache[j];
	}

	for(i = -hwidth + 1;i < hwidth;i++) {
		if(i == 0) continue;		/* we did it already */

		if(stry == 1) {			/* nearer the y- than the x- axis */
			x0 = x0_r + i;
			y0 = y0_r;
			x1 = x1_r + i;
			y1 = y1_r;
		} else {
			x0 = x0_r;
			y0 = y0_r + i;
			x1 = x1_r;
			y1 = y1_r + i;
		}
		(void)lextract_pixels(in->ROWS, nrow, ncol, cache, x0, y0, x1, y1,
							  a, dsml, dlg, strx, stry, diagx);
		for(j = 0;j < dlg;j++) {
			section[j] += cache[j];
		}
	}

	for(j = 0;j < dlg;j++) {
		section[j] /= (2*hwidth - 1);
	}

	*nval = dlg;
	return(section);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Create an object wherever the cell in a profile is above a given
 * threshold. Assumes that the profile is circular --- but see also
 * phObjectNewFromProfileLinear
 */
#define INCR_NSPAN										\
	if(++nspan >= nalloc) {								\
		nalloc *= 2;									\
		span0 = shRealloc(span0,nalloc*sizeof(SPAN));	\
		span = &span0[nspan];							\
	}


OBJECT *
phObjectNewFromProfile(
	const CELL_STATS *cstats, /* extracted profile */
	int ncol, int nrow, /* size of REGION in which
						 object is embedded */
	float thresh,	/* set bits if profile exceeds thresh*/
	int set_sectors,	/* no. of sectors above threshold
						 to mask the entire annulus*/
	int clip		/* if >= 0, calculate clipped mean for
					 annulus, and compare this to thresh
					 to mask entire annulus */
	)
{
	int set[NCELL];			/* cells that we want to set */
	int i,j;
	int sdy_l, sdy_u;			/* These are the indexes into cellorig
								 based on which y-subpixel the
								 object's centre lies in, for the
								 lower and upper half of the object*/
	int rlim,rlimyl,rlimyu;
	int srlim;
	int annlim;				/* first annulus beyond limit */
	int cellim;				/* first cell beyond limit */
	int cxext;
	int lxext,rxext;			/* extents left and right */
	int slxext,srxext;			/* scaled extents left and right */
	int iyc,ixc,sxc;
	int slen,sxorig;
	int xorig;
	int corig;
	register int len,cell;
	struct cellorig *pcell;
	int iscr;
	int ccareful = 0;
	int lcareful = 0;			/* flags to be careful at edges */
	int rcareful = 0;
/*
 * span stuff
 */
	int nspan;				/* number of SPANs used */
	int nalloc;				/* number of SPANs allocated */
	OBJECT *obj;
	SPAN *span0;				/* base of SPAN array */
	SPAN *span;				/* pointer to current SPAN */
	int y, x1;				/* the starting coordinates of a SPAN*/

	shAssert(cstats != NULL);
	shAssert(cstats->annular);

	nalloc = nrow;
	span0 = shMalloc(nalloc*sizeof(SPAN));
	nspan = 0; span = span0;
/*
 * find outermost annulus that we need to process given orad
 */
	for(annlim = 0;cstats->orad > anndex[annlim];annlim++) continue;
	rlim = anndex[annlim];		/* first radius beyond limit--ie
								 r < rlim */
	srlim = rlim<<YSHFT;			/* scaled limiting radius */
	cellim = (annlim-1)*NSEC + 1;	/* first cell beyond limit */
/*
 * See which cells to set; we only want contiguous cells above thresh
 */
	if(cstats->cells[0].qt[1] < thresh) {
		shFree(span0);
		return(phObjectNew(1));
	}

	set[0] = 1;
	if(clip >= 0) {
		for(i = 0;i < annlim - 1;i++) {	/* sort sector values */
			if(phProfileMedian(cstats,i,clip,0,NULL) >= thresh) {
				for(j = 0;j < NSEC;j++) {
					set[i*NSEC + j + 1] = 1;
				}
			} else {
				for(j = i*NSEC + 1;j < cellim;j++) {
					set[j] = 0;
				}
				break;
			}
		}
	} else {
		for(cell = 1;cell < cellim;cell++) {
			if(cstats->cells[cell].qt[1] < thresh) {
				set[cell] = 0;
				continue;
			}
			if(cell <= 12) {		/* touches central cell, which is on */
				set[cell] = 1;
				continue;
			}

			set[cell] = set[cell - NSEC];	/* contiguity condition */
		}

		if(set_sectors > 0) {		/* they wanted to set entire annulus */
			int nset;			/* number of set sectors in annulus */
	 
			for(i = 0;i < annlim - 1;i++) {
				nset = 0;
				for(j = 0;j < NSEC;j++) {
					if(i*NSEC + j + 1 >= cellim) {
						fprintf(stderr,"Index too high\n");
						break;
					}
					nset += (set[i*NSEC + j + 1]);
				}
				if(nset >= set_sectors) {
					for(j = 0;j < NSEC;j++) {
						set[i*NSEC + j + 1] = 1;
					}
				} else {
					for(j = i*NSEC + 1;j < cellim;j++) {
						set[j] = 0;
					}
					break;
				}
			}
		}
	}
/*
 * setup subpixellation. We assume that the object is centred on a pixel,
 * so (sxc,syc)%YSCL = (7.5, 7.5)
 */
    iyc = (int)cstats->row_c;
    sdy_u = YSCL*0.5;			/* for upper half */
    shAssert(sdy_u >= 0 && sdy_u < YSCL);
    sdy_l = YSCL - sdy_u - 1;		/* for lower half */
    shAssert(sdy_u != sdy_l);

    rlimyl = rlimyu = rlim;
    if(rlimyl > iyc) rlimyl = iyc;
    if(rlimyu > (iscr = nrow - iyc - 1)) rlimyu = iscr;

    ixc = (int)cstats->col_c;
    sxc = ixc*YSCL + 0.5 ;		/* nearest subpixel center */
/*
 * check on x edges; if the region intersects an edge, various
 * flags are set which can be checked later with no computation.
 * the flags are rcareful, which is set if the object overlaps
 * the right edge of the region, lcareful the left, and ccareful
 * if the central sectors might intersect the edge.
 */
	cxext = (rlim*SSEC1)>>8; /* approximate x-extent of sectors 3,9 */
	lxext = rxext = rlim;
	if(lxext > ixc){
		lxext = ixc;
		lcareful = 1;
		if(ixc < cxext) ccareful = 1;
	}
	slxext = lxext*YSCL;
	if(rxext > (iscr = ncol - ixc - 1)){
		rxext = iscr;
		rcareful = 1;
		if(iscr < cxext) ccareful = 1;
	}
	srxext = rxext*YSCL;
/*
 * Find the spans that are above threshold
 *
 * First the outer region, if it's needed. We could do this by starting at the
 * central column and working to the right, then returning to the centre and
 * working left, but this isn't quite right as there's the sector that contains
 * the central column to worry about. This is the `central' column, and doesn't
 * exist if the line that we are processing passes through the central synced
 * region.
 */
    if(cellim >= NCELLIN) {
/*
 * First the lower half (including the central row)
 */
		for(i = 0;i < rlimyl;i++) {
			pcell = cellorig[sdy_l][i];
			y = iyc - i;
/*
 * Process the lower half, central segment, if it exists
 */
			if((corig = pcell[0].c_orig) < 0) {
				cell = pcell[0].c_lrcell;
				if(set[cell]) {
					sxorig = (sxc + corig + YSCL - 1);
					xorig = ((sxorig + srlim)>>YSHFT) - rlim;  /* positivity */
					len  = ((sxorig + (pcell[0].c_len))>>YSHFT) - xorig;
					x1 = xorig;
					if(ccareful){
						if(xorig < 0){
							len += xorig;
							x1 = 0;
						}else{
							if((iscr = xorig+len-ncol) > 0) len -= iscr;
						}
					}

					span->y = y;
					span->x1 = x1;
					span->x2 = x1 + len - 1;
	     
					INCR_NSPAN;
					span = &span0[nspan];
				}

				pcell++;			/* we don't want to see the central
									 cell again */
			}
/*
 * Now the lower half, right side
 */
			for(j = 0;(corig = pcell[j].c_orig) < srxext &&
					(cell = pcell[j].c_lrcell) < cellim;j++) {
				if(set[cell]) {
					sxorig = (sxc + corig + YSCL - 1);
					xorig = (sxorig >> YSHFT);
					len  = ((sxorig + (pcell[j].c_len))>>YSHFT) - xorig;
					x1 = xorig;
					if(rcareful){
						if((iscr=xorig+len-ncol) > 0) len -= iscr;
					}

					span->y = y;
					span->x1 = x1;
					span->x2 = x1 + len - 1;

					INCR_NSPAN;
					span = &span0[nspan];
				}
			}
/*
 * and the lower half, left side
 */
			for(j = 0;(corig = pcell[j].c_orig) < slxext &&
					(cell=pcell[j].c_llcell) < cellim;j++) {
				if(set[cell]) {
					slen = pcell[j].c_len;
					sxorig = (sxc - corig - slen + YSCL);
					xorig = ((sxorig+srlim)>>YSHFT) - rlim; /* positivity*/
					len  = ((sxorig + slen)>>YSHFT) - xorig;
					x1 = xorig;
					if(lcareful){
						if(xorig < 0){
							len += xorig;
							x1 = 0;
						}
					}
		
					span->y = y;
					span->x1 = x1;
					span->x2 = x1 + len - 1;

					INCR_NSPAN;
					span = &span0[nspan];
				}
			}
		}
/*
 * Now the upper half, again central, right, and left
 */
		for(i = 1;i < rlimyu;i++) {
			pcell = cellorig[sdy_u][i];
			y = iyc + i;
/*
 * Central segment if it exists
 */
			if((corig = pcell[0].c_orig) < 0) {
				cell = pcell[0].c_urcell;
				if(set[cell]) {
					sxorig = (sxc + corig + YSCL - 1);
					xorig = ((sxorig + srlim)>>YSHFT) - rlim;  /* positivity */
					len  = ((sxorig + (pcell[0].c_len))>>YSHFT) - xorig;
					x1 = xorig;
					if(ccareful){
						if(xorig < 0){
							len += xorig;
							x1 = 0;
						}else{
							if((iscr = xorig+len-ncol) > 0) len -= iscr;
						}
					}

					span->y = y;
					span->x1 = x1;
					span->x2 = x1 + len - 1;

					INCR_NSPAN;
					span = &span0[nspan];
				}
				pcell++;
			}
/*
 * Now the upper half, right side
 */
			for(j = 0;(corig = pcell[j].c_orig) < srxext &&
					(cell=pcell[j].c_urcell) < cellim;j++) { /* in bounds */
				if(set[cell]) {
					sxorig = (sxc + corig + YSCL - 1);   /* is this right ?? */
					xorig = (sxorig >> YSHFT);
					len  = ((sxorig + (pcell[j].c_len))>>YSHFT) - xorig;
					x1 = xorig;
		
					if(rcareful){
						if((iscr=xorig+len-ncol) > 0) len -= iscr;
					}
		
					span->y = y;
					span->x1 = x1;
					span->x2 = x1 + len - 1;

					INCR_NSPAN;
					span = &span0[nspan];
				}
			}
/*
 * and the upper half, left side
 */
			for(j = 0;(corig = pcell[j].c_orig) < slxext &&
					(cell = pcell[j].c_ulcell) < cellim;j++) { /* in bounds */
				if(set[cell]) {
					slen = pcell[j].c_len;
					sxorig = (sxc - corig - slen + YSCL);
					xorig = ((sxorig+srlim)>>YSHFT) - rlim; /* positivity*/
					len  = ((sxorig + slen)>>YSHFT) - xorig;
					x1 = xorig;
					if(lcareful) {
						if(xorig < 0){
							len += xorig;
							x1 = 0;
						}
					}
		
					span->y = y;
					span->x1 = x1;
					span->x2 = x1 + len - 1;

					INCR_NSPAN;
					span = &span0[nspan];
				}
			}
		}
    }
/*
 * done--now do inner region. We have to be a little careful, as there's
 * no guarantee that the inner region is entirely within the REGION
 */
	xorig = ixc - (unsigned)SYNC_REG_SIZE/2;
	for(i=0;i < SYNC_REG_SIZE;i++) {
		struct cellid *cells = cellid[i];
		int in_span;			/* are we currently in a span? */
		int j0, j1;			/* range of j values keeping us
							 within the REGION */
      
		if((y = iyc - (unsigned)SYNC_REG_SIZE/2 + i) < 0 || y >= nrow) {
			continue;
		}
      
		in_span = 0;
		j0 = (xorig <= 0) ? 1 - xorig : 0;
		j1 = (xorig + SYNC_REG_SIZE >= ncol) ? ncol - xorig : SYNC_REG_SIZE;
		for(j = j0;j < j1;j++) {
			if((cell = cells[j].c_id) >= NCELLIN) { /* not in sync region */
				if(in_span) {
					span->x2 = xorig + j - 1;
					INCR_NSPAN;
					span = &span0[nspan];
					in_span = 0;
				}
				continue;
			}
			if(set[cell]) {
				if(!in_span) {
					span->y = y;
					span->x1 = xorig + j;
					in_span = 1;
				}
			} else if(in_span) {
				span->x2 = xorig + j - 1;
				INCR_NSPAN;
				span = &span0[nspan];
				in_span = 0;
			}
		}
		if(in_span) {
			span->x2 = xorig + j - 1;
			INCR_NSPAN;
			span = &span0[nspan];
			in_span = 0;
		}
	}
/*
 * pack up the SPANs into an OBJECT
 */
	obj = phObjectNew(1);
	obj->sv[0]->nspan = obj->sv[0]->size = nspan;
	obj->sv[0]->s = shRealloc(span0,nspan*sizeof(SPAN));
	phCanonizeObjmask(obj->sv[0],0);

	return(obj);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Set mask bits wherever a cell is above a given threshold. Assumes that
 * the profile is circular --- but see also phObjmaskSetFromProfileLinear
 */
OBJMASK *
phObjmaskSetFromProfile(int ncol,	/* size of REGION in which */
						int nrow,	/*                 the OBJMASK lives */
						const CELL_STATS *cstats, /* extracted profile */
						float thresh,	/* set bits if profile exceeds thresh*/
						int set_sectors, /* no. of sectors above threshold
										  to mask the entire annulus*/
						int clip	/* if >= 0, calculate clipped mean for
									 annulus, and compare this to thresh
									 to mask entire annulus */
	)
{
	OBJECT *obj;				/* the OBJECT set from profile */
	OBJMASK *sm;				/* OBJMASK to return */
   
	shAssert(cstats != NULL);

	obj = phObjectNewFromProfile(cstats,ncol,nrow,thresh,set_sectors,clip);
	sm = obj->sv[0]; obj->sv[0] = NULL;

	phObjectDel(obj);

	return(sm);
}

/*****************************************************************************/
/*
 * Set mask bits for an area along a specified line
 *
 * First a utility functions to trace the left and right side
 * of a masked region
 */
static int
do_edge(SPAN *span, int nrow, int ncol,
		int xb, int yb, int xl, int yl, int left_edge)
{
	int a, dsml, dlg;			/* Bresenham internals */
	int strx, stry, diagx;		/* "  "   "   "   */
	int j;
	int old_y;
	const SPAN *span0 = span;		/* initial value of span */
	int x0, x1, y0, y1;
	int x;				/* clipped version of x0 */

	setup_bresenham(xb, yb, xl, yl, &x0, &y0, &x1, &y1,
					&a, &dsml, &dlg, &diagx, &strx, &stry);
	old_y = y0 - 1;
	for(j = 0;j <= dlg;j++) {
		if(y0 != old_y) {
			if(y0 >= 0 && y0 < nrow) {
				x = (x0 < 0) ? 0 : ((x0 >= ncol) ? ncol - 1 : x0);

				if(left_edge) {
					span->y = y0;
					span->x1 = x;
				} else {
					if(y0 != span->y) {
						fprintf(stderr,"Span %d:%d %d  bad y %d\n",
								span->y,span->x1,span->x2,y0);
					}
					span->x2 = x;
				}
				span++;
				old_y = y0;
			}
		}
		STEP(x0,y0);
	}
   
	return(span - span0);
}

/*
 * <AUTO EXTRACT>
 * Set mask bits along a specified ridgeline.
 *
 * If fy1 == fy2, we take the left of the region to be above; otherwise
 * left and right are unambiguous
 */
void
phObjmaskSetLinear(OBJMASK *mask,	/* mask to set */
				   int ncol, int nrow,	/* size of REGION mask lives in */
				   float fx0, float fy0, /* one end of ridgeline... */
				   float fx1, float fy1, /* and the other */
				   int hwidth_l,	/* half-width of region to mask 
									 to left of ridgeline */
				   int hwidth_r		/* half-width of region to mask
									 to right of ridgeline */
    )
{
	int i;
	int nspan;				/* number of spans set by do_edge */
	SPAN *newspans;			/* spans set by this function */
	int newsize;				/* size of added SPANs */
	int npix;				/* number of pixels in OBJMASK */
	float theta;				/* angle of ridgeline to columns */
	int x0, y0;				/* start of line */
	int x1, y1;				/* end of line */
	int xs, ys, xe, ye;			/* start and end points of a line */

	shAssert(mask != NULL);
/*
 * The ridgeline runs from x0 to x1, and we wish to mask the area
 * within the lines
 *
 *                  _xt
 *                 |  |__
 *                _|	 |x1
 *               |       .  |__
 *              _|      .      |xr
 *             |       .      _|
 *            _|      .      |
 *           |	     .      _|
 *          _|	    .      |
 *         |  	   .      _|
 *        _|      .      |
 *     xl|       .      _|
 *       |__    .      |
 *          |_x0      _|
 *             |__   |
 *                |__|xb
 *
 * find the bottom point that we wish to mask (xb) and work along the left
 * and bottom boundaries, filling the y and x1 fields of a SPAN for each row.
 * When we reach the top (xt) we return to xb, and work up the right and top
 * sides filling the x2 field
 */
	if(fy0 < fy1) {
		x0 = fx0 + 0.5;
		y0 = fy0 + 0.5;
		x1 = fx1 + 0.5;
		y1 = fy1 + 0.5;
		theta = atan2(fy1 - fy0,fx1 - fx0);
	} else {
		x0 = fx1 + 0.5;
		y0 = fy1 + 0.5;
		x1 = fx0 + 0.5;
		y1 = fy0 + 0.5;
		theta = atan2(fy0 - fy1,fx0 - fx1);
	}
/*
 * figure out how many SPANs we need
 */
	if(theta < M_PI/2) {
		ys = y0 - hwidth_r*cos(theta) + 0.5; ye = y1 + hwidth_l*cos(theta) + 0.5;
	} else {
		ys = y0 + hwidth_l*cos(theta) + 0.5; ye = y1 - hwidth_r*cos(theta) + 0.5;
	}
	newsize = ye - ys + 1;
	if(mask->nspan + newsize > mask->size) {
		phObjmaskRealloc(mask,mask->nspan + newsize);
	}
	newspans = &mask->s[mask->nspan];	/* where to start setting spans */
/*
 * and fill them.
 */
	if(theta < M_PI/2) {
		xs = x0 + hwidth_r*sin(theta) + 0.5; ys = y0 - hwidth_r*cos(theta) + 0.5;
		xe = x0 - hwidth_l*sin(theta) + 0.5; ye = y0 + hwidth_l*cos(theta) + 0.5;
	} else {
		xs = x0 - hwidth_l*sin(theta) + 0.5; ys = y0 + hwidth_l*cos(theta) + 0.5;
		xe = x1 - hwidth_l*sin(theta) + 0.5; ye = y1 + hwidth_l*cos(theta) + 0.5;
	}
	nspan = do_edge(&newspans[0], nrow, ncol, xs, ys, xe, ye, 1);
	if(nspan > 0 && ye < nrow) nspan--;	/* we'll see that point again */
	shAssert(nspan <= newsize);

	xs = xe; ys = ye;
	if(theta < M_PI/2) {
		xe = x1 - hwidth_l*sin(theta) + 0.5; ye = y1 + hwidth_l*cos(theta) + 0.5;
	} else {
		xe = x1 + hwidth_r*sin(theta) + 0.5; ye = y1 - hwidth_r*cos(theta) + 0.5;
	}
	nspan += do_edge(&newspans[nspan], nrow, ncol, xs, ys, xe, ye, 1);
	shAssert(nspan <= newsize);

	newsize = nspan;			/* we know its value now */
	mask->nspan += newsize;
/*
 * and now the right-hand-sides
 */
	if(theta < M_PI/2) {
		xs = x0 + hwidth_r*sin(theta) + 0.5; ys = y0 - hwidth_r*cos(theta) + 0.5;
		xe = x1 + hwidth_r*sin(theta) + 0.5; ye = y1 - hwidth_r*cos(theta) + 0.5;
	} else {
		xs = x0 - hwidth_l*sin(theta) + 0.5; ys = y0 + hwidth_l*cos(theta) + 0.5;
		xe = x0 + hwidth_r*sin(theta) + 0.5; ye = y0 - hwidth_r*cos(theta) + 0.5;
	}
	nspan = do_edge(&newspans[0], nrow, ncol, xs, ys, xe, ye, 0);
	if(nspan > 0 && ye < nrow) nspan--;	/* we'll see that point again */
	shAssert(nspan <= newsize);

	xs = xe; ys = ye;
	if(theta < M_PI/2) {
		xe = x1 - hwidth_l*sin(theta) + 0.5; ye = y1 + hwidth_l*cos(theta) + 0.5;
	} else {
		xe = x1 + hwidth_r*sin(theta) + 0.5; ye = y1 - hwidth_r*cos(theta) + 0.5;
	}
	nspan += do_edge(&newspans[nspan], nrow, ncol, xs, ys, xe, ye, 0);
	shAssert(nspan == newsize);
/*
 * ensure that x2 >= x1, and increment npix appropriately
 */
	npix = mask->nspan;
	for(i = 0;i < newsize;i++) {
		SPAN *s = &newspans[i];
		xs = s->x1; xe = s->x2;
		if(xs > xe) {
			s->x1 = xe; s->x2 = xs;
		}
		npix += xe - xs + 1;
	}
	mask->npix = npix;
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Set mask bits wherever a cell is above a given threshold. Assumes that
 * the profile is linear --- but see also phMaskSetFromProfile
 */
OBJMASK *
phObjmaskSetFromProfileLinear(int ncol, int nrow,
							  const CELL_STATS *cstats,
							  float thresh	/* threshold to use */
	)
{
	int i;
	int hwidth_l, hwidth_r;		/* half widths to left and right */
	OBJMASK *mask;			/* mask to set */

	shAssert(cstats != NULL && !cstats->annular);

	mask = phObjmaskNew(0);

	if(cstats->cells[cstats->ncell/2].qt[1] < thresh) { /* object's below
														 threshold */
		return(mask);
	}
   
	for(i = cstats->ncell/2;i >= 0;i--) {
		if(cstats->cells[i].qt[1] < thresh) {
			break;
		}
	}
	hwidth_l = (i >= 0) ? -cstats->mgeom[i].col : 10000;
   
	for(i = cstats->ncell/2 + 1;i < cstats->ncell;i++) {
		if(cstats->cells[i].qt[1] < thresh) {
			break;
		}
	}
	hwidth_r = (i < cstats->ncell) ? cstats->mgeom[i].col : 10000;
   
	phObjmaskSetLinear(mask, ncol, nrow, cstats->col_c, cstats->row_c,
					   cstats->col_1, cstats->row_1, hwidth_l, hwidth_r);

	return(mask);
}

/*****************************************************************************/
/*
 * Allocates memory for all the arrays used by the profile extractor
 */
static void
profallocv(void)
{
    int i;
    int vsiz;				/* total number of pixels in a sector*/
    int scr;
    int sizecache[NANN];		/* numbers of pixels in a single cell
								 in each annulus */

    if(cellinfo != NULL) {
		shErrStackPush("Profile Arrays Already Allocated");
		return;
    }
/*
 * allocate cell info struct array; the extra 1 is for a cell for all of the
 * pixels that lie in the central sinc shifted region, the "hole".
 */
    cellinfo = (struct pstats *)shMalloc((NCELL + 1)*sizeof(struct pstats));
    cellinfo[NCELL].data = shMalloc(SYNC_REG_SIZE*SYNC_REG_SIZE*sizeof(PIX));
    for (i=0; i<=NCELL; i++)
        cellinfo[i].flg = 0;
/*
 * allocate cell value cache
 *
 * (i-2)/NSEC + 1 is the annulus index for cell (i-1).
 * The cell value cache is organized as an array of PIX vectors
 * of length appropriate to the given cell. The cells are arranged
 * in annuli, with the index going from sector to sector ccw
 * within an annulus and then to the next annulus outward; the
 * sectors are arranged as in the picture at c. line 285
 */
    vsiz = 0;
    for(i=0;i<NANN;i++){
        scr = anndex[i+1] - anndex[i] + 2;
        scr = (scr*scr*5)/4 ; /* sloppy, but OK */
        if(i<NSYNCANN) scr *= 2;  /* in inner region, we double-count */
        vsiz += (sizecache[i] = scr);
    }

    cellval = shMalloc(NSEC*NANN*sizeof(PIX *));
    cellval[0] = shMalloc(NSEC*vsiz*sizeof(PIX) + /* space */
						  RLIM*2*7*sizeof(PIX)); /* 2-pixel ovfl +
												  inner corners*/

    cellval[1] = cellval[0] + sizecache[0];   /* central pixel */
    for(i=2;i<=NSEC*NANN;i++){
        cellval[i] = cellval[i-1] + sizecache[(i-2)/NSEC + 1];
    }

    /* allocate patch for sync-shifting into and its associated pointers */
    syncscr = shRegNew("syncscr",SYNC_REG_SIZEI,SYNC_REG_SIZEI,TYPE_PIX);
    syncext = shRegNew("syncext",SYNC_REG_SIZEI,SYNC_REG_SIZEI,TYPE_PIX);
    syncext_smoothed =
		shRegNew("syncext",SYNC_REG_SIZEI,SYNC_REG_SIZEI,TYPE_PIX);
    {
		const int size = SYNC_REG_SIZE + 2*SYNCEXTRA;
		syncreg = shSubRegNew("syncreg", syncext,
							  size, size, SYBELL-1, SYBELL-1, NO_FLAGS);
		syncreg_smoothed = shSubRegNew("syncreg", syncext_smoothed,
									   size, size, SYBELL-1, SYBELL-1, NO_FLAGS);
    }
    syncin = shRegNew("syncin",SYNC_REG_SIZEI,0,TYPE_PIX); /* n.b. pointers
															must be set! */


    /* allocate histogram */
    cellhist = (U16 *)shMalloc(0x10000*sizeof(U16));
}

static void
profallocs(void)
{
    int i,j;
    int nann;
    int nsec;
    int ninter;
    int ntinter;
    int ka;
    int ks;
    int ysec[4];

    /* allocate cell info struct arrays */
    cellgeom = shMalloc((NCELL + NLCELL)*sizeof(struct cellgeom));
    lcellgeom = cellgeom + NCELL;
    cellmod = shMalloc((NCELL + NLCELL)*sizeof(struct cellmod));
    lcellmod = cellmod + NCELL;
    nmod = shMalloc(NCELL*sizeof(float));

    /* allocate cellid */
    cellid = (struct cellid **)shMalloc(
		SYNC_REG_SIZE*sizeof(struct cellid *) +
		SYNC_REG_SIZE*SYNC_REG_SIZE*sizeof(struct cellid));
    cellid[0] = (struct cellid *)((char *)cellid +
								  SYNC_REG_SIZE*sizeof(struct cellid *));
    for(i=1;i<SYNC_REG_SIZE;i++) cellid[i] = cellid[i-1] + SYNC_REG_SIZE;
    /* These matrices give the cell index for
     * each pixel in the inner region, arranged as cellid[y][x]
     */


    /* allocate cell origin arrays */

    for(i=0;i<3;i++){
        ysec[i] = (RLIM+2)*sin((i+0.5)*M_PI/(NSEC/2));
        /* height of sector at boundary is <= ysec[i] */
    }
    ysec[3] = RLIM + 2;
    nann = NANN - NSYNCANN + 2;   /* # of annulus intersec along x axis;
                                   * this decreases by 1 at each line #
                                   * equal to the next anndex[] AFTER
                                   * NSYNCANN + 1; 1 more to allow for
                                   * end marker.
                                   */
    nsec = NSEC/4;                /* # of sector intersec along x-axis; for
                                   * other lines, decreases by one as y
                                   * passes each successive ysec[]
                                   */
    ka = NSYNCANN + 1;
    ks = 0;
    ntinter = 0;
    for(i=0;i<RLIM;i++){
        if(i > anndex[ka]){
            ka++;
            nann--;
        }
        if(i > ysec[ks]){
            ks++;
            nsec--;
        }
        ntinter += (nann + nsec);
    }
/*
 * cellorig is arranged as an array of matrices, one per scaled
 * y-origin, of RLIM vectors of cellorig structures, each of which
 * contains the scaled origins and lengths of the segments for
 * each cell on that line.
 */
    cellorig = (struct cellorig ***)shMalloc(
		YSCL*sizeof(struct cellorig **) +
		YSCL*RLIM*sizeof(struct cellorig *) +
		YSCL*ntinter*sizeof(struct cellorig));
    /* and set up pointers */
    cellorig[0] = (struct cellorig **)((char *)cellorig +
									   YSCL*sizeof(struct cellorig **));
    for(i=1;i<YSCL;i++){
        cellorig[i] = (struct cellorig **)((char *)cellorig[i-1] +
										   RLIM*sizeof(struct cellorig *) +
										   ntinter*sizeof(struct cellorig));
    }
    for(i=0;i<YSCL;i++){
        cellorig[i][0] = (struct cellorig *)((char *)cellorig[i] +
											 RLIM*sizeof(struct cellorig *));
        ninter = NSEC/4 + NANN - NSYNCANN + 2;
        ka = NSYNCANN + 1;    /* 2 more to allow for zero origin and end mkr*/
        ks = 0;
        for(j=1;j<RLIM;j++){
            if(j > anndex[ka]){
                ka++;
                ninter--;
            }
            if(j > ysec[ks]){
                ks++;
                ninter--;
            }
            cellorig[i][j] = cellorig[i][j-1] + ninter;
        }
    }

    cstats.radii = annrad;
    cstats.area = area;
    cstats.cells = cellinfo;
    cstats.geom = cellgeom;
    cstats.mgeom = cellmod;
}

/*
 * Frees the volatile profile arrays
 */

static void
provfree(void)
{
    if(cellinfo != NULL) {
		shFree(cellinfo[NCELL].data);
		shFree(cellinfo);
		cellinfo = NULL;
    }
    if(cellval) {
		shFree(cellval[0]);
		shFree(cellval);
		cellval  = NULL;
    }
    if(syncreg != NULL)  {
		shRegDel(syncreg);
		shRegDel(syncreg_smoothed);
		shRegDel(syncext);
		shRegDel(syncext_smoothed);
		shRegDel(syncin);
		shRegDel(syncscr);

		syncreg = NULL;
    }
    if(cellhist) {shFree(cellhist); cellhist = NULL;}
}

/*
 * Frees the static profile arrays
 */

static void
prosfree(void)
{
    if(cellid)   {shFree(cellid);   cellid   = NULL;}
    if(cellorig) {shFree(cellorig); cellorig = NULL;}
    if(cellgeom)  {shFree(cellgeom);  cellgeom  = NULL;}
    if(cellmod)  {shFree(cellmod);  cellmod  = NULL;}
    if(nmod) {shFree(nmod); nmod = NULL; }
}

/*************** MAKEDSSPROF() ******************************************/
/* This routine takes the populated cell cache and constructs the profile
 * quantities (quartiles, means) for the cells in the object. It expects
 * a populated cell cache and a ntot entry in each cellinfo struct.
 * It fills out the rest of the data in the cellinfo structs.
 */

static void
do_pstats(struct pstats *cinfo,
		  int floor,
		  int band,			/* band data was taken in */
		  float fxc, float fyc,		/* centre of obj/start of line */
		  const struct cellgeom *cgeom)	/* cell geometry, or NULL */
{
	int npt = cinfo->ntot;		/* number of points in data */
	int min;				/* min. value of histogrammed data */
	int rng;				/* range of histogrammed data */

	if(npt < 0) {			/* no data available */
		badstats(cinfo);
		return;
	} else if(npt == 0) {
		cinfo->area = npt;
		return;
	}

	cinfo->nel = npt;			/* we may reconsider after clipping */
	if(npt < NSIMPLE) {
		simplestats(cinfo);
	} else {
		if(npt < NINSERT){
			/*
			 * do straight Shell or insertion sort; shinstats does everything
			 */
			shinstats(cinfo);
		} else {
			/* check for max, make mean, calc. range */
			cinfo->min = floor;
			region_maxmean_crude(cinfo);
			rng = cinfo->rng;
			min = cinfo->min;
			if(SHELLWINS(npt,rng)){   /* use Shellsort */
				shinstats(cinfo);
			} else {		/* use histsort */
				shAssert(min + rng <= MAX_U16);
				memset(&cellhist[min],'\0',(rng + 1)*sizeof(cellhist[0]));
				if(MBTHRESH < 0 || npt < MBTHRESH) {
					get_quartiles_from_array(cellhist,cinfo);
#if MBTHRESH >= 0			/* make compiler happy */
				} else {
					get_quartiles_from_array_clipped(cellhist,cinfo,
													 band, fxc, fyc, cgeom);
#endif
				}
			}
		}
	}
    //trace("cinfo->flg = 0x%x\n", cinfo->flg);
	cinfo->area = (cinfo->flg & EXTRACT_SINC) ? 0.5*cinfo->ntot : cinfo->ntot;
}

static void
makedssprof(int cell0,			/* first cell to process */
			int cell1,			/* last cell to process */
			double sky,			/* sky level */
			double skysig,		/* sky variance */
			int band,			/* band data is taken in */
			float fxc, float fyc)	/* centre of object */
{
    int i;
    int skyfloor = sky - 5.*skysig;	 /* sky - 5*sigma should be safe, but
									  region_maxmean_crude() checks */

    if(skyfloor < 0) skyfloor = 0;

    for(i = cell0;i <= cell1;i++){
        //trace("makedssprof: setting cellinfo[%i].  NCELLIN=%i, NCELL=%i\n", i, NCELLIN, NCELL);
        //trace("flag: 0x%x\n", cellinfo[i].flg);
		if(i < NCELLIN) {
			cellinfo[i].flg |= EXTRACT_SINC;
		}
       
		do_pstats(&cellinfo[i], skyfloor, band, fxc, fyc, &cellgeom[i]);
		if(cellinfo[i].ntot > 0) {
			cellinfo[i].mean -= sky;
			cellinfo[i].qt[0] -= sky;
			cellinfo[i].qt[1] -= sky;
			cellinfo[i].qt[2] -= sky;
			cellinfo[i].min -= sky;
			cellinfo[i].sum -= cellinfo[i].nel*sky;
		}
    }
}


/*****************************************************************************/
/*
 * subtract a sky level from a CELL_STATS
 */
void
phProfileSkySubtract(CELL_STATS *cstats,
					 float sky)
{
	int i;

	shAssert(cstats != NULL);
   
	for(i = 0;i < cstats->ncell;i++){
		if(cellinfo[i].ntot > 0) {
			cellinfo[i].mean -= sky;
			cellinfo[i].qt[0] -= sky;
			cellinfo[i].qt[1] -= sky;
			cellinfo[i].qt[2] -= sky;
			cellinfo[i].min -= sky;
			cellinfo[i].sum -= cellinfo[i].nel*sky;
		}
	}
}


/*****************************************************************************/
static void
cellboundary(int cell,			/* the cell in question */
			 struct cellgeom *cbsp	/* description of the cell */
	)
{
    int annulus;
    int sector;

    shAssert(cell >= 0 && cell < NCELL);

    if(cell == 0) {
        cbsp->ann = 0;
        cbsp->sec = 0;
        cbsp->inner = 0.;
        cbsp->outer = 0.5;
        cbsp->cw = 0.;
        cbsp->ccw = 2*M_PI;
	} else {
        annulus = (cell-1)/NSEC + 1;
        sector = (cell-1)%NSEC;

        cbsp->ann = annulus;
        cbsp->sec = sector;
        cbsp->inner = annrad[annulus];
        cbsp->outer = annrad[annulus + 1];
        cbsp->ccw = 2*M_PI - ((sector + 0.5)*(2*M_PI/NSEC));
        if(cbsp->ccw < 0.) cbsp->ccw += 2*M_PI;
        cbsp->cw = cbsp->ccw + 2*M_PI/NSEC;
    }
}

/*****************************************************************************/

#if defined(DEBUG)
/*
 * print the cellorig array for subpixel sdy
 */
static void
print_cellorig(int sdy)
{
	struct cellorig **cells;
	struct cellorig cell;
	int x,y;

	if(sdy < 0 || sdy > YSCL) {
		fprintf(stderr,"Invalid subpixel: %d\n",sdy);
		return;
	}
	cells = cellorig[sdy];

	for(y = 0;y < RLIM;y++) {
		for(x = 0;cells[y][x].c_orig != 0x7fff;x++) {
			cell = cells[y][x];
			printf("%d %d   %d %d   %d %d %d %d\n",y,x,cell.c_orig,cell.c_len,
				   cell.c_llcell,cell.c_lrcell,cell.c_urcell,cell.c_ulcell);
		}
	}
}
#endif

/*****************************************************************************/
/*
 * Convolve syncreg by a Gaussian, using the syncext/syncscr regions,
 * returning the smoothed data
 *
 * N.b. syncreg and the extracted profile aren't modified
 */
const REGION *
phConvolveSyncregWithGaussian(float dsigma)
{
	int ret =
		phConvolveWithGaussian(syncext_smoothed, syncext, syncscr, 2*SYBELL-1,
							   dsigma, 0, CONVOLVE_MULT);
	shAssert(ret == SH_SUCCESS);

	return(syncreg_smoothed);
}
