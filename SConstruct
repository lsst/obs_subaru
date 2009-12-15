# -*- python -*-
#
# Setup our environment
#
import glob, os.path, sys, traceback
import lsst.SConsUtils as scons

env = scons.makeEnv(
	"deblender",
    r"$HeadURL:$",
    [
	["boost", "boost/version.hpp", "boost_system:C++"],
	["boost", "boost/test/unit_test.hpp", "boost_unit_test_framework:C++"],
	["pex_exceptions", "lsst/pex/exceptions.h", "pex_exceptions:C++"],
	["utils", "lsst/utils/Utils.h", "utils:C++"],
	["daf_base", "lsst/daf/base.h", "daf_base:C++"],
	["pex_logging", "lsst/pex/logging/Trace.h", "pex_logging:C++"],
	["security", "lsst/security/Security.h", "security:C++"],
	["pex_policy", "lsst/pex/policy/Policy.h", "pex_policy:C++"],
	["daf_persistence", "lsst/daf/persistence.h", "daf_persistence:C++"],
	["daf_data", "lsst/daf/data.h", "daf_data:C++"],
	["afw", "lsst/afw/image/MaskedImage.h", "afw"],
    ],
	)

##################################################
# Libraries needed to link libraries/executables
##################################################
env.libs["deblender"] += env.getlibs("pex_exceptions afw boost utils daf_base daf_data daf_persistence pex_logging pex_policy security")

env.Append(CPPPATH = Dir("include/lsst/meas/deblender/photo"))

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
LSST Application Framework packages
""")

