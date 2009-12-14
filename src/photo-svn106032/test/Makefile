SHELL = /bin/sh
#
default :
	echo "Please invoke this makefile using sdssmake"

BIN = ../bin
TEST = ../test

CFLAGS  = $(SDSS_CFLAGS) -I../include -DCHECK_LEAKS
LDFLAGS = ../lib/libphoto.a $(DERVISH_DIR)/lib/libdervish.a -lm

PROGS = tst_mathUtils tst_LM_minimise
SCRIPTS = tst_sky.tcl tst_utils.tcl tst_correct_frames.tcl \
	  tst_library.tcl tst_finder.tcl \
	  tst_random.tcl tst_mo.tcl tst_cr.tcl tst_extractLinear.tcl \
	  tst_fitsTbl.tcl tst_findPsf.tcl tst_mergeColors.tcl \
          tst_findPhotoParams.tcl tst_cellfit.tcl \
	  tst_peaks.tcl

#
# Note that if you set the environment variable SED_FLAGS it will be used
# in place of the "-n" in the sed command filtering output; in particular
# if it's set to "-p" no filtering will take place. 
#
all: progs scripts

progs: $(PROGS)
	@ rm -f .failed~; \
	if [ "$(PROGS)" != "" ]; then \
		if [ X$${SED_FLAGS} = X ]; then SED_FLAGS=-n; fi; \
		if [ X$${SED_FLAGS} = X-p ]; then SED_FLAGS=; fi; \
		export SED_FLAGS; \
		for f in ""$(PROGS); do \
			echo Running $$f; \
			if $(TEST)/$$f 2>&1; then \
				echo Passed all tests >&2; \
			else \
				echo Failed some tests: >&2; touch .failed~; \
			fi | sed $${SED_FLAGS} -e 's/^TEST-ERR: */   /p' ; \
		done; \
	fi; \
	if [ -f .failed~ ]; then \
		echo; echo "Failed to pass tests" >&2; exit 1; \
	fi
	@ rm -f .failed~
#
# Tcl Test scripts
#
scripts : $(SCRIPTS)
	@ rm -f .failed~; \
	if [ "$(SCRIPTS)" != "" ]; then \
		unset PHOTO_USER; export PHOTO_USER; \
		unset DERVISH_USER; export DERVISH_USER; \
		if [ X$${SED_FLAGS} = X ]; then SED_FLAGS=-n; fi; \
		if [ X$${SED_FLAGS} = X-p ]; then SED_FLAGS=; fi; \
		export SED_FLAGS; \
		for f in $(SCRIPTS); do \
			echo Running $(BIN)/photo -file $(TEST)/$$f; \
			if $(BIN)/photo -file $(TEST)/$$f 2>&1; then \
				echo Passed all tests >&2; \
			else \
				echo Failed some tests: >&2; \
				echo "  $$f" >> .failed~; \
			fi | sed $${SED_FLAGS} -e 's/^TEST-ERR: */   /p' ; \
		done; \
	fi; \
	if [ -f .failed~ ]; then \
		echo; echo "Failed to pass tests:" >&2; cat .failed~ >&2; \
		rm -f .failed~; \
		exit 1; \
	fi
#
# Build test programmes
#
tst_mathUtils : tst_mathUtils.c ../lib/libphoto.a
	$(CC) -o tst_mathUtils $(CCCHK) $(CFLAGS) tst_mathUtils.c $(LDFLAGS)
tst_LM_minimise : tst_LM_minimise.c  ../lib/libphoto.a
	$(CC) -o tst_LM_minimise $(CCCHK) $(CFLAGS) tst_LM_minimise.c \
				$(LDFLAGS)

install :
	@if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi 
	@rm -rf $(PHOTO_DIR)/test
	@mkdir  $(PHOTO_DIR)/test
	cp Makefile tst_params tst_procs.tcl *.par test_cr.fit \
				$(PROGS) $(SCRIPTS) $(PHOTO_DIR)/test

clean :
	rm -f *~ core *.bak *.orig *.old .#* #*# $(PROGS)
	rm -rf tmp
