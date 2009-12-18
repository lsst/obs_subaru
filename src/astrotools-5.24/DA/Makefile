# Makefile for DA directory in ASTROTOOLS
# !!!!! DO NOT USE THIS MAKEFILE IF COMPLILING DA ASTROLINE!!!!!!!!!!!!!!!!!

SHELL = /bin/sh

#=============================================================================
# Makefile for DA directory in ASTROTOOLS
#=============================================================================

INSTALL_DIR   = $(ASTROTOOLS_INSTALL_DIR)
SUBDIRECTORIES= include src

#===========================================================================
# STANDARD TARGETS
#===========================================================================

default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

all :
	@echo "Updating libraries and directories"
	@echo ""
	@ for f in $(SUBDIRECTORIES); do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) all ); \
	done

install :
	@if [ "$(INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi 
	@ rm -rf $(INSTALL_DIR)/DA
	@ mkdir $(INSTALL_DIR)/DA
	@ for f in $(SUBDIRECTORIES); do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) install ); \
	done
	/bin/cp -p Makefile $(INSTALL_DIR)/DA

clean :
	@ echo in .
	rm -f *~ core *.bak *.orig *.old .#* #*#
	@ for f in $(SUBDIRECTORIES); do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) clean); \
	done
