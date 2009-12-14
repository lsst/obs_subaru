
/*
 * <AUTO>
 *
 * FILE: phTclVerbs.c
 *
 * DESCRIPTION:
 * Declare TCL verbs used in PHOTO
 *
 * </AUTO>
 */

#include <stdlib.h>
#include <stdio.h>
#include "dervish.h"
#include "tclMath.h"


RET_CODE phSchemaLoadFromPhoto(void);
RET_CODE atSchemaLoadFromAstrotools(void);

/*****************************************************************************/
/*
 * Prototypes for functions declaring photo commands at the TCL level
 */
void atVerbsDeclare(Tcl_Interp *interp);

void phTclPhfooDeclare(Tcl_Interp *interp);
void phTclMagDeclare(Tcl_Interp *interp);
void phTclExtinctionDeclare(Tcl_Interp *interp);
void phTclStar1Declare(Tcl_Interp *interp);
void phTclStarcDeclare(Tcl_Interp *interp);
void phTclCalib1Declare(Tcl_Interp *interp);
void phTclCalib1bytimeDeclare(Tcl_Interp *interp);
void phTclCalib1byframeDeclare(Tcl_Interp *interp);
void phTclStar1starcmergeDeclare(Tcl_Interp *interp);
void phTclFindPhotoParamsDeclare(Tcl_Interp *interp);
void phTclObjcDeclare(Tcl_Interp *interp);
void phTclObject1Declare(Tcl_Interp *interp);
void phTclOffsetDeclare(Tcl_Interp *interp);
void phTclPhotparDeclare(Tcl_Interp *interp);
void phTclFieldstatDeclare(Tcl_Interp *interp);
void phTclDgpsfDeclare(Tcl_Interp *interp);
void phTclCcdparsDeclare(Tcl_Interp *interp);
void phTclFrameinfoDeclare(Tcl_Interp *interp);
void phTclDefineTranslateDeclare(Tcl_Interp *interp);
void phTclQuartilesDeclare(Tcl_Interp *interp);
void phTclFrameinfoDeclare(Tcl_Interp *interp);
void phTclHistDeclare(Tcl_Interp *interp);
void phTclKnownobjDeclare(Tcl_Interp *interp);
void phTclMergeColorsDeclare(Tcl_Interp *interp);
void phTclMeasureObjDeclare(Tcl_Interp *interp); /* test of MeasureObj */
void phTclRegFinderDeclare(Tcl_Interp *interp);
void phTclCorrectFramesDeclare(Tcl_Interp *interp);
void phTclDitherDeclare(Tcl_Interp *interp);
void phTclFlatfieldsCalcDeclare(Tcl_Interp *interp);
void phTclFlatfieldsInterpDeclare(Tcl_Interp *interp);
void phTclLibraryDeclare(Tcl_Interp *interp);
void phTclLibraryCRDeclare(Tcl_Interp *interp);
void phTclRandomDeclare(Tcl_Interp *interp);
void phTclSkyUtilsDeclare(Tcl_Interp *interp);
void phTclMakeStarlistDeclare(Tcl_Interp *interp);
void phTclStar1cDeclare(Tcl_Interp *interp);
void phTclFindPsfDeclare(Tcl_Interp *interp);
void phTclFindKnownObjectsDeclare(Tcl_Interp *interp);
void phPhotoFitsBinDeclare(Tcl_Interp *interp);
void phTclExtractDeclare(Tcl_Interp *interp);
void phTclBrightObjectsDeclare(Tcl_Interp *interp);
void phTclAnalysisDeclare(Tcl_Interp *interp);
void phTclSpanmaskDeclare(Tcl_Interp *interp);
void phSkyObjectsDeclare(Tcl_Interp *interp);
void phTclFitobjDeclare(Tcl_Interp *interp);
void phTclUmedianDeclare(Tcl_Interp *interp);
void phTclWallpaperDeclare(Tcl_Interp *interp);
void phTclVecUtilsDeclare(Tcl_Interp *interp);
void phTclVariablePsfDeclare(Tcl_Interp *interp);
void phTclFscDeclare(Tcl_Interp *interp);
void phTclMeschachDeclare(Tcl_Interp *interp);
void phTclFootprintDeclare(Tcl_Interp *interp);

