SHELL = /bin/sh
#
#
#  Makefile to build the MeasureObjects Module's library
#
#  This file is designed to be invoked by a shell script called sdssmake
#  which selects supported options.
#
.c.o :
	$(CC) -c $(CCCHK) $(CFLAGS) $*.c
	@- if [ -f ../.config ]; then \
		echo $(CC) -c $(CCCHK) $(CFLAGS) measureObj/$*.c >>../.config;\
	fi
#
INC	= ../../include
LIB	= ../../lib
INCDIRS	= -I$(INC) -I$(INC)/prvt -I$(DERVISH_DIR)/include \
			-I$(DERVISH_DIR)/contrib -I$(FTCL_DIR)/include \
			-I$(FTCL_DIR)/src -I$(LIBFITS_DIR)/src\
			-I$(FPGPLOT_DIR)/include -I$(PGPLOT_DIR)/dst \
			-I$(TCLX_DIR)/include -I$(TCLX_DIR)/src/src \
			-I$(TCL_DIR)/include -I$(TCL_DIR)/src \
			-I$(SDSSMATH_DIR)/include \
			-I$(SLALIB_DIR)/src_c -I$(ASTROTOOLS_DIR)/include


CFLAGS  = $(SDSS_CFLAGS) $(INCDIRS) -DCHECK_LEAKS $(EXTRA_CFLAGS)

OBJECTS = setupMeasureObj.o atlasImages.o deblend.o \
	  measureObj.o peaks.o stokes.o adaptive.o

LDFLAGS = 

#=============================================================================
# TARGETS
#=============================================================================

default:	
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

all : $(LIB)/libmeasureObj.a 
 
$(LIB)/libmeasureObj.a : $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB)/libmeasureObj.a $?
	- if [ "$(RANLIB)" != "" -a -x "$(RANLIB)" ]; then \
		$(RANLIB) $(LIB)/libmeasureObj.a; \
	fi
	@echo "=== PHOTO BUILD : Building Unix library (libmeasureObj.a) ==="
	@echo
#
# These files include other .c files
#
stokes.o : stokes_coeffs.c

clean :
	rm -f *~ core *.bak *.orig *.old .#* #*#
	rm -f *.o

install :
	/bin/cp Makefile $(PHOTO_DIR)/src/measureObj/Makefile
	/bin/cp *.c $(PHOTO_DIR)/src/measureObj

tags :
	/usr/local/bin/etags -aet -f ../etc/TAGS *.[ch]
#
make :
	@if [ "$(CCENV)" = "" ]; then ccenv=`uname`; else ccenv=$(CCENV); fi; \
	echo \
	"make_make -cc '$(CC) $(CFLAGS)' -nostd -file Makefile" \
							"-env $$ccenv *.c"; \
	$(SDSSTOOLS_DIR)/bin/make_make -cc '$(CC) $(CFLAGS)' -nostd \
					-file Makefile -env $$ccenv *.c
#
# include file dependencies.
#
# All line below START_DEPEND are machine generated; Do Not Edit
#
#START_DEPEND
adaptive.o: adaptive.c $(DERVISH_DIR)/include/dervish.h \
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
  $(DERVISH_DIR)/include/shTclTree.h ../../include/phMeasureObj.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phCalib1.h \
  ../../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../../include/phExtract.h ../../include/phSpanUtil.h \
  ../../include/phConsts.h ../../include/phFramestat.h \
  ../../include/phSkyUtils.h ../../include/phRandom.h \
  ../../include/phRanlib.h ../../include/phVariablePsf.h \
  ../../include/phObjc.h ../../include/phPeaks.h \
  ../../include/prvt/p_phMeasureObj.h ../../include/phObjectCenter.h \
  ../../include/phMathUtils.h ../../include/phMeschach.h \
  ../../include/phMathUtils.h
atlasImages.o: atlasImages.c $(DERVISH_DIR)/include/dervish.h \
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
  $(DERVISH_DIR)/include/shTclTree.h ../../include/phRandom.h \
  ../../include/phRanlib.h ../../include/phMeasureObj.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phCalib1.h \
  ../../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../../include/phExtract.h ../../include/phSpanUtil.h \
  ../../include/phConsts.h ../../include/phFramestat.h \
  ../../include/phSkyUtils.h ../../include/phRandom.h \
  ../../include/phVariablePsf.h ../../include/phObjc.h \
  ../../include/phPeaks.h ../../include/prvt/p_phMeasureObj.h \
  ../../include/phOffset.h ../../include/prvt/phObjc_p.h
