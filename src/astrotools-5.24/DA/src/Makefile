# Makefile for DA directory in ASTROTOOLS
# !!!!! DO NOT USE THIS MAKEFILE IF COMPLILING DA ASTROLINE!!!!!!!!!!!!!!!!!

SHELL = /bin/sh

#=============================================================================
#
#  INSTALL_DIR   --- Root directory into which the product is to be installed.
#  INSTALL_FILES --- Names of the files to be installed.
#  CLEAN_FILES   --- Names of specific files to be deleted when the directory
#                    is cleaned.  Files matching one of a default set of file
#                    name templates will be deleted automatically
#                    (i.e., *~ core *.bak *.orig *.old .#* #*# *.o).  This
#                    variable is for declaring additional files to delete which
#                    don't match any of the file name templates.
#  OBJECTS       --- List of object file names.
#  DISKIO_FILES  --- *.h files which include structures which need to be read
#                    by make_io in order to create their associated schema.
#  DISKIO_PREFIX --- Two letter prefix to use with generated schema.
#  DISKIO_NAME   --- Name used to group all of the schema.
#  LIB_NAME      --- Name of the single object code library.  If this product
#                    creates more than one library, then this file will have to
#                    be edited beyond the usual required edits.
#  EXECUTABLE    --- Name of the single executable.  If this product creates
#                    more than one executable, then this file will have to
#                    be edited beyond the usual required edits.
#  MAIN_PGM      --- Name of the source file for the main program.
#
#  If more than one library or executable is associated with this product,
#  additional changes will be required to this file.  This should require:
#  1) Replicating similar targets for the additional libraries and executables
#     from the existing targets in the "OTHER TARGETS" section of this file.
#  2) Adding the new targets to the list of dependencies for the "all" target.
#
#=============================================================================

INSTALL_DIR   = $(ASTROTOOLS_INSTALL_DIR)
INSTALL_FILES = *.c Makefile
CLEAN_FILES   = *.o

TCLOBJECTS    =

OBJECTS       = atCentroid.o atGaussianWidth.o

DISKIO_FILES  =
DISKIO_PREFIX = at
DISKIO_NAME   = Astrotools

INC	= ../../include
DAINC	= ../include
SRC	= .

INCDIRS = -I$(INC) -I$(DAINC) -I$(FTCL_DIR)/include -I$(FTCL_DIR)/src \
	  -I$(LIBFITS_DIR)/src -I$(FPGPLOT_DIR)/include \
	  -I$(PGPLOT_DIR)/cpg -I$(DERVISH_DIR)/include \
	  -I$(SDSSMATH_DIR)/include \
	  -I$(SLALIB_DIR)/src_c \
	  -I$(DERVISH_DIR)/contrib \
	  -I$(TCLX_DIR)/include -I$(TCLX_DIR)/src/src \
	  -I$(TCL_DIR)/include -I$(TCL_DIR)/src
CFLAGS  = $(SDSS_CFLAGS) $(INCDIRS) $(CCCHK)
CFLAGS_DEFGP  = $(SDSS_CFLAGS_DEFGP) $(INCDIRS)

#   default rule for compiling
.c.o:
	$(CC) -c $(CFLAGS) $*.c


#===========================================================================
# STANDARD TARGETS
#===========================================================================

default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

all : $(OBJECTS)

