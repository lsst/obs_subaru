SHELL = /bin/sh

#=============================================================================
#
#  INSTALL_DIR   --- Root directory into which the product is to be installed.
#  LIBS          --- Names of all object code libraries to be installed.
#  INSTALL_FILES --- Names of the files to be installed.  This must include the
#                    names of the libraries (just include $(LIBRARIES)).
#  CLEAN_FILES   --- Names of specific files to be deleted when the directory
#                    is cleaned.  Files matching one of a default set of file
#                    name templates will be deleted automatically
#                    (i.e., *~ core *.bak *.orig *.old .#* #*# *.o).  This
#                    variable is for declaring additional files to delete which
#                    don't match any of the file name templates.
#
#=============================================================================

INSTALL_DIR   = $(ASTROTOOLS_INSTALL_DIR)
LIBS          = libastrotools.a
INSTALL_FILES = $(LIBS) Makefile 
CLEAN_FILES   = $(LIBS)

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
	@ rm -rf $(INSTALL_DIR)/lib
	@ mkdir $(INSTALL_DIR)/lib
	@if [ "$(INSTALL_FILES)" != "" ]; then \
		cp -p $(INSTALL_FILES) $(INSTALL_DIR)/lib; \
	fi
	-@if [ "$(RANLIB)" != "" -a -x "$(RANLIB)" ]; then \
		for f in $(LIBS); do \
			$(RANLIB) $(INSTALL_DIR)/lib/$$f; \
		done; \
	fi
	@if [ "$(INSTALL_FILES)" != "" ]; then \
		chmod 644 $(INSTALL_DIR)/lib/*.a; \
	fi

clean :
	rm -f $(CLEAN_FILES) *.a *~ core *.bak *.orig *.old .#* #*#
