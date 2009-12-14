SHELL = /bin/sh
#
#
#  Makefile to rebuild all of photo
#
#  1) This file is designed to be invoked by a shell script called premake
#  which selects supported options.
#
#  2) This file re-builds the PHOTO product. Those expecting to extend PHOTO
#  by adding application-specific TCL extensions should re-build using the 
#  premake and make files in the "../examples" sub-directory.
#
# 3) Environment:
# Environmental variables
#	$LIBFITS_DIR The Libfits package, libfits having been setup by ups
#	$FTCL_DIR    The FTCL Package, ftcl having ben setup by ups
#	$PGPLOT_DIR  The PGPLOT package pgplot having been setup by ups
# Command-line macro selecting switches to support the three compilation
# environments:
#	SGI C Compiler under IRIX
#	SUN ACC Compilers under SunOS
#	GNU C Compiler under SunOS
#
# Expected variables on the command line:
# CC=        The name of the appropriate compiler, with suitable flags
# CCCHK=     A set of compiler flags which check ANSI C strictly
# LLIBS=     A set of libraries for linking against.
#
.c.o :
	$(CC) -c $(CCCHK) $(CFLAGS) $*.c
	@- if [ -f .config ]; then \
		echo $(CC) -c $(CCCHK) $(CFLAGS) $*.c >> .config; \
	fi
.c.s :
	$(CC) -S $(CCCHK) $(CFLAGS) $*.c
.c.cpp :
	$(CC) -E $(CCCHK) $(CFLAGS) $*.c > $*.cpp
#
SRC	= ../src
INC	= ../include
LIB	= ../lib
BIN	= ../bin
ETC	= ../etc
DOC	= ../doc
UPS	= ../ups
INCDIRS	= -I$(INC) -I$(INC)/prvt -I$(DERVISH_DIR)/contrib -I$(FTCL_DIR)/include\
			-I$(FTCL_DIR)/src -I$(LIBFITS_DIR)/src\
			-I$(FPGPLOT_DIR)/include\
			-I$(PGPLOT_DIR)/dst -I$(DERVISH_DIR)/include\
			-I$(TCLX_DIR)/include -I$(TCLX_DIR)/src/src\
			-I$(TCL_DIR)/include -I$(TCL_DIR)/src\
			-I$(SDSSMATH_DIR)/include -I$(FFTW_DIR)/src \
			-I$(SLALIB_DIR)/src_c -I$(ASTROTOOLS_DIR)/include 

CFLAGS  = $(SDSS_CFLAGS) $(INCDIRS) -DCHECK_LEAKS $(EXTRA_CFLAGS)
LDFLAGS  = $(SDSS_LDFLAGS)

#
# Directories with modules
#
SUBDIRS = measureObj
#
#
OBJECTS = photo_main.o $(TCLOBJECTS) mag.o \
	star1.o starc.o calib1.o extinction.o \
	star1starcmerge.o diskio_gen.o\
	objc.o object1.o photpar.o \
	offset.o mergeColors.o \
	finder.o \
	correctFrames.o \
	framestat.o\
	utils.o dgpsf.o \
	ccdpars.o frameinfo.o \
	brightObjects.o \
	quartiles.o flatfieldsInterp.o \
	findPhotoParams.o \
	flatfieldsCalc.o \
	fsc.o \
        library.o histU16.o \
        phInterp.o CR.o \
	random.o medianSky.o makeStarlist.o findPsf.o knownobj.o \
	findKnownObjects.o sky.o convolve.o photoFitsIO.o \
	extract.o extract_utils.o phSpanUtil.o phAnalysis.o \
	fitobj.o makeprof.o cellfitobj.o \
	objectUtils.o skyObjects.o mathUtils.o LM_minimise.o astcen.o \
	apertures.o star1c.o umedian.o meschach.o apodize.o dataIo.o rice.o \
	wallpaper.o vecUtils.o variablePsf.o ranlib.o footprint.o

TCLOBJECTS = tclUtils.o tclMag.o \
	tclStar1.o tclStarc.o tclCalib1.o tclExtinction.o\
	tclCalib1bytime.o tclCalib1byframe.o \
	tclStar1starcmerge.o \
	tclMeasureObj.o tclPhotpar.o \
	tclObjc.o tclObject1.o \
	tclOffset.o \
	tclMergeColors.o tclCorrectFrames.o \
	tclBrightObjects.o \
	tclFramestat.o \
	tclCcdpars.o tclFrameinfo.o phTclVerbs.o \
	tclDefineTranslate.o \
	tclFinder.o \
	tclFsc.o \
	tclSpanUtil.o \
        tclQuartiles.o  tclDgpsf.o  \
	tclFindPhotoParams.o  \
	tclFlatfieldsCalc.o tclFlatfieldsInterp.o \
        tclLibrary.o tclHistU16.o \
        tclCR.o tclRandom.o tclMakeStarlist.o tclFindPsf.o \
	tclKnownobj.o tclFindKnownObjects.o tclSkyUtils.o tclPhotoFitsIO.o \
	tclExtract.o tclAnalysis.o \
	tclFitobj.o tclStar1c.o tclUmedian.o \
	tclWallpaper.o tclVecUtils.o tclVariablePsf.o tclMeschach.o tclFootprint.o
        
MEASUREOBJECTS = measureObj.o

LIBS = -L$(LIB) -lphoto -lmeasureObj \
	$(ASTROTOOLS_DIR)/lib/libastrotools.a \
        $(SDSSMATH_DIR)/lib/libsdssmath.a $(FFTW_DIR)/src/libfftw.a \
	$(SLALIB_DIR)/src_c/libsla.a \
	$(SLATEC_DIR)/lib/libslatec.a \
	$(LAPACK_DIR)/liblapack.a $(LAPACK_DIR)/libblas.a \
	$(DERVLIBS)

DERVLIBS = \
	$(LIBSHV) $(LIBFITS) $(PGPLOTLIB) \
	$(CONTRIBLIB) \
	$(LIBFTCL) \
	$(LLIBS) $(PGLIB) $(FLIBS) $(ELIBS)

#
# A debugging version of the malloc libraries, e.g. -lmalloc_cv on SGIs
#
#MALLOC = -lmalloc_cv

#=============================================================================
# TARGETS
#=============================================================================

default:	
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

