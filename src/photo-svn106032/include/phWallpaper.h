#if !defined(PHWALLPAPER_H)
#define PHWALLPAPER_H

REGION *
phRegIntToU8Linear(const REGION *reg,	/* region to convert */
		   int base,		/* pixel value to map to 0x0 */
		   int top);		/* pixel value to map to 0xff */

REGION *
phRegIntToU8LUT(const REGION *reg,	/* region to convert */
		const REGION *lutreg);	/* the LUT */

REGION **
phRegIntRGBToU8LUT(const REGION *regR,	/* R-region to convert */
		   const REGION *regG,	/* G-region to convert */
		   const REGION *regB,	/* B-region to convert */
		   const REGION *lutreg, /* the LUT */
		   int separate_bands);	/* scale each band separately */

void
phFixSaturatedU8Regs(REGION *regR,	/* R-region to fix */
		     REGION *regG,	/* G-region to fix */
		     REGION *regB,	/* B-region to fix */
		     REGION *reg16R,	/* U16 data for R-region, or NULL */
		     REGION *reg16G,	/* U16 data for G-region, or NULL */
		     REGION *reg16B,	/* U16 data for B-region, or NULL */
		     int minU16,	/* value to subtract from U16 regs */
		     int fixupLevel,	/* fix pixels above this in U16 */
		     int fix_all_satur,	/* fix all saturated pixels */
		     int use_mean_color);/* If true, use mean of (U8)-saturated
					    pixels; else use pixels just
					    outsize saturated region */

int
phOnecolorPostscriptWrite(const char *file, /* file to write */
			  const REGION *reg1, /* U8 region */
			  float size,	/* size of output, in inches */
			  float xoff,	/* origin of Bounding Box */
			  float yoff);	/*                        in inches */
int
phTruecolorPostscriptWrite(const char *file, /* Postscript file to write */
			   const REGION *reg1, /* U8 regions: R */
			   const REGION *reg2, /*             G */
			   const REGION *reg3, /*             B */
			   float size,	/* size of output, in inches */
                           float xoff,	/* origin of Bounding Box */
                           float yoff);	/*                        in inches */
int
phOnecolorPGMWrite(const char *file,	/* file to write */
		   const REGION *reg1);	/* U8 region */
int
phTruecolorPPMWrite(const char *file,	/* PPM file to write */
		    const REGION *reg1, /* U8 regions: R */
		    const REGION *reg2, /*             G */
		    const REGION *reg3); /*             B */

void
phObjectChainSetInRegion(REGION *reg,	/* region to set */
			 const CHAIN *ch, /* from these OBJECTs */
			 int val,	/* set region's pixels to this value */
			 int drow, int dcol); /* offset OBJECTs this much */

int
phRegU8Median(REGION *reg1,		/* U8 regions: R */
	      REGION *reg2,		/*             G */
	      REGION *reg3,		/*             B */
	      int size,			/* size of desired filter */
	      int which);		/* filter: 0: intensity; 1: chromaticity: 2: saturation */

#endif