deblend.o: deblend.c $(DERVISH_DIR)/include/dervish.h \
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
  $(DERVISH_DIR)/include/shTclTree.h ../../include/phMeasureObj.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phCalib1.h \
  ../../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../../include/phExtract.h ../../include/phSpanUtil.h \
  ../../include/phConsts.h ../../include/phFramestat.h \
  ../../include/phSkyUtils.h ../../include/phRandom.h \
  ../../include/phRanlib.h ../../include/phVariablePsf.h \
  ../../include/phObjc.h ../../include/phPeaks.h \
  ../../include/prvt/p_phMeasureObj.h ../../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h $(DERVISH_DIR)/include/shChain.h \
  ../../include/phMaskbittype.h ../../include/phUtils.h \
  ../../include/phObjectCenter.h ../../include/phPeaks.h \
  ../../include/phUtils.h ../../include/phMathUtils.h \
  ../../include/phMeschach.h ../../include/phMathUtils.h \
  ../../include/phCellFitobj.h ../../include/phFitobj.h \
  ../../include/phMergeColors.h ../../include/phOffset.h
measureObj.o: measureObj.c $(DERVISH_DIR)/include/dervish.h \
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
  $(DERVISH_DIR)/include/shTclTree.h ../../include/phSignals.h \
  ../../include/phExtract.h ../../include/phSpanUtil.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phConsts.h \
  ../../include/phMathUtils.h ../../include/phMeschach.h \
  ../../include/phMathUtils.h ../../include/phMeasureObj.h \
  ../../include/phCalib1.h ../../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../../include/phExtract.h ../../include/phFramestat.h \
  ../../include/phSkyUtils.h ../../include/phRandom.h \
  ../../include/phRanlib.h ../../include/phVariablePsf.h \
  ../../include/phObjc.h ../../include/phPeaks.h \
  ../../include/prvt/p_phMeasureObj.h ../../include/phCellFitobj.h \
  ../../include/phFitobj.h ../../include/phOffset.h \
  ../../include/phCosmic.h ../../include/phUtils.h \
  ../../include/phObjectCenter.h ../../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h $(DERVISH_DIR)/include/shChain.h \
  ../../include/phMaskbittype.h ../../include/phUtils.h \
  ../../include/phTestInfo.h
peaks.o: peaks.c $(DERVISH_DIR)/include/dervish.h \
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
  $(DERVISH_DIR)/include/shTclTree.h ../../include/phObjects.h \
  $(DERVISH_DIR)/include/region.h $(DERVISH_DIR)/include/shChain.h \
  ../../include/phMaskbittype.h ../../include/phExtract.h \
  ../../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../../include/phConsts.h ../../include/phFramestat.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phFilter.h \
  ../../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../../include/phSkyUtils.h ../../include/phRandom.h \
  ../../include/phRanlib.h ../../include/phVariablePsf.h \
  ../../include/phObjc.h ../../include/phPeaks.h ../../include/phUtils.h
setupMeasureObj.o: setupMeasureObj.c $(DERVISH_DIR)/include/dervish.h \
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
  $(DERVISH_DIR)/include/shTclTree.h ../../include/phMaskbittype.h \
  ../../include/phMeasureObj.h $(DERVISH_DIR)/include/dervish.h \
  ../../include/phCalib1.h ../../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../../include/phExtract.h ../../include/phSpanUtil.h \
  ../../include/phConsts.h ../../include/phFramestat.h \
  ../../include/phSkyUtils.h ../../include/phRandom.h \
  ../../include/phRanlib.h ../../include/phVariablePsf.h \
  ../../include/phObjc.h ../../include/phPeaks.h \
  ../../include/prvt/p_phMeasureObj.h ../../include/phCellFitobj.h \
  ../../include/phFitobj.h ../../include/phUtils.h \
  ../../include/phVariablePsf.h
stokes.o: stokes.c $(DERVISH_DIR)/include/dervish.h \
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
  $(DERVISH_DIR)/include/shTclTree.h ../../include/phExtract.h \
  ../../include/phSpanUtil.h $(DERVISH_DIR)/include/dervish.h \
  ../../include/phConsts.h ../../include/phMeasureObj.h \
  ../../include/phCalib1.h ../../include/phFilter.h \
  $(ASTROTOOLS_DIR)/include/atTrans.h \
  $(DERVISH_DIR)/include/dervish.h ../../include/phDgpsf.h \
  $(ASTROTOOLS_DIR)/include/atConversions.h \
  $(SDSSMATH_DIR)/include/smaValues.h \
  ../../include/phExtract.h ../../include/phFramestat.h \
  ../../include/phSkyUtils.h ../../include/phRandom.h \
  ../../include/phRanlib.h ../../include/phVariablePsf.h \
  ../../include/phObjc.h ../../include/phPeaks.h \
  ../../include/prvt/p_phMeasureObj.h stokes_coeffs.c
