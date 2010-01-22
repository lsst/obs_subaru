# -*- python -*-
#
# Setup our environment
#
import glob, os.path, sys, traceback
import lsst.SConsUtils as scons

env = scons.makeEnv(
	"meas_deblender",
    r"$HeadURL:$",
    [
	["boost", "boost/version.hpp", "boost_system:C++"],
	["boost", "boost/test/unit_test.hpp", "boost_unit_test_framework:C++"],
	["python", "Python.h"],
	["m", "math.h", "m", "sqrt"],
	["wcslib", "wcslib/wcs.h", "wcs"],
	["pex_exceptions", "lsst/pex/exceptions.h", "pex_exceptions:C++"],
	["utils", "lsst/utils/Utils.h", "utils:C++"],
	["daf_base", "lsst/daf/base.h", "daf_base:C++"],
	["pex_logging", "lsst/pex/logging/Trace.h", "pex_logging:C++"],
	["security", "lsst/security/Security.h", "security:C++"],
	["pex_policy", "lsst/pex/policy/Policy.h", "pex_policy:C++"],
	["daf_persistence", "lsst/daf/persistence.h", "daf_persistence:C++"],
	["daf_data", "lsst/daf/data.h", "daf_data:C++"],
	["afw", "lsst/afw/image/MaskedImage.h", "afw"],
	["eigen", "Eigen/Core.h"],
	["gsl", "gsl/gsl_rng.h", "gslcblas gsl"],
	["minuit2", "Minuit2/FCNBase.h", "Minuit2:C++"],
	["meas_algorithms", "lsst/meas/algorithms.h", "meas_algorithms"],
	#["numpy", "Python.h numpy/arrayobject.h"], # see numpy workaround below
   ])

if True:
    #
    # Workaround SConsUtils failure to find numpy .h files. Fixed in sconsUtils >= 3.3.2
    #
    import numpy
    env.Append(CCFLAGS = ["-I", numpy.get_include()])

##################################################
# Libraries needed to link libraries/executables
##################################################
env.libs["meas_deblender"] += env.getlibs("pex_exceptions afw boost utils daf_base daf_data daf_persistence pex_logging pex_policy security")
# wcslib?

env.Append(CPPPATH = Dir("include/lsst/meas/deblender/photo"))
env.Append(CPPPATH = Dir("include/lsst/meas/deblender/dervish"))
env.Append(CPPPATH = Dir("include/lsst/meas/deblender/astrotools"))
env.Append(CCFLAGS = ['-DNOTCL'])
# for dervish/shGarbage.c
#env.Append(CCFLAGS = ['-DNDEBUG'])
# dervish/shGarbage.c fails to compile without this...
env.Append(CCFLAGS = ['-DCHECK_LEAKS=HELLYA'])


##################################################
# Build/install things
##################################################
for d in Split("src lib examples python/lsst/meas/deblender tests doc"):
    SConscript(os.path.join(d, "SConscript"))

env['IgnoreFiles'] = r"(~$|\.pyc$|^\.svn$|\.o$)"

#Alias("install", env.Install(env['prefix'], "python"))
Alias("install", env.Install(env['prefix'], "include"))
Alias("install", env.Install(env['prefix'], "lib"))
Alias("install", env.InstallEups(os.path.join(env['prefix'], "ups")))

scons.CleanTree(r"*~ core *.so *.os *.o *.pyc")

###################################################
# Build TAGS files
###################################################
#files = scons.filesToTag()
#if files:
#    env.Command("TAGS", files, "etags -o $TARGET $SOURCES")
#
env.Declare()
env.Help("""
LSST Deblender package
""")

