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
#
#  For example, for the product "myProduct":
#     INSTALL_DIR   = $(MYPRODUCT_DIR)
#     INSTALL_FILES = *.html
#     CLEAN_FILES   = 
#
#=============================================================================

INSTALL_DIR   = $(ASTROTOOLS_INSTALL_DIR)
INSTALL_FILES = *.html Makefile
CLEAN_FILES   = atHgMath.html atLinearFits.html atMatch.html atSlalib.html \
	atSurveyGeometry.html atAirmass.html atTrans.html tclHgMath.html \
	tclLinearFits.html tclMatch.html tclSlalib.html tclTrans.html \
	tclSurveyGeometry.html tclAirmass.html atCali.html tclCali.html \
	atEphemeris.html tclEphemeris.html atRegSci.html tclRegSci.html \
	atGalaxy.html tclGalaxy.html atGauss.html atImSim.html tclImSim.html \
	atExternalSet.html atPMatch.html tclPMatch.html atSlalib.h.html \
	atCentroid.html atObject.html tclObject.html atGaussianWidth.html \
	atQuasar.html atAperture.html tclAperture.html atDustGetval.html \
        tclDustGetval.html astrotools.C.list.html astrotools.tcl.list.html

BUILD_FILES   = atHgMath atLinearFits atMatch atSlalib atSurveyGeometry \
	atAirmass atTrans atQuasar tclHgMath tclLinearFits tclMatch tclSlalib \
	tclTrans tclSurveyGeometry tclAirmass atCali tclCali atEphemeris \
	tclEphemeris atRegSci tclRegSci atGalaxy tclGalaxy atGauss \
	atImSim tclImSim atExternalSet atPMatch tclPMatch  \
	atObject tclObject atAperture tclAperture tclDustGetval atDustGetval

DA_BUILD_FILES	= atCentroid atGaussianWidth

BUILD_FILES_H = atSlalib.h

#=============================================================================
#
# You should not have to edit any of the rest of this file.
#
#=============================================================================

#===========================================================================
# STANDARD TARGETS
#===========================================================================

default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

all :
	@ for f in $(BUILD_FILES); do \
		(c2html ../../src/$$f.c > $$f.html); \
	done
	@ for f in $(DA_BUILD_FILES); do \
		(c2html ../../DA/src/$$f.c > $$f.html); \
	done
	@ for f in $(BUILD_FILES_H); do \
		(c2html ../../include/$$f > $$f.html); \
	done
	@ ../../etc/astropage.pl; 

install :
	@if [ "$(INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi     
	@ rm -rf $(INSTALL_DIR)/doc/www
	@ mkdir $(INSTALL_DIR)/doc/www
	@if [ "$(INSTALL_FILES)" != "" ]; then \
		cp -p $(INSTALL_FILES) $(INSTALL_DIR)/doc/www; \
		chmod 644 $(INSTALL_DIR)/doc/www/*; \
	fi

clean :
	rm -f $(CLEAN_FILES) *~ core *.bak *.orig *.old .#* #*#
