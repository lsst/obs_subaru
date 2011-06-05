# -*- python -*-
#
# Setup our environment
#
import glob, os.path, sys, traceback
import lsst.SConsUtils as scons

# boilerplate copied from meas_algorithms/SConstruct

try:
    scons.ConfigureDependentProducts
except AttributeError:
    import lsst.afw.SconsUtils
    scons.ConfigureDependentProducts = lsst.afw.SconsUtils.ConfigureDependentProducts

me = 'meas_deblender'

env = scons.makeEnv(me,
                    r'$HeadURL$',
                    scons.ConfigureDependentProducts(me))

# I copied this list from meas_alg also.
env.libs[me] += env.getlibs('daf_base daf_data daf_persistence pex_logging pex_exceptions ' +
                            'pex_policy security ndarray afw boost minuit2 utils wcslib' +
                            'eigen gsl meas_algorithms')
if True:
    #
    # Workaround SConsUtils failure to find numpy .h files. Fixed in sconsUtils >= 3.3.2
    #
    import numpy
    env.Append(CCFLAGS = ["-I", numpy.get_include()])
#
# Build/install things
#
for d in Split("doc examples lib src tests") + \
        Split("include/lsst/meas/deblend python/lsst/meas/deblender"):
    SConscript(os.path.join(d, "SConscript"))
    
env['IgnoreFiles'] = r"(~$|\.pyc$|^\.svn$|\.o$)"

Alias("install", [
    env.Install(env['prefix'], "doc"),
    env.Install(env['prefix'], "etc"),
    env.Install(env['prefix'], "examples"),
    env.Install(env['prefix'], "include"),
    env.Install(env['prefix'], "lib"),
    env.Install(env['prefix'], "policy"),
    env.Install(env['prefix'], "python"),
    env.Install(env['prefix'], "src"),
    env.Install(env['prefix'], "tests"),
    env.InstallEups(os.path.join(env['prefix'], "ups")),
])

scons.CleanTree(r"*~ core *.so *.os *.o")
#
# Build TAGS files
#
files = scons.filesToTag()
if files:
    env.Command("TAGS", files, "etags -o $TARGET $SOURCES")

env.Declare()
env.Help("""
LSST FrameWork packages
""")


env.Append(CPPPATH = Dir("include/lsst/meas/deblender/photo"))
env.Append(CPPPATH = Dir("include/lsst/meas/deblender/dervish"))
env.Append(CPPPATH = Dir("include/lsst/meas/deblender/astrotools"))
env.Append(CCFLAGS = ['-DNOTCL'])
# for dervish/shGarbage.c
#env.Append(CCFLAGS = ['-DNDEBUG'])
# dervish/shGarbage.c fails to compile without this...
env.Append(CCFLAGS = ['-DCHECK_LEAKS=HELLYA'])