all : diskio $(LIB)/libphoto.a Modules $(BIN)/photo
	@echo "Updating libraries and directories"
	@echo ""
 
library :	$(LIB)/libphoto.a

# all these targets are subdirectories 
Modules: 
	@echo "Updating module directories"
	@ for f in $(SUBDIRS); do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) all ); \
	done
	@- if [ -f $(BIN)/photo ]; then \
	  if [ "`find $(LIB) -name \*.a -newer $(BIN)/photo -print`" != "" ]; \
									 then \
	     echo "$(BIN)/photo is out of date wrt libraries; removing"; \
	     rm -f $(BIN)/photo; \
          fi; \
	fi

$(LIB)/libphoto.a : $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB)/libphoto.a $?
	- if [ "$(RANLIB)" != "" -a -x "$(RANLIB)" ]; then \
		$(RANLIB) $(LIB)/libphoto.a; \
	fi
	@echo "=== PHOTO BUILD : Building Unix library (libphoto.a) ==="
	@echo

main :		$(BIN)/photo
#
# Standard photo main program.
#
$(BIN)/photo : $(LIB)/libphoto.a
	$(CC) -o $@ $(LDFLAGS) $(CCCHK) $(SRC)/diskio_gen.o $(LIBS) $(MALLOC)
#
# Profiled versions
#
# N.b. some makes don't like executables named *.p
# OSF/1 requires -non_shared to profile the shared libraries, and
# -lFutil -ldnet_stub to be able to link when -non_shared is specified.
# This is a bug.
#
$(BIN)/photo.p : $(LIB)/libphoto.a Modules
	$(CC) -p -o photo.p~ \
			$(LDFLAGS) $(CCCHK) $(SRC)/diskio_gen.o $(LIBS)
	@ mv photo.p~ $@
/dev/nonexistant:;
$(BIN)/photo.pg : /dev/nonexistant
	EXTRA_CFLAGS="-pg" $(MAKE) $(LIB)/libphoto.a Modules
	PROFFLAGS=-all $(CC) -pg -o $(BIN)/photo.pg \
			$(LDFLAGS) $(CCCHK) $(SRC)/diskio_gen.o $(LIBS)

# Generate DISK IO source code for photo_specific structures.
#
diskio : diskio_gen.c 
DISKIO_FILES = \
	$(INC)/phFilter.h $(INC)/phSystem.h\
	$(INC)/phMag.h $(INC)/phExtract.h \
	$(INC)/phStar1.h $(INC)/phStar1c.h \
	$(INC)/phStarc.h $(INC)/phCalib1.h\
	$(INC)/phExtinction.h \
	$(INC)/phStar1starcmerge.h \
	$(INC)/phObjc.h $(INC)/phOffset.h \
	$(INC)/phPhotpar.h $(INC)/phFramestat.h \
	$(INC)/phDgpsf.h \
	$(INC)/phCcdpars.h $(INC)/phQuartiles.h $(INC)/phFrameinfo.h \
	$(INC)/phHist.h \
	$(INC)/phRandom.h $(INC)/phKnownobj.h \
	$(INC)/phObjects.h .phUtils.h $(INC)/phPhotoFitsIO.h \
	$(INC)/phSpanUtil.h $(INC)/phSkyUtils.h \
	$(INC)/phFitobj.h $(INC)/phTestInfo.h \
        $(INC)/phBrightObjects.h $(INC)/phPeaks.h $(INC)/phVariablePsf.h \
	$(INC)/phFsc.h $(INC)/phConsts.h $(INC)/phFootprint.h

#
# Replace the definition of PIX in ATLAS_IMAGE by the value from phConsts.h
# This is done for us by the CPP, but make_io doesn't know that.
#
.phObjc.h : $(INC)/phConsts.h $(INC)/phObjc.h
	@ cp $(INC)/phConsts.h .phConsts.c
	@ echo 'PIX_TYPE = PIX' >> .phConsts.c
	@ $(CC) -E .phConsts.c | \
		perl -e 'while(<>) {if(/PIX_TYPE = (\S*)/){$$PIX=$$1}}\
			open(OFD,">.phObjc.h") ||die("Cannot open .phObjc.h");\
			open(FD, "$(INC)/phObjc.h");\
			while(<FD>) { s/PIX/$$PIX/; print OFD; }'
#
# Replace the definition of PIX in PHOTO_INFO by the value from phConsts.h
# This is done for us by the CPP, but make_io doesn't know that.
#
.phUtils.h : $(INC)/phConsts.h $(INC)/phUtils.h
	@ cp $(INC)/phConsts.h .phConsts.c
	@ echo 'PIX_TYPE = PIX' >> .phConsts.c
	@ $(CC) -E .phConsts.c | \
		perl -e 'while(<>) {if(/PIX_TYPE = (\S*)/){$$PIX=$$1}} open(OFD,">.phUtils.h") ||die("Cannot open .phUtils.h"); open(FD, "$(INC)/phUtils.h"); while(<FD>) { s/PIX/$$PIX/; print OFD; }'
#
diskio_gen.c :  $(DERVISH_DIR)/bin/make_io $(DISKIO_FILES)
	rm -f diskio_gen.[c]
	$(DERVISH_DIR)/bin/make_io -v1 -p ph -m Photo diskio_gen.c $(DISKIO_FILES)
	chmod 444 diskio_gen.[c]
#
# These files include other .c files
#
apodize.o : apo_coeffs.c
aperture.o : photom_coeffs.c
#
# Some stand alone utilities.
#
# Calculate interpolation coefficients for correct frames
#
$(BIN)/make_interp : make_interp.o
	$(CC) -o $(BIN)/$@ make_interp.o $(LIBS)
$(BIN)/make_photom_coeffs : make_photom_coeffs.o
	$(CC) -o $(BIN)/$@ make_photom_coeffs.o $(LIBS)
#
# Calculate biases for isophote fitting in measure objects. Note that we
# _do_ need to search $(LIBS) twice...
#
find_iso_bias : find_iso_bias.o
	$(CC) -o $(BIN)/$@ $@.o $(LIBS) $(LIBS)
#
# Read an atlas image from a photo output file
#
read_atlas_gen.c : $(INC)/phObjc.h $(BIN)/photo
	$(BIN)/photo -file make_atlas_gen_c.tcl