/***************************************************************************
 * <AUTO EXTRACT>
 *
 * ROUTINE: phTclVerbsDeclare
 * 
 * DESCRIPTION: 
 * Declare all the public TCL verbs used in PHOTO.
 *
 * return: nothing
 *
 * </AUTO>
 */

void
phTclVerbsDeclare(
   Tcl_Interp *interp         /* I: TCL interpreter */
   )
{
   phTclMagDeclare(interp);      /* Adds the PHOTO MAG struct */
   phTclExtinctionDeclare(interp); /* Adds the PHOTO EXTINCTION struct */
   phTclStar1Declare(interp); /* Adds the PHOTO STAR1 struct */
   phTclStarcDeclare(interp); /* Adds the PHOTO STARC struct */
   phTclCalib1Declare(interp); /* Adds the PHOTO CALIB1 struct */
   phTclCalib1bytimeDeclare(interp); /* Adds PHOTO CALIB1BYTIME struct */
   phTclCalib1byframeDeclare(interp); /*Adds PHOTO CALIB1BYFRAME struct*/
   phTclStar1starcmergeDeclare(interp);
                                /*Adds the PHOTO STAR1STARCMERGE struct*/
   phTclObjcDeclare(interp);     /* define OBJCs */
   phTclObject1Declare(interp);  /* define OBJECT1s */
   phTclPhotparDeclare(interp);  /* defines PHOTPARs */
   phTclOffsetDeclare(interp);   /* define OFFSETs */
   phTclFieldstatDeclare(interp);/* define FIELDSTAT and FRAMESTAT */
   phTclDgpsfDeclare(interp);    /* defines DGPSF type */
   phTclCcdparsDeclare(interp);    /* defines CCDPARS type */
   phTclDefineTranslateDeclare(interp);    /* defines CCDPARS type */
   phTclQuartilesDeclare(interp);    /* defines CCDPARS type */
   phTclFrameinfoDeclare(interp); /* defines FRAMEINFO type */
   phTclHistDeclare(interp);      /* defines PHHIST type */

   /* --- PLACE YOUR MODULE DECLARATIONS HERE --- */
   phTclCorrectFramesDeclare(interp);   /* correctFrames Module   */
   phTclMeasureObjDeclare(interp);      /* test of MeasureObj module */
   phTclMergeColorsDeclare(interp); /* merge colours Module */
   phTclRegFinderDeclare(interp); /* Object Finder */
   phTclFindPhotoParamsDeclare(interp); /* findPhotoParams Module */
   phTclFlatfieldsInterpDeclare(interp); /*  flatfieldsInterp Module */
   phTclFlatfieldsCalcDeclare(interp); /*  flatfieldsCalc Module */
   phTclLibraryDeclare(interp);          /* Mirella library functions */
   phTclLibraryCRDeclare(interp);          /* Cosmic Ray library functions */
   phTclRandomDeclare(interp);		/* support random numbers */
   phTclSkyUtilsDeclare(interp);	/* sky measuring utils */
   phTclMakeStarlistDeclare(interp);		/* MakeStarlist Module*/
   phTclStar1cDeclare(interp);		/* star1c Module*/
   phTclFindPsfDeclare(interp);		/* Find Psf  Module*/
   phTclKnownobjDeclare(interp);/* Find KnownObjects Module*/
   phTclFindKnownObjectsDeclare(interp);/* Find KnownObjects Module*/
   phPhotoFitsBinDeclare(interp);
   phTclExtractDeclare(interp);
   phTclAnalysisDeclare(interp);
   phTclBrightObjectsDeclare(interp);
   phTclSpanmaskDeclare(interp);
   phSkyObjectsDeclare(interp);
   phTclFitobjDeclare(interp);
   phTclUmedianDeclare(interp);
   phTclWallpaperDeclare(interp);
   phTclVecUtilsDeclare(interp);
   phTclVariablePsfDeclare(interp);
   phTclFscDeclare(interp);
   phTclMeschachDeclare(interp);
   phTclFootprintDeclare(interp);
   
   /* --- DECLARES PHOTO STRUCTURES IN DERVISH SCHEMA --- */
   (void)phSchemaLoadFromPhoto();
}
