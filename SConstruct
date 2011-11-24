# -*- python -*-
from lsst.sconsUtils import scripts
env = scripts.BasicSConstruct("meas_deblender")



#import numpy
#env.Append(CCFLAGS = ["-I", numpy.get_include()])
env.Append(CPPPATH = Dir("include/lsst/meas/deblender/photo"))
env.Append(CPPPATH = Dir("include/lsst/meas/deblender/dervish"))
env.Append(CPPPATH = Dir("include/lsst/meas/deblender/astrotools"))
env.Append(CCFLAGS = ['-DNOTCL'])
## for dervish/shGarbage.c
##env.Append(CCFLAGS = ['-DNDEBUG'])
## dervish/shGarbage.c fails to compile without this...
env.Append(CCFLAGS = ['-DCHECK_LEAKS=HELLYA'])