read_atlas : read_atlas.o read_atlas_gen.o \
		measureObj/atlasImages.o photoFitsIO.o phSpanUtil.o dummies.o
	$(CC) -o $(BIN)/$@ read_atlas.o read_atlas_gen.o \
		measureObj/atlasImages.o photoFitsIO.o phSpanUtil.o dummies.o \
		$(DERVLIBS)

clean :
	rm -f *~ core *.bak *.orig *.old .#* #*#
	rm -f *.o .config
	@ for f in $(SUBDIRS) ; do \
		(cd $$f ; echo In $$f ; $(MAKE) $(MFLAGS) clean); \
	done

install :
	@ if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi
	@ rm -rf $(PHOTO_DIR)/src
	@ mkdir $(PHOTO_DIR)/src
	cp -r *.c $(PHOTO_DIR)/src
	cp Makefile $(PHOTO_DIR)/src
	@for f in $(SUBDIRS); do \
	   (mkdir $(PHOTO_DIR)/src/$$f; cd $$f; echo In $$f; $(MAKE) $(MFLAGS) install); \
	done

#
make :
	@if [ "$(CCENV)" = "" ]; then ccenv=`uname`; else ccenv=$(CCENV); fi; \
	echo \
	"make_make -cc '$(CC) $(CFLAGS)' -nostd -file Makefile" \
							"-env $$ccenv *.c"; \
	$(SDSSTOOLS_DIR)/bin/make_make -cc '$(CC) $(CFLAGS)' -nostd \
			-file Makefile -env $$ccenv `ls *.c | grep -v foo.c`
	$(PHOTO_DIR)/bin/fix-make_make Makefile
	@- for f in $(SUBDIRS); do \
		echo In $$f; \
		(cd $$f; \
		 $(MAKE) $(MFLAGS) make; \
		 $(PHOTO_DIR)/bin/fix-make_make Makefile ) \
	done
#
# include file dependencies.
#
# All line below START_DEPEND are machine generated; Do Not Edit
#
#START_DEPEND
apertures.o: apertures.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phExtract.h \
  ../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h photom_coeffs.c
apodize.o: apodize.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phMath.h apo_coeffs.c
astcen.o: astcen.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phConsts.h ../include/phObjectCenter.h
brightObjects.o: brightObjects.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phMathUtils.h ../include/phMeschach.h \
  ../include/phMathUtils.h ../include/phBrightObjects.h \
  ../include/phCalib1.h ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phObjects.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phMaskbittype.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phObjc.h \
  ../include/phPeaks.h ../include/phUtils.h ../include/phOffset.h \
  ../include/phExtract.h ../include/phMath.h
calib1.o: calib1.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phCalib1.h \
  ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h
ccdpars.o: ccdpars.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h
cellfitobj.o: cellfitobj.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phObjc.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phExtract.h \
  ../include/phSpanUtil.h ../include/phConsts.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phDgpsf.h \
  ../include/phFitobj.h ../include/phCellFitobj.h ../include/phFitobj.h \
  ../include/phMathUtils.h ../include/phMeschach.h \
  ../include/phMathUtils.h ../include/phUtils.h ../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h $(DERVISH_DIR)/include/shChain.h \
  ../include/phMaskbittype.h ../include/phObjc.h ../include/phUtils.h \
  ../include/phConsts.h ../include/phFocus.h ../include/phObjectCenter.h \
  ../include/phMeasureObj.h ../include/phCalib1.h \
  ../include/prvt/p_phMeasureObj.h ../include/phTestInfo.h
convolve.o: convolve.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phSkyUtils.h \
  ../include/phRandom.h ../include/phRanlib.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phConsts.h
correctFrames.o: correctFrames.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phSignals.h \
  ../include/phConsts.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phCorrectFrames.h $(DERVISH_DIR)/include/region.h \
  ../include/phFramestat.h $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phVariablePsf.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phQuartiles.h \
  ../include/phCalib1.h ../include/prvt/phCorrectFrames_p.h \
  ../include/phMaskbittype.h ../include/phSpanUtil.h \
  ../include/phObjects.h ../include/phMaskbittype.h ../include/phObjc.h \
  ../include/phPeaks.h ../include/phUtils.h ../include/phMathUtils.h \
  ../include/phMeschach.h ../include/phMathUtils.h
CR.o: CR.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phMaskbittype.h ../include/phCosmic.h ../include/phDgpsf.h \
  ../include/phVariablePsf.h ../include/phSpanUtil.h \
  ../include/phCorrectFrames.h $(DERVISH_DIR)/include/region.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phCcdpars.h $(DERVISH_DIR)/include/shChain.h \
  ../include/phQuartiles.h ../include/phCalib1.h ../include/phConsts.h
dataIo.o: dataIo.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phSpanUtil.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phObjc.h ../include/phExtract.h ../include/phSpanUtil.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/prvt/phObjc_p.h ../include/phDataIo.h ../include/phObjc.h \
  ../include/phRice.h
dgpsf.o: dgpsf.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phConsts.h
diskio_gen.o: diskio_gen.c $(DERVISH_DIR)/include/dervish_msg_c.h \
  ../include/phFilter.h ../include/phSystem.h ../include/phMag.h \
  ../include/phFilter.h ../include/phExtract.h ../include/phSpanUtil.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phStar1.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phCalib1.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phObjc.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/phStar1c.h ../include/phStarc.h ../include/phMag.h \
  ../include/phCalib1.h ../include/phExtinction.h \
  ../include/phStar1starcmerge.h ../include/phStar1.h \
  ../include/phStarc.h ../include/phObjc.h ../include/phOffset.h \
  ../include/phPhotpar.h ../include/phFramestat.h ../include/phDgpsf.h \
  ../include/phCcdpars.h ../include/phQuartiles.h \
  ../include/phFrameinfo.h ../include/phHist.h ../include/phRandom.h \
  ../include/phKnownobj.h ../include/phOffset.h ../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h ../include/phMaskbittype.h \
  ../include/phUtils.h .phUtils.h ../include/phPhotoFitsIO.h \
  ../include/phSpanUtil.h ../include/phSkyUtils.h ../include/phFitobj.h \
  ../include/phTestInfo.h ../include/phBrightObjects.h \
  ../include/phObjects.h ../include/phPeaks.h ../include/phVariablePsf.h \
  ../include/phFsc.h ../include/phConsts.h \
  $(DERVISH_DIR)/include/shCSchema.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCDiskio.h \
  $(DERVISH_DIR)/include/prvt/utils_p.h
