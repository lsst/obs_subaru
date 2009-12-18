SHELL = /bin/sh

#=============================================================================
#
#  At a minimum, to use this file, fill in the definition for the
#  variables below:
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
#     INSTALL_FILES = *setup*
#     CLEAN_FILES   =
#
#=============================================================================

INSTALL_DIR   = $(ASTROTOOLS_INSTALL_DIR)
INSTALL_FILES = 
CLEAN_FILES   =

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

install :
	@if [ "$(INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi     
	@ rm -rf $(INSTALL_DIR)/ups
	@ mkdir $(INSTALL_DIR)/ups
	@ cp -p Makefile $(INSTALL_DIR)/ups
	$(SDSSTOOLS_DIR)/bin/install_ups_table astrotools.table $(INSTALL_DIR)/ups

clean :
	rm -f $(CLEAN_FILES) *~ core *.bak *.orig *.old .#* #*#
