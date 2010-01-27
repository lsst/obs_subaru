all: ../lib/libmeas_deblender.dylib ../tests/Deblender ../python/lsst/meas/deblender/_deblenderLib.so

INC := \
	-I/usr/local/lsst-sandbox/DarwinX86/external/boost/1.37.0/include \
	-I/usr/local/lsst/DarwinX86/external/python/2.5.2/include/python2.5 \
	-I/usr/local/lsst/DarwinX86/external/python/2.5.2/include \
	-I/usr/local/lsst-sandbox/DarwinX86/external/wcslib/4.4.4/include \
	-I/usr/local/lsst-sandbox/DarwinX86/pex_exceptions/3.2.2/include \
	-I/usr/local/lsst-sandbox/DarwinX86/utils/3.4.3/include \
	-I/usr/local/lsst-sandbox/DarwinX86/daf_base/3.2.9/include \
	-I/usr/local/lsst-sandbox/DarwinX86/pex_logging/3.3.4/include \
	-I/usr/local/lsst-sandbox/DarwinX86/security/3.2.2/include \
	-I/usr/local/lsst-sandbox/DarwinX86/pex_policy/3.4.1/include \
	-I/usr/local/lsst-sandbox/DarwinX86/daf_persistence/3.3.7+1/include \
	-I/usr/local/lsst-sandbox/DarwinX86/daf_data/3.2.3+1/include \
	-I/usr/local/lsst-sandbox/DarwinX86/afw/3.3.21/include \
	-I/usr/local/lsst-sandbox/DarwinX86/external/eigen/2.0.0/include \
	-I/usr/local/lsst-sandbox/DarwinX86/external/gsl/1.8/include \
	-I/usr/local/lsst-sandbox/DarwinX86/external/minuit2/5.22.00+1/include \
	-I/usr/local/lsst-sandbox/DarwinX86/meas_algorithms/3.0.12/include \
	-I../include -I../include/lsst/meas/deblender/photo \
	-I../include/lsst/meas/deblender/dervish \
	-I../include/lsst/meas/deblender/astrotools

CFLAGS := -g -Wall -O0 -DNDEBUG -DCHECK_LEAKS=INDEED -DLSST_HAVE_TR1=1 -DLSST_LITTLE_ENDIAN=1 -DNOTCL -fPIC

LIB := -L/usr/local/lsst-sandbox/DarwinX86/external/boost/1.37.0/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/pex_exceptions/3.2.2/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/utils/3.4.3/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/daf_base/3.2.9/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/pex_logging/3.3.4/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/security/3.2.2/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/pex_policy/3.4.1/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/daf_persistence/3.3.7+1/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/daf_data/3.2.3+1/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/afw/3.3.18/lib
LIB +=  -L../lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/external/wcslib/4.2+3/lib
#LIB +=  -L.
LIB +=  -lpex_exceptions -lafw -lboost_system-xgcc40-mt 
LIB +=  -lboost_unit_test_framework-xgcc40-mt -lutils -ldaf_base -ldaf_data 
LIB +=  -ldaf_persistence -lpex_logging -lpex_policy -lsecurity

LIB_MEAS_ALG := -L/usr/local/lsst-sandbox/DarwinX86/meas_algorithms/3.0.12/lib -lmeas_algorithms

CPPOBJS := SDSSDeblender.os 

COBJS := photo/deblend.os \
	photo/framestat.os photo/sky.os photo/measureObj.os photo/brightObjects.os \
	photo/variablePsf.os photo/meschach.os photo/convolve.os photo/objc.os photo/object1.os \
	photo/phSpanUtil.os photo/finder.os photo/atlasImages.os photo/random-fake.os \
	photo/utils.os photo/offset.os photo/mathUtils.os photo/objectUtils.os photo/astcen.os \
	photo/medianSky.os photo/dgpsf.os photo/apertures.os photo/extract.os \
	photo/extract_utils.os photo/fake.os \
	astrotools/atTrans.os astrotools/atExternalSet.os \
	dervish/shChain.os dervish/shGarbage.os dervish/schema.os dervish/utils.os dervish/errStack.os \
	dervish/diskio.os dervish/vecOps.os dervish/shHash.os dervish/region.os dervish/hdr.os dervish/tree.os \
	dervish/maskUtils.os dervish/fake-fitsio.os