extinction.o: extinction.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phExtinction.h \
  ../include/phMag.h ../include/phFilter.h
extract.o: extract.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/prvt/phExtract_p.h ../include/phSkyUtils.h \
  ../include/phRandom.h ../include/phRanlib.h ../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h $(DERVISH_DIR)/include/shChain.h \
  ../include/phMaskbittype.h ../include/phExtract.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  ../include/phSkyUtils.h ../include/phVariablePsf.h ../include/phObjc.h \
  ../include/phPeaks.h ../include/phUtils.h
extract_utils.o: extract_utils.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phConsts.h ../include/phExtract.h ../include/phSpanUtil.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/prvt/phExtract_p.h
finder.o: finder.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSignals.h ../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h $(DERVISH_DIR)/include/shChain.h \
  ../include/phMaskbittype.h ../include/phExtract.h \
  ../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phObjc.h ../include/phPeaks.h \
  ../include/phUtils.h ../include/phMaskbittype.h ../include/phSkyUtils.h
find_iso_bias.o: find_iso_bias.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phExtract.h \
  ../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phMathUtils.h ../include/phMeschach.h \
  ../include/phMathUtils.h ../include/phMeasureObj.h \
  ../include/phCalib1.h ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phFramestat.h ../include/phSkyUtils.h \
  ../include/phRandom.h ../include/phRanlib.h ../include/phVariablePsf.h \
  ../include/phObjc.h ../include/phPeaks.h \
  ../include/prvt/p_phMeasureObj.h
findKnownObjects.o: findKnownObjects.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phKnownobj.h \
  ../include/phOffset.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  ../include/phFilter.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phSkyUtils.h \
  ../include/phRandom.h ../include/phRanlib.h ../include/phVariablePsf.h \
  ../include/phObjc.h ../include/phPeaks.h ../include/phOffset.h \
  ../include/phMaskbittype.h
findPhotoParams.o: findPhotoParams.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(SDSSMATH_DIR)/include/sdssmath.h \
  $(SDSSMATH_DIR)/include/tclMath.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(SDSSMATH_DIR)/include/slatec.h \
  $(SDSSMATH_DIR)/include/smaFunc.h \
  $(SDSSMATH_DIR)/include/smaFft.h \
  $(FFTW_DIR)/src/fftw.h \
  ../include/phFindPhotoParams.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phCalib1.h \
  ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phExtinction.h ../include/phMag.h ../include/phFrameinfo.h \
  ../include/phStar1starcmerge.h ../include/phStar1.h \
  ../include/phCcdpars.h ../include/phObjc.h ../include/phFramestat.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phStarc.h \
  ../include/phFilter.h ../include/phFramestat.h ../include/phUtils.h \
  ../include/phMathUtils.h ../include/phMeschach.h \
  ../include/phMathUtils.h
findPsf.o: findPsf.c \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phFindPsf.h ../include/phCalib1.h ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phFramestat.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phStar1.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phObjc.h \
  ../include/phPeaks.h ../include/phStar1c.h ../include/phCellFitobj.h \
  ../include/phFitobj.h ../include/phMathUtils.h ../include/phMeschach.h \
  ../include/phMathUtils.h ../include/phMeasureObj.h \
  ../include/prvt/p_phMeasureObj.h ../include/phBrightObjects.h \
  ../include/phObjects.h $(DERVISH_DIR)/include/region.h \
  ../include/phMaskbittype.h ../include/phUtils.h ../include/phOffset.h \
  ../include/phObjc.h ../include/phObjectCenter.h ../include/phExtract.h
fitobj.o: fitobj.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phObjc.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phExtract.h \
  ../include/phSpanUtil.h ../include/phConsts.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h $(DERVISH_DIR)/include/shChain.h \
  ../include/phMaskbittype.h ../include/phObjc.h ../include/phUtils.h \
  ../include/phExtract.h ../include/phDgpsf.h ../include/phFitobj.h \
  ../include/phCellFitobj.h ../include/phFitobj.h ../include/phSkyUtils.h \
  ../include/phMathUtils.h ../include/phMeschach.h \
  ../include/phMathUtils.h ../include/phUtils.h \
  ../include/phObjectCenter.h
flatfieldsCalc.o: flatfieldsCalc.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phQuartiles.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phCorrectFrames.h \
  $(DERVISH_DIR)/include/region.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phQuartiles.h \
  ../include/phCalib1.h ../include/prvt/phCorrectFrames_p.h \
  ../include/phConsts.h
flatfieldsInterp.o: flatfieldsInterp.c $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h ../include/phCorrectFrames.h \
  $(DERVISH_DIR)/include/region.h ../include/phFramestat.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/region.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phQuartiles.h \
  ../include/phCalib1.h ../include/prvt/phCorrectFrames_p.h
frameinfo.o: frameinfo.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phFrameinfo.h
framestat.o: framestat.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFramestat.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  ../include/phFilter.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h
fsc.o: fsc.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phFsc.h \
  ../include/phConsts.h
histU16.o: histU16.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phHist.h
knownobj.o: knownobj.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phKnownobj.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phOffset.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h
library.o: library.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phUtils.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phLibrary.h \
  $(DERVISH_DIR)/include/region.h ../include/phHist.h \
  ../include/phSpanUtil.h ../include/phSpanUtil.h \
  $(PGPLOT_DIR)/dst/cpgplot.h
LM_minimise.o: LM_minimise.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phMathUtils.h \
  ../include/phMeschach.h ../include/phMathUtils.h
mag.o: mag.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phMag.h \
  ../include/phFilter.h
make_photom_coeffs.o: make_photom_coeffs.c ../include/phMathUtils.h \
  ../include/phMeschach.h ../include/phMathUtils.h
makeprof.o: makeprof.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phObjc.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phExtract.h \
  ../include/phSpanUtil.h ../include/phConsts.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phDgpsf.h \
  ../include/phFitobj.h ../include/phUtils.h
makeStarlist.o: makeStarlist.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phStar1.h \
  ../include/phDgpsf.h $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phCalib1.h ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phObjc.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/phStar1c.h ../include/phExtract.h ../include/phMath.h \
  ../include/phRandom.h ../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h ../include/phMaskbittype.h \
  ../include/phUtils.h ../include/phCellFitobj.h ../include/phFitobj.h \
  ../include/phMathUtils.h ../include/phMeschach.h \
  ../include/phMathUtils.h
