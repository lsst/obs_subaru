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
INSTALL_FILES = 
CLEAN_FILES   =	*.dvi *.ps *.log *.toc *.aux

#=============================================================================
#
# You will have to edit the rest of this file, if you want DVI and postscript
# book versions of your online help.
#
#=============================================================================

WWW  = ./www/
BOOK = astrotools

#===========================================================================
# STANDARD TARGETS
#===========================================================================

default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

#all : $(BOOK).ps
all :
	@ for f in www; do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) all); \
	done

install :
	@if [ "$(INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi     
	@ rm -rf $(INSTALL_DIR)/doc
	@ mkdir $(INSTALL_DIR)/doc
	@if [ "$(INSTALL_FILES)" != "" ]; then \
		cp -p $(INSTALL_FILES) $(INSTALL_DIR)/doc; \
		chmod 644 $(INSTALL_DIR)/doc/*; \
	fi
	/bin/cp -p Makefile $(INSTALL_DIR)/doc
	/bin/cp -p RELEASENOTES $(INSTALL_DIR)/doc
	/bin/cp -p howToCutAstrotools.txt $(INSTALL_DIR)/doc
	@ for f in www; do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) install); \
	done

clean :
	rm -f $(CLEAN_FILES) *~ core *.bak *.orig *.old .#* #*#
	@ for f in www; do \
		(cd $$f ; echo In $$f; $(MAKE) $(MFLAGS) clean); \
	done

#===========================================================================
# TARGETS
#===========================================================================

#
# Generate TeX
#
#
#       The Book
#

#$(BOOK).dvi : $(BOOK).tex
#	-latex $(BOOK).tex
#	-latex $(BOOK).tex

#$(BOOK).ps : $(BOOK).dvi
#	-dvips $(BOOK) -o $(BOOK).ps

# Remove likely artifacts of building, but not the desirata 
tidy:
	rm -f *.log *.toc *.aux
	rm -f *~ core *.bak *.orig *.old .#* #*#