install :
	@if [ "$(INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi     
	@ rm -rf $(INSTALL_DIR)/DA/src
	@ mkdir $(INSTALL_DIR)/DA/src
	@if [ "$(INSTALL_FILES)" != "" ]; then \
		cp -p $(INSTALL_FILES) $(INSTALL_DIR)/DA/src; \
		chmod 644 $(INSTALL_DIR)/DA/src/*; \
	fi

clean :
	rm -f $(CLEAN_FILES) *~ core *.bak *.orig *.old .#* #*#

#===========================================================================
# SEMI-STANDARD TARGETS
#===========================================================================

tags :
	/usr/local/bin/etags -aet -f $(ETC)/TAGS *.[ch]

make :
	@if [ "$(CCENV)" = "" ]; then ccenv=`uname`; else ccenv=$(CCENV); fi; \
	echo \
	"make_make -cc '$(CC) $(CFLAGS)' -nostd -file Makefile" \
							"-env $$ccenv *.c"; \
	make_make -cc '$(CC) $(CFLAGS)' -nostd -file Makefile \
							 -env $$ccenv *.c

#===========================================================================
# INCLUDE FILE DEPENDENCIES
#===========================================================================
#
# include file dependencies.
# All lines below START_DEPEND are machine generated; Do Not Edit
# 
#START_DEPEND
atCentroid.o:	atCentroid.c
atCentroid.o:	$(DERVISH_DIR)/include/dervish.h
atCentroid.o:	$(TCLX_DIR)/include/tclExtend.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(FTCL_DIR)/include/ftcl.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(FTCL_DIR)/include/ftclParseArgv.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCAlign.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCHdr.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shC.h
atCentroid.o:	$(DERVISH_DIR)/include/shChain.h
atCentroid.o:	$(DERVISH_DIR)/include/shCArray.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCUtils.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/shCErrStack.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclErrStack.h
atCentroid.o:	$(FTCL_DIR)/include/ftcl.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(DERVISH_DIR)/include/shCArray.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchemaCnv.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclHandle.h
atCentroid.o:	$(FTCL_DIR)/include/ftcl.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/shCList.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/shChain.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/shCTbl.h
atCentroid.o:	$(LIBFITS_DIR)/src/libfits.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/shCAlign.h
atCentroid.o:	$(DERVISH_DIR)/include/shChain.h
atCentroid.o:	$(DERVISH_DIR)/include/shCArray.h
atCentroid.o:	$(DERVISH_DIR)/include/shCFitsIo.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/shCHdr.h
atCentroid.o:	$(DERVISH_DIR)/include/shCUtils.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclHandle.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/shCTbl.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchemaSupport.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclHandle.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
atCentroid.o:	$(DERVISH_DIR)/include/shCTbl.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchemaSupport.h
atCentroid.o:	$(DERVISH_DIR)/include/shCHdr.h
atCentroid.o:	$(DERVISH_DIR)/include/shCFitsIo.h
atCentroid.o:	$(DERVISH_DIR)/include/shCFits.h
atCentroid.o:	$(LIBFITS_DIR)/src/libfits.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclHandle.h
atCentroid.o:	$(DERVISH_DIR)/include/shCFitsIo.h
atCentroid.o:	$(DERVISH_DIR)/include/shCTbl.h
atCentroid.o:	$(DERVISH_DIR)/include/shCTbl.h
atCentroid.o:	$(DERVISH_DIR)/include/shCMaskUtils.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/shCRegUtils.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/shCRegPrint.h
atCentroid.o:	$(DERVISH_DIR)/include/shCHash.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCAssert.h
atCentroid.o:	$(DERVISH_DIR)/include/shCUtils.h
atCentroid.o:	$(DERVISH_DIR)/include/shCEnvscan.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
atCentroid.o:	$(FTCL_DIR)/include/ftcl.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSao.h
atCentroid.o:	$(DERVISH_DIR)/include/imtool.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclParseArgv.h
atCentroid.o:	$(FTCL_DIR)/include/ftcl.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclVerbs.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclSao.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(FTCL_DIR)/include/ftclCmdLine.h
atCentroid.o:	$(DERVISH_DIR)/include/imtool.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSao.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclUtils.h
atCentroid.o:	$(FTCL_DIR)/include/ftcl.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(DERVISH_DIR)/include/shTk.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclHandle.h
atCentroid.o:	$(DERVISH_DIR)/include/shCGarbage.h
atCentroid.o:	$(DERVISH_DIR)/include/shCUtils.h
atCentroid.o:	$(DERVISH_DIR)/include/shCHg.h
atCentroid.o:	$(DERVISH_DIR)/include/shCErrStack.h
atCentroid.o:	$(DERVISH_DIR)/include/shCRegUtils.h
atCentroid.o:	$(DERVISH_DIR)/include/shCAssert.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclVerbs.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclHandle.h
atCentroid.o:	$(DERVISH_DIR)/include/shCLink.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCList.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/shCVecExpr.h
atCentroid.o:	$(DERVISH_DIR)/include/shCLut.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
atCentroid.o:	$(DERVISH_DIR)/include/shCVecExpr.h
atCentroid.o:	$(DERVISH_DIR)/include/shCVecChain.h
atCentroid.o:	$(DERVISH_DIR)/include/shChain.h
atCentroid.o:	$(DERVISH_DIR)/include/shCVecExpr.h
atCentroid.o:	$(DERVISH_DIR)/include/shCHg.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/imtool.h
atCentroid.o:	$(DERVISH_DIR)/include/shCAlign.h
atCentroid.o:	$(DERVISH_DIR)/include/shCDemo.h
atCentroid.o:	$(DERVISH_DIR)/include/shCDiskio.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/shCList.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/shChain.h
atCentroid.o:	$(DERVISH_DIR)/include/shCHist.h
atCentroid.o:	$(DERVISH_DIR)/include/shCLink.h
atCentroid.o:	$(DERVISH_DIR)/include/shCList.h
atCentroid.o:	$(DERVISH_DIR)/include/shCMask.h
atCentroid.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atCentroid.o:	$(DERVISH_DIR)/include/shCFitsIo.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/shCMemory.h
atCentroid.o:	$(DERVISH_DIR)/include/shCRegPhysical.h
atCentroid.o:	$(DERVISH_DIR)/include/region.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclAlign.h
atCentroid.o:	$(FTCL_DIR)/include/ftcl.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(DERVISH_DIR)/include/shCSchema.h
atCentroid.o:	$(DERVISH_DIR)/include/shCAlign.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclArray.h
atCentroid.o:	$(FTCL_DIR)/include/ftcl.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(DERVISH_DIR)/include/shCArray.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclFits.h
atCentroid.o:	$(FTCL_DIR)/include/ftcl.h
atCentroid.o:	$(TCL_DIR)/include/tcl.h
atCentroid.o:	$(DERVISH_DIR)/include/shCFitsIo.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclHdr.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclSao.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclSupport.h
atCentroid.o:	$(DERVISH_DIR)/include/shTclTree.h
atCentroid.o:	../../include/atConversions.h
atCentroid.o:	../include/atGaussianWidth.h
atCentroid.o:	../include/atCentroid.h
atGaussianWidth.o:	atGaussianWidth.c
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish.h
atGaussianWidth.o:	$(TCLX_DIR)/include/tclExtend.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftcl.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftclParseArgv.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCAlign.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCHdr.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shC.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shChain.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCArray.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCUtils.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCErrStack.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclErrStack.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftcl.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCArray.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchemaCnv.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclHandle.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftcl.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCList.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shChain.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCTbl.h
atGaussianWidth.o:	$(LIBFITS_DIR)/src/libfits.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCAlign.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shChain.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCArray.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCFitsIo.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCHdr.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCUtils.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclHandle.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCTbl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchemaSupport.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclHandle.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCTbl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchemaSupport.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCHdr.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCFitsIo.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCFits.h
atGaussianWidth.o:	$(LIBFITS_DIR)/src/libfits.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclHandle.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCFitsIo.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCTbl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCTbl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCMaskUtils.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCRegUtils.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCRegPrint.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCHash.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCAssert.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCUtils.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCEnvscan.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftcl.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSao.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/imtool.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclParseArgv.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftcl.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclVerbs.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclSao.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftclCmdLine.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/imtool.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSao.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclUtils.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftcl.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTk.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclHandle.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCGarbage.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCUtils.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCHg.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCErrStack.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCRegUtils.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCAssert.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclVerbs.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclHandle.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCLink.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCList.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCVecExpr.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCLut.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCVecExpr.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCVecChain.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shChain.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCVecExpr.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCHg.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/imtool.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCAlign.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCDemo.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCDiskio.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCList.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shChain.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCHist.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCLink.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCList.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCMask.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCFitsIo.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCMemory.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCRegPhysical.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/region.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclAlign.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftcl.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCSchema.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCAlign.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclArray.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftcl.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCArray.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclFits.h
atGaussianWidth.o:	$(FTCL_DIR)/include/ftcl.h
atGaussianWidth.o:	$(TCL_DIR)/include/tcl.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shCFitsIo.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclHdr.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclSao.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclSupport.h
atGaussianWidth.o:	$(DERVISH_DIR)/include/shTclTree.h
atGaussianWidth.o:	../include/atGaussianWidth.h