mathUtils.o: mathUtils.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/astrotools.h \
  $(ASTROTOOLS_DIR)/include/atLinearFits.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atSlalib.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(SDSSMATH_DIR)/include/sdssmath.h \
  $(SDSSMATH_DIR)/include/tclMath.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(SDSSMATH_DIR)/include/slatec.h \
  $(SDSSMATH_DIR)/include/smaFunc.h \
  $(SDSSMATH_DIR)/include/smaFft.h \
  $(FFTW_DIR)/src/fftw.h \
  $(ASTROTOOLS_DIR)/include/atSlalib_c.h \
  $(ASTROTOOLS_DIR)/include/atTclVerbs.h \
  $(ASTROTOOLS_DIR)/include/atSurveyGeometry.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  $(ASTROTOOLS_DIR)/include/atAirmass.h \
  $(ASTROTOOLS_DIR)/include/atGalaxy.h \
  $(ASTROTOOLS_DIR)/include/atObject.h \
  $(ASTROTOOLS_DIR)/include/atGauss.h \
  $(ASTROTOOLS_DIR)/include/atImSim.h \
  $(ASTROTOOLS_DIR)/include/atRegSci.h \
  $(ASTROTOOLS_DIR)/include/atCali.h \
  $(ASTROTOOLS_DIR)/include/atEphemeris.h \
  $(ASTROTOOLS_DIR)/include/atPMatch.h \
  $(ASTROTOOLS_DIR)/include/atCentroid.h \
  $(ASTROTOOLS_DIR)/include/atGaussianWidth.h \
  $(ASTROTOOLS_DIR)/include/atQuasar.h \
  $(ASTROTOOLS_DIR)/include/atAperture.h \
  ../include/phMathUtils.h ../include/phMeschach.h \
  ../include/phMathUtils.h ../include/phConsts.h
medianSky.o: medianSky.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phHist.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phUtils.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phConsts.h ../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h $(DERVISH_DIR)/include/shChain.h \
  ../include/phMaskbittype.h ../include/phExtract.h \
  ../include/phSpanUtil.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phVariablePsf.h ../include/phObjc.h \
  ../include/phPeaks.h ../include/phUtils.h ../include/phMathUtils.h \
  ../include/phMeschach.h ../include/phMathUtils.h
mergeColors.o: mergeColors.c ../include/phSignals.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phMaskbittype.h \
  ../include/phMergeColors.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phObjc.h ../include/phExtract.h ../include/phSpanUtil.h \
  ../include/phConsts.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phOffset.h \
  ../include/phPeaks.h ../include/phObjc.h ../include/phMeasureObj.h \
  ../include/phCalib1.h ../include/prvt/p_phMeasureObj.h \
  ../include/phUtils.h
meschach.o: meschach.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phMeschach.h ../include/phMathUtils.h \
  ../include/phMeschach.h
objc.o: objc.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phPeaks.h \
  ../include/phObjc.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phTestInfo.h
object1.o: object1.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h $(DERVISH_DIR)/include/shChain.h \
  ../include/phMaskbittype.h ../include/phExtract.h \
  ../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phObjc.h ../include/phPeaks.h \
  ../include/phUtils.h ../include/phObjc.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h
objectUtils.o: objectUtils.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phSignals.h \
  ../include/phObjects.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phMaskbittype.h \
  ../include/phExtract.h ../include/phSpanUtil.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phObjc.h ../include/phPeaks.h \
  ../include/phUtils.h ../include/phMaskbittype.h ../include/phExtract.h \
  ../include/phBrightObjects.h ../include/phCalib1.h \
  ../include/phObjects.h ../include/phOffset.h ../include/phSkyUtils.h \
  ../include/phObjectCenter.h ../include/phMergeColors.h \
  ../include/phConsts.h
offset.o: offset.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phOffset.h \
  ../include/phFramestat.h $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phConsts.h ../include/phObjc.h \
  ../include/phPeaks.h ../include/phUtils.h
phAnalysis.o: phAnalysis.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phObjc.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phExtract.h \
  ../include/phSpanUtil.h ../include/phConsts.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phAnalysis.h \
  ../include/phObjc.h $(PGPLOT_DIR)/dst/cpgplot.h
phInterp.o: phInterp.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phInterp.h ../include/phUtils.h
photoFitsIO.o: photoFitsIO.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phPhotoFitsIO.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phSpanUtil.h \
  ../include/phConsts.h ../include/phUtils.h ../include/phObjc.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phDataIo.h \
  ../include/phObjc.h ../include/phRice.h \
  $(DERVISH_DIR)/include/prvt/region_p.h
photo_main.o: photo_main.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(SDSSMATH_DIR)/include/tclMath.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/astrotools.h \
  $(ASTROTOOLS_DIR)/include/atLinearFits.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atSlalib.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(SDSSMATH_DIR)/include/sdssmath.h \
  $(SDSSMATH_DIR)/include/tclMath.h \
  $(SDSSMATH_DIR)/include/slatec.h \
  $(SDSSMATH_DIR)/include/smaFunc.h \
  $(SDSSMATH_DIR)/include/smaFft.h \
  $(FFTW_DIR)/src/fftw.h \
  $(ASTROTOOLS_DIR)/include/atSlalib_c.h \
  $(ASTROTOOLS_DIR)/include/atTclVerbs.h \
  $(ASTROTOOLS_DIR)/include/atSurveyGeometry.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  $(ASTROTOOLS_DIR)/include/atAirmass.h \
  $(ASTROTOOLS_DIR)/include/atGalaxy.h \
  $(ASTROTOOLS_DIR)/include/atObject.h \
  $(ASTROTOOLS_DIR)/include/atGauss.h \
  $(ASTROTOOLS_DIR)/include/atImSim.h \
  $(ASTROTOOLS_DIR)/include/atRegSci.h \
  $(ASTROTOOLS_DIR)/include/atCali.h \
  $(ASTROTOOLS_DIR)/include/atEphemeris.h \
  $(ASTROTOOLS_DIR)/include/atPMatch.h \
  $(ASTROTOOLS_DIR)/include/atCentroid.h \
  $(ASTROTOOLS_DIR)/include/atGaussianWidth.h \
  $(ASTROTOOLS_DIR)/include/atQuasar.h \
  $(ASTROTOOLS_DIR)/include/atAperture.h \
  ../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phSignals.h ../include/phMeschach.h \
  ../include/phMathUtils.h ../include/phMeschach.h
