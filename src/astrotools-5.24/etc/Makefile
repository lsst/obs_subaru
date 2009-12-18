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
#=============================================================================

INSTALL_DIR   = $(ASTROTOOLS_INSTALL_DIR)
INSTALL_FILES = *.tcl *.par
CLEAN_FILES   =

#===========================================================================
# STANDARD TARGETS
#===========================================================================

default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

all :;

install :
	@if [ "$(INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi     
	@ rm -rf $(INSTALL_DIR)/etc
	@ mkdir $(INSTALL_DIR)/etc
	@if [ "$(INSTALL_FILES)" != "" ]; then \
		cp -p $(INSTALL_FILES) $(INSTALL_DIR)/etc; \
		chmod 644 $(INSTALL_DIR)/etc/*; \
	fi
	/bin/cp -p Makefile $(INSTALL_DIR)/etc

clean :
	rm -f $(CLEAN_FILES) *~ core *.bak *.orig *.old .#* #*#
