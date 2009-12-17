SHELL = /bin/sh
#
#
#  Makefile to rebuild all of dervish.
#
# Environment:
# Environmental variables
#	$LIBFITS_DIR The Libfits package, libfits having been setup by ups
#	$FTCL_DIR    The FTCL Package, ftcl having ben setup by ups
#	$PGPLOT_DIR  The PGPLOT package pgplot having been setup by ups
# Command-line macro selecting switches to support the three compilation
# environments:
#	SGI C Compiler under IRIX
#	SUN ACC Compilers under SunOS
#	GNU C Compiler under SunOS
#
# Expected variables on the command line:
# CC=        The name of the appropriate compiler, with suitable flags
# CCCHK=     A set of compiler flags which check ANSI C strictly
# LLIBS=     A set of default libraries set by premake

.c.o :
	$(CC) -c $(CFLAGS) $*.c

BIN	= ../bin
TEST	= ../test

INCDIRS	= -I$(DERVISH_DIR)/include -I$(DERVISH_DIR)/src -I$(DERVISH_DIR)/test \
	  -I$(DERVISH_DIR)/contrib -I$(LIBFITS_DIR)/src

INCTCLDIRS = \
	-I$(FTCL_DIR)/include -I$(FTCL_DIR)/src \
	-I$(TCLX_DIR)/include -I$(TCLX_DIR)/src/src -I$(TCLX_DIR)/src/tksrc \
	-I$(TK_DIR)/include -I$(TK_DIR)/src \
	-I$(TCL_DIR)/include -I$(TCL_DIR)/src

LDFLAGS = $(LIBSHV) $(LIBFITS) $(PGPLOTLIB) \
	  $(CONTRIBLIB) $(LIBFTCL) \
	  $(LLIBS) $(PGLIB) $(FLIBS)

#LDTCLFLAGS = -L$(DERVISH_DIR)/lib -ldervish $(LIBFITS) $(PGPLOTLIB) \
#             -L$(TK_DIR)/lib -ltkx -ltk \
#             -L$(FTCL_DIR)/lib -lftcl -ltclx -ltcl $(LLIBS)
#
CC_VW		= cc68k
INCDIRS_VW	= -I$(VXWORKS_DIR)/h -I$(INC) \
	          -I$(FTCL_DIR)/src -I$(FTCL_DIR)/fermitcl/include
CFLAGS_VW	= -c -g -DVXWORKS -DEDITORTEST $(INCDIRS_VW)

CFLAGS  = $(SDSS_CFLAGS) $(INCDIRS) $(CCCHK) $(INCTCLDIRS) -DCHECK_LEAKS

#
#
#
PROGS = tst_dervish_h tst_memory tst_region tst_chain
#Please when adding to SCRIPTS, leave the last line last.
SCRIPTS = \
	tst_perf.tcl \
	tst_vector.tcl tst_expr.tcl \
	tst_sch.tcl tst_TBLCOL.tcl tst_tbl.tcl tst_bigtbl.tcl \
	tst_subRegGet.tcl tst_util.tcl \
	tst_regReadAsFits.tcl tst_chain.tcl tst_hdr.tcl\
        tst_maskIo.tcl tst_dumpIO.tcl\
	tst_params.tcl tst_SEval.tcl \
	tst_fsao.tcl tst_saoDraw.tcl \
	tst_hg.tcl tst_pgplot.tcl
SHSCRIPTS = tst_env.sh

#=============================================================================
# TARGETS
#=============================================================================
all: progs shscripts scripts

progs: $(PROGS)
	@ rm -f .failed~; \
	for f in $(PROGS); do \
		echo Running $$f; \
		if $(TEST)/$$f 2>&1; then \
			echo Passed all  tests >&2; \
		else \
			echo Failed some tests: >&2;\
			echo "  $$f" >> .failed~; touch .failed~; \
		fi | sed -n -e 's/^TEST-ERR: */   /p' ; \
	done; \
	if [ -f .failed~ ]; then \
		echo; echo "Failed to pass tests" >&2; \
		cat .failed~; exit 1; \
	fi
	@ rm -f .failed~

shscripts : $(SHSCRIPTS)
	@ for f in $(SHSCRIPTS); do \
		echo Running $$f; \
		if `dirname $$f`/`basename $$f` 2>&1; then \
			echo Passed all tests >&2; \
		else \
			echo Failed some tests: >&2; \
			echo " $$f" >> .failed~; \
		fi | sed -n -e 's/^TEST-ERR: */   /p' ; \
	done; \
	if [ -f .failed~ ]; then \
		echo; echo "Failed to pass the sh script tests" >&2; \
		cat .failed~ >&2 ; \
		rm -f .failed~; \
		exit 1; \
	fi

#
# Tcl Test scripts
#
scripts : $(SCRIPTS)
	@rm -f .failed~; \
	unset DERVISH_USER; \
	for f in $(SCRIPTS); do \
		echo Running ../bin/dervish -file $(TEST)/$$f; \
		if $(BIN)/dervish -file $(TEST)/$$f 2>&1; then \
			echo Passed all tests >&2; \
		else \
			echo Failed some tests: >&2; echo "  $$f" >> .failed~; \
		fi | sed -n -e 's/^TEST-ERR: */   /p' ; \
	done; \
	if [ -f .failed~ ]; then \
		echo; echo "Failed to pass the following tests" >&2; \
		cat .failed~ >&2 ; \
		rm -f .failed~; \
		exit 1; \
	fi