photpar.o: photpar.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phPhotpar.h \
  $(DERVISH_DIR)/include/dervish.h
phSpanUtil.o: phSpanUtil.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phUtils.h
phTclVerbs.o: phTclVerbs.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(SDSSMATH_DIR)/include/tclMath.h \
  $(DERVISH_DIR)/include/dervish.h
quartiles.o: quartiles.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phQuartiles.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/prvt/phCorrectFrames_p.h
random.o: random.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phRandom.h \
  ../include/phRanlib.h
ranlib.o: ranlib.c ../include/phRanlib.h
rice.o: rice.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phRice.h \
  ../include/phConsts.h
sky.o: sky.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phUtils.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phSpanUtil.h
skyObjects.o: skyObjects.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phObjc.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phExtract.h \
  ../include/phSpanUtil.h ../include/phConsts.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phRandom.h \
  ../include/phUtils.h
star1.o: star1.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phStar1.h \
  ../include/phDgpsf.h $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phCalib1.h ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phObjc.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/phFindPsf.h ../include/phDgpsf.h ../include/phUtils.h \
  ../include/phObjectCenter.h ../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h ../include/phMaskbittype.h \
  ../include/phUtils.h ../include/phObjc.h ../include/phMeasureObj.h \
  ../include/prvt/p_phMeasureObj.h ../include/phExtract.h \
  ../include/phMathUtils.h ../include/phMeschach.h \
  ../include/phMathUtils.h ../include/phFocus.h ../include/phCellFitobj.h \
  ../include/phFitobj.h ../include/phCosmic.h ../include/phRandom.h
star1c.o: star1c.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/astrotools.h \
  $(ASTROTOOLS_DIR)/include/atLinearFits.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atSlalib.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(SDSSMATH_DIR)/include/sdssmath.h \
  $(SDSSMATH_DIR)/include/tclMath.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(SDSSMATH_DIR)/include/slatec.h \
  $(SDSSMATH_DIR)/include/smaFunc.h \
  $(SDSSMATH_DIR)/include/smaFft.h \
  $(FFTW_DIR)/src/fftw.h \
  $(ASTROTOOLS_DIR)/include/atSlalib_c.h \
  $(ASTROTOOLS_DIR)/include/atTclVerbs.h \
  $(ASTROTOOLS_DIR)/include/atSurveyGeometry.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  $(ASTROTOOLS_DIR)/include/atAirmass.h \
  $(ASTROTOOLS_DIR)/include/atGalaxy.h \
  $(ASTROTOOLS_DIR)/include/atObject.h \
  $(ASTROTOOLS_DIR)/include/atGauss.h \
  $(ASTROTOOLS_DIR)/include/atImSim.h \
  $(ASTROTOOLS_DIR)/include/atRegSci.h \
  $(ASTROTOOLS_DIR)/include/atCali.h \
  $(ASTROTOOLS_DIR)/include/atEphemeris.h \
  $(ASTROTOOLS_DIR)/include/atPMatch.h \
  $(ASTROTOOLS_DIR)/include/atCentroid.h \
  $(ASTROTOOLS_DIR)/include/atGaussianWidth.h \
  $(ASTROTOOLS_DIR)/include/atQuasar.h \
  $(ASTROTOOLS_DIR)/include/atAperture.h \
  ../include/phCorrectFrames.h $(DERVISH_DIR)/include/region.h \
  ../include/phFramestat.h $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  ../include/phFilter.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phQuartiles.h \
  ../include/phCalib1.h ../include/phStar1.h ../include/phObjc.h \
  ../include/phPeaks.h ../include/phStar1c.h ../include/phFocus.h \
  ../include/phOffset.h ../include/phMeasureObj.h \
  ../include/prvt/p_phMeasureObj.h ../include/phCellFitobj.h \
  ../include/phFitobj.h
star1starcmerge.o: star1starcmerge.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phStar1starcmerge.h \
  ../include/phStar1.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phCalib1.h ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phObjc.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/phStarc.h ../include/phMag.h
starc.o: starc.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phStarc.h \
  ../include/phMag.h ../include/phFilter.h
tclAnalysis.o: tclAnalysis.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phAnalysis.h \
  ../include/phObjc.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phObjc.h
tclBrightObjects.o: tclBrightObjects.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phBrightObjects.h \
  ../include/phCalib1.h ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phObjects.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phMaskbittype.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phObjc.h \
  ../include/phPeaks.h ../include/phUtils.h ../include/phOffset.h
tclCalib1byframe.o: tclCalib1byframe.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phCalib1.h \
  ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phUtils.h
tclCalib1bytime.o: tclCalib1bytime.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phCalib1.h \
  ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phUtils.h
tclCalib1.o: tclCalib1.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phCalib1.h \
  ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h
tclCcdpars.o: tclCcdpars.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h
tclCorrectFrames.o: tclCorrectFrames.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phUtils.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phCorrectFrames.h \
  $(DERVISH_DIR)/include/region.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phSkyUtils.h \
  ../include/phRandom.h ../include/phRanlib.h ../include/phVariablePsf.h \
  ../include/phCcdpars.h $(DERVISH_DIR)/include/shChain.h \
  ../include/phQuartiles.h ../include/phCalib1.h ../include/phObjects.h \
  ../include/phMaskbittype.h ../include/phObjc.h ../include/phPeaks.h \
  ../include/phUtils.h
tclCR.o: tclCR.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phLibrary.h \
  $(DERVISH_DIR)/include/region.h ../include/phHist.h \
  ../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phCosmic.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phVariablePsf.h ../include/phUtils.h
tclDefineTranslate.o: tclDefineTranslate.c \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phFilter.h
tclDgpsf.o: tclDgpsf.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phUtils.h
tclDither.o: tclDither.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phDither.h
tclExtinction.o: tclExtinction.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phExtinction.h ../include/phMag.h ../include/phFilter.h
tclExtract.o: tclExtract.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phExtract.h ../include/phSpanUtil.h
tclFinder.o: tclFinder.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phObjects.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phMaskbittype.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phObjc.h ../include/phPeaks.h \
  ../include/phUtils.h ../include/phMeasureObj.h ../include/phCalib1.h \
  ../include/prvt/p_phMeasureObj.h
