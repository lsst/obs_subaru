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
INSTALL_FILES = *.c *.tcl Makefile *.rc *locusfile*
CLEAN_FILES   = pgplot.ps log.581.11.10
SCRIPTS = testAll.tcl
PROGS	=

#===========================================================================
# STANDARD TARGETS
#===========================================================================

default:
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1

all: scripts scd

progs: $(PROGS)
	@ rm -f .failed~; \
	for f in $(PROGS); do \
		echo Running $$f; \
		if $(TEST)/$$f 2>&1; then \
			echo Passed all progs tests >&2; \
		else \
			echo Failed some progs tests: >&2;\
			echo "  $$f" >> .failed~; \
			touch .failed~; \
		fi; \
	done; \
	if [ -f .failed~ ]; then \
		echo; echo "Failed to pass progs tests" >&2; \
		cat .failed~; exit 1; \
	fi
	@ rm -f .failed~

scripts: $(SCRIPTS)
	@rm -f .failed~; \
	for f in $(SCRIPTS); do \
		echo Running ../bin/astrotools -file $$f; \
		if ../bin/astrotools -file $$f 2>&1; then \
			echo Passed all scripts tests >&2; \
		else \
			echo Failed some scripts tests: >&2; \
			touch .failed~; \
		fi; \
	done; \
	if [ -f .failed~ ]; then \
		echo; echo "Failed to pass scripts tests" >&2; exit 1; \
	fi
	@ rm -f .failed~

scd: scdtestfiles scdtest scdclean

scdtestfiles:
	./scd_make_files.rc
scdtest:
	./scd_test.rc
	./scd_types.rc
scdclean:
	rm -f test.fits* uncompressable.fitsi* nullfile.fitsi* wrap*16*.fits*

install :
	@if [ "$(INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi     
	@ rm -rf $(INSTALL_DIR)/test
	@ mkdir $(INSTALL_DIR)/test
	@if [ "$(INSTALL_FILES)" != "" ]; then \
		cp -p $(INSTALL_FILES) $(INSTALL_DIR)/test; \
		chmod 644 $(INSTALL_DIR)/test/*; \
	fi

clean : scdclean
	rm -f $(CLEAN_FILES) *~ core *.bak *.orig *.old .#* #*#
