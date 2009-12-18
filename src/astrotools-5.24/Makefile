SHELL = /bin/sh

#=============================================================================
#
#  This is the top level makefile for ASTROTOOLS
#
#  This file is designed to be invoked by a shell scyript called sdssmake
#  which selects supported options.
#
#  INSTALL_DIR    --- Root directory into which the product is to be installed.
#  SUBDIRECTORIES --- List of subdirectories contained in the root directory
#                     for this product (i.e., this directory).
#
#=============================================================================

INSTALL_DIR   = $(ASTROTOOLS_INSTALL_DIR)
SUBDIRECTORIES= bin etc include DA lib src doc test ups

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
	@echo "You should be sure to have updated before doing this."
	@echo "You will be installing in $(INSTALL_DIR)"
	@echo ""
	@echo "I'll give you 5 seconds to think about it"
	@ for sec in 5 4 3 2 1; \
	  do \
	    echo "     " | sed -e 's/ /'$$sec'/'$$sec; \
	    sleep 1; \
	  done
	@if [ "$(INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi 
	@ rm -rf $(INSTALL_DIR)
	@ mkdir $(INSTALL_DIR)
	@ for f in $(SUBDIRECTORIES); do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) install ); \
	done
	@/bin/cp -p Makefile $(INSTALL_DIR)

protect :
	@echo "Fixing directory and file permissions on $(INSTALL_DIR)..."
	@(cd $(INSTALL_DIR); \
		find . -exec chmod u+w {} \; ; \
		find . -exec chmod g-w {} \; ; \
		find . -exec chmod +r {} \; ; \
		find $(SUBDIRECTORIES) -type d -exec chmod 755 {} \; ; \
		find $(SUBDIRECTORIES) -type d -exec chgrp products {} \; ; \
		find . -exec chown products {} \;)

clean :
	@ echo in .
	rm -f *~ core *.bak *.orig *.old .#* #*#
	@ for f in $(SUBDIRECTORIES); do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) clean); \
	done