tclFindKnownObjects.o: tclFindKnownObjects.c \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phKnownobj.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phOffset.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phUtils.h
tclFindPhotoParams.o: tclFindPhotoParams.c \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phFindPhotoParams.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phCalib1.h \
  ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h \
  ../include/phExtinction.h ../include/phMag.h ../include/phFrameinfo.h \
  ../include/phStar1starcmerge.h ../include/phStar1.h \
  ../include/phCcdpars.h ../include/phObjc.h ../include/phFramestat.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phStarc.h
tclFindPsf.o: tclFindPsf.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phStar1.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phCalib1.h \
  ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phObjc.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/phFindPsf.h
tclFitobj.o: tclFitobj.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phObjects.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phMaskbittype.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phObjc.h ../include/phPeaks.h \
  ../include/phUtils.h ../include/phFitobj.h ../include/phCellFitobj.h \
  ../include/phFitobj.h ../include/phExtract.h
tclFlatfieldsCalc.o: tclFlatfieldsCalc.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phQuartiles.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phCorrectFrames.h \
  $(DERVISH_DIR)/include/region.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phQuartiles.h \
  ../include/phCalib1.h
tclFlatfieldsInterp.o: tclFlatfieldsInterp.c \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phCorrectFrames.h \
  $(DERVISH_DIR)/include/region.h ../include/phFramestat.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phQuartiles.h \
  ../include/phCalib1.h
tclFrameinfo.o: tclFrameinfo.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phFrameinfo.h
tclFramestat.o: tclFramestat.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  ../include/phFilter.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phSkyUtils.h \
  ../include/phRandom.h ../include/phRanlib.h ../include/phVariablePsf.h
tclFsc.o: tclFsc.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phFsc.h
tclHistU16.o: tclHistU16.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phHist.h
tclKnownobj.o: tclKnownobj.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phKnownobj.h ../include/phOffset.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phSkyUtils.h \
  ../include/phRandom.h ../include/phRanlib.h ../include/phVariablePsf.h
tclLibrary.o: tclLibrary.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phLibrary.h $(DERVISH_DIR)/include/region.h \
  ../include/phHist.h ../include/phSpanUtil.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phUtils.h
tclMag.o: tclMag.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phMag.h \
  ../include/phFilter.h
tclMakeStarlist.o: tclMakeStarlist.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phStar1.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phCalib1.h \
  ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phObjc.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/phStar1c.h ../include/phRandom.h
tclMeasureObj.o: tclMeasureObj.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phMeasureObj.h ../include/phCalib1.h ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phFramestat.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phObjc.h ../include/phPeaks.h \
  ../include/prvt/p_phMeasureObj.h
tclMergeColors.o: tclMergeColors.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phMergeColors.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phObjc.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phOffset.h \
  ../include/phUtils.h
tclMeschach.o: tclMeschach.c ../include/phMeschach.h \
  ../include/phMathUtils.h ../include/phMeschach.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h
tclObjc.o: tclObjc.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phObjc.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phExtract.h \
  ../include/phSpanUtil.h ../include/phConsts.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phUtils.h \
  ../include/phTestInfo.h ../include/phDataIo.h ../include/phObjc.h \
  ../include/phRice.h
tclObject1.o: tclObject1.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phObjc.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phExtract.h \
  ../include/phSpanUtil.h ../include/phConsts.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h
tclOffset.o: tclOffset.c ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phOffset.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phSkyUtils.h \
  ../include/phRandom.h ../include/phRanlib.h ../include/phVariablePsf.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h
tclPhotoFitsIO.o: tclPhotoFitsIO.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phPhotoFitsIO.h
tclPhotpar.o: tclPhotpar.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phPhotpar.h \
  $(DERVISH_DIR)/include/dervish.h
tclQuartiles.o: tclQuartiles.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phQuartiles.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h
tclRandom.o: tclRandom.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phRandom.h ../include/phRanlib.h
tclSkyUtils.o: tclSkyUtils.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h
tclSpanUtil.o: tclSpanUtil.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phSpanUtil.h ../include/phDataIo.h ../include/phObjc.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phPeaks.h ../include/phRice.h
tclStar1.o: tclStar1.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phStar1.h ../include/phDgpsf.h ../include/phCalib1.h \
  ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phObjc.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/phUtils.h
tclStar1c.o: tclStar1c.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phStar1.h ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phCalib1.h \
  ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phObjc.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/phStar1c.h ../include/phRandom.h ../include/phConsts.h
tclStar1starcmerge.o: tclStar1starcmerge.c \
  $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phStar1starcmerge.h \
  ../include/phStar1.h ../include/phDgpsf.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phConsts.h \
  ../include/phCalib1.h ../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phCcdpars.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phObjc.h \
  ../include/phFramestat.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/phStarc.h ../include/phMag.h
tclStarc.o: tclStarc.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/astrotools.h \
  $(ASTROTOOLS_DIR)/include/atLinearFits.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(ASTROTOOLS_DIR)/include/atSlalib.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(SDSSMATH_DIR)/include/sdssmath.h \
  $(SDSSMATH_DIR)/include/tclMath.h \
  $(DERVISH_DIR)/include/dervish.h \
  $(SDSSMATH_DIR)/include/slatec.h \
  $(SDSSMATH_DIR)/include/smaFunc.h \
  $(SDSSMATH_DIR)/include/smaFft.h \
  $(FFTW_DIR)/src/fftw.h \
  $(ASTROTOOLS_DIR)/include/atSlalib_c.h \
  $(ASTROTOOLS_DIR)/include/atTclVerbs.h \
  $(ASTROTOOLS_DIR)/include/atSurveyGeometry.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  $(ASTROTOOLS_DIR)/include/atAirmass.h \
  $(ASTROTOOLS_DIR)/include/atGalaxy.h \
  $(ASTROTOOLS_DIR)/include/atObject.h \
  $(ASTROTOOLS_DIR)/include/atGauss.h \
  $(ASTROTOOLS_DIR)/include/atImSim.h \
  $(ASTROTOOLS_DIR)/include/atRegSci.h \
  $(ASTROTOOLS_DIR)/include/atCali.h \
  $(ASTROTOOLS_DIR)/include/atEphemeris.h \
  $(ASTROTOOLS_DIR)/include/atPMatch.h \
  $(ASTROTOOLS_DIR)/include/atCentroid.h \
  $(ASTROTOOLS_DIR)/include/atGaussianWidth.h \
  $(ASTROTOOLS_DIR)/include/atQuasar.h \
  $(ASTROTOOLS_DIR)/include/atAperture.h \
  ../include/phStarc.h ../include/phMag.h ../include/phFilter.h
