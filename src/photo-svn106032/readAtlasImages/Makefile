SHELL = /bin/sh
#
.c.o :
	$(CC) -c $(CCCHK) $(CFLAGS) $*.c
#
CFLAGS  = $(SDSS_CFLAGS) -DCHECK_LEAKS -DSTAND_ALONE $(DERVINC) -I.
#
DERVINC	= -I$(DERVISH_DIR)/contrib -I$(FTCL_DIR)/include\
	-I$(FTCL_DIR)/src -I$(LIBFITS_DIR)/src\
	-I$(FPGPLOT_DIR)/include\
	-I$(PGPLOT_DIR)/dst -I$(DERVISH_DIR)/include\
	-I$(TCLX_DIR)/include -I$(TCLX_DIR)/src/src\
	-I$(TCL_DIR)/include -I$(TCL_DIR)/src
#DERVINC =

DERVLIB = \
	$(LIBSHV) $(LIBFITS) $(PGPLOTLIB) \
	$(CONTRIBLIB) \
	$(LIBFTCL) \
	$(LLIBS) $(PGLIB) $(FLIBS) $(ELIBS)
#DERVLIB = 

PROGS = read_atlas_image read_mask read_PSF
#
all : $(PROGS) tests
#
PHOTO_OBJ = atlasImages.o dataIo.o geometry.o phSpanUtil.o rice.o variablePsf.o random.o
OBJ = dervish.o read.o write.o $(PHOTO_OBJ)
#
libatlas.a : $(OBJ)
	$(AR) $(ARFLAGS) libatlas.a $?
	- if [ "$(RANLIB)" != "" -a -x "$(RANLIB)" ]; then \
		$(RANLIB) libatlas.a; \
	fi
#
read_atlas_image : main.o libatlas.a 
	$(CC) -o read_atlas_image main.o $(DERVLIB) libatlas.a $(LDFLAGS) -lm
read_mask : main_mask.o libatlas.a 
	$(CC) -o read_mask main_mask.o $(DERVLIB) libatlas.a $(LDFLAGS) -lm
read_PSF : main_PSF.o libatlas.a 
	$(CC) -o read_PSF main_PSF.o $(DERVLIB) libatlas.a $(LDFLAGS) -lm
geometry : libatlas.a 
	$(CC) -o geometry geometry.o $(DERVLIB) libatlas.a $(LDFLAGS)
#
hough : hough.o libatlas.a 
	$(CC) -o hough hough.o $(DERVLIB) libatlas.a $(LDFLAGS) -lm
#
# install libatlas.a
#
install : libatlas.a $(PROGS)
	@ if [ ! -d $(PHOTO_DIR)/lib ]; then \
	   mkdir $(PHOTO_DIR)/lib; \
	fi
	cp libatlas.a $(PHOTO_DIR)/lib
	-@if [ "$(RANLIB)" != "" -a -x "$(RANLIB)" ]; then \
		$(RANLIB) $(PHOTO_DIR)/lib/libatlas.a; \
	fi
	@chmod 444 $(PHOTO_DIR)/lib/libatlas.a

	@ if [ ! -d $(PHOTO_DIR)/bin ]; then \
	   mkdir $(PHOTO_DIR)/bin; \
	fi
	cp $(PROGS) $(PHOTO_DIR)/bin
	@(cd $(PHOTO_DIR)/bin; chmod 555 $(PROGS))
#
# Check that all the files in this directory that are copies are identical
# to the originals
#
tests :
	@ if [ -x ../bin/photo ]; then \
		unset PHOTO_USER; export PHOTO_USER; \
		unset DERVISH_USER; export DERVISH_USER; \
		if [ X$${SED_FLAGS} = X ]; then SED_FLAGS=-n; fi; \
		if [ X$${SED_FLAGS} = X-p ]; then SED_FLAGS=; fi; \
		export SED_FLAGS; \
		if ../bin/photo -file crc.tcl 2>&1; then \
			echo Passed all tests >&2; \
		fi | sed $${SED_FLAGS} -e 's/^TEST-ERR: */   /p' ; \
	  fi
#
clean :
	-/bin/rm -f *.a *.o *~ core $(PROGS) geometry
#
# Make the makefile
#
make :
	make_make -cc '$(CC) $(CFLAGS)' -nostd -file Makefile -env $$ccenv *.c
	fix-make_make Makefile
#
# Here are all the dependencies:
#
#START_DEPEND
atlasImages.o :	dervish.h
atlasImages.o :	phMeasureObj.h
atlasImages.o :	phOffset.h
atlasImages.o :	phRandom.h
dataIo.o :	dervish.h
dataIo.o :	phDataIo.h
dataIo.o :	phObjc.h
dataIo.o :	phSpanUtil.h
dervish.o :	dervish.h
dervish.o :	phConsts.h
geometry.o :	dervish.h
geometry.o :	phGeometry.h
geometry.o :	phSpanUtil.h
main.o :	dervish.h
main.o :	phConsts.h
main.o :	phFits.h
main_mask.o :	dervish.h
main_mask.o :	phConsts.h
main_mask.o :	phFits.h
main_mask.o :	phGeometry.h
main_mask.o :	phSpanUtil.h
main_PSF.o :	dervish.h
main_PSF.o :	phConsts.h
main_PSF.o :	phFits.h
main_PSF.o :	phVariablePsf.h
phSpanUtil.o :	dervish.h
phSpanUtil.o :	phConsts.h
phSpanUtil.o :	phSpanUtil.h
phSpanUtil.o :	phUtils.h
read.o :	dervish.h
read.o :	phDataIo.h
read.o :	phFits.h
read.o :	phVariablePsf.h
rice.o :	dervish.h
rice.o :	phRice.h
variablePsf.o :	atConversions.h
variablePsf.o :	dervish.h
variablePsf.o :	phConsts.h
variablePsf.o :	phMeschach.h
variablePsf.o :	phSkyUtils.h
variablePsf.o :	phUtils.h
variablePsf.o :	phVariablePsf.h
write.o :	phFits.h
