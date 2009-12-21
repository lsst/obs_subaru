all: ../lib/libdeblender.dylib Deblender



INC := -I/usr/local/lsst-sandbox/DarwinX86/external/boost/1.37.0/include -I/usr/local/lsst-sandbox/DarwinX86/pex_exceptions/3.2.2/include -I/usr/local/lsst-sandbox/DarwinX86/utils/3.4.3/include -I/usr/local/lsst-sandbox/DarwinX86/daf_base/3.2.9/include -I/usr/local/lsst-sandbox/DarwinX86/pex_logging/3.3.4/include -I/usr/local/lsst-sandbox/DarwinX86/security/3.2.2/include -I/usr/local/lsst-sandbox/DarwinX86/pex_policy/3.4.1/include -I/usr/local/lsst-sandbox/DarwinX86/daf_persistence/3.3.7/include -I/usr/local/lsst-sandbox/DarwinX86/daf_data/3.2.3/include -I/usr/local/lsst-sandbox/DarwinX86/afw/3.3.18/include -I../include -I../include/lsst/meas/deblender/photo -I../include/lsst/meas/deblender/dervish -I../include/lsst/meas/deblender/astrotools

CFLAGS := -g -Wall -O0 -DLSST_HAVE_TR1=1 -DLSST_LITTLE_ENDIAN=1 -DNOTCL -fPIC

LIB := -L/usr/local/lsst-sandbox/DarwinX86/external/boost/1.37.0/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/pex_exceptions/3.2.2/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/utils/3.4.3/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/daf_base/3.2.9/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/pex_logging/3.3.4/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/security/3.2.2/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/pex_policy/3.4.1/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/daf_persistence/3.3.7/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/daf_data/3.2.3/lib
LIB +=  -L/usr/local/lsst-sandbox/DarwinX86/afw/3.3.18/lib
LIB +=  -L../lib
#LIB +=  -L.
LIB +=  -lpex_exceptions -lafw -lboost_system-xgcc40-mt 
LIB +=  -lboost_unit_test_framework-xgcc40-mt -lutils -ldaf_base -ldaf_data 
LIB +=  -ldaf_persistence -lpex_logging -lpex_policy -lsecurity

CPPOBJS := SDSSDeblender.os 

COBJS := photo/deblend.os \
	photo/framestat.os photo/sky.os photo/measureObj.os photo/brightObjects.os \
	photo/variablePsf.os photo/meschach.os photo/convolve.os photo/objc.os photo/object1.os \
	photo/phSpanUtil.os photo/finder.os photo/atlasImages.os photo/random-fake.os \
	photo/utils.os photo/offset.os photo/mathUtils.os photo/objectUtils.os \
	astrotools/atTrans.os astrotools/atExternalSet.os \
	dervish/shChain.os dervish/shGarbage.os dervish/schema.os dervish/utils.os dervish/errStack.os \
	dervish/diskio.os dervish/vecOps.os dervish/shHash.os dervish/region.os dervish/hdr.os dervish/tree.os \
	dervish/maskUtils.os dervish/fake-fitsio.os

OBJS := $(CPPOBJS) $(COBJS)

MYLIB := ../lib/libdeblender.dylib

$(MYLIB): $(OBJS)
	g++ -o $@ -dynamiclib -Wl,-install_name -Wl,libdeblender.dylib \
		$(OBJS) $(LIB)

#TAGS:
#	etags $(shell find src/ include/ -name *.c -o -name *.h -o -name *.cc -o -name *.no)x3

$(CPPOBJS): %.os: %.cc
	g++ -o $@ -c $(CFLAGS) $(INC) $<

$(COBJS): %.os: %.c
	gcc -o $@ -c $(CFLAGS) $(INC) $<

TOBJS := ../tests/Deblender.o

$(TOBJS): %.o: %.cc
	g++ -o $@ -c $(CFLAGS) $(INC) $<

TEXEC := Deblender

$(TEXEC): $(TOBJS) $(MYLIB)
	g++ -o $@ $(LIB) $^ -ldeblender