tclUmedian.o: tclUmedian.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phHist.h ../include/phUmedian.h
tclUtils.o: tclUtils.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phDervishUtils.h \
  ../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phUtils.h ../include/phChainDiff.h ../include/phObjc.h \
  ../include/phExtract.h ../include/phSpanUtil.h ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phVariablePsf.h ../include/phPeaks.h \
  ../include/phObjectCenter.h ../include/phSignals.h
tclVariablePsf.o: tclVariablePsf.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phVariablePsf.h $(DERVISH_DIR)/include/dervish.h
tclVecUtils.o: tclVecUtils.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phVecUtils.h
tclWallpaper.o: tclWallpaper.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phWallpaper.h
umedian.o: umedian.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phMath.h ../include/phUtils.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phHist.h ../include/phLibrary.h \
  $(DERVISH_DIR)/include/region.h ../include/phHist.h \
  ../include/phSpanUtil.h ../include/phMathUtils.h \
  ../include/phMeschach.h ../include/phMathUtils.h
utils.o: utils.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phDervishUtils.h ../include/phChainDiff.h \
  ../include/phRandom.h ../include/phRanlib.h ../include/phExtract.h \
  ../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phSkyUtils.h ../include/phRandom.h \
  $(DERVISH_DIR)/include/prvt/region_p.h ../include/phVariablePsf.h \
  ../include/phMeschach.h ../include/phMathUtils.h \
  ../include/phMeschach.h ../include/phUtils.h
variablePsf.o: variablePsf.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phUtils.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phSkyUtils.h ../include/phRandom.h \
  ../include/phRanlib.h ../include/phMeschach.h ../include/phMathUtils.h \
  ../include/phMeschach.h ../include/phConsts.h \
  ../include/phVariablePsf.h
vecUtils.o: vecUtils.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phUtils.h $(DERVISH_DIR)/include/dervish.h \
  ../include/phConsts.h ../include/phVecUtils.h
wallpaper.o: wallpaper.c $(DERVISH_DIR)/include/dervish.h \
  $(TCLX_DIR)/include/tclExtend.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftcl.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclParseArgv.h \
  $(DERVISH_DIR)/include/dervish_msg_c.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shCAlign.h $(DERVISH_DIR)/include/shCSchema.h \
  $(DERVISH_DIR)/include/shCHdr.h $(DERVISH_DIR)/include/shC.h \
  $(DERVISH_DIR)/include/shChain.h $(DERVISH_DIR)/include/shCArray.h \
  $(DERVISH_DIR)/include/shCVecExpr.h $(DERVISH_DIR)/include/shCUtils.h \
  $(DERVISH_DIR)/include/shCErrStack.h \
  $(DERVISH_DIR)/include/shTclErrStack.h \
  $(DERVISH_DIR)/include/shCSchemaCnv.h \
  $(DERVISH_DIR)/include/shTclHandle.h $(DERVISH_DIR)/include/shCList.h \
  $(DERVISH_DIR)/include/shCTbl.h \
  $(LIBFITS_DIR)/src/libfits.h \
  $(DERVISH_DIR)/include/shCFitsIo.h \
  $(DERVISH_DIR)/include/shCSchemaTrans.h \
  $(DERVISH_DIR)/include/shCSchemaSupport.h \
  $(DERVISH_DIR)/include/shCFits.h $(DERVISH_DIR)/include/shCMaskUtils.h \
  $(DERVISH_DIR)/include/shCRegUtils.h \
  $(DERVISH_DIR)/include/shCRegPrint.h $(DERVISH_DIR)/include/shCHash.h \
  $(DERVISH_DIR)/include/shCAssert.h $(DERVISH_DIR)/include/shCEnvscan.h \
  $(DERVISH_DIR)/include/shTclRegSupport.h \
  $(DERVISH_DIR)/include/shCSao.h $(DERVISH_DIR)/include/imtool.h \
  $(DERVISH_DIR)/include/shTclParseArgv.h \
  $(DERVISH_DIR)/include/shTclVerbs.h $(DERVISH_DIR)/include/shTclSao.h \
  $(TCL_DIR)/include/tcl.h \
  $(FTCL_DIR)/include/ftclCmdLine.h \
  $(DERVISH_DIR)/include/shTclUtils.h $(DERVISH_DIR)/include/shTk.h \
  $(DERVISH_DIR)/include/shCGarbage.h $(DERVISH_DIR)/include/shCHg.h \
  $(DERVISH_DIR)/include/shCLink.h $(DERVISH_DIR)/include/shCLut.h \
  $(DERVISH_DIR)/include/shCVecChain.h $(DERVISH_DIR)/include/shCDemo.h \
  $(DERVISH_DIR)/include/shCDiskio.h $(DERVISH_DIR)/include/shCHist.h \
  $(DERVISH_DIR)/include/shCMask.h $(DERVISH_DIR)/include/shCMemory.h \
  $(DERVISH_DIR)/include/shCRegPhysical.h \
  $(DERVISH_DIR)/include/shTclAlign.h $(DERVISH_DIR)/include/shTclArray.h \
  $(DERVISH_DIR)/include/shTclFits.h $(DERVISH_DIR)/include/shTclHdr.h \
  $(DERVISH_DIR)/include/shTclSupport.h \
  $(DERVISH_DIR)/include/shTclTree.h ../include/phConsts.h \
  ../include/phObjects.h $(DERVISH_DIR)/include/region.h \
  $(DERVISH_DIR)/include/shChain.h ../include/phMaskbittype.h \
  ../include/phExtract.h ../include/phSpanUtil.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phConsts.h \
  ../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../include/phFilter.h \
  ../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../include/phSkyUtils.h ../include/phRandom.h ../include/phRanlib.h \
  ../include/phVariablePsf.h ../include/phObjc.h ../include/phPeaks.h \
  ../include/phUtils.h ../include/phSpanUtil.h ../include/phWallpaper.h
