SHELL = /bin/sh
#  Makefile to rebuild dervish with no_tcl.
# 
#  1) This file is designed to be invoked by a shell script called premake
#  which selects supported options.
#
#  2) This file re-builds the DERVISH product. Those expecting to extend DERVISH 
#  by adding application-specific TCL extensions should re-build using the 
#  premake and make files in the "../examples" sub-directory.
#
# 3) Environment:
# Environmental variables
#	$LIBFITS_DIR The Libfits package, libfits having been setup by ups
#	$FTCL_DIR    The FTCL Package, ftcl having been setup by ups
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
# LLIBS=     A set of libraries for linking against.
#
.c.o :
	$(CC) -c $(CCCHK) $(CFLAGS) $*.c

#
SRC	= ../../src
INC	= ../../include
CONTRIB = ../../contrib
LIB	= ../../lib
BIN	= ../../bin
EXS	= ../../examples
ETC	= ../../etc
DOC	= ../../doc
UPS	= ../../ups
INCDIRS	= -I$(INC) -I$(CONTRIB) \
	   -I$(FTCL_DIR)/include -I$(FTCL_DIR)/src \
	   -I$(LIBFITS_DIR)/src -I$(FPGPLOT_DIR)/include -I$(PGPLOT_DIR)/dst \
	   -I$(TK_DIR)/include -I$(TK_DIR)/src \
	   -I$(TCLX_DIR)/src/src  -I$(TCLX_DIR)/include \
	   -I$(TCL_DIR)/include -I$(TCL_DIR)/src

SDSSTOOLS = $(SDSSTOOLS_DIR)/bin

# NOTE:	CFLAGS_DEFGP is provided because some IRIX systems do not handle -G 0
#	well when loading (ld) images.  The 'defgp' stands for default global
#	pointer processing (that is, no -G n option).

CFLAGS		= $(SDSS_CFLAGS) $(INCDIRS) -DCHECK_LEAKS -DNOTCL
CFLAGS_DEFGP	= $(SDSS_CFLAGS_DEFGP) $(INCDIRS)
CFLAGScia	= $(INCDIRS) -DCHECK_LEAKS

#
#
OBJECTS_NOTCL = regUtils.o hdr.o region.o errStack.o \
	tree.o utils.o diskio.o diskio_gen_notcl.o schema.o \
	envscan.o fitsIo.o shSchema.o \
	shAlign.o shArray.o shFits.o shTbl.o\
	lists.o mask.o shChain.o\
	shGarbage.o shHash.o maskUtils.o\
	handleExpr.o vecExpr.o vecChain.o vecOps.o

SRCS_NOTCL = regUtils.c hdr.c region.c errStack.c \
	tree.c utils.c diskio.c schema.c \
	envscan.c fitsIo.c shSchema.c \
	shAlign.c shArray.c shFits.c shTbl.c\
	lists.c mask.c shChain.c\
	shGarbage.c shHash.c maskUtils.c\
	handleExpr.c vecExpr.c vecChain.c vecOps.c


LDFLAGS = $(LIBSHV) $(LIBFITS) $(PGPLOTLIB) \
	  $(CONTRIBLIB) $(LIBFTCL) \
	  $(LLIBS) $(PGLIB) $(FLIBS)

#=============================================================================
# TARGETS
#=============================================================================

default:	
	@echo Please invoke this makefile using sdssmake. >&2
	@exit 1


all: mklinks lib_notcl

lib_notcl : $(LIB)/libdervishnotcl.a

mklinks : 
	@echo "=== DERVISH NOTCL BUILD : Making links for no_tcl library ==="
	@for SRCS in $(SRCS_NOTCL); do \
		if [ ! -f $$SRCS ]; then \
			ln -s ../$$SRCS $$SRCS; \
		fi; \
	done
	@echo "=== DERVISH NOTCL BUILD : Links for no_tcl library made ==="
	@echo

#
# Compile DERVISH library source
#
$(LIB)/libdervishnotcl.a : $(OBJECTS_NOTCL)
	@echo "=== DERVISH BUILD : Building Unix library (libdervishnotcl.a) ==="
	$(AR) $(ARFLAGS) $(LIB)/libdervishnotcl.a $(OBJECTS_NOTCL)
	-if [ "$(RANLIB)" != "" -a -x "$(RANLIB)" ]; then \
		$(RANLIB) $(LIB)/libdervishnotcl.a; \
	fi
	@echo "=== DERVISH BUILD : Unix library (libdervishnotcl.a) built ==="
	@echo


#
DISKIO_NOTCL_FILES =  $(INC)/region.h \
               $(INC)/shCArray.h \
               $(INC)/shCAlign.h \
               $(INC)/shCHdr.h \
               $(INC)/shCTbl.h \
               $(INC)/shCSchemaCnv.h \
               $(INC)/shCSchemaSupport.h \
               $(INC)/shCSchemaTrans.h \
               $(INC)/shChain.h 
#
diskio_gen_notcl.c : $(DISKIO_NOTCL_FILES)
	rm -f diskio_gen_notcl.c
	@echo "=== DERVISH BUILD : make_io started ==="
	$(BIN)/make_io -v1 -m Dervish -p sh diskio_gen_notcl.c $(DISKIO_NOTCL_FILES)
	@echo "=== DERVISH BUILD : make_io complete ==="
	chmod 444 diskio_gen_notcl.c
  
#
# Now standard targets
#
clean :
	@$(SDSSTOOLS_DIR)/bin/clean

install :
	@if [ "$(DERVISH_INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi
	@ rm -rf $(DERVISH_INSTALL_DIR)/src
	@ mkdir $(DERVISH_INSTALL_DIR)/src
	cp -pr Makefile README_NOTCL $(DERVISH_INSTALL_DIR)/src
	cp -pr *.[lycf] $(DERVISH_INSTALL_DIR)/src
	@chmod 644 $(DERVISH_INSTALL_DIR)/src/*.[lycf]
	@( \
		cd $(DERVISH_INSTALL_DIR)/src ;\
		find . \( -type d -name CVS -prune -o \
			-name Makefile -name \*~ \) -exec rm -rf {} \; \
	)
tags :
	/usr/local/bin/etags -at -f $(ETC)/TAGS *.[ch]
#
make :
	@if [ "$(CCENV)" = "" ]; then ccenv=`uname`; else ccenv=$(CCENV); fi; \
	echo \
	"$(SDSSTOOLS)/make_make -cc '$(CC) $(CFLAGS)' -nostd -file Makefile" \
							"-env $$ccenv *.c"; \
	 $(SDSSTOOLS)/make_make -cc '$(CC) $(CFLAGS)' -nostd -file Makefile \
							 -env $$ccenv *.c