#
# Test codes
#
tst_hdr.tcl : Getasdbl_values Getasint_values Getaslogical_values hdrNew_values header_values

#tst_dervish_h : TST_DERVISH_H
#TST_DERVISH_H : tst_dervish_h.c
tst_dervish_h : tst_dervish_h.c
	$(CC) -o tst_dervish_h $(CFLAGS) tst_dervish_h.c

tst_region : tst_region.o tst_utils.o ../lib/libdervish.a
	$(CC) -o tst_region tst_region.o tst_utils.o $(LDFLAGS)
#
tst_memory : tst_memory.o tst_utils.o ../lib/libdervish.a
	$(CC) -o tst_memory tst_memory.o tst_utils.o $(LDFLAGS)
#
tst_dump : tst_dump.o ../lib/libdervish.a
	$(CC) -o tst_dump tst_dump.o $(LDFLAGS)
tst_chain : tst_chain.o ../lib/libdervish.a
	@echo PGPLOT: $(PGPLOTLIB)
	$(CC) -o tst_chain tst_chain.o $(LDFLAGS)
tst_tcl  : tst_tcl.tcl
#
#
#
#
# Now standard targets
#
install :
	@if [ "$(DERVISH_INSTALL_DIR)" = "" ]; then \
	        echo You have not specified a destination directory >&2; \
	        exit 1; \
	fi
	@rm -rf           $(DERVISH_INSTALL_DIR)/test
	@mkdir            $(DERVISH_INSTALL_DIR)/test
	@cp -p tst_*      $(DERVISH_INSTALL_DIR)/test
	@cp -p Get*       $(DERVISH_INSTALL_DIR)/test
	@cp -p small.fits $(DERVISH_INSTALL_DIR)/test
	@cp -p ocNew.par  $(DERVISH_INSTALL_DIR)/test
	@cp -p h*value*   $(DERVISH_INSTALL_DIR)/test
	@cp -p Makefile   $(DERVISH_INSTALL_DIR)/test
	@cp -p test.par   $(DERVISH_INSTALL_DIR)/test

clean:
	@$(SDSSTOOLS_DIR)/bin/clean
	rm -f *.o $(PROGS) tst_dump
#
make :
	@if [ "$(CCENV)" = "" ]; then ccenv=`uname`; else ccenv=$(CCENV); fi; \
	echo \
	"$(SDSSTOOLS_DIR)/bin/make_make -cc '$(CC) $(CFLAGS)' -nostd -file Makefile" \
							"-env $$ccenv *.c"; \
	 $(SDSSTOOLS_DIR)/bin/make_make -cc '$(CC) $(CFLAGS)' -nostd -file Makefile \
							 -env $$ccenv *.c