OBJS := $(CPPOBJS) $(COBJS)

SDSSDeblender.os: ../include/lsst/meas/deblender/deblender.h
../tests/Deblender.o: ../include/lsst/meas/deblender/deblender.h

$(OBJS): ../include/lsst/meas/deblender/photo/phFake.h

MYLIB := ../lib/libmeas_deblender.dylib

$(MYLIB): $(OBJS)
	g++ -o $@ -dynamiclib -Wl,-install_name -Wl,libmeas_deblender.dylib \
		$(OBJS) $(LIB)

#TAGS:
#	etags $(shell find src/ include/ -name *.c -o -name *.h -o -name *.cc -o -name *.no)

$(CPPOBJS): %.os: %.cc
	g++ -o $@ -c $(CFLAGS) $(INC) $<

$(COBJS): %.os: %.c
	gcc -o $@ -c $(CFLAGS) $(INC) $<

TOBJS := ../tests/Deblender.o

$(TOBJS): %.o: %.cc
	g++ -o $@ -c $(CFLAGS) $(INC) $<

TEXEC := ../tests/Deblender

$(TEXEC): $(MYLIB)

$(TEXEC): $(TOBJS)
	g++ -o $@ $(LIB) $(TOBJS) -lmeas_deblender $(LIB_MEAS_ALG)

WRAP_OBJS := ../python/lsst/meas/deblender/deblenderLib_wrap.os
WRAP_CPPS := $(addsuffix .cc,$(basename $(WRAP_OBJS)))

SWIG_INC := \
	-I/usr/local/lsst-sandbox/DarwinX86/utils/3.4.3/python \
	-I/usr/local/lsst-sandbox/DarwinX86/afw/3.3.21/python \
	-I/usr/local/lsst-sandbox/DarwinX86/daf_base/3.2.9/python \
	-I/usr/local/lsst-sandbox/DarwinX86/daf_data/3.2.3+1/python \
	-I/usr/local/lsst-sandbox/DarwinX86/daf_persistence/3.3.7+1/python \
	-I/usr/local/lsst-sandbox/DarwinX86/pex_logging/3.3.4/python \
	-I/usr/local/lsst-sandbox/DarwinX86/pex_policy/3.4.1/python \
	-I/usr/local/lsst-sandbox/DarwinX86/pex_exceptions/3.2.2/python \
	-I/usr/local/lsst-sandbox/DarwinX86/security/3.2.2/python \
	-I/usr/local/lsst-sandbox/DarwinX86/external/swig/1.3.36+2/share/swig/1.3.36/python \
	-I/usr/local/lsst-sandbox//DarwinX86/external/swig/1.3.36+2/share/swig/1.3.36

$(WRAP_OBJS): %.os: %.cc
	g++ -o $@ -c $(CFLAGS) $(INC) $(SWIG_INC) $<

$(WRAP_CPPS): %_wrap.cc: %.i
	swig -c++ -python -o $@ $(INC) $(SWIG_INC) $<

$(WRAP_CPPS): ../include/lsst/meas/deblender/deblender.h

WRAP_LIBS := ../python/lsst/meas/deblender/_deblenderLib.so

SWIG_LIB := \
	-L/usr/local/lsst/DarwinX86/external/python/2.5.2/lib/python2.5/config \
	-L/usr/local/lsst/DarwinX86/external/tcltk/8.5a4/lib \
	-L/usr/X11R6/lib \
	-lmeas_deblender

$(WRAP_LIBS): $(WRAP_OBJS)
	g++ -o $@ -bundle -undefined suppress -flat_namespace $< $(LIB) $(SWIG_LIB)

#debug:
#	echo WRAP_OBJS $(WRAP_OBJS)
#	echo WRAP_CPPS $(WRAP_CPPS)


# Cancel implicit rules
.SUFFIXES:
SUFFIXES:
%: %.o
%: %.c
%: %.cc
%: %.C
%: %.cpp
%: %.p
%: %.f
%: %.F
%: %.m
%: %.r
%: %.s
%: %.S
%: %.mod
%: %.sh
%: RCS/%,v
%: RCS/%
%: s.%
%: %,v
%: SCCS/s.%

%.c: %.y
%.c: %.l
%.c: %.w
%.c: %.w %.ch

%.tex: %.w %.ch
%.tex: %.w
