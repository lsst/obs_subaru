# Makefile for DA directory in ASTROTOOLS
# !!!!! DO NOT USE THIS MAKEFILE IF COMPLILING DA ASTROLINE!!!!!!!!!!!!!!!!!

SHELL = /bin/sh

#=============================================================================
# Makefile for DA directory in ASTROTOOLS
#=============================================================================

INSTALL_DIR   = $(ASTROTOOLS_INSTALL_DIR)
SUBDIRECTORIES= 

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
	@ rm -rf $(INSTALL_DIR)/DA/include
	@ mkdir $(INSTALL_DIR)/DA/include
	/bin/cp -p Makefile *.h $(INSTALL_DIR)/DA/include

clean :
	@ echo in .
	rm -f *~ core *.bak *.orig *.old .#* #*#