#
# include file dependencies.
# All lines below START_DEPEND are machine generated; Do Not Edit
#
#START_DEPEND
foo.o:	foo.c
foo.o:	$(DERVISH_DIR)/include/shCAssert.h
foo.o:	$(DERVISH_DIR)/include/shCUtils.h
foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
foo.o:	$(DERVISH_DIR)/include/shCSchema.h
foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
foo.o:	$(DERVISH_DIR)/include/shCUtils.h
foo.o:	$(DERVISH_DIR)/include/shCSchema.h
foo.o:	$(DERVISH_DIR)/include/shCList.h
foo.o:	$(DERVISH_DIR)/include/shCSchema.h
foo.o:	./foo.h
foo.o:	$(DERVISH_DIR)/include/shCList.h
tst_chain.o:	tst_chain.c
tst_chain.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_chain.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_chain.o:	$(DERVISH_DIR)/include/shChain.h
tst_chain.o:	$(DERVISH_DIR)/include/shCArray.h
tst_chain.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_chain.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_chain.o:	$(DERVISH_DIR)/include/region.h
tst_chain.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_chain.o:	$(DERVISH_DIR)/include/shCAlign.h
tst_chain.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_chain.o:	$(DERVISH_DIR)/include/shCHdr.h
tst_chain.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_chain.o:	$(DERVISH_DIR)/include/shCHg.h
tst_chain.o:	$(DERVISH_DIR)/include/shCErrStack.h
tst_chain.o:	$(DERVISH_DIR)/include/shCRegUtils.h
tst_chain.o:	$(DERVISH_DIR)/include/region.h
tst_chain.o:	$(DERVISH_DIR)/include/shCAssert.h
tst_chain.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_chain.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_chain.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_chain.o:	$(DERVISH_DIR)/include/shTclVerbs.h
tst_chain.o:	$(DERVISH_DIR)/include/shTclSao.h
tst_chain.o:	$(TCL_DIR)/include/tcl.h
tst_chain.o:	$(FTCL_DIR)/include/ftclCmdLine.h
tst_chain.o:	$(DERVISH_DIR)/include/imtool.h
tst_chain.o:	$(DERVISH_DIR)/include/region.h
tst_chain.o:	$(DERVISH_DIR)/include/shCSao.h
tst_chain.o:	$(DERVISH_DIR)/include/imtool.h
tst_chain.o:	$(DERVISH_DIR)/include/region.h
tst_chain.o:	$(DERVISH_DIR)/include/shTclHandle.h
tst_chain.o:	$(FTCL_DIR)/include/ftcl.h
tst_chain.o:	$(TCL_DIR)/include/tcl.h
tst_chain.o:	$(FTCL_DIR)/include/ftclParseArgv.h
tst_chain.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_chain.o:	$(DERVISH_DIR)/include/shCList.h
tst_chain.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_chain.o:	$(DERVISH_DIR)/include/shChain.h
tst_chain.o:	$(DERVISH_DIR)/include/shCLink.h
tst_chain.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_chain.o:	$(DERVISH_DIR)/include/shCList.h
tst_chain.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_chain.o:	$(DERVISH_DIR)/include/shCVecExpr.h
tst_dump.o:	tst_dump.c
tst_dump.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_dump.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dump.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dump.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dump.o:	$(DERVISH_DIR)/include/region.h
tst_dump.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dump.o:	$(DERVISH_DIR)/include/shCAlign.h
tst_dump.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dump.o:	$(DERVISH_DIR)/include/shCHdr.h
tst_dump.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dump.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dump.o:	$(DERVISH_DIR)/include/shCDiskio.h
tst_dump.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dump.o:	$(DERVISH_DIR)/include/shCList.h
tst_dump.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dump.o:	$(DERVISH_DIR)/include/region.h
tst_dump.o:	$(DERVISH_DIR)/include/shChain.h
tst_dump.o:	$(DERVISH_DIR)/include/shCArray.h
tst_dump.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dump.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dump.o:	./foo.h
tst_dump.o:	$(DERVISH_DIR)/include/shCList.h
tst_memory.o:	tst_memory.c
tst_memory.o:	$(DERVISH_DIR)/include/shCGarbage.h
tst_memory.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_memory.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_memory.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_memory.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_memory.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_memory.o:	./tst_utils.h
tst_memory.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_memory.o:	$(DERVISH_DIR)/include/prvt/utils_p.h
tst_region.o:	tst_region.c
tst_region.o:	./tst_utils.h
tst_region.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_region.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_region.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_region.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_region.o:	$(DERVISH_DIR)/include/region.h
tst_region.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_region.o:	$(DERVISH_DIR)/include/shCAlign.h
tst_region.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_region.o:	$(DERVISH_DIR)/include/shCHdr.h
tst_region.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_region.o:	$(DERVISH_DIR)/include/prvt/region_p.h
tst_dervish_h.o:	tst_dervish_h.c
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish.h
tst_dervish_h.o:	$(TCLX_DIR)/include/tclExtend.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftcl.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftclParseArgv.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCAlign.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCHdr.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shC.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shChain.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCArray.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCErrStack.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclErrStack.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftcl.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCArray.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchemaCnv.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclHandle.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftcl.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCList.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shChain.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCTbl.h
tst_dervish_h.o:	$(LIBFITS_DIR)/src/libfits.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCAlign.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shChain.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCArray.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCFitsIo.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCHdr.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclHandle.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCTbl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchemaSupport.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclHandle.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCTbl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchemaSupport.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCHdr.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCFitsIo.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCFits.h
tst_dervish_h.o:	$(LIBFITS_DIR)/src/libfits.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclHandle.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCFitsIo.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCTbl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCTbl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCMaskUtils.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCRegUtils.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCRegPrint.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCHash.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCAssert.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCEnvscan.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftcl.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSao.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/imtool.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclParseArgv.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftcl.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclVerbs.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclSao.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftclCmdLine.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/imtool.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSao.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclUtils.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftcl.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTk.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclHandle.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCGarbage.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCHg.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCErrStack.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCRegUtils.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCAssert.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclVerbs.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclHandle.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCLink.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCList.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCVecExpr.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCLut.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCVecExpr.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCVecChain.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shChain.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCVecExpr.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCHg.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/imtool.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCAlign.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCDemo.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCDiskio.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCList.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shChain.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCHist.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCLink.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCList.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCMask.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCFitsIo.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCMemory.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCRegPhysical.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/region.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclAlign.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftcl.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCAlign.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclArray.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftcl.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCArray.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclFits.h
tst_dervish_h.o:	$(FTCL_DIR)/include/ftcl.h
tst_dervish_h.o:	$(TCL_DIR)/include/tcl.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shCFitsIo.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclHdr.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclSao.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclSupport.h
tst_dervish_h.o:	$(DERVISH_DIR)/include/shTclTree.h
tst_utils.o:	tst_utils.c
tst_utils.o:	./tst_utils.h
tst_utils.o:	$(DERVISH_DIR)/include/shCUtils.h
tst_utils.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_utils.o:	$(DERVISH_DIR)/include/shCSchema.h
tst_utils.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tst_utils.o:	$(DERVISH_DIR)/include/shCUtils.h
